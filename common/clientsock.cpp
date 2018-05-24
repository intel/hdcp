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
//! \file       clientsock.cpp
//! \brief
//!

#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <linux/un.h>

#include "clientsock.h"
#include "hdcpdef.h"
#include "socketdata.h"

LocalClientSocket::LocalClientSocket(void)
{
}

LocalClientSocket::~LocalClientSocket(void)
{
}

int32_t LocalClientSocket::Connect(const std::string& path)
{
    HDCP_FUNCTION_ENTER;

    struct sockaddr_un addr = {};
    int32_t ret = InitSockAddr(&addr, path);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Failed to initialize socket address!");
        return ret;
    }

    while (true)
    {
        ret = connect(
                m_Fd,
                reinterpret_cast<struct sockaddr *>(&addr),
                sizeof(addr));
        if (ERROR == ret)
        {
            if (EINTR == errno)
            {
                continue;
            }

            if (EISCONN == errno)
            {
                // This is actually Ok, but there are whole articles written
                // on "why".
                break;
            }

            // Some other err...
            HDCP_ASSERTMESSAGE(
                        "Failed to connect to socket! Err: %s",
                        strerror(errno));

            return errno;
        }

        // It worked!
        break;
    }

    // It connected, but the daemon might immediately close if all the sessions
    // are consumed. Read a response back to verify:
    SocketData response;
    ret = GetMessage(response);
    if (ENOTCONN == ret)
    {
        // The daemon closed the socket, let the caller know it was refused
        HDCP_ASSERTMESSAGE("The daemon refused the connection!");
        ret = ECONNREFUSED;
    }
    else if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to get connection verification! Err: %s",
                    strerror(ret));
    }
    else if (HDCP_STATUS_SUCCESSFUL != response.Status)
    {
        // This makes absolutely no sense, but maybe in the future someone
        // changes it without updating the message?
        HDCP_ASSERTMESSAGE("Received bad response for connection verification!");
        ret = EBADMSG;
    }

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t LocalClientSocket::SendMessage(const SocketData& req)
{
    HDCP_FUNCTION_ENTER;

    int32_t ret = WriteData(m_Fd, &req.Bytes, sizeof(req));

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t LocalClientSocket::SendSrmData(
                                const uint8_t *data,
                                const int32_t dataSz)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(data, EINVAL);

    // Bit of a sanity check here
    if (dataSz > MAX_SRM_DATA_SZ)
    {
        HDCP_ASSERTMESSAGE(
                "Size to send %d is larger than maximum allowed srm size %d",
                dataSz,
                MAX_SRM_DATA_SZ);
        return EMSGSIZE;
    }

    int32_t ret = WriteData(m_Fd, data, dataSz);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t LocalClientSocket::ReceiveKsvList(
                                uint8_t *ksvList,
                                const uint8_t ksvCount)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(ksvList, EINVAL);

    // Sanity check on the input
    if (ksvCount > MAX_KSV_COUNT || ksvCount == 0)
    {
        HDCP_ASSERTMESSAGE(
            "Number of ksvs to receive %d is invalid! Maximum allowed is %d",
            ksvCount,
            MAX_KSV_COUNT);
        return EMSGSIZE;
    }

    // Get the data
    int32_t ret = ReadData(m_Fd, ksvList, ksvCount * KSV_SIZE);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t LocalClientSocket::GetMessage(SocketData& rsp)
{
    HDCP_FUNCTION_ENTER;

    static_assert(
            sizeof(rsp) <= SSIZE_MAX,
            "response size is greater than read() can handle!");

    int32_t ret = ReadData(m_Fd, &rsp.Bytes, sizeof(rsp));

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}
