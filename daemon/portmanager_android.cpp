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
//! \file       portmanager_android.cpp
//! \brief
//!

#include <list>
#include <unistd.h>

#include "port.h"
#include "portmanager.h"
#include "portmanager_android.h"

#include <hwcserviceapi.h>
#include <log/log.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <iservice.h>

#include "xf86drm.h"
#include "xf86drmMode.h"

  static int getProtectionInfo(int32_t m_DrmFd,
		      DrmObject *drmObject,
                      uint8_t *cpValue,
                      uint8_t *cpType)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(drmObject, EINVAL);
    CHECK_PARAM_NULL(cpValue, EINVAL);
    CHECK_PARAM_NULL(cpType, EINVAL);

    // Invalid the cpValue/cpType
    *cpValue = CP_VALUE_INVALID;
    *cpType = CP_TYPE_INVALID;

    // Query from KMD, get the Content Protection and Content Type value
    auto properties = drmModeObjectGetProperties(
                                        m_DrmFd,
                                        drmObject->GetDrmId(),
                                        DRM_MODE_OBJECT_CONNECTOR);
    if (nullptr == properties)
    {
        HDCP_ASSERTMESSAGE("Failed to get properties");
        return EBUSY;
    }

    for (uint32_t i = 0; i < properties->count_props; i++)
    {
        if (properties->props[i] ==
                drmObject->GetPropertyId(CONTENT_PROTECTION))
        {
            *cpValue = properties->prop_values[i];
        }
        else if (properties->props[i] ==
                drmObject->GetPropertyId(CP_CONTENT_TYPE))
        {
            *cpType = properties->prop_values[i];
        }
    }

    if (CP_ENABLED == *cpValue && CP_TYPE_INVALID == *cpType)
    {
        *cpType = CP_TYPE_0;
    }

    drmModeFreeObjectProperties(properties);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
} //getProtectionInfo

int32_t setPortProperty_hwcservice(int32_t m_DrmFd,
				   int32_t drmId,
				   uint32_t propId,
				   int32_t size,
				   const uint8_t value,
				   const uint32_t numRetry,
				   DrmObject *drmObject)
{

    HDCP_FUNCTION_ENTER;
    // If the size isn't sizeof(uint8_t), it means SRM data, need create blob
    // then set the blob id by drmModeConnectorSetProperty

    int ret = EINVAL;
    uint32_t propValue;
    if (sizeof(uint8_t) != size)
    {
        ret = drmModeCreatePropertyBlob(m_DrmFd, &value, size, &propValue);
        if (SUCCESS != ret)
        {
            HDCP_ASSERTMESSAGE("Could not create blob");
            return EBUSY;
        }
    }
    else
    {
        propValue = value;
    }

    android::ProcessState::initWithDriver("/dev/vndbinder");

    // Connect to HWC service
    HWCSHANDLE hwcs = HwcService_Connect();
    if (hwcs == NULL) {
        HDCP_ASSERTMESSAGE("Could not connect to hwcservice");
        return EINVAL;
    }

    //Disable CP is defined by numRetry parsing value equal to 1
    if (propId == drmObject->GetPropertyId(CONTENT_PROTECTION) &&
        propValue == CP_OFF &&
        numRetry == 1)
    {
        ret = HwcService_Video_DisableHDCPSession_ForDisplay (
                hwcs,
                drmId);
    }

    //Enable CP and SendSRM
    if (numRetry > 1)
    {
        for (uint32_t i = 0; i < numRetry; ++i)
        {
           if(propId == drmObject->GetPropertyId(CONTENT_PROTECTION))
           {
	       HDCP_ASSERTMESSAGE("Attempting HDCP Enable");
               ret = HwcService_Video_EnableHDCPSession_ForDisplay(
                   hwcs, drmId, (EHwcsContentType)propValue);
           } else if(propId == drmObject->GetPropertyId(CP_CONTENT_TYPE))
           {
               //This is only for HDCP2.2
	       HDCP_ASSERTMESSAGE("Attempting HDCP Enable Type 1");
               ret = HwcService_Video_EnableHDCPSession_ForDisplay(
                   hwcs, drmId, (EHwcsContentType)propValue);
           } else if(propId == drmObject->GetPropertyId(CP_SRM))
           {
               HDCP_ASSERTMESSAGE("Set SRM for Display");
               ret = HwcService_Video_SetHDCPSRM_ForDisplay(hwcs, drmId,
                   (const int8_t *)&value, size);
           } else
           {
               HwcService_Disconnect(hwcs);
               ret = EINVAL;
           }
           if(SUCCESS == ret)
           {
               // Invalid the cpValue/cpType
               uint8_t cpValue = CP_VALUE_INVALID;
               uint8_t cpType = CP_TYPE_INVALID;
               ret = getProtectionInfo(m_DrmFd, drmObject, &cpValue, &cpType);
               if (SUCCESS == ret)
               {
                   if (cpValue != CP_ENABLED)
                       continue;

                   else if (CP_ENABLED == cpValue && CP_TYPE_INVALID == cpType)
                   {
                       propValue = CP_TYPE_0;
                       break;
                   }
               }
               else
               {
		   ret = SUCCESS;
		   HDCP_ASSERTMESSAGE("HDCP Enable Success");
                   break;
               }
           }
        }
    }

    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Failed to Enable HDCP");
	HDCP_FUNCTION_EXIT(EBUSY);
        return EBUSY;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
} //setPortProperty_hwcservice
