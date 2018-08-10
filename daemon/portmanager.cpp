/*
* Copyright (c) 2009-2018, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/
//!
//! \file       portmanager.cpp
//! \brief
//!

#include <list>
#include <new>
#include <vector>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sched.h>
#include <fcntl.h>

#include "portmanager.h"
#include "hdcpdef.h"
#include "hdcpapi.h"
#include "port.h"
#include "daemon.h"
#include "srm.h"

#include "xf86drm.h"
#include "xf86drmMode.h"

#define UEVENT_MSG_SIZE             1024

#define UEVENT_MSG_PART1            1
#define UEVENT_MSG_PART3            3
#define UEVENT_MSG_PART4            4
#define UEVENT_MSG_PART7            7

#define UEVENT_MSG_STR_CHANGE       "ACTION=change"
#define UEVENT_MSG_STR_CARD         "DEVNAME=dri/card0"
#define UEVENT_MSG_STR_HOTPLUG      "HOTPLUG=1"
#define UEVENT_MSG_STR_BACKLIGHT    "SUBSYSTEM=backlight"
#define UEVENT_MSG_STR_S0           "GSTATE=0"
#define UEVENT_MSG_STR_S3           "GSTATE=3"

#define MAX_MSG_STRS                16
#define HDCPD_NUM_AUTH_RETRIES      3

// Local static instance of the PortManager that this module accesses
static PortManager          *portMgr = nullptr;
static pthread_t            uEventThread;
static pthread_t            integrityCheckThread;

static int32_t              eventSocket = -1;

static bool                 isDestroyThreads = false;

static pthread_barrier_t    createThreadBarrier;

///////////////////////////////////////////////////////////////////////////////
/// \brief  Exit the thread when receiving the SIGUSR1 signal
///
/// \param[in]  sig,    Action signal being received
/// \return     void
///
/// This is used as a method to kill the AuthStep3 and HotPlug threads
///////////////////////////////////////////////////////////////////////////////
static void ExitThread(int32_t sig)
{
    HDCP_FUNCTION_ENTER;

    if (SIGUSR1 == sig)
    {
        // Don't do anything.
        // We use this to break a blocking Get call on a socket so that we can
        // properly detect the async interrupt and exit cleanly.
    }
    else
    {
        HDCP_ASSERTMESSAGE("Received unknown signal %d", sig);
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief  Wrapper for the port manager's Auth Step 3 work
///
/// \return     Nothing, but pthread requires pointer, so nullptr
///////////////////////////////////////////////////////////////////////////////
static void *InitPeriodicIntegrityCheck(void *data)
{
    HDCP_FUNCTION_ENTER;

    struct sigaction actions = {};

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = ExitThread;
    sigaction(SIGUSR1, &actions, nullptr);

    pthread_barrier_wait(&createThreadBarrier);
    
    HDCP_NORMALMESSAGE("Periodic link integrity thread is active");

    while (true)
    {
        // Declare local variable to calm the KW violation..
        bool localIsDestroyThreads = isDestroyThreads;
        if (localIsDestroyThreads)
        {
            HDCP_NORMALMESSAGE("Integrity check thread is being destroyed");
            break;
        }

        portMgr->CheckIntegrity();

        // Only check integrity periodically 
        SLEEP_MSEC(INTEGRITY_CHECK_DELAY_MS);
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief  Wrapper for the port manager's UEvent handler thread
///
/// \return     Nothing, but pthread requires pointer, so nullptr
///////////////////////////////////////////////////////////////////////////////
static void *InitUEventHandler(void *data)
{
    HDCP_FUNCTION_ENTER;

    int32_t             ret                     = EINVAL;
    int32_t             bytesReceived           = -1;
    char                buf[UEVENT_MSG_SIZE]    = {};
    const uint32_t      sockOptBufferSize       = sizeof(buf);
    struct sockaddr_nl  snl                     = {};
    struct sigaction    actions                 = {};
    
    size_t                      index           = 0;
    size_t                      nextTerminator  = 0;
    std::string                 msg;
    std::vector<std::string>    messages;

    HDCP_NORMALMESSAGE("UEvent message handling thread is active");

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = ExitThread;
    sigaction(SIGUSR1, &actions, nullptr);
    
    pthread_barrier_wait(&createThreadBarrier);

    // set the socket address
    snl.nl_family = AF_NETLINK;
    snl.nl_pad    = 0;
    snl.nl_pid    = getpid();
    snl.nl_groups = 1; // receive multicast

    // create the netlink socket for receiving uevent from kernel
    eventSocket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (ERROR == eventSocket)
    {
        HDCP_ASSERTMESSAGE("Init Socket Failed");
        return nullptr;
    }

    // set receive buffer size
    ret = setsockopt(
        eventSocket,
        SOL_SOCKET,
        SO_RCVBUF,
        &sockOptBufferSize,
        sizeof(sockOptBufferSize));
    if (ERROR == ret)
    {
        HDCP_ASSERTMESSAGE(
                "Failed to set the socket options. Err: %s",
                strerror(errno));
        return nullptr;
    }

    ret = bind(
            eventSocket,
            reinterpret_cast<struct sockaddr*>(&snl),
            sizeof(snl));
    if (ERROR == ret)
    {
        HDCP_ASSERTMESSAGE("Bind Socket Failed. Err: %s", strerror(errno));
        return nullptr;
    }

    while (true)
    {
        messages.clear();
        memset(buf, 0, UEVENT_MSG_SIZE);

        bytesReceived = recv(eventSocket,
                            &buf,
                            sizeof(buf),
                            0);

        if (bytesReceived <= 0)
        {
            HDCP_ASSERTMESSAGE("Failed to recv on UEvent socket");
            continue;
        }

        // Declare local variable to calm the KW violation..
        bool localIsDestroyThreads = isDestroyThreads;
        if (localIsDestroyThreads)
        {
            HDCP_NORMALMESSAGE("UEvent thread is being destroyed");
            break;
        }

        buf[UEVENT_MSG_SIZE - 1] = 0;
        msg = std::string(buf, UEVENT_MSG_SIZE);

        // The message is filled with a bunch of nullptr separated substrings,
        // we need to cut them into individual strings for parsing.
        index = 0;
        do
        {
            nextTerminator = msg.find('\0', index);
            if (std::string::npos == nextTerminator)
            {
                break;
            }

            messages.push_back(msg.substr(index, nextTerminator - index));
            index = nextTerminator + 1;
        } while ((index < UEVENT_MSG_SIZE) &&
                (messages.size() <= MAX_MSG_STRS));

        if (messages.size() < 8)
        {
            continue;
        }

        // Check for HotPlug messages from the drm subsystem
        // ACTION=change/HOTPLUG=1/DEVNAME=dri/card0
        if ((messages[UEVENT_MSG_PART1] == UEVENT_MSG_STR_CHANGE)   &&
            (messages[UEVENT_MSG_PART4] == UEVENT_MSG_STR_HOTPLUG)  &&
            (messages[UEVENT_MSG_PART7] == UEVENT_MSG_STR_CARD))
        {
            HDCP_NORMALMESSAGE("Detected hotplug event");
            PortManagerProcessHotPlug();
        }

        // ACTION=change/GSTATE=0
        else if ((messages[UEVENT_MSG_PART1] == UEVENT_MSG_STR_CHANGE)  &&
                 (messages[UEVENT_MSG_PART4] == UEVENT_MSG_STR_S0))
        {
            HDCP_NORMALMESSAGE("Detected power state 0 event");
            // Not implemented yet, this is staged for a Netflix WA,
            // but not needed for now.
        }

        // ACTION=change/GSTATE=3 
        else if ((messages[UEVENT_MSG_PART1] == UEVENT_MSG_STR_CHANGE)  &&
                 (messages[UEVENT_MSG_PART4] == UEVENT_MSG_STR_S3))
        {
            HDCP_NORMALMESSAGE("Detected power state 3 event");
            // Power change not verified so far
        }
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return nullptr;
}

int32_t PortManagerInit(HdcpDaemon& socket)
{
    HDCP_FUNCTION_ENTER;

    if (nullptr != portMgr)
    {
        HDCP_ASSERTMESSAGE("Attempting to initialize a non-null Port Manager!");
        return EEXIST;
    }

    portMgr = new (std::nothrow) PortManager(socket);
    if (nullptr == portMgr)
    {
        HDCP_ASSERTMESSAGE("Failed to allocate the port manager!");
        return ENOMEM;
    }

    if (!portMgr->IsValid())
    {
        delete portMgr;
        portMgr = nullptr;
        return ENODEV;
    }

    pthread_barrier_wait(&createThreadBarrier);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

void PortManagerRelease(void)
{
    HDCP_FUNCTION_ENTER;

    delete portMgr;
    portMgr = nullptr;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void PortManagerProcessHotPlug()
{
    HDCP_FUNCTION_ENTER;

    portMgr->ProcessHotPlug();

    HDCP_FUNCTION_EXIT(SUCCESS);

}

int32_t PortManagerEnumeratePorts(
                        Port (&portList)[NUM_PHYSICAL_PORTS_MAX],
                        uint32_t& portCount)
{
    HDCP_FUNCTION_ENTER;

    // PortList should already be checked against nullptr in HdcpDaemon, don't
    // need to do it again here.

    CHECK_PARAM_NULL(portMgr, ENODEV);

    int32_t ret = portMgr->EnumeratePorts(portList, portCount);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t PortManagerEnablePort(
                        const uint32_t portId,
                        const uint32_t appId,
                        const uint8_t level)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(portMgr, ENODEV);

    int32_t ret = portMgr->EnablePort(portId, appId, level);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t PortManagerDisablePort(const uint32_t portId, const uint32_t appId)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(portMgr, ENODEV);

    int32_t ret = portMgr->DisablePort(portId, appId);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t PortManagerGetStatus(
                        const uint32_t portId,
                        PORT_STATUS *portStatus)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(portStatus, EINVAL);
    CHECK_PARAM_NULL(portMgr, ENODEV);
    
    int32_t ret = portMgr->GetStatus(portId, portStatus);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t PortManagerGetKsvList(
                        const uint32_t portId,
                        uint8_t *ksvCount,
                        uint8_t *depth,
                        uint8_t *ksvList)
{
    CHECK_PARAM_NULL(ksvCount, EINVAL); 
    CHECK_PARAM_NULL(depth, EINVAL); 
    CHECK_PARAM_NULL(ksvList, EINVAL); 
    CHECK_PARAM_NULL(portMgr, ENODEV);

    uint32_t ret = portMgr->GetKsvList(portId, ksvCount, depth, ksvList);
    return ret;
}

int32_t PortManagerSendSRMDdata(const uint8_t *data, const uint32_t size)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(portMgr, ENODEV);

    int32_t ret = portMgr->SendSRMData(data, size);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

void PortManagerHandleAppExit(const uint32_t appId)
{
    HDCP_FUNCTION_ENTER;

    if (nullptr != portMgr)
    {
        portMgr->RemoveAppFromPorts(appId);
    }
    else
    {
        HDCP_ASSERTMESSAGE("Attempting to use an uninitialized PortManager!");
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void PortManagerDisableAllPorts()
{
    HDCP_FUNCTION_ENTER;

    if (nullptr != portMgr)
    {
        portMgr->DisableAllPorts(); 
    }
    else
    {
        HDCP_ASSERTMESSAGE("Attempting to use an uninitialized PortManager!");
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void PortManager::SigCatcher(int sig)
{
    if (SIGTERM == sig || SIGINT == sig || SIGKILL == sig)
    {
        PortManagerDisableAllPorts();
    }
    exit(SUCCESS);
}

PortManager::PortManager(HdcpDaemon& daemonSocket) :
                m_DaemonSocket(daemonSocket)
{
    HDCP_FUNCTION_ENTER;

    // Set default state
    m_IsValid = false;

    m_DrmFd = drmOpen("i915", nullptr);
    if (m_DrmFd < 0)
    {
        HDCP_ASSERTMESSAGE("Failed to open i915 device!");
        return;
    }
    
    if (SUCCESS != InitDrmObjects())
    {
        HDCP_ASSERTMESSAGE("Failed to initialize m_DrmObjects");
        return;
    }

    if (SUCCESS != pthread_barrier_init(&createThreadBarrier, nullptr, 3))
    {
        HDCP_ASSERTMESSAGE("Failed to initialize barrier");
        return;
    }

    int32_t sts = pthread_create(
                &integrityCheckThread,
                nullptr,
                InitPeriodicIntegrityCheck,
                nullptr);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("Failed to create integrity check thread!");
        return;
    }

    sts = pthread_create(
                &uEventThread,
                nullptr,
                InitUEventHandler,
                nullptr);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("Failed to create UEVENT thread!");
        return;
    }

    // Regster terminate signal hanler, to destory all enabled ports
    struct sigaction actions = {};

    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = SigCatcher;
    sigaction(SIGTERM, &actions, nullptr);
    sigaction(SIGINT, &actions, nullptr);
    sigaction(SIGKILL, &actions, nullptr);

    m_IsValid = true;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

PortManager::~PortManager(void)
{
    HDCP_FUNCTION_ENTER;

    if (!(m_DrmFd < 0))
        drmClose(m_DrmFd);

    isDestroyThreads = true;

    pthread_join(integrityCheckThread, nullptr);
    HDCP_NORMALMESSAGE("Destroyed Periodic Integrity Check thread");
    
    // Sending an async signal should interrupt the recv call.
    // There is a bit of an issue with deadlocking if the thread
    // is not actually blocked on the recv call. So give if we
    // fail at first, give it time to resolve then try again.
    pthread_kill(uEventThread, SIGUSR1);
    SLEEP_MSEC(20);
    pthread_kill(uEventThread, SIGUSR1);
    SLEEP_MSEC(20);
    pthread_kill(uEventThread, SIGUSR1);
    pthread_join(uEventThread, nullptr);
    HDCP_NORMALMESSAGE("Destroyed UEvent Handler thread");

    if (-1 != eventSocket)
    {
        HDCP_NORMALMESSAGE("Close NetLink UEvent Socket");
        close(eventSocket);
        eventSocket = -1;
    }

    //Free m_DrmObjects
    for (auto drmObject : m_DrmObjects)
        delete drmObject;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

bool PortManager::IsValid(void)
{
    return m_IsValid;
}

int32_t PortManager::InitDrmObjects()
{
    HDCP_FUNCTION_ENTER;

    //Get connnector resource
    drmModeRes *res = drmModeGetResources(m_DrmFd);
    if (nullptr == res)
    {
        HDCP_WARNMESSAGE("Could not get resource");
        return EBUSY;
    }

    uint32_t portIdx = 0;

    for (int32_t i = 0; i < res->count_connectors; i++)
    {
        auto properties = drmModeObjectGetProperties(
                                m_DrmFd,
                                res->connectors[i],
                                DRM_MODE_OBJECT_CONNECTOR);
        if (nullptr == properties)
        {
            HDCP_WARNMESSAGE("Could not get properties");
            continue;
        }
        
        for (uint32_t j = 0; j < properties->count_props; j++)
        {
            auto property = drmModeGetProperty(m_DrmFd, properties->props[j]); 
            if (nullptr == property)
            {
                HDCP_WARNMESSAGE("Could not get property");
                continue;
            }

            // Only generate DrmObject for connectors with CP property
            std::string prop_name = property->name;

            if (CONTENT_PROTECTION == prop_name ||
                CP_CONTENT_TYPE == prop_name    ||
                CP_SRM == prop_name             ||
                CP_DOWNSTREAM_INFO == prop_name)
            {
                DrmObject *drmObject = GetDrmObjectByDrmId(res->connectors[i]);
                if (nullptr == drmObject)
                {
                    drmObject = new (std::nothrow) DrmObject(
                                                        res->connectors[i],
                                                        portIdx++); 
                    m_DrmObjects.push_back(drmObject);
                }

                drmObject->AddDrmProperty(
                                    prop_name,
                                    properties->props[j],
                                    properties->prop_values[j]);
            }

            drmModeFreeProperty(property);
        }

        drmModeFreeObjectProperties(properties);
    }

    drmModeFreeResources(res);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t PortManager::EnumeratePorts(
                    Port (&portList)[NUM_PHYSICAL_PORTS_MAX],
                    uint32_t& portCount)
{
    HDCP_FUNCTION_ENTER;

    // Initialize the PortList to "all disconnected"
    for (uint32_t i = 0; i < NUM_PHYSICAL_PORTS_MAX; ++i)
    {
        portList[i].Id = 0;
        portList[i].status = PORT_STATUS_DISCONNECTED;
    }

    portCount = 0;

    // All public members check proper initialization
    if (!m_IsValid)
    {
        HDCP_ASSERTMESSAGE("Failed to initialize Portmanager");
        return ENODEV;
    }

    // Only return the connectted ports 
    for (auto drmObject : m_DrmObjects)
    {
        drmObject->ConnAtomicBegin();

        auto connector = drmModeGetConnector(m_DrmFd, drmObject->GetDrmId());
        if (nullptr == connector)
        {
            drmObject->ConnAtomicEnd();
            HDCP_ASSERTMESSAGE("Failed to get connector");
            return ENOENT;
        }

        if (connector->connection == DRM_MODE_CONNECTED)
        {
            portList[portCount].Id = drmObject->GetPortId();
            portList[portCount].status = PORT_STATUS_CONNECTED;
            portCount++;
        }

        drmObject->SetConnection(connector->connection);
        drmModeFreeConnector(connector);

        drmObject->ConnAtomicEnd();
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t PortManager::EnablePort(
                        const uint32_t portId,
                        const uint32_t appId,
                        const uint8_t level)
{
    HDCP_FUNCTION_ENTER;

    DrmObject *drmObject = GetDrmObjectByPortId(portId); 
    if (nullptr == drmObject ||
        DRM_MODE_DISCONNECTED == drmObject->GetConnection())
    {
        HDCP_ASSERTMESSAGE("Port %d is invalid", portId);
        return ENOENT;
    }

    bool type1Capable =
    (UINT32_MAX != drmObject->GetPropertyId(CP_CONTENT_TYPE));

    if (!type1Capable)
    {
        if (HDCP_LEVEL2 == level)
        {
            HDCP_ASSERTMESSAGE("Level %d is not supported", level);
            return EINVAL;
        }
    }

    uint8_t cpType  = CP_VALUE_INVALID;
    int32_t ret     = EINVAL;
    
    // Check if the port is already enabled, do not enable it any more
    // if level == 1, then content type == 0 or 1 means enabled
    // if level == 2, then content type == 1 means enabled,
    //                     content type == 0 means upgrade to type 1.
    uint8_t currCpType = drmObject->GetCpType();
    if (CP_TYPE_INVALID != currCpType && (uint32_t)(level - 1) <= currCpType)
    {
        drmObject->AddRefAppId(appId);
        HDCP_NORMALMESSAGE("Port with id %d is already enabled", portId);
        return SUCCESS;
    }

    // Only set Cp_Content_Type property when the port is HDCP type1 capable  
    if (type1Capable)
    {
        // Translate level to cpType
        switch (level)
        {
            case HDCP_LEVEL1: cpType = CP_TYPE_0; break;
            case HDCP_LEVEL2: cpType = CP_TYPE_1; break;
            default : break;
        }

        // Set CP_Content_Type property, try at most AUTH_NUM_RETRY times
        ret = SetPortProperty(
                        drmObject->GetDrmId(),
                        drmObject->GetPropertyId(CP_CONTENT_TYPE),
                        sizeof(uint8_t),
                        &cpType,
                        AUTH_NUM_RETRY);
        if (SUCCESS != ret)
        {
            HDCP_ASSERTMESSAGE(
                        "Failed to enable port with id %d,"
                        "set content_type property faild",
                        portId);
            return EBUSY;
        }
    }

    // The port hasn't been enabled so far
    // Set Content Protection property, try at most AUTH_NUM_RETRY times
    uint8_t cpValue = CP_DESIRED;
    ret = SetPortProperty(
                    drmObject->GetDrmId(),
                    drmObject->GetPropertyId(CONTENT_PROTECTION),
                    sizeof(uint8_t),
                    &cpValue,
                    AUTH_NUM_RETRY);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to enable port with id %d, set property faild",
                    portId);
        return EBUSY;
    }

    // Wait for the authentication complete
    SLEEP_MSEC(AUTH_CHECK_DELAY_MS);

    // Check whether Content Protection property is CP_ENABLED or not 
    drmObject->CpTypeAtomicBegin();
    ret = GetProtectionInfo(drmObject, &cpValue, &cpType);
    if (SUCCESS != ret) 
    {
        drmObject->CpTypeAtomicEnd();
        HDCP_ASSERTMESSAGE("Failed to get protection info");
        return EBUSY;
    }

    if (cpValue != CP_ENABLED)
    {
        drmObject->CpTypeAtomicEnd();
        HDCP_ASSERTMESSAGE(
                    "Failed to enable port with id %d, check proerty failed",
                    portId);
        return EBUSY;
    }

    drmObject->SetCpType(cpType);
    drmObject->AddRefAppId(appId);
    drmObject->CpTypeAtomicEnd();

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t PortManager::DisablePort(const uint32_t portId, const uint32_t appId)
{
    HDCP_FUNCTION_ENTER;

    // If the port isn't in our list, it is definitely not enabled.
    // No need to call disable on it, just return success.
    DrmObject *drmObject = GetDrmObjectByPortId(portId); 
    if (nullptr == drmObject                                ||
        drmObject->GetConnection() == DRM_MODE_DISCONNECTED)
    {
        HDCP_NORMALMESSAGE(
                "Port %d is invalid, but harmless when disabling..",
                portId);
        return SUCCESS;
    }

    // We should not disable the port if other session still use it,
    // just remove current appId from the m_AppIds list
    drmObject->RemoveRefAppId(appId);
    if (drmObject->GetRefAppCount() >= 1)
    {
        HDCP_NORMALMESSAGE(
                    "Port %d is in use by other app,"
                    "romove app Id %d from appId list",
                    drmObject->GetPortId(),
                    appId);

        return SUCCESS;
    }

    //Disable the port
    uint8_t cpValue = CP_OFF;
    int32_t ret = SetPortProperty(
                    drmObject->GetDrmId(),
                    drmObject->GetPropertyId(CONTENT_PROTECTION),
                    sizeof(uint8_t),
                    &cpValue,
                    1);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to enable port with id %d, set property faild",
                    portId);
        return EBUSY;
    }

    // Check whether Content Protection property is CP_OFF or not 
    drmObject->CpTypeAtomicBegin();
    uint8_t cpType = CP_TYPE_INVALID;
    ret = GetProtectionInfo(drmObject, &cpValue, &cpType);
    if (SUCCESS != ret) 
    {
        drmObject->CpTypeAtomicEnd();
        HDCP_ASSERTMESSAGE("Failed to get protection info");
        return EBUSY;
    }

    if (CP_OFF != cpValue)
    {
        drmObject->CpTypeAtomicEnd();
        HDCP_ASSERTMESSAGE(
                    "Failed to disable port with id %d, check property failed",
                    portId);
        return EBUSY;
    }

    drmObject->SetCpType(CP_TYPE_INVALID);
    drmObject->CpTypeAtomicEnd();

    HDCP_NORMALMESSAGE("Success to disable port with id %d", portId);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t PortManager::GetStatus(const uint32_t portId, PORT_STATUS *portStatus)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(portStatus, EINVAL);

    DrmObject *drmObject = GetDrmObjectByPortId(portId); 
    if (nullptr == drmObject)
    {
        return ENOENT;
    }

    // Firstly check if this port is connected
    auto connector = drmModeGetConnector(m_DrmFd, drmObject->GetDrmId());
    if (nullptr == connector)
    {
        HDCP_ASSERTMESSAGE("Failed to get connector");
        return ENOENT;
    }
    
    if (DRM_MODE_DISCONNECTED == connector->connection) 
    {
        *portStatus = PORT_STATUS_DISCONNECTED;
        drmModeFreeConnector(connector);
        return SUCCESS;
    }

    *portStatus = PORT_STATUS_CONNECTED;

    // Then check if this port is HDCP enabled
    uint8_t cpValue = CP_VALUE_INVALID;
    uint8_t cpType = CP_TYPE_INVALID;
    int32_t ret = GetProtectionInfo(drmObject, &cpValue, &cpType);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Failed to get protection info");
        drmModeFreeConnector(connector);
        return EBUSY;
    }

    if (CP_ENABLED == cpValue)
    {
        switch (cpType)
        {
            case CP_TYPE_0 :
                *portStatus |= PORT_STATUS_HDCP_TYPE0_ENABLED;
                break;
            case CP_TYPE_1 :
                *portStatus |= PORT_STATUS_HDCP_TYPE1_ENABLED;
                break;
            default:
                break;
        }
    }
    
    drmModeFreeConnector(connector);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t PortManager::GetKsvList(
                        const uint32_t portId,
                        uint8_t *ksvCount,
                        uint8_t *depth,
                        uint8_t *ksvList)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(ksvCount, EINVAL);
    CHECK_PARAM_NULL(depth, EINVAL);
    CHECK_PARAM_NULL(ksvList, EINVAL);

    DownstreamInfo dsInfo;
    int32_t ret = GetDownstreamInfo(
                            GetDrmObjectByPortId(portId),
                            (uint8_t *)&dsInfo);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Faild to get down stream info");
        return EBUSY;
    }

    *depth = dsInfo.depth + 1;
    *ksvCount = dsInfo.deviceCount + 1;

    HDCP_NORMALMESSAGE(
                "Downstream Info : device count %d depth %d",
                dsInfo.deviceCount,
                dsInfo.depth);

    memmove(ksvList, dsInfo.bksv, KSV_SIZE);
    memmove(ksvList + KSV_SIZE, dsInfo.ksvList, (*ksvCount) * KSV_SIZE);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t PortManager::SendSRMData(const uint8_t *data, const uint32_t size)
{
    HDCP_FUNCTION_ENTER;

    for (auto drmObject : m_DrmObjects)
    {
        int32_t ret = SetPortProperty(drmObject->GetDrmId(),
                        drmObject->GetPropertyId(CP_SRM),
                        size,
                        data,
                        1); 
        if (SUCCESS != ret)
        {
            HDCP_WARNMESSAGE("Faild to send SRM Data");
            return ret;
        }
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

void PortManager::RemoveAppFromPorts(const uint32_t appId)
{
    HDCP_FUNCTION_ENTER;

    for (auto drmObject : m_DrmObjects)
    {
        DisablePort(drmObject->GetPortId(), appId);
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void PortManager::DisableAllPorts()
{
    HDCP_FUNCTION_ENTER;
    
    for (auto drmObject : m_DrmObjects)
    {
        drmObject->ClearRefAppId();
        drmObject->AddRefAppId(0);
        DisablePort(drmObject->GetPortId(), 0);
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void PortManager::ProcessHotPlug()
{
    HDCP_FUNCTION_ENTER;

    // Uevent has been triggered, need to traverse the m_DrmObjects list to find
    // out which port was plug in/out.
    for (auto drmObject : m_DrmObjects)  
    {
        drmObject->ConnAtomicBegin(); 

        auto connector = drmModeGetConnector(m_DrmFd, drmObject->GetDrmId());
        if (nullptr == connector)
        {
            drmObject->ConnAtomicEnd(); 
            HDCP_WARNMESSAGE("Port %d does not exist", drmObject->GetPortId());
            continue;
        }

        if (connector->connection == drmObject->GetConnection())
        {
            drmModeFreeConnector(connector);
            drmObject->ConnAtomicEnd(); 
            continue;
        }

        switch(connector->connection)
        {
            case DRM_MODE_DISCONNECTED:
                m_DaemonSocket.ReportStatus(
                                PORT_EVENT_PLUG_OUT,
                                drmObject->GetPortId());

                HDCP_NORMALMESSAGE(
                                "Hotplug out with port %d",
                                drmObject->GetPortId());

                break;
            case DRM_MODE_CONNECTED:
                m_DaemonSocket.ReportStatus(
                                PORT_EVENT_PLUG_IN,
                                drmObject->GetPortId());

                HDCP_NORMALMESSAGE(
                                "Hotplug in with port %d",
                                drmObject->GetPortId());
                break;
            default:
                break;
        }

        // Update port connecton state
        drmObject->SetConnection(connector->connection);
        drmModeFreeConnector(connector);

        drmObject->ConnAtomicEnd(); 
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void PortManager::CheckIntegrity()
{
    // Traverse the m_DrmObjects list to check integrity of enabled ports
    for (auto drmObject : m_DrmObjects)
    {
        // Skip if the port has not been enabled yet
        drmObject->CpTypeAtomicBegin();
        if (CP_TYPE_INVALID == drmObject->GetCpType())
        {
            drmObject->CpTypeAtomicEnd();
            continue;
        }
         
        // Check if the "Content Protection" is still enabled
        // if it changes to off or desired, link lost!
        uint8_t cpValue = CP_VALUE_INVALID; 
        uint8_t cpType = CP_TYPE_INVALID; 
        int32_t ret = GetProtectionInfo(drmObject, &cpValue, &cpType);
        if (SUCCESS != ret)
        {
            HDCP_WARNMESSAGE("Failed to get protection info");
            drmObject->CpTypeAtomicEnd();
            continue;
        }

        if (CP_ENABLED != cpValue)
        {
            m_DaemonSocket.ReportStatus(
                            PORT_EVENT_LINK_LOST,
                            drmObject->GetPortId());

            HDCP_WARNMESSAGE(
                        "Link lost with port %d",
                        drmObject->GetPortId());

            drmObject->SetCpType(CP_TYPE_INVALID);
        }

        drmObject->CpTypeAtomicEnd();
    }
}

int32_t PortManager::SetPortProperty(
                            int32_t drmId,
                            int32_t propId,
                            int32_t size,
                            const uint8_t *value,
                            const uint32_t numRetry)
{
    HDCP_FUNCTION_ENTER;

    DrmObject *drmObject = GetDrmObjectByDrmId(drmId);
    if (nullptr == drmObject)
    {
        return ENOENT;
    }
    
    if (drmSetMaster(m_DrmFd) < 0)
    {
        HDCP_ASSERTMESSAGE("Could not get drm master privilege");
        return EBUSY;
    }
    
    // If the size isn't sizeof(uint8_t), it means SRM data, need create blob
    // then set the blob id by drmModeConnectorSetProperty
    int ret = EINVAL;
    uint32_t propValue;
    if (sizeof(uint8_t) != size)
    {
        ret = drmModeCreatePropertyBlob(m_DrmFd, value, size, &propValue);
        if (SUCCESS != ret)
        {
            HDCP_ASSERTMESSAGE("Could not create blob");
            return EBUSY;
        }
    }
    else
    {
        propValue = *value;   
    }

    // Set property
    for (uint32_t i = 0; i < numRetry; ++i)
    {
        ret = drmModeConnectorSetProperty(
                                m_DrmFd,
                                drmObject->GetDrmId(),
                                propId,
                                propValue);
        if (SUCCESS == ret)
            break;
    }
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Could not set port property");
        return EBUSY;
    }

    //We must drop master privilege here
    if (drmDropMaster(m_DrmFd) < 0)
    {
        HDCP_ASSERTMESSAGE("Could not drop drm master privilege");
        return EBUSY;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t PortManager::GetProtectionInfo(
                            DrmObject *drmObject,
                            uint8_t *cpValue,
                            uint8_t *cpType)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(drmObject, EINVAL);
    CHECK_PARAM_NULL(cpValue, EINVAL);
    CHECK_PARAM_NULL(cpType, EINVAL);

    // Invalid the cpValue/cpType
    *cpValue = CP_VALUE_INVALID;
    *cpType = CP_TYPE_INVALID;

    // Query from KMD, get the Content Protection and Content Type value
    auto properties = drmModeObjectGetProperties(
                                        m_DrmFd,
                                        drmObject->GetDrmId(),
                                        DRM_MODE_OBJECT_CONNECTOR);
    if (nullptr == properties)
    {
        HDCP_ASSERTMESSAGE("Failed to get properties");
        return EBUSY;
    }
    
    for (uint32_t i = 0; i < properties->count_props; i++)
    {
        if (properties->props[i] ==
                drmObject->GetPropertyId(CONTENT_PROTECTION))
        {
            *cpValue = properties->prop_values[i];
        }
        else if (properties->props[i] ==
                drmObject->GetPropertyId(CP_CONTENT_TYPE))
        {
            *cpType = properties->prop_values[i];
        }
    }

    if (CP_ENABLED == *cpValue && CP_TYPE_INVALID == *cpType) 
    {
        *cpType = CP_TYPE_0;
    }

    drmModeFreeObjectProperties(properties);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t PortManager::GetDownstreamInfo(
                            DrmObject *drmObject,
                            uint8_t *downstreamInfo)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(drmObject, EINVAL);
    CHECK_PARAM_NULL(downstreamInfo, EINVAL);

    // Get the drm properties of this drmObject
    auto properties = drmModeObjectGetProperties(
                                    m_DrmFd,
                                    drmObject->GetDrmId(),
                                    DRM_MODE_OBJECT_CONNECTOR);
    if (nullptr == properties)
    {
        HDCP_ASSERTMESSAGE("Failed to get properties");
        return EBUSY;
    }

    // Because down stream info is blob object, need get the blob id first
    int32_t blobId  = -1;
    for (uint32_t i = 0; i < properties->count_props; i++)
    {
        if (properties->props[i] !=
            drmObject->GetPropertyId(CP_DOWNSTREAM_INFO))
            continue;
        
        blobId = properties->prop_values[i];
    }
    
    // Use the blob id to get the blob object
    auto blobInfo = drmModeGetPropertyBlob(
                                m_DrmFd,
                                blobId);
    if (nullptr == blobInfo)
    {
        HDCP_ASSERTMESSAGE("Failed to get downstream info blob");
        drmModeFreeObjectProperties(properties); 
        return EBUSY;
    }
    
    memmove(downstreamInfo, blobInfo->data, sizeof(DownstreamInfo));

    drmModeFreePropertyBlob(blobInfo);
    drmModeFreeObjectProperties(properties); 

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

DrmObject *PortManager::GetDrmObjectByPortId(const uint32_t id)
{
    for (auto drmObject : m_DrmObjects)
    {
        if (drmObject->GetPortId() == id)
        {
            return drmObject;
        }
    }

    return nullptr;
}

DrmObject *PortManager::GetDrmObjectByDrmId(const uint32_t drmId)
{
    for (auto drmObject : m_DrmObjects)
    {
        if (drmObject->GetDrmId() == drmId)
        {
            return drmObject;
        }
    }

    return nullptr;
}

