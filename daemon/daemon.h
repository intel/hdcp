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
//! \file       daemon.h
//! \brief
//!

#ifndef __HDCP_DAEMON_H__
#define __HDCP_DAEMON_H__

#include <list>
#include <pthread.h>

#include "hdcpdef.h"
#include "hdcpapi.h"
#include "servsock.h"
#include "clientsock.h"
#include "socketdata.h"

#define APP_ID_INTERNAL     0

class HdcpDaemon
{
private:
    LocalServerSocket   m_SdkSocket;
    std::list<int32_t>  m_CallBackList;
    pthread_mutex_t     m_CallBackListMutex;

    bool                m_IsValid;

public:
    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Construct the HdcpDaemon object
    ///
    /// \return     Nothing
    ////////////////////////////////////////////////////////////////////////////
    HdcpDaemon(void);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Destroy the HdcpDaemon object
    ///
    /// \return     Nothing
    ////////////////////////////////////////////////////////////////////////////
    virtual ~HdcpDaemon(void);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Initialize main socket for listening for SDK requests
    ///
    /// \return     SUCCESS or errno
    ////////////////////////////////////////////////////////////////////////////
    int32_t Init(void);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Main message loop for receiving messages from the SDK
    ///
    /// \return     Nothing
    ////////////////////////////////////////////////////////////////////////////
    void MessageResponseLoop(void);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Parse the command in SocketData, and then dispatch it to the
    ///             dedicated function to handle
    ///
    /// \param[in/out]  data   General message packet, it contains a port list
    ///                         the daemon can use to fill in port information
    /// \param[out]     sendResponse whether need to send response to SDK or not 
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ////////////////////////////////////////////////////////////////////////////
    void DispatchCommand(SocketData& data, int32_t appId, bool& sendResponse);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Enumerate connected HDCP-capabile displays.
    ///
    /// \param[in/out]  data   General message packet, it contains a port list
    ///                         the daemon can use to fill in port information
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ////////////////////////////////////////////////////////////////////////////
    void EnumeratePorts(SocketData& data);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      get HDCP capability of connect HDCP display
    ///
    /// \param[in/out]  data   General message packet, it contains a port list
    ///                         the daemon can use to fill in port information
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ////////////////////////////////////////////////////////////////////////////
    void GetCapability(SocketData& data);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Enable/Disable HDCP on the specifed port connection.
    ///
    /// \param[in/out]  data    General message packet. The user has filled in
    ///                         PortId.
    /// \param[in]      appId   Id of the corresponding app's connection
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ///
    /// This function will add the appId to a list of active apps for the
    /// specified port. If this is the first app to request HDCP encryption on
    /// the port, then it will also perform the steps of authentication.
    ////////////////////////////////////////////////////////////////////////////
    void SetProtectionLevel(SocketData& data, uint32_t appId);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Get the HDCP enabled/disabled status of the specified port.
    ///
    /// \param[in/out]  data    General message packet. The user has filled in
    ///                         PortId.
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ////////////////////////////////////////////////////////////////////////////
    void GetStatus(SocketData& data);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Get the KsvList of connected HDCP1.X devices
    ///
    /// \param[in/out]  data    General message packet. The user has filled in
    ///                         PortId.
    /// \param[in]      appId   Id of the corresponding app's connection
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ////////////////////////////////////////////////////////////////////////////
    void GetKsvList(SocketData& data, uint32_t appId);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Get the ksv count (number of ksv in the topology)
    ///
    /// \param[in/out]  data    General message packet. The user has filled in
    ///                         PortId.
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ////////////////////////////////////////////////////////////////////////////
    void GetKsvCount(SocketData& data);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief       Get the depth of the topology.
    ///
    /// \param[in/out]  data    General message packet. The user has filled in
    ///                         PortId.
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ////////////////////////////////////////////////////////////////////////////
    void GetDepth(SocketData& data);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Send SRM data.
    ///
    /// \param[in/out]  data    General message packet.
    /// \param[in]      appId   Id of the corresponding app's connection
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ///
    /// This function will parse the SRM data from application, and update the
    /// daemon's local copy if it is newer and valid.
    ////////////////////////////////////////////////////////////////////////////
    void SendSRMData(SocketData& data, uint32_t appId);
     
    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Get current SRM version.
    ///
    /// \param[in/out]  data    General message packet.
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ///
    /// This function will return SRM version to application
    ////////////////////////////////////////////////////////////////////////////
    void GetSRMVersion(SocketData& data);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Set configuration for HDCP daemon.
    ///
    /// \param[in/out]  data    General message packet.
    /// \return         Nothing (Status is embedded in the SocketData structure)
    ////////////////////////////////////////////////////////////////////////////
    void Config(SocketData& data);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Report an event to the apps using the callback sockets
    ///
    /// \param[in]  event   Specific event we have encountered
    /// \param[in]  portId  Port on which the event has occurred
    /// \return     SUCCESS or errno
    ///
    /// Used to report hotplug events, link lost, etc.
    ////////////////////////////////////////////////////////////////////////////
    void ReportStatus(PORT_EVENT event, uint32_t portId);

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Test if the daemon construction succeeded.
    ///
    /// \return     true if successful, false otherwise
    ////////////////////////////////////////////////////////////////////////////
    bool IsValid(void) {return m_IsValid;}

    ////////////////////////////////////////////////////////////////////////////
    /// \brief      Get the daemon's internal messaging socket.
    ///
    /// \return     Reference to the socket
    ///
    /// This should only be used by test code. It should probably be removed!
    ////////////////////////////////////////////////////////////////////////////
    LocalServerSocket& GetSdkSocket(void) {return m_SdkSocket;}
};

#endif  // __HDCP_DAEMON_H__
