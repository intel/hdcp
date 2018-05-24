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
//! \file       servsock.h
//! \brief
//!

#ifndef __HDCP_SERVSOCK_H__
#define __HDCP_SERVSOCK_H__

#include <sys/un.h>
#include <poll.h>

#include "gensock.h"
#include "hdcpdef.h"

#define SESSION_COUNT_MAX       6

// The number of pending connections allowed should account for the total
// number of allowed sessions as well as the same number of potential requests
// to register callbacks.
#define SERV_SOCKET_BACKLOG     (SESSION_COUNT_MAX * 2)

struct SocketData;

class LocalServerSocket : public GenericStreamSocket
{
public:
    ///////////////////////////////////////////////////////////////////////////
    /// \brief  LocalServerSocket Constructor
    /// \par    Initializes member variables.
    ///
    /// \return None
    ///////////////////////////////////////////////////////////////////////////
    LocalServerSocket(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  LocalServerSocket Destructor
    ///
    /// \return None
    ///////////////////////////////////////////////////////////////////////////
    ~LocalServerSocket(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Listen
    /// \par    Places the socket in a state of listening for client requests.
    ///
    /// \return SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t Listen(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  GetSrmData
    /// \par    This call blocks until a client process writes raw data to this
    ///         socket. This should only be used to send a SRM buffer to the
    ///         Daemon.
    ///
    /// \param[out] data    Pointer to the data received
    /// \param[in]  dataSz  Number of bytes of data received
    /// \param[in]  appId   FileDescriptor used for the communication
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetSrmData(
                    uint8_t *data,
                    const int32_t dataSz,
                    const int32_t appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief SendResponse
    /// \par   Send a response to the client process via this socket.
    ///
    /// \param[in]  rsp     SocketData structure containing our response
    /// \param[in]  appId   FileDescriptor used for the communication
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t SendResponse(const SocketData& rsp, const int32_t appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief SendKsvListData
    /// \par   Send a ksvList to the client process via this socket.
    ///
    /// \param[in]  data    KsvList data
    /// \param[in]  dataSz  Size of ksvList in bytes
    /// \param[in]  appId   FileDescriptor used for the communication
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t SendKsvListData(
                        const uint8_t *data,
                        const int32_t dataSz,
                        const int32_t appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  GetTask
    /// \par    Wait for the next request from applications to come in through
    ///         the socket layer.
    ///
    /// \param[in]  req     SocketData structure containing the request
    /// \param[in]  appId   FileDescriptor used for the communication
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetTask(SocketData& req, int32_t& appId);

private:
    ///////////////////////////////////////////////////////////////////////////
    /// \brief  GetRequest
    /// \par    Read the request data through a specific socket.
    ///
    /// \param[out] req     SocketData structure containing the request
    /// \param[in]  appId   FileDescriptor used for the communication
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetRequest(SocketData& req, const int32_t appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  PollForEvent
    /// \par    Cycle over the 'poll' syscall until one of the file descriptors
    ///         indicates there is an event to handle.
    ///
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t PollForEvent(void);
    
    ///////////////////////////////////////////////////////////////////////////
    /// \brief  ProcessNewConnections
    /// \par    Cycle through and handle any pending connection requests.
    ///
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t ProcessNewConnections(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  SigCatcher
    /// \par    Callback handler for Terminal signal
    ///////////////////////////////////////////////////////////////////////////
    static void SigCatcher(int32_t sig);

private:
    bool            m_IsMainFdListening;

    struct pollfd   m_SessionFdArray[SESSION_COUNT_MAX];
    uint32_t        m_FdIndex;

    static bool     m_ReceivedKillSignal;
};

#endif  // __HDCP_SERVSOCK_H__
