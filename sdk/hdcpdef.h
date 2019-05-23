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
//! \file       hdcp_def.h
//! \brief
//!

#ifndef __HDCP_DEF_H__
#define __HDCP_DEF_H__

#include <stdio.h>
#include <stdint.h>
#include <cerrno>
#include <time.h>

#ifdef ANDROID
#include <log/log.h>
#endif

#define SRM_MIN_LENGTH  8

#ifdef SUCCESS
#error "\"SUCCESS\" macro has already been defined!"
#else
#define SUCCESS             0
#endif

#ifdef ERROR
#error "\"ERROR\" macro has already been defined!"
#else
#define ERROR               -1 
#endif

#define BIT(x)              (1 << (x))

// Eliminate unused variable warnings
#define UNUSED_VAR(x)       (x) = (x);

#define SLEEP_NSEC(x)                                                       \
    do                                                                      \
    {                                                                       \
        struct timespec __tv;                                               \
        __tv.tv_sec = (x) / 1000000000;                                     \
        __tv.tv_nsec = (x) % 1000000000;                                    \
        nanosleep(&__tv, nullptr);                                          \
    } while (0);

#define SLEEP_USEC(x)       SLEEP_NSEC((x) * 1000)
#define SLEEP_MSEC(x)       SLEEP_USEC((x) * 1000)
#define SLEEP_SEC(x)        sleep(x);

#define CHECK_PARAM_NULL(x, ret_val)                                        \
    if (nullptr == (x))                                                     \
    {                                                                       \
        HDCP_ASSERTMESSAGE(#x "found be nullptr");                          \
        return (ret_val);                                                   \
    }

#define ACQUIRE_LOCK(mutex)                                                 \
    if (SUCCESS != pthread_mutex_lock(mutex))                               \
    {                                                                       \
        HDCP_ASSERTMESSAGE("Acquire mutex fail");                           \
    }   

#define RELEASE_LOCK(mutex)                                                 \
    if (SUCCESS != pthread_mutex_unlock(mutex))                             \
    {                                                                       \
        HDCP_ASSERTMESSAGE("Release mutex fail");                           \
    }   

#define DESTROY_LOCK(mutex)                                                 \
    if (SUCCESS != pthread_mutex_destroy(mutex))                            \
    {                                                                       \
        HDCP_ASSERTMESSAGE("Destroy mutex fail");                           \
    }   

#define WAIT_CV(mutex, cond_var)                                            \
    if (SUCCESS != pthread_cond_wait(mutex, cond_var))                      \
    {                                                                       \
        HDCP_ASSERTMESSAGE("wait condition variable fail");                 \
    }

#define DESTROY_CV(cond_var)                                                \
    if (SUCCESS != pthread_cond_destroy(cond_var))                          \
    {                                                                       \
        HDCP_ASSERTMESSAGE("Destroy condition variable fail");              \
    }

// Daemon log file
extern FILE* dmLog;

///////////////////////////////////////////////////////////////////////////////
/// \brief  Logging macro's used for console-based ULTs
///
/// \param[in]  fmt     Format string for printing
/// \param[in]  args    Variable length argument list for the format string
///////////////////////////////////////////////////////////////////////////////
#if defined(LOG_CONSOLE)

#ifndef ANDROID
#define DAEMON_LOG(fmt, args...) \
    printf("%s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##args)

#define HDCP_ASSERTMESSAGE(_message, ...) \
    DAEMON_LOG("ERROR: " _message, ##__VA_ARGS__)
#define HDCP_WARNMESSAGE(_message, ...) \
    DAEMON_LOG("WARNING: " _message, ##__VA_ARGS__)
#define HDCP_NORMALMESSAGE(_message, ...) \
    DAEMON_LOG(_message, ##__VA_ARGS__)

#ifdef HDCP_USE_VERBOSE_LOGGING
// This is selectively enabled for debugging gmbus/dpaux failures which spit
// out tons of generally useless information
#define HDCP_VERBOSEMESSAGE(_message, ...) \
    DAEMON_LOG(_message, ##__VA_ARGS__)
#else
#define HDCP_VERBOSEMESSAGE(_message, ...)
#endif

#ifdef HDCP_USE_FUNCTION_LOGGING
#define HDCP_FUNCTION_ENTER \
    printf("ENTER    - %s\n", __FUNCTION__);
#define HDCP_FUNCTION_EXIT(_r) \
    printf("EXIT     - %s: ret = 0x%x\n", __FUNCTION__, _r);
#else
#define HDCP_FUNCTION_ENTER
#define HDCP_FUNCTION_EXIT(_r)
#endif

#ifdef HDCP_USE_LINK_FUNCTION_LOGGING
#define HDCP_LINK_FUNCTION_ENTER        HDCP_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)     HDCP_FUNCTION_EXIT(_r)
#else
#define HDCP_LINK_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)
#endif

#else // =========== ANDROID ===========

#define HDCP_ASSERTMESSAGE(_message, ...) \
    ALOGE("%s:%d:ERROR: " _message, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define HDCP_WARNMESSAGE(_message, ...) \
    ALOGW("%s:%d:WARN: " _message, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define HDCP_NORMALMESSAGE(_message, ...) \
    ALOGI("%s:%d:INFO: " _message, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#ifdef HDCP_USE_VERBOSE_LOGGING
// This is selectively enabled for debugging gmbus/dpaux failures which spit
// out tons of generally useless information
#define HDCP_VERBOSEMESSAGE(_message, ...) \
    ALOGV("%s:%d:VERBOSE: " _message, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define HDCP_VERBOSEMESSAGE(_message, ...)
#endif

#ifdef HDCP_USE_FUNCTION_LOGGING
#define HDCP_FUNCTION_ENTER \
    ALOGV("ENTER    - %s", __FUNCTION__);
#define HDCP_FUNCTION_EXIT(_r) \
    ALOGV("EXIT     - %s: ret = 0x%x", __FUNCTION__, _r);
#else
#define HDCP_FUNCTION_ENTER
#define HDCP_FUNCTION_EXIT(_r)
#endif

#ifdef HDCP_USE_LINK_FUNCTION_LOGGING
#define HDCP_LINK_FUNCTION_ENTER        HDCP_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)     HDCP_FUNCTION_EXIT(_r)
#else
#define HDCP_LINK_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)
#endif

#endif // =========== ANDROID ==========

///////////////////////////////////////////////////////////////////////////////
/// \brief  Empty macro definitions for release and non-logging builds
///
/// \param[in]  args    dummy list of arguments
///////////////////////////////////////////////////////////////////////////////
#elif !defined(HDCP_DEBUG)

#define DAEMON_LOG(arg...)
#define HDCP_ASSERTMESSAGE(arg...)
#define HDCP_WARNMESSAGE(arg...)
#define HDCP_NORMALMESSAGE(arg...)
#define HDCP_VERBOSEMESSAGE(arg...)
#define HDCP_FUNCTION_ENTER
#define HDCP_FUNCTION_EXIT(_r)
#define HDCP_LINK_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)


///////////////////////////////////////////////////////////////////////////////
/// \brief  Logging macro's based on the MOS logging system
///
/// \param[in]  _message    Format string for printing
/// \param[in]  args        Variable length argument list for the format string
///////////////////////////////////////////////////////////////////////////////
#elif defined(HDCP_USE_MOS_LOGGING)

#include "cp_debug.h"
#define HDCP_ASSERTMESSAGE(_message, ...) \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_HDCP, "ERROR: " _message, ##__VA_ARGS__)
#define HDCP_WARNMESSAGE(_message, ...) \
    CP_ASSERTMESSAGE(MOS_CP_SUBCOMP_HDCP, "WARNING: " _message, ##__VA_ARGS__)
#define HDCP_NORMALMESSAGE(_message, ...) \
    CP_NORMALMESSAGE(MOS_CP_SUBCOMP_HDCP, _message, ##__VA_ARGS__)
#define HDCP_VERBOSEMESSAGE(_message, ...) \
    CP_VERBOSEMESSAGE(MOS_CP_SUBCOMP_HDCP, _message, ##__VA_ARGS__)

#define HDCP_FUNCTION_ENTER \
    CP_FUNCTION_ENTER(MOS_CP_SUBCOMP_HDCP)
#define HDCP_FUNCTION_EXIT(_r) \
    CP_FUNCTION_EXIT(MOS_CP_SUBCOMP_HDCP, _r)

#ifdef HDCP_USE_LINK_FUNCTION_LOGGING
#define HDCP_LINK_FUNCTION_ENTER        HDCP_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)     HDCP_FUNCTION_EXIT(_r)
#else
#define HDCP_LINK_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)
#endif


///////////////////////////////////////////////////////////////////////////////
/// \brief  Logging macro's for Non-android systems (Linux or Chrome)
///
/// \param[in]  _level  Log severity level (implied in some macros below)
/// \param[in]  _fmt    Format string for printing
/// \param[in]  _args   Variable length argument list for the format string
///////////////////////////////////////////////////////////////////////////////
#else

#define HDCP_LOG_FILE               "/var/run/hdcp/hdcpd.log"
#define DAEMON_LOG(_fmt, _args...)                                            \
    do                                                                        \
    {                                                                         \
        fprintf(dmLog, "%s:%d: " _fmt "\n", __FUNCTION__, __LINE__, ##_args); \
        fflush(dmLog);                                                        \
    } while (0);

#define HDCP_ASSERTMESSAGE(_message, ...) \
    DAEMON_LOG("ERROR: " _message, ##__VA_ARGS__)
#define HDCP_WARNMESSAGE(_message, ...) \
    DAEMON_LOG("WARNING: " _message, ##__VA_ARGS__)
#define HDCP_NORMALMESSAGE(_message, ...) \
    DAEMON_LOG(_message, ##__VA_ARGS__)

#ifdef HDCP_USE_VERBOSE_LOGGING
// This is selectively enabled for debugging gmbus/dpaux failures which spit
// out tons of generally useless information
#define HDCP_VERBOSEMESSAGE(_message, ...) \
    DAEMON_LOG(_message, ##__VA_ARGS__)
#else
#define HDCP_VERBOSEMESSAGE(_message, ...)
#endif

#ifdef HDCP_USE_FUNCTION_LOGGING
#define HDCP_FUNCTION_ENTER                                                 \
    do                                                                      \
    {                                                                       \
        fprintf(dmLog, "ENTER    - %s\n", __FUNCTION__);                    \
        fflush(dmLog);                                                      \
    } while (0);
#define HDCP_FUNCTION_EXIT(_r)                                              \
    do                                                                      \
    {                                                                       \
        fprintf(dmLog, "EXIT     - %s: ret = 0x%x\n", __FUNCTION__, _r);    \
        fflush(dmLog);                                                      \
    } while (0);
#else
#define HDCP_FUNCTION_ENTER
#define HDCP_FUNCTION_EXIT(_r)
#endif

#ifdef HDCP_USE_LINK_FUNCTION_LOGGING
#define HDCP_LINK_FUNCTION_ENTER        HDCP_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)     HDCP_FUNCTION_EXIT(_r)
#else
#define HDCP_LINK_FUNCTION_ENTER
#define HDCP_LINK_FUNCTION_EXIT(_r)
#endif

#endif

#endif  // __HDCP_DEF_H__
