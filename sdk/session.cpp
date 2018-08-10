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
//! \file       session.cpp
//! \brief
//!

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "session.h"
#include "hdcpdef.h"
#include "socketdata.h"
#include "gensock.h"
#include "servsock.h"
#include "clientsock.h"
#include "sessionmanager.h"

FILE*  dmLog = nullptr;

HdcpSession::HdcpSession(
                    const uint32_t handle,
                    const CallBackFunction func,
                    const Context ctx) :
    m_CallBack(func),
    m_Handle(handle),
    m_Context(ctx),    
    m_IsValid(true),
    m_ActiveReferences(0)
{
    HDCP_FUNCTION_ENTER;

#ifdef HDCP_LOG_FILE
    if (dmLog == nullptr)
    {
        dmLog = fopen(HDCP_LOG_FILE, "w");
    }
#endif

    if ((SUCCESS != pthread_mutex_init(&m_ReferenceMutex, nullptr)) ||
        (SUCCESS != pthread_cond_init(&m_ReferenceCV, nullptr))     ||
        (SUCCESS != pthread_mutex_init(&m_SocketMutex, nullptr)))
    {
        m_IsValid = false;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return;
}

HdcpSession::~HdcpSession()
{
    HDCP_FUNCTION_ENTER;

    ACQUIRE_LOCK(&m_ReferenceMutex);
    if (m_ActiveReferences > 0)
    {
        WAIT_CV(&m_ReferenceCV, &m_ReferenceMutex);
    }
    RELEASE_LOCK(&m_ReferenceMutex);

    DESTROY_LOCK(&m_ReferenceMutex);
    DESTROY_CV(&m_ReferenceCV);

    DESTROY_LOCK(&m_SocketMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpSession::IncreaseReference(void)
{
    HDCP_FUNCTION_ENTER;

    ACQUIRE_LOCK(&m_ReferenceMutex);
    ++m_ActiveReferences;
    RELEASE_LOCK(&m_ReferenceMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

void HdcpSession::DecreaseReference(void)
{
    HDCP_FUNCTION_ENTER;

    ACQUIRE_LOCK(&m_ReferenceMutex);
    --m_ActiveReferences;

    if (m_ActiveReferences == 0)
    {
        pthread_cond_signal(&m_ReferenceCV);
    }
    RELEASE_LOCK(&m_ReferenceMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
}

HDCP_STATUS HdcpSession::Create(void)
{
    HDCP_FUNCTION_ENTER;

    // Connect to the daemon socket
    ACQUIRE_LOCK(&m_SocketMutex);
    int32_t sts = m_SdkSocket.Connect(HDCP_SDK_SOCKET_PATH);
    RELEASE_LOCK(&m_SocketMutex);

    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("Failed to connect to daemon socket!");
        return HDCP_STATUS_ERROR_MSG_TRANSACTION;
    }

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

HDCP_STATUS HdcpSession::PerformMessageTransaction(SocketData &data)
{
    HDCP_FUNCTION_ENTER;

    // Send the request to daemon
    int32_t sts = m_SdkSocket.SendMessage(data);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("Failed to send request to daemon!");
        return HDCP_STATUS_ERROR_MSG_TRANSACTION;
    }

    // Get reply from daemon
    sts = m_SdkSocket.GetMessage(data);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE("Failed to get response from daemon!");
        return HDCP_STATUS_ERROR_MSG_TRANSACTION;
    }

    HDCP_FUNCTION_EXIT(data.Status);
    return data.Status;
}

HDCP_STATUS HdcpSession::EnumerateDisplay(PortList *portList)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(portList, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    SocketData  data;

    data.Size       = sizeof(SocketData);
    data.Command    = HDCP_API_ENUMERATE_HDCP_DISPLAY;

    ACQUIRE_LOCK(&m_SocketMutex);
    HDCP_STATUS ret = PerformMessageTransaction(data);
    RELEASE_LOCK(&m_SocketMutex);

    if (HDCP_STATUS_SUCCESSFUL != ret)
    {
        HDCP_ASSERTMESSAGE("Message transactions failed!");
        return ret;
    }

    if (data.PortCount > NUM_PHYSICAL_PORTS_MAX)
    {
        HDCP_ASSERTMESSAGE(
                    "Port count returned %d exceeds physical port abilities %d",
                    data.PortCount,
                    NUM_PHYSICAL_PORTS_MAX);
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    // Fill the port information
    portList->PortCount = data.PortCount;
    for (uint32_t i = 0; i < data.PortCount; ++i)
    {
        portList->Ports[i].Id     = data.Ports[i].Id;
        portList->Ports[i].status = data.Ports[i].status;
    }

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

HDCP_STATUS HdcpSession::SetProtectionLevel(
                            const uint32_t portId,
                            const HDCP_LEVEL level)
{
    HDCP_FUNCTION_ENTER;

    SocketData  data;

    data.Size           = sizeof(SocketData);
    data.Command        = HDCP_API_SET_PROTECTION_LEVEL;
    data.PortCount      = 1;
    data.SinglePort.Id  = portId;
    data.Level          = level;

    ACQUIRE_LOCK(&m_SocketMutex);
    HDCP_STATUS ret = PerformMessageTransaction(data);
    RELEASE_LOCK(&m_SocketMutex);

    if (HDCP_STATUS_SUCCESSFUL != ret && (level != HDCP_LEVEL0))
    {
        HDCP_ASSERTMESSAGE("Message transactions failed!");
        SetProtectionLevel(portId, HDCP_LEVEL0);
        return ret;
    }

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

HDCP_STATUS HdcpSession::GetStatus(
                            const uint32_t portId,
                            PORT_STATUS *portStatus)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(portStatus, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    SocketData  data;

    data.Size           = sizeof(SocketData);
    data.Command        = HDCP_API_GETSTATUS;
    data.PortCount      = 1;
    data.SinglePort.Id  = portId;

    ACQUIRE_LOCK(&m_SocketMutex);
    HDCP_STATUS ret = PerformMessageTransaction(data);
    RELEASE_LOCK(&m_SocketMutex);

    if (HDCP_STATUS_SUCCESSFUL != ret)
    {
        HDCP_ASSERTMESSAGE("Message transactions failed!");
        return ret;
    }

    // Fill the port status
    *portStatus = data.SinglePort.status;

    HDCP_NORMALMESSAGE("session port Status %d", data.SinglePort.status);

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

HDCP_STATUS HdcpSession::GetKsvList(
                            const uint32_t portId,
                            uint8_t *ksvCount,
                            uint8_t *depth,
                            uint8_t *ksvList)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(ksvList, HDCP_STATUS_ERROR_INVALID_PARAMETER);
    CHECK_PARAM_NULL(ksvCount, HDCP_STATUS_ERROR_INVALID_PARAMETER);
    CHECK_PARAM_NULL(depth, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    if (portId > NUM_PHYSICAL_PORTS_MAX)
    {
        HDCP_ASSERTMESSAGE("Invalid port id");
        return HDCP_STATUS_ERROR_INVALID_PARAMETER;
    }

    SocketData  data;

    data.Size               = sizeof(SocketData);
    data.Command            = HDCP_API_GETKSVLIST;
    data.PortCount          = 1;
    data.SinglePort.Id      = portId;
    
    ACQUIRE_LOCK(&m_SocketMutex);

    HDCP_STATUS ret = PerformMessageTransaction(data);
    if (HDCP_STATUS_SUCCESSFUL != ret)
    {
        RELEASE_LOCK(&m_SocketMutex);
        HDCP_ASSERTMESSAGE("Message transactions failed!");
        return ret;
    }

    *depth = data.Depth;
    *ksvCount = data.KsvCount;

    // Receive the KSV LIST from the daemon
    int32_t sts = m_SdkSocket.ReceiveKsvList(ksvList, data.KsvCount);
    if (SUCCESS != sts)
    {
        RELEASE_LOCK(&m_SocketMutex);
        HDCP_ASSERTMESSAGE("Failed to receive ksv list from daemon!");
        return HDCP_STATUS_ERROR_MSG_TRANSACTION;
    }

    RELEASE_LOCK(&m_SocketMutex);

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

HDCP_STATUS HdcpSession::SendSRMData(
                            const uint32_t srmSize,
                            const uint8_t *pSrmData)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(pSrmData, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    SocketData  data;

    data.Size               = sizeof(SocketData);
    data.Command            = HDCP_API_SENDSRMDATA;
    data.SrmOrKsvListDataSz = srmSize;

    ACQUIRE_LOCK(&m_SocketMutex);

    HDCP_STATUS ret = PerformMessageTransaction(data);
    if (HDCP_STATUS_SUCCESSFUL != ret)
    {
        RELEASE_LOCK(&m_SocketMutex);
        HDCP_ASSERTMESSAGE("Message transactions failed!");
        return ret;
    }

    // Send SRM data to daemon
    int32_t sts = m_SdkSocket.SendSrmData(pSrmData, srmSize);
    if (SUCCESS != sts)
    {
        RELEASE_LOCK(&m_SocketMutex);
        HDCP_ASSERTMESSAGE("Failed to send SRM data to daemon!");
        return HDCP_STATUS_ERROR_MSG_TRANSACTION;
    }

    // Get reply from daemon
    sts = m_SdkSocket.GetMessage(data);
    if (SUCCESS != sts)
    {
        RELEASE_LOCK(&m_SocketMutex);
        HDCP_ASSERTMESSAGE("Failed to get response from daemon!");
        return HDCP_STATUS_ERROR_MSG_TRANSACTION;
    }
   
    RELEASE_LOCK(&m_SocketMutex);

    if (SUCCESS != data.Status)
    {
        HDCP_ASSERTMESSAGE("Failed to send SRM data to daemon!");
        return data.Status;
    }

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

HDCP_STATUS HdcpSession::GetSRMVersion(uint16_t *version)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(version, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    SocketData  data;

    data.Size           = sizeof(SocketData);
    data.Size           = sizeof(SocketData);
    data.Command        = HDCP_API_GETSRMVERSION;

    ACQUIRE_LOCK(&m_SocketMutex);
    HDCP_STATUS ret = PerformMessageTransaction(data);
    RELEASE_LOCK(&m_SocketMutex);

    if (HDCP_STATUS_SUCCESSFUL != ret)
    {
        HDCP_ASSERTMESSAGE("Message transactions failed!");
        return ret;
    }

    // Fill the srm version
    *version = data.SrmVersion;

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

HDCP_STATUS HdcpSession::Config(const HDCP_CONFIG config)
{
    HDCP_FUNCTION_ENTER;

    SocketData  data;

    data.Size               = sizeof(SocketData);
    data.Command            = HDCP_API_CONFIG;
    data.Config.type        = config.type;

    switch (config.type)
    {
        case SRM_STORAGE_CONFIG:
            data.Config.disableSrmStorage = config.disableSrmStorage;
            break;
        default:
            HDCP_ASSERTMESSAGE("Input config type is invalid!");
            return HDCP_STATUS_ERROR_INVALID_PARAMETER;
    }

    ACQUIRE_LOCK(&m_SocketMutex);
    HDCP_STATUS ret = PerformMessageTransaction(data);
    RELEASE_LOCK(&m_SocketMutex);

    if (HDCP_STATUS_SUCCESSFUL != ret)
    {
        HDCP_ASSERTMESSAGE("Message transactions failed!");
        return ret;
    }

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

