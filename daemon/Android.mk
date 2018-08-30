#----------------------------------------------------------------------------
# INTEL CONFIDENTIAL
# Copyright (2002-2017) Intel Corporation All Rights Reserved.
# The source code contained or described herein and all documents related to
# the source code ("Material") are owned by Intel Corporation or its suppliers
# or licensors. Title to the Material remains with Intel Corporation or its
# suppliers and licensors. The Material contains trade secrets and proprietary
# and confidential information of Intel or its suppliers and licensors. The
# Material is protected by worldwide copyright and trade secret laws and
# treaty provisions. No part of the Material may be used, copied, reproduced,
# modified, published, uploaded, posted, transmitted, distributed, or
# disclosed in any way without Intel's prior express written permission.
#
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or
# delivery of the Materials, either expressly, by implication, inducement,
# estoppel or otherwise. Any license under such intellectual property rights
# must be express and approved by Intel in writing.
#
#----------------------------------------------------------------------------

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPPFLAGS :=                                           \
    -DANDROID \
    -DANDROID_VERSION=800 \
    -DHDCP_LOG_TAG="\"HDCPD\""

ifeq ($(ENABLE_DEBUG),1)
    LOCAL_CPPFLAG += \
        -DLOG_CONSOLE \
        -DHDCP_USE_VERBOSE_LOGGING \
        -DHDCP_USE_FUNCTION_LOGGING \
        -DHDCP_USE_LINK_FUNCTION_LOGGING
endif

#WA
LOCAL_CPPFLAGS += \
    -Wno-error

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libbinder \
    liblog \
    libcrypto \
    libdrm \
    libssl \
    libhwcservice

LOCAL_STATIC_LIBRARIES := \
    libhdcpcommon \

LOCAL_SRC_FILES := \
    main.cpp \
    daemon.cpp \
    port.cpp \
    srm.cpp \
    portmanager.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../sdk \
    $(LOCAL_PATH)/../common

LOCAL_MODULE := hdcpd
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)
