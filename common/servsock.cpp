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
//! \file       servsock.cpp
//! \brief
//!

#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <poll.h>
#include <signal.h>
#include <string.h>

#include "servsock.h"
#include "hdcpdef.h"
#include "socketdata.h"

bool LocalServerSocket::m_ReceivedKillSignal = false;

LocalServerSocket::LocalServerSocket(void) :
                    m_IsMainFdListening(false),
                    m_FdIndex(0)
{
    HDCP_FUNCTION_ENTER;

    uint32_t            i       = 0;
    struct sigaction    actions = {};

    // The server socket will block (intentionally) waiting for tasks to come
    // through (on the main listener socket for hdcpd), but there are
    // requirements for the daemon to exit gracefully, so we must catch the
    // SIGTERM signal and set a shutdown flag asynchronously.
    // This requires the 'poll' call to check the flag after receiving a result
    // of EINTR.
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = SigCatcher;
    sigaction(SIGTERM, &actions, nullptr);

    memset(m_SessionFdArray, 0, sizeof(m_SessionFdArray));
    for (i = 0; i < SESSION_COUNT_MAX; ++i)
    {
        m_SessionFdArray[i].fd = -1;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

LocalServerSocket::~LocalServerSocket(void)
{
}

void LocalServerSocket::SigCatcher(int32_t sig)
{
    HDCP_FUNCTION_ENTER;

    if (SIGTERM == sig)
    {
        // Set the clean-up variable so we can tear down nicely
        m_ReceivedKillSignal = true;
    }
    else
    {
        HDCP_ASSERTMESSAGE("Received unknown signal %d", sig);
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
}

int32_t LocalServerSocket::GetRequest(SocketData& req, const int32_t fd)
{
    HDCP_FUNCTION_ENTER;

    int32_t ret = ReadData(fd, &req.Bytes, sizeof(req));

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t LocalServerSocket::SendResponse(const SocketData& rsp, const int32_t fd)
{
    HDCP_FUNCTION_ENTER;

    int32_t ret = WriteData(fd, &rsp.Bytes, sizeof(rsp));

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t LocalServerSocket::SendKsvListData(
                                const uint8_t *data,
                                const int32_t dataSz,
                                const int32_t fd)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(data, EINVAL);

    // Bit of a sanity check here
    if (dataSz > (KSV_SIZE * MAX_KSV_COUNT))
    {
        HDCP_ASSERTMESSAGE(
                "Size to send %d is larger than maximum allowed srm size %d",
                dataSz,
                KSV_SIZE * MAX_KSV_COUNT);
        return EMSGSIZE;
    }

    int32_t ret = WriteData(fd, data, dataSz);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t LocalServerSocket::Listen(void)
{
    HDCP_FUNCTION_ENTER;

    int32_t ret = listen(m_Fd, SERV_SOCKET_BACKLOG);
    if (ERROR == ret)
    {
        HDCP_ASSERTMESSAGE("Failed to listen! Err: %s", strerror(errno));
        return errno;
    }

    // Set the fd in our array
    m_SessionFdArray[0].fd = m_Fd;
    m_SessionFdArray[0].events = POLLIN;

    m_IsMainFdListening = true;

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t LocalServerSocket::ProcessNewConnections(void)
{
    HDCP_FUNCTION_ENTER;

    int32_t     ret         = EINVAL;
    uint32_t    emptyIndex  = 0;
    int32_t     incomingFd  = 0; // Invalid would be -1, but 0 enters the loop
    int32_t     i           = -1;

    while (incomingFd != -1)
    {
        incomingFd = accept(m_Fd, nullptr, nullptr);
        if (ERROR == incomingFd)
        {
            ret = errno;
            if (EWOULDBLOCK == ret)
            {
                // Normally, accept would block until it received a connection,
                // but the base class's bind call includes setting the socket to
                // nonblocking. In this case the accept call returns right away
                // with this EWOULDBLOCK status.

                // Got all of the connections that were queued up!
                ret = SUCCESS;
                break;
            }

            // Non fatal errors:
            if ((EINTR == ret)          ||
                (ECONNABORTED == ret))
            {
                // Go back to the accept above and try again
                continue;
            }

            HDCP_ASSERTMESSAGE("Failed to accept! Err: %s", strerror(ret));
            break; // REVIEW: quit or move on?
        }

        // We successfuly accepted the new connection and need to find a place
        // to put it now
        for (i = emptyIndex; i < SESSION_COUNT_MAX; ++i)
        {
            if (-1 == m_SessionFdArray[i].fd)
            {
                // This fd slot is not in use
                m_SessionFdArray[i]         = {};
                m_SessionFdArray[i].fd      = incomingFd;
                m_SessionFdArray[i].events  = POLLIN;
                break;
            }
        }
        emptyIndex = i;

        if (SESSION_COUNT_MAX == i)
        {
            // We didn't find an empty session

            // Don't send a response, just close it. The client should be
            // waiting to receive the "success" response, but will detect the
            // socket closure to know the connection was rejected.
            close(incomingFd);
            incomingFd = -1;
            HDCP_WARNMESSAGE("Refused a new session due to space!");
            continue;
        }

        // Send the client a message indicating we had room for the connection
        SocketData response;    // SocketData initializes within constructor
        response.Size       = sizeof(response);
        response.Command    = HDCP_API_CREATE;
        response.Status     = HDCP_STATUS_SUCCESSFUL;
        SendResponse(response, incomingFd);
    }

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t LocalServerSocket::PollForEvent(void)
{
    HDCP_FUNCTION_ENTER;

    int32_t ret = EINVAL;

    // We don't want to enter a "poll" wait unless we have our listening
    // thread active
    if (!m_IsMainFdListening)
    {
        return EPROTO;
    }

    while (true)
    {
        if (m_ReceivedKillSignal)
        {
            return ECANCELED;
        }

        ret = poll(m_SessionFdArray, SESSION_COUNT_MAX, -1);
        if (ERROR == ret)
        {
            if (EINTR == errno)
            {
                // It happens...
                continue;
            }

            HDCP_ASSERTMESSAGE("Failed to poll! Err: %s", strerror(errno));
            return errno;
        }
        else if (0 == ret)
        {
            HDCP_WARNMESSAGE("Poll timed out before receiving an event");
            continue;
        }
        else
        {
            // poll returned a result greater than 0 (a real event)
            break;
        }
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t LocalServerSocket::GetTask(SocketData& req, int32_t& appId)
{
    HDCP_FUNCTION_ENTER;

    int32_t ret = EINVAL;
    uint32_t i   = 0;

    appId = -1;

    // No need to zero-out the "req" first. This was done when the SocketData
    // type was instantiated.

    static_assert(
                sizeof(req) <= SSIZE_MAX,
                "request size is greater than read() can handle!");

    while (-1 == appId)
    {
        // Block on poll until we get an event
        ret = PollForEvent();
        if (SUCCESS != ret)
        {
            HDCP_ASSERTMESSAGE("Failed to poll for event!");
            return ret;
        }

        // An event occurred, check the fd's
        // to prevent overflow, use add instead of deduce
        for (i = m_FdIndex;
            i != (m_FdIndex +(SESSION_COUNT_MAX -1)) % SESSION_COUNT_MAX;
            i = (i + 1) % SESSION_COUNT_MAX)
        {
            // A little future proofing
            if ((m_FdIndex >= SESSION_COUNT_MAX) || (i >= SESSION_COUNT_MAX))
            {
                // This should never happen in release. This would only occur
                // if a developer altered the loop logic here or changed
                // m_FdIndex somewhere outside of this function.
                // (That's why this fails out instead of resetting the index)
                HDCP_ASSERTMESSAGE(" Index out of session array bounds!");
                return ERANGE;
            }

            // This fd slot is not in use
            // Nothing happened on this fd
            if ((-1 == m_SessionFdArray[i].fd)      ||
                (0 == m_SessionFdArray[i].revents))
            {
                continue;
            }

            if (((POLLIN & m_SessionFdArray[i].revents) == 0)       &&
                ((POLLHUP & m_SessionFdArray[i].revents) == 0))
            {
                HDCP_WARNMESSAGE(
                        "Received unexpected event on fd %d, event 0x%x",
                        m_SessionFdArray[i].fd,
                        m_SessionFdArray[i].revents);
                // This should just continue; It's a DoS if we actually quit!
                continue;
            }

            if (m_Fd == m_SessionFdArray[i].fd)
            {
                // This is the fd of the main listener
                ret = ProcessNewConnections();
                if (SUCCESS != ret)
                {
                    // If this fails, something is fatally wrong with the main
                    // listener socket. Destroy the daemon!
                    HDCP_ASSERTMESSAGE("Listener socket critically failed!");
                    return ret;
                }
                continue;
            }

            // This request will go on for processing, give it the fd's id
            appId = m_SessionFdArray[i].fd;

            ret = GetRequest(req, appId);
            if (SUCCESS != ret)
            {
                if (ENOTCONN != ret)
                {
                    // We only  warn if it isn't an intentional disconnect
                    HDCP_ASSERTMESSAGE("Failed to check the status of a fd!");
                }

                req.Size       = sizeof(SocketData);
                req.Command    = HDCP_API_DESTROY;
                close(m_SessionFdArray[i].fd);
            }

            // There are a couple of API's that would cause us to free up the
            // spot in our list of fd's
            if ((HDCP_API_CREATE_CALLBACK == req.Command)   ||
                (HDCP_API_DESTROY == req.Command))
            {
                m_SessionFdArray[i] = {};
                m_SessionFdArray[i].fd = -1;
            }

            // Found an event, so stop searching
            break;
        }

        // Increment m_FdIndex so we use a round-robin approach to handling
        // the requests.
        m_FdIndex = i;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t LocalServerSocket::GetSrmData(
                            uint8_t *data,
                            const int32_t dataSz,
                            const int32_t appId)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(data, EINVAL);

    // For security and sanity it should be restricted to the maximum size of
    // the message we would ever want to handle.
    // So, that's why we check MAX_SRM_DATA_SZ here.
    if (dataSz > MAX_SRM_DATA_SZ)
    {
        HDCP_ASSERTMESSAGE(
                    "Desired size %d is greater than maximum srm size %d",
                    dataSz,
                    MAX_SRM_DATA_SZ);
        return EMSGSIZE;
    }

    static_assert(
                MAX_SRM_DATA_SZ <= SSIZE_MAX,
                "max request size is greater than read() can handle!");

    int32_t ret = ReadData(appId, data, dataSz);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}
