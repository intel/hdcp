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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPPFLAGS := \
    -DANDROID \
    -DANDROID_VERSION=800 \
    -DHDCP_LOG_TAG="\"HDCPD\"" \

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

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../sdk \
    $(LOCAL_PATH)/../common \

LOCAL_SHARED_LIBRARIES := \
    libutils \
    liblog \

LOCAL_SRC_FILES := \
    clientsock.cpp \
    gensock.cpp \
    servsock.cpp \
    socketdata.cpp \

LOCAL_MODULE := libhdcpcommon
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_STATIC_LIBRARY)
