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
//! \file       clientsock.h
//! \brief
//!

#ifndef __HDCP_CLIENTSOCK_H__
#define __HDCP_CLIENTSOCK_H__

#include "gensock.h"
#include "hdcpdef.h"
#include "socketdata.h"

struct SocketData;

class LocalClientSocket : public GenericStreamSocket
{
public:
    ///////////////////////////////////////////////////////////////////////////
    /// \brief LocalClientSocket Constructor
    ///
    /// \return None
    ///////////////////////////////////////////////////////////////////////////
    LocalClientSocket(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief LocalClientSocket Destructor
    ///
    /// \return None
    ///////////////////////////////////////////////////////////////////////////
    ~LocalClientSocket(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Request a connection with the socket.
    ///
    /// \param[in]  path   Socket file path for connection
    /// \return     SUCCESS or errno otherwise
    ///
    /// When this fails it leaves the fd open. It is the caller's
    /// responsibility to properly close the fd.
    ///////////////////////////////////////////////////////////////////////////
    int32_t Connect(const std::string& path);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send a request to the server process via this socket.
    ///
    /// \param[in]  req     SocketData packet to send to the server
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t SendMessage(const SocketData& req);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Send SRM data the server process via this socket.
    ///
    /// \param[in]  data    Buffer containing SRM message
    /// \param[in]  dataSz  Size of the SRM message
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t SendSrmData(const uint8_t *data, const int32_t dataSz);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Receive KsvList data from the socket.
    ///
    /// \param[in]  ksvList    Buffer containing ksvList.
    /// \param[in]  ksvCount   Number of ksv in ksvList (1 ksv = 5 bytes).
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t ReceiveKsvList(uint8_t *ksvList, const uint8_t ksvCount);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Blocks until the server writes a response to this socket.
    ///        The response is copied to 'rsp' on success.
    ///
    /// \param[out] rsp     SocketData packet received from the server
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetMessage(SocketData& rsp);
};

#endif  // __HDCP_CLIENTSOCK_H__
