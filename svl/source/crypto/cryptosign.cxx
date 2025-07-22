/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <algorithm>
#include <array>

#include <svl/cryptosign.hxx>
#include <svl/sigstruct.hxx>
#include <config_crypto.h>
#include <o3tl/numeric.hxx>

#if USE_CRYPTO_NSS
#include <systools/curlinit.hxx>
#endif

#include <rtl/character.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/string.hxx>
#include <sal/log.hxx>
#include <tools/datetime.hxx>
#include <tools/stream.hxx>
#include <comphelper/base64.hxx>
#include <comphelper/hash.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/random.hxx>
#include <comphelper/scopeguard.hxx>
#include <comphelper/lok.hxx>
#include <com/sun/star/security/XCertificate.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <o3tl/char16_t2wchar_t.hxx>

#if USE_CRYPTO_NSS
// NSS headers for PDF signing
#include <cert.h>
#include <keyhi.h>
#include <pk11pub.h>
#include <hasht.h>
#include <secerr.h>
#include <sechash.h>
#include <cms.h>
#include <cmst.h>

// We use curl for RFC3161 time stamp requests
#include <curl/curl.h>

#include <com/sun/star/xml/crypto/DigestID.hpp>
#include <com/sun/star/xml/crypto/NSSInitializer.hpp>
#include <mutex>
#endif

#if USE_CRYPTO_MSCAPI
// WinCrypt headers for PDF signing
// Note: this uses Windows 7 APIs and requires the relevant data types
#include <prewin.h>
#include <wincrypt.h>
#include <postwin.h>
#include <comphelper/windowserrorstring.hxx>
#endif

using namespace com::sun::star;

namespace {
#if USE_CRYPTO_NSS
char *PDFSigningPKCS7PasswordCallback(PK11SlotInfo * /*slot*/, PRBool /*retry*/, void *arg)
{
    return PL_strdup(static_cast<char *>(arg));
}

// ASN.1 used in the (much simpler) time stamp request. From RFC3161
// and other sources.

/*
AlgorithmIdentifier  ::=  SEQUENCE  {
     algorithm  OBJECT IDENTIFIER,
     parameters ANY DEFINED BY algorithm OPTIONAL  }
                   -- contains a value of the type
                   -- registered for use with the
                   -- algorithm object identifier value

MessageImprint ::= SEQUENCE  {
    hashAlgorithm AlgorithmIdentifier,
    hashedMessage OCTET STRING  }
*/

struct MessageImprint {
    SECAlgorithmID hashAlgorithm;
    SECItem hashedMessage;
};

/*
Extension  ::=  SEQUENCE  {
    extnID    OBJECT IDENTIFIER,
    critical  BOOLEAN DEFAULT FALSE,
    extnValue OCTET STRING  }
*/

struct Extension {
    SECItem  extnID;
    SECItem  critical;
    SECItem  extnValue;
};

/*
Extensions ::= SEQUENCE SIZE (1..MAX) OF Extension
*/

/*
TSAPolicyId ::= OBJECT IDENTIFIER

TimeStampReq ::= SEQUENCE  {
    version            INTEGER  { v1(1) },
    messageImprint     MessageImprint,
    --a hash algorithm OID and the hash value of the data to be
    --time-stamped
    reqPolicy          TSAPolicyId         OPTIONAL,
    nonce              INTEGER             OPTIONAL,
    certReq            BOOLEAN             DEFAULT FALSE,
    extensions     [0] IMPLICIT Extensions OPTIONAL  }
*/

struct TimeStampReq {
    SECItem version;
    MessageImprint messageImprint;
    SECItem reqPolicy;
    SECItem nonce;
    SECItem certReq;
    Extension *extensions;
};

/**
 * General name, defined by RFC 3280.
 */
struct GeneralName
{
    CERTName name;
};

/**
 * List of general names (only one for now), defined by RFC 3280.
 */
struct GeneralNames
{
    GeneralName names;
};

/**
 * Supplies different fields to identify a certificate, defined by RFC 5035.
 */
struct IssuerSerial
{
    GeneralNames issuer;
    SECItem serialNumber;
};

/**
 * Supplies different fields that are used to identify certificates, defined by
 * RFC 5035.
 */
struct ESSCertIDv2
{
    SECAlgorithmID hashAlgorithm;
    SECItem certHash;
    IssuerSerial issuerSerial;
};

/**
 * This attribute uses the ESSCertIDv2 structure, defined by RFC 5035.
 */
struct SigningCertificateV2
{
    ESSCertIDv2** certs;

    SigningCertificateV2()
        : certs(nullptr)
    {
    }
};

/**
 * GeneralName ::= CHOICE {
 *      otherName                       [0]     OtherName,
 *      rfc822Name                      [1]     IA5String,
 *      dNSName                         [2]     IA5String,
 *      x400Address                     [3]     ORAddress,
 *      directoryName                   [4]     Name,
 *      ediPartyName                    [5]     EDIPartyName,
 *      uniformResourceIdentifier       [6]     IA5String,
 *      iPAddress                       [7]     OCTET STRING,
 *      registeredID                    [8]     OBJECT IDENTIFIER
 * }
 */
const SEC_ASN1Template GeneralNameTemplate[] =
{
    {SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(GeneralName)},
    {SEC_ASN1_INLINE, offsetof(GeneralName, name), CERT_NameTemplate, 0},
    {0, 0, nullptr, 0}
};

/**
 * GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName
 */
const SEC_ASN1Template GeneralNamesTemplate[] =
{
    {SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(GeneralNames)},
    {SEC_ASN1_INLINE | SEC_ASN1_CONTEXT_SPECIFIC | 4, offsetof(GeneralNames, names), GeneralNameTemplate, 0},
    {0, 0, nullptr, 0}
};

/**
 * IssuerSerial ::= SEQUENCE {
 *     issuer GeneralNames,
 *     serialNumber CertificateSerialNumber
 * }
 */
const SEC_ASN1Template IssuerSerialTemplate[] =
{
    {SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(IssuerSerial)},
    {SEC_ASN1_INLINE, offsetof(IssuerSerial, issuer), GeneralNamesTemplate, 0},
    {SEC_ASN1_INTEGER, offsetof(IssuerSerial, serialNumber), nullptr, 0},
    {0, 0, nullptr, 0}
};


/**
 * Hash ::= OCTET STRING
 *
 * ESSCertIDv2 ::= SEQUENCE {
 *     hashAlgorithm AlgorithmIdentifier DEFAULT {algorithm id-sha256},
 *     certHash Hash,
 *     issuerSerial IssuerSerial OPTIONAL
 * }
 */

SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)

const SEC_ASN1Template ESSCertIDv2Template[] =
{
    {SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(ESSCertIDv2)},
    {SEC_ASN1_INLINE | SEC_ASN1_XTRN, offsetof(ESSCertIDv2, hashAlgorithm), SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate), 0},
    {SEC_ASN1_OCTET_STRING, offsetof(ESSCertIDv2, certHash), nullptr, 0},
    {SEC_ASN1_INLINE | SEC_ASN1_XTRN, offsetof(ESSCertIDv2, issuerSerial), IssuerSerialTemplate, 0},
    {0, 0, nullptr, 0}
};

/**
 * SigningCertificateV2 ::= SEQUENCE {
 * }
 */
const SEC_ASN1Template SigningCertificateV2Template[] =
{
    {SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(SigningCertificateV2)},
    {SEC_ASN1_SEQUENCE_OF, offsetof(SigningCertificateV2, certs), ESSCertIDv2Template, 0},
    {0, 0, nullptr, 0}
};

struct PKIStatusInfo {
    SECItem status;
    SECItem statusString;
    SECItem failInfo;
};

const SEC_ASN1Template PKIStatusInfo_Template[] =
{
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(PKIStatusInfo) },
    { SEC_ASN1_INTEGER, offsetof(PKIStatusInfo, status), nullptr, 0 },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_SEQUENCE | SEC_ASN1_OPTIONAL, offsetof(PKIStatusInfo, statusString), nullptr, 0 },
    { SEC_ASN1_BIT_STRING | SEC_ASN1_OPTIONAL, offsetof(PKIStatusInfo, failInfo), nullptr, 0 },
    { 0, 0, nullptr, 0 }
};

const SEC_ASN1Template Any_Template[] =
{
    { SEC_ASN1_ANY, 0, nullptr, sizeof(SECItem) }
};

struct TimeStampResp {
    PKIStatusInfo status;
    SECItem timeStampToken;
};

const SEC_ASN1Template TimeStampResp_Template[] =
{
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(TimeStampResp) },
    { SEC_ASN1_INLINE, offsetof(TimeStampResp, status), PKIStatusInfo_Template, 0 },
    { SEC_ASN1_ANY | SEC_ASN1_OPTIONAL, offsetof(TimeStampResp, timeStampToken), Any_Template, 0 },
    { 0, 0, nullptr, 0 }
};

const SEC_ASN1Template MessageImprint_Template[] =
{
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(MessageImprint) },
    { SEC_ASN1_INLINE, offsetof(MessageImprint, hashAlgorithm), SECOID_AlgorithmIDTemplate, 0 },
    { SEC_ASN1_OCTET_STRING, offsetof(MessageImprint, hashedMessage), nullptr, 0 },
    { 0, 0, nullptr, 0 }
};

const SEC_ASN1Template Extension_Template[] =
{
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(Extension) },
    { SEC_ASN1_OBJECT_ID, offsetof(Extension, extnID), nullptr, 0 },
    { SEC_ASN1_BOOLEAN, offsetof(Extension, critical), nullptr, 0 },
    { SEC_ASN1_OCTET_STRING, offsetof(Extension, extnValue), nullptr, 0 },
    { 0, 0, nullptr, 0 }
};

const SEC_ASN1Template Extensions_Template[] =
{
    { SEC_ASN1_SEQUENCE_OF, 0, Extension_Template, 0 }
};

const SEC_ASN1Template TimeStampReq_Template[] =
{
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(TimeStampReq) },
    { SEC_ASN1_INTEGER, offsetof(TimeStampReq, version), nullptr, 0 },
    { SEC_ASN1_INLINE, offsetof(TimeStampReq, messageImprint), MessageImprint_Template, 0 },
    { SEC_ASN1_OBJECT_ID | SEC_ASN1_OPTIONAL, offsetof(TimeStampReq, reqPolicy), nullptr, 0 },
    { SEC_ASN1_INTEGER | SEC_ASN1_OPTIONAL, offsetof(TimeStampReq, nonce), nullptr, 0 },
    { SEC_ASN1_BOOLEAN | SEC_ASN1_OPTIONAL, offsetof(TimeStampReq, certReq), nullptr, 0 },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | 0, offsetof(TimeStampReq, extensions), Extensions_Template, 0 },
    { 0, 0, nullptr, 0 }
};

// 1.2.840.113549.1.9.16.2.47
constexpr unsigned char OID_SIGNINGCERTIFICATEV2[] {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x10, 0x02, 0x2f};
// 1.2.840.113549.1.9.16.2.14
constexpr unsigned char OID_TIMESTAMPTOKEN[] {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x10, 0x02, 0x0e};

struct cms_recode_attribute {
  SECItem type;
  SECItem **values;
};

struct cms_recode_signer_info {
  SECItem version;
  SECItem signerIdentifier;
  SECItem digestAlg;
  SECItem authAttr;
  SECItem digestEncAlg;
  SECItem encDigest;
  cms_recode_attribute **unAuthAttr;
};

struct cms_recode_signed_data {
  SECItem version;
  SECItem digestAlgorithms;
  SECItem contentInfo;
  SECItem rawCerts;
  SECItem crls;
  cms_recode_signer_info **signerInfos;
};

struct cms_recode_message {
  SECItem contentType;
  cms_recode_signed_data signedData;
};

const SEC_ASN1Template recode_attribute_template[] = {
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(cms_recode_attribute) },
    { SEC_ASN1_OBJECT_ID, offsetof(cms_recode_attribute, type), nullptr, 0 },
    { SEC_ASN1_SET_OF, offsetof(cms_recode_attribute, values), SEC_AnyTemplate, 0 },
    {0, 0, nullptr, 0}};

const SEC_ASN1Template recode_set_of_attribute_template[] = {
    { SEC_ASN1_SET_OF, 0, recode_attribute_template, 0 }
};

const SEC_ASN1Template recode_signer_info_template[] = {
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(cms_recode_signer_info) },
    { SEC_ASN1_ANY, offsetof(cms_recode_signer_info, version), nullptr, 0 },
    { SEC_ASN1_ANY, offsetof(cms_recode_signer_info, signerIdentifier), nullptr, 0 },
    { SEC_ASN1_ANY, offsetof(cms_recode_signer_info, digestAlg), nullptr, 0 },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
      offsetof(cms_recode_signer_info, authAttr), SEC_AnyTemplate, 0 },
    { SEC_ASN1_ANY, offsetof(cms_recode_signer_info, digestEncAlg), nullptr, 0 },
    { SEC_ASN1_ANY, offsetof(cms_recode_signer_info, encDigest), nullptr, 0 },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1,
      offsetof(cms_recode_signer_info, unAuthAttr), recode_set_of_attribute_template, 0 },
    {0, 0, nullptr, 0}};

const SEC_ASN1Template recode_signed_data_template[] = {
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(cms_recode_signed_data) },
    { SEC_ASN1_ANY, offsetof(cms_recode_signed_data, version), nullptr, 0 },
    { SEC_ASN1_ANY, offsetof(cms_recode_signed_data, digestAlgorithms), nullptr, 0 },
    { SEC_ASN1_ANY, offsetof(cms_recode_signed_data, contentInfo), nullptr, 0 },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
      offsetof(cms_recode_signed_data, rawCerts), SEC_AnyTemplate, 0 },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1,
      offsetof(cms_recode_signed_data, crls), SEC_AnyTemplate, 0 },
    { SEC_ASN1_SET_OF, offsetof(cms_recode_signed_data, signerInfos), recode_signer_info_template,
      0 },
    {0, 0, nullptr, 0}};

const SEC_ASN1Template recode_message_template[] = {
    { SEC_ASN1_SEQUENCE, 0, nullptr, sizeof(cms_recode_message) },
    { SEC_ASN1_ANY, offsetof(cms_recode_message, contentType), nullptr, 0 },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
      offsetof(cms_recode_message, signedData), recode_signed_data_template, 0 },
    {0, 0, nullptr, 0}};

size_t AppendToBuffer(char const *ptr, size_t size, size_t nmemb, void *userdata)
{
    OStringBuffer *pBuffer = static_cast<OStringBuffer*>(userdata);
    pBuffer->append(ptr, size*nmemb);

    return size*nmemb;
}

OUString PKIStatusToString(int n)
{
    switch (n)
    {
    case 0: return u"granted"_ustr;
    case 1: return u"grantedWithMods"_ustr;
    case 2: return u"rejection"_ustr;
    case 3: return u"waiting"_ustr;
    case 4: return u"revocationWarning"_ustr;
    case 5: return u"revocationNotification"_ustr;
    default: return "unknown (" + OUString::number(n) + ")";
    }
}

OUString PKIStatusInfoToString(const PKIStatusInfo& rStatusInfo)
{
    OUString result = u"{status="_ustr;
    if (rStatusInfo.status.len == 1)
        result += PKIStatusToString(rStatusInfo.status.data[0]);
    else
        result += "unknown (len=" + OUString::number(rStatusInfo.status.len);

    // FIXME: Perhaps look at rStatusInfo.statusString.data but note
    // that we of course can't assume it contains proper UTF-8. After
    // all, it is data from an external source. Also, RFC3161 claims
    // it should be a SEQUENCE (1..MAX) OF UTF8String, but another
    // source claimed it would be a single UTF8String, hmm?

    // FIXME: Worth it to decode failInfo to cleartext, probably not at least as long as this is only for a SAL_INFO

    result += "}";

    return result;
}

// SEC_StringToOID() and NSS_CMSSignerInfo_AddUnauthAttr() are
// not exported from libsmime, so copy them here. Sigh.

NSSCMSAttribute *
my_NSS_CMSAttributeArray_FindAttrByOidTag(NSSCMSAttribute **attrs, SECOidTag oidtag, PRBool only)
{
    SECOidData *oid;
    NSSCMSAttribute *attr1, *attr2;

    if (attrs == nullptr)
        return nullptr;

    oid = SECOID_FindOIDByTag(oidtag);
    if (oid == nullptr)
        return nullptr;

    while ((attr1 = *attrs++) != nullptr) {
    if (attr1->type.len == oid->oid.len && PORT_Memcmp (attr1->type.data,
                                oid->oid.data,
                                oid->oid.len) == 0)
        break;
    }

    if (attr1 == nullptr)
        return nullptr;

    if (!only)
        return attr1;

    while ((attr2 = *attrs++) != nullptr) {
    if (attr2->type.len == oid->oid.len && PORT_Memcmp (attr2->type.data,
                                oid->oid.data,
                                oid->oid.len) == 0)
        break;
    }

    if (attr2 != nullptr)
        return nullptr;

    return attr1;
}

SECStatus
my_NSS_CMSArray_Add(PLArenaPool *poolp, void ***array, void *obj)
{
    int n = 0;
    void **dest;

    PORT_Assert(array != NULL);
    if (array == nullptr)
        return SECFailure;

    if (*array == nullptr) {
        dest = static_cast<void **>(PORT_ArenaAlloc(poolp, 2 * sizeof(void *)));
    } else {
        void **p = *array;
        while (*p++)
            n++;
        dest = static_cast<void **>(PORT_ArenaGrow (poolp,
                      *array,
                      (n + 1) * sizeof(void *),
                      (n + 2) * sizeof(void *)));
    }

    if (dest == nullptr)
        return SECFailure;

    dest[n] = obj;
    dest[n+1] = nullptr;
    *array = dest;
    return SECSuccess;
}

SECOidTag
my_NSS_CMSAttribute_GetType(const NSSCMSAttribute *attr)
{
    SECOidData *typetag;

    typetag = SECOID_FindOID(&(attr->type));
    if (typetag == nullptr)
        return SEC_OID_UNKNOWN;

    return typetag->offset;
}

SECStatus
my_NSS_CMSAttributeArray_AddAttr(PLArenaPool *poolp, NSSCMSAttribute ***attrs, NSSCMSAttribute *attr)
{
    NSSCMSAttribute *oattr;
    void *mark;
    SECOidTag type;

    mark = PORT_ArenaMark(poolp);

    /* find oidtag of attr */
    type = my_NSS_CMSAttribute_GetType(attr);

    /* see if we have one already */
    oattr = my_NSS_CMSAttributeArray_FindAttrByOidTag(*attrs, type, PR_FALSE);
    PORT_Assert (oattr == NULL);
    if (oattr != nullptr)
        goto loser; /* XXX or would it be better to replace it? */

    /* no, shove it in */
    if (my_NSS_CMSArray_Add(poolp, reinterpret_cast<void ***>(attrs), static_cast<void *>(attr)) != SECSuccess)
        goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(poolp, mark);
    return SECFailure;
}

SECStatus
my_NSS_CMSSignerInfo_AddAuthAttr(NSSCMSSignerInfo *signerinfo, NSSCMSAttribute *attr)
{
    return my_NSS_CMSAttributeArray_AddAttr(signerinfo->cmsg->poolp, &(signerinfo->authAttr), attr);
}

NSSCMSMessage *CreateCMSMessage(const PRTime* time,
                                NSSCMSSignedData **cms_sd,
                                NSSCMSSignerInfo **cms_signer,
                                CERTCertificate *cert,
                                SECItem *digest)
{
    NSSCMSMessage *result = NSS_CMSMessage_Create(nullptr);
    if (!result)
    {
        SAL_WARN("svl.crypto", "NSS_CMSMessage_Create failed");
        return nullptr;
    }

    *cms_sd = NSS_CMSSignedData_Create(result);
    if (!*cms_sd)
    {
        SAL_WARN("svl.crypto", "NSS_CMSSignedData_Create failed");
        NSS_CMSMessage_Destroy(result);
        return nullptr;
    }

    NSSCMSContentInfo *cms_cinfo = NSS_CMSMessage_GetContentInfo(result);
    if (NSS_CMSContentInfo_SetContent_SignedData(result, cms_cinfo, *cms_sd) != SECSuccess)
    {
        SAL_WARN("svl.crypto", "NSS_CMSContentInfo_SetContent_SignedData failed");
        NSS_CMSSignedData_Destroy(*cms_sd);
        NSS_CMSMessage_Destroy(result);
        return nullptr;
    }

    cms_cinfo = NSS_CMSSignedData_GetContentInfo(*cms_sd);

    // Attach NULL data as detached data
    if (NSS_CMSContentInfo_SetContent_Data(result, cms_cinfo, nullptr, PR_TRUE) != SECSuccess)
    {
        SAL_WARN("svl.crypto", "NSS_CMSContentInfo_SetContent_Data failed");
        NSS_CMSSignedData_Destroy(*cms_sd);
        NSS_CMSMessage_Destroy(result);
        return nullptr;
    }

    // workaround: with legacy "dbm:", NSS can't find the private key - try out
    // if it works, and fallback if it doesn't.
    if (SECKEYPrivateKey * pPrivateKey = PK11_FindKeyByAnyCert(cert, nullptr))
    {
        if (!comphelper::LibreOfficeKit::isActive())
        {
            // pPrivateKey only exists in the memory in the LOK case, don't delete it.
            SECKEY_DestroyPrivateKey(pPrivateKey);
        }
        *cms_signer = NSS_CMSSignerInfo_Create(result, cert, SEC_OID_SHA256);
    }
    else
    {
        pPrivateKey = PK11_FindKeyByDERCert(cert->slot, cert, nullptr);
        SECKEYPublicKey *const pPublicKey = CERT_ExtractPublicKey(cert);
        if (pPublicKey && pPrivateKey)
        {
            *cms_signer = NSS_CMSSignerInfo_CreateWithSubjKeyID(result, &cert->subjectKeyID, pPublicKey, pPrivateKey, SEC_OID_SHA256);
            SECKEY_DestroyPrivateKey(pPrivateKey);
            SECKEY_DestroyPublicKey(pPublicKey);
            if (*cms_signer)
            {
                // this is required in NSS_CMSSignerInfo_IncludeCerts()
                // (and NSS_CMSSignerInfo_GetSigningCertificate() doesn't work)
                (**cms_signer).cert = CERT_DupCertificate(cert);
            }
        }
    }
    if (!*cms_signer)
    {
        SAL_WARN("svl.crypto", "NSS_CMSSignerInfo_Create failed");
        NSS_CMSSignedData_Destroy(*cms_sd);
        NSS_CMSMessage_Destroy(result);
        return nullptr;
    }

    if (time && NSS_CMSSignerInfo_AddSigningTime(*cms_signer, *time) != SECSuccess)
    {
        SAL_WARN("svl.crypto", "NSS_CMSSignerInfo_AddSigningTime failed");
        NSS_CMSSignedData_Destroy(*cms_sd);
        NSS_CMSMessage_Destroy(result);
        return nullptr;
    }

    if (NSS_CMSSignerInfo_IncludeCerts(*cms_signer, NSSCMSCM_CertChain, certUsageEmailSigner) != SECSuccess)
    {
        SAL_WARN("svl.crypto", "NSS_CMSSignerInfo_IncludeCerts failed");
        NSS_CMSSignedData_Destroy(*cms_sd);
        NSS_CMSMessage_Destroy(result);
        return nullptr;
    }

    if (NSS_CMSSignedData_AddSignerInfo(*cms_sd, *cms_signer) != SECSuccess)
    {
        SAL_WARN("svl.crypto", "NSS_CMSSignedData_AddSignerInfo failed");
        NSS_CMSSignedData_Destroy(*cms_sd);
        NSS_CMSMessage_Destroy(result);
        return nullptr;
    }

    if (NSS_CMSSignedData_SetDigestValue(*cms_sd, SEC_OID_SHA256, digest) != SECSuccess)
    {
        SAL_WARN("svl.crypto", "NSS_CMSSignedData_SetDigestValue failed");
        NSS_CMSSignedData_Destroy(*cms_sd);
        NSS_CMSMessage_Destroy(result);
        return nullptr;
    }

    return result;
}

#elif USE_CRYPTO_MSCAPI // ends USE_CRYPTO_NSS

/// Counts how many bytes are needed to encode a given length.
size_t GetDERLengthOfLength(size_t nLength)
{
    size_t nRet = 1;

    if(nLength > 127)
    {
        while (nLength >> (nRet * 8))
            ++nRet;
        // Long form means one additional byte: the length of the length and
        // the length itself.
        ++nRet;
    }
    return nRet;
}

/// Writes the length part of the header.
void WriteDERLength(SvStream& rStream, size_t nLength)
{
    size_t nLengthOfLength = GetDERLengthOfLength(nLength);
    if (nLengthOfLength == 1)
    {
        // We can use the short form.
        rStream.WriteUInt8(nLength);
        return;
    }

    // 0x80 means that the we use the long form: the first byte is the length
    // of length with the highest bit set to 1, not the actual length.
    rStream.WriteUInt8(0x80 | (nLengthOfLength - 1));
    for (size_t i = 1; i < nLengthOfLength; ++i)
        rStream.WriteUInt8(nLength >> ((nLengthOfLength - i - 1) * 8));
}

const unsigned nASN1_INTEGER = 0x02;
const unsigned nASN1_OCTET_STRING = 0x04;
const unsigned nASN1_NULL = 0x05;
const unsigned nASN1_OBJECT_IDENTIFIER = 0x06;
const unsigned nASN1_SEQUENCE = 0x10;
/// An explicit tag on a constructed value.
const unsigned nASN1_TAGGED_CONSTRUCTED = 0xa0;
const unsigned nASN1_CONSTRUCTED = 0x20;

/// Create payload for the 'signing-certificate' signed attribute.
bool CreateSigningCertificateAttribute(void const * pDerEncoded, int nDerEncoded, PCCERT_CONTEXT pCertContext, SvStream& rEncodedCertificate)
{
    // CryptEncodeObjectEx() does not support encoding arbitrary ASN.1
    // structures, like SigningCertificateV2 from RFC 5035, so let's build it
    // manually.

    // Count the certificate hash and put it to aHash.
    // 2.16.840.1.101.3.4.2.1, i.e. sha256.
    std::vector<unsigned char> aSHA256{0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01};

    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        SAL_WARN("svl.crypto", "CryptAcquireContext() failed");
        return false;
    }

    HCRYPTHASH hHash = 0;
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
    {
        SAL_WARN("svl.crypto", "CryptCreateHash() failed");
        return false;
    }

    if (!CryptHashData(hHash, static_cast<const BYTE*>(pDerEncoded), nDerEncoded, 0))
    {
        SAL_WARN("svl.crypto", "CryptHashData() failed");
        return false;
    }

    DWORD nHash = 0;
    if (!CryptGetHashParam(hHash, HP_HASHVAL, nullptr, &nHash, 0))
    {
        SAL_WARN("svl.crypto", "CryptGetHashParam() failed to provide the hash length");
        return false;
    }

    std::vector<unsigned char> aHash(nHash);
    if (!CryptGetHashParam(hHash, HP_HASHVAL, aHash.data(), &nHash, 0))
    {
        SAL_WARN("svl.crypto", "CryptGetHashParam() failed to provide the hash");
        return false;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    // Collect info for IssuerSerial.
    BYTE* pIssuer = pCertContext->pCertInfo->Issuer.pbData;
    DWORD nIssuer = pCertContext->pCertInfo->Issuer.cbData;
    BYTE* pSerial = pCertContext->pCertInfo->SerialNumber.pbData;
    DWORD nSerial = pCertContext->pCertInfo->SerialNumber.cbData;
    // pSerial is LE, aSerial is BE.
    std::vector<BYTE> aSerial(nSerial);
    for (size_t i = 0; i < nSerial; ++i)
        aSerial[i] = *(pSerial + nSerial - i - 1);

    // We now have all the info to count the lengths.
    // The layout of the payload is:
    // SEQUENCE: SigningCertificateV2
    //     SEQUENCE: SEQUENCE OF ESSCertIDv2
    //         SEQUENCE: ESSCertIDv2
    //             SEQUENCE: AlgorithmIdentifier
    //                 OBJECT: algorithm
    //                 NULL: parameters
    //             OCTET STRING: certHash
    //             SEQUENCE: IssuerSerial
    //                 SEQUENCE: GeneralNames
    //                     cont [ 4 ]: Name
    //                         SEQUENCE: Issuer blob
    //                 INTEGER: CertificateSerialNumber

    size_t nAlgorithm = 1 + GetDERLengthOfLength(aSHA256.size()) + aSHA256.size();
    size_t nParameters = 1 + GetDERLengthOfLength(1);
    size_t nAlgorithmIdentifier = 1 + GetDERLengthOfLength(nAlgorithm + nParameters) + nAlgorithm + nParameters;
    size_t nCertHash = 1 + GetDERLengthOfLength(aHash.size()) + aHash.size();
    size_t nName = 1 + GetDERLengthOfLength(nIssuer) + nIssuer;
    size_t nGeneralNames = 1 + GetDERLengthOfLength(nName) + nName;
    size_t nCertificateSerialNumber = 1 + GetDERLengthOfLength(nSerial) + nSerial;
    size_t nIssuerSerial = 1 + GetDERLengthOfLength(nGeneralNames + nCertificateSerialNumber) + nGeneralNames + nCertificateSerialNumber;
    size_t nESSCertIDv2 = 1 + GetDERLengthOfLength(nAlgorithmIdentifier + nCertHash + nIssuerSerial) + nAlgorithmIdentifier + nCertHash + nIssuerSerial;
    size_t nESSCertIDv2s = 1 + GetDERLengthOfLength(nESSCertIDv2) + nESSCertIDv2;

    // Write SigningCertificateV2.
    rEncodedCertificate.WriteUInt8(nASN1_SEQUENCE | nASN1_CONSTRUCTED);
    WriteDERLength(rEncodedCertificate, nESSCertIDv2s);
    // Write SEQUENCE OF ESSCertIDv2.
    rEncodedCertificate.WriteUInt8(nASN1_SEQUENCE | nASN1_CONSTRUCTED);
    WriteDERLength(rEncodedCertificate, nESSCertIDv2);
    // Write ESSCertIDv2.
    rEncodedCertificate.WriteUInt8(nASN1_SEQUENCE | nASN1_CONSTRUCTED);
    WriteDERLength(rEncodedCertificate, nAlgorithmIdentifier + nCertHash + nIssuerSerial);
    // Write AlgorithmIdentifier.
    rEncodedCertificate.WriteUInt8(nASN1_SEQUENCE | nASN1_CONSTRUCTED);
    WriteDERLength(rEncodedCertificate, nAlgorithm + nParameters);
    // Write algorithm.
    rEncodedCertificate.WriteUInt8(nASN1_OBJECT_IDENTIFIER);
    WriteDERLength(rEncodedCertificate, aSHA256.size());
    rEncodedCertificate.WriteBytes(aSHA256.data(), aSHA256.size());
    // Write parameters.
    rEncodedCertificate.WriteUInt8(nASN1_NULL);
    rEncodedCertificate.WriteUInt8(0);
    // Write certHash.
    rEncodedCertificate.WriteUInt8(nASN1_OCTET_STRING);
    WriteDERLength(rEncodedCertificate, aHash.size());
    rEncodedCertificate.WriteBytes(aHash.data(), aHash.size());
    // Write IssuerSerial.
    rEncodedCertificate.WriteUInt8(nASN1_SEQUENCE | nASN1_CONSTRUCTED);
    WriteDERLength(rEncodedCertificate, nGeneralNames + nCertificateSerialNumber);
    // Write GeneralNames.
    rEncodedCertificate.WriteUInt8(nASN1_SEQUENCE | nASN1_CONSTRUCTED);
    WriteDERLength(rEncodedCertificate, nName);
    // Write Name.
    rEncodedCertificate.WriteUInt8(nASN1_TAGGED_CONSTRUCTED | 4);
    WriteDERLength(rEncodedCertificate, nIssuer);
    rEncodedCertificate.WriteBytes(pIssuer, nIssuer);
    // Write CertificateSerialNumber.
    rEncodedCertificate.WriteUInt8(nASN1_INTEGER);
    WriteDERLength(rEncodedCertificate, nSerial);
    rEncodedCertificate.WriteBytes(aSerial.data(), aSerial.size());

    return true;
}
#endif // USE_CRYPTO_MSCAPI

} // anonymous namespace

namespace svl::crypto {

std::vector<unsigned char> DecodeHexString(std::string_view rHex)
{
    std::vector<unsigned char> aRet;
    size_t nHexLen = rHex.size();
    {
        int nByte = 0;
        int nCount = 2;
        for (size_t i = 0; i < nHexLen; ++i)
        {
            nByte = nByte << 4;
            sal_Int8 nParsed = o3tl::convertToHex<int>(rHex[i]);
            if (nParsed == -1)
            {
                SAL_WARN("svl.crypto", "DecodeHexString: invalid hex value");
                return aRet;
            }
            nByte += nParsed;
            --nCount;
            if (!nCount)
            {
                aRet.push_back(nByte);
                nCount = 2;
                nByte = 0;
            }
        }
    }

    return aRet;
}

bool Signing::Sign(OStringBuffer& rCMSHexBuffer)
{
#if !USE_CRYPTO_ANY
    (void)rCMSHexBuffer;
    (void)m_rSigningContext;
    return false;
#else
    // Create the PKCS#7 object.
    css::uno::Sequence<sal_Int8> aDerEncoded;
    if (m_rSigningContext.m_xCertificate.is())
    {
        aDerEncoded = m_rSigningContext.m_xCertificate->getEncoded();
        if (!aDerEncoded.hasElements())
        {
            SAL_WARN("svl.crypto", "Crypto::Signing: empty certificate");
            return false;
        }
    }

#if USE_CRYPTO_NSS
    std::vector<unsigned char> aHashResult;
    {
        comphelper::Hash aHash(comphelper::HashType::SHA256);

        for (const auto& pair : m_dataBlocks)
            aHash.update(pair.first, pair.second);

        aHashResult = aHash.finalize();
    }
    SECItem digest;
    digest.data = aHashResult.data();
    digest.len = aHashResult.size();

    PRTime now = PR_Now();

    // The context unit is milliseconds, PR_Now() unit is microseconds.
    if (m_rSigningContext.m_nSignatureTime)
    {
        now = m_rSigningContext.m_nSignatureTime * 1000;
    }
    else
    {
        m_rSigningContext.m_nSignatureTime = now / 1000;
    }

    if (!m_rSigningContext.m_xCertificate.is())
    {
        m_rSigningContext.m_aDigest = std::move(aHashResult);
        // No certificate is provided: don't actually sign -- just update the context with the
        // parameters for the signing and return.
        return false;
    }

    CERTCertificate *cert = CERT_DecodeCertFromPackage(reinterpret_cast<char *>(aDerEncoded.getArray()), aDerEncoded.getLength());

    if (!cert)
    {
        SAL_WARN("svl.crypto", "CERT_DecodeCertFromPackage failed");
        return false;
    }

    NSSCMSSignedData *cms_sd(nullptr);
    NSSCMSSignerInfo *cms_signer(nullptr);
    NSSCMSMessage *cms_msg = CreateCMSMessage(nullptr, &cms_sd, &cms_signer, cert, &digest);
    if (!cms_msg)
        return false;

    OString pass(OUStringToOString( m_aSignPassword, RTL_TEXTENCODING_UTF8 ));

    // Add the signing certificate as a signed attribute.
    ESSCertIDv2* aCertIDs[2];
    ESSCertIDv2 aCertID;
    // Write ESSCertIDv2.hashAlgorithm.
    aCertID.hashAlgorithm.algorithm.data = nullptr;
    aCertID.hashAlgorithm.parameters.data = nullptr;
    SECOID_SetAlgorithmID(nullptr, &aCertID.hashAlgorithm, SEC_OID_SHA256, nullptr);
    comphelper::ScopeGuard aAlgoGuard(
        [&aCertID] () { SECOID_DestroyAlgorithmID(&aCertID.hashAlgorithm, false); } );
    // Write ESSCertIDv2.certHash.
    SECItem aCertHashItem;
    auto pDerEncoded = reinterpret_cast<const unsigned char *>(aDerEncoded.getArray());
    std::vector<unsigned char> aCertHashResult = comphelper::Hash::calculateHash(pDerEncoded, aDerEncoded.getLength(), comphelper::HashType::SHA256);
    aCertHashItem.type = siBuffer;
    aCertHashItem.data = aCertHashResult.data();
    aCertHashItem.len = aCertHashResult.size();
    aCertID.certHash = aCertHashItem;
    // Write ESSCertIDv2.issuerSerial.
    IssuerSerial aSerial;
    GeneralName aName;
    aName.name = cert->issuer;
    aSerial.issuer.names = aName;
    aSerial.serialNumber = cert->serialNumber;
    aCertID.issuerSerial = aSerial;
    // Write SigningCertificateV2.certs.
    aCertIDs[0] = &aCertID;
    aCertIDs[1] = nullptr;
    SigningCertificateV2 aCertificate;
    aCertificate.certs = &aCertIDs[0];
    SECItem* pEncodedCertificate = SEC_ASN1EncodeItem(nullptr, nullptr, &aCertificate, SigningCertificateV2Template);
    if (!pEncodedCertificate)
    {
        SAL_WARN("svl.crypto", "SEC_ASN1EncodeItem() failed");
        return false;
    }

    NSSCMSAttribute aAttribute;
    SECItem aAttributeValues[2];
    SECItem* pAttributeValues[2];
    pAttributeValues[0] = aAttributeValues;
    pAttributeValues[1] = nullptr;
    aAttributeValues[0] = *pEncodedCertificate;
    aAttributeValues[1].type = siBuffer;
    aAttributeValues[1].data = nullptr;
    aAttributeValues[1].len = 0;
    aAttribute.values = pAttributeValues;

    SECOidData aOidData;
    auto cert_oid_buffer = std::to_array(OID_SIGNINGCERTIFICATEV2);
    aOidData.oid.data = cert_oid_buffer.data();
    aOidData.oid.len = cert_oid_buffer.size();
    /*
     * id-aa-signingCertificateV2 OBJECT IDENTIFIER ::=
     * { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) pkcs9(9)
     *   smime(16) id-aa(2) 47 }
     */
    aOidData.offset = SEC_OID_UNKNOWN;
    aOidData.desc = "id-aa-signingCertificateV2";
    aOidData.mechanism = CKM_SHA_1;
    aOidData.supportedExtension = UNSUPPORTED_CERT_EXTENSION;
    aAttribute.typeTag = &aOidData;
    aAttribute.type = aOidData.oid;
    aAttribute.encoded = PR_TRUE;

    if (my_NSS_CMSSignerInfo_AddAuthAttr(cms_signer, &aAttribute) != SECSuccess)
    {
        SAL_WARN("svl.crypto", "my_NSS_CMSSignerInfo_AddAuthAttr() failed");
        return false;
    }

    SECItem cms_output;
    cms_output.data = nullptr;
    cms_output.len = 0;
    PLArenaPool *arena = PORT_NewArena(10000);
    const ::comphelper::ScopeGuard aScopeGuard(
        [&arena]() mutable { PORT_FreeArena(arena, true); } );
    NSSCMSEncoderContext *cms_ecx;

    // Possibly it would work to even just pass NULL for the password callback function and its
    // argument here. After all, at least with the hardware token and associated software I tested
    // with, the software itself pops up a dialog asking for the PIN (password). But I am not going
    // to test it and risk locking up my token...

    cms_ecx = NSS_CMSEncoder_Start(cms_msg, nullptr, nullptr, &cms_output, arena, PDFSigningPKCS7PasswordCallback,
                                   const_cast<char*>(pass.getStr()), nullptr, nullptr, nullptr, nullptr);

    if (!cms_ecx)
    {
        SAL_WARN("svl.crypto", "NSS_CMSEncoder_Start failed");
        return false;
    }

    if (NSS_CMSEncoder_Finish(cms_ecx) != SECSuccess)
    {
        SAL_WARN("svl.crypto", "NSS_CMSEncoder_Finish failed");
        return false;
    }

    if( !m_aSignTSA.isEmpty() )
    {
        TimeStampReq src;
        OStringBuffer response_buffer;
        TimeStampResp response;
        SECItem response_item;
        cms_recode_attribute timestamp;
        SECItem values[2];
        SECItem *valuesp[2];
        valuesp[0] = values;
        valuesp[1] = nullptr;

        std::vector<unsigned char> aTsHashResult = comphelper::Hash::calculateHash(cms_signer->encDigest.data, cms_signer->encDigest.len, comphelper::HashType::SHA256);
        SECItem ts_digest;
        ts_digest.type = siBuffer;
        ts_digest.data = aTsHashResult.data();
        ts_digest.len = aTsHashResult.size();

        unsigned char cOne = 1;
        unsigned char cTRUE = 0xff; // under DER rules true is 0xff, false is 0x00
        src.version.type = siUnsignedInteger;
        src.version.data = &cOne;
        src.version.len = sizeof(cOne);

        src.messageImprint.hashAlgorithm.algorithm.data = nullptr;
        src.messageImprint.hashAlgorithm.parameters.data = nullptr;
        SECOID_SetAlgorithmID(nullptr, &src.messageImprint.hashAlgorithm, SEC_OID_SHA256, nullptr);
        src.messageImprint.hashedMessage = ts_digest;

        src.reqPolicy.type = siBuffer;
        src.reqPolicy.data = nullptr;
        src.reqPolicy.len = 0;

        unsigned int nNonce = comphelper::rng::uniform_uint_distribution(0, SAL_MAX_UINT32);
        src.nonce.type = siUnsignedInteger;
        src.nonce.data = reinterpret_cast<unsigned char*>(&nNonce);
        src.nonce.len = sizeof(nNonce);

        src.certReq.type = siUnsignedInteger;
        src.certReq.data = &cTRUE;
        src.certReq.len = sizeof(cTRUE);

        src.extensions = nullptr;

        SECItem* timestamp_request = SEC_ASN1EncodeItem(nullptr, nullptr, &src, TimeStampReq_Template);
        if (timestamp_request == nullptr)
        {
            SAL_WARN("svl.crypto", "SEC_ASN1EncodeItem failed");
            return false;
        }

        if (timestamp_request->data == nullptr)
        {
            SAL_WARN("svl.crypto", "SEC_ASN1EncodeItem succeeded but got NULL data");
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        SAL_INFO("svl.crypto", "request length=" << timestamp_request->len);

        // Send time stamp request to TSA server, receive response

        CURL* curl = curl_easy_init();
        CURLcode rc;
        struct curl_slist* slist = nullptr;

        if (!curl)
        {
            SAL_WARN("svl.crypto", "curl_easy_init failed");
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        ::InitCurl_easy(curl);

        SAL_INFO("svl.crypto", "Setting curl to verbose: " << (curl_easy_setopt(curl, CURLOPT_VERBOSE, 1) == CURLE_OK ? "OK" : "FAIL"));

        if ((rc = curl_easy_setopt(curl, CURLOPT_URL, OUStringToOString(m_aSignTSA, RTL_TEXTENCODING_UTF8).getStr())) != CURLE_OK)
        {
            SAL_WARN("svl.crypto", "curl_easy_setopt(CURLOPT_URL) failed: " << curl_easy_strerror(rc));
            curl_easy_cleanup(curl);
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        slist = curl_slist_append(slist, "Content-Type: application/timestamp-query");
        slist = curl_slist_append(slist, "Accept: application/timestamp-reply");

        if ((rc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist)) != CURLE_OK)
        {
            SAL_WARN("svl.crypto", "curl_easy_setopt(CURLOPT_HTTPHEADER) failed: " << curl_easy_strerror(rc));
            curl_slist_free_all(slist);
            curl_easy_cleanup(curl);
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        if ((rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<tools::Long>(timestamp_request->len))) != CURLE_OK ||
            (rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, timestamp_request->data)) != CURLE_OK)
        {
            SAL_WARN("svl.crypto", "curl_easy_setopt(CURLOPT_POSTFIELDSIZE or CURLOPT_POSTFIELDS) failed: " << curl_easy_strerror(rc));
            curl_easy_cleanup(curl);
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        if ((rc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer)) != CURLE_OK ||
            (rc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AppendToBuffer)) != CURLE_OK)
        {
            SAL_WARN("svl.crypto", "curl_easy_setopt(CURLOPT_WRITEDATA or CURLOPT_WRITEFUNCTION) failed: " << curl_easy_strerror(rc));
            curl_easy_cleanup(curl);
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        if ((rc = curl_easy_setopt(curl, CURLOPT_POST, 1)) != CURLE_OK)
        {
            SAL_WARN("svl.crypto", "curl_easy_setopt(CURLOPT_POST) failed: " << curl_easy_strerror(rc));
            curl_easy_cleanup(curl);
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        char error_buffer[CURL_ERROR_SIZE];
        if ((rc = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer)) != CURLE_OK)
        {
            SAL_WARN("svl.crypto", "curl_easy_setopt(CURLOPT_ERRORBUFFER) failed: " << curl_easy_strerror(rc));
            curl_easy_cleanup(curl);
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        // Use a ten second timeout
        if ((rc = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10)) != CURLE_OK ||
            (rc = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10)) != CURLE_OK)
        {
            SAL_WARN("svl.crypto", "curl_easy_setopt(CURLOPT_TIMEOUT or CURLOPT_CONNECTTIMEOUT) failed: " << curl_easy_strerror(rc));
            curl_easy_cleanup(curl);
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        if (curl_easy_perform(curl) != CURLE_OK)
        {
            SAL_WARN("svl.crypto", "curl_easy_perform failed: " << error_buffer);
            curl_easy_cleanup(curl);
            SECITEM_FreeItem(timestamp_request, PR_TRUE);
            return false;
        }

        SAL_INFO("svl.crypto", "PDF signing: got response, length=" << response_buffer.getLength());

        curl_slist_free_all(slist);
        curl_easy_cleanup(curl);
        SECITEM_FreeItem(timestamp_request, PR_TRUE);

        memset(&response, 0, sizeof(response));

        response_item.type = siBuffer;
        response_item.data = reinterpret_cast<unsigned char*>(const_cast<char*>(response_buffer.getStr()));
        response_item.len = response_buffer.getLength();

        if (SEC_ASN1DecodeItem(nullptr, &response, TimeStampResp_Template, &response_item) != SECSuccess)
        {
            SAL_WARN("svl.crypto", "SEC_ASN1DecodeItem failed");
            return false;
        }

        SAL_INFO("svl.crypto", "TimeStampResp received and decoded, status=" << PKIStatusInfoToString(response.status));

        if (response.status.status.len != 1 ||
            (response.status.status.data[0] != 0 && response.status.status.data[0] != 1))
        {
            SAL_WARN("svl.crypto", "Timestamp request was not granted");
            return false;
        }

        // timestamp.type filled in below

        // Not sure if we actually need two entries in the values array, now when valuesp is an
        // array too, the pointer to the values array followed by a null pointer. But I don't feel
        // like experimenting.
        values[0] = response.timeStampToken;
        values[1].type = siBuffer;
        values[1].data = nullptr;
        values[1].len = 0;

        timestamp.values = valuesp;

        auto ts_oid_buffer = std::to_array(OID_TIMESTAMPTOKEN);
        // id-aa-timeStampToken OBJECT IDENTIFIER ::= { iso(1)
        // member-body(2) us(840) rsadsi(113549) pkcs(1) pkcs-9(9)
        // smime(16) aa(2) 14 }

        timestamp.type.data = ts_oid_buffer.data();
        timestamp.type.len =  ts_oid_buffer.size();

        cms_recode_message decoded_cms_output {};

        if (SEC_ASN1DecodeItem(arena, &decoded_cms_output, recode_message_template, &cms_output) != SECSuccess)
        {
            SAL_WARN("svl.crypto", "SEC_ASN1DecodeItem failed");
            return false;
        }

        // now insert the new attribute

        cms_recode_signer_info **decoded_signerinfos = decoded_cms_output.signedData.signerInfos;
        if (!decoded_signerinfos || !*decoded_signerinfos) {
            SAL_WARN("svl.crypto", "Decoded signed message invalid");
            return false;
        }

        std::vector<cms_recode_attribute *> updated_attrs;

        // there are no unauthenticated attributes at the moment
        // if this ever changes, make sure to preserve them
        cms_recode_attribute **existing_attrs = (*decoded_signerinfos)->unAuthAttr ;

        if (existing_attrs)
            while (*existing_attrs)
                updated_attrs.push_back(*existing_attrs++);

        updated_attrs.push_back(&timestamp);
        updated_attrs.push_back(nullptr);

        (*decoded_signerinfos)->unAuthAttr = updated_attrs.data();

        SECItem * ts_cms_output = SEC_ASN1EncodeItem(arena, nullptr, &decoded_cms_output, recode_message_template);
        if (!ts_cms_output)
        {
            SAL_WARN("svl.crypto", "SEC_ASN1EncodeItem failed");
            return false;
        }

        cms_output = *ts_cms_output;
    }

    if (cms_output.len*2 > MAX_SIGNATURE_CONTENT_LENGTH)
    {
        SAL_WARN("svl.crypto", "Signature requires more space (" << cms_output.len*2 << ") than we reserved (" << MAX_SIGNATURE_CONTENT_LENGTH << ")");
        NSS_CMSMessage_Destroy(cms_msg);
        return false;
    }

    for (unsigned int i = 0; i < cms_output.len ; i++)
        appendHex(cms_output.data[i], rCMSHexBuffer);

    SECITEM_FreeItem(pEncodedCertificate, PR_TRUE);
    NSS_CMSMessage_Destroy(cms_msg);

    return true;

#elif USE_CRYPTO_MSCAPI // ends USE_CRYPTO_NSS

    PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, reinterpret_cast<const BYTE*>(aDerEncoded.getArray()), aDerEncoded.getLength());
    if (pCertContext == nullptr)
    {
        SAL_WARN("svl.crypto", "CertCreateCertificateContext failed: " << comphelper::WindowsErrorString(GetLastError()));
        return false;
    }

    CRYPT_SIGN_MESSAGE_PARA aPara = {};
    aPara.cbSize = sizeof(aPara);
    aPara.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    aPara.pSigningCert = pCertContext;
    aPara.HashAlgorithm.pszObjId = const_cast<LPSTR>(szOID_NIST_sha256);
    aPara.HashAlgorithm.Parameters.cbData = 0;
    aPara.cMsgCert = 1;
    aPara.rgpMsgCert = &pCertContext;

    NCRYPT_KEY_HANDLE hCryptKey = 0;
    DWORD dwFlags = CRYPT_ACQUIRE_CACHE_FLAG | CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG;
    HCRYPTPROV_OR_NCRYPT_KEY_HANDLE* phCryptProvOrNCryptKey = &hCryptKey;
    DWORD nKeySpec;
    BOOL bFreeNeeded;

    if (!CryptAcquireCertificatePrivateKey(pCertContext,
                                           dwFlags,
                                           nullptr,
                                           phCryptProvOrNCryptKey,
                                           &nKeySpec,
                                           &bFreeNeeded))
    {
        SAL_WARN("svl.crypto", "CryptAcquireCertificatePrivateKey failed: " << comphelper::WindowsErrorString(GetLastError()));
        CertFreeCertificateContext(pCertContext);
        return false;
    }
    assert(!bFreeNeeded);

    CMSG_SIGNER_ENCODE_INFO aSignerInfo = {};
    aSignerInfo.cbSize = sizeof(aSignerInfo);
    aSignerInfo.pCertInfo = pCertContext->pCertInfo;
    aSignerInfo.hNCryptKey = hCryptKey;
    aSignerInfo.dwKeySpec = nKeySpec;
    aSignerInfo.HashAlgorithm.pszObjId = const_cast<LPSTR>(szOID_NIST_sha256);
    aSignerInfo.HashAlgorithm.Parameters.cbData = 0;

    // Add the signing certificate as a signed attribute.
    CRYPT_INTEGER_BLOB aCertificateBlob;
    SvMemoryStream aEncodedCertificate;
    if (!CreateSigningCertificateAttribute(aDerEncoded.getArray(), aDerEncoded.getLength(), pCertContext, aEncodedCertificate))
    {
        SAL_WARN("svl.crypto", "CreateSigningCertificateAttribute() failed");
        return false;
    }
    aCertificateBlob.pbData = const_cast<BYTE*>(static_cast<const BYTE*>(aEncodedCertificate.GetData()));
    aCertificateBlob.cbData = aEncodedCertificate.GetSize();
    CRYPT_ATTRIBUTE aCertificateAttribute;
    /*
     * id-aa-signingCertificateV2 OBJECT IDENTIFIER ::=
     * { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) pkcs9(9)
     *   smime(16) id-aa(2) 47 }
     */
    aCertificateAttribute.pszObjId = const_cast<LPSTR>("1.2.840.113549.1.9.16.2.47");
    aCertificateAttribute.cValue = 1;
    aCertificateAttribute.rgValue = &aCertificateBlob;
    aSignerInfo.cAuthAttr = 1;
    aSignerInfo.rgAuthAttr = &aCertificateAttribute;

    CMSG_SIGNED_ENCODE_INFO aSignedInfo = {};
    aSignedInfo.cbSize = sizeof(aSignedInfo);
    aSignedInfo.cSigners = 1;
    aSignedInfo.rgSigners = &aSignerInfo;

    CERT_BLOB aCertBlob;

    aCertBlob.cbData = pCertContext->cbCertEncoded;
    aCertBlob.pbData = pCertContext->pbCertEncoded;

    aSignedInfo.cCertEncoded = 1;
    aSignedInfo.rgCertEncoded = &aCertBlob;

    HCRYPTMSG hMsg = CryptMsgOpenToEncode(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                          CMSG_DETACHED_FLAG,
                                          CMSG_SIGNED,
                                          &aSignedInfo,
                                          nullptr,
                                          nullptr);
    if (!hMsg)
    {
        SAL_WARN("svl.crypto", "CryptMsgOpenToEncode failed: " << comphelper::WindowsErrorString(GetLastError()));
        CertFreeCertificateContext(pCertContext);
        return false;
    }

    for (size_t i = 0; i < m_dataBlocks.size(); ++i)
    {
        const bool last = (i == m_dataBlocks.size() - 1);
        if (!CryptMsgUpdate(hMsg, static_cast<const BYTE *>(m_dataBlocks[i].first), m_dataBlocks[i].second, last))
        {
            SAL_WARN("svl.crypto", "CryptMsgUpdate failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hMsg);
            CertFreeCertificateContext(pCertContext);
            return false;
        }
    }
    CertFreeCertificateContext(pCertContext);

    PCRYPT_TIMESTAMP_CONTEXT pTsContext = nullptr;
    DWORD dwEncodedMessageParamType = CMSG_CONTENT_PARAM;

    if( !m_aSignTSA.isEmpty() )
    {
        HCRYPTMSG hDecodedMsg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                                     CMSG_DETACHED_FLAG,
                                                     CMSG_SIGNED,
                                                     0,
                                                     nullptr,
                                                     nullptr);
        if (!hDecodedMsg)
        {
            SAL_WARN("svl.crypto", "CryptMsgOpenToDecode failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hMsg);
            return false;
        }

        DWORD nTsSigLen = 0;

        if (!CryptMsgGetParam(hMsg, CMSG_BARE_CONTENT_PARAM, 0, nullptr, &nTsSigLen))
        {
            SAL_WARN("svl.crypto", "CryptMsgGetParam(CMSG_BARE_CONTENT_PARAM) failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            CryptMsgClose(hMsg);
            return false;
        }

        SAL_INFO("svl.crypto", "nTsSigLen=" << nTsSigLen);

        std::unique_ptr<BYTE[]> pTsSig(new BYTE[nTsSigLen]);

        if (!CryptMsgGetParam(hMsg, CMSG_BARE_CONTENT_PARAM, 0, pTsSig.get(), &nTsSigLen))
        {
            SAL_WARN("svl.crypto", "CryptMsgGetParam(CMSG_BARE_CONTENT_PARAM) failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            CryptMsgClose(hMsg);
            return false;
        }

        CryptMsgClose(hMsg);

        if (!CryptMsgUpdate(hDecodedMsg, pTsSig.get(), nTsSigLen, TRUE))
        {
            SAL_WARN("svl.crypto", "CryptMsgUpdate failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            return false;
        }

        DWORD nDecodedSignerInfoLen = 0;
        if (!CryptMsgGetParam(hDecodedMsg, CMSG_SIGNER_INFO_PARAM, 0, nullptr, &nDecodedSignerInfoLen))
        {
            SAL_WARN("svl.crypto", "CryptMsgGetParam(CMSG_SIGNER_INFO_PARAM) failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            return false;
        }

        std::unique_ptr<BYTE[]> pDecodedSignerInfoBuf(new BYTE[nDecodedSignerInfoLen]);

        if (!CryptMsgGetParam(hDecodedMsg, CMSG_SIGNER_INFO_PARAM, 0, pDecodedSignerInfoBuf.get(), &nDecodedSignerInfoLen))
        {
            SAL_WARN("svl.crypto", "CryptMsgGetParam(CMSG_SIGNER_INFO_PARAM) failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            return false;
        }

        CMSG_SIGNER_INFO *pDecodedSignerInfo = reinterpret_cast<CMSG_SIGNER_INFO *>(pDecodedSignerInfoBuf.get());

        CRYPT_TIMESTAMP_PARA aTsPara;
        unsigned int nNonce = comphelper::rng::uniform_uint_distribution(0, SAL_MAX_UINT32);

        aTsPara.pszTSAPolicyId = nullptr;
        aTsPara.fRequestCerts = TRUE;
        aTsPara.Nonce.cbData = sizeof(nNonce);
        aTsPara.Nonce.pbData = reinterpret_cast<BYTE *>(&nNonce);
        aTsPara.cExtension = 0;
        aTsPara.rgExtension = nullptr;

        if (!CryptRetrieveTimeStamp(o3tl::toW(m_aSignTSA.getStr()),
                     0,
                     10000,
                     szOID_NIST_sha256,
                     &aTsPara,
                     pDecodedSignerInfo->EncryptedHash.pbData,
                     pDecodedSignerInfo->EncryptedHash.cbData,
                     &pTsContext,
                     nullptr,
                     nullptr))
        {
            SAL_WARN("svl.crypto", "CryptRetrieveTimeStamp failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            return false;
        }

        SAL_INFO("svl.crypto", "Time stamp size is " << pTsContext->cbEncoded << " bytes");

        CRYPT_INTEGER_BLOB aTimestampBlob;
        aTimestampBlob.cbData = pTsContext->cbEncoded;
        aTimestampBlob.pbData = pTsContext->pbEncoded;

        CRYPT_ATTRIBUTE aTimestampAttribute;
        aTimestampAttribute.pszObjId = const_cast<LPSTR>(
            "1.2.840.113549.1.9.16.2.14");
        aTimestampAttribute.cValue = 1;
        aTimestampAttribute.rgValue = &aTimestampBlob;

        DWORD nEncodedTsAttributeLen = 0;
        if (!CryptEncodeObject(PKCS_7_ASN_ENCODING, PKCS_ATTRIBUTE, &aTimestampAttribute, nullptr, &nEncodedTsAttributeLen))
        {
            SAL_WARN("svl.crypto", "CryptEncodeObject(PKCS_ATTRIBUTE) failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            return false;
        }

        std::unique_ptr<BYTE[]> pEncodedTsAttributeBuf(new BYTE[nEncodedTsAttributeLen]);

        CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR_PARA aAddTsAttrPara;
        aAddTsAttrPara.dwSignerIndex = 0;
        aAddTsAttrPara.cbSize = sizeof(aAddTsAttrPara);

        if (!CryptEncodeObject(PKCS_7_ASN_ENCODING, PKCS_ATTRIBUTE, &aTimestampAttribute, pEncodedTsAttributeBuf.get(), &nEncodedTsAttributeLen))
        {
            SAL_WARN("svl.crypto", "CryptEncodeObject(PKCS_ATTRIBUTE) failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            return false;
        }

         aAddTsAttrPara.blob.cbData = nEncodedTsAttributeLen;
         aAddTsAttrPara.blob.pbData = pEncodedTsAttributeBuf.get();

        if (!CryptMsgControl(hDecodedMsg, 0, CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR, &aAddTsAttrPara))
        {
            SAL_WARN("svl.crypto", "CryptMsgControl(CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR) failed: " << comphelper::WindowsErrorString(GetLastError()));
            CryptMsgClose(hDecodedMsg);
            return false;
        }
        hMsg = hDecodedMsg;
        dwEncodedMessageParamType = CMSG_ENCODED_MESSAGE;
    }

    DWORD nSigLen = 0;

    if (!CryptMsgGetParam(hMsg, dwEncodedMessageParamType, 0, nullptr, &nSigLen))
    {
        SAL_WARN("svl.crypto", "Reading the encoded message via CryptMsgGetParam() failed: " << comphelper::WindowsErrorString(GetLastError()));
        if (pTsContext)
            CryptMemFree(pTsContext);
        CryptMsgClose(hMsg);
        return false;
    }

    if (nSigLen*2 > MAX_SIGNATURE_CONTENT_LENGTH)
    {
        SAL_WARN("svl.crypto", "Signature requires more space (" << nSigLen*2 << ") than we reserved (" << MAX_SIGNATURE_CONTENT_LENGTH << ")");
        if (pTsContext)
            CryptMemFree(pTsContext);
        CryptMsgClose(hMsg);
        return false;
    }

    SAL_INFO("svl.crypto", "Signature size is " << nSigLen << " bytes");
    std::unique_ptr<BYTE[]> pSig(new BYTE[nSigLen]);

    if (!CryptMsgGetParam(hMsg, dwEncodedMessageParamType, 0, pSig.get(), &nSigLen))
    {
        SAL_WARN("svl.crypto", "Reading the encoded message via CryptMsgGetParam() failed: " << comphelper::WindowsErrorString(GetLastError()));
        if (pTsContext)
            CryptMemFree(pTsContext);
        CryptMsgClose(hMsg);
        return false;
    }

    // Release resources
    if (pTsContext)
        CryptMemFree(pTsContext);
    CryptMsgClose(hMsg);

    for (unsigned int i = 0; i < nSigLen ; i++)
        appendHex(pSig[i], rCMSHexBuffer);

    return true;
#endif // USE_CRYPTO_MSCAPI
#endif // USE_CRYPTO_ANY
}

namespace
{
#if USE_CRYPTO_NSS
/// Similar to NSS_CMSAttributeArray_FindAttrByOidTag(), but works directly with a SECOidData.
NSSCMSAttribute* CMSAttributeArray_FindAttrByOidData(NSSCMSAttribute** attrs, SECOidData const * oid, PRBool only)
{
    NSSCMSAttribute* attr1, *attr2;

    if (attrs == nullptr)
        return nullptr;

    if (oid == nullptr)
        return nullptr;

    while ((attr1 = *attrs++) != nullptr)
    {
        if (attr1->type.len == oid->oid.len && PORT_Memcmp(attr1->type.data,
                oid->oid.data,
                oid->oid.len) == 0)
            break;
    }

    if (attr1 == nullptr)
        return nullptr;

    if (!only)
        return attr1;

    while ((attr2 = *attrs++) != nullptr)
    {
        if (attr2->type.len == oid->oid.len && PORT_Memcmp(attr2->type.data,
                oid->oid.data,
                oid->oid.len) == 0)
            break;
    }

    if (attr2 != nullptr)
        return nullptr;

    return attr1;
}

#elif USE_CRYPTO_MSCAPI // ends USE_CRYPTO_NSS

/// Verifies a non-detached signature using CryptoAPI.
bool VerifyNonDetachedSignature(const std::vector<unsigned char>& aData, const std::vector<BYTE>& rExpectedHash)
{
    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        SAL_WARN("svl.crypto", "CryptAcquireContext() failed");
        return false;
    }

    HCRYPTHASH hHash = 0;
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
    {
        SAL_WARN("svl.crypto", "CryptCreateHash() failed");
        return false;
    }

    if (!CryptHashData(hHash, aData.data(), aData.size(), 0))
    {
        SAL_WARN("svl.crypto", "CryptHashData() failed");
        return false;
    }

    DWORD nActualHash = 0;
    if (!CryptGetHashParam(hHash, HP_HASHVAL, nullptr, &nActualHash, 0))
    {
        SAL_WARN("svl.crypto", "CryptGetHashParam() failed to provide the hash length");
        return false;
    }

    std::vector<unsigned char> aActualHash(nActualHash);
    if (!CryptGetHashParam(hHash, HP_HASHVAL, aActualHash.data(), &nActualHash, 0))
    {
        SAL_WARN("svl.crypto", "CryptGetHashParam() failed to provide the hash");
        return false;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return aActualHash.size() == rExpectedHash.size() &&
           !std::memcmp(aActualHash.data(), rExpectedHash.data(), aActualHash.size());
}

OUString GetSubjectName(PCCERT_CONTEXT pCertContext)
{
    OUString subjectName;

    // Get Subject name size.
    DWORD dwData = CertGetNameStringW(pCertContext,
                                      CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                      0,
                                      nullptr,
                                      nullptr,
                                      0);
    if (!dwData)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: CertGetNameString failed");
        return subjectName;
    }

    // Allocate memory for subject name.
    LPWSTR szName = static_cast<LPWSTR>(
        LocalAlloc(LPTR, dwData * sizeof(WCHAR)));
    if (!szName)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: Unable to allocate memory for subject name");
        return subjectName;
    }

    // Get subject name.
    if (!CertGetNameStringW(pCertContext,
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            0,
                            nullptr,
                            szName,
                            dwData))
    {
        LocalFree(szName);
        SAL_WARN("svl.crypto", "ValidateSignature: CertGetNameString failed");
        return subjectName;
    }

    subjectName = o3tl::toU(szName);
    LocalFree(szName);

    return subjectName;
}
#endif // USE_CRYPTO_MSCAPI

#if USE_CRYPTO_NSS
    void ensureNssInit()
    {
        // e.g. tdf#122599 ensure NSS library is initialized for NSS_CMSMessage_CreateFromDER
        css::uno::Reference<css::xml::crypto::XNSSInitializer>
            xNSSInitializer = css::xml::crypto::NSSInitializer::create(comphelper::getProcessComponentContext());

        // this calls NSS_Init
        xNSSInitializer->getDigestContext(css::xml::crypto::DigestID::SHA256,
                                          uno::Sequence<beans::NamedValue>());
    }
#endif
} // anonymous namespace

bool Signing::Verify(const std::vector<unsigned char>& aData,
                     const bool bNonDetached,
                     const std::vector<unsigned char>& aSignature,
                     SignatureInformation& rInformation)
{
#if USE_CRYPTO_NSS
    // ensure NSS_Init() is called before using NSS_CMSMessage_CreateFromDER
    static std::once_flag aInitOnce;
    std::call_once(aInitOnce, ensureNssInit);

    // Validate the signature.
    SECItem aSignatureItem;
    aSignatureItem.data = const_cast<unsigned char*>(aSignature.data());
    aSignatureItem.len = aSignature.size();
    NSSCMSMessage* pCMSMessage = NSS_CMSMessage_CreateFromDER(&aSignatureItem,
                                 /*cb=*/nullptr,
                                 /*cb_arg=*/nullptr,
                                 /*pwfn=*/nullptr,
                                 /*pwfn_arg=*/nullptr,
                                 /*decrypt_key_cb=*/nullptr,
                                 /*decrypt_key_cb_arg=*/nullptr);
    if (!NSS_CMSMessage_IsSigned(pCMSMessage))
    {
        SAL_WARN("svl.crypto", "ValidateSignature: message is not signed");
        return false;
    }

    NSSCMSContentInfo* pCMSContentInfo = NSS_CMSMessage_ContentLevel(pCMSMessage, 0);
    if (!pCMSContentInfo)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: NSS_CMSMessage_ContentLevel() failed");
        return false;
    }

    auto pCMSSignedData = static_cast<NSSCMSSignedData*>(NSS_CMSContentInfo_GetContent(pCMSContentInfo));
    if (!pCMSSignedData)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: NSS_CMSContentInfo_GetContent() failed");
        return false;
    }

    // Import certificates from the signed data temporarily, so it'll be
    // possible to verify the signature, even if we didn't have the certificate
    // previously.
    std::vector<CERTCertificate*> aDocumentCertificates;
    if (auto aCerts = pCMSSignedData->rawCerts) {
        while (*aCerts)
            aDocumentCertificates.push_back(CERT_NewTempCertificate(CERT_GetDefaultCertDB(), *aCerts++, nullptr, 0, 0));
    }

    NSSCMSSignerInfo* pCMSSignerInfo = NSS_CMSSignedData_GetSignerInfo(pCMSSignedData, 0);
    if (!pCMSSignerInfo)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: NSS_CMSSignedData_GetSignerInfo() failed");
        return false;
    }

    auto aDigestAlgs = NSS_CMSSignedData_GetDigestAlgs(pCMSSignedData);
    if (!aDigestAlgs || !*aDigestAlgs) {
        SAL_WARN("svl.crypto", "ValidateSignature: digestAlgorithms missing");
        return false;
    }

    SECItem aAlgorithm = aDigestAlgs[0]->algorithm;
    SECOidTag eOidTag = SECOID_FindOIDTag(&aAlgorithm);

    // Map a sign algorithm to a digest algorithm.
    // See NSS_CMSUtil_MapSignAlgs(), which is private to us.
    switch (eOidTag)
    {
    case SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION:
        eOidTag = SEC_OID_SHA1;
        break;
    case SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION:
        eOidTag = SEC_OID_SHA256;
        break;
    case SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION:
        eOidTag = SEC_OID_SHA512;
        break;
    default:
        break;
    }

    HASH_HashType eHashType = HASH_GetHashTypeByOidTag(eOidTag);
    HASHContext* pHASHContext = HASH_Create(bNonDetached ? HASH_AlgSHA1 : eHashType);
    if (!pHASHContext)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: HASH_Create() failed");
        return false;
    }

    // We have a hash, update it with the byte ranges.
    HASH_Update(pHASHContext, aData.data(), aData.size());

    // Find out what is the expected length of the hash.
    unsigned int nMaxResultLen = 0;
    switch (eOidTag)
    {
    case SEC_OID_SHA1:
        nMaxResultLen = comphelper::SHA1_HASH_LENGTH;
        rInformation.nDigestID = xml::crypto::DigestID::SHA1;
        break;
    case SEC_OID_SHA256:
        nMaxResultLen = comphelper::SHA256_HASH_LENGTH;
        rInformation.nDigestID = xml::crypto::DigestID::SHA256;
        break;
    case SEC_OID_SHA512:
        nMaxResultLen = comphelper::SHA512_HASH_LENGTH;
        rInformation.nDigestID = xml::crypto::DigestID::SHA512;
        break;
    default:
        SAL_WARN("svl.crypto", "ValidateSignature: unrecognized algorithm");
        return false;
    }

    auto pActualResultBuffer = static_cast<unsigned char*>(PORT_Alloc(nMaxResultLen));
    unsigned int nActualResultLen;
    HASH_End(pHASHContext, pActualResultBuffer, &nActualResultLen, nMaxResultLen);

    CERTCertificate* pCertificate = NSS_CMSSignerInfo_GetSigningCertificate(pCMSSignerInfo, CERT_GetDefaultCertDB());
    if (!pCertificate)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: NSS_CMSSignerInfo_GetSigningCertificate() failed");
        return false;
    }
    else
    {
        uno::Sequence<sal_Int8> aDerCert(pCertificate->derCert.len);
        auto aDerCertRange = asNonConstRange(aDerCert);
        for (size_t i = 0; i < pCertificate->derCert.len; ++i)
            aDerCertRange[i] = pCertificate->derCert.data[i];
        OUStringBuffer aBuffer;
        comphelper::Base64::encode(aBuffer, aDerCert);
        SignatureInformation::X509Data temp;
        temp.emplace_back();
        temp.back().X509Certificate = aBuffer.makeStringAndClear();
        temp.back().X509Subject = OUString(pCertificate->subjectName, PL_strlen(pCertificate->subjectName), RTL_TEXTENCODING_UTF8);
        rInformation.X509Datas.clear();
        rInformation.X509Datas.emplace_back(temp);
    }

    PRTime nSigningTime;
    // This may fail, in which case the date should be taken from the PDF's dictionary's "M" key,
    // so not critical for PDF at least.
    if (NSS_CMSSignerInfo_GetSigningTime(pCMSSignerInfo, &nSigningTime) == SECSuccess)
    {
        // First convert the UTC UNIX timestamp to a tools::DateTime.
        // nSigningTime is in microseconds.
        DateTime aDateTime = DateTime::CreateFromUnixTime(static_cast<double>(nSigningTime) / 1000000);

        // Then convert to a local UNO DateTime.
        aDateTime.ConvertToLocalTime();
        rInformation.stDateTime = aDateTime.GetUNODateTime();
        if (rInformation.ouDateTime.isEmpty())
        {
            OUStringBuffer rBuffer;
            rBuffer.append(static_cast<sal_Int32>(aDateTime.GetYear()));
            rBuffer.append('-');
            if (aDateTime.GetMonth() < 10)
                rBuffer.append('0');
            rBuffer.append(static_cast<sal_Int32>(aDateTime.GetMonth()));
            rBuffer.append('-');
            if (aDateTime.GetDay() < 10)
                rBuffer.append('0');
            rBuffer.append(static_cast<sal_Int32>(aDateTime.GetDay()));
            rInformation.ouDateTime = rBuffer.makeStringAndClear();
        }
    }

    // Check if we have a signing certificate attribute.
    SECOidData aOidData;
    auto cert_oid_buffer = std::to_array(OID_SIGNINGCERTIFICATEV2);
    aOidData.oid.data = cert_oid_buffer.data();
    aOidData.oid.len = cert_oid_buffer.size();
    /*
     * id-aa-signingCertificateV2 OBJECT IDENTIFIER ::=
     * { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) pkcs9(9)
     *   smime(16) id-aa(2) 47 }
     */
    aOidData.offset = SEC_OID_UNKNOWN;
    aOidData.desc = "id-aa-signingCertificateV2";
    aOidData.mechanism = CKM_SHA_1;
    aOidData.supportedExtension = UNSUPPORTED_CERT_EXTENSION;
    NSSCMSAttribute* pAttribute = CMSAttributeArray_FindAttrByOidData(pCMSSignerInfo->authAttr, &aOidData, PR_TRUE);
    if (pAttribute)
        rInformation.bHasSigningCertificate = true;

    SECItem aSignedDigestItem {siBuffer, nullptr, 0};

    SECItem* pContentInfoContentData = pCMSSignedData->contentInfo.content.data;
    if (pContentInfoContentData && pContentInfoContentData->data)
    {
        // Not a detached signature.
        if (bNonDetached && nActualResultLen == pContentInfoContentData->len &&
            !std::memcmp(pActualResultBuffer, pContentInfoContentData->data, nActualResultLen) &&
            HASH_HashBuf(eHashType, pActualResultBuffer, pContentInfoContentData->data, nActualResultLen) == SECSuccess)
        {
            aSignedDigestItem.data = pActualResultBuffer;
            aSignedDigestItem.len = nMaxResultLen;
        }
    }
    else if (!bNonDetached)
    {
        // Detached, the usual case.
        aSignedDigestItem.data = pActualResultBuffer;
        aSignedDigestItem.len = nActualResultLen;
    }

    if (aSignedDigestItem.data && NSS_CMSSignerInfo_Verify(pCMSSignerInfo, &aSignedDigestItem, nullptr) == SECSuccess)
            rInformation.nStatus = xml::crypto::SecurityOperationStatus_OPERATION_SUCCEEDED;

    // Everything went fine
    PORT_Free(pActualResultBuffer);
    HASH_Destroy(pHASHContext);
    NSS_CMSSignerInfo_Destroy(pCMSSignerInfo);
    for (auto pDocumentCertificate : aDocumentCertificates)
        CERT_DestroyCertificate(pDocumentCertificate);

    return true;

#elif USE_CRYPTO_MSCAPI // ends USE_CRYPTO_NSS

    // Open a message for decoding.
    HCRYPTMSG hMsg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                          CMSG_DETACHED_FLAG,
                                          0,
                                          0,
                                          nullptr,
                                          nullptr);
    if (!hMsg)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgOpenToDecode() failed");
        return false;
    }

    // Update the message with the encoded header blob.
    if (!CryptMsgUpdate(hMsg, aSignature.data(), aSignature.size(), TRUE))
    {
        SAL_WARN("svl.crypto", "ValidateSignature, CryptMsgUpdate() for the header failed: " << comphelper::WindowsErrorString(GetLastError()));
        return false;
    }

    if (!bNonDetached)
    {
        // Update the message with the content blob.
        if (!CryptMsgUpdate(hMsg, aData.data(), aData.size(), FALSE))
        {
            SAL_WARN("svl.crypto", "ValidateSignature, CryptMsgUpdate() for the content failed: " << comphelper::WindowsErrorString(GetLastError()));
            return false;
        }

        if (!CryptMsgUpdate(hMsg, nullptr, 0, TRUE))
        {
            SAL_WARN("svl.crypto", "ValidateSignature, CryptMsgUpdate() for the last content failed: " << comphelper::WindowsErrorString(GetLastError()));
            return false;
        }
    }
    // Get the CRYPT_ALGORITHM_IDENTIFIER from the message.
    DWORD nDigestID = 0;
    if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_HASH_ALGORITHM_PARAM, 0, nullptr, &nDigestID))
    {
        SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgGetParam() failed: " << comphelper::WindowsErrorString(GetLastError()));
        return false;
    }
    std::unique_ptr<BYTE[]> pDigestBytes(new BYTE[nDigestID]);
    if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_HASH_ALGORITHM_PARAM, 0, pDigestBytes.get(), &nDigestID))
    {
        SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgGetParam() failed: " << comphelper::WindowsErrorString(GetLastError()));
        return false;
    }
    auto pDigestID = reinterpret_cast<CRYPT_ALGORITHM_IDENTIFIER*>(pDigestBytes.get());
    if (std::string_view(szOID_NIST_sha256) == pDigestID->pszObjId)
        rInformation.nDigestID = xml::crypto::DigestID::SHA256;
    else if (std::string_view(szOID_RSA_SHA1RSA) == pDigestID->pszObjId || std::string_view(szOID_OIWSEC_sha1) == pDigestID->pszObjId)
        rInformation.nDigestID = xml::crypto::DigestID::SHA1;
    else
        // Don't error out here, we can still verify the message digest correctly, just the digest ID won't be set.
        SAL_WARN("svl.crypto", "ValidateSignature: unhandled algorithm identifier '"<<pDigestID->pszObjId<<"'");

    // Get the signer CERT_INFO from the message.
    DWORD nSignerCertInfo = 0;
    if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_CERT_INFO_PARAM, 0, nullptr, &nSignerCertInfo))
    {
        SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgGetParam() failed");
        return false;
    }
    std::unique_ptr<BYTE[]> pSignerCertInfoBuf(new BYTE[nSignerCertInfo]);
    if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_CERT_INFO_PARAM, 0, pSignerCertInfoBuf.get(), &nSignerCertInfo))
    {
        SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgGetParam() failed");
        return false;
    }
    PCERT_INFO pSignerCertInfo = reinterpret_cast<PCERT_INFO>(pSignerCertInfoBuf.get());

    // Open a certificate store in memory using CERT_STORE_PROV_MSG, which
    // initializes it with the certificates from the message.
    HCERTSTORE hStoreHandle = CertOpenStore(CERT_STORE_PROV_MSG,
                                            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                            0,
                                            0,
                                            hMsg);
    if (!hStoreHandle)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: CertOpenStore() failed");
        return false;
    }

    // Find the signer's certificate in the store.
    PCCERT_CONTEXT pSignerCertContext = CertGetSubjectCertificateFromStore(hStoreHandle,
                                        PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                        pSignerCertInfo);
    if (!pSignerCertContext)
    {
        SAL_WARN("svl.crypto", "ValidateSignature: CertGetSubjectCertificateFromStore() failed");
        return false;
    }
    else
    {
        // Write rInformation.ouX509Certificate.
        uno::Sequence<sal_Int8> aDerCert(pSignerCertContext->cbCertEncoded);
        std::copy_n(pSignerCertContext->pbCertEncoded, pSignerCertContext->cbCertEncoded,
                    aDerCert.getArray());
        OUStringBuffer aBuffer;
        comphelper::Base64::encode(aBuffer, aDerCert);
        SignatureInformation::X509Data temp;
        temp.emplace_back();
        temp.back().X509Certificate = aBuffer.makeStringAndClear();
        temp.back().X509Subject = GetSubjectName(pSignerCertContext);
        rInformation.X509Datas.clear();
        rInformation.X509Datas.emplace_back(temp);
    }

    std::vector<BYTE> aContentParam;

    if (bNonDetached)
    {
        // Not a detached signature.
        DWORD nContentParam = 0;
        if (!CryptMsgGetParam(hMsg, CMSG_CONTENT_PARAM, 0, nullptr, &nContentParam))
        {
            SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgGetParam() failed");
            return false;
        }

        aContentParam.resize(nContentParam);
        if (!CryptMsgGetParam(hMsg, CMSG_CONTENT_PARAM, 0, aContentParam.data(), &nContentParam))
        {
            SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgGetParam() failed");
            return false;
        }
    }

    if (!bNonDetached || VerifyNonDetachedSignature(aData, aContentParam))
    {
        // Use the CERT_INFO from the signer certificate to verify the signature.
        if (CryptMsgControl(hMsg, 0, CMSG_CTRL_VERIFY_SIGNATURE, pSignerCertContext->pCertInfo))
            rInformation.nStatus = xml::crypto::SecurityOperationStatus_OPERATION_SUCCEEDED;
    }

    // Check if we have a signing certificate attribute.
    DWORD nSignedAttributes = 0;
    if (CryptMsgGetParam(hMsg, CMSG_SIGNER_AUTH_ATTR_PARAM, 0, nullptr, &nSignedAttributes))
    {
        std::unique_ptr<BYTE[]> pSignedAttributesBuf(new BYTE[nSignedAttributes]);
        if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_AUTH_ATTR_PARAM, 0, pSignedAttributesBuf.get(), &nSignedAttributes))
        {
            SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgGetParam() authenticated failed");
            return false;
        }
        auto pSignedAttributes = reinterpret_cast<PCRYPT_ATTRIBUTES>(pSignedAttributesBuf.get());
        for (size_t nAttr = 0; nAttr < pSignedAttributes->cAttr; ++nAttr)
        {
            CRYPT_ATTRIBUTE& rAttr = pSignedAttributes->rgAttr[nAttr];
            /*
             * id-aa-signingCertificateV2 OBJECT IDENTIFIER ::=
             * { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) pkcs9(9)
             *   smime(16) id-aa(2) 47 }
             */
            if (std::string_view("1.2.840.113549.1.9.16.2.47") == rAttr.pszObjId)
            {
                rInformation.bHasSigningCertificate = true;
                break;
            }
        }
    }

    // Get the unauthorized attributes.
    nSignedAttributes = 0;
    if (CryptMsgGetParam(hMsg, CMSG_SIGNER_UNAUTH_ATTR_PARAM, 0, nullptr, &nSignedAttributes))
    {
        std::unique_ptr<BYTE[]> pSignedAttributesBuf(new BYTE[nSignedAttributes]);
        if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_UNAUTH_ATTR_PARAM, 0, pSignedAttributesBuf.get(), &nSignedAttributes))
        {
            SAL_WARN("svl.crypto", "ValidateSignature: CryptMsgGetParam() unauthenticated failed");
            return false;
        }
        auto pSignedAttributes = reinterpret_cast<PCRYPT_ATTRIBUTES>(pSignedAttributesBuf.get());
        for (size_t nAttr = 0; nAttr < pSignedAttributes->cAttr; ++nAttr)
        {
            CRYPT_ATTRIBUTE& rAttr = pSignedAttributes->rgAttr[nAttr];
            // Timestamp blob
            if (std::string_view("1.2.840.113549.1.9.16.2.14") == rAttr.pszObjId)
            {
                PCRYPT_TIMESTAMP_CONTEXT pTsContext;
                if (!CryptVerifyTimeStampSignature(rAttr.rgValue->pbData, rAttr.rgValue->cbData, nullptr, 0, nullptr, &pTsContext, nullptr, nullptr))
                {
                    SAL_WARN("svl.crypto", "CryptMsgUpdate failed: " << comphelper::WindowsErrorString(GetLastError()));
                    break;
                }

                DateTime aDateTime = DateTime::CreateFromWin32FileDateTime(pTsContext->pTimeStamp->ftTime.dwLowDateTime, pTsContext->pTimeStamp->ftTime.dwHighDateTime);

                // Then convert to a local UNO DateTime.
                aDateTime.ConvertToLocalTime();
                rInformation.stDateTime = aDateTime.GetUNODateTime();
                if (rInformation.ouDateTime.isEmpty())
                {
                    OUStringBuffer rBuffer;
                    rBuffer.append(static_cast<sal_Int32>(aDateTime.GetYear()));
                    rBuffer.append('-');
                    if (aDateTime.GetMonth() < 10)
                        rBuffer.append('0');
                    rBuffer.append(static_cast<sal_Int32>(aDateTime.GetMonth()));
                    rBuffer.append('-');
                    if (aDateTime.GetDay() < 10)
                        rBuffer.append('0');
                    rBuffer.append(static_cast<sal_Int32>(aDateTime.GetDay()));
                    rInformation.ouDateTime = rBuffer.makeStringAndClear();
                }
                break;
            }
        }
    }

    CertCloseStore(hStoreHandle, CERT_CLOSE_STORE_FORCE_FLAG);
    CryptMsgClose(hMsg);
    return true;
#else
    // Not implemented.
    (void)aData;
    (void)bNonDetached;
    (void)aSignature;
    (void)rInformation;
    return false;
#endif
}

bool Signing::Verify(SvStream& rStream,
                     const std::vector<std::pair<size_t, size_t>>& aByteRanges,
                     const bool bNonDetached,
                     const std::vector<unsigned char>& aSignature,
                     SignatureInformation& rInformation)
{
#if USE_CRYPTO_ANY
    std::vector<unsigned char> buffer;

    // Copy the byte ranges into a single buffer.
    for (const auto& rByteRange : aByteRanges)
    {
        rStream.Seek(rByteRange.first);
        const size_t size = buffer.size();
        buffer.resize(size + rByteRange.second);
        rStream.ReadBytes(buffer.data() + size, rByteRange.second);
    }

    return Verify(buffer, bNonDetached, aSignature, rInformation);

#else
    // Not implemented.
    (void)rStream;
    (void)aByteRanges;
    (void)bNonDetached;
    (void)aSignature;
    (void)rInformation;
    return false;
#endif
}

void Signing::appendHex(sal_Int8 nInt, OStringBuffer& rBuffer)
{
    static const char pHexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    rBuffer.append( pHexDigits[ (nInt >> 4) & 15 ] );
    rBuffer.append( pHexDigits[ nInt & 15 ] );
}

bool CertificateOrName::Is() const
{
    return m_xCertificate.is() || !m_aName.isEmpty();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
