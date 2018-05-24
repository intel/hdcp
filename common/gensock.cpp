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
//! \file       gensock.cpp
//! \brief
//!

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <linux/limits.h>
#include <string>
#include <fcntl.h>

#include "gensock.h"
#include "hdcpdef.h"

GenericStreamSocket::GenericStreamSocket(void) :
                            m_Domain(PF_LOCAL),
                            m_Type(SOCK_STREAM),
                            m_Protocol(0),
                            m_Fd(-1)
{
    HDCP_FUNCTION_ENTER;

    m_Fd = socket(m_Domain, m_Type, m_Protocol);
    if (0 > m_Fd)
    {
        HDCP_ASSERTMESSAGE("Failed to create a new socket!");
        m_Fd = -1;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

GenericStreamSocket::~GenericStreamSocket(void)
{
    HDCP_FUNCTION_ENTER;

    if (0 <= m_Fd)
    {
        if (ERROR == close(m_Fd))
        {
            HDCP_ASSERTMESSAGE(
                    "Failed to close socket file! Err: %s",
                    strerror(errno));
        }

        m_Fd = -1;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

int32_t GenericStreamSocket::Bind(const std::string& path)
{
    HDCP_FUNCTION_ENTER;

    int32_t             ret             = EINVAL;
    struct sockaddr_un  addr            = {};
    const uint32_t      sockOptReusable = 1;
    int32_t             fdFlags         = -1;

    ret = InitSockAddr(&addr, path);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Failed to initialize the socket address!");
        return EINVAL;
    }

    // Try to cleanup the file in case it happened to already exist
    ret = unlink(addr.sun_path);
    if (ERROR == ret)
    {
        if (errno != ENOENT)
        {
            HDCP_ASSERTMESSAGE("Unlink failed! Err: %s", strerror(ret));
            // In this case, there was an error and it wasn't the
            // harmless ENOENT one. We should respond to this error.
            return errno;
        }
    }

    // Set the socket to be re-usable
    ret = setsockopt(
            m_Fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &sockOptReusable,
            sizeof(sockOptReusable));
    if (ERROR == ret)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to set the socket options. Err: %s",
                    strerror(errno));
        return errno;
    }

    // Do a RMW on the file status flags to set non-blocking
    fdFlags = fcntl(m_Fd, F_GETFL);
    if (ERROR == fdFlags)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to get the file status flags. Err: %s",
                    strerror(errno));
        return errno;
    }

    ret = fcntl(m_Fd, F_SETFL, fdFlags | O_NONBLOCK);
    if (ERROR == ret)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to set the file status flags. Err: %s",
                    strerror(errno));
        return errno;
    }

    ret = bind(m_Fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ERROR == ret)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to bind to the socket! Err: %s",
                    strerror(errno));
        return errno;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t GenericStreamSocket::InitSockAddr(
                                struct sockaddr_un* addr,
                                const std::string& path)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(addr, EINVAL);

    // There is an assumption that sun_path has a shorter max length than
    // MAX_PATH. If this is not true in the future, then someone needs
    // to adjust our max length check here!
    const size_t pathLimit = sizeof(addr->sun_path);

    static_assert(
                pathLimit <= PATH_MAX,
                "ERROR: Our max path assumptions are no longer valid!");

    if (path.length() > (pathLimit - 1)) // (-1) for the nullptr terminator
    {
        HDCP_ASSERTMESSAGE(
                    "Path length %zu exceeds limit %zu",
                    path.length(),
                    pathLimit - 1);
        return ENAMETOOLONG;
    }

    memset(addr, 0, sizeof(sockaddr_un));
    addr->sun_family = AF_UNIX;

    path.copy(addr->sun_path, pathLimit);
    if (0 != addr->sun_path[pathLimit - 1])
    {
        HDCP_ASSERTMESSAGE("Copied path does not end in a nullptr terminator!");

        addr->sun_path[pathLimit - 1] = 0;
        return EINVAL;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t GenericStreamSocket::ReadData(
                                const int32_t fd,
                                uint8_t *data,
                                const int32_t dataSz)
{
    HDCP_FUNCTION_ENTER;

    ssize_t count           = 0;
    ssize_t bytesRemaining  = dataSz;
    uint32_t offset          = 0;

    if ((-1 == fd)      ||
        (nullptr == data))
    {
        return EINVAL;
    }

    while (bytesRemaining > 0)
    {
        count = read(fd, &data[offset], bytesRemaining);
        if (-1 == count)
        {
            if ((EINTR == errno) || (EAGAIN == errno))
            {
                continue;
            }

            HDCP_ASSERTMESSAGE("Failed to read! Err: %s", strerror(errno));
            return errno;
        }

        if (0 == count)
        {
            HDCP_NORMALMESSAGE("Success to read, but the content is empty!");
            return ENOTCONN;
        }

        bytesRemaining -= count;
        offset += count;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t GenericStreamSocket::WriteData(
                                const int32_t fd,
                                const uint8_t *data,
                                const int32_t dataSz)
{
    HDCP_FUNCTION_ENTER;

    if ((-1 == fd)      ||
        (nullptr == data))
    {
        return EINVAL;
    }

    ssize_t bytesRemaining  = dataSz;
    uint32_t offset         = 0;

    while (bytesRemaining > 0)
    {
        ssize_t count = send(fd, &data[offset], bytesRemaining, MSG_NOSIGNAL);
        if (-1 == count)
        {
            if ((EINTR == errno) || (EAGAIN == errno))
            {
                continue;
            }

            HDCP_ASSERTMESSAGE("Failed to send! Err: %s", strerror(errno));
            return errno;
        }

        bytesRemaining -= count;
        offset += count;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}
