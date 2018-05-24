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
//! \file       socketdata.h
//! \brief
//!

#ifndef __HDCP_SOCKETDATA_H__
#define __HDCP_SOCKETDATA_H__

#include "hdcpdef.h"
#include "hdcpapi.h"

#define ONE_PORT                    1
#define MAX_LISTENER_SOCKET_PATH    64

// socket file used by SDK and daemon
#define HDCP_DIR_BASE               "/var/run/hdcp/"
#define HDCP_DIR_BASE_PERMISSIONS   (S_IRWXU | S_IRWXG | S_IRWXO)
#define HDCP_SDK_SOCKET_PATH        HDCP_DIR_BASE ".sdk_socket"

typedef enum _HDCP_API_TYPE
{
    HDCP_API_INVALID,
    HDCP_API_CREATE,
    HDCP_API_DESTROY,
    HDCP_API_ENUMERATE_HDCP_DISPLAY,
    HDCP_API_SENDSRMDATA,
    HDCP_API_GETSRMVERSION,
    HDCP_API_ENABLE,
    HDCP_API_DISABLE,
    HDCP_API_GETSTATUS,
    HDCP_API_GETKSVLIST,
    HDCP_API_REPORTSTATUS,
    HDCP_API_TERM_MSG_LOOP,
    HDCP_API_CREATE_CALLBACK,
    HDCP_API_SET_PROTECTION_LEVEL,
    HDCP_API_CONFIG,
    HDCP_API_ILLEGAL
} HDCP_API_TYPE;

struct SocketData
{
public:
    SocketData(void);
    ~SocketData(void);

    union
    {
        uint8_t Bytes;
        struct
        {
            uint32_t        Size;
            HDCP_API_TYPE   Command;
            HDCP_STATUS     Status;

            uint8_t         KsvCount;   // Number of KSV in topology
            uint8_t         Depth;      // Depth of topology
            bool            isType1Capable; // Port whether support HDCP2.2
            union
            {
                Port        Ports[NUM_PHYSICAL_PORTS_MAX];
                Port        SinglePort;
            };

            uint32_t        PortCount;

            uint32_t        SrmOrKsvListDataSz;
            uint16_t        SrmVersion;

            HDCP_CONFIG     Config;

            uint8_t         Level;
        };
    };
};

#endif // __HDCP_SOCKETDATA_H__
