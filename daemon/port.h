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
//! \file       port.h
//! \brief
//!

#ifndef __HDCP_PORT_H__
#define __HDCP_PORT_H__

#include <list>
#include <memory>
#include <map>
#include <string>
#include <pthread.h>

#include "hdcpdef.h"
#include "hdcpapi.h"

class DrmObject
{
private: 
    class DrmProperty
    {
        public:
            std::string name;
            uint32_t propertyId;
            uint32_t propertyValue;
    };

    // Id of this drm object (i.e. the connector id)
    uint32_t m_DrmId;
    
    // Id of the port for SDK
    uint32_t m_PortId;

    // Connection state, for detecting in or out hotplug uevent
    uint32_t m_Connection;

    // PortManager uEvent thread will read and write m_CpTye,
    // need a dedicate lock to protect it
    pthread_mutex_t m_ConnectionMutex;

    // Depth of the topology connected to this port 
    uint32_t m_Depth;

    // DeviceCount of the topology connected to this port 
    uint32_t m_DeviceCount;

    // content type of this port
    uint32_t m_CpType;

    // PortManager integrity check thread will read and write m_CpTye,
    // need a dedicate lock to protect it
    pthread_mutex_t m_CpTypeMutex;

    // DrmProperties of this port
    std::list<DrmProperty> m_PropertyList;

    // processes tha enabled this port
    std::list<uint32_t>   m_AppIds;

public:

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Constructor for the DrmObject class
    ///////////////////////////////////////////////////////////////////////////
    DrmObject(uint32_t drm_id, uint32_t port_id);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Destructor for the DrmObject class
    ///////////////////////////////////////////////////////////////////////////
    ~DrmObject();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get the drm id of the drm object
    ///
    /// \return      drm id
    ///////////////////////////////////////////////////////////////////////////
    uint32_t GetDrmId();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get the port id of the drm object
    ///
    /// \return      port id
    ///////////////////////////////////////////////////////////////////////////
    uint32_t GetPortId();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Generate DrmProperty for this port
    ///
    /// \param[in] name,   name of the property
    /// \param[in] prop_id, id of the property
    /// \param[in] prop_val, value of the property
    ///////////////////////////////////////////////////////////////////////////
    void AddDrmProperty(std::string name, uint32_t prop_id, uint32_t prop_val);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get property id by name 
    ///
    /// \param[in] name,   name of the property
    ///
    /// \return     property id
    ///////////////////////////////////////////////////////////////////////////
    uint32_t GetPropertyId(std::string name);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get property value by name 
    ///
    /// \param[in] name,   name of the property
    ///
    /// \return     property id
    ///////////////////////////////////////////////////////////////////////////
    uint32_t GetPropertyValue(std::string name);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get topology depth 
    ///
    /// \return     depth
    ///////////////////////////////////////////////////////////////////////////
    uint32_t GetDepth();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Set downstream topology depth 
    ///
    /// \param[in] depth,   depth of the downstream topology
    ///////////////////////////////////////////////////////////////////////////
    void SetDepth(uint32_t depth);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get downstream topology device count 
    ///
    /// \return     device count
    ///////////////////////////////////////////////////////////////////////////
    uint32_t GetDeviceCount();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Set downstream topology device count 
    ///
    /// \param[in] device count
    ///////////////////////////////////////////////////////////////////////////
    void SetDeviceCount(uint32_t deviceCount);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get port connection status 
    ///
    /// \return     connection
    ///////////////////////////////////////////////////////////////////////////
    uint32_t GetConnection();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Set port connection status 
    ///
    /// \param[in] device count
    ///////////////////////////////////////////////////////////////////////////
    void SetConnection(uint32_t connection);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Set port content protection type 
    ///
    /// \param[in] content protection type
    ///////////////////////////////////////////////////////////////////////////
    void SetCpType(uint8_t cpType);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get port content protection type 
    ///
    /// \return     content type
    ///////////////////////////////////////////////////////////////////////////
    uint8_t GetCpType();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Add appId to appId list
    ///
    /// \param[in] appId
    ///////////////////////////////////////////////////////////////////////////
    void AddRefAppId(uint32_t appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Remove appId from appId list
    ///
    /// \param[in] appId
    ///////////////////////////////////////////////////////////////////////////
    void RemoveRefAppId(uint32_t appId);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get process count that enabled this port 
    ///
    /// \return     app count
    ///////////////////////////////////////////////////////////////////////////
    uint32_t GetRefAppCount();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Remove all AppId 
    ///////////////////////////////////////////////////////////////////////////
    void ClearRefAppId();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Begin atomic operation for m_Connection 
    ///////////////////////////////////////////////////////////////////////////
    void ConnAtomicBegin();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  End atomic operation for m_Connection 
    ///////////////////////////////////////////////////////////////////////////
    void ConnAtomicEnd();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Begin atomic operation for m_CpType 
    ///////////////////////////////////////////////////////////////////////////
    void CpTypeAtomicBegin();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  End atomic operation for m_CpType 
    ///////////////////////////////////////////////////////////////////////////
    void CpTypeAtomicEnd();
};

#endif // __HDCP_PORT_H__
