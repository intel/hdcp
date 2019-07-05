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

#include <list>
#include <new>
#include <openssl/dsa.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/opensslv.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <memory.h>

#include "srm.h"
#include "hdcpdef.h"

// Add compatibility layer for openssl 1.0 as suggested by openssl
// https://wiki.openssl.org/index.php/OpenSSL_1.1.0_Changes#Compatibility_Layer
#if OPENSSL_VERSION_NUMBER < 0x10100000
int32_t DSA_set0_pqg(DSA *d, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
    // If the fields p, q and g in d are nullptr, the corresponding input
    // parameters MUST be non-nullptr.
    if ((nullptr == d)                      ||
        (nullptr == d->p && nullptr == p)   ||
        (nullptr == d->q && nullptr == q)  ||
        (nullptr == d->g && nullptr == g))
    {
        return DSA_FAIL;
    }

    if (nullptr != p)
    {
        BN_free(d->p);
        d->p = p;
    }

    if (nullptr != q)
    {
        BN_free(d->q);
        d->q = q;
    }

    if (nullptr != g)
    {
        BN_free(d->g);
        d->g = g;
    }

    return DSA_SUCCESS;
}

int32_t DSA_SIG_set0(DSA_SIG *sig, BIGNUM *r, BIGNUM *s)
{
    if (nullptr == sig || nullptr == r || nullptr == s)
    {
        return DSA_FAIL;
    }

    BN_clear_free(sig->r);
    BN_clear_free(sig->s);
    sig->r = r;
    sig->s = s;

    return DSA_SUCCESS;
}

int32_t DSA_set0_key(DSA *d, BIGNUM *pub_key, BIGNUM *priv_key)
{
    // If the field pub_key in d is nullptr, the corresponding input
    // parameters MUST be non-nullptr.  The priv_key field may be left nullptr.
    if (nullptr == d || (nullptr == d->pub_key && nullptr == pub_key))
    {
        return DSA_FAIL;
    }

    if (nullptr != pub_key)
    {
        BN_free(d->pub_key);
        d->pub_key = pub_key;
    }

    if (nullptr != priv_key)
    {
        BN_free(d->priv_key);
        d->priv_key = priv_key;
    }

    return DSA_SUCCESS;
}
#endif

// Global Variables
static SrmTable* g_pSrmTable = nullptr;

// prime modulus for DSA
static const uint8_t g_dsaP[] =
{
    0xd3, 0xc3, 0xf5, 0xb2, 0xfd, 0x17, 0x61, 0xb7, 0x01, 0x8d, 0x75, 0xf7,
    0x93, 0x43, 0x78, 0x6b, 0x17, 0x39, 0x5b, 0x35, 0x5a, 0x52, 0xc7, 0xb8,
    0xa1, 0xa2, 0x4f, 0xc3, 0x6a, 0x70, 0x58, 0xff, 0x8e, 0x7f, 0xa1, 0x64,
    0xf5, 0x00, 0xe0, 0xdc, 0xa0, 0xd2, 0x84, 0x82, 0x1d, 0x96, 0x9e, 0x4b,
    0x4f, 0x34, 0xdc, 0x0c, 0xae, 0x7c, 0x76, 0x67, 0xb8, 0x44, 0xc7, 0x47,
    0xd4, 0xc6, 0xb9, 0x83, 0xe5, 0x2b, 0xa7, 0x0e, 0x54, 0x47, 0xcf, 0x35,
    0xf4, 0x04, 0xa0, 0xbc, 0xd1, 0x97, 0x4c, 0x3a, 0x10, 0x71, 0x55, 0x09,
    0xb3, 0x72, 0x15, 0x30, 0xa7, 0x3f, 0x32, 0x07, 0xb9, 0x98, 0x20, 0x49,
    0x5c, 0x7b, 0x9c, 0x14, 0x32, 0x75, 0x73, 0x3b, 0x02, 0x8a, 0x49, 0xfd,
    0x96, 0x89, 0x19, 0x54, 0x2a, 0x39, 0x95, 0x1c, 0x46, 0xed, 0xc2, 0x11,
    0x8c, 0x59, 0x80, 0x2b, 0xf3, 0x28, 0x75, 0x27
};

// prime divisor for DSA
static const uint8_t g_dsaQ[] =
{
    0xee, 0x8a, 0xf2, 0xce, 0x5e, 0x6d, 0xb5, 0x6a, 0xcd, 0x6d, 0x14, 0xe2,
    0x97, 0xef, 0x3f, 0x4d, 0xf9, 0xc7, 0x08, 0xe7
};

// generator for DSA
static const uint8_t g_dsaG[] =
{
    0x92, 0xf8, 0x5d, 0x1b, 0x6a, 0x4d, 0x52, 0x13, 0x1a, 0xe4, 0x3e, 0x24,
    0x45, 0xde, 0x1a, 0xb5, 0x02, 0xaf, 0xde, 0xac, 0xa9, 0xbe, 0xd7, 0x31,
    0x5d, 0x56, 0xd7, 0x66, 0xcd, 0x27, 0x86, 0x11, 0x8f, 0x5d, 0xb1, 0x4a,
    0xbd, 0xec, 0xa9, 0xd2, 0x51, 0x62, 0x97, 0x7d, 0xa8, 0x3e, 0xff, 0xa8,
    0x8e, 0xed, 0xc6, 0xbf, 0xeb, 0x37, 0xe1, 0xa9, 0x0e, 0x29, 0xcd, 0x0c,
    0xa0, 0x3d, 0x79, 0x9e, 0x92, 0xdd, 0x29, 0x45, 0xf7, 0x78, 0x58, 0x5f,
    0xf7, 0xc8, 0x35, 0x64, 0x2c, 0x21, 0xba, 0x7f, 0xb1, 0xa0, 0xb6, 0xbe,
    0x81, 0xc8, 0xa5, 0xe3, 0xc8, 0xab, 0x69, 0xb2, 0x1d, 0xa5, 0x42, 0x42,
    0xc9, 0x8e, 0x9b, 0x8a, 0xab, 0x4a, 0x9d, 0xc2, 0x51, 0xfa, 0x7d, 0xac,
    0x29, 0x21, 0x6f, 0xe8, 0xb9, 0x3f, 0x18, 0x5b, 0x2f, 0x67, 0x40, 0x5b,
    0x69, 0x46, 0x24, 0x42, 0xc2, 0xba, 0x0b, 0xd9
};

// Intel public key for DSA
static const uint8_t g_publicKey[] =
{
    0xc7, 0x06, 0x00, 0x52, 0x6b, 0xa0, 0xb0, 0x86, 0x3a, 0x80, 0xfb, 0xe0,
    0xa3, 0xac, 0xff, 0x0d, 0x4f, 0x0d, 0x76, 0x65, 0x8a, 0x17, 0x54, 0xa8,
    0xe7, 0x65, 0x47, 0x55, 0xf1, 0x5b, 0xa7, 0x8d, 0x56, 0x95, 0x0e, 0x48,
    0x65, 0x4f, 0x0b, 0xbd, 0xe1, 0x68, 0x04, 0xde, 0x1b, 0x54, 0x18, 0x74,
    0xdb, 0x22, 0xe1, 0x4f, 0x03, 0x17, 0x04, 0xdb, 0x8d, 0x5c, 0xb2, 0xa4,
    0x17, 0xc4, 0x56, 0x6c, 0x27, 0xba, 0x97, 0x3c, 0x43, 0xd8, 0x4e, 0x0d,
    0xa2, 0xa7, 0x08, 0x56, 0xfe, 0x9e, 0xa4, 0x8d, 0x87, 0x25, 0x90, 0x38,
    0xb1, 0x65, 0x53, 0xe6, 0x62, 0x43, 0x5f, 0xf7, 0xfd, 0x52, 0x06, 0xe2,
    0x7b, 0xb7, 0xff, 0xbd, 0x88, 0x6c, 0x24, 0x10, 0x95, 0xc8, 0xdc, 0x8d,
    0x66, 0xf6, 0x62, 0xcb, 0xd8, 0x8f, 0x9d, 0xf7, 0xe9, 0xb3, 0xfb, 0x83,
    0x62, 0xa9, 0xf7, 0xfa, 0x36, 0xe5, 0x37, 0x99
};

#ifdef SRM_ULT_BUILD
// Public Key for testing facsimile SRMs
// in Table A21 of HDCP on HDMI Spec Rev2_2_Final1 (July 8, 2009)
// DO _NOT_ USE THIS KEY IN PRODUCTION
// IT IS ONLY FOR TESTING THE SPECIFIC TEST SRMS FROM THE HDCP SPEC
static const uint8_t g_facsimilePublicKey[] = 
{
    0x8d, 0x13, 0xe1, 0x9f, 0x34, 0x0e, 0x11, 0xce, 0xb0, 0xdb, 0x95, 0xeb,
    0x3e, 0xb0, 0x74, 0x31, 0x95, 0xdf, 0xc4, 0x02, 0xb7, 0xdc, 0x8c, 0xaa,
    0xc7, 0x75, 0x2e, 0x47, 0xde, 0xd8, 0xe8, 0xc0, 0x0b, 0x11, 0x5f, 0x8e,
    0x5e, 0x08, 0xc7, 0xa6, 0x64, 0xcb, 0xbb, 0xa3, 0x97, 0x86, 0xef, 0xd7,
    0x1c, 0x01, 0x2e, 0x83, 0x94, 0xaf, 0x79, 0xcd, 0x01, 0xf7, 0x22, 0xa0,
    0x92, 0x69, 0x52, 0xe8, 0xde, 0x85, 0x7c, 0xbd, 0x2e, 0x72, 0x95, 0xe6,
    0xb1, 0xd8, 0x8c, 0xc0, 0xff, 0x5d, 0xcc, 0x0a, 0xb1, 0x6d, 0x14, 0xfa,
    0x11, 0xa4, 0x8e, 0xb5, 0x0f, 0xca, 0x83, 0xa3, 0x7e, 0xd1, 0x8d, 0xe1,
    0x6d, 0x97, 0x35, 0x65, 0xdf, 0x8a, 0x78, 0x4e, 0x85, 0x42, 0x96, 0xac,
    0x70, 0x0b, 0x2e, 0x03, 0x0f, 0xd2, 0xa9, 0x81, 0x83, 0xaa, 0x7b, 0x22,
    0xa6, 0x3b, 0x57, 0xbe, 0xe5, 0xc2, 0xb9, 0x46
};

static bool g_UseFacsimileKey = false;

void EnableFacsimileKeyUse(void)
{
    g_UseFacsimileKey = true;
}

void DisableFacsimileKeyUse(void)
{
    g_UseFacsimileKey = false;
}
#endif

int32_t SrmTable::VerifySignature(
                const uint8_t   *pMsg,
                const uint32_t  msgLen,
                const uint8_t   pR[DSA_SIG_LENGTH],
                const uint8_t   pS[DSA_SIG_LENGTH])
{
    HDCP_FUNCTION_ENTER;

    DSA *dsa = DSA_new();
    if (nullptr == dsa)
    {
        return ENOMEM;
    }

    DSA_SIG *sig = DSA_SIG_new();
    if (nullptr == sig)
    {
        DSA_free(dsa);
        return ENOMEM;
    }

    // Do an endian conversion and convert type to BIGNUM for the values
    BIGNUM *p, *q, *g, *pub_key, *r, *s;

    p = BN_bin2bn(g_dsaP, sizeof(g_dsaP), nullptr);
    q = BN_bin2bn(g_dsaQ, sizeof(g_dsaQ), nullptr);
    g = BN_bin2bn(g_dsaG, sizeof(g_dsaG), nullptr);

    if(DSA_SUCCESS != DSA_set0_pqg(dsa, p, q, g)) 
    {
       DSA_free(dsa);
       DSA_SIG_free(sig);
       BN_free(p);
       BN_free(q);
       BN_free(g);
       return EINVAL;
    }

#ifdef SRM_ULT_BUILD
    if (g_UseFacsimileKey)
    {
        pub_key = BN_bin2bn(g_facsimilePublicKey, sizeof(g_publicKey), nullptr);
    }
    else
    {
        pub_key = BN_bin2bn(g_publicKey, sizeof(g_publicKey), nullptr);
    }
#else
    pub_key = BN_bin2bn(g_publicKey, sizeof(g_publicKey), nullptr);
#endif

    if(DSA_SUCCESS != DSA_set0_key(dsa, pub_key, nullptr)) 
    {
        DSA_free(dsa);
        DSA_SIG_free(sig);
        BN_free(pub_key);
        return EINVAL;
    }

    r = BN_bin2bn(pR, DSA_SIG_LENGTH, nullptr);
    s = BN_bin2bn(pS, DSA_SIG_LENGTH, nullptr);

    if(DSA_SUCCESS != DSA_SIG_set0(sig, r, s)) 
    {
        DSA_free(dsa);
        DSA_SIG_free(sig);
        BN_free(r);
        BN_free(s);
        return EINVAL;
    }
    
    uint8_t md[SHA_DIGEST_LENGTH];
    uint32_t result = DSA_do_verify(
                    SHA1(pMsg, msgLen, md),
                    SHA_DIGEST_LENGTH,
                    sig,
                    dsa);

    DSA_free(dsa);
    DSA_SIG_free(sig);

    if (DSA_SUCCESS != result)
    {
        return EINVAL;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

VectorRevocationList::VectorRevocationList(
                        const uint8_t *buf,
                        const uint16_t length) :
    m_IsValid(false),
    m_NumberOfDevices(0),
    m_KsvArray(nullptr)
{
    HDCP_FUNCTION_ENTER;

    // Handle the empty VRL list:
    if (length == 0)
    {
        // m_KsvArray will be nullptr now, so be careful in later code
        m_NumberOfDevices = 0;
        m_IsValid = true;
        return;
    }

    // The first byte of the vrl is header info
    // The first _bit_ is reserved to be 0 (hence the mask here)
    uint32_t ksvCount = buf[0] & 0x7f;
    uint32_t ksvArraySize = ksvCount * KSV_SIZE;
    // Make sure the length matches the number of devices
    if (length != (ksvArraySize + 1))
    {
        HDCP_ASSERTMESSAGE("Length of VRL buffer does not match stated size");
        return;
    }

    m_NumberOfDevices = ksvCount;

    m_KsvArray = new (std::nothrow) uint8_t[ksvArraySize];
    if (m_KsvArray == nullptr)
    {
        HDCP_ASSERTMESSAGE("Failed to allocate an array for KSVs");
        return;
    }

    memmove(m_KsvArray, &buf[1], ksvArraySize);

    for (int32_t i = 0; i < m_NumberOfDevices; i++)
    {
        HDCP_VERBOSEMESSAGE(
                    "Number of devices: %d, checking number of device %d",
                    m_NumberOfDevices,
                    i);

        HDCP_VERBOSEMESSAGE(
                    "RevokeList is %x, %x, %x, %x, %x",
                    m_KsvArray[i * KSV_SIZE + 4],
                    m_KsvArray[i * KSV_SIZE + 3],
                    m_KsvArray[i * KSV_SIZE + 2],
                    m_KsvArray[i * KSV_SIZE + 1],
                    m_KsvArray[i * KSV_SIZE + 0]);
    }
    

    m_IsValid = true;

    HDCP_FUNCTION_EXIT(SUCCESS);
    return;
}

VectorRevocationList::~VectorRevocationList(void)
{
    HDCP_FUNCTION_ENTER;

    delete [] m_KsvArray;
    m_KsvArray = nullptr;

    HDCP_FUNCTION_EXIT(0);
}

bool VectorRevocationList::IsValid(void)
{
    return m_IsValid;
}

bool VectorRevocationList::ContainsKsv(const uint8_t ksv[KSV_SIZE])
{
    HDCP_FUNCTION_ENTER;

    // It is valid for us to have stored an empty list, so check that case first
    if (0 == m_NumberOfDevices)
    {
        return false;
    }

    if (nullptr == m_KsvArray)
    {
        HDCP_ASSERTMESSAGE("Attempting to check a nullptr ksv array!");
        // REVIEW: This is very inconsistent state... how should this fail?
        // (Just let it return true so that the ksv will
        //  look revoked and not enable HDCP for the device)
        // This needs testing and code review
        return true;
    }

    // We've assumed KSV is 5 in the check within the loop below
    // Let the compiler confirm this in case KSV ever gets udpated
    static_assert(
            KSV_SIZE == 5,
            "ERROR: KSV_SIZE doesn't match the explicit expectation"
            "of 5 in the loop below!");

    HDCP_VERBOSEMESSAGE("Start to check RevokeList with BKSV");

    for (uint32_t i = 0; i < m_NumberOfDevices; i++)
    {
        HDCP_VERBOSEMESSAGE(
                    "Number of devices: %d, checking number of device %d",
                    m_NumberOfDevices,
                    i);

        HDCP_VERBOSEMESSAGE(
                    "RevokeList is %x, %x, %x, %x, %x",
                    m_KsvArray[i * KSV_SIZE + 4],
                    m_KsvArray[i * KSV_SIZE + 3],
                    m_KsvArray[i * KSV_SIZE + 2],
                    m_KsvArray[i * KSV_SIZE + 1],
                    m_KsvArray[i * KSV_SIZE + 0]);

        HDCP_VERBOSEMESSAGE(
                    "BKSV to check is %x, %x, %x, %x, %x",
                    ksv[0],
                    ksv[1],
                    ksv[2],
                    ksv[3],
                    ksv[4]);

        // Check against all 5 bytes of the input KSV (bytewise)
        // (The bits are reversed)
        if ((m_KsvArray[i * KSV_SIZE + 4] == ksv[0]) &&
            (m_KsvArray[i * KSV_SIZE + 3] == ksv[1]) &&
            (m_KsvArray[i * KSV_SIZE + 2] == ksv[2]) &&
            (m_KsvArray[i * KSV_SIZE + 1] == ksv[3]) &&
            (m_KsvArray[i * KSV_SIZE + 0] == ksv[4]))
        {
            break;
        }
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

SrmTable::SrmTable(void) :
            m_IsValid(false),
            m_Version(0),
            m_Generation(0),
            m_IsSrmStorageDisable(false),
            m_RevocationListsByGeneration(nullptr)
{
    HDCP_FUNCTION_ENTER;

    int32_t sts = pthread_mutex_init(&m_RevocationListMutex, nullptr);
    if (0 != sts)
    {
        return;
    }

    m_RevocationListsByGeneration =
                    new (std::nothrow) std::list<VectorRevocationList*>;
    if (nullptr == m_RevocationListsByGeneration)
    {
        HDCP_ASSERTMESSAGE("Failed to allocate a new list of VRLs");
        return;
    }

    // Failing to find the SRM file is bad and this potentially should fail
    // conservatively, but apparently it should just continue until an app
    // actually sends srm data to use.
    m_IsValid = true;

    int32_t fd = open(SRM_STORAGE_FILENAME, O_RDONLY);
    if (ERROR == fd)
    {
        HDCP_ASSERTMESSAGE(
                    "Could not open the SRM file. Err: %s",
                    strerror(errno));
        return;
    }

    struct stat sb  = {};
    if (ERROR == fstat(fd, &sb))
    {
        HDCP_ASSERTMESSAGE(
                    "Could not get stats for the SRM file. Err: %s",
                    strerror(errno));
        close(fd);
        return;
    }

    if (!S_ISREG(sb.st_mode))
    {
        HDCP_ASSERTMESSAGE("SRM file is not a regular file");
        close(fd);
        return;
    }

    if (0 == sb.st_size)
    {
        // There is nothing in the file.
        // Our work here is done!
        close(fd);
        return;
    }

    void *buf = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (MAP_FAILED == buf)
    {
        HDCP_ASSERTMESSAGE(
                "Failed to map a buffer for the SRM file. Err: %s",
                strerror(errno));
        close(fd);
        return;
    }

    sts = RetrieveSrmFromBuffer(static_cast<uint8_t*>(buf), sb.st_size);
    munmap(buf, sb.st_size);
    if (SUCCESS != sts)
    {
        HDCP_ASSERTMESSAGE(
                "Failed to retrieve SRM list from non-volatile storage!");
        close(fd);
        return;
    }

    close(fd);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return;
}

SrmTable::~SrmTable(void)
{
    HDCP_FUNCTION_ENTER;

    VectorRevocationList *vrl = nullptr;

    if (nullptr == m_RevocationListsByGeneration)
    {
        // Doesn't exist... this would be weird... but harmless
        return;
    }

    ACQUIRE_LOCK(&m_RevocationListMutex);
    while (!m_RevocationListsByGeneration->empty())
    {
        vrl = m_RevocationListsByGeneration->back();
        delete vrl;
        m_RevocationListsByGeneration->pop_back();
    }

    delete m_RevocationListsByGeneration;
    RELEASE_LOCK(&m_RevocationListMutex);

    DESTROY_LOCK(&m_RevocationListMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return;
}

bool SrmTable::IsValid(void)
{
    return m_IsValid;
}

void SrmTable::SetSrmStorageDisable(const bool disableSrmStorage)
{
    m_IsSrmStorageDisable = disableSrmStorage;
}

bool SrmTable::IsSrmStorageDisable(void)
{
    return m_IsSrmStorageDisable;
}

int32_t SrmTable::RetrieveSrmFromBuffer(const uint8_t *buf, const size_t length)
{
    HDCP_FUNCTION_ENTER;

    int32_t     ret             = EINVAL;

    SrmHeader   srmHeader       = {};
    uint32_t    vrlLength       = 0;
    const uint8_t     *vrlList         = nullptr;
    const uint8_t     *signatureR      = nullptr;
    const uint8_t     *signatureS      = nullptr;

    uint32_t    offset          = 0;
    uint32_t    gen1BufLength   = 0;

    VectorRevocationList                *vrl                = nullptr;
    std::list<VectorRevocationList*>    *revocationLists    = nullptr;

    CHECK_PARAM_NULL(buf, EINVAL);

    // Make sure the length is at least large enough for us to get header info
    // and also the 3 bytes of VRL length
    if (length < (SRM_HEADER_LENGTH + 3))
    {
        HDCP_ASSERTMESSAGE("Buffer not large enough to contain a header!");
        ret = EINVAL;
    }

    // Grab the header from the new message
    srmHeader.srm_id        =  buf[offset + 0] >> 4;
    srmHeader.version       =  buf[offset + 3];
    srmHeader.version       |= buf[offset + 2] << 8;
    srmHeader.generation    =  buf[offset + 4];
    offset += SRM_HEADER_LENGTH;

    if (srmHeader.srm_id != SRM_HEADER_ID)
    {
        HDCP_ASSERTMESSAGE("Buffer does not have SRM header format!");
        return EINVAL;
    }
    if (srmHeader.version < m_Version)
    {
        // Our SRM info is more up-to-date than the sender's
        HDCP_ASSERTMESSAGE("The SRM version isn't newer than current!");
        return EAGAIN;
    }

    // The first 3 bytes of the buffer contain length of the gen1 VRL
    vrlLength = 0;
    vrlLength |= buf[offset++] << 16;
    vrlLength |= buf[offset++] << 8;
    vrlLength |= buf[offset++] << 0;

    gen1BufLength = SRM_HEADER_LENGTH + vrlLength;
    if (gen1BufLength > length)
    {
        HDCP_ASSERTMESSAGE("Buffer is too short to contain SRM information!");
        return EINVAL;
    }
    // This gets used in VerifySignature for SHA calculation.
    // It should not contain the S/R signatures
    gen1BufLength -= (2 * DSA_SIG_LENGTH);

    // vrlLength contains:
    //      3 bytes required for the length itself
    //      40 bytes of signature data (2 signatures) at the end of the buffer
    //      variable number of bytes of actual VRL lists
    if (vrlLength < (3 + (2 * DSA_SIG_LENGTH)))
    {
        HDCP_ASSERTMESSAGE("VRL length could not fit DSA sig and length bits!");
        return EINVAL;
    }
    vrlLength -= 3;
    vrlLength -= (2 * DSA_SIG_LENGTH);

    // After that comes the variable length VRL lists
    vrlList = &buf[offset];
    offset += vrlLength; 

    // And next comes the r & s values of the sigature
    // (using DSA - Digital Signature Algorithm)
    signatureR = &buf[offset];
    offset += DSA_SIG_LENGTH;
    signatureS = &buf[offset];
    offset += DSA_SIG_LENGTH;

    // Now validate the signature of the header & data
    // (only for gen1 vrl, for gen2+ only validate the data)
    ret = VerifySignature(buf, gen1BufLength, signatureR, signatureS); 
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Could not verify DSA signature of buffer!");
        return ret;
    }

    // At this point we need to start building the new list of VRLs
    revocationLists = new (std::nothrow) std::list<VectorRevocationList*>;
    if (revocationLists == nullptr)
    {
        HDCP_ASSERTMESSAGE("Failed to allocate a new list of VRLs!");
        return ENOMEM;
    }

    vrl = new (std::nothrow) VectorRevocationList(vrlList, vrlLength);
    if (vrl == nullptr)
    {
        HDCP_ASSERTMESSAGE("Allocation of new VectorRevocationList failed!");
        ret = ENOMEM;
        goto out_kill_list;
    }
    if (!vrl->IsValid())
    {
        HDCP_ASSERTMESSAGE("Creation of new VectorRevocationList failed!");
        ret = EINVAL;
        delete vrl;
        goto out_kill_list;
    }

    revocationLists->push_back(vrl);

    while (offset < length)
    {
        // Make sure enough msg left to read the next VRL length (2 bytes)
        if (offset + 2 > length)
        {
            HDCP_ASSERTMESSAGE("VRL header is too small to read!");
            ret = EINVAL;
            goto out_kill_list;
        }
        else
        {
            // The first 2 bytes of the buffer contain length of the gen2+ VRL
            vrlLength = 0;
            vrlLength |= buf[offset++] << 8;
            vrlLength |= buf[offset++] << 0;
            if ((offset + vrlLength) > length)
            {
                HDCP_ASSERTMESSAGE(
                    "VRL length doesn't match the proclaimed length!");
                ret = EINVAL;
                goto out_kill_list;
            }
            
            // vrlLength contains:
            //      2 bytes required for the length itself
            //      40 bytes of signature data (2 signatures R/S)
            //      variable number of bytes of actual VRL lists
            if (vrlLength < (2 + (2 * DSA_SIG_LENGTH)))
            {
                HDCP_ASSERTMESSAGE(
                    "VRL length could not fit DSA sig and length bits!");
                ret = EINVAL;
                goto out_kill_list;
            }
            else
            {
                vrlLength -= 2;
                vrlLength -= (2 * DSA_SIG_LENGTH);
            
                // After that comes the variable length VRL lists
                vrlList = &buf[offset];
                offset += vrlLength; 
            
                // And next comes the r & s values of the sigature
                // (using DSA - Digital Signature Algorithm)
                signatureR = &buf[offset];
                offset += DSA_SIG_LENGTH;
                signatureS = &buf[offset];
                offset += DSA_SIG_LENGTH;
            
                HDCP_ASSERTMESSAGE("vrlLength %d", vrlLength);
                // Now validate the signature of the data
                ret = VerifySignature(
                                vrlList,
                                vrlLength,
                                signatureR,
                                signatureS); 
                if (SUCCESS != ret)
                {
                    HDCP_ASSERTMESSAGE(
                            "Failed to verify the DSA signature of the VRL!");
                    goto out_kill_list;
                }
            
                vrl = new (std::nothrow) VectorRevocationList(
                                                    vrlList,
                                                    vrlLength);
                if (nullptr == vrl)
                {
                    HDCP_ASSERTMESSAGE(
                            "Allocation of new VectorRevocationList failed!");
                    ret = ENOMEM;
                    goto out_kill_list;
                }
                if (!vrl->IsValid())
                {
                    HDCP_ASSERTMESSAGE(
                            "Creation of new VectorRevocationList failed!");
                    ret = EINVAL;
                    delete vrl;
                    goto out_kill_list;
                }
            
                revocationLists->push_back(vrl);
            }
        }   
    }

    // If we hit this, the new Srm Table was built successfully
    // We can destroy the old list
    ACQUIRE_LOCK(&m_RevocationListMutex);
    while (!m_RevocationListsByGeneration->empty())
    {
        vrl = m_RevocationListsByGeneration->back();
        delete vrl;
        m_RevocationListsByGeneration->pop_back();
    }
    delete m_RevocationListsByGeneration;
    m_RevocationListsByGeneration = revocationLists;
    m_Version = srmHeader.version;
    m_Generation = srmHeader.generation;
    RELEASE_LOCK(&m_RevocationListMutex);

    ret = SUCCESS;
    goto out;

out_kill_list:
    while (!revocationLists->empty())
    {
        vrl = revocationLists->back();
        delete vrl;
        revocationLists->pop_back();
    }
    delete revocationLists;
out:
    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t SrmTable::CheckSrmRevoke(const uint8_t ksv[KSV_SIZE])
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(ksv, EINVAL);
    
    ACQUIRE_LOCK(&m_RevocationListMutex);

    if (nullptr == m_RevocationListsByGeneration)
    {
        // This is extremely rare state that is never expected to be hit
        // An srm call was made while the SrmTable is being torn down
        RELEASE_LOCK(&m_RevocationListMutex);
        return ENODEV;
    }

    for (auto vrl = m_RevocationListsByGeneration->begin();
            vrl != m_RevocationListsByGeneration->end();
            ++vrl)
    {
        if (nullptr == (*vrl))
        {
            // This is impossibly inconsistent state.
            continue;
        }

        if ((*vrl)->ContainsKsv(ksv))
        {
            // Our KSV is on the revocation list!
            RELEASE_LOCK(&m_RevocationListMutex);
            return EACCES;
        }
    }
    RELEASE_LOCK(&m_RevocationListMutex);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t SrmTable::GetSrmVersion(uint16_t *version)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(version, EINVAL);
    *version = m_Version;

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t GetSrmVersion(uint16_t *version)
{
    HDCP_FUNCTION_ENTER;

    CHECK_PARAM_NULL(version, EINVAL);

    if (nullptr == g_pSrmTable)
    {
        HDCP_ASSERTMESSAGE("SrmModule has not been initialized correctly!");
        return ENODEV;
    }

    uint32_t ret = g_pSrmTable->GetSrmVersion(version);

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t StoreSrm(const uint8_t *data, const uint32_t size)
{
    HDCP_FUNCTION_ENTER;

    if (g_pSrmTable == nullptr)
    {
        HDCP_ASSERTMESSAGE("SrmModule has not been initialized correctly!");
        return ENODEV;
    }

    // Update our current copy of the SRM
    int32_t ret = g_pSrmTable->RetrieveSrmFromBuffer(data, size);
    if (SUCCESS != ret)
    {
        HDCP_ASSERTMESSAGE("Failed to update local copy of SRM!");
        return ret;
    }

    if (!g_pSrmTable->IsSrmStorageDisable())
    {
        // Store our copy of the SRM to non-volatile storage
        int32_t fd = open(
                    SRM_STORAGE_FILENAME,
                    O_WRONLY|O_CREAT,
                    S_IRUSR|S_IWUSR);
        if (ERROR == fd)
        {
            HDCP_ASSERTMESSAGE(
                    "Could not open non-volatile storage to save SRM file %s!",
                    SRM_STORAGE_FILENAME);
            return EIO;
        }

        size_t bytesWritten = write(fd, data, size);
        if (bytesWritten != static_cast<size_t>(size))
        {
            HDCP_ASSERTMESSAGE(
            "Wrote a different number of bytes to"
            "storage than srm message length!");

            close(fd);
            return EIO;
        }

        close(fd);
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

int32_t CheckSrmRevoke(const uint8_t ksv[KSV_SIZE])
{
    HDCP_FUNCTION_ENTER;

    int32_t ret = ENODEV;

    if (g_pSrmTable != nullptr)
    {
        ret = g_pSrmTable->CheckSrmRevoke(ksv);
    }
    else
    {
        HDCP_ASSERTMESSAGE("SrmTable is nullptr!");
    }

    HDCP_FUNCTION_EXIT(ret);
    return ret;
}

int32_t SrmInit(void)
{
    HDCP_FUNCTION_ENTER;

    if (nullptr != g_pSrmTable)
    {
        HDCP_WARNMESSAGE("Srm module has already been initialized!");
        return EEXIST;
    }

    g_pSrmTable = new (std::nothrow) SrmTable;
    if (nullptr == g_pSrmTable)
    {
        HDCP_ASSERTMESSAGE("Failed to allocate the SrmTable module!");
        return ENOMEM;
    }

    if (!g_pSrmTable->IsValid())
    {
        HDCP_ASSERTMESSAGE("Failed to initialize the SrmTable!");
        delete g_pSrmTable;
        g_pSrmTable = nullptr;
        return EINVAL;
    }

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}

void SrmRelease(void)
{
    HDCP_FUNCTION_ENTER;

    delete g_pSrmTable;
    g_pSrmTable = nullptr;

    HDCP_FUNCTION_EXIT(SUCCESS);
}

// this API is designed to let customer config whether to
// store SRM by daemon or not.
//
// But a potential security risk is: if another customer wants daemon to store
// the SRM, and some other customer purposely calls this API to disable storing
// SRM by daemon, SRM won't be stored on disk in this case.
//
// In the future, it's better to force daemon to save SRM in a secure storage to
// avoid user delete it purposely or accidentally, and to forbid user config the
// SRM storage.
int32_t SrmConfig(const bool disableSrmStorage)
{
    HDCP_FUNCTION_ENTER;

    if (nullptr == g_pSrmTable)
    {
        HDCP_ASSERTMESSAGE("SrmModule has not been initialized correctly!");
        return ENODEV;
    }

    g_pSrmTable->SetSrmStorageDisable(disableSrmStorage);

    HDCP_FUNCTION_EXIT(SUCCESS);
    return SUCCESS;
}
