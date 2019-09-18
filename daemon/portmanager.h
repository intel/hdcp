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
//! \file       portmanager.h
//! \brief
//!

#ifndef __HDCP_PORTMANAGER_H__
#define __HDCP_PORTMANAGER_H__

#include <list>
#include <pthread.h>
#include <time.h>

#include "hdcpdef.h"
#include "hdcpapi.h"
#include "port.h"

#ifdef ANDROID
#include <hwcserviceapi.h>
#include <log/log.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <iservice.h>

#define BINDER_IPC          "/dev/vndbinder"
#endif

#define THREAD_STARTUP_BACKOFF_DELAY_US     100
#define AUTH_CHECK_DELAY_MS                 1000
#define INTEGRITY_CHECK_DELAY_MS            500
#define AUTH_NUM_RETRY                      3

//KMD content protection value
#define CP_VALUE_INVALID    UINT8_MAX
#define CP_OFF              0
#define CP_DESIRED          1
#define CP_ENABLED          2

//KMD content type value
#define CP_TYPE_INVALID     UINT8_MAX 
#define CP_TYPE_0           0
#define CP_TYPE_1           1

//KMD property name
#define CONTENT_PROTECTION          "Content Protection" 
#define CP_CONTENT_TYPE             "CP_Content_Type" 
#define CP_DOWNSTREAM_INFO          "CP_Downstream_Info" 
#define CP_SRM                      "CP_SRM" 

typedef struct _DownstreamInfo
{
    // HDCP ver in force
    uint32_t hdcpVersion;
    uint8_t cpType;

    // KSV of immediate HDCP Sink. In Little-Endian Format.
    char bksv[KSV_SIZE];

    // Whether Immediate HDCP sink is a repeater?
    bool isRepeater;

    // Depth received from immediate downstream repeater
    uint8_t depth;

    // Device count received from immediate downstream repeater
    uint32_t deviceCount;

    // Max buffer required to hold ksv list received from immediate
    // repeater. In this array first deviceCount * KSV_SIZE
    // will hold the valid ksv bytes.
    // If authentication specification is
    //      HDCP1.4 - each KSV's Bytes will be in Little-Endian format.
    //      HDCP2.2 - each KSV's Bytes will be in Big-Endian format.
    // 
    char ksvList[KSV_SIZE * MAX_KSV_COUNT];
} DownstreamInfo;

class HdcpDaemon;

class PortManager
{
    // Declare private variables
private:
    HdcpDaemon&             m_DaemonSocket;
    bool                    m_IsValid;
    int32_t                 m_DrmFd;
    std::list<DrmObject *>  m_DrmObjects;

    // Declare public interface functions
public:

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Constructor for the PortManager class
    ///
    /// \return     Nothing
    ///
    /// Callers must check the newly created manager against IsValid.
    ///////////////////////////////////////////////////////////////////////////
    PortManager(HdcpDaemon& daemonSocket);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Virtual destructor for PortManager
    ///
    /// \return     nothing -- It's a destructor
    ///
    /// This will cleanup any ports currently in the port list
    /// Any dynamic memory controlled by the PortManager should be released
    ///////////////////////////////////////////////////////////////////////////
    virtual ~PortManager(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Checks if the portmanager was successfully created
    ///
    /// \return     true if valid,
    ///             false otherwise
    ///////////////////////////////////////////////////////////////////////////
    bool IsValid(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Enumerate potential hdcp ports
    ///
    /// \param[out] portList,   A list of Port Ids and Statuses
    /// \param[out] portCount,  Number of ports found
    /// \return     SUCCESS or errno otherwise
    ///
    /// Applications call this function to determine which ports can be used
    /// for HDCP.
    ///////////////////////////////////////////////////////////////////////////
    int32_t EnumeratePorts(
            Port (&portList)[NUM_PHYSICAL_PORTS_MAX],
            uint32_t& portCount);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Enable HDCP on the specified port
    ///
    /// \param[in]  portId, Port Id on which to enable HDCP
    /// \param[in]  appId,  Id corresponding to the calling application
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t EnablePort(
                    const uint32_t portId,
                    const uint32_t appId,
                    const uint8_t level);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Disable HDCP on the specified port
    ///
    /// \param[in]  portId, Port Id on which to disable HDCP
    /// \param[in]  appId,  Id corresponding to the calling application
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t DisablePort(const uint32_t portId, const uint32_t appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get the status of the specified port
    ///
    /// \param[in]  portId,     Port Id on which to enable HDCP
    /// \param[out] portStatus, Status
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetStatus(const uint32_t portId, PORT_STATUS *portStatus);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get the ksv list of connected/enabled devices in the topology
    ///
    /// \param[in]   portId,  Port Id on which to get ksv list
    /// \param[out]  ksvList, list of ksvs connected/enabled in topology
    /// \return      SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetKsvList(
                    const uint32_t portId,
                    uint8_t *ksvCount,
                    uint8_t *depth,
                    uint8_t *ksvList);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Send the SRM Data to the topology
    ///
    /// \param[in]   portId,  Port Id on which to get ksv list
    /// \param[out]  ksvCount, the number of KSVs in the topology
    /// \return      SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t SendSRMData(const uint8_t *data, const uint32_t size);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Process an HDCP hotplug in or out uEvent
    ///////////////////////////////////////////////////////////////////////////
    void ProcessHotPlug();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Process an HDCP Integrity check
    ///////////////////////////////////////////////////////////////////////////
    void CheckIntegrity();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Remove an appId from the active lists of all ports
    ///
    /// \param[in]  appId,      Id of the app we wish to purge
    ///////////////////////////////////////////////////////////////////////////
    void RemoveAppFromPorts(const uint32_t appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Disable all ports
    ///////////////////////////////////////////////////////////////////////////
    void DisableAllPorts();

    // Declare private functions
protected:

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get DrmObject by port Id
    ///
    /// \param[in]  id,      Id of the port
    /// \return     DrmObject*  Pointer of the drm_object
    ///////////////////////////////////////////////////////////////////////////
    DrmObject* GetDrmObjectByPortId(const uint32_t id);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get DrmObject by drm Id
    ///
    /// \param[in]  id,      Id of the drm object
    /// \return     DrmObject*  Pointer of the drm_object
    ///////////////////////////////////////////////////////////////////////////
    DrmObject* GetDrmObjectByDrmId(const uint32_t id);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get Content Protection Value
    ///
    /// \param[in]  drmObject,  drm object
    /// \param[in]  cpValue,    address of the protection value
    /// \param[in]  cpType,     address of the protection type
    /// \return     int32_t     Function return status
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetProtectionInfo(
                        DrmObject *drmObject,
                        uint8_t *cpValue,
                        uint8_t *cpType);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Virtual function set port property. By default, port property is
    ///         set through drm ioctl call. On ClearLinux, port property is set
    ///         through IAS. On Andorid, port property is set through Hardware
    ///         Composer.
    ///
    /// \param[in]  drmId,      Id of the drm object
    /// \param[in]  propertyId, Id of property
    /// \param[in]  size,       Length of value, it's an array
    /// \param[in]  value,      Pointer of the array
    /// \param[in]  numRetry,   the retry times
    /// \return     int32_t     Function return status
    ///////////////////////////////////////////////////////////////////////////
    virtual int32_t SetPortProperty(
                        int32_t drmId,
                        int32_t propertyId,
                        int32_t size,
                        const uint8_t *value,
                        uint32_t numRetry);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get DownStreamInfo
    ///
    /// \return     int32_t     Function return status
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetDownstreamInfo(
                        DrmObject *drmObject,
                        uint8_t *downstreamInfo);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get information of connectors and initialize m_DrmObjects
    ///
    /// \return     int32_t     Function return status
    ///////////////////////////////////////////////////////////////////////////
    int32_t InitDrmObjects();

    static void SigCatcher(int sig);
};

#ifdef ANDROID
class PortManagerHWComposer : public PortManager
{
public:

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Constructor for the PortManagerHWComposer class,
    ///         this is for Android platform.
    ///
    /// \return     Nothing
    ///
    /// Callers must check the newly created manager against IsValid.
    ///////////////////////////////////////////////////////////////////////////
    PortManagerHWComposer(HdcpDaemon& daemonSocket);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Virtual function set port property. By default, port property is
    ///         set through drm ioctl call. On ClearLinux, port property is set
    ///         through IAS. On Andorid, port property is set through Hardware
    ///         Composer.
    ///
    /// \param[in]  drmId,      Id of the drm object
    /// \param[in]  propertyId, Id of property
    /// \param[in]  size,       Length of value, it's an array
    /// \param[in]  value,      Pointer of the array
    /// \param[in]  numRetry,   the retry times
    /// \return     int32_t     Function return status
    ///////////////////////////////////////////////////////////////////////////
    virtual int32_t SetPortProperty(
                        int32_t drmId,
                        int32_t propertyId,
                        int32_t size,
                        const uint8_t *value,
                        uint32_t numRetry);
};
#endif

///////////////////////////////////////////////////////////////////////////////
/// \brief  Create the local instance of the PortManager
///
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t PortManagerInit(HdcpDaemon& socket);

void PortManagerSetPortProperty(
                            int32_t drmId,
                            int32_t propertyId,
                            int32_t size,
                            uint8_t *value);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Create the local instance of the PortManager
///
/// \return     void
///////////////////////////////////////////////////////////////////////////////
void PortManagerRelease(void);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Enumerate potential hdcp ports
///
/// \param[out] portList,   A list of Port Ids and Statuses
/// \param[out] portCount,  Number of ports found
/// \return     SUCCESS or errno otherwise
///
/// Applications call this function to determine which ports can be used for
/// HDCP.
/// This function uses libdrm to enumerate the display information.
///////////////////////////////////////////////////////////////////////////////
int32_t PortManagerEnumeratePorts(
        Port (&portList)[NUM_PHYSICAL_PORTS_MAX],
        uint32_t& portCount);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Enable HDCP on the specified port
///
/// \param[in]  portId, Port Id on which to enable HDCP
/// \param[in]  appId,  Id corresponding to the calling application
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t PortManagerEnablePort(
                        const uint32_t portId,
                        const uint32_t appId,
                        const uint8_t level);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Disable HDCP on the specified port
///
/// \param[in]  portId, Port Id on which to disable HDCP
/// \param[in]  appId,  Id corresponding to the calling application
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t PortManagerDisablePort(const uint32_t portId, const uint32_t appId);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Get the status of the specified port
///
/// \param[in]  portId,     Port Id on which to enable HDCP
/// \param[out] portStatus, Status
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t PortManagerGetStatus(const uint32_t portId, PORT_STATUS *portStatus);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Get the ksv list of connected/enabled HDCP1 devices in the topology
///
/// \param[in]   portId,  Port Id on which to get ksv list
/// \param[out]  ksvCount, number of KSVs in the topology
/// \param[out]  depth, depth of the HDCP1 topology
/// \param[out]  ksvList, list of ksvs connected/enabled in topology
/// \return      SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t PortManagerGetKsvList(
                        const uint32_t portId,
                        uint8_t *ksvCount,
                        uint8_t *depth,
                        uint8_t *ksvList);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Send SRM Data to the topology
///
///////////////////////////////////////////////////////////////////////////////
int32_t PortManagerSendSRMDdata(const uint8_t *data, const uint32_t size);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Process an HDCP hotplug in or out uEvent
///
///////////////////////////////////////////////////////////////////////////////
void PortManagerProcessHotPlug();

///////////////////////////////////////////////////////////////////////////////
/// \brief  Remove app from from ports' activity lists
///////////////////////////////////////////////////////////////////////////////
void PortManagerHandleAppExit(const uint32_t appId);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Disable all ports
///////////////////////////////////////////////////////////////////////////////
void PortManagerDisableAllPorts();

#endif // __HDCP_PORTMANAGER_H__
