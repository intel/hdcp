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
//! \file       srm.cpp
//! \brief
//!

#ifndef __HDCP_SRM_H__
#define __HDCP_SRM_H__

#include <list>
#include <pthread.h>

#include "hdcpdef.h"
#include "hdcpapi.h"
#include "port.h"

#ifdef ANDROID
#define SRM_STORAGE_FILENAME    "/data/hdcp/.hdcpsrmlist.bin"
#else
#define SRM_STORAGE_FILENAME    "/var/run/.hdcpsrmlist.bin"
#endif

#define DSA_SIG_LENGTH          20
#define SRM_HEADER_LENGTH       5

#define SRM_HEADER_ID           0x8

#define DSA_SUCCESS      1
#define DSA_FAIL         0

#define SRM_FIRST_GEN_MAX_SIZE  5116    // From HDCP HDMI spec

typedef struct _SrmHeader
{
    union
    {
        uint8_t bytes[5];
        struct
        {
            uint32_t srm_id      : 4;    // bits 3-0
            uint32_t reserved    : 12;   // bits 15-4
            uint16_t version     : 16;   // bits 31-16
            uint8_t generation  : 8;    // bits 39-32
        };
    };
} SrmHeader;

class VectorRevocationList
{
    // Declare member variables
private:
    bool    m_IsValid;

    uint8_t m_NumberOfDevices;
    uint8_t *m_KsvArray;

    // Declare member functions
public:

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Constructor for the VectorRevocationList class
    ///
    /// \param[in]  buf     VRL containing number of KSVs and array of KSVs
    /// \param[in]  length  Total size of the buffer in bytes
    ///
    /// This will parse the buffer and build the data structure holding KSVs.
    /// If the buffer contains inconsistent data then this instance of VRL will
    /// have the m_IsValid value set to FALSE.
    ///
    /// It is important for calling functions to call IsValid on the VRL after
    /// creation.
    ///////////////////////////////////////////////////////////////////////////
    VectorRevocationList(const uint8_t *buf, const uint16_t length);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Destructor for the VectorRevocationList class
    ///
    /// This will clean up the memory allocated by the VRL to hold the KSVs
    ///////////////////////////////////////////////////////////////////////////
    ~VectorRevocationList(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Check the KSV array within the VRL for a specific KSV
    ///
    /// \param[in]  ksv     KSV we are trying to match
    /// \return     true if a match is found, false if no match
    ///////////////////////////////////////////////////////////////////////////
    bool ContainsKsv(const uint8_t ksv[KSV_SIZE]);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Test if the VRL was created successfully
    ///
    /// \return true if successfully created, false otherwise
    ///
    /// Note: This should be called after creating a new VRL as it will be set
    /// to false if the input buffer to the constructor contained inconsistent
    /// data
    ///////////////////////////////////////////////////////////////////////////
    bool IsValid(void);

private:
    // Remove copy/assignment operations
    VectorRevocationList(const VectorRevocationList&) = delete;
    VectorRevocationList & operator=(const VectorRevocationList&) = delete;
};

class SrmTable
{
    // Declare member variables
private:
    bool        m_IsValid;
    uint16_t    m_Version;   // higher number means more recent
    uint8_t     m_Generation;
    bool        m_IsSrmStorageDisable;

    std::list<VectorRevocationList*>    *m_RevocationListsByGeneration;
    pthread_mutex_t                     m_RevocationListMutex;

    // Declare member functions
public:

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Constructor for the SrmTable class
    ///
    /// Note: Call IsValid after creating the SRM module to be sure that it
    /// read the most recent SRM from non-volatile storage successfully
    ///////////////////////////////////////////////////////////////////////////
    SrmTable(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Destructor for the SrmTable class
    ///
    /// Cleans up memory allocated implicitly by the SrmTable
    ///////////////////////////////////////////////////////////////////////////
    ~SrmTable(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Checks if the SrmTable is valid
    ///
    /// \returns true if it was successfully created, false otherwise
    ///
    /// Note: This should be called after creating a new SrmTable as it will
    /// be set to false if initial SRM list cannot be read from non-volatile
    /// storage.
    ///////////////////////////////////////////////////////////////////////////
    bool IsValid(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  set state variable m_IsSrmStorageDisable
    ///
    /// \return void
    ///////////////////////////////////////////////////////////////////////////
    void SetSrmStorageDisable(const bool disableSrmStorage);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Checks the value of state variable m_IsSrmStorageDisable
    ///
    /// \return true if m_IsSrmStorageDisable is true, false otherwise
    ///////////////////////////////////////////////////////////////////////////
    bool IsSrmStorageDisable(void);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Check if the provided KSV has been revoked
    ///
    /// \param[in]  ksv     The ksv we want to check revocation status for
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t CheckSrmRevoke(const uint8_t ksv[KSV_SIZE]);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  Parse a new set of VRLs from a SRM buffer
    ///
    /// \param[in]  buf     The SRM message
    /// \param[in]  length  Raw size in bytes of the SRM message
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t RetrieveSrmFromBuffer(const uint8_t *buf, const size_t length);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Verify the signature of a message using DSA
    ///
    /// \param[in]  pMsg    data to validate
    /// \param[in]  msgLen  length of the message
    /// \param[in]  pR      The R signature value used in validation
    /// \param[in]  pS      The S signature value used in validation
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t VerifySignature(
            const uint8_t   *pMsg,
            const uint32_t  msgLen,
            const uint8_t   pR[DSA_SIG_LENGTH],
            const uint8_t   pS[DSA_SIG_LENGTH]);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief  GetSrmVersion
    ///
    /// \param[out]    Srm current version
    /// \return     SUCCESS or errno otherwise
    ///////////////////////////////////////////////////////////////////////////
    int32_t GetSrmVersion(uint16_t *version);

private:
    // Remove copy/assignment operations
    SrmTable(const SrmTable&) = delete;
    SrmTable & operator=(const SrmTable&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
/// \brief  Initialze the SRM Module
///
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t SrmInit(void);

///////////////////////////////////////////////////////////////////////////////
/// \brief  wrapper for SrmTable GetSrmVersion
///
/// \param[out]    Srm current version
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t GetSrmVersion(uint16_t *version);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Destroy the SRM Module
///
/// \return void
///////////////////////////////////////////////////////////////////////////////
void SrmRelease(void);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Store a new SRM message
///
/// \param[in]  data    The new message to store
/// \param[in]  size    Size in bytes of the new message
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t StoreSrm(const uint8_t *data, const uint32_t size);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Interface to the SRM module for checking the validity of a KSV
///
/// \param[in]  ksv     KSV we wish to check the revocation status of
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t CheckSrmRevoke(const uint8_t ksv[KSV_SIZE]);

///////////////////////////////////////////////////////////////////////////////
/// \brief  Interface to configure whether SRM will be stored to device or not
///
/// \param[in]  disableSrmStorage  if set to true, SRM won't be stored   
/// \return     SUCCESS or errno otherwise
///////////////////////////////////////////////////////////////////////////////
int32_t SrmConfig(const bool disableSrmStorage);

#endif // __HDCP_SRM_H__
