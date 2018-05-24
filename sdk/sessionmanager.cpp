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
//! \file       sessionmanager.cpp
//! \brief
//!

#include <forward_list>
#include <list>
#include <new>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "sessionmanager.h"
#include "hdcpdef.h"
#include "hdcpapi.h"
#include "socketdata.h"
#include "gensock.h"
#include "servsock.h"
#include "clientsock.h"
#include "session.h"

std::forward_list<HdcpSession *> HdcpSessionManager::m_SessionList;
std::list<HdcpSession *>         HdcpSessionManager::m_PendingDestroyQueue;

CallBackFunction    HdcpSessionManager::m_CallBack = nullptr;
LocalClientSocket   *HdcpSessionManager::m_CallBackSocket = nullptr;
bool                HdcpSessionManager::m_IsCallBackInitialized = false;

pthread_t           HdcpSessionManager::m_SocketThread;

// Avoid starting the incrementer at 0 since some implementers might
// initialize their data with 0.
uint32_t            HdcpSessionManager::m_HandleIncrementor = 1;
pthread_mutex_t     HdcpSessionManager::m_HandleIncrementorMutex = PTHREAD_MUTEX_INITIALIZER;

uint32_t            HdcpSessionManager::m_IteratorReference = 0;
pthread_mutex_t     HdcpSessionManager::m_IteratorMutex = PTHREAD_MUTEX_INITIALIZER;


void HdcpSessionManager::IteratorCriticalSectionEnter(void)
{
    HDCP_FUNCTION_ENTER;

    ACQUIRE_LOCK(&m_IteratorMutex);
    ++m_IteratorReference;
    RELEASE_LOCK(&m_IteratorMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpSessionManager::IteratorCriticalSectionExit(void)
{
    HDCP_FUNCTION_ENTER;

    ACQUIRE_LOCK(&m_IteratorMutex);
    --m_IteratorReference;

    if (0 == m_IteratorReference)
    {
        // Destroy each session that might have been queued up for
        // destruction while threads were iterating
        for (auto session : m_PendingDestroyQueue)
        {
            delete session;
            m_SessionList.remove(session);
        }
        m_PendingDestroyQueue.clear();
    }
    RELEASE_LOCK(&m_IteratorMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

uint32_t HdcpSessionManager::GetUniqueHandle(void)
{
    HDCP_FUNCTION_ENTER;

    uint32_t ret = BAD_SESSION_HANDLE;

    ACQUIRE_LOCK(&m_HandleIncrementorMutex);
    ret = m_HandleIncrementor++;
    RELEASE_LOCK(&m_HandleIncrementorMutex);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t HdcpSessionManager::InitCallBack(void)
{
    HDCP_FUNCTION_ENTER;

    if (m_IsCallBackInitialized)
    {
        // Thread is already launched
        return SUCCESS;
    }

    m_CallBackSocket = new (std::nothrow) LocalClientSocket;
    if (nullptr == m_CallBackSocket)
    {
        return ENOMEM;
    }

    int32_t ret = m_CallBackSocket->Connect(HDCP_SDK_SOCKET_PATH);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Failed to Connect!");
        return ret;
    }

    SocketData  data;
    data.Size       = sizeof(SocketData);
    data.Command    = HDCP_API_CREATE_CALLBACK;

    ret = m_CallBackSocket->SendMessage(data);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("SendMessage failed for creating Callback!");
        return ret;
    }

    ret = pthread_create(&m_SocketThread,
            nullptr,
            reinterpret_cast<void* (*)(void*)>(CallBackManager),
            nullptr);
    if (0 != ret)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to create callback handler thread! Err: %s",
                    strerror(ret));
        delete m_CallBackSocket;
        m_CallBackSocket = nullptr;
        return errno;
    }

    m_IsCallBackInitialized = true;

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

void HdcpSessionManager::DestroyCallback(void)
{
    HDCP_FUNCTION_ENTER;

    ACQUIRE_LOCK(&m_IteratorMutex);

    while (!m_SessionList.empty())
    {
        delete m_SessionList.front();
        m_SessionList.remove(m_SessionList.front());
    }

    delete m_CallBackSocket;
    m_CallBackSocket = nullptr;
    m_CallBack = nullptr;

    RELEASE_LOCK(&m_IteratorMutex);

    // REVIEW: pthread_join here for callback thread?
    //pthread_join(m_SocketThread, nullptr);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

uint32_t HdcpSessionManager::CreateSession(
                                const CallBackFunction func,
                                const Context ctx)
{
    HDCP_FUNCTION_ENTER;

    uint32_t handle = GetUniqueHandle();

    // Check if the call back has been created
    // This is a 0->1 _ONLY_ transition. Checking here is safe without
    // a lock as long as we are guaranteed to lock and check state before
    // actually initializing.
    if (!m_IsCallBackInitialized)
    {
        ACQUIRE_LOCK(&m_HandleIncrementorMutex);
        int32_t sts = InitCallBack();
        RELEASE_LOCK(&m_HandleIncrementorMutex);

        if (SUCCESS != sts)
        {
            HDCP_ASSERTMESSAGE("Failed to init callback when first create!");
            return BAD_SESSION_HANDLE;
        }
    }

    HdcpSession *session = new (std::nothrow) HdcpSession(handle, func, ctx);
    if (nullptr == session)
    {
        return BAD_SESSION_HANDLE;
    }

    if (!session->IsValid())
    {
        delete session;
        return BAD_SESSION_HANDLE;
    }

    ACQUIRE_LOCK(&m_IteratorMutex);
    m_SessionList.push_front(session);
    RELEASE_LOCK(&m_IteratorMutex);

    HDCP_FUNCTION_EXIT(handle);
    return handle;
}

void HdcpSessionManager::DestroySession(const uint32_t handle)
{
    HDCP_FUNCTION_ENTER;

    HdcpSession* session = nullptr;

    ACQUIRE_LOCK(&m_IteratorMutex);

    for (auto tempSession : m_SessionList)
    {
        if (handle == tempSession->GetHandle())
        {
            session = tempSession;
            break;
        }
    }

    if (nullptr != session)
    {
        if (m_IteratorReference > 0)
        {
            m_PendingDestroyQueue.push_back(session);
        }
        else
        {
            // It is safe to manipulate the list memory, so free it now
            delete session;
            m_SessionList.remove(session);
        }
    }

    RELEASE_LOCK(&m_IteratorMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

HdcpSession* HdcpSessionManager::GetInstance(const uint32_t handle)
{
    HDCP_FUNCTION_ENTER;

    HdcpSession* session = nullptr;

    IteratorCriticalSectionEnter();
    for (auto tempSession : m_SessionList)
    {
        if (handle == tempSession->GetHandle())
        {
            session = tempSession;
            break;
        }
    }

    if (nullptr == session)
    {
        IteratorCriticalSectionExit();
        return nullptr;
    }

    // Check if the handle has been added to the destroy queue
    // This isn't critical for thread safety, it is just an attempt
    // to improve QoS slightly.
    // No elements of the destroy queue could be deleted as long as
    // this thread holds a reference to the iterator CS, so there
    // is no data race around that list.
    for (auto tempSession : m_PendingDestroyQueue)
    {
        if (tempSession == session)
        {
            IteratorCriticalSectionExit();
            return nullptr;
        }
    }

    // We found the session and it is not queued up for removal
    session->IncreaseReference();

    IteratorCriticalSectionExit();
    HDCP_FUNCTION_EXIT(SUCCESS);
    return session;
}

void HdcpSessionManager::PutInstance(const uint32_t handle)
{
    HDCP_FUNCTION_ENTER;

    IteratorCriticalSectionEnter();
    for (auto session : m_SessionList)
    {
        if (handle == session->GetHandle())
        {
            session->DecreaseReference();
            break;
        }
    }
    IteratorCriticalSectionExit();

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void *HdcpSessionManager::CallBackManager(void *dummy)
{
    HDCP_FUNCTION_ENTER;

    SocketData  data;
    int32_t     sts = EINVAL;

    while (true)
    {
        data = {};

        sts = m_CallBackSocket->GetMessage(data);
        if (SUCCESS != sts)
        {
            if (ENOTCONN == sts)
            {
                // The connection has disconnected and we should exit
                DestroyCallback(); 
                break;
            }

            HDCP_ASSERTMESSAGE("GetMessage failed on callback socket!");
            continue;
        }

        if (data.Size != sizeof(data))
        {
            HDCP_ASSERTMESSAGE(
                "Received a message with invalid size %d on callback socket!",
                data.Size);
            continue;
        }

        if (HDCP_API_REPORTSTATUS != data.Command)
        {
            HDCP_WARNMESSAGE("Received an unknown request on callback socket!");
            continue;
        }

        // Execute the callback function for each session handle we have
        IteratorCriticalSectionEnter();
        for (auto session : m_SessionList)
        {
            if (nullptr != session->GetCallBackFunction())
            {
                (session->GetCallBackFunction())(session->GetHandle(),
                                        data.SinglePort.Id,
                                        data.SinglePort.Event,
                                        session->GetContext());
            }
        }
        IteratorCriticalSectionExit();
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return nullptr;
}
