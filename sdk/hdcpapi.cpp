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
//! \file       hdcp_api.cpp
//! \brief
//!

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "hdcpdef.h"
#include "hdcpapi.h"
#include "socketdata.h"
#include "gensock.h"
#include "servsock.h"
#include "clientsock.h"
#include "session.h"
#include "sessionmanager.h"

#ifdef __cplusplus
extern "C" {
#endif

HDCP_STATUS HDCPCreate(
                    uint32_t *pHdcpHandle,
                    CallBackFunction func,
                    void *context)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(pHdcpHandle, HDCP_STATUS_ERROR_INVALID_PARAMETER);
    *pHdcpHandle = 0;

    // Don't nullptr check func. nullptr is used to indicate the session doesn't
    // want to handle callback events.

    // Create the context
    uint32_t sessionHandle = HdcpSessionManager::CreateSession(func, context);
    if (BAD_SESSION_HANDLE == sessionHandle)
    {
        return HDCP_STATUS_ERROR_INSUFFICIENT_MEMORY;
    }

    // Send Create message to daemon
    HdcpSession *session = HdcpSessionManager::GetInstance(sessionHandle);
    if (nullptr == session)
    {
        HDCP_ASSERTMESSAGE("Session is invalid!");
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    HDCP_STATUS ret = session->Create();
    HdcpSessionManager::PutInstance(sessionHandle);

    // Destroy the context if fail
    if (ret != HDCP_STATUS_SUCCESSFUL)
    {
        HdcpSessionManager::DestroySession(sessionHandle);
        return ret;
    }

    // Set the context
    *pHdcpHandle = sessionHandle;

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

HDCP_STATUS HDCPDestroy(const uint32_t hdcpHandle)
{
    HDCP_FUNCTION_ENTER;

    // All we need to do is close our connection to the daemon
    // This is done by deleting the session which is done in the
    // HdcpSessionManager class with DestroySession.
    HdcpSessionManager::DestroySession(hdcpHandle);

    HDCP_FUNCTION_EXIT(HDCP_STATUS_SUCCESSFUL);
    return HDCP_STATUS_SUCCESSFUL;
}

HDCP_STATUS HDCPEnumerateDisplay(
                    const uint32_t hdcpHandle,
                    PortList *pPortList)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(pPortList, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    memset(pPortList->Ports, 0, sizeof(pPortList->Ports));
    pPortList->PortCount = 0;

    // Send EnumerateDisplay message to daemon
    HdcpSession *session = HdcpSessionManager::GetInstance(hdcpHandle);
    if (nullptr == session)
    {
        HDCP_ASSERTMESSAGE("Session is invalid!");
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    HDCP_STATUS ret = session->EnumerateDisplay(pPortList);
    HdcpSessionManager::PutInstance(hdcpHandle);

    if (HDCP_STATUS_SUCCESSFUL != ret)
    {
        memset(pPortList->Ports, 0, sizeof(pPortList->Ports));
        pPortList->PortCount = 0;
    }

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

HDCP_STATUS HDCPSetProtectionLevel(
                    const uint32_t hdcpHandle,
                    const uint32_t portId,
                    const HDCP_LEVEL level)
{
    HDCP_FUNCTION_ENTER;

    if (portId > NUM_PHYSICAL_PORTS_MAX)
    {
        HDCP_ASSERTMESSAGE("Invalid port id");
        return HDCP_STATUS_ERROR_INVALID_PARAMETER;
    }

    // send Enable message to daemon    
    HdcpSession *session = HdcpSessionManager::GetInstance(hdcpHandle);
    if (nullptr == session)
    {
        HDCP_ASSERTMESSAGE("Session is invalid!");
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    HDCP_STATUS ret = session->SetProtectionLevel(portId, level);
    HdcpSessionManager::PutInstance(hdcpHandle);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

HDCP_STATUS HDCPGetStatus(
                    const uint32_t hdcpHandle,
                    const uint32_t portId,
                    PORT_STATUS *portStatus)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(portStatus, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    if (portId > NUM_PHYSICAL_PORTS_MAX)
    {
        HDCP_ASSERTMESSAGE("Invalid port id");
        return HDCP_STATUS_ERROR_INVALID_PARAMETER;
    }

    // send GetStatus message to daemon    
    HdcpSession *session = HdcpSessionManager::GetInstance(hdcpHandle);
    if (nullptr == session)
    {
        HDCP_ASSERTMESSAGE("Session is invalid!");
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    HDCP_STATUS ret = session->GetStatus(portId, portStatus);
    HdcpSessionManager::PutInstance(hdcpHandle);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

HDCP_STATUS HDCPGetKsvList(
                    const uint32_t hdcpHandle,
                    const uint32_t portId,
                    uint8_t *ksvCount,
                    uint8_t *depth,
                    uint8_t *ksvList)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(ksvCount, HDCP_STATUS_ERROR_INVALID_PARAMETER);
    CHECK_PARAM_NULL(depth, HDCP_STATUS_ERROR_INVALID_PARAMETER);
    CHECK_PARAM_NULL(ksvList, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    if (portId > NUM_PHYSICAL_PORTS_MAX)
    {
        HDCP_ASSERTMESSAGE("Invalid port id");
        return HDCP_STATUS_ERROR_INVALID_PARAMETER;
    }

    // send GetStatus message to daemon
    HdcpSession *session = HdcpSessionManager::GetInstance(hdcpHandle);
    if (nullptr == session)
    {
        HDCP_ASSERTMESSAGE("Session is invalid!");
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    HDCP_STATUS ret = session->GetKsvList(portId, ksvCount, depth, ksvList);
    HdcpSessionManager::PutInstance(hdcpHandle);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

HDCP_STATUS HDCPSendSRMData(
                    const uint32_t hdcpHandle,
                    const uint32_t srmSize,
                    const uint8_t *pSrmData)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(pSrmData, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    if (srmSize < SRM_MIN_LENGTH)
    {
        HDCP_ASSERTMESSAGE("srmSize is invalid!");
        return HDCP_STATUS_ERROR_INVALID_PARAMETER;
    }

    HdcpSession *session = HdcpSessionManager::GetInstance(hdcpHandle);
    if (nullptr == session)
    {
        HDCP_ASSERTMESSAGE("Session is invalid!");
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    HDCP_STATUS ret = session->SendSRMData(srmSize, pSrmData);

    HdcpSessionManager::PutInstance(hdcpHandle);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

HDCP_STATUS HDCPGetSRMVersion(const uint32_t hdcpHandle, uint16_t *version)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(version, HDCP_STATUS_ERROR_INVALID_PARAMETER);

    HdcpSession *session = HdcpSessionManager::GetInstance(hdcpHandle);
    if (nullptr == session)
    {
        HDCP_ASSERTMESSAGE("Session is invalid!");
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    HDCP_STATUS ret = session->GetSRMVersion(version);
    HdcpSessionManager::PutInstance(hdcpHandle);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

HDCP_STATUS HDCPConfig(const uint32_t hdcpHandle, HDCP_CONFIG Config)
{
    HDCP_FUNCTION_ENTER;

    if (Config.type == INVALID_CONFIG)
    {
        return HDCP_STATUS_ERROR_INVALID_PARAMETER;
    }

    HdcpSession *session = HdcpSessionManager::GetInstance(hdcpHandle);
    if (nullptr == session)
    {
        HDCP_ASSERTMESSAGE("Session is invalid!");
        return HDCP_STATUS_ERROR_INTERNAL;
    }

    HDCP_STATUS ret = session->Config(Config);

    HdcpSessionManager::PutInstance(hdcpHandle);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

#ifdef __cplusplus
}
#endif
