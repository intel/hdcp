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
//! \file       port.cpp
//! \brief      Defination of DrmObject, DrmObject represents one connector
//!

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <new>
#include <list>
#include <random>

#include "port.h"
#include "hdcpdef.h"
#include "hdcpapi.h"
#include "daemon.h"
#include "srm.h"

DrmObject::DrmObject(uint32_t drm_id, uint32_t port_id)
    :m_DrmId(drm_id)
{
    m_PortId = port_id;
    m_Connection = UINT32_MAX;
    m_CpType = UINT32_MAX; 
    m_Depth = UINT32_MAX;
    m_DeviceCount = UINT32_MAX;
    m_PropertyList = {};

    pthread_mutex_init(&m_ConnectionMutex, nullptr);
    pthread_mutex_init(&m_CpTypeMutex, nullptr);
}

DrmObject::~DrmObject()
{
    DESTROY_LOCK(&m_ConnectionMutex);
    DESTROY_LOCK(&m_CpTypeMutex);
}

uint32_t DrmObject::GetDrmId()
{
    return m_DrmId;
}

uint32_t DrmObject::GetPortId()
{
    return m_PortId;
}

void DrmObject::AddDrmProperty(std::string name, uint32_t id, uint32_t value)
{

    DrmProperty drmProperty;
    drmProperty.name = name;
    drmProperty.propertyId = id;
    drmProperty.propertyValue = value;
    m_PropertyList.push_back(drmProperty);
}

uint32_t DrmObject::GetPropertyId(std::string name)
{
    for (auto prop : m_PropertyList)
    {
        if(!prop.name.compare(name))
        {
            return prop.propertyId;
        }
    }

    return UINT32_MAX;
}

uint32_t DrmObject::GetPropertyValue(std::string name)
{
    for (auto prop : m_PropertyList)
    {
        if(!prop.name.compare(name))
        {
            return prop.propertyValue;
        }
    }

    return UINT32_MAX;
}

uint32_t DrmObject::GetDepth()
{
    return m_Depth;
}

void DrmObject::SetDepth(uint32_t depth)
{
    m_Depth = depth;
}

uint32_t DrmObject::GetDeviceCount()
{
    return m_DeviceCount; 
}

void DrmObject::SetDeviceCount(uint32_t deviceCount)
{
    m_DeviceCount = deviceCount;
}

uint32_t DrmObject::GetConnection()
{
    return m_Connection;
}

void DrmObject::SetConnection(uint32_t connection)
{
    m_Connection = connection;
}

uint8_t DrmObject::GetCpType()
{
    return m_CpType; 
}

void DrmObject::SetCpType(uint8_t cpType)
{
    m_CpType = cpType;
}

void DrmObject::AddRefAppId(uint32_t appId)
{
    bool exist = false;
    for (auto id : m_AppIds)
    {
        if (id == appId)
        {
            exist = true;
        }
    }
    
    if (!exist)
    {
        m_AppIds.push_back(appId);
    }
}

void DrmObject::RemoveRefAppId(uint32_t appId)
{
    m_AppIds.remove(appId);
}

uint32_t DrmObject::GetRefAppCount()
{
    return m_AppIds.size();
}

void DrmObject::ClearRefAppId()
{
    m_AppIds.clear();
}

void DrmObject::ConnAtomicBegin()
{
    ACQUIRE_LOCK(&m_ConnectionMutex);
}

void DrmObject::ConnAtomicEnd()
{
    RELEASE_LOCK(&m_ConnectionMutex);
}

void DrmObject::CpTypeAtomicBegin()
{
    ACQUIRE_LOCK(&m_CpTypeMutex);
}

void DrmObject::CpTypeAtomicEnd()
{
    RELEASE_LOCK(&m_CpTypeMutex);
}
