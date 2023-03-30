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
//! \file       main.cpp
//! \brief
//!

#include <list>
#include <pthread.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>

#include "hdcpdef.h"
#include "srm.h"
#include "portmanager.h"
#include "socketdata.h"
#include "daemon.h"
#include "display_window_util.h"

FILE *dmLog = nullptr;

bool AlreadyRunning()
{
    const std::string pidFile = HDCP_PIDFILE;

    int fd = open(pidFile.c_str(),
                O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        HDCP_ASSERTMESSAGE("Could not open pid file : %s", pidFile.c_str());
        return true;
    }

    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    if (fcntl(fd, F_SETLK, &fl) < 0)
    {
        HDCP_ASSERTMESSAGE("Could not lock pid file");
        close(fd);
        return true;
    }

    std::string pid = std::to_string(getpid());
    if (write(fd, pid.c_str(), pid.length()) < 0)
    {
        HDCP_ASSERTMESSAGE("Could not write pid file");
        close(fd);
        return true;
    }

    close(fd);
    return false;
}

int32_t daemon_init(void)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        return -1;
    }
    else if (pid != 0)
    {
        int status;
        wait(&status);
        exit(SUCCESS);    // parent exit
    }

    setsid();   // become session leader

    close(0);   // close stdin
    close(1);   // close stdout
    close(2);   // close stderr

    return 0;
}

int32_t InitializeDirectory()
{
    HDCP_FUNCTION_ENTER;

    DIR *dir = opendir(HDCP_DIR_BASE);
    if (nullptr != dir)
    {
        closedir(dir);
        return SUCCESS;
    }

    HDCP_ASSERTMESSAGE("Failed to open HDCP temp dir!");
    HDCP_NORMALMESSAGE("Attempting to create %s", HDCP_DIR_BASE);

    int32_t ret = mkdir(HDCP_DIR_BASE, HDCP_DIR_BASE_PERMISSIONS);
    if (ERROR == ret)
    {
        HDCP_ASSERTMESSAGE(
                    "Failed to create HDCP temp dir! Err: %s",
                    strerror(errno));
        return errno;
    }

    struct passwd *mediaId = getpwnam("media");
    if (nullptr == mediaId)
    {
        HDCP_ASSERTMESSAGE("Failed to find info for \"media\" user!");
        return errno;
    }

    ret = chown(HDCP_DIR_BASE, mediaId->pw_uid, mediaId->pw_gid);
    if (ERROR == ret)
    {
        HDCP_ASSERTMESSAGE(
            "Failed to change ownership to \"media\" for HDCP temp dir!");
        return errno;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t InitializeWithMinimalPrivileges()
{
    HDCP_FUNCTION_ENTER;

    int32_t     ret = 1;
    HdcpDaemon  daemon;

    ret = SrmInit();
    if (SUCCESS != ret)
    {
        ret = 1;
        HDCP_ASSERTMESSAGE("SrmInit failed, destroying the daemon.");
        goto out;
    }

    ret = PortManagerInit(daemon);
    if (SUCCESS != ret)
    {
        ret = 1;
        HDCP_ASSERTMESSAGE("PortManagerInit failed, destroying the daemon.");
        goto out;
    }

    ret = daemon.Init();
    if (SUCCESS != ret)
    {
        ret = 1;
        HDCP_ASSERTMESSAGE("Failed to init daemon socket connection");
        goto out;
    }

    if (!daemon.IsValid())
    {
        ret = 1;
        HDCP_ASSERTMESSAGE("Daemon construction failed.");
        goto out;
    }

    // MessageResponseLoop will enter an infinite loop
    daemon.MessageResponseLoop();
    HDCP_NORMALMESSAGE("Daemon has exitted MessageResponseLoop loop, closing");

out:
    PortManagerRelease();
    SrmRelease();

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

/*----------------------------------------------------------------------------
| Name      : main
| Purpose   : Start the HDCP daemon.
| Arguments : argc [in]
|             argv [in]
| Returns   : Zero (0) on success, one (1) on failure.
\---------------------------------------------------------------------------*/
int32_t main(void)
{
    HDCP_FUNCTION_ENTER;

    int32_t ret = -1;
    struct passwd *mediaId = getpwnam("media");
     // This ias_env will be used to determine running with IAS or not
    char *ias_env = NULL;
    ias_env = getenv("XDG_RUNTIME_DIR");

    if (AlreadyRunning())
    {
        HDCP_ASSERTMESSAGE("hdcp aleady already running");
        return 1;
    }

    if (nullptr == mediaId)
    {
        HDCP_ASSERTMESSAGE(
                "Getpwnam for media failed. Err: %s",
                strerror(errno));
        return 1;
    }

#ifndef LOG_CONSOLE
    ret = daemon_init();
    if (ret < 0)
    {
        return 1;
    }
#endif

    // On Linux we use code to initialize the socket directory
    ret = InitializeDirectory();
    if (SUCCESS != ret)
    {
        return 1;
    }

    if(ias_env)
    {
        util_create_display(0);
    }

#ifdef HDCP_LOG_FILE
    if (nullptr == dmLog)
    {
        umask(S_IRUSR | S_IWUSR);
        dmLog = fopen(HDCP_LOG_FILE, "w");
        if (nullptr == dmLog)
        {
            ret = errno;
            HDCP_WARNMESSAGE("Failed to open log file. Err: %s", strerror(ret));
        }

        ret = chown(HDCP_LOG_FILE, mediaId->pw_uid, mediaId->pw_gid);
        if (ERROR == ret)
        {
            ret = errno;
            HDCP_WARNMESSAGE(
                    "Failed to change ownership to \"media\" for logfile!");
        }
    }
#endif

    // This will cycle on the main messaging loop
    ret = InitializeWithMinimalPrivileges();

    if (dmLog != nullptr)
    {
        fclose(dmLog);
        dmLog = nullptr;
    }
    if(ias_env)
    {
        util_destroy_display(0);
    }

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}
