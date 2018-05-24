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
//! \file       sessionmanager.h
//! \brief
//!

#ifndef __HDCP_SESSIONMANAGER_H__
#define __HDCP_SESSIONMANAGER_H__

#include <forward_list>
#include <list>
#include <pthread.h>

#include "hdcpdef.h"
#include "session.h"

#define BAD_SESSION_HANDLE ((uint32_t)-1)

class HdcpSessionManager
{
public:
    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Create a new session within this Process space
    ///
    /// \param[in]  func    Function pointer to the application's callback
    ///                     function.
    /// \return     Handle used within the sdk to manage sessions
    ///
    /// This creates a unique identifier for the Application to use to
    /// reference its instance. A single instance of the SDK can have multiple
    /// sessions and uses the handle to identify them (one process can have
    /// multiple connections to the daemon, additionally multiple processes
    /// can connect to the daemon).
    /// This handle is only meaningful within the context of the process, so
    /// there should be no reason to add obscurity to it.
    /// There are two situations to consider:
    /// 1) The sdk is a static library:
    ///     The sdk exists entirely within a process's address space. Every
    ///     process will get their own copy of the sdk.
    /// 2) The sdk is a shared library:
    ///     The sdk exists in read-only/copy-on-write memory that can be mapped
    ///     into any process's address space.
    ///     Any time a process attempts to write to it, however, the page will
    ///     be copied into the process's address space and disconnected from
    ///     the potential values writen by any other process.
    /// This means that no process can interfere with the integrity of sessions
    /// in any other process.
    ///////////////////////////////////////////////////////////////////////////
    static uint32_t CreateSession(
                            const CallBackFunction func,
                            const Context ctx);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Destroy the provided session within this Process space
    ///
    /// \param[in]  handle      Handle associated with this session
    /// \return     None
    ///////////////////////////////////////////////////////////////////////////
    static void DestroySession(const uint32_t handle);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get a reference to the session associated with the handle
    ///
    /// \param[in]  handle      Handle associated with this session
    /// \return     HdcpSession allocated for that Handle
    ///
    /// Note, this does not address synchronization over the session map.
    ///////////////////////////////////////////////////////////////////////////
    static HdcpSession* GetInstance(const uint32_t handle);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Release a reference to the session associated with the handle
    ///
    /// \param[in]  handle      Handle associated with this session
    /// \return     None
    ///
    /// Note, this does not address synchronization over the session map.
    ///////////////////////////////////////////////////////////////////////////
    static void PutInstance(const uint32_t handle);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Initialize the callback thread and socket connection
    ///
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    static int32_t InitCallBack(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Destroy the callback thread and socket; release any sessions
    ///
    /// \return     Nothing
    ///////////////////////////////////////////////////////////////////////////
    static void DestroyCallback(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Main function of the callback thread
    ///
    /// \param[in]  pData   Not used, but required for pthread_create
    /// \return     Nothing
    ///
    /// There is one callback thread per process. It will iterative over all
    /// open sessions and execute the provided callback function for each one.
    ///////////////////////////////////////////////////////////////////////////
    static void *CallBackManager(void *pData);

private:
    HdcpSessionManager();
    //~HdcpSessionManager();
    HdcpSessionManager& operator=(HdcpSessionManager&);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Get a new handle for a new session
    ///
    /// \return     Unique handle
    ///////////////////////////////////////////////////////////////////////////
    static uint32_t GetUniqueHandle(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Synchronize manipulation or iteration over the session map
    ///
    /// \return     Nothing
    ///
    /// New addtions to the map will be blocked while any thread is iterating.
    /// Session removal will be queued in a garbage list until no threads are
    /// iterating.
    /// Attempts to look up sessions that have been moved to the garbage list
    /// will return "not found."
    ///////////////////////////////////////////////////////////////////////////
    static void IteratorCriticalSectionEnter(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Synchronize manipulation or iteration over the session map
    ///
    /// \return     Nothing
    ///
    /// New addtions to the map will be blocked while any thread is iterating.
    /// Session removal will be queued in a garbage list until no threads are
    /// iterating.
    /// When the last active critical section reference is done, any sessions
    /// in the garbage list will finally be removed from the session map.
    ///////////////////////////////////////////////////////////////////////////
    static void IteratorCriticalSectionExit(void);

    // Declare member variables
private:
    static std::forward_list<HdcpSession*>  m_SessionList;

    static CallBackFunction                 m_CallBack;
    static LocalClientSocket                *m_CallBackSocket;
    static bool                             m_IsCallBackInitialized;
    static pthread_t                        m_SocketThread;

    // This does not tell the number of active sessions, it is just used to
    // prevent sessions from overlapping.
    static uint32_t                         m_HandleIncrementor;
    static pthread_mutex_t                  m_HandleIncrementorMutex;

    static pthread_mutex_t                  m_IteratorMutex;
    static uint32_t                         m_IteratorReference;
    
    static std::list<HdcpSession*>          m_PendingDestroyQueue;
};

#endif // __HDCP_SESSIONMANAGER_H__
