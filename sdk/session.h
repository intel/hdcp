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
//! \file       session.h
//! \brief
//!

#ifndef __HDCP_SESSION_H__
#define __HDCP_SESSION_H__

#include <string>
#include <pthread.h>

#include "hdcpdef.h"
#include "socketdata.h"
#include "clientsock.h"
#include "servsock.h"

#define SOCKET_NAME_RETRY_MAX   10

class HdcpSession
{
public:
    //////////////////////////////////////////////////////////////////////////
    /// \brief      Construct the Session object
    ///
    /// \return     Nothing
    //////////////////////////////////////////////////////////////////////////
    HdcpSession(
            const uint32_t handle,
            const CallBackFunction func,
            const Context ctx);

    //////////////////////////////////////////////////////////////////////////
    /// \brief      Destroy the Session object
    ///
    /// \return     Nothing
    ///
    /// Attempts to delete the session will be blocked in the destructor if
    /// there are still active references on the Session. When the number of
    /// active references falls to 0, the destructor get's signalled to
    /// proceed.
    //////////////////////////////////////////////////////////////////////////
    ~HdcpSession();

    //////////////////////////////////////////////////////////////////////////
    /// \brief      Test if the daemon construction succeeded.
    ///
    /// \return     true if successful, false otherwise
    //////////////////////////////////////////////////////////////////////////
    bool IsValid(void) {return m_IsValid;}

    //////////////////////////////////////////////////////////////////////////
    /// \brief      Synchronously increase the number of active references.
    ///
    /// \return     Nothing
    //////////////////////////////////////////////////////////////////////////
    void IncreaseReference(void);

    //////////////////////////////////////////////////////////////////////////
    /// \brief      Synchronously decrease the number of active references.
    ///             Active references will block destruction of the session,
    ///             and once all references are released, a thread attempting
    ///             destruction will be allowed to proceed.
    ///
    /// \return     Nothing
    //////////////////////////////////////////////////////////////////////////
    void DecreaseReference(void);

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Open a new connection with the daemon's main socket
    ///
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INTERNAL
    ///             HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY
    ///
    /// Note, there is no corresponding Destroy call within the Session module
    /// because closing the socket (which is done when session's socket gets
    /// closed in the destructor) will create an event for the Daemon's main
    /// socket to operate on.
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS Create(void);

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Get the ports' connection statuses
    ///
    /// \param[out] PortList    Array to fill with ports' data
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INTERNAL
    ///             HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS EnumerateDisplay(PortList *PortList);

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Request enabling/disabing of HDCP on the specified port
    ///
    /// \param[in]  PortId      Id of the desired port
    /// \param[in]  level       0: disable HDCP,
    ///                         1: enable highest level
    ///                         2: force enable HDCP2.2
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INTERNAL
    ///             HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS SetProtectionLevel(const uint32_t portId, HDCP_LEVEL level);

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Get the port connection and encryption status
    ///
    /// \param[in]  PortId      Id of the desired port
    /// \param[out] portStatus  PORT_STATUS bitfield 
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INTERNAL
    ///             HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS GetStatus(const uint32_t portId, PORT_STATUS *portStatus);

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Get the ksv list of connected/enabled devices in HDCP1 topology
    ///
    /// \param[in]  portId      Id of the desired port
    /// \param[out] ksvList     ksv list of devices in HDCP1 topology
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INTERNAL
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS GetKsvList(
                        const uint32_t portId,
                        uint8_t *ksvCount,
                        uint8_t *depth,
                        uint8_t *ksvList);

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Send an SRM message to the daemon
    ///
    /// \param[in]  SrmSize     Size of the message in bytes
    /// \param[in]  srmData     Pointer to the byte array holding the message
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INTERNAL
    ///             HDCP_STATUS_ERROR_SRM_INVALID
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS SendSRMData(const uint32_t SrmSize, const uint8_t *psrmData);
    
//////////////////////////////////////////////////////////////////////////
    /// \brief  send GetSRMversion command to the daemon
    ///
    /// \param[out]  version     the value of current SRM data version.
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INTERNAL
    ///             HDCP_STATUS_ERROR_SRM_INVALID
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS GetSRMVersion(uint16_t *version);
    //////////////////////////////////////////////////////////////////////////
    /// \brief  Set configuration for HDCP daemon.
    ///
    /// \param[in]  config     the structure to contain configuration info.
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INTERNAL
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS Config(const HDCP_CONFIG config);

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Get the handle of this session
    ///
    /// \return     Handle belonging to this session
    //////////////////////////////////////////////////////////////////////////
    uint32_t GetHandle(void) {return m_Handle;}

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Get the CallBack function for this session
    ///
    /// \return     Function pointer responding to the callback
    //////////////////////////////////////////////////////////////////////////
    CallBackFunction GetCallBackFunction(void) {return m_CallBack;}

    //////////////////////////////////////////////////////////////////////////
    /// \brief  Get the object ctx pointer
    ///
    /// \return     Pointer pointing to the the application object of callback
    //////////////////////////////////////////////////////////////////////////
    void *GetContext(void) {return m_Context;}

private:
    //////////////////////////////////////////////////////////////////////////
    /// \brief  Send a request and receive a response in a synchronous manner
    ///
    /// \param[in]  data    SocketData holding the request details
    ///                     This will be filled with the response upon return
    /// \return     HDCP_STATUS_SUCCESSFUL
    ///             HDCP_STATUS_ERROR_INVALID_PARAMETER
    ///             HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY
    ///             HDCP_STATUS_ERROR_INTERNAL
    //////////////////////////////////////////////////////////////////////////
    HDCP_STATUS PerformMessageTransaction(SocketData &data);

    // Private member variables
    LocalClientSocket   m_SdkSocket;
    pthread_mutex_t     m_SocketMutex;

    CallBackFunction    m_CallBack;         // callback function pointer of app
    uint32_t            m_Handle;           // ctx handle
    void                *m_Context;          // object pointer of app
    bool                m_IsValid;

    uint32_t            m_ActiveReferences;
    pthread_mutex_t     m_ReferenceMutex;
    pthread_cond_t      m_ReferenceCV;
};

#endif  // __HDCP_SESSION_H__
