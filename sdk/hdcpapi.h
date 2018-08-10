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
//! \file       hdcp_api.h
//! \brief
//!

#ifndef __HDCP_API_H__
#define __HDCP_API_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/// \define NUM_PHYSICAL_PORTS_MAX
/// \brief The maximum number of ports that can connect to physical systems.
#define NUM_PHYSICAL_PORTS_MAX  5

#define PORT_ID_MAX 5

/// \define MAX_KSV_COUNT
/// \brief The maximum number of devices that can connect in the topology.
#define MAX_KSV_COUNT 127
/// \define MAX_TOPOLOGY_DEPTH
/// \brief The maximum depth that can connect in the topology.
#define MAX_TOPOLOGY_DEPTH 7
/// \define KSV_SIZE
/// \brief The size of KSV in number of bytes
#define KSV_SIZE 5

#define HDCP_SRM_ID             0x80

/// the PORT_STATUS_* macros.
/// Status bits can be OR'd together when appropriate.
typedef unsigned int PORT_STATUS;   // each bit represents one status

/// \define PORT_STATUS_DISCONNECTED
/// \brief The port is disconnected from any downstream device.
#define PORT_STATUS_DISCONNECTED        0x00

/// \define PORT_STATUS_CONNECTED
/// \brief The port is connected to an downstream device.
#define PORT_STATUS_CONNECTED           0x01

/// \define PORT_STATUS_HDCP14_ENABLED
/// \brief The HDCP link is active.  Premium content is enabled.
#define PORT_STATUS_HDCP_TYPE0_ENABLED        0x02

/// \define PORT_STATUS_HDCP14_ENABLED
/// \brief The HDCP link is active.  Premium content is enabled.
#define PORT_STATUS_HDCP_TYPE1_ENABLED        0x04

/// \define PORT_STATUS_REPEATER_ATTACHED
/// \brief A repeater is connected to this port.
#define PORT_STATUS_REPEATER_ATTACHED   0x08

/// \define PORT_STATUS_INVALID
/// \brief The port is pending disable or Meta status
#define PORT_STATUS_INVALID             ((PORT_STATUS) - 1)

/// \typedef PORT_EVENT
/// \brief An enumeration listing the possible events reported from the KMD
///         back to the user via the application callback function.
typedef enum _PORT_EVENT
{
    PORT_EVENT_NONE = 0,
    PORT_EVENT_PLUG_IN,         // hot plug in
    PORT_EVENT_PLUG_OUT,        // hot plug out
    PORT_EVENT_LINK_LOST,       // HDCP authentication step3 fail
} PORT_EVENT;

/// \typedef Port
/// \brief A structure that holds the identifier and status for a given port.
typedef struct _Port
{
    uint32_t        Id;
    PORT_STATUS     status;
    PORT_EVENT      Event;
} Port;

/// \typedef PortList
/// \brief During enumeration calls, a list of available DP and HDMI is returned
/// in this structure.
typedef struct _PortList
{
    Port         Ports[NUM_PHYSICAL_PORTS_MAX];
    unsigned int PortCount;
} PortList;

/// \typedef HDCP_CONFIG_TYPE
/// \brief An enumeration listing the possible types of HDCP configuration
///         set by customer.
enum HDCP_CONFIG_TYPE
{
    INVALID_CONFIG = 0,             // invalid configure type
    SRM_STORAGE_CONFIG,             // config to disable/enable SRM storage
};

/// \typedef HDCP_CONFIG
/// \brief Customer can pass configuration to HDCP daemon via this structure.
typedef struct _HDCP_CONFIG
{
    enum HDCP_CONFIG_TYPE type;
    bool disableSrmStorage;
} HDCP_CONFIG;

/// \typedef HDCP_STATUS
/// \brief function call return values supported by the HDCP SDK.
typedef enum _HDCP_STATUS
{
    // no error
    HDCP_STATUS_SUCCESSFUL = 0,

    // only call HDCPCreate once
    HDCP_STATUS_ERROR_ALREADY_CREATED,

    // the parameters are invalid
    HDCP_STATUS_ERROR_INVALID_PARAMETER,

    // no display connected in the port
    HDCP_STATUS_ERROR_NO_DISPLAY,

    // the display is a revoked device
    HDCP_STATUS_ERROR_REVOKED_DEVICE,

    // the SRM is invalid
    HDCP_STATUS_ERROR_SRM_INVALID,

    // no memory for allocation
    HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY,

    // internal error happens
    HDCP_STATUS_ERROR_INTERNAL,

    // the SRM's version number is less than that in memory
    HDCP_STATUS_ERROR_SRM_NOT_RECENT,

    // can't open non-volatile storage to save SRM file
    HDCP_STATUS_ERROR_SRM_FILE_STORAGE,

    // more than 127 downstream devices are attached
    HDCP_STATUS_ERROR_MAX_DEVICES_EXCEEDED,

    // more than 7 levels of repeater are cascaded
    HDCP_STATUS_ERROR_MAX_DEPTH_EXCEEDED,

    // socket channel is broken
    HDCP_STATUS_ERROR_MSG_TRANSACTION,
} HDCP_STATUS;

/// \enum HDCP level 0 means disable HDCP,
/// 1 enable highest level HDCP, 2 force enable HDCP2.2.
/// @{
enum HDCP_LEVEL
{
    HDCP_LEVEL0     = 0,   // Disable HDCP
    HDCP_LEVEL1     = 1,   // Enable port maxmum supported HDCP version.
    HDCP_LEVEL2     = 2   // Force Enable HDCP 2.2 for play Type1 content
};

/// \typedef CallBackFunction
/// \brief Prototype for application callback function.
typedef void (*CallBackFunction)(
                            uint32_t hdcpHandle,
                            uint32_t portId,
                            PORT_EVENT portEvent,
                            void *context);

typedef void * Context;

////////////////////////////////////////////////////////////////////////////////
/// \brief      Create an HDCP context and register with the daemon code.
/// \par        Details:
/// \li         Create an HDCP context.
/// \li         Send Create message to daemon,
///             and on failure destroy the HDCP context.
///
/// \param[out] pHdcpHandle Receive HDCP handle.
/// \param[in]  func Appliation callback function to process kernel and
///             link status events.
/// \param[in]  context Pointer which points to the application object
///             that handles the callback.
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INVALID_PARAMETER
///             if pHDCPContext or func are NULL.
/// \return     HDCP_STATUS_ERROR_ALREADY_CREATED
///             if HDCPCreate was already called.
/// \return     HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY
///             if a dynamic memory allocation failed.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPCreate(
                    uint32_t *pHdcpHandle,
                    CallBackFunction func,
                    void *context);

////////////////////////////////////////////////////////////////////////////////
/// \brief      Destroy an HDCP context and deregister with the daemon code.
/// \par        Details:
/// \li         Send Destroy message to daemon. Return directly on failure.
/// \li         Destroy the HDCP context.
///
/// \param[in]  hdcpHandle The HDCP handle to destroy.
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INVALID_PARAMETER if HDCPContext is NULL.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPDestroy(const uint32_t hdcpHandle);

////////////////////////////////////////////////////////////////////////////////
/// \brief      Enumerate available ports and return connection status.
///
/// \param[in]  hdcpHandle The HDCP handle.
/// \param[out] portList The structure to receive the list of available ports.
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INVALID_PARAMETER
///             if HDCPContext or portList are NULL.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPEnumerateDisplay(
                    const uint32_t hdcpHandle,
                    PortList *pPortList);

////////////////////////////////////////////////////////////////////////////////
/// \brief      Enable/Disable  the HDCP link on the specified port.
///
/// \param[in]  hdcpHandle The HDCP handle.
/// \param[in]  portId The identifier of the port.
/// \param[in]  level.
///             0 is to disable HDCP.
///             1 to enable maxmum HDCP version of downstream device.
///             2 force to enable HDCP2.2.
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INVALID_PARAMETER
///             if HDCPContext is NULL or portId is out of range.
/// \return     HDCP_STATUS_ERROR_NO_DISPLAY
///             No display is attached to the specified port.
/// \return     HDCP_STATUS_ERROR_REVOKED_DEVICE
///             The attached device is not authorized.
/// \return     HDCP_STATUS_ERROR_MAX_DEVICES_EXCEEDED
///             more than 127 downstream devices are attached.
/// \return     HDCP_STATUS_ERROR_MAX_DEPTH_EXCEEDED
///             more than 7 levels of repeater are cascaded.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPSetProtectionLevel(
                    const uint32_t hdcpHandle,
                    const uint32_t portId,
                    const HDCP_LEVEL level);

////////////////////////////////////////////////////////////////////////////////
/// \brief      Get the status of the specified port.
///
/// \param[in]  hdcpHandle The HDCP handle.
/// \param[in]  portId The identifier of the port.
/// \param[out] portStatus receives the port status.
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INVALID_PARAMETER
///             if HDCPContext or portStatus are NULL, or portId out of range.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPGetStatus(
                    const uint32_t hdcpHandle,
                    const uint32_t portId,
                    PORT_STATUS *portStatus);

////////////////////////////////////////////////////////////////////////////////
/// \brief      Get the ksv list of connected/enabled HDCP devices
///             in the topology by big endian format
///
/// \param[in]  hdcpHandle The HDCP handle.
/// \param[in]  portId The identifier of the port.
/// \param[out] ksvCount The number of KSV's connected in the topology.
/// \param[out] depth The number of repeaters cascaded in the topology.
/// \param[out] ksvList receives ksv list of connected/enabled HDCP devices
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INVALID_PARAMETER
///             if HDCPContext or portStatus are NULL, or portId out of range.
/// \return     HDCP_STATUS_ERROR_MAX_DEVICES_EXCEEDED
///             more than 127 downstream devices are attached.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPGetKsvList(
                    const uint32_t hdcpHandle,
                    const uint32_t portId,
                    uint8_t *ksvCount,
                    uint8_t *depth,
                    uint8_t *ksvList);

////////////////////////////////////////////////////////////////////////////////
/// \brief      Send SRM data to the system for processing.
///
/// \param[in]  srmSize Size of the SRM data.
/// \param[in]  pSrmData Pointer to buffer containing the SRM data.
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INVALID_PARAMETER
///             if HDCPContext or pSrmData are NULL.
/// \return     HDCP_STATUS_ERROR_SRM_INVALID if the SRM signature is invalid.
/// \return     HDCP_STATUS_ERROR_SRM_NOT_RECENT
///             if the SRM's version number is less than that in memory.
/// \return     HDCP_STATUS_ERROR_SRM_FILE_STORAGE
///             if failed to open non-volatile storage to save SRM file.
/// \return     HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY
///             if a dynamic memory allocation failed.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPSendSRMData(
                    const uint32_t hdcpHandle,
                    const uint32_t srmSize,
                    const uint8_t *pSrmData);

////////////////////////////////////////////////////////////////////////////////
/// \brief      Get current SRM version to app.
///
/// \param[in]  hdcpHandle The HDCP handle.
/// \param[out] version  current Srmdata version.
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INVALID_PARAMETER if version is NULL.
/// \return     HDCP_STATUS_ERROR_SRM_INVALID if the Get SRM version failed.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPGetSRMVersion(const uint32_t hdcpHandle, uint16_t *version);

////////////////////////////////////////////////////////////////////////////////
/// \brief      Set HDCP configuration and send the configuration
///             info to daemon.
///
/// \param[in]  Config, the structure to contain configuration info.
///             In this structure, customer can set config type to
///             be SRM_STORAGE_CONFIG and indicate whether SRM will be stored to 
///             device by daemon or not as IoTG's request.
/// \return     HDCP_STATUS_SUCCESSFUL if successful
/// \return     HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY
///             if a dynamic memory allocation failed.
/// \return     HDCP_STATUS_ERROR_INTERNAL for any other error.
////////////////////////////////////////////////////////////////////////////////
HDCP_STATUS HDCPConfig(const uint32_t hdcpHandle, HDCP_CONFIG Config);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // __HDCP_API_H__
