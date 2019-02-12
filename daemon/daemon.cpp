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
//! \file       daemon.cpp
//! \brief
//!

#include <list>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include <pthread.h>

#include "daemon.h"
#include "hdcpdef.h"
#include "socketdata.h"
#include "clientsock.h"
#include "servsock.h"
#include "srm.h"
#include "portmanager.h"

#include "xf86drm.h"
#include "xf86drmMode.h"

HdcpDaemon::HdcpDaemon(void) :
    m_IsValid(false)
{
    HDCP_FUNCTION_ENTER;

    if (SUCCESS == pthread_mutex_init(&m_CallBackListMutex, nullptr))
    {
        m_IsValid = true;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

HdcpDaemon::~HdcpDaemon(void)
{
    HDCP_FUNCTION_ENTER;

    if (!m_IsValid)
    {
        // Initialization failed and we don't have valid mutexes.
        return;
    }

    ACQUIRE_LOCK(&m_CallBackListMutex);
    while (!m_CallBackList.empty())
    {
        int32_t fd = m_CallBackList.front();
        int32_t ret = close(fd);
        if (SUCCESS != ret)
        {
            HDCP_ASSERTMESSAGE(
                    "Failed to close callback connection. Err: %s",
                    strerror(errno));
        }
        m_CallBackList.pop_front();
    }
    RELEASE_LOCK(&m_CallBackListMutex);

    DESTROY_LOCK(&m_CallBackListMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

int32_t HdcpDaemon::Init(void)
{
    HDCP_FUNCTION_ENTER;

    // set up the server side of the SDK socket communications
    int32_t ret = m_SdkSocket.Bind(HDCP_SDK_SOCKET_PATH);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Failed to bind SDK socket.");
        return ret;
    }

    ret = m_SdkSocket.Listen();
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Failed to set SDK socket to listen mode.");
        return ret;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

void HdcpDaemon::DispatchCommand(
                            SocketData& data,
                            int32_t appId,
                            bool& sendResponse)
{
    HDCP_FUNCTION_ENTER;

    // Need to send response to SDK default
    sendResponse = true;

    switch(data.Command)
    {
        case HDCP_API_ENUMERATE_HDCP_DISPLAY:
            HDCP_NORMALMESSAGE("Daemon received 'EnumeratePorts' request");
            EnumeratePorts(data);
            break;

        case HDCP_API_CREATE:
            HDCP_NORMALMESSAGE("Daemon received 'Create' request");
            sendResponse = false;
            break;

        case HDCP_API_DESTROY:
            HDCP_NORMALMESSAGE("Daemon received 'Destroy' request");
            PortManagerHandleAppExit(appId);
            sendResponse = false;
            break;

        case HDCP_API_CREATE_CALLBACK:
            HDCP_NORMALMESSAGE("Daemon received 'CreateCallBack' request");
            // We don't want duplicates. If it exists already then something
            // has gone wrong in our socket interface and we should remove
            // prior instance.
            // If it doesn't exist, then the remove call is harmless.
            ACQUIRE_LOCK(&m_CallBackListMutex);
            m_CallBackList.remove(appId);
            m_CallBackList.push_back(appId);
            RELEASE_LOCK(&m_CallBackListMutex);
            sendResponse = false;
            break;

        case HDCP_API_SET_PROTECTION_LEVEL:
            HDCP_NORMALMESSAGE("Daemon received 'SetProtectionLevel' request");
            SetProtectionLevel(data,appId);
            break;

        case HDCP_API_GETSTATUS:
            HDCP_NORMALMESSAGE("Daemon received 'GetStatus' request");
            GetStatus(data);
            break;

        case HDCP_API_GETKSVLIST:
            HDCP_NORMALMESSAGE("daemon received 'GetKsvList' request");
            // No need to send reponse after the function call,
            // since this function already done the communication itself.
            GetKsvList(data, appId);
            sendResponse = false;
            break;

        case HDCP_API_SENDSRMDATA:
            HDCP_NORMALMESSAGE("Daemon received 'SendSrmData' request");
            SendSRMData(data, appId);
            break;

        case HDCP_API_GETSRMVERSION:
            HDCP_NORMALMESSAGE("Daemon received 'GetSrmVersion' request");
            GetSRMVersion(data);
            break;

        case HDCP_API_CONFIG:
            HDCP_NORMALMESSAGE("Daemon received 'Config' request");
            Config(data);
            break;

        default:
            HDCP_WARNMESSAGE(
                        "Daemon received unknown command: %d",
                        data.Command);
            // We want this to go ahead and send the response back
            data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
            break;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpDaemon::MessageResponseLoop(void)
{
    HDCP_FUNCTION_ENTER;

    do
    {
        // SocketData clears itself on instantiation, so do that within the
        // scope of the do/while, or explicitly clear it if you move this.
        SocketData data;

        // No need to zero-out data, it's done in the SocketData constructor
        int32_t appId  = -1;
        int32_t sts = m_SdkSocket.GetTask(data, appId);
        if (SUCCESS != sts)
        {
            if (ECANCELED == sts)
            {
                // We received a kill signal
                return;
            }

            HDCP_ASSERTMESSAGE("GetTask failed.");
            continue;
        }

        bool sendResponse = true;
        // Verify valid socket data size
        if (sizeof(data) != data.Size)
        {
            HDCP_ASSERTMESSAGE("Invalid data received");
            // We want this to go ahead and send a response back
            data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
            data.Size = sizeof(data);
        }
        else
        {
            DispatchCommand(data, appId, sendResponse);
        }

        if (sendResponse)
        {
            // The protocol requires that the packet sent back match the request
            // in size.
            sts = m_SdkSocket.SendResponse(data, appId);
            if (SUCCESS != sts)
            {
                HDCP_ASSERTMESSAGE("SendResponse failed. %d", data.Status);

                // If we can't communicate with the app, destroy the connection
                PortManagerHandleAppExit(appId);
            }
        }
    } while (true);
}

void HdcpDaemon::ReportStatus(PORT_EVENT event, uint32_t portId)
{
    HDCP_FUNCTION_ENTER;

    SocketData                  data;

    data.Size               = sizeof(data);
    data.Command            = HDCP_API_REPORTSTATUS;
    data.PortCount          = 1;
    data.SinglePort.Event   = event;
    data.SinglePort.Id      = portId;

    ACQUIRE_LOCK(&m_CallBackListMutex);

    auto next = m_CallBackList.begin();
    while (next != m_CallBackList.end())
    {
        auto fd = next;

        int32_t sts = m_SdkSocket.SendResponse(data, *fd);
        if (SUCCESS != sts)
        {
            // If we failed then the connection is bad or gone and
            // we should just go ahead and clean it up
            sts = close(*fd);
            if (SUCCESS != sts)
            {
                HDCP_WARNMESSAGE(
                        "Failed to close fd of a bad socket %d! Err: %s",
                        *fd,
                        strerror(errno));
            }

            HDCP_VERBOSEMESSAGE("Remove unavailable callback socket from list");
            next = m_CallBackList.erase(fd);
            continue;
        }

        next++;
    }

    RELEASE_LOCK(&m_CallBackListMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpDaemon::EnumeratePorts(SocketData& data)
{
    HDCP_FUNCTION_ENTER;

    int32_t sts = PortManagerEnumeratePorts(data.Ports, data.PortCount);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("Enumerate failed\n");
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    HDCP_NORMALMESSAGE("Enumerate successfully\n");
    data.Status = HDCP_STATUS_SUCCESSFUL;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpDaemon::SetProtectionLevel(SocketData& data, uint32_t appId)
{
    HDCP_FUNCTION_ENTER;

    // this API expects one and only one port to be spec'd
    if (ONE_PORT != data.PortCount)
    {
        data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
        return;
    }

    if (PORT_ID_MAX < data.SinglePort.Id)
    {
        HDCP_ASSERTMESSAGE("Invalid port id");
        data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
        return;
    }

    int32_t sts = EINVAL;
    if (data.Level == HDCP_LEVEL1 || data.Level == HDCP_LEVEL2)
    {
        sts = PortManagerEnablePort(data.SinglePort.Id, appId, data.Level);
    }
    else if (data.Level == HDCP_LEVEL0)
    {
        sts = PortManagerDisablePort(data.SinglePort.Id, appId);
    }
    else
    {
        HDCP_ASSERTMESSAGE("Invalid protection level!\n");
        data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
        return;
    }

    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("SetProtectionLevel failed!");
        switch (sts)
        {
            case ENOENT:
                data.Status = HDCP_STATUS_ERROR_NO_DISPLAY;
                break;
            default:
                data.Status = HDCP_STATUS_ERROR_INTERNAL;
                break;
        }
        return;
    }

    HDCP_NORMALMESSAGE("SetProtectionLevel %d successfully", data.Level);
    data.Status = HDCP_STATUS_SUCCESSFUL;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpDaemon::GetStatus(SocketData& data)
{
    HDCP_FUNCTION_ENTER;

    // should be one and only one port specified
    if (ONE_PORT != data.PortCount)
    {
        data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
        return;
    }

    if (data.SinglePort.Id > PORT_ID_MAX)
    {
        data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
        return;
    }

    int32_t sts = PortManagerGetStatus(
                        data.SinglePort.Id,
                        &data.SinglePort.status);
    if (SUCCESS != sts)
    {
        switch (sts)
        {
            case ENOENT:
                data.Status = HDCP_STATUS_ERROR_NO_DISPLAY;
                break;
            default:
                data.Status = HDCP_STATUS_ERROR_INTERNAL;
                break;
        }
        return;
    }

    data.Status = HDCP_STATUS_SUCCESSFUL;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpDaemon::GetKsvList(SocketData& data, uint32_t appId)
{
    HDCP_FUNCTION_ENTER;

    // this API expects one and only one port to be spec'd
    if (ONE_PORT != data.PortCount)
    {
        HDCP_ASSERTMESSAGE("We expect only 1 PORT!");
        data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
        return;
    }

    if (data.SinglePort.Id > PORT_ID_MAX)
    {
        HDCP_ASSERTMESSAGE("Invalid Port ID");
        data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
        return;
    }
    
    std::unique_ptr<uint8_t> ksvList(
                    new (std::nothrow) uint8_t[MAX_KSV_COUNT * KSV_SIZE]);
    if (nullptr == ksvList.get())
    {
        HDCP_ASSERTMESSAGE("Unable to allocate ksvList buffer");
        data.Status = HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY;
        return;
    }

    int32_t sts = PortManagerGetKsvList(
                                data.SinglePort.Id,
                                &data.KsvCount,
                                &data.Depth,
                                ksvList.get());
    if (SUCCESS != sts)
    {
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }
    
    if (data.KsvCount > MAX_KSV_COUNT)
    {
        HDCP_ASSERTMESSAGE("Invalid ksvCount");
        data.Status = HDCP_STATUS_ERROR_MAX_DEVICES_EXCEEDED;
        return;
    }

    if (data.Depth > MAX_TOPOLOGY_DEPTH)
    {
        HDCP_ASSERTMESSAGE("Invalid depth");
        data.Status = HDCP_STATUS_ERROR_MAX_DEPTH_EXCEEDED;
        return;
    }
    
    // Send Ksv Count and depth across the socket
    data.Status = HDCP_STATUS_SUCCESSFUL;
    sts = m_SdkSocket.SendResponse(data, appId);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("SendKsvCount failed");
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    // Send Ksv List across the socket
    sts = m_SdkSocket.SendKsvListData(
                            ksvList.get(),
                            data.KsvCount * KSV_SIZE,
                            appId);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("SendKsvListData failed");
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    HDCP_NORMALMESSAGE("GetKsvList successfully");
    data.Status = HDCP_STATUS_SUCCESSFUL;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpDaemon::SendSRMData(SocketData& data, uint32_t appId)
{
    HDCP_FUNCTION_ENTER;

    // According to HDCP spec, 5kb limit on 1st generation SRM Message
    if (data.SrmOrKsvListDataSz > SRM_FIRST_GEN_MAX_SIZE)
    {
        HDCP_ASSERTMESSAGE(
                    "SRM message size %d is too large!",
                    data.SrmOrKsvListDataSz);
        data.Status = HDCP_STATUS_ERROR_INVALID_PARAMETER;
        return;
    }

    std::unique_ptr<uint8_t> srmData(
                        new (std::nothrow) uint8_t[data.SrmOrKsvListDataSz]);
    if (nullptr == srmData.get())
    {
        HDCP_ASSERTMESSAGE("Unable to allocate srm buffer");
        data.Status = HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY;
        return;
    }

    // We received the size successfully, so respond with such
    data.Status = HDCP_STATUS_SUCCESSFUL;
    int32_t sts = m_SdkSocket.SendResponse(data, appId);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("SendResponse failed");
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    // We need the first socket communication to get the size of the srm
    // buffer, then the second communication fills the buffer we allocate
    // based on that size.
    sts = m_SdkSocket.GetSrmData(srmData.get(), data.SrmOrKsvListDataSz, appId);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("Failed to receive srm buffer");
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    sts = StoreSrm(srmData.get(), data.SrmOrKsvListDataSz);
    if (SUCCESS != sts)
    {
        switch(sts)
        {
            case EINVAL:
                data.Status = HDCP_STATUS_ERROR_SRM_INVALID;
                break;
            case EAGAIN:
                data.Status = HDCP_STATUS_ERROR_SRM_NOT_RECENT;
                break;
            default:
                data.Status = HDCP_STATUS_ERROR_INTERNAL;
        }
        return;
    }
    
    sts = PortManagerSendSRMDdata(srmData.get(), data.SrmOrKsvListDataSz);
    if (SUCCESS != sts)
    {
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    data.Status = HDCP_STATUS_SUCCESSFUL;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpDaemon::GetSRMVersion(SocketData& data)
{
    HDCP_FUNCTION_ENTER;

    int32_t sts = GetSrmVersion(&(data.SrmVersion));
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("GetSrmVersion failed!");
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    HDCP_NORMALMESSAGE("GetSrmVersion successfully");
    data.Status = HDCP_STATUS_SUCCESSFUL;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpDaemon::Config(SocketData& data)
{
    HDCP_FUNCTION_ENTER;

    if (data.Config.type != data.Config.type)
    {
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    int32_t sts = SrmConfig(data.Config.disableSrmStorage);
    if (SUCCESS != sts)
    {
        data.Status = HDCP_STATUS_ERROR_INTERNAL;
        return;
    }

    data.Status = HDCP_STATUS_SUCCESSFUL;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

