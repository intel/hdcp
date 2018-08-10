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
//! \file       socketdata.cpp
//! \brief
//!

#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#include "socketdata.h"
#include "hdcpdef.h"
#include "hdcpapi.h"

SocketData::SocketData(void) :
    Size(sizeof(SocketData)),
    Command(HDCP_API_ILLEGAL),
    Status(HDCP_STATUS_ERROR_INTERNAL),
    PortCount(0),
    SrmOrKsvListDataSz(0)
{
    uint32_t i = 0;

    for (i = 0; i < NUM_PHYSICAL_PORTS_MAX; ++i) {
        Ports[i].Id     = 0;
        Ports[i].status = 0;
    }
}

SocketData::~SocketData(void)
{
}
