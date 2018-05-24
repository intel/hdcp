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
//! \file       gensock.h
//! \brief
//!

#ifndef __HDCP_GENSOCK_H__
#define __HDCP_GENSOCK_H__

#include <string>

#include "hdcpdef.h"

#define MAX_SRM_DATA_SZ         (6 * 1024)

class GenericStreamSocket
{
public:
    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Initializes member variables and creates a UNIX socket
    ///
    /// \return None
    ///
    /// Socket state is updated to SOCK_STATE_CREATE.
    ///////////////////////////////////////////////////////////////////////////
    GenericStreamSocket(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Closes the socket, and destroys the object.
    ///
    /// \return None
    ///////////////////////////////////////////////////////////////////////////
    ~GenericStreamSocket(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Tests if the file descriptor is valid.
    ///
    /// \return true if file descriptor is valid, false otherwise.
    ///////////////////////////////////////////////////////////////////////////
    bool IsValidDesc(void) {return (m_Fd >= 0);}

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Bind the socket to a UNIX address (file system node).
    ///
    /// \param[in]  path    Path of the socket file
    /// \return     SUCCESS or errno
    ///////////////////////////////////////////////////////////////////////////
    int32_t Bind(const std::string& path);

protected:
    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Clear the sockaddr instances and set filename path
    ///
    /// \param[out] addr    Pointer to the sockaddr_un object to initialize
    /// \param[in]  path    Path of the socket file
    /// \return     SUCCESS or errno
    ///////////////////////////////////////////////////////////////////////////
    int32_t InitSockAddr(struct sockaddr_un* addr, const std::string& path);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Read a specific amount of data with handling of signals and
    ///         partial reads.
    ///
    /// \param[in]  fd      FileDescriptor used for the communication
    /// \param[out] data    Pointer to the buffer to fill with data
    /// \param[in]  dataSz  Number of bytes to read
    /// \return     SUCCESS or errno
    ///////////////////////////////////////////////////////////////////////////
    int32_t ReadData(const int32_t fd, uint8_t *data, const int32_t dataSz);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Write a specific amount of data with handling of signals and
    ///         partial writes.
    ///
    /// \param[in]  fd      FileDescriptor used for the communication
    /// \param[in]  data    Pointer to the buffer containing with data
    /// \param[in]  dataSz  Number of bytes to write
    /// \return     SUCCESS or errno
    ///////////////////////////////////////////////////////////////////////////
    int32_t WriteData(
                    const int32_t fd,
                    const uint8_t *data,
                    const int32_t dataSz);

protected:
    int32_t m_Domain;
    int32_t m_Type;
    int32_t m_Protocol;
    int32_t m_Fd;
};

#endif // __HDCP_GENSOCK_H__
