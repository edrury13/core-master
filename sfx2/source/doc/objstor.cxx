/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <config_features.h>

#include <sal/config.h>
#include <sal/log.hxx>

#include <cassert>

#include <svl/eitem.hxx>
#include <svl/stritem.hxx>
#include <svl/intitem.hxx>
#include <com/sun/star/frame/theGlobalEventBroadcaster.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XModule.hpp>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/document/XImporter.hpp>
#include <com/sun/star/document/XExporter.hpp>
#include <com/sun/star/packages/zip/ZipIOException.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/document/MacroExecMode.hpp>
#include <com/sun/star/ui/dialogs/ExtendedFilePickerElementIds.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/embed/XTransactedObject.hpp>
#include <com/sun/star/embed/XEmbeddedObject.hpp>
#include <com/sun/star/embed/XEmbedPersist.hpp>
#include <com/sun/star/embed/XOptimizedStorage.hpp>
#include <com/sun/star/embed/XEncryptionProtectedStorage.hpp>
#include <com/sun/star/io/WrongFormatException.hpp>
#include <com/sun/star/io/XTruncate.hpp>
#include <com/sun/star/util/XModifiable.hpp>
#include <com/sun/star/util/RevisionTag.hpp>
#include <com/sun/star/security/DocumentDigitalSignatures.hpp>
#include <com/sun/star/text/XTextRange.hpp>
#include <com/sun/star/xml/crypto/CipherID.hpp>
#include <com/sun/star/xml/crypto/DigestID.hpp>
#include <com/sun/star/xml/crypto/KDFID.hpp>

#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <comphelper/fileformat.h>
#include <comphelper/processfactory.hxx>
#include <svtools/langtab.hxx>
#include <svtools/sfxecode.hxx>
#include <unotools/configmgr.hxx>
#include <unotools/streamwrap.hxx>

#include <unotools/saveopt.hxx>
#include <unotools/useroptions.hxx>
#include <unotools/securityoptions.hxx>
#include <tools/urlobj.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <comphelper/memorystream.hxx>
#include <unotools/ucbhelper.hxx>
#include <unotools/tempfile.hxx>
#include <unotools/docinfohelper.hxx>
#include <unotools/mediadescriptor.hxx>
#include <ucbhelper/content.hxx>
#include <sot/storage.hxx>
#include <sot/exchange.hxx>
#include <sot/formats.hxx>
#include <comphelper/storagehelper.hxx>
#include <comphelper/documentconstants.hxx>
#include <comphelper/string.hxx>
#include <vcl/errinf.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <basic/modsizeexceeded.hxx>
#include <officecfg/Office/Common.hxx>
#include <osl/file.hxx>
#include <comphelper/scopeguard.hxx>
#include <comphelper/lok.hxx>
#include <i18nlangtag/languagetag.hxx>

#include <sfx2/signaturestate.hxx>
#include <sfx2/app.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/gdrivesync.hxx>
#include <sfx2/sfxresid.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/docfac.hxx>
#include <appopen.hxx>
#include <objshimp.hxx>
#include <sfx2/lokhelper.hxx>
#include <sfx2/strings.hrc>
#include <sfx2/sfxsids.hrc>
#include <sfx2/dispatch.hxx>
#include <sfx2/sfxuno.hxx>
#include <sfx2/event.hxx>
#include <sfx2/infobar.hxx>
#include <fltoptint.hxx>
#include <sfx2/viewfrm.hxx>
#include "graphhelp.hxx"
#include <appbaslib.hxx>
#include <guisaveas.hxx>
#include "objstor.hxx"
#include "exoticfileloadexception.hxx"
#include <unicode/ucsdet.h>
#include <o3tl/string_view.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::document;
using namespace ::cppu;


static css::uno::Any getODFVersionAny(SvtSaveOptions::ODFSaneDefaultVersion v)
{
    if (v >= SvtSaveOptions::ODFSaneDefaultVersion::ODFSVER_014)
        return css::uno::Any(ODFVER_014_TEXT);
    else if (v >= SvtSaveOptions::ODFSaneDefaultVersion::ODFSVER_013)
        return css::uno::Any(ODFVER_013_TEXT);
    else
        return css::uno::Any(ODFVER_012_TEXT);
}


void impl_addToModelCollection(const css::uno::Reference< css::frame::XModel >& xModel)
{
    if (!xModel.is())
        return;

    const css::uno::Reference< css::uno::XComponentContext >& xContext = ::comphelper::getProcessComponentContext();
    css::uno::Reference< css::frame::XGlobalEventBroadcaster > xModelCollection =
        css::frame::theGlobalEventBroadcaster::get(xContext);
    try
    {
        xModelCollection->insert(css::uno::Any(xModel));
    }
    catch ( uno::Exception& )
    {
        SAL_WARN( "sfx.doc", "The document seems to be in the collection already!" );
    }
}


bool SfxObjectShell::Save()
{
    SaveChildren();
    return true;
}


bool SfxObjectShell::SaveAs( SfxMedium& rMedium )
{
    return SaveAsChildren( rMedium );
}


bool SfxObjectShell::QuerySlotExecutable( sal_uInt16 /*nSlotId*/ )
{
    return true;
}

namespace sfx2 {

bool UseODFWholesomeEncryption(SvtSaveOptions::ODFSaneDefaultVersion const nODFVersion)
{
    return nODFVersion == SvtSaveOptions::ODFSVER_LATEST_EXTENDED;
}

} // namespace sfx2

bool GetEncryptionData_Impl( const SfxItemSet* pSet, uno::Sequence< beans::NamedValue >& o_rEncryptionData )
{
    bool bResult = false;
    if ( pSet )
    {
        const SfxUnoAnyItem* pEncryptionDataItem = SfxItemSet::GetItem<SfxUnoAnyItem>(pSet, SID_ENCRYPTIONDATA, false);
        if ( pEncryptionDataItem )
        {
            pEncryptionDataItem->GetValue() >>= o_rEncryptionData;
            bResult = true;
        }
        else
        {
            const SfxStringItem* pPasswordItem = SfxItemSet::GetItem<SfxStringItem>(pSet, SID_PASSWORD, false);
            if ( pPasswordItem )
            {
                o_rEncryptionData = ::comphelper::OStorageHelper::CreatePackageEncryptionData( pPasswordItem->GetValue() );
                bResult = true;
            }
        }
    }

    return bResult;
}


bool SfxObjectShell::PutURLContentsToVersionStream_Impl(
                                            const OUString& aURL,
                                            const uno::Reference< embed::XStorage >& xDocStorage,
                                            const OUString& aStreamName )
{
    bool bResult = false;
    try
    {
        uno::Reference< embed::XStorage > xVersion = xDocStorage->openStorageElement(
                                                        u"Versions"_ustr,
                                                        embed::ElementModes::READWRITE );

        DBG_ASSERT( xVersion.is(),
                "The method must throw an exception if the storage can not be opened!" );
        if ( !xVersion.is() )
            throw uno::RuntimeException();

        uno::Reference< io::XStream > xVerStream = xVersion->openStreamElement(
                                                                aStreamName,
                                                                embed::ElementModes::READWRITE );
        DBG_ASSERT( xVerStream.is(), "The method must throw an exception if the storage can not be opened!" );
        if ( !xVerStream.is() )
            throw uno::RuntimeException();

        uno::Reference< io::XOutputStream > xOutStream = xVerStream->getOutputStream();
        uno::Reference< io::XTruncate > xTrunc( xOutStream, uno::UNO_QUERY_THROW );

        uno::Reference< io::XInputStream > xTmpInStream =
            ::comphelper::OStorageHelper::GetInputStreamFromURL(
                aURL, comphelper::getProcessComponentContext() );
        assert( xTmpInStream.is() );

        xTrunc->truncate();
        ::comphelper::OStorageHelper::CopyInputToOutput( xTmpInStream, xOutStream );
        xOutStream->closeOutput();

        uno::Reference< embed::XTransactedObject > xTransact( xVersion, uno::UNO_QUERY );
        DBG_ASSERT( xTransact.is(), "The storage must implement XTransacted interface!\n" );
        if ( xTransact.is() )
            xTransact->commit();

        bResult = true;
    }
    catch( uno::Exception& )
    {
        // TODO/LATER: handle the error depending on exception
        SetError(ERRCODE_IO_GENERAL);
    }

    return bResult;
}


OUString SfxObjectShell::CreateTempCopyOfStorage_Impl( const uno::Reference< embed::XStorage >& xStorage )
{
    OUString aTempURL = ::utl::CreateTempURL();

    DBG_ASSERT( !aTempURL.isEmpty(), "Can't create a temporary file!\n" );
    if ( !aTempURL.isEmpty() )
    {
        try
        {
            uno::Reference< embed::XStorage > xTempStorage =
                ::comphelper::OStorageHelper::GetStorageFromURL( aTempURL, embed::ElementModes::READWRITE );

            // the password will be transferred from the xStorage to xTempStorage by storage implementation
            xStorage->copyToStorage( xTempStorage );

            // the temporary storage was committed by the previous method and it will die by refcount
        }
        catch ( uno::Exception& )
        {
            SAL_WARN( "sfx.doc", "Creation of a storage copy is failed!" );
            ::utl::UCBContentHelper::Kill( aTempURL );

            aTempURL.clear();

            // TODO/LATER: may need error code setting based on exception
            SetError(ERRCODE_IO_GENERAL);
        }
    }

    return aTempURL;
}


SvGlobalName const & SfxObjectShell::GetClassName() const
{
    return GetFactory().GetClassId();
}


void SfxObjectShell::SetupStorage( const uno::Reference< embed::XStorage >& xStorage,
                                   sal_Int32 nVersion, bool bTemplate ) const
{
    uno::Reference< beans::XPropertySet > xProps( xStorage, uno::UNO_QUERY );

    if ( !xProps.is() )
        return;

    SotClipboardFormatId nClipFormat = SotClipboardFormatId::NONE;

    SvGlobalName aName;
    OUString aFullTypeName;
    FillClass( &aName, &nClipFormat, &aFullTypeName, nVersion, bTemplate );

    if ( nClipFormat == SotClipboardFormatId::NONE )
        return;

    // basic doesn't have a ClipFormat
    // without MediaType the storage is not really usable, but currently the BasicIDE still
    // is an SfxObjectShell and so we can't take this as an error
    datatransfer::DataFlavor aDataFlavor;
    SotExchange::GetFormatDataFlavor( nClipFormat, aDataFlavor );
    if ( aDataFlavor.MimeType.isEmpty() )
        return;

    try
    {
        xProps->setPropertyValue(u"MediaType"_ustr, uno::Any( aDataFlavor.MimeType ) );
    }
    catch( uno::Exception& )
    {
        const_cast<SfxObjectShell*>( this )->SetError(ERRCODE_IO_GENERAL);
    }

    SvtSaveOptions::ODFSaneDefaultVersion nDefVersion = SvtSaveOptions::ODFSVER_014;
    if (!comphelper::IsFuzzing())
    {
        nDefVersion = GetODFSaneDefaultVersion();
    }

    // the default values, that should be used for ODF1.1 and older formats
    uno::Sequence< beans::NamedValue > aEncryptionAlgs
    {
        { u"StartKeyGenerationAlgorithm"_ustr, css::uno::Any(xml::crypto::DigestID::SHA1) },
        { u"EncryptionAlgorithm"_ustr, css::uno::Any(xml::crypto::CipherID::BLOWFISH_CFB_8) },
        { u"ChecksumAlgorithm"_ustr, css::uno::Any(xml::crypto::DigestID::SHA1_1K) },
        { u"KeyDerivationFunction"_ustr, css::uno::Any(xml::crypto::KDFID::PBKDF2) },
    };

    if (nDefVersion >= SvtSaveOptions::ODFSVER_012)
    {
        try
        {
            // older versions can not have this property set, it exists only starting from ODF1.2
            xProps->setPropertyValue(u"Version"_ustr, getODFVersionAny(nDefVersion));
        }
        catch( uno::Exception& )
        {
        }

        auto pEncryptionAlgs = aEncryptionAlgs.getArray();
        pEncryptionAlgs[0].Value <<= xml::crypto::DigestID::SHA256;
        if (::sfx2::UseODFWholesomeEncryption(nDefVersion))
        {
            pEncryptionAlgs[1].Value <<= xml::crypto::CipherID::AES_GCM_W3C;
            pEncryptionAlgs[2].Value.clear();
            if (!getenv("LO_ARGON2_DISABLE"))
            {
                pEncryptionAlgs[3].Value <<= xml::crypto::KDFID::Argon2id;
            }
        }
        else
        {
            pEncryptionAlgs[1].Value <<= xml::crypto::CipherID::AES_CBC_W3C_PADDING;
            pEncryptionAlgs[2].Value <<= xml::crypto::DigestID::SHA256_1K;
        }
    }

    try
    {
        // set the encryption algorithms accordingly;
        // the setting does not trigger encryption,
        // it just provides the format for the case that contents should be encrypted
        uno::Reference< embed::XEncryptionProtectedStorage > xEncr( xStorage, uno::UNO_QUERY_THROW );
        xEncr->setEncryptionAlgorithms( aEncryptionAlgs );
    }
    catch( uno::Exception& )
    {
        const_cast<SfxObjectShell*>( this )->SetError(ERRCODE_IO_GENERAL);
    }
}


void SfxObjectShell::PrepareSecondTryLoad_Impl()
{
    // only for internal use
    pImpl->m_xDocStorage.clear();
    pImpl->mxObjectContainer.reset();
    pImpl->nDocumentSignatureState = SignatureState::UNKNOWN;
    pImpl->nScriptingSignatureState = SignatureState::UNKNOWN;
    pImpl->m_bIsInit = false;
    ResetError();
}


bool SfxObjectShell::GeneralInit_Impl( const uno::Reference< embed::XStorage >& xStorage,
                                            bool bTypeMustBeSetAlready )
{
    if ( pImpl->m_bIsInit )
        return false;

    pImpl->m_bIsInit = true;
    if ( xStorage.is() )
    {
        // no notification is required the storage is set the first time
        pImpl->m_xDocStorage = xStorage;

        try {
            uno::Reference < beans::XPropertySet > xPropSet( xStorage, uno::UNO_QUERY_THROW );
            Any a = xPropSet->getPropertyValue(u"MediaType"_ustr);
            OUString aMediaType;
            if ( !(a>>=aMediaType) || aMediaType.isEmpty() )
            {
                if ( bTypeMustBeSetAlready )
                {
                    SetError(ERRCODE_IO_BROKENPACKAGE);
                    return false;
                }

                SetupStorage( xStorage, SOFFICE_FILEFORMAT_CURRENT, false );
            }
        }
        catch ( uno::Exception& )
        {
            SAL_WARN( "sfx.doc", "Can't check storage's mediatype!" );
        }
    }
    else
        pImpl->m_bCreateTempStor = true;

    return true;
}


bool SfxObjectShell::InitNew( const uno::Reference< embed::XStorage >& xStorage )
{
    return GeneralInit_Impl( xStorage, false );
}


bool SfxObjectShell::Load( SfxMedium& rMedium )
{
    return GeneralInit_Impl(rMedium.GetStorage(), true);
}

void SfxObjectShell::DoInitUnitTest()
{
    pMedium = new SfxMedium;
}

bool SfxObjectShell::DoInitNew()
/*  [Description]

    This from SvPersist inherited virtual method is called to initialize
    the SfxObjectShell instance from a storage (PStore! = 0) or (PStore == 0)

    Like with all Do...-methods there is a from a control, the actual
    implementation is done by the virtual method in which also the
    InitNew(SvStorate *) from the SfxObjectShell-Subclass is implemented.

    For pStore == 0 the SfxObjectShell-instance is connected to an empty
    SfxMedium, otherwise a SfxMedium, which refers to the SotStorage
    passed as a parameter.

    The object is only initialized correctly after InitNew() or Load().

    [Return value]
    true            The object has been initialized.
    false           The object could not be initialized
*/

{
    ModifyBlocker_Impl aBlock( this );
    pMedium = new SfxMedium;

    pMedium->CanDisposeStorage_Impl( true );

    if ( InitNew( nullptr ) )
    {
        // empty documents always get their macros from the user, so there is no reason to restrict access
        pImpl->aMacroMode.allowMacroExecution();
        if ( SfxObjectCreateMode::EMBEDDED == eCreateMode )
            SetTitle(SfxResId(STR_NONAME));

        uno::Reference< frame::XModel >  xModel = GetModel();
        if ( xModel.is() )
        {
            SfxItemSet &rSet = GetMedium()->GetItemSet();
            uno::Sequence< beans::PropertyValue > aArgs;
            TransformItems( SID_OPENDOC, rSet, aArgs );
            sal_Int32 nLength = aArgs.getLength();
            aArgs.realloc( nLength + 1 );
            auto pArgs = aArgs.getArray();
            pArgs[nLength].Name = "Title";
            pArgs[nLength].Value <<= GetTitle( SFX_TITLE_DETECT );
            xModel->attachResource( OUString(), aArgs );
            if (!comphelper::IsFuzzing())
                impl_addToModelCollection(xModel);
        }

        SetInitialized_Impl( true );
        return true;
    }

    return false;
}

bool SfxObjectShell::ImportFromGeneratedStream_Impl(
                    const uno::Reference< io::XStream >& xStream,
                    const uno::Sequence< beans::PropertyValue >& rMediaDescr )
{
    if ( !xStream.is() )
        return false;

    if ( pMedium && pMedium->HasStorage_Impl() )
        pMedium->CloseStorage();

    bool bResult = false;

    try
    {
        uno::Reference< embed::XStorage > xStorage =
            ::comphelper::OStorageHelper::GetStorageFromStream( xStream );

        if ( !xStorage.is() )
            throw uno::RuntimeException();

        if ( !pMedium )
            pMedium = new SfxMedium( xStorage, OUString() );
        else
            pMedium->SetStorage_Impl( xStorage );

        SfxAllItemSet aSet( SfxGetpApp()->GetPool() );
        TransformParameters( SID_OPENDOC, rMediaDescr, aSet );
        pMedium->GetItemSet().Put( aSet );
        pMedium->CanDisposeStorage_Impl( false );
        uno::Reference<text::XTextRange> xInsertTextRange;
        for (const auto& rProp : rMediaDescr)
        {
            if (rProp.Name == "TextInsertModeRange")
            {
                rProp.Value >>= xInsertTextRange;
            }
        }

        if (xInsertTextRange.is())
        {
            bResult = InsertGeneratedStream(*pMedium, xInsertTextRange);
        }
        else
        {

            // allow the subfilter to reinit the model
            if ( pImpl->m_bIsInit )
                pImpl->m_bIsInit = false;

            if ( LoadOwnFormat( *pMedium ) )
            {
                bHasName = true;
                if ( !IsReadOnly() && IsLoadReadonly() )
                    SetReadOnlyUI();

                bResult = true;
                OSL_ENSURE( pImpl->m_xDocStorage == xStorage, "Wrong storage is used!" );
            }
        }

        // now the medium can be disconnected from the storage
        // the medium is not allowed to dispose the storage so CloseStorage() can be used
        pMedium->CloseStorage();
    }
    catch( uno::Exception& )
    {
    }

    return bResult;
}


bool SfxObjectShell::DoLoad( SfxMedium *pMed )
{
    ModifyBlocker_Impl aBlock( this );

    pMedium = pMed;
    pMedium->CanDisposeStorage_Impl( true );

    bool bOk = false;
    std::shared_ptr<const SfxFilter> pFilter = pMed->GetFilter();
    SfxItemSet& rSet = pMedium->GetItemSet();
    if( pImpl->nEventId == SfxEventHintId::NONE )
    {
        const SfxBoolItem* pTemplateItem = rSet.GetItem(SID_TEMPLATE, false);
        SetActivateEvent_Impl(
            ( pTemplateItem && pTemplateItem->GetValue() )
            ? SfxEventHintId::CreateDoc : SfxEventHintId::OpenDoc );
    }

    const SfxStringItem* pBaseItem = rSet.GetItem(SID_BASEURL, false);
    OUString aBaseURL;
    const SfxStringItem* pSalvageItem = rSet.GetItem(SID_DOC_SALVAGE, false);
    if( pBaseItem )
        aBaseURL = pBaseItem->GetValue();
    else
    {
        if ( pSalvageItem )
        {
            osl::FileBase::getFileURLFromSystemPath( pMed->GetPhysicalName(), aBaseURL );
        }
        else
            aBaseURL = pMed->GetBaseURL();
    }
    pMed->GetItemSet().Put( SfxStringItem( SID_DOC_BASEURL, aBaseURL ) );

    pImpl->nLoadedFlags = SfxLoadedFlags::NONE;
    pImpl->bModelInitialized = false;

    if (pFilter && !pFilter->IsEnabled())
    {
        SetError( ERRCODE_IO_FILTERDISABLED );
    }

    if ( pFilter && pFilter->IsExoticFormat() && !QueryAllowExoticFormat_Impl( getInteractionHandler(), aBaseURL, pMed->GetFilter()->GetUIName() ) )
    {
        SetError( ERRCODE_IO_ABORT );
    }

    // initialize static language table so language-related extensions are learned before the document loads
    (void)SvtLanguageTable::GetLanguageEntryCount();

    //TODO/LATER: make a clear strategy how to handle "UsesStorage" etc.
    bool bOwnStorageFormat = IsOwnStorageFormat( *pMedium );
    bool bHasStorage = IsPackageStorageFormat_Impl( *pMedium );
    if ( pMedium->GetFilter() )
    {
        ErrCode nError = HandleFilter( pMedium, this );
        if ( nError != ERRCODE_NONE )
            SetError(nError);

        if (pMedium->GetFilter()->GetFilterFlags() & SfxFilterFlags::STARTPRESENTATION)
            rSet.Put(SfxUInt16Item(SID_DOC_STARTPRESENTATION, 1));
    }

    EnableSetModified( false );

    // tdf#53614 - don't try to lock file after cancelling the import process
    if (GetErrorIgnoreWarning() != ERRCODE_ABORT)
        pMedium->LockOrigFileOnDemand( true, false );
    if ( GetErrorIgnoreWarning() == ERRCODE_NONE && bOwnStorageFormat && ( !pFilter || !( pFilter->GetFilterFlags() & SfxFilterFlags::STARONEFILTER ) ) )
    {
        uno::Reference< embed::XStorage > xStorage;
        if ( pMedium->GetErrorIgnoreWarning() == ERRCODE_NONE )
            xStorage = pMedium->GetStorage();

        if( xStorage.is() && pMedium->GetLastStorageCreationState() == ERRCODE_NONE )
        {
            DBG_ASSERT( pFilter, "No filter for storage found!" );

            try
            {
                bool bWarnMediaTypeFallback = false;

                // treat the package as broken if the mediatype was retrieved as a fallback
                uno::Reference< beans::XPropertySet > xStorProps( xStorage, uno::UNO_QUERY_THROW );
                xStorProps->getPropertyValue(u"MediaTypeFallbackUsed"_ustr)
                                                                    >>= bWarnMediaTypeFallback;

                if (pMedium->IsRepairPackage())
                {
                    // the macros in repaired documents should be disabled
                    pMedium->GetItemSet().Put( SfxUInt16Item( SID_MACROEXECMODE, document::MacroExecMode::NEVER_EXECUTE ) );

                    // the mediatype was retrieved by using fallback solution but this is a repairing mode
                    // so it is acceptable to open the document if there is no contents that required manifest.xml
                    bWarnMediaTypeFallback = false;
                }

                if (bWarnMediaTypeFallback || !xStorage->getElementNames().hasElements())
                    SetError(ERRCODE_IO_BROKENPACKAGE);
            }
            catch( uno::Exception& )
            {
                // TODO/LATER: may need error code setting based on exception
                SetError(ERRCODE_IO_GENERAL);
            }

            // Load
            if ( !GetErrorIgnoreWarning() )
            {
                pImpl->nLoadedFlags = SfxLoadedFlags::NONE;
                pImpl->bModelInitialized = false;
                bOk = xStorage.is() && LoadOwnFormat( *pMed );
                if ( bOk )
                {
                    // the document loaded from template has no name
                    const SfxBoolItem* pTemplateItem = rSet.GetItem(SID_TEMPLATE, false);
                    if ( !pTemplateItem || !pTemplateItem->GetValue() )
                        bHasName = true;
                }
                else
                    SetError(ERRCODE_ABORT);
            }
        }
        else
            SetError(pMed->GetLastStorageCreationState());
    }
    else if ( GetErrorIgnoreWarning() == ERRCODE_NONE && InitNew(nullptr) )
    {
        // set name before ConvertFrom, so that GetSbxObject() already works
        bHasName = true;
        SetName( SfxResId(STR_NONAME) );

        if( !bHasStorage )
            pMedium->GetInStream();
        else
            pMedium->GetStorage();

        if ( GetErrorIgnoreWarning() == ERRCODE_NONE )
        {
            // Experimental PDF importing using PDFium. This is currently enabled for LOK only and
            // we handle it not via XmlFilterAdaptor but a new SdPdfFilter.
#if !HAVE_FEATURE_POPPLER
            constexpr bool bUsePdfium = true;
#else
            const bool bUsePdfium
                = comphelper::LibreOfficeKit::isActive() || getenv("LO_IMPORT_USE_PDFIUM");
#endif
            const bool bPdfiumImport
                = bUsePdfium && pMedium->GetFilter()
                  && (pMedium->GetFilter()->GetFilterName() == "draw_pdf_import");

            pImpl->nLoadedFlags = SfxLoadedFlags::NONE;
            pImpl->bModelInitialized = false;
            if (pMedium->GetFilter()
                && (pMedium->GetFilter()->GetFilterFlags() & SfxFilterFlags::STARONEFILTER)
                && !bPdfiumImport)
            {
                uno::Reference < beans::XPropertySet > xSet( GetModel(), uno::UNO_QUERY );
                static constexpr OUString sLockUpdates(u"LockUpdates"_ustr);
                bool bSetProperty = true;
                try
                {
                    xSet->setPropertyValue( sLockUpdates, Any( true ) );
                }
                catch(const beans::UnknownPropertyException& )
                {
                    bSetProperty = false;
                }
                bOk = ImportFrom(*pMedium, nullptr);
                if(bSetProperty)
                {
                    try
                    {
                        xSet->setPropertyValue( sLockUpdates, Any( false ) );
                    }
                    catch(const beans::UnknownPropertyException& )
                    {}
                }
                UpdateLinks();
                FinishedLoading();
            }
            else
            {
                if (tools::isEmptyFileUrl(pMedium->GetName()))
                {
                    // The import filter would fail with empty input.
                    bOk = true;
                }
                else
                {
                    bOk = ConvertFrom(*pMedium);
                }
                InitOwnModel_Impl();
            }
        }
    }

    if ( bOk )
    {
        if ( IsReadOnlyMedium() || IsLoadReadonly() )
            SetReadOnlyUI();

        try
        {
            ::ucbhelper::Content aContent( pMedium->GetName(), utl::UCBContentHelper::getDefaultCommandEnvironment(), comphelper::getProcessComponentContext() );
            css::uno::Reference < XPropertySetInfo > xProps = aContent.getProperties();
            if ( xProps.is() )
            {
                static constexpr OUString aAuthor( u"Author"_ustr );
                static constexpr OUString aKeywords( u"Keywords"_ustr );
                static constexpr OUString aSubject( u"Subject"_ustr );
                Any aAny;
                OUString aValue;
                uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                    GetModel(), uno::UNO_QUERY_THROW);
                uno::Reference<document::XDocumentProperties> xDocProps
                    = xDPS->getDocumentProperties();
                if ( xProps->hasPropertyByName( aAuthor ) )
                {
                    aAny = aContent.getPropertyValue( aAuthor );
                    if ( aAny >>= aValue )
                        xDocProps->setAuthor(aValue);
                }
                if ( xProps->hasPropertyByName( aKeywords ) )
                {
                    aAny = aContent.getPropertyValue( aKeywords );
                    if ( aAny >>= aValue )
                        xDocProps->setKeywords(
                          ::comphelper::string::convertCommaSeparated(aValue));
;
                }
                if ( xProps->hasPropertyByName( aSubject ) )
                {
                    aAny = aContent.getPropertyValue( aSubject );
                    if ( aAny >>= aValue ) {
                        xDocProps->setSubject(aValue);
                    }
                }
            }
        }
        catch( Exception& )
        {
        }

        // If not loaded asynchronously call FinishedLoading
        if ( !( pImpl->nLoadedFlags & SfxLoadedFlags::MAINDOCUMENT ) &&
              ( !pMedium->GetFilter() || pMedium->GetFilter()->UsesStorage() )
            )
            FinishedLoading( SfxLoadedFlags::MAINDOCUMENT );

        Broadcast( SfxHint(SfxHintId::NameChanged) );

        if ( SfxObjectCreateMode::EMBEDDED != eCreateMode )
        {
            const SfxBoolItem* pAsTempItem = rSet.GetItem(SID_TEMPLATE, false);
            const SfxBoolItem* pPreviewItem = rSet.GetItem(SID_PREVIEW, false);
            const SfxBoolItem* pHiddenItem = rSet.GetItem(SID_HIDDEN, false);
            if( !pMedium->GetOrigURL().isEmpty()
            && !( pAsTempItem && pAsTempItem->GetValue() )
            && !( pPreviewItem && pPreviewItem->GetValue() )
            && !( pHiddenItem && pHiddenItem->GetValue() ) )
            {
                AddToRecentlyUsedList();
            }
        }

        const SfxBoolItem* pDdeReconnectItem = rSet.GetItem(SID_DDE_RECONNECT_ONLOAD, false);

        bool bReconnectDde = true; // by default, we try to auto-connect DDE connections.
        if (pDdeReconnectItem)
            bReconnectDde = pDdeReconnectItem->GetValue();

        if (bReconnectDde)
            ReconnectDdeLinks(*this);
    }

    return bOk;
}

bool SfxObjectShell::DoLoadExternal( SfxMedium *pMed )
{
    pMedium = pMed;
    return LoadExternal(*pMedium);
}

const ::std::unordered_map<std::string, rtl_TextEncoding>  mapCharSets =
                            {{"UTF-8", RTL_TEXTENCODING_UTF8},
                            {"UTF-16BE", RTL_TEXTENCODING_UCS2},
                            {"UTF-16LE", RTL_TEXTENCODING_UCS2},
                            {"UTF-32BE", RTL_TEXTENCODING_UCS4},
                            {"UTF-32LE", RTL_TEXTENCODING_UCS4},
                            {"Shift_JIS", RTL_TEXTENCODING_SHIFT_JIS},
                            {"ISO-2022-JP", RTL_TEXTENCODING_ISO_2022_JP},
                            {"ISO-2022-CN", RTL_TEXTENCODING_ISO_2022_CN},
                            {"ISO-2022-KR", RTL_TEXTENCODING_ISO_2022_KR},
                            {"GB18030", RTL_TEXTENCODING_GB_18030},
                            {"Big5", RTL_TEXTENCODING_BIG5},
                            {"EUC-JP", RTL_TEXTENCODING_EUC_JP},
                            {"EUC-KR", RTL_TEXTENCODING_EUC_KR},
                            {"ISO-8859-1", RTL_TEXTENCODING_ISO_8859_1},
                            {"ISO-8859-2", RTL_TEXTENCODING_ISO_8859_2},
                            {"ISO-8859-5", RTL_TEXTENCODING_ISO_8859_5},
                            {"ISO-8859-6", RTL_TEXTENCODING_ISO_8859_6},
                            {"ISO-8859-7", RTL_TEXTENCODING_ISO_8859_7},
                            {"ISO-8859-8", RTL_TEXTENCODING_ISO_8859_8},
                            {"ISO-8859-9", RTL_TEXTENCODING_ISO_8859_9},
                            {"windows-1250", RTL_TEXTENCODING_MS_1250},
                            {"windows-1251", RTL_TEXTENCODING_MS_1251},
                            {"windows-1252", RTL_TEXTENCODING_MS_1252},
                            {"windows-1253", RTL_TEXTENCODING_MS_1253},
                            {"windows-1254", RTL_TEXTENCODING_MS_1254},
                            {"windows-1255", RTL_TEXTENCODING_MS_1255},
                            {"windows-1256", RTL_TEXTENCODING_MS_1256},
                            {"KOI8-R", RTL_TEXTENCODING_KOI8_R}};

void SfxObjectShell::DetectCharSet(SvStream& stream, rtl_TextEncoding& eCharSet, SvStreamEndian &endian)
{
    constexpr size_t buffsize = 4096;
    sal_Int8 bytes[buffsize] = { 0 };
    sal_uInt64 nInitPos = stream.Tell();
    sal_Int32 nRead = stream.ReadBytes(bytes, buffsize);

    stream.Seek(nInitPos);
    eCharSet = RTL_TEXTENCODING_DONTKNOW;

    if (!nRead)
        return;

    UErrorCode uerr = U_ZERO_ERROR;
    UCharsetDetector* ucd = ucsdet_open(&uerr);
    if (!U_SUCCESS(uerr))
        return;

    const UCharsetMatch* match = nullptr;
    const char* pEncodingName = nullptr;
    ucsdet_setText(ucd, reinterpret_cast<const char*>(bytes), nRead, &uerr);
    if (U_SUCCESS(uerr))
        match = ucsdet_detect(ucd, &uerr);

    if (U_SUCCESS(uerr))
        pEncodingName = ucsdet_getName(match, &uerr);

    if (U_SUCCESS(uerr) && pEncodingName)
    {
        const auto it = mapCharSets.find(pEncodingName);
        if (it != mapCharSets.end())
            eCharSet = it->second;

        if (eCharSet == RTL_TEXTENCODING_UNICODE && !strcmp("UTF-16LE", pEncodingName))
            endian = SvStreamEndian::LITTLE;
        else if (eCharSet == RTL_TEXTENCODING_UNICODE && !strcmp("UTF-16BE", pEncodingName))
            endian = SvStreamEndian::BIG;
    }

    ucsdet_close(ucd);
}

void SfxObjectShell::DetectCsvSeparators(SvStream& stream, rtl_TextEncoding eCharSet, OUString& separators, sal_Unicode cStringDelimiter)
{
    OUString sLine;
    std::vector<std::unordered_map<sal_Unicode, sal_uInt32>> aLinesCharsCount;
    std::unordered_map<sal_Unicode, sal_uInt32> aCharsCount;
    std::unordered_map<sal_Unicode, std::pair<sal_uInt32, sal_uInt32>> aStats;
    constexpr sal_uInt32 nTimeout = 500; // Timeout for detection in ms
    sal_uInt32 nLinesCount = 0;
    OUString sInitSeps;
    OUString sCommonSeps = u",\t;:| \\/"_ustr;//Sorted by importance
    std::unordered_set<sal_Unicode> usetCommonSeps;
    bool bIsDelimiter = false;
    // The below two are needed to handle a "not perfect" structure.
    sal_uInt32 nMaxLinesSameChar = 0;
    sal_uInt32 nMinDiffs = 0xFFFFFFFF;
    sal_uInt64 nInitPos = stream.Tell();
    sal_uInt64 nStartTime = tools::Time::GetSystemTicks();

    if (!cStringDelimiter)
        cStringDelimiter = '\"';

    for (sal_Int32 nComSepIdx = sCommonSeps.getLength() - 1; nComSepIdx >= 0; nComSepIdx --)
        usetCommonSeps.insert(sCommonSeps[nComSepIdx]);
    aLinesCharsCount.reserve(128);
    separators = "";

    stream.StartReadingUnicodeText(eCharSet);
    while (stream.ReadUniOrByteStringLine(sLine, eCharSet) && (tools::Time::GetSystemTicks() - nStartTime < nTimeout))
    {
        if (sLine.isEmpty())
            continue;

        if (!nLinesCount)
        {
            if (sLine.getLength() == 5 && sLine.startsWithIgnoreAsciiCase("sep="))
            {
                separators += OUStringChar(sLine[4]);
                break;
            }
            else if (sLine.getLength() == 7 && sLine[6] == '"' && sLine.startsWithIgnoreAsciiCase("\"sep="))
            {
                separators += OUStringChar(sLine[5]);
                break;
            }
        }

        // Count the occurrences of each character within the line.
        // Skip strings.
        const sal_Unicode *pEnd = sLine.getStr() + sLine.getLength();
        for (const sal_Unicode *p = sLine.getStr(); p < pEnd; p++)
        {
            if (*p == cStringDelimiter)
            {
                bIsDelimiter = !bIsDelimiter;
                continue;
            }
            if (bIsDelimiter)
                continue;

            // If restricted only to common separators then skip the rest
            if (usetCommonSeps.find(*p) == usetCommonSeps.end())
                continue;

            auto it_elem = aCharsCount.find(*p);
            if (it_elem == aCharsCount.cend())
                aCharsCount.insert(std::pair<sal_uInt32, sal_uInt32>(*p, 1));
            else
                it_elem->second ++;
        }

        if (bIsDelimiter)
            continue;

        nLinesCount ++;

        // For each character count the lines that contain it and different number of occurrences.
        // And the global maximum for the first statistic.
        for (auto aCurLineChar=aCharsCount.cbegin(); aCurLineChar != aCharsCount.cend(); aCurLineChar++)
        {
            auto aCurStats = aStats.find(aCurLineChar->first);
            if (aCurStats == aStats.cend())
                aCurStats = aStats.insert(std::pair<sal_Unicode, std::pair<sal_uInt32, sal_uInt32>>(aCurLineChar->first, std::pair<sal_uInt32, sal_uInt32>(1, 1))).first;
            else
            {
                aCurStats->second.first ++;// Increment number of lines that contain the current character

                std::vector<std::unordered_map<sal_Unicode, sal_uInt32>>::const_iterator aPrevLineChar;
                for (aPrevLineChar=aLinesCharsCount.cbegin(); aPrevLineChar != aLinesCharsCount.cend(); aPrevLineChar++)
                {
                    auto aPrevStats = aPrevLineChar->find(aCurLineChar->first);
                    if (aPrevStats != aPrevLineChar->cend() && aPrevStats->second == aCurLineChar->second)
                        break;
                }
                if (aPrevLineChar == aLinesCharsCount.cend())
                    aCurStats->second.second ++;// Increment number of different number of occurrences.
            }

            // Update the maximum of number of lines that contain the same character. This is a global value.
            if (nMaxLinesSameChar < aCurStats->second.first)
                nMaxLinesSameChar = aCurStats->second.first;
        }

        aLinesCharsCount.emplace_back();
        aLinesCharsCount[aLinesCharsCount.size() - 1].swap(aCharsCount);
    }

    SAL_INFO("sfx.doc", "" << nLinesCount << " lines processed in " << tools::Time::GetSystemTicks() - nStartTime << " ms while detecting separator.");

    // Compute the global minimum of different number of occurrences.
    // But only for characters which occur in a maximum number of lines (previously computed).
    for (auto it=aStats.cbegin(); it != aStats.cend(); it++)
        if (it->second.first == nMaxLinesSameChar && nMinDiffs > it->second.second)
            nMinDiffs = it->second.second;

    // Compute the initial list of separators: those with the maximum lines of occurrence and
    // the minimum of different number of occurrences.
    for (auto it=aStats.cbegin(); it != aStats.cend(); it++)
        if (it->second.first == nMaxLinesSameChar && it->second.second == nMinDiffs)
            sInitSeps += OUStringChar(it->first);

    // If forced to most common or there are multiple separators then pick up only the most common by importance.
    sal_Int32 nInitSepIdx;
    sal_Int32 nComSepIdx;
    for (nComSepIdx = 0; nComSepIdx < sCommonSeps.getLength(); nComSepIdx++)
    {
        sal_Unicode c = sCommonSeps[nComSepIdx];
        for (nInitSepIdx = sInitSeps.getLength() - 1; nInitSepIdx >= 0; nInitSepIdx --)
        {
            if (c == sInitSeps[nInitSepIdx])
            {
                separators += OUStringChar(c);
                break;
            }
        }

    }

    stream.Seek(nInitPos);
}

void SfxObjectShell::DetectCsvFilterOptions(SvStream& stream, OUString& aFilterOptions)
{
    rtl_TextEncoding eCharSet = RTL_TEXTENCODING_DONTKNOW;
    std::u16string_view aSeps;
    std::u16string_view aDelimiter;
    std::u16string_view aCharSet;
    std::u16string_view aRest;
    OUString aOrigFilterOpts = aFilterOptions;
    bool bDelimiter = false, bCharSet = false, bRest = false; // This indicates the presence of the token even if empty ;)

    if (aFilterOptions.isEmpty())
        return;
    const std::u16string_view aDetect = u"DETECT";
    sal_Int32 nPos = 0;

    // Get first three tokens as they are the only tokens that affect detection.
    aSeps = o3tl::getToken(aOrigFilterOpts, 0, ',', nPos);
    bDelimiter = (nPos >= 0);
    if (bDelimiter)
        aDelimiter = o3tl::getToken(aOrigFilterOpts, 0, ',', nPos);
    bCharSet = (nPos >= 0);
    if (bCharSet)
        aCharSet = o3tl::getToken(aOrigFilterOpts, 0, ',', nPos);
    bRest = (nPos >= 0);
    if (bRest)
        aRest = std::basic_string_view<sal_Unicode>(aOrigFilterOpts.getStr() + nPos, aOrigFilterOpts.getLength() - nPos);

    // Detect charset
    if (aCharSet == aDetect)
    {
        SvStreamEndian endian;
        DetectCharSet(stream, eCharSet, endian);
        if (eCharSet == RTL_TEXTENCODING_UNICODE)
            stream.SetEndian(endian);
    }
    else if (!aCharSet.empty())
        eCharSet = o3tl::toInt32(aCharSet);


    //Detect separators
    if (aSeps == aDetect)
    {
        aFilterOptions = "";
        OUString separators;
        DetectCsvSeparators(stream, eCharSet, separators, static_cast<sal_Unicode>(o3tl::toInt32(aDelimiter)));

        sal_Int32 nLen = separators.getLength();
        for (sal_Int32 nSep = 0; nSep < nLen; nSep ++)
        {
            if (nSep)
                aFilterOptions += "/";
            aFilterOptions += OUString::number(separators[nSep]);
        }
    }
    else
        // For now keep the provided values.
        aFilterOptions = aSeps;

    OUStringChar cComma = u',';
    if (bDelimiter)
        aFilterOptions += cComma + aDelimiter;
    if (bCharSet)
        aFilterOptions += cComma + (aCharSet == aDetect ? OUString::number(eCharSet) : aCharSet);
    if (bRest)
        aFilterOptions += cComma + aRest;
}

void SfxObjectShell::DetectFilterOptions(SfxMedium* pMedium)
{
    std::shared_ptr<const SfxFilter> pFilter = pMedium->GetFilter();
    SfxItemSet& rSet = pMedium->GetItemSet();
    const SfxStringItem* pOptions = rSet.GetItem(SID_FILE_FILTEROPTIONS, false);

    // Skip if filter options are missing
    if (!pFilter || !pOptions)
        return;

    if (pFilter->GetName() == "Text - txt - csv (StarCalc)")
    {
        css::uno::Reference< css::io::XInputStream > xInputStream = pMedium->GetInputStream();
        if (!xInputStream.is())
            return;
        std::unique_ptr<SvStream> pInStream = utl::UcbStreamHelper::CreateStream(xInputStream);
        if (!pInStream)
            return;

        OUString aFilterOptions = pOptions->GetValue();
        DetectCsvFilterOptions(*pInStream, aFilterOptions);
        rSet.Put(SfxStringItem(SID_FILE_FILTEROPTIONS, aFilterOptions));
    }
}

ErrCode SfxObjectShell::HandleFilter( SfxMedium* pMedium, SfxObjectShell const * pDoc )
{
    ErrCode nError = ERRCODE_NONE;
    SfxItemSet& rSet = pMedium->GetItemSet();
    const SfxStringItem* pOptions = rSet.GetItem(SID_FILE_FILTEROPTIONS, false);
    const SfxUnoAnyItem* pData = rSet.GetItem(SID_FILTER_DATA, false);
    const bool bTiledRendering = comphelper::LibreOfficeKit::isActive();

    // Process earlier as the input could contain express detection instructions.
    // This is relevant for "automatic" use case. For interactive use case the
    // FilterOptions should not be detected here (the detection is done before entering
    // interactive state). For now this is focused on CSV files.
    DetectFilterOptions(pMedium);

    if ( !pData && (bTiledRendering || !pOptions) )
    {
        css::uno::Reference< XMultiServiceFactory > xServiceManager = ::comphelper::getProcessServiceFactory();
        css::uno::Reference< XNameAccess > xFilterCFG;
        if( xServiceManager.is() )
        {
            xFilterCFG.set( xServiceManager->createInstance(u"com.sun.star.document.FilterFactory"_ustr),
                            UNO_QUERY );
        }

        if( xFilterCFG.is() )
        {
            try {
                bool bAbort = false;
                std::shared_ptr<const SfxFilter> pFilter = pMedium->GetFilter();
                Sequence < PropertyValue > aProps;
                Any aAny = xFilterCFG->getByName( pFilter->GetName() );
                if ( aAny >>= aProps )
                {
                    auto pProp = std::find_if(std::cbegin(aProps), std::cend(aProps),
                        [](const PropertyValue& rProp) { return rProp.Name == "UIComponent"; });
                    if (pProp != std::cend(aProps))
                    {
                        OUString aServiceName;
                        pProp->Value >>= aServiceName;
                        if( !aServiceName.isEmpty() )
                        {
                            css::uno::Reference< XInteractionHandler > rHandler = pMedium->GetInteractionHandler();
                            if( rHandler.is() )
                            {
                                // we need some properties in the media descriptor, so we have to make sure that they are in
                                Any aStreamAny;
                                aStreamAny <<= pMedium->GetInputStream();
                                if ( rSet.GetItemState( SID_INPUTSTREAM ) < SfxItemState::SET )
                                    rSet.Put( SfxUnoAnyItem( SID_INPUTSTREAM, aStreamAny ) );
                                if ( rSet.GetItemState( SID_FILE_NAME ) < SfxItemState::SET )
                                    rSet.Put( SfxStringItem( SID_FILE_NAME, pMedium->GetName() ) );
                                if ( rSet.GetItemState( SID_FILTER_NAME ) < SfxItemState::SET )
                                    rSet.Put( SfxStringItem( SID_FILTER_NAME, pFilter->GetName() ) );

                                Sequence< PropertyValue > rProperties;
                                TransformItems( SID_OPENDOC, rSet, rProperties );
                                rtl::Reference<RequestFilterOptions> pFORequest = new RequestFilterOptions( pDoc->GetModel(), rProperties );

                                rHandler->handle( pFORequest );

                                if ( !pFORequest->isAbort() )
                                {
                                        SfxAllItemSet aNewParams( pDoc->GetPool() );
                                        TransformParameters( SID_OPENDOC,
                                                        pFORequest->getFilterOptions(),
                                                        aNewParams );

                                        const SfxStringItem* pFilterOptions = aNewParams.GetItem<SfxStringItem>(SID_FILE_FILTEROPTIONS, false);
                                        if ( pFilterOptions )
                                            rSet.Put( *pFilterOptions );

                                        const SfxUnoAnyItem* pFilterData = aNewParams.GetItem<SfxUnoAnyItem>(SID_FILTER_DATA, false);
                                        if ( pFilterData )
                                            rSet.Put( *pFilterData );
                                }
                                else
                                    bAbort = true;
                            }
                        }
                    }
                }

                if( bAbort )
                {
                    // filter options were not entered
                    nError = ERRCODE_ABORT;
                }
            }
            catch( NoSuchElementException& )
            {
                // the filter name is unknown
                nError = ERRCODE_IO_INVALIDPARAMETER;
            }
            catch( Exception& )
            {
                nError = ERRCODE_ABORT;
            }
        }
    }

    return nError;
}


bool SfxObjectShell::IsOwnStorageFormat(const SfxMedium &rMedium)
{
    return !rMedium.GetFilter() || // Embedded
           ( rMedium.GetFilter()->IsOwnFormat() &&
             rMedium.GetFilter()->UsesStorage() &&
             rMedium.GetFilter()->GetVersion() >= SOFFICE_FILEFORMAT_60 );
}


bool SfxObjectShell::IsPackageStorageFormat_Impl(const SfxMedium &rMedium)
{
    return !rMedium.GetFilter() || // Embedded
           ( rMedium.GetFilter()->UsesStorage() &&
             rMedium.GetFilter()->GetVersion() >= SOFFICE_FILEFORMAT_60 );
}


bool SfxObjectShell::DoSave()
// DoSave is only invoked for OLE. Save your own documents in the SFX through
// DoSave_Impl order to allow for the creation of backups.
// Save in your own format again.
{
    bool bOk = false ;
    {
        ModifyBlocker_Impl aBlock( this );

        pImpl->bIsSaving = true;

        if (IsOwnStorageFormat(*GetMedium()))
        {
            SvtSaveOptions::ODFSaneDefaultVersion nDefVersion = SvtSaveOptions::ODFSVER_013;
            if (!comphelper::IsFuzzing())
            {
                nDefVersion = GetODFSaneDefaultVersion();
            }
            uno::Reference<beans::XPropertySet> const xProps(GetMedium()->GetStorage(), uno::UNO_QUERY);
            assert(xProps.is());
            if (nDefVersion >= SvtSaveOptions::ODFSVER_012) // property exists only since ODF 1.2
            {
                try // tdf#134582 set Version on embedded objects as they
                {   // could have been loaded with a different/old version
                    xProps->setPropertyValue(u"Version"_ustr, getODFVersionAny(nDefVersion));
                }
                catch (uno::Exception&)
                {
                    TOOLS_WARN_EXCEPTION("sfx.doc", "SfxObjectShell::DoSave");
                }
            }
        }

        if ( IsPackageStorageFormat_Impl( *GetMedium() ) )
        {
            GetMedium()->GetStorage(); // sets encryption properties if necessary
            if (GetMedium()->GetErrorCode())
            {
                SetError(ERRCODE_IO_GENERAL);
            }
            else
            {
                bOk = true;
            }
#if HAVE_FEATURE_SCRIPTING
            if ( HasBasic() )
            {
                try
                {
                    // The basic and dialogs related contents are still not able to proceed with save operation ( saveTo only )
                    // so since the document storage is locked a workaround has to be used

                    uno::Reference< embed::XStorage > xTmpStorage = ::comphelper::OStorageHelper::GetTemporaryStorage();
                    DBG_ASSERT( xTmpStorage.is(), "If a storage can not be created an exception must be thrown!\n" );
                    if ( !xTmpStorage.is() )
                        throw uno::RuntimeException();

                    static constexpr OUString aBasicStorageName( u"Basic"_ustr  );
                    static constexpr OUString aDialogsStorageName( u"Dialogs"_ustr  );
                    if ( GetMedium()->GetStorage()->hasByName( aBasicStorageName ) )
                        GetMedium()->GetStorage()->copyElementTo( aBasicStorageName, xTmpStorage, aBasicStorageName );
                    if ( GetMedium()->GetStorage()->hasByName( aDialogsStorageName ) )
                        GetMedium()->GetStorage()->copyElementTo( aDialogsStorageName, xTmpStorage, aDialogsStorageName );

                    GetBasicManager();

                    // disconnect from the current storage
                    pImpl->aBasicManager.setStorage( xTmpStorage );

                    // store to the current storage
                    pImpl->aBasicManager.storeLibrariesToStorage( GetMedium()->GetStorage() );

                    // connect to the current storage back
                    pImpl->aBasicManager.setStorage( GetMedium()->GetStorage() );
                }
                catch( uno::Exception& )
                {
                    SetError(ERRCODE_IO_GENERAL);
                    bOk = false;
                }
            }
#endif
        }

        if (bOk)
            bOk = Save();

        if (bOk)
            bOk = pMedium->Commit();
    }

    return bOk;
}

namespace
{
class LockUIGuard
{
public:
    LockUIGuard(SfxObjectShell const* pDoc)
        : m_pDoc(pDoc)
    {
        Lock_Impl();
    }
    ~LockUIGuard() { Unlock(); }

    void Unlock()
    {
        if (m_bUnlock)
            Lock_Impl();
    }

private:
    void Lock_Impl()
    {
        SfxViewFrame* pFrame = SfxViewFrame::GetFirst(m_pDoc);
        while (pFrame)
        {
            pFrame->GetDispatcher()->Lock(!m_bUnlock);
            pFrame->Enable(m_bUnlock);
            pFrame = SfxViewFrame::GetNext(*pFrame, m_pDoc);
        }
        m_bUnlock = !m_bUnlock;
    }
    SfxObjectShell const* m_pDoc;
    bool m_bUnlock = false;
};
}

static OUString lcl_strip_template(const OUString &aString)
{
    static constexpr OUString sPostfix(u"_template"_ustr);
    OUString sRes(aString);
    if (sRes.endsWith(sPostfix))
        sRes = sRes.copy(0, sRes.getLength() - sPostfix.getLength());
    return sRes;
}

bool SfxObjectShell::SaveTo_Impl
(
     SfxMedium &rMedium, // Medium, in which it will be stored
     const SfxItemSet* pSet
)

/*  [Description]

    Writes the current contents to the medium rMedium. If the target medium is
    no storage, then saving to a temporary storage, or directly if the medium
    is transacted, if we ourselves have opened it, and if we are a server
    either the container a transacted storage provides or created a
    temporary storage by one self.
*/

{
    SAL_INFO( "sfx.doc", "saving \"" << rMedium.GetName() << "\"" );

    UpdateDocInfoForSave();

    ModifyBlocker_Impl aMod(this);
    // tdf#41063, tdf#135244: prevent jumping to cursor at any temporary modification
    auto aViewGuard(LockAllViews());

    uno::Reference<uno::XComponentContext> const& xContext(
        ::comphelper::getProcessComponentContext());

    std::shared_ptr<const SfxFilter> pFilter = rMedium.GetFilter();
    if ( !pFilter )
    {
        // if no filter was set, use the default filter
        // this should be changed in the feature, it should be an error!
        SAL_WARN( "sfx.doc","No filter set!");
        pFilter = GetFactory().GetFilterContainer()->GetAnyFilter( SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT );
        rMedium.SetFilter(pFilter);
    }

    bool bStorageBasedSource = IsPackageStorageFormat_Impl( *pMedium );
    bool bStorageBasedTarget = IsPackageStorageFormat_Impl( rMedium );
    bool bOwnSource = IsOwnStorageFormat( *pMedium );
    bool bOwnTarget = IsOwnStorageFormat( rMedium );

    // Examine target format to determine whether to query if any password
    // protected libraries exceed the size we can handler
    if ( bOwnTarget && !QuerySaveSizeExceededModules_Impl( rMedium.GetInteractionHandler() ) )
    {
        SetError(ERRCODE_IO_ABORT);
        return false;
    }

    SvtSaveOptions::ODFSaneDefaultVersion nVersion(SvtSaveOptions::ODFSVER_LATEST_EXTENDED);
    if (bOwnTarget && !comphelper::IsFuzzing())
    {
        nVersion = GetODFSaneDefaultVersion();
    }

    bool bNeedsDisconnectionOnFail = false;

    bool bStoreToSameLocation = false;

    // the detection whether the script is changed should be done before saving
    bool bTryToPreserveScriptSignature = false;
    // no way to detect whether a filter is oasis format, have to wait for saving process
    bool bNoPreserveForOasis = false;
    if ( bOwnSource && bOwnTarget
      && ( pImpl->nScriptingSignatureState == SignatureState::OK
        || pImpl->nScriptingSignatureState == SignatureState::NOTVALIDATED
        || pImpl->nScriptingSignatureState == SignatureState::INVALID ) )
    {
        // the checking of the library modified state iterates over the libraries, should be done only when required
        // currently the check is commented out since it is broken, we have to check the signature every time we save
        // TODO/LATER: let isAnyContainerModified() work!
        bTryToPreserveScriptSignature = true; // !pImpl->pBasicManager->isAnyContainerModified();
        if ( bTryToPreserveScriptSignature )
        {
            // check that the storage format stays the same

            OUString aODFVersion;
            try
            {
                uno::Reference < beans::XPropertySet > xPropSet( GetStorage(), uno::UNO_QUERY );
                if (xPropSet)
                    xPropSet->getPropertyValue(u"Version"_ustr) >>= aODFVersion;
            }
            catch( uno::Exception& )
            {}

            // preserve only if the same filter has been used
            // for templates, strip the _template from the filter name for comparison
            const OUString aMediumFilter = lcl_strip_template(pMedium->GetFilter()->GetFilterName());
            bTryToPreserveScriptSignature = pMedium->GetFilter() && pFilter && aMediumFilter == lcl_strip_template(pFilter->GetFilterName());

            // signatures were specified in ODF 1.2 but were used since much longer.
            // LO will still correctly validate an old style signature on an ODF 1.2
            // document, but technically this is not correct, so this prevents old
            // signatures to be copied over to a version 1.2 document
            bNoPreserveForOasis = (
                                   (0 <= aODFVersion.compareTo(ODFVER_012_TEXT) && nVersion < SvtSaveOptions::ODFSVER_012) ||
                                   (aODFVersion.isEmpty() && nVersion >= SvtSaveOptions::ODFSVER_012)
                                  );
        }
    }

    rtl::Reference< comphelper::UNOMemoryStream > xODFDecryptedInnerPackageStream;
    uno::Reference<embed::XStorage> xODFDecryptedInnerPackage;
    uno::Sequence<beans::NamedValue> aEncryptionData;
    if (GetEncryptionData_Impl(&rMedium.GetItemSet(), aEncryptionData))
    {
        assert(aEncryptionData.getLength() != 0);
        if (bOwnTarget && ::sfx2::UseODFWholesomeEncryption(nVersion))
        {
            // when embedded objects are stored here, it should be called from
            // this function for the root document and encryption data was cleared
            assert(GetCreateMode() != SfxObjectCreateMode::EMBEDDED);
            // clear now to store inner package (+ embedded objects) unencrypted
            rMedium.GetItemSet().ClearItem(SID_ENCRYPTIONDATA);
            rMedium.GetItemSet().ClearItem(SID_PASSWORD);
            xODFDecryptedInnerPackageStream = new comphelper::UNOMemoryStream();
            xODFDecryptedInnerPackage = ::comphelper::OStorageHelper::GetStorageOfFormatFromStream(
                PACKAGE_STORAGE_FORMAT_STRING, xODFDecryptedInnerPackageStream,
                css::embed::ElementModes::WRITE, xContext, false);
            assert(xODFDecryptedInnerPackage.is());
        }
    }

    bool isStreamAndInputStreamCleared(false);
    // use UCB for case sensitive/insensitive file name comparison
    if ( !pMedium->GetName().equalsIgnoreAsciiCase("private:stream")
      && !rMedium.GetName().equalsIgnoreAsciiCase("private:stream")
      && ::utl::UCBContentHelper::EqualURLs( pMedium->GetName(), rMedium.GetName() ) )
    {
        // Do not unlock the file during saving.
        // need to modify this for WebDAV if this method is called outside of
        // the process of saving a file
        pMedium->DisableUnlockWebDAV();
        bStoreToSameLocation = true;

        if ( pMedium->DocNeedsFileDateCheck() )
        {
            rMedium.CheckFileDate( pMedium->GetInitFileDate( false ) );
            if (rMedium.GetErrorCode() == ERRCODE_ABORT)
            {
                // if user cancels the save, exit early to avoid resetting SfxMedium values that
                // would cause an invalid subsequent filedate check
                return false;
            }
        }

        // before we overwrite the original file, we will make a backup if there is a demand for that
        // if the backup is not created here it will be created internally and will be removed in case of successful saving
        const bool bDoBackup = officecfg::Office::Common::Save::Document::CreateBackup::get() && !comphelper::LibreOfficeKit::isActive();
        if ( bDoBackup )
        {
            rMedium.DoBackup_Impl(/*bForceUsingBackupPath=*/false);
            if ( rMedium.GetErrorIgnoreWarning() )
            {
                SetError(rMedium.GetErrorCode());
                rMedium.ResetError();
            }
        }

        if ( bStorageBasedSource && bStorageBasedTarget )
        {
            // The active storage must be switched. The simple saving is not enough.
            // The problem is that the target medium contains target MediaDescriptor.

                // In future the switch of the persistence could be done on stream level:
                // a new wrapper service will be implemented that allows to exchange
                // persistence on the fly. So the real persistence will be set
                // to that stream only after successful commit of the storage.
                // TODO/LATER:
                // create wrapper stream based on the URL
                // create a new storage based on this stream
                // store to this new storage
                // commit the new storage
                // call saveCompleted based with this new storage ( get rid of old storage and "frees" URL )
                // commit the wrapper stream ( the stream will connect the URL only on commit, after that it will hold it )
                // if the last step is failed the stream should stay to be transacted and should be committed on any flush
                // so we can forget the stream in any way and the next storage commit will flush it

            bNeedsDisconnectionOnFail = DisconnectStorage_Impl(
                *pMedium, rMedium );
            if ( bNeedsDisconnectionOnFail
              || ConnectTmpStorage_Impl( pMedium->GetStorage(), pMedium ) )
            {
                pMedium->CloseAndRelease();
                isStreamAndInputStreamCleared = true;

                // TODO/LATER: for now the medium must be closed since it can already contain streams from old medium
                //             in future those streams should not be copied in case a valid target url is provided,
                //             if the url is not provided ( means the document is based on a stream ) this code is not
                //             reachable.
                rMedium.CloseAndRelease();
                rMedium.SetHasEmbeddedObjects(GetEmbeddedObjectContainer().HasEmbeddedObjects());
                if (xODFDecryptedInnerPackageStream.is())
                {
                    assert(!rMedium.GetItemSet().GetItem(SID_STREAM));
                    rMedium.SetInnerStorage_Impl(xODFDecryptedInnerPackage);
                }
                else
                {
                    rMedium.GetOutputStorage();
                }
                rMedium.SetHasEmbeddedObjects(false);
            }
        }
        else if ( !bStorageBasedSource && !bStorageBasedTarget )
        {
            // the source and the target formats are alien
            // just disconnect the stream from the source format
            // so that the target medium can use it

            pMedium->CloseAndRelease();
            rMedium.CloseAndRelease();
            isStreamAndInputStreamCleared = true;
            rMedium.CreateTempFileNoCopy();
            rMedium.GetOutStream();
        }
        else if ( !bStorageBasedSource && bStorageBasedTarget )
        {
            // the source format is an alien one but the target
            // format is an own one so just disconnect the source
            // medium

            pMedium->CloseAndRelease();
            rMedium.CloseAndRelease();
            isStreamAndInputStreamCleared = true;
            if (xODFDecryptedInnerPackageStream.is())
            {
                assert(!rMedium.GetItemSet().GetItem(SID_STREAM));
                rMedium.SetInnerStorage_Impl(xODFDecryptedInnerPackage);
            }
            else
            {
                rMedium.GetOutputStorage();
            }
        }
        else // means if ( bStorageBasedSource && !bStorageBasedTarget )
        {
            // the source format is an own one but the target is
            // an alien format, just connect the source to temporary
            // storage

            bNeedsDisconnectionOnFail = DisconnectStorage_Impl(
                *pMedium, rMedium );
            if ( bNeedsDisconnectionOnFail
              || ConnectTmpStorage_Impl( pMedium->GetStorage(), pMedium ) )
            {
                pMedium->CloseAndRelease();
                rMedium.CloseAndRelease();
                isStreamAndInputStreamCleared = true;
                rMedium.CreateTempFileNoCopy();
                rMedium.GetOutStream();
            }
        }
        pMedium->DisableUnlockWebDAV(false);
    }
    else
    {
        // This is SaveAs or export action, prepare the target medium
        // the alien filters still might write directly to the file, that is of course a bug,
        // but for now the framework has to be ready for it
        // TODO/LATER: let the medium be prepared for alien formats as well

        rMedium.CloseAndRelease();
        isStreamAndInputStreamCleared = true;
        if ( bStorageBasedTarget )
        {
            rMedium.SetHasEmbeddedObjects(GetEmbeddedObjectContainer().HasEmbeddedObjects());
            if (xODFDecryptedInnerPackageStream.is())
            {
                assert(!rMedium.GetItemSet().GetItem(SID_STREAM));
                // this should set only xStorage, all of the streams remain null
                rMedium.SetInnerStorage_Impl(xODFDecryptedInnerPackage);
            }
            else
            {
                rMedium.GetOutputStorage();
            }
            rMedium.SetHasEmbeddedObjects(false);
        }
    }

    // TODO/LATER: error handling
    if( rMedium.GetErrorCode() || pMedium->GetErrorCode() || GetErrorCode() )
    {
        SAL_WARN("sfx.doc", "SfxObjectShell::SaveTo_Impl: "
                 " very early error return " << rMedium.GetErrorCode() << " "
                 << pMedium->GetErrorCode() << " " << GetErrorCode());
        return false;
    }

    // these have been cleared on all paths that don't take above error return
    assert(isStreamAndInputStreamCleared); (void) isStreamAndInputStreamCleared;

    rMedium.LockOrigFileOnDemand( false, false );

    if ( bStorageBasedTarget )
    {
        if ( rMedium.GetErrorCode() )
            return false;

        // If the filter is a "cross export" filter ( f.e. a filter for exporting an impress document from
        // a draw document ), the ClassId of the destination storage is different from the ClassId of this
        // document. It can be retrieved from the default filter for the desired target format
        SotClipboardFormatId nFormat = rMedium.GetFilter()->GetFormat();
        SfxFilterMatcher& rMatcher = SfxGetpApp()->GetFilterMatcher();
        std::shared_ptr<const SfxFilter> pFilt = rMatcher.GetFilter4ClipBoardId( nFormat );
        if ( pFilt )
        {
            if ( pFilt->GetServiceName() != rMedium.GetFilter()->GetServiceName() )
            {
                datatransfer::DataFlavor aDataFlavor;
                SotExchange::GetFormatDataFlavor( nFormat, aDataFlavor );

                try
                {
                    uno::Reference< beans::XPropertySet > xProps( rMedium.GetStorage(), uno::UNO_QUERY );
                    if (xProps)
                        xProps->setPropertyValue(u"MediaType"_ustr,
                                                uno::Any( aDataFlavor.MimeType ) );
                }
                catch( uno::Exception& )
                {
                }
            }
        }
    }

    // TODO/LATER: error handling
    if( rMedium.GetErrorCode() || pMedium->GetErrorCode() || GetErrorCode() )
        return false;

    bool bOldStat = pImpl->bForbidReload;
    pImpl->bForbidReload = true;

    // lock user interface while saving the document
    LockUIGuard aLockUIGuard(this);

    bool bCopyTo = false;
    SfxItemSet& rMedSet = rMedium.GetItemSet();
    const SfxBoolItem* pSaveToItem = rMedSet.GetItem(SID_SAVETO, false);
    bCopyTo =   GetCreateMode() == SfxObjectCreateMode::EMBEDDED ||
                (pSaveToItem && pSaveToItem->GetValue());

    bool bOk = true;
    // TODO/LATER: get rid of bOk
    if (bOwnTarget && pFilter && !(pFilter->GetFilterFlags() & SfxFilterFlags::STARONEFILTER))
    {
        uno::Reference< embed::XStorage > xMedStorage = rMedium.GetStorage();
        if (!xMedStorage.is() || rMedium.GetErrorCode())
        {
            // no saving without storage
            pImpl->bForbidReload = bOldStat;
            return false;
        }

        // transfer password from the parameters to the storage
        bool const bPasswdProvided(aEncryptionData.getLength() != 0);
        pFilter = rMedium.GetFilter();

        const SfxStringItem *pVersionItem = !rMedium.IsInCheckIn()? SfxItemSet::GetItem<SfxStringItem>(pSet, SID_DOCINFO_COMMENTS, false): nullptr;
        OUString aTmpVersionURL;

        if ( bOk )
        {
            bOk = false;
            // currently the case that the storage is the same should be impossible
            if ( xMedStorage == GetStorage() )
            {
                OSL_ENSURE( !pVersionItem, "This scenario is impossible currently!" );
                // usual save procedure
                bOk = Save();
            }
            else
            {
                // save to target
                bOk = SaveAsOwnFormat( rMedium );
                if ( bOk && pVersionItem )
                {
                    aTmpVersionURL = CreateTempCopyOfStorage_Impl( xMedStorage );
                    bOk =  !aTmpVersionURL.isEmpty();
                }
            }
        }

        //fdo#61320: only store thumbnail image if the corresponding option is enabled in the configuration
        if ( bOk && officecfg::Office::Common::Save::Document::GenerateThumbnail::get()
                && GetCreateMode() != SfxObjectCreateMode::EMBEDDED && !bPasswdProvided && IsUseThumbnailSave() )
        {
            // store the thumbnail representation image
            // the thumbnail is not stored in case of encrypted document
            if ( !GenerateAndStoreThumbnail( bPasswdProvided, xMedStorage ) )
            {
                // TODO: error handling
                SAL_WARN( "sfx.doc", "Couldn't store thumbnail representation!" );
            }
        }

        if ( bOk )
        {
            if ( pImpl->bIsSaving || pImpl->bPreserveVersions )
            {
                try
                {
                    const Sequence < util::RevisionTag > aVersions = rMedium.GetVersionList();
                    if ( aVersions.hasElements() )
                    {
                        // copy the version streams
                        static constexpr OUString aVersionsName( u"Versions"_ustr  );
                        uno::Reference< embed::XStorage > xNewVerStor = xMedStorage->openStorageElement(
                                                        aVersionsName,
                                                        embed::ElementModes::READWRITE );
                        uno::Reference< embed::XStorage > xOldVerStor = GetStorage()->openStorageElement(
                                                        aVersionsName,
                                                        embed::ElementModes::READ );
                        if ( !xNewVerStor.is() || !xOldVerStor.is() )
                            throw uno::RuntimeException();

                        for ( const auto& rVersion : aVersions )
                        {
                            if ( xOldVerStor->hasByName( rVersion.Identifier ) )
                                xOldVerStor->copyElementTo( rVersion.Identifier, xNewVerStor, rVersion.Identifier );
                        }

                        uno::Reference< embed::XTransactedObject > xTransact( xNewVerStor, uno::UNO_QUERY );
                        if ( xTransact.is() )
                            xTransact->commit();
                    }
                }
                catch( uno::Exception& )
                {
                    SAL_WARN( "sfx.doc", "Couldn't copy versions!" );
                    bOk = false;
                    // TODO/LATER: a specific error could be set
                }
            }

            if ( bOk && pVersionItem && !rMedium.IsInCheckIn() )
            {
                // store a version also
                const SfxStringItem *pAuthorItem = SfxItemSet::GetItem<SfxStringItem>(pSet, SID_DOCINFO_AUTHOR, false);

                // version comment
                util::RevisionTag aInfo;
                aInfo.Comment = pVersionItem->GetValue();

                // version author
                if ( pAuthorItem )
                    aInfo.Author = pAuthorItem->GetValue();
                else
                    // if not transferred as a parameter, get it from user settings
                    aInfo.Author = SvtUserOptions().GetFullName();

                DateTime aTime( DateTime::SYSTEM );
                aInfo.TimeStamp.Day = aTime.GetDay();
                aInfo.TimeStamp.Month = aTime.GetMonth();
                aInfo.TimeStamp.Year = aTime.GetYear();
                aInfo.TimeStamp.Hours = aTime.GetHour();
                aInfo.TimeStamp.Minutes = aTime.GetMin();
                aInfo.TimeStamp.Seconds = aTime.GetSec();

                // add new version information into the versionlist and save the versionlist
                // the version list must have been transferred from the "old" medium before
                rMedium.AddVersion_Impl(aInfo);
                rMedium.SaveVersionList_Impl();
                bOk = PutURLContentsToVersionStream_Impl(aTmpVersionURL, xMedStorage,
                                                         aInfo.Identifier);
            }
            else if ( bOk && ( pImpl->bIsSaving || pImpl->bPreserveVersions ) )
            {
                rMedium.SaveVersionList_Impl();
            }
        }

        if ( !aTmpVersionURL.isEmpty() )
            ::utl::UCBContentHelper::Kill( aTmpVersionURL );
    }
    else
    {
        // it's a "SaveAs" in an alien format
        if ( rMedium.GetFilter() && ( rMedium.GetFilter()->GetFilterFlags() & SfxFilterFlags::STARONEFILTER ) )
            bOk = ExportTo( rMedium );
        else
            bOk = ConvertTo( rMedium );

        // after saving the document, the temporary object storage must be updated
        // if the old object storage was not a temporary one, it will be updated also, because it will be used
        // as a source for copying the objects into the new temporary storage that will be created below
        // updating means: all child objects must be stored into it
        // ( same as on loading, where these objects are copied to the temporary storage )
        // but don't commit these changes, because in the case when the old object storage is not a temporary one,
        // all changes will be written into the original file !

        if( bOk && !bCopyTo )
            // we also don't touch any graphical replacements here
            SaveChildren( true );
    }

    if ( bOk )
    {
        uno::Any mediaType;
        if (xODFDecryptedInnerPackageStream.is())
        {   // before the signature copy closes it
            mediaType = uno::Reference<beans::XPropertySet>(xODFDecryptedInnerPackage,
                uno::UNO_QUERY_THROW)->getPropertyValue(u"MediaType"_ustr);
        }

        // if ODF version of oasis format changes on saving the signature should not be preserved
        if ( bTryToPreserveScriptSignature && bNoPreserveForOasis )
            bTryToPreserveScriptSignature = ( SotStorage::GetVersion( rMedium.GetStorage() ) == SOFFICE_FILEFORMAT_60 );

        uno::Reference< security::XDocumentDigitalSignatures > xDDSigns;
        if (bTryToPreserveScriptSignature)
        {
            // if the scripting code was not changed and it is signed the signature should be preserved
            // unfortunately at this point we have only information whether the basic code has changed or not
            // so the only way is to check the signature if the basic was not changed
            try
            {
                // get the ODF version of the new medium
                OUString aVersion;
                try
                {
                    uno::Reference < beans::XPropertySet > xPropSet( rMedium.GetStorage(), uno::UNO_QUERY );
                    if (xPropSet)
                        xPropSet->getPropertyValue(u"Version"_ustr) >>= aVersion;
                }
                catch( uno::Exception& )
                {
                }

                xDDSigns = security::DocumentDigitalSignatures::createWithVersion(comphelper::getProcessComponentContext(), aVersion);

                const OUString aScriptSignName = xDDSigns->getScriptingContentSignatureDefaultStreamName();

                if ( !aScriptSignName.isEmpty() )
                {
                    // target medium is still not committed, it should not be closed
                    // commit the package storage and close it, but leave the streams open
                    rMedium.StorageCommit_Impl();
                    rMedium.CloseStorage();

                    // signature must use Zip storage, not Package storage
                    uno::Reference<embed::XStorage> const xReadOrig(
                            pMedium->GetScriptingStorageToSign_Impl());
                    uno::Reference<embed::XStorage> xTarget;
                    if (xODFDecryptedInnerPackageStream.is())
                    {
                        xTarget = ::comphelper::OStorageHelper::GetStorageOfFormatFromStream(
                            ZIP_STORAGE_FORMAT_STRING, xODFDecryptedInnerPackageStream);
                    }
                    else
                    {
                        xTarget = rMedium.GetZipStorageToSign_Impl(false);
                    }

                    if ( !xReadOrig.is() )
                        throw uno::RuntimeException();
                    uno::Reference< embed::XStorage > xMetaInf = xReadOrig->openStorageElement(
                                u"META-INF"_ustr,
                                embed::ElementModes::READ );

                    if ( !xTarget.is() )
                        throw uno::RuntimeException();
                    uno::Reference< embed::XStorage > xTargetMetaInf = xTarget->openStorageElement(
                                u"META-INF"_ustr,
                                embed::ElementModes::READWRITE );

                    if ( xMetaInf.is() && xTargetMetaInf.is() )
                    {
                        xMetaInf->copyElementTo( aScriptSignName, xTargetMetaInf, aScriptSignName );

                        uno::Reference< embed::XTransactedObject > xTransact( xTargetMetaInf, uno::UNO_QUERY );
                        if ( xTransact.is() )
                            xTransact->commit();

                        xTargetMetaInf->dispose();

                        // now check the copied signature
                        uno::Sequence< security::DocumentSignatureInformation > aInfos =
                            xDDSigns->verifyScriptingContentSignatures( xTarget,
                                                                        uno::Reference< io::XInputStream >() );
                        SignatureState nState = DocumentSignatures::getSignatureState(aInfos);
                        if ( nState == SignatureState::OK || nState == SignatureState::NOTVALIDATED
                            || nState == SignatureState::PARTIAL_OK)
                        {
                            rMedium.SetCachedSignatureState_Impl( nState );

                            // commit the ZipStorage from target medium
                            xTransact.set( xTarget, uno::UNO_QUERY );
                            if ( xTransact.is() )
                                xTransact->commit();
                            if (xODFDecryptedInnerPackageStream.is())
                            {   // recreate, to have it with copied sig
                                xODFDecryptedInnerPackage = ::comphelper::OStorageHelper::GetStorageOfFormatFromStream(
                                    PACKAGE_STORAGE_FORMAT_STRING, xODFDecryptedInnerPackageStream,
                                    css::embed::ElementModes::WRITE, xContext, false);
                            }
                        }
                        else
                        {
                            // it should not happen, the copies signature is invalid!
                            // throw the changes away
                            SAL_WARN( "sfx.doc", "An invalid signature was copied!" );
                        }
                    }
                }
            }
            catch( uno::Exception& )
            {
            }

            rMedium.CloseZipStorage_Impl();
        }

        if (xODFDecryptedInnerPackageStream.is())
        {
            rMedium.StorageCommit_Impl();
            // prevent dispose as inner storage will be needed later
            assert(!rMedium.WillDisposeStorageOnClose_Impl());
            rMedium.CloseStorage();
            // restore encryption for outer package, note: disable for debugging
            rMedium.GetItemSet().Put(SfxUnoAnyItem(SID_ENCRYPTIONDATA, uno::Any(aEncryptionData)));
            assert(xODFDecryptedInnerPackageStream.is());
            // now create the outer storage
            uno::Reference<embed::XStorage> const xOuterStorage(rMedium.GetOutputStorage());
            assert(xOuterStorage.is());
            assert(!rMedium.GetErrorCode());
            // the outer storage needs the same properties as the inner one
            bool const isTemplate{rMedium.GetFilter()->IsOwnTemplateFormat()};
            SetupStorage(xOuterStorage, SOFFICE_FILEFORMAT_CURRENT, isTemplate);

            uno::Reference<io::XStream> const xEncryptedInnerPackage =
                xOuterStorage->openStreamElement(
                    u"encrypted-package"_ustr, embed::ElementModes::WRITE);
            uno::Reference<beans::XPropertySet> const xEncryptedPackageProps(
                    xEncryptedInnerPackage, uno::UNO_QUERY_THROW);
            xEncryptedPackageProps->setPropertyValue(u"MediaType"_ustr, mediaType);

            // encryption: just copy into package stream
            xODFDecryptedInnerPackageStream->seek(0);
            comphelper::OStorageHelper::CopyInputToOutput(
                xODFDecryptedInnerPackageStream->getInputStream(),
                xEncryptedInnerPackage->getOutputStream());
            // rely on Commit() below
        }

        const OUString sName( rMedium.GetName( ) );
        bOk = rMedium.Commit();
        const OUString sNewName( rMedium.GetName( ) );

        if ( sName != sNewName )
            GetMedium( )->SwitchDocumentToFile( sNewName );

        if (xODFDecryptedInnerPackageStream.is())
        {   // set the inner storage on the medium again, after Switch
            rMedium.SetInnerStorage_Impl(xODFDecryptedInnerPackage);
        }

        if ( bOk )
        {
            // if the target medium is an alien format and the "old" medium was an own format and the "old" medium
            // has a name, the object storage must be exchanged, because now we need a new temporary storage
            // as object storage
            if ( !bCopyTo && bStorageBasedSource && !bStorageBasedTarget )
            {
                if ( bStoreToSameLocation )
                {
                    // if the old medium already disconnected from document storage, the storage still must
                    // be switched if backup file is used
                    if ( bNeedsDisconnectionOnFail )
                        ConnectTmpStorage_Impl( pImpl->m_xDocStorage, nullptr );
                }
                else if (!pMedium->GetName().isEmpty()
                  || ( pMedium->HasStorage_Impl() && pMedium->WillDisposeStorageOnClose_Impl() ) )
                {
                    OSL_ENSURE(!pMedium->GetName().isEmpty(), "Fallback is used, the medium without name should not dispose the storage!");
                    // copy storage of old medium to new temporary storage and take this over
                    if( !ConnectTmpStorage_Impl( pMedium->GetStorage(), pMedium ) )
                    {
                        SAL_WARN( "sfx.doc", "Process after storing has failed." );
                        bOk = false;
                    }
                }
            }
        }
        else
        {
            SAL_WARN( "sfx.doc", "Storing has failed." );

            // in case the document storage was connected to backup temporarily it must be disconnected now
            if ( bNeedsDisconnectionOnFail )
                ConnectTmpStorage_Impl( pImpl->m_xDocStorage, nullptr );
        }
    }

    // unlock user interface
    aLockUIGuard.Unlock();
    pImpl->bForbidReload = bOldStat;

    // ucbhelper::Content is unable to do anything useful with a private:stream
    if (bOk && !rMedium.GetName().equalsIgnoreAsciiCase("private:stream"))
    {
        try
        {
            ::ucbhelper::Content aContent( rMedium.GetName(), utl::UCBContentHelper::getDefaultCommandEnvironment(), comphelper::getProcessComponentContext() );
            css::uno::Reference < XPropertySetInfo > xProps = aContent.getProperties();
            if ( xProps.is() )
            {
                static constexpr OUString aAuthor( u"Author"_ustr );
                static constexpr OUString aKeywords( u"Keywords"_ustr );
                static constexpr OUString aSubject( u"Subject"_ustr );

                uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                    GetModel(), uno::UNO_QUERY_THROW);
                uno::Reference<document::XDocumentProperties> xDocProps
                    = xDPS->getDocumentProperties();

                if ( xProps->hasPropertyByName( aAuthor ) )
                {
                    aContent.setPropertyValue( aAuthor, Any(xDocProps->getAuthor()) );
                }
                if ( xProps->hasPropertyByName( aKeywords ) )
                {
                    Any aAny;
                    aAny <<= ::comphelper::string::convertCommaSeparated(
                                xDocProps->getKeywords());
                    aContent.setPropertyValue( aKeywords, aAny );
                }
                if ( xProps->hasPropertyByName( aSubject ) )
                {
                    aContent.setPropertyValue( aSubject, Any(xDocProps->getSubject()) );
                }
            }
        }
        catch( Exception& )
        {
        }
    }

    return bOk;
}

bool SfxObjectShell::DisconnectStorage_Impl( SfxMedium& rSrcMedium, SfxMedium& rTargetMedium )
{
    // this method disconnects the storage from source medium, and attaches it to the backup created by the target medium

    uno::Reference< embed::XStorage > xStorage = rSrcMedium.GetStorage();

    bool bResult = false;
    if ( xStorage == pImpl->m_xDocStorage )
    {
        try
        {
            uno::Reference< embed::XOptimizedStorage > xOptStorage( xStorage, uno::UNO_QUERY_THROW );
            const OUString aBackupURL = rTargetMedium.GetBackup_Impl();
            if ( aBackupURL.isEmpty() )
            {
                // the backup could not be created, try to disconnect the storage and close the source SfxMedium
                // in this case the optimization is not possible, connect storage to a temporary file
                rTargetMedium.ResetError();
                xOptStorage->writeAndAttachToStream( uno::Reference< io::XStream >() );
                rSrcMedium.CanDisposeStorage_Impl( false );
                rSrcMedium.Close();

                // now try to create the backup
                rTargetMedium.GetBackup_Impl();
            }
            else
            {
                // the following call will only compare stream sizes
                // TODO/LATER: this is a very risky part, since if the URL contents are different from the storage
                // contents, the storage will be broken
                xOptStorage->attachToURL( aBackupURL, true );

                // the storage is successfully attached to backup, thus it is owned by the document not by the medium
                rSrcMedium.CanDisposeStorage_Impl( false );
                bResult = true;
            }
        }
        catch ( uno::Exception& )
        {}
    }
    return bResult;
}


bool SfxObjectShell::ConnectTmpStorage_Impl(
    const uno::Reference< embed::XStorage >& xStorage,
    SfxMedium* pMediumArg )

/*   [Description]

     If the application operates on a temporary storage, then it may not take
     the temporary storage from the SaveCompleted. Therefore the new storage
     is connected already here in this case and SaveCompleted then does nothing.
*/

{
    bool bResult = false;

    if ( xStorage.is() )
    {
        try
        {
            // the empty argument means that the storage will create temporary stream itself
            uno::Reference< embed::XOptimizedStorage > xOptStorage( xStorage, uno::UNO_QUERY_THROW );
            xOptStorage->writeAndAttachToStream( uno::Reference< io::XStream >() );

            // the storage is successfully disconnected from the original sources, thus the medium must not dispose it
            if ( pMediumArg )
                pMediumArg->CanDisposeStorage_Impl( false );

            bResult = true;
        }
        catch( uno::Exception& )
        {
        }

        // if switching of the storage does not work for any reason ( nonroot storage for example ) use the old method
        if ( !bResult ) try
        {
            uno::Reference< embed::XStorage > xTmpStorage = ::comphelper::OStorageHelper::GetTemporaryStorage();

            DBG_ASSERT( xTmpStorage.is(), "If a storage can not be created an exception must be thrown!\n" );
            if ( !xTmpStorage.is() )
                throw uno::RuntimeException();

            // TODO/LATER: may be it should be done in SwitchPersistence also
            // TODO/LATER: find faster way to copy storage; perhaps sharing with backup?!
            xStorage->copyToStorage( xTmpStorage );
            bResult = SaveCompleted( xTmpStorage );

            if ( bResult )
            {
                pImpl->aBasicManager.setStorage( xTmpStorage );

                if (pImpl->xBasicLibraries)
                    pImpl->xBasicLibraries->setRootStorage( xTmpStorage );
                if (pImpl->xDialogLibraries)
                    pImpl->xDialogLibraries->setRootStorage( xTmpStorage );
            }
        }
        catch( uno::Exception& )
        {}

        if ( !bResult )
        {
            // TODO/LATER: may need error code setting based on exception
            SetError(ERRCODE_IO_GENERAL);
        }
    }
    else if (!GetMedium()->GetFilter()->IsOwnFormat())
        bResult = true;

    return bResult;
}


bool SfxObjectShell::DoSaveObjectAs( SfxMedium& rMedium, bool bCommit )
{
    bool bOk = false;

    ModifyBlocker_Impl aBlock( this );

    uno::Reference < embed::XStorage > xNewStor = rMedium.GetStorage();
    if ( !xNewStor.is() )
        return false;

    uno::Reference < beans::XPropertySet > xPropSet( xNewStor, uno::UNO_QUERY );
    if ( !xPropSet.is() )
        return false;

    Any a = xPropSet->getPropertyValue(u"MediaType"_ustr);
    OUString aMediaType;
    if ( !(a>>=aMediaType) || aMediaType.isEmpty() )
    {
        SAL_WARN( "sfx.doc", "The mediatype must be set already!" );
        SetupStorage( xNewStor, SOFFICE_FILEFORMAT_CURRENT, false );
    }

    pImpl->bIsSaving = false;
    bOk = SaveAsOwnFormat( rMedium );

    if ( bCommit )
    {
        try {
            uno::Reference< embed::XTransactedObject > xTransact( xNewStor, uno::UNO_QUERY_THROW );
            xTransact->commit();
        }
        catch( uno::Exception& )
        {
            SAL_WARN( "sfx.doc", "The storage was not committed on DoSaveAs!" );
        }
    }

    return bOk;
}


// TODO/LATER: may be the call must be removed completely
bool SfxObjectShell::DoSaveAs( SfxMedium& rMedium )
{
    // here only root storages are included, which are stored via temp file
    rMedium.CreateTempFileNoCopy();
    SetError(rMedium.GetErrorCode());
    if ( GetErrorIgnoreWarning() )
        return false;

    // copy version list from "old" medium to target medium, so it can be used on saving
    if ( pImpl->bPreserveVersions )
        rMedium.TransferVersionList_Impl( *pMedium );

    bool bRet = SaveTo_Impl( rMedium, nullptr );
    if ( !bRet )
        SetError(rMedium.GetErrorCode());
    return bRet;
}


bool SfxObjectShell::DoSaveCompleted( SfxMedium* pNewMed, bool bRegisterRecent )
{
    bool bOk = true;
    bool bMedChanged = pNewMed && pNewMed!=pMedium;

    DBG_ASSERT( !pNewMed || pNewMed->GetErrorIgnoreWarning() == ERRCODE_NONE, "DoSaveCompleted: Medium has error!" );

    // delete Medium (and Storage!) after all notifications
    SfxMedium* pOld = pMedium;
    if ( bMedChanged )
    {
        pMedium = pNewMed;
        pMedium->CanDisposeStorage_Impl( true );
    }

    std::shared_ptr<const SfxFilter> pFilter = pMedium ? pMedium->GetFilter() : nullptr;
    if ( pNewMed )
    {
        if( bMedChanged )
        {
            if (!pNewMed->GetName().isEmpty())
                bHasName = true;
            Broadcast( SfxHint(SfxHintId::NameChanged) );
            EnableSetModified(false);
            getDocProperties()->setGenerator(
               ::utl::DocInfoHelper::GetGeneratorString() );
            EnableSetModified();
        }

        uno::Reference< embed::XStorage > xStorage;
        if ( !pFilter || IsPackageStorageFormat_Impl( *pMedium ) )
        {
            uno::Reference < embed::XStorage > xOld = GetStorage();

            // when the package based medium is broken and has no storage or if the storage
            // is the same as the document storage the current document storage should be preserved
            xStorage = pMedium->GetStorage();
            bOk = SaveCompleted( xStorage );
            if ( bOk && xStorage.is() && xOld != xStorage
              && (!pOld || !pOld->HasStorage_Impl() || xOld != pOld->GetStorage() ) )
            {
                // old own storage was not controlled by old Medium -> dispose it
                try {
                    xOld->dispose();
                } catch( uno::Exception& )
                {
                    // the storage is disposed already
                    // can happen during reload scenario when the medium has
                    // disposed it during the closing
                    // will be fixed in one of the next milestones
                }
            }
        }
        else
        {
            if (pImpl->m_bSavingForSigning && pFilter && pFilter->GetSupportsSigning())
                // So that pMedium->pImpl->xStream becomes a non-empty
                // reference, and at the end we attempt locking again in
                // SfxMedium::LockOrigFileOnDemand().
                pMedium->GetMedium_Impl();

            if( pMedium->GetOpenMode() & StreamMode::WRITE )
                pMedium->GetInStream();
            xStorage = GetStorage();
        }

        // TODO/LATER: may be this code will be replaced, but not sure
        // Set storage in document library containers
        pImpl->aBasicManager.setStorage( xStorage );

        if (pImpl->xBasicLibraries)
            pImpl->xBasicLibraries->setRootStorage( xStorage );
        if (pImpl->xDialogLibraries)
            pImpl->xDialogLibraries->setRootStorage( xStorage );
    }
    else
    {
        if( pMedium )
        {
            if( pFilter && !IsPackageStorageFormat_Impl( *pMedium ) && (pMedium->GetOpenMode() & StreamMode::WRITE ))
            {
                pMedium->ReOpen();
                bOk = SaveCompletedChildren();
            }
            else
                bOk = SaveCompleted( nullptr );
        }
        // either Save or ConvertTo
        else
            bOk = SaveCompleted( nullptr );
    }

    if ( bOk && pNewMed )
    {
        if( bMedChanged )
        {
            delete pOld;

            uno::Reference< frame::XModel > xModel = GetModel();
            if ( xModel.is() )
            {
                const OUString& aURL {pNewMed->GetOrigURL()};
                uno::Sequence< beans::PropertyValue > aMediaDescr;
                TransformItems( SID_OPENDOC, pNewMed->GetItemSet(), aMediaDescr );
                try
                {
                    xModel->attachResource( aURL, aMediaDescr );
                }
                catch( uno::Exception& )
                {}
            }

            const SfxBoolItem* pTemplateItem = pMedium->GetItemSet().GetItem(SID_TEMPLATE, false);
            bool bTemplate = pTemplateItem && pTemplateItem->GetValue();

            // before the title regenerated the document must lose the signatures
            pImpl->nDocumentSignatureState = SignatureState::NOSIGNATURES;
            if (!bTemplate)
            {
                pImpl->nScriptingSignatureState = pNewMed->GetCachedSignatureState_Impl();
                OSL_ENSURE( pImpl->nScriptingSignatureState != SignatureState::BROKEN, "The signature must not be broken at this place" );

                // TODO/LATER: in future the medium must control own signature state, not the document
                pNewMed->SetCachedSignatureState_Impl( SignatureState::NOSIGNATURES ); // set the default value back
            }
            else
                pNewMed->SetCachedSignatureState_Impl( pImpl->nScriptingSignatureState );

            // Set new title
            if (!pNewMed->GetName().isEmpty() && SfxObjectCreateMode::EMBEDDED != eCreateMode)
                InvalidateName();
            SetModified(false); // reset only by set medium
            Broadcast( SfxHint(SfxHintId::ModeChanged) );

            // this is the end of the saving process, it is possible that
            // the file was changed
            // between medium commit and this step (attributes change and so on)
            // so get the file date again
            if ( pNewMed->DocNeedsFileDateCheck() )
                pNewMed->GetInitFileDate( true );
        }
    }

    pMedium->ClearBackup_Impl();
    pMedium->LockOrigFileOnDemand( true, false );

    if (bRegisterRecent)
        AddToRecentlyUsedList();

    // Check if this is a Google Drive file that needs uploading
    if (bOk && pMedium)
    {
        OUString sURL = pMedium->GetURLObject().GetMainURL(INetURLObject::DecodeMechanism::NONE);
        sfx2::GoogleDriveSync& rSync = sfx2::GoogleDriveSync::getInstance();
        
        if (rSync.isGoogleDriveFile(sURL))
        {
            SAL_INFO("sfx.doc", "Detected Google Drive file save, marking for upload: " << sURL);
            rSync.markForUpload(sURL);
            
            // Upload the file immediately
            auto fileInfo = rSync.getFileInfo(sURL);
            if (fileInfo && fileInfo->needsUpload)
            {
                // Call static upload method from GoogleDriveFilePicker
                SAL_INFO("sfx.doc", "Uploading Google Drive file: " << fileInfo->fileId);
                
                // Need to include the header for GoogleDriveFilePicker
                // For now, we'll use the upload method from GoogleDriveSync
                rSync.uploadFile(sURL);
            }
        }
    }

    return bOk;
}

void SfxObjectShell::AddToRecentlyUsedList()
{
    INetURLObject aUrl( pMedium->GetOrigURL() );

    if ( aUrl.GetProtocol() == INetProtocol::File )
    {
        std::shared_ptr<const SfxFilter> pOrgFilter = pMedium->GetFilter();
        Application::AddToRecentDocumentList( aUrl.GetURLNoPass( INetURLObject::DecodeMechanism::NONE ),
                                              pOrgFilter ? pOrgFilter->GetMimeType() : OUString(),
                                              pOrgFilter ? pOrgFilter->GetServiceName() : OUString() );
    }
}


bool SfxObjectShell::ConvertFrom
(
    SfxMedium&  /*rMedium*/     /*  <SfxMedium>, which describes the source file
                                    (for example file name, <SfxFilter>,
                                    Open-Modi and so on) */
)

/*  [Description]

    This method is called for loading of documents over all filters which are
    not SfxFilterFlags::OWN or for which no clipboard format has been registered
    (thus no storage format that is used). In other words, with this method
    it is imported.

    Files which are to be opened here should be opened through 'rMedium'
    to guarantee the right open modes. Especially if the format is retained
    (only possible with SfxFilterFlags::SIMULATE or SfxFilterFlags::OWN) file which must
    be opened STREAM_SHARE_DENYWRITE.

    [Return value]

    bool                true
                        The document could be loaded.

                        false
                        The document could not be loaded, an error code
                        received through  <SvMedium::GetError()const>

    [Example]

    bool DocSh::ConvertFrom( SfxMedium &rMedium )
    {
        SvStreamRef xStream = rMedium.GetInStream();
        if( xStream.is() )
        {
            xStream->SetBufferSize(4096);
            *xStream >> ...;

            // Do not call 'rMedium.CloseInStream()'! Keep File locked!
            return ERRCODE_NONE == rMedium.GetError();
        }

        return false;
    }

    [Cross-references]

    <SfxObjectShell::ConvertTo(SfxMedium&)>
    <SfxFilterFlags::REGISTRATION>
*/
{
    return false;
}

bool SfxObjectShell::ImportFrom(SfxMedium& rMedium,
        css::uno::Reference<css::text::XTextRange> const& xInsertPosition)
{
    const OUString aFilterName( rMedium.GetFilter()->GetFilterName() );

    uno::Reference< lang::XMultiServiceFactory >  xMan = ::comphelper::getProcessServiceFactory();
    uno::Reference < lang::XMultiServiceFactory > xFilterFact (
                xMan->createInstance( u"com.sun.star.document.FilterFactory"_ustr ), uno::UNO_QUERY );

    uno::Sequence < beans::PropertyValue > aProps;
    uno::Reference < container::XNameAccess > xFilters ( xFilterFact, uno::UNO_QUERY );
    if ( xFilters->hasByName( aFilterName ) )
    {
        xFilters->getByName( aFilterName ) >>= aProps;
        rMedium.GetItemSet().Put( SfxStringItem( SID_FILTER_NAME, aFilterName ) );
    }

    OUString aFilterImplName;
    auto pProp = std::find_if(std::cbegin(aProps), std::cend(aProps),
        [](const beans::PropertyValue& rFilterProp) { return rFilterProp.Name == "FilterService"; });
    if (pProp != std::cend(aProps))
        pProp->Value >>= aFilterImplName;

    uno::Reference< document::XFilter > xLoader;
    if ( !aFilterImplName.isEmpty() )
    {
        try
        {
            xLoader.set( xFilterFact->createInstanceWithArguments( aFilterName, uno::Sequence < uno::Any >() ), uno::UNO_QUERY );
        }
        catch(const uno::Exception&)
        {
            xLoader.clear();
        }
    }
    if ( xLoader.is() )
    {
        // it happens that xLoader does not support xImporter!
        try
        {
            uno::Reference< lang::XComponent >  xComp( GetModel(), uno::UNO_QUERY_THROW );
            uno::Reference< document::XImporter > xImporter( xLoader, uno::UNO_QUERY_THROW );
            xImporter->setTargetDocument( xComp );

            uno::Sequence < beans::PropertyValue > lDescriptor;
            rMedium.GetItemSet().Put( SfxStringItem( SID_FILE_NAME, rMedium.GetName() ) );
            TransformItems( SID_OPENDOC, rMedium.GetItemSet(), lDescriptor );

            css::uno::Sequence < css::beans::PropertyValue > aArgs ( lDescriptor.getLength() );
            css::beans::PropertyValue * pNewValue = aArgs.getArray();
            const css::beans::PropertyValue * pOldValue = lDescriptor.getConstArray();
            static constexpr OUString sInputStream ( u"InputStream"_ustr  );

            bool bHasInputStream = false;
            bool bHasBaseURL = false;
            sal_Int32 nEnd = lDescriptor.getLength();

            for ( sal_Int32 i = 0; i < nEnd; i++ )
            {
                pNewValue[i] = pOldValue[i];
                if ( pOldValue [i].Name == sInputStream )
                    bHasInputStream = true;
                else if ( pOldValue[i].Name == "DocumentBaseURL" )
                    bHasBaseURL = true;
            }

            if ( !bHasInputStream )
            {
                aArgs.realloc ( ++nEnd );
                auto pArgs = aArgs.getArray();
                pArgs[nEnd-1].Name = sInputStream;
                pArgs[nEnd-1].Value <<= css::uno::Reference < css::io::XInputStream > ( new utl::OSeekableInputStreamWrapper ( *rMedium.GetInStream() ) );
            }

            if ( !bHasBaseURL )
            {
                aArgs.realloc ( ++nEnd );
                auto pArgs = aArgs.getArray();
                pArgs[nEnd-1].Name = "DocumentBaseURL";
                pArgs[nEnd-1].Value <<= rMedium.GetBaseURL();
            }

            if (xInsertPosition.is()) {
                aArgs.realloc( nEnd += 2 );
                auto pArgs = aArgs.getArray();
                pArgs[nEnd-2].Name = "InsertMode";
                pArgs[nEnd-2].Value <<= true;
                pArgs[nEnd-1].Name = "TextInsertModeRange";
                pArgs[nEnd-1].Value <<= xInsertPosition;
            }

            // #i119492# During loading, some OLE objects like chart will be set
            // modified flag, so needs to reset the flag to false after loading
            bool bRtn = xLoader->filter(aArgs);
            const uno::Sequence < OUString > aNames = GetEmbeddedObjectContainer().GetObjectNames();
            for ( const auto& rName : aNames )
            {
                uno::Reference < embed::XEmbeddedObject > xObj = GetEmbeddedObjectContainer().GetEmbeddedObject( rName );
                OSL_ENSURE( xObj.is(), "An empty entry in the embedded objects list!" );
                if ( xObj.is() )
                {
                    sal_Int32 nState = xObj->getCurrentState();
                    if ( nState == embed::EmbedStates::LOADED || nState == embed::EmbedStates::RUNNING )    // means that the object is not active
                    {
                        uno::Reference< util::XModifiable > xModifiable( xObj->getComponent(), uno::UNO_QUERY );
                        if (xModifiable.is() && xModifiable->isModified())
                        {
                            uno::Reference<embed::XEmbedPersist> const xPers(xObj, uno::UNO_QUERY);
                            assert(xPers.is() && "Modified object without persistence!");
                            // store it before resetting modified!
                            xPers->storeOwn();
                            xModifiable->setModified(false);
                        }
                    }
                }
            }

            // tdf#107690 import custom document property _MarkAsFinal as SecurityOptOpenReadonly
            // (before this fix, LibreOffice opened read-only OOXML documents as editable,
            // also saved and exported _MarkAsFinal=true silently, resulting unintended read-only
            // warning info bar in MSO)
            uno::Reference< document::XDocumentPropertiesSupplier > xPropSupplier(GetModel(), uno::UNO_QUERY_THROW);
            uno::Reference<document::XDocumentProperties> xDocProps = xPropSupplier->getDocumentProperties() ;
            uno::Reference<beans::XPropertyContainer> xPropertyContainer = xDocProps->getUserDefinedProperties();
            if (xPropertyContainer.is())
            {
                uno::Reference<beans::XPropertySet> xPropertySet(xPropertyContainer, uno::UNO_QUERY);
                if (xPropertySet.is())
                {
                    uno::Reference<beans::XPropertySetInfo> xPropertySetInfo = xPropertySet->getPropertySetInfo();
                    if (xPropertySetInfo.is() && xPropertySetInfo->hasPropertyByName(u"_MarkAsFinal"_ustr))
                    {
                        Any anyMarkAsFinal = xPropertySet->getPropertyValue(u"_MarkAsFinal"_ustr);
                        if (
                               ( (anyMarkAsFinal.getValueType() == cppu::UnoType<bool>::get()) && (anyMarkAsFinal.get<bool>()) ) ||
                               ( (anyMarkAsFinal.getValueType() == cppu::UnoType<OUString>::get()) && (anyMarkAsFinal.get<OUString>() == "true") )
                        )
                        {
                            uno::Reference< lang::XMultiServiceFactory > xFactory(GetModel(), uno::UNO_QUERY);
                            uno::Reference< beans::XPropertySet > xSettings(xFactory->createInstance(u"com.sun.star.document.Settings"_ustr), uno::UNO_QUERY);
                            xSettings->setPropertyValue(u"LoadReadonly"_ustr, uno::Any(true));
                        }
                        xPropertyContainer->removeProperty(u"_MarkAsFinal"_ustr);
                    }
                }
            }

            return bRtn;
        }
        catch (const packages::zip::ZipIOException&)
        {
            SetError(ERRCODE_IO_BROKENPACKAGE);
        }
        catch (const lang::WrappedTargetRuntimeException& rWrapped)
        {
            io::WrongFormatException e;
            if (rWrapped.TargetException >>= e)
            {
                SetError(ErrCodeMsg(ERRCODE_SFX_FORMAT_ROWCOL,
                    e.Message, DialogMask::ButtonsOk | DialogMask::MessageError ));
            }
        }
        catch (const css::io::IOException& e)
        {
            SetError(ErrCodeMsg(ERRCODE_SFX_FORMAT_ROWCOL,
                e.Message, DialogMask::ButtonsOk | DialogMask::MessageError ));
        }
        catch (const std::exception& e)
        {
            const char *msg = e.what();
            const OUString sError(msg, strlen(msg), RTL_TEXTENCODING_ASCII_US);
            SAL_WARN("sfx.doc", "exception importing " << sError);
            SetError(ErrCodeMsg(ERRCODE_SFX_DOLOADFAILED,
                sError, DialogMask::ButtonsOk | DialogMask::MessageError));
        }
        catch (...)
        {
            std::abort(); // cannot happen
        }
    }

    return false;
}

bool SfxObjectShell::ExportTo( SfxMedium& rMedium )
{
    const OUString aFilterName( rMedium.GetFilter()->GetFilterName() );
    uno::Reference< document::XExporter > xExporter;

    {
        uno::Reference< lang::XMultiServiceFactory >  xMan = ::comphelper::getProcessServiceFactory();
        uno::Reference < lang::XMultiServiceFactory > xFilterFact (
                xMan->createInstance( u"com.sun.star.document.FilterFactory"_ustr ), uno::UNO_QUERY );

        uno::Sequence < beans::PropertyValue > aProps;
        uno::Reference < container::XNameAccess > xFilters ( xFilterFact, uno::UNO_QUERY );
        if ( xFilters->hasByName( aFilterName ) )
            xFilters->getByName( aFilterName ) >>= aProps;

        OUString aFilterImplName;
        auto pProp = std::find_if(std::cbegin(aProps), std::cend(aProps),
            [](const beans::PropertyValue& rFilterProp) { return rFilterProp.Name == "FilterService"; });
        if (pProp != std::cend(aProps))
            pProp->Value >>= aFilterImplName;

        if ( !aFilterImplName.isEmpty() )
        {
            try
            {
                xExporter.set( xFilterFact->createInstanceWithArguments( aFilterName, uno::Sequence < uno::Any >() ), uno::UNO_QUERY );
            }
            catch(const uno::Exception&)
            {
                xExporter.clear();
            }
        }
    }

    if ( xExporter.is() )
    {
        try{
        uno::Reference< lang::XComponent >  xComp( GetModel(), uno::UNO_QUERY_THROW );
        uno::Reference< document::XFilter > xFilter( xExporter, uno::UNO_QUERY_THROW );
        xExporter->setSourceDocument( xComp );

        css::uno::Sequence < css::beans::PropertyValue > aOldArgs;
        SfxItemSet& rItems = rMedium.GetItemSet();
        TransformItems( SID_SAVEASDOC, rItems, aOldArgs );

        const css::beans::PropertyValue * pOldValue = aOldArgs.getConstArray();
        css::uno::Sequence < css::beans::PropertyValue > aArgs ( aOldArgs.getLength() );
        css::beans::PropertyValue * pNewValue = aArgs.getArray();

        // put in the REAL file name, and copy all PropertyValues
        static constexpr OUString sOutputStream ( u"OutputStream"_ustr  );
        static constexpr OUString sStream ( u"StreamForOutput"_ustr  );
        bool bHasOutputStream = false;
        bool bHasStream = false;
        bool bHasBaseURL = false;
        bool bHasFilterName = false;
        bool bIsRedactMode = false;
        bool bIsPreview = false;
        sal_Int32 nEnd = aOldArgs.getLength();

        for ( sal_Int32 i = 0; i < nEnd; i++ )
        {
            pNewValue[i] = pOldValue[i];
            if ( pOldValue[i].Name == "FileName" )
                pNewValue[i].Value <<= rMedium.GetName();
            else if ( pOldValue[i].Name == sOutputStream )
                bHasOutputStream = true;
            else if ( pOldValue[i].Name == sStream )
                bHasStream = true;
            else if ( pOldValue[i].Name == "DocumentBaseURL" )
                bHasBaseURL = true;
            else if( pOldValue[i].Name == "FilterName" )
                bHasFilterName = true;
        }

        const css::uno::Sequence<css::beans::PropertyValue>& rMediumArgs = rMedium.GetArgs();
        for ( sal_Int32 i = 0; i < rMediumArgs.getLength(); i++ )
        {
            if( rMediumArgs[i].Name == "IsPreview" )
                rMediumArgs[i].Value >>= bIsPreview;
        }

        // FIXME: Handle this inside TransformItems()
        if (rItems.GetItemState(SID_IS_REDACT_MODE) == SfxItemState::SET)
            bIsRedactMode = true;

        if ( !bHasOutputStream )
        {
            aArgs.realloc ( ++nEnd );
            auto pArgs = aArgs.getArray();
            pArgs[nEnd-1].Name = sOutputStream;
            pArgs[nEnd-1].Value <<= css::uno::Reference < css::io::XOutputStream > ( new utl::OOutputStreamWrapper ( *rMedium.GetOutStream() ) );
        }

        // add stream as well, for OOX export and maybe others
        if ( !bHasStream )
        {
            aArgs.realloc ( ++nEnd );
            auto pArgs = aArgs.getArray();
            pArgs[nEnd-1].Name = sStream;
            pArgs[nEnd-1].Value <<= css::uno::Reference < css::io::XStream > ( new utl::OStreamWrapper ( *rMedium.GetOutStream() ) );
        }

        if ( !bHasBaseURL )
        {
            aArgs.realloc ( ++nEnd );
            auto pArgs = aArgs.getArray();
            pArgs[nEnd-1].Name = "DocumentBaseURL";
            pArgs[nEnd-1].Value <<= rMedium.GetBaseURL( true );
        }

        if( !bHasFilterName )
        {
            aArgs.realloc( ++nEnd );
            auto pArgs = aArgs.getArray();
            pArgs[nEnd-1].Name = "FilterName";
            pArgs[nEnd-1].Value <<= aFilterName;
        }

        if (bIsRedactMode)
        {
            aArgs.realloc( ++nEnd );
            auto pArgs = aArgs.getArray();
            pArgs[nEnd-1].Name = "IsRedactMode";
            pArgs[nEnd-1].Value <<= bIsRedactMode;
        }

        if (bIsPreview)
        {
            aArgs.realloc( ++nEnd );
            auto pArgs = aArgs.getArray();
            pArgs[nEnd-1].Name = "IsPreview";
            pArgs[nEnd-1].Value <<= bIsPreview;
        }

        return xFilter->filter( aArgs );
        }
        catch (const css::uno::RuntimeException&)
        {
            css::uno::Any ex(cppu::getCaughtException());
            TOOLS_INFO_EXCEPTION("sfx.doc", "exception: " << exceptionToString(ex));
        }
        catch (const std::exception& e)
        {
            TOOLS_INFO_EXCEPTION("sfx.doc", "exception: " << e.what());
        }
        catch(...)
        {
            TOOLS_INFO_EXCEPTION("sfx.doc", "Unknown exception!");
        }
    }

    return false;
}


bool SfxObjectShell::ConvertTo
(
    SfxMedium&  /*rMedium*/   /*  <SfxMedium>, which describes the target file
                                    (for example file name, <SfxFilter>,
                                    Open-Modi and so on) */
)

/*  [Description]

    This method is called for saving of documents over all filters which are
    not SfxFilterFlags::OWN or for which no clipboard format has been registered
    (thus no storage format that is used). In other words, with this method
    it is exported.

    Files which are to be opened here should be opened through 'rMedium'
    to guarantee the right open modes. Especially if the format is retained
    (only possible with SfxFilterFlags::SIMULATE or SfxFilterFlags::OWN) file which must
    be opened STREAM_SHARE_DENYWRITE.

    [Return value]

    bool                true
                        The document could be saved.

                        false
                        The document could not be saved, an error code is
                        received by <SvMedium::GetError()const>


    [Example]

    bool DocSh::ConvertTo( SfxMedium &rMedium )
    {
        SvStreamRef xStream = rMedium.GetOutStream();
        if ( xStream.is() )
        {
            xStream->SetBufferSize(4096);
            *xStream << ...;

            rMedium.CloseOutStream(); // opens the InStream automatically
            return ERRCODE_NONE == rMedium.GetError();
        }
        return false ;
    }

    [Cross-references]

    <SfxObjectShell::ConvertFrom(SfxMedium&)>
    <SfxFilterFlags::REGISTRATION>
*/

{
    return false;
}


bool SfxObjectShell::DoSave_Impl( const SfxItemSet* pArgs )
{
    SfxMedium* pRetrMedium = GetMedium();
    std::shared_ptr<const SfxFilter> pFilter = pRetrMedium->GetFilter();

    // copy the original itemset, but remove the "version" item, because pMediumTmp
    // is a new medium "from scratch", so no version should be stored into it
    std::shared_ptr<SfxItemSet> pSet = std::make_shared<SfxAllItemSet>(pRetrMedium->GetItemSet());
    pSet->ClearItem( SID_VERSION );
    pSet->ClearItem( SID_DOC_BASEURL );

    // copy the version comment and major items for the checkin only
    if ( pRetrMedium->IsInCheckIn( ) )
    {
        const SfxPoolItem* pMajor = pArgs->GetItem( SID_DOCINFO_MAJOR );
        if ( pMajor )
            pSet->Put( *pMajor );

        const SfxPoolItem* pComments = pArgs->GetItem( SID_DOCINFO_COMMENTS );
        if ( pComments )
            pSet->Put( *pComments );
    }

    // create a medium as a copy; this medium is only for writing, because it
    // uses the same name as the original one writing is done through a copy,
    // that will be transferred to the target (of course after calling HandsOff)
    SfxMedium* pMediumTmp = new SfxMedium(pRetrMedium->GetName(), pRetrMedium->GetOpenMode(),
                                          std::move(pFilter), std::move(pSet));
    pMediumTmp->SetInCheckIn( pRetrMedium->IsInCheckIn( ) );
    pMediumTmp->SetLongName( pRetrMedium->GetLongName() );
    if ( pMediumTmp->GetErrorCode() != ERRCODE_NONE )
    {
        SetError(pMediumTmp->GetErrorIgnoreWarning());
        delete pMediumTmp;
        return false;
    }

    // copy version list from "old" medium to target medium, so it can be used on saving
    if (pImpl->bPreserveVersions)
        pMediumTmp->TransferVersionList_Impl( *pRetrMedium );

    // Save the original interaction handler
    Any aOriginalInteract;
    if (const SfxUnoAnyItem *pItem = pRetrMedium->GetItemSet().GetItemIfSet(SID_INTERACTIONHANDLER, false))
    {
        aOriginalInteract = pItem->GetValue();
#ifndef NDEBUG
        // The original pRetrMedium and potential replacement pMediumTmp have the same interaction handler at this point
        const SfxUnoAnyItem *pMediumItem = pMediumTmp->GetItemSet().GetItemIfSet(SID_INTERACTIONHANDLER, false);
        assert(pMediumItem && pMediumItem->GetValue() == aOriginalInteract);
#endif
    }

    // an interaction handler here can acquire only in case of GUI Saving
    // and should be removed after the saving is done
    css::uno::Reference< XInteractionHandler > xInteract;
    const SfxUnoAnyItem* pxInteractionItem = SfxItemSet::GetItem<SfxUnoAnyItem>(pArgs, SID_INTERACTIONHANDLER, false);
    if ( pxInteractionItem && ( pxInteractionItem->GetValue() >>= xInteract ) && xInteract.is() )
        pMediumTmp->GetItemSet().Put( SfxUnoAnyItem( SID_INTERACTIONHANDLER, Any( xInteract ) ) );

    const SfxBoolItem* pNoFileSync = pArgs->GetItem<SfxBoolItem>(SID_NO_FILE_SYNC, false);
    if (pNoFileSync && pNoFileSync->GetValue())
        pMediumTmp->DisableFileSync(true);

    bool bSaved = false;
    if( !GetErrorIgnoreWarning() && SaveTo_Impl( *pMediumTmp, pArgs ) )
    {
        bSaved = true;

        if (aOriginalInteract.hasValue())
            pMediumTmp->GetItemSet().Put(SfxUnoAnyItem(SID_INTERACTIONHANDLER, aOriginalInteract));
        else
            pMediumTmp->GetItemSet().ClearItem(SID_INTERACTIONHANDLER);
        pMediumTmp->GetItemSet().ClearItem( SID_PROGRESS_STATUSBAR_CONTROL );

        SetError(pMediumTmp->GetErrorCode());

        bool bOpen = DoSaveCompleted( pMediumTmp );

        DBG_ASSERT(bOpen,"Error handling for DoSaveCompleted not implemented");
    }
    else
    {
        // transfer error code from medium to objectshell
        ErrCodeMsg errCode = pMediumTmp->GetErrorIgnoreWarning();
        SetError(errCode);

        if (errCode == ERRCODE_ABORT)
        {
            // avoid doing DoSaveCompleted() which updates the SfxMedium timestamp values
            // and prevents subsequent filedate checks from being accurate
            delete pMediumTmp;
            return false;
        }

        // reconnect to object storage
        DoSaveCompleted();

        if (aOriginalInteract.hasValue())
            pRetrMedium->GetItemSet().Put(SfxUnoAnyItem(SID_INTERACTIONHANDLER, aOriginalInteract));
        else
            pRetrMedium->GetItemSet().ClearItem(SID_INTERACTIONHANDLER);
        pRetrMedium->GetItemSet().ClearItem( SID_PROGRESS_STATUSBAR_CONTROL );

        delete pMediumTmp;
    }

    SetModified( !bSaved );
    return bSaved;
}


bool SfxObjectShell::Save_Impl( const SfxItemSet* pSet )
{
    if ( IsReadOnly() )
    {
        SetError(ERRCODE_SFX_DOCUMENTREADONLY);
        return false;
    }

    pImpl->bIsSaving = true;
    bool bSaved = false;
    const SfxStringItem* pSalvageItem = GetMedium()->GetItemSet().GetItem(SID_DOC_SALVAGE, false);
    if ( pSalvageItem )
    {
        const SfxStringItem* pFilterItem = GetMedium()->GetItemSet().GetItem(SID_FILTER_NAME, false);
        std::shared_ptr<const SfxFilter> pFilter;
        if ( pFilterItem )
            pFilter = SfxFilterMatcher( GetFactory().GetFactoryName() ).GetFilter4FilterName( OUString() );

        SfxMedium *pMed = new SfxMedium(
            pSalvageItem->GetValue(), StreamMode::READWRITE | StreamMode::SHARE_DENYWRITE | StreamMode::TRUNC,
            std::move(pFilter) );

        const SfxStringItem* pPasswordItem = GetMedium()->GetItemSet().GetItem(SID_PASSWORD, false);
        if ( pPasswordItem )
            pMed->GetItemSet().Put( *pPasswordItem );

        bSaved = DoSaveAs( *pMed );
        if ( bSaved )
            bSaved = DoSaveCompleted( pMed );
        else
            delete pMed;
    }
    else
        bSaved = DoSave_Impl( pSet );
    return bSaved;
}

bool SfxObjectShell::CommonSaveAs_Impl(const INetURLObject& aURL, const OUString& aFilterName,
                                       SfxItemSet& rItemSet,
                                       const uno::Sequence<beans::PropertyValue>& rArgs)
{
    if( aURL.HasError() )
    {
        SetError(ERRCODE_IO_INVALIDPARAMETER);
        return false;
    }

    if ( aURL != INetURLObject( u"private:stream" ) )
    {
        // Is there already a Document with this name?
        SfxObjectShell* pDoc = nullptr;
        for ( SfxObjectShell* pTmp = SfxObjectShell::GetFirst();
                pTmp && !pDoc;
                pTmp = SfxObjectShell::GetNext(*pTmp) )
        {
            if( ( pTmp != this ) && pTmp->GetMedium() )
            {
                INetURLObject aCompare( pTmp->GetMedium()->GetName() );
                if ( aCompare == aURL )
                    pDoc = pTmp;
            }
        }
        if ( pDoc )
        {
            // Then error message: "already opened"
            SetError(ERRCODE_SFX_ALREADYOPEN);
            return false;
        }
    }

    DBG_ASSERT( aURL.GetProtocol() != INetProtocol::NotValid, "Illegal URL!" );
    DBG_ASSERT( rItemSet.Count() != 0, "Incorrect Parameter");

    const SfxBoolItem* pSaveToItem = rItemSet.GetItem<SfxBoolItem>(SID_SAVETO, false);
    bool bSaveTo = pSaveToItem && pSaveToItem->GetValue();

    std::shared_ptr<const SfxFilter> pFilter = GetFactory().GetFilterContainer()->GetFilter4FilterName( aFilterName );
    if ( !pFilter
        || !pFilter->CanExport()
        || (!bSaveTo && !pFilter->CanImport()) )
    {
        SetError(ERRCODE_IO_INVALIDPARAMETER);
        return false;
    }


    const SfxBoolItem* pCopyStreamItem = rItemSet.GetItem(SID_COPY_STREAM_IF_POSSIBLE, false);
    if ( bSaveTo && pCopyStreamItem && pCopyStreamItem->GetValue() && !IsModified() )
    {
        if (pMedium->TryDirectTransfer(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE), rItemSet))
            return true;
    }
    rItemSet.ClearItem( SID_COPY_STREAM_IF_POSSIBLE );

    SfxMedium *pActMed = GetMedium();
    const INetURLObject aActName(pActMed->GetName());

    bool bWasReadonly = IsReadOnly();

    if ( aURL == aActName && aURL != INetURLObject( u"private:stream" )
        && IsReadOnly() )
    {
        SetError(ERRCODE_SFX_DOCUMENTREADONLY);
        return false;
    }

    if (SfxItemState::SET != rItemSet.GetItemState(SID_UNPACK) && officecfg::Office::Common::Save::Document::Unpacked::get())
        rItemSet.Put(SfxBoolItem(SID_UNPACK, false));

#if HAVE_FEATURE_MULTIUSER_ENVIRONMENT
    OUString aTempFileURL;
    if ( IsDocShared() )
        aTempFileURL = pMedium->GetURLObject().GetMainURL( INetURLObject::DecodeMechanism::NONE );
#endif

    if (PreDoSaveAs_Impl(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE), aFilterName,
                         rItemSet, rArgs))
    {
        // Update Data on media
        SfxItemSet& rSet = GetMedium()->GetItemSet();
        rSet.ClearItem( SID_INTERACTIONHANDLER );
        rSet.ClearItem( SID_PROGRESS_STATUSBAR_CONTROL );
        rSet.ClearItem( SID_PATH );

        if ( !bSaveTo )
        {
            rSet.ClearItem( SID_REFERER );
            rSet.ClearItem( SID_POSTDATA );
            rSet.ClearItem( SID_TEMPLATE );
            rSet.ClearItem( SID_DOC_READONLY );
            rSet.ClearItem( SID_CONTENTTYPE );
            rSet.ClearItem( SID_CHARSET );
            rSet.ClearItem( SID_FILTER_NAME );
            rSet.ClearItem( SID_OPTIONS );
            rSet.ClearItem( SID_VERSION );
            rSet.ClearItem( SID_EDITDOC );
            rSet.ClearItem( SID_OVERWRITE );
            rSet.ClearItem( SID_DEFAULTFILEPATH );
            rSet.ClearItem( SID_DEFAULTFILENAME );

            const SfxStringItem* pFilterItem = rItemSet.GetItem<SfxStringItem>(SID_FILTER_NAME, false);
            if ( pFilterItem )
                rSet.Put( *pFilterItem );

            const SfxStringItem* pOptionsItem = rItemSet.GetItem<SfxStringItem>(SID_OPTIONS, false);
            if ( pOptionsItem )
                rSet.Put( *pOptionsItem );

            const SfxStringItem* pFilterOptItem = rItemSet.GetItem<SfxStringItem>(SID_FILE_FILTEROPTIONS, false);
            if ( pFilterOptItem )
                rSet.Put( *pFilterOptItem );

#if HAVE_FEATURE_MULTIUSER_ENVIRONMENT
            if ( IsDocShared() && !aTempFileURL.isEmpty() )
            {
                // this is a shared document that has to be disconnected from the old location
                FreeSharedFile( aTempFileURL );

                if ( pFilter->IsOwnFormat()
                  && pFilter->UsesStorage()
                  && pFilter->GetVersion() >= SOFFICE_FILEFORMAT_60 )
                {
                    // the target format is the own format
                    // the target document must be shared
                    SwitchToShared( true, false );
                }
            }
#endif
        }

        if ( bWasReadonly && !bSaveTo )
            Broadcast( SfxHint(SfxHintId::ModeChanged) );

        return true;
    }
    else
        return false;
}

bool SfxObjectShell::PreDoSaveAs_Impl(const OUString& rFileName, const OUString& aFilterName,
                                      SfxItemSet const& rItemSet,
                                      const uno::Sequence<beans::PropertyValue>& rArgs)
{
    // copy all items stored in the itemset of the current medium
    std::shared_ptr<SfxAllItemSet> xMergedParams = std::make_shared<SfxAllItemSet>( pMedium->GetItemSet() );

    // in "SaveAs" title and password will be cleared ( maybe the new itemset contains new values, otherwise they will be empty )
    // #i119366# - As the SID_ENCRYPTIONDATA and SID_PASSWORD are using for setting password together, we need to clear them both.
    // Also, ( maybe the new itemset contains new values, otherwise they will be empty )
    if (xMergedParams->HasItem(SID_ENCRYPTIONDATA))
    {
        bool bPasswordProtected = true;
        const SfxUnoAnyItem* pEncryptionDataItem
            = xMergedParams->GetItem<SfxUnoAnyItem>(SID_ENCRYPTIONDATA, false);
        if (pEncryptionDataItem)
        {
            uno::Sequence<beans::NamedValue> aEncryptionData;
            pEncryptionDataItem->GetValue() >>= aEncryptionData;
            for (const auto& rItem : aEncryptionData)
            {
                if (rItem.Name == "CryptoType")
                {
                    OUString aValue;
                    rItem.Value >>= aValue;
                    if (aValue != "StrongEncryptionDataSpace")
                    {
                        // This is not just a password protected document. Let's keep encryption data as is.
                        bPasswordProtected = false;
                    }
                    break;
                }
            }
        }
        if (bPasswordProtected)
        {
            // For password protected documents remove encryption data during "Save as..."
            xMergedParams->ClearItem(SID_PASSWORD);
            xMergedParams->ClearItem(SID_ENCRYPTIONDATA);
        }
    }

    xMergedParams->ClearItem( SID_DOCINFO_TITLE );

    xMergedParams->ClearItem( SID_INPUTSTREAM );
    xMergedParams->ClearItem( SID_STREAM );
    xMergedParams->ClearItem( SID_CONTENT );
    xMergedParams->ClearItem( SID_DOC_READONLY );
    xMergedParams->ClearItem( SID_DOC_BASEURL );

    xMergedParams->ClearItem( SID_REPAIRPACKAGE );

    // "SaveAs" will never store any version information - it's a complete new file !
    xMergedParams->ClearItem( SID_VERSION );

    // merge the new parameters into the copy
    // all values present in both itemsets will be overwritten by the new parameters
    xMergedParams->Put(rItemSet);

    SAL_WARN_IF( xMergedParams->GetItemState( SID_DOC_SALVAGE) >= SfxItemState::SET,
        "sfx.doc","Salvage item present in Itemset, check the parameters!");

    // should be unnecessary - too hot to handle!
    xMergedParams->ClearItem( SID_DOC_SALVAGE );

    // create a medium for the target URL
    SfxMedium *pNewFile = new SfxMedium( rFileName, StreamMode::READWRITE | StreamMode::SHARE_DENYWRITE | StreamMode::TRUNC, nullptr, xMergedParams );
    pNewFile->SetArgs(rArgs);

    const SfxBoolItem* pNoFileSync = xMergedParams->GetItem<SfxBoolItem>(SID_NO_FILE_SYNC, false);
    if (pNoFileSync && pNoFileSync->GetValue())
        pNewFile->DisableFileSync(true);

    bool bUseThumbnailSave = IsUseThumbnailSave();
    comphelper::ScopeGuard aThumbnailGuard(
        [this, bUseThumbnailSave] { this->SetUseThumbnailSave(bUseThumbnailSave); });
    const SfxBoolItem* pNoThumbnail = xMergedParams->GetItem<SfxBoolItem>(SID_NO_THUMBNAIL, false);
    if (pNoThumbnail)
        // Thumbnail generation should be avoided just for this save.
        SetUseThumbnailSave(!pNoThumbnail->GetValue());
    else
        aThumbnailGuard.dismiss();

    // set filter; if no filter is given, take the default filter of the factory
    if ( !aFilterName.isEmpty() )
    {
        pNewFile->SetFilter( GetFactory().GetFilterContainer()->GetFilter4FilterName( aFilterName ) );

        if (aFilterName == "writer_pdf_Export")
        {
            uno::Sequence< beans::PropertyValue > aSaveToFilterDataOptions(2);
            auto pSaveToFilterDataOptions = aSaveToFilterDataOptions.getArray();
            bool bRet = false;

            for(int i = 0 ; i< rArgs.getLength() ; ++i)
            {
                const auto& rProp = rArgs[i];
                if (rProp.Name == "EncryptFile")
                {
                    pSaveToFilterDataOptions[0].Name = rProp.Name;
                    pSaveToFilterDataOptions[0].Value = rProp.Value;
                    bRet = true;
                }
                else if (rProp.Name == "DocumentOpenPassword")
                {
                    pSaveToFilterDataOptions[1].Name = rProp.Name;
                    pSaveToFilterDataOptions[1].Value = rProp.Value;
                    bRet = true;
                }
            }

            if( bRet )
                pNewFile->GetItemSet().Put( SfxUnoAnyItem(SID_FILTER_DATA, uno::Any(aSaveToFilterDataOptions)));
        }
    }
    else
        pNewFile->SetFilter( GetFactory().GetFilterContainer()->GetAnyFilter( SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT ) );

    if ( pNewFile->GetErrorCode() != ERRCODE_NONE )
    {
        // creating temporary file failed ( f.e. floppy disk not inserted! )
        SetError(pNewFile->GetErrorIgnoreWarning());
        delete pNewFile;
        return false;
    }

    if (comphelper::LibreOfficeKit::isActive())
    {
        // Before saving, commit in-flight changes.
        TerminateEditing();
    }

    // check if a "SaveTo" is wanted, no "SaveAs"
    const SfxBoolItem* pSaveToItem = xMergedParams->GetItem<SfxBoolItem>(SID_SAVETO, false);
    bool bCopyTo = GetCreateMode() == SfxObjectCreateMode::EMBEDDED || (pSaveToItem && pSaveToItem->GetValue());

    // distinguish between "Save" and "SaveAs"
    pImpl->bIsSaving = false;

    // copy version list from "old" medium to target medium, so it can be used on saving
    if ( pImpl->bPreserveVersions )
        pNewFile->TransferVersionList_Impl( *pMedium );

    // Save the document ( first as temporary file, then transfer to the target URL by committing the medium )
    bool bOk = false;
    if ( !pNewFile->GetErrorCode() && SaveTo_Impl( *pNewFile, nullptr ) )
    {
        // transfer a possible error from the medium to the document
        SetError(pNewFile->GetErrorCode());

        // notify the document that saving was done successfully
        if ( !bCopyTo )
        {
            bOk = DoSaveCompleted( pNewFile );
        }
        else
            bOk = DoSaveCompleted();

        if( bOk )
        {
            if( !bCopyTo )
                SetModified( false );
        }
        else
        {
            // TODO/LATER: the code below must be dead since the storage commit makes all the stuff
            //       and the DoSaveCompleted call should not be able to fail in general

            DBG_ASSERT( !bCopyTo, "Error while reconnecting to medium, can't be handled!");
            SetError(pNewFile->GetErrorCode());

            if ( !bCopyTo )
            {
                // reconnect to the old medium
                bool bRet = DoSaveCompleted( pMedium );
                DBG_ASSERT( bRet, "Error in DoSaveCompleted, can't be handled!");
            }

            // TODO/LATER: disconnect the new file from the storage for the case when pure saving is done
            //       if storing has corrupted the file, probably it must be restored either here or
            //       by the storage
            delete pNewFile;
            pNewFile = nullptr;
        }
    }
    else
    {
        SetError(pNewFile->GetErrorCode());

        // reconnect to the old storage
        DoSaveCompleted();

        delete pNewFile;
        pNewFile = nullptr;
    }

    if ( bCopyTo )
        delete pNewFile;
    else if( !bOk )
        SetModified();

    return bOk;
}


bool SfxObjectShell::LoadFrom( SfxMedium& /*rMedium*/ )
{
    SAL_WARN( "sfx.doc", "Base implementation, must not be called in general!" );
    return true;
}


bool SfxObjectShell::CanReload_Impl()

/*  [Description]

    Internal method for determining whether a reload of the document
    (as RevertToSaved or last known version) is possible.
*/

{
    return pMedium && HasName() && !IsInModalMode() && !pImpl->bForbidReload;
}


HiddenInformation SfxObjectShell::GetHiddenInformationState( HiddenInformation nStates )
{
    HiddenInformation nState = HiddenInformation::NONE;
    if ( nStates & HiddenInformation::DOCUMENTVERSIONS )
    {
        if ( GetMedium()->GetVersionList().hasElements() )
            nState |= HiddenInformation::DOCUMENTVERSIONS;
    }

    return nState;
}

void SfxObjectShell::QueryHiddenInformation(HiddenWarningFact eFact)
{
    SvtSecurityOptions::EOption eOption = SvtSecurityOptions::EOption();

    switch ( eFact )
    {
        case HiddenWarningFact::WhenSaving :
        {
            eOption = SvtSecurityOptions::EOption::DocWarnSaveOrSend;
            break;
        }
        case HiddenWarningFact::WhenPrinting :
        {
            eOption = SvtSecurityOptions::EOption::DocWarnPrint;
            break;
        }
        case HiddenWarningFact::WhenSigning :
        {
            eOption = SvtSecurityOptions::EOption::DocWarnSigning;
            break;
        }
        case HiddenWarningFact::WhenCreatingPDF :
        {
            eOption = SvtSecurityOptions::EOption::DocWarnCreatePdf;
            break;
        }
        default:
            assert(false); // this cannot happen
    }

    if ( SvtSecurityOptions::IsOptionSet( eOption ) )
    {
        OUString sMessage;
        HiddenInformation nWantedStates = HiddenInformation::RECORDEDCHANGES | HiddenInformation::NOTES;
        if ( eFact != HiddenWarningFact::WhenPrinting )
            nWantedStates |= HiddenInformation::DOCUMENTVERSIONS;
        HiddenInformation nStates = GetHiddenInformationState( nWantedStates );

        if ( nStates & HiddenInformation::RECORDEDCHANGES )
        {
            sMessage += SfxResId(STR_HIDDENINFO_RECORDCHANGES) + "\n";
        }
        if ( nStates & HiddenInformation::NOTES )
        {
            sMessage += SfxResId(STR_HIDDENINFO_NOTES) + "\n";
        }
        if ( nStates & HiddenInformation::DOCUMENTVERSIONS )
        {
            sMessage += SfxResId(STR_HIDDENINFO_DOCVERSIONS) + "\n";
        }

        SfxViewFrame* pFrame = SfxViewFrame::GetFirst(this);
        if (pFrame)
            pFrame->HandleSecurityInfobar(!sMessage.isEmpty() ? sMessage.trim().replaceAll("\n", ", ") : sMessage);

    }
}

bool SfxObjectShell::IsSecurityOptOpenReadOnly() const
{
    return IsLoadReadonly();
}

void SfxObjectShell::SetSecurityOptOpenReadOnly( bool _b )
{
    SetLoadReadonly( _b );
}

bool SfxObjectShell::LoadOwnFormat( SfxMedium& rMedium )
{
    SAL_INFO( "sfx.doc", "loading \" " << rMedium.GetName() << "\"" );

    uno::Reference< embed::XStorage > xStorage = rMedium.GetStorage();
    if ( xStorage.is() )
    {
        // Password
        const SfxStringItem* pPasswdItem = rMedium.GetItemSet().GetItem(SID_PASSWORD, false);
        if ( pPasswdItem || ERRCODE_IO_ABORT != CheckPasswd_Impl( this, pMedium ) )
        {
            // note: this could be needed in case no interaction handler is
            // provided (which CheckPasswd_Impl needs) but a password item is,
            // but it could be done in a better way
            uno::Sequence< beans::NamedValue > aEncryptionData;
            if ( GetEncryptionData_Impl(&pMedium->GetItemSet(), aEncryptionData) )
            {
                try
                {
                    // the following code must throw an exception in case of failure
                    ::comphelper::OStorageHelper::SetCommonStorageEncryptionData( xStorage, aEncryptionData );
                }
                catch( uno::Exception& )
                {
                    // TODO/LATER: handle the error code
                }
            }

            // load document
            return Load( rMedium );
        }
        return false;
    }
    else
        return false;
}

bool SfxObjectShell::SaveAsOwnFormat( SfxMedium& rMedium )
{
    uno::Reference< embed::XStorage > xStorage = rMedium.GetStorage();
    if( xStorage.is() )
    {
        sal_Int32 nVersion = rMedium.GetFilter()->GetVersion();

        // OASIS templates have own mediatypes (SO7 also actually, but it is too late to use them here)
        const bool bTemplate = rMedium.GetFilter()->IsOwnTemplateFormat()
            && nVersion > SOFFICE_FILEFORMAT_60;

        SetupStorage( xStorage, nVersion, bTemplate );
#if HAVE_FEATURE_SCRIPTING
        if ( HasBasic() )
        {
            // Initialize Basic
            GetBasicManager();

            // Save dialog/script container
            pImpl->aBasicManager.storeLibrariesToStorage( xStorage );
        }
#endif

        if (comphelper::LibreOfficeKit::isActive())
        {
            // Because XMLTextFieldExport::ExportFieldDeclarations (called from SwXMLExport)
            // calls SwXTextFieldMasters::getByName, which in turn maps property names by
            // calling SwStyleNameMapper::GetTextUINameArray, which uses
            // SvtSysLocale().GetUILanguageTag() to do the mapping, saving indirectly depends
            // on the UI language. This is an unfortunate dependency. Here we use the loader's language.
            const LanguageTag& viewLanguage = comphelper::LibreOfficeKit::getLanguageTag();
            const LanguageTag loadLanguage = SfxLokHelper::getLoadLanguage();

            // Use the default language for saving and restore later if necessary.
            bool restoreLanguage = false;
            if (viewLanguage != loadLanguage)
            {
                restoreLanguage = true;
                comphelper::LibreOfficeKit::setLanguageTag(loadLanguage);
            }

            // Restore the view's original language automatically and as necessary.
            const ::comphelper::ScopeGuard aGuard(
                [&viewLanguage, restoreLanguage]()
                {
                    if (restoreLanguage
                        && viewLanguage != comphelper::LibreOfficeKit::getLanguageTag())
                        comphelper::LibreOfficeKit::setLanguageTag(viewLanguage);
                });

            return SaveAs(rMedium);
        }

        return SaveAs( rMedium );
    }
    else return false;
}

uno::Reference< embed::XStorage > const & SfxObjectShell::GetStorage()
{
    if ( !pImpl->m_xDocStorage.is() )
    {
        OSL_ENSURE( pImpl->m_bCreateTempStor, "The storage must exist already!" );
        try {
            // no notification is required the storage is set the first time
            pImpl->m_xDocStorage = ::comphelper::OStorageHelper::GetTemporaryStorage();
            OSL_ENSURE( pImpl->m_xDocStorage.is(), "The method must either return storage or throw exception!" );

            SetupStorage( pImpl->m_xDocStorage, SOFFICE_FILEFORMAT_CURRENT, false );
            pImpl->m_bCreateTempStor = false;
            if (!comphelper::IsFuzzing())
                SfxGetpApp()->NotifyEvent( SfxEventHint( SfxEventHintId::StorageChanged, GlobalEventConfig::GetEventName(GlobalEventId::STORAGECHANGED), this ) );
        }
        catch( uno::Exception& )
        {
            // TODO/LATER: error handling?
            TOOLS_WARN_EXCEPTION("sfx.doc", "SfxObjectShell::GetStorage");
        }
    }

    OSL_ENSURE( pImpl->m_xDocStorage.is(), "The document storage must be created!" );
    return pImpl->m_xDocStorage;
}


void SfxObjectShell::SaveChildren( bool bObjectsOnly )
{
    if ( pImpl->mxObjectContainer )
    {
        bool bOasis = ( SotStorage::GetVersion( GetStorage() ) > SOFFICE_FILEFORMAT_60 );
        GetEmbeddedObjectContainer().StoreChildren(bOasis,bObjectsOnly);
    }
}

bool SfxObjectShell::SaveAsChildren( SfxMedium& rMedium )
{
    uno::Reference < embed::XStorage > xStorage = rMedium.GetStorage();
    if ( !xStorage.is() )
        return false;

    if ( xStorage == GetStorage() )
    {
        SaveChildren();
        return true;
    }

    bool AutoSaveEvent = false;
    utl::MediaDescriptor lArgs(rMedium.GetArgs());
    lArgs[utl::MediaDescriptor::PROP_AUTOSAVEEVENT] >>= AutoSaveEvent;

    if ( pImpl->mxObjectContainer )
    {
        bool bOasis = ( SotStorage::GetVersion( xStorage ) > SOFFICE_FILEFORMAT_60 );
        GetEmbeddedObjectContainer().StoreAsChildren(bOasis,SfxObjectCreateMode::EMBEDDED == eCreateMode,AutoSaveEvent,xStorage);
    }

    uno::Sequence<OUString> aExceptions;
    if (const SfxBoolItem* pNoEmbDS = rMedium.GetItemSet().GetItem(SID_NO_EMBEDDED_DS, false))
    {
        // Don't save data source in case a temporary is being saved for preview in MM wizard
        if (pNoEmbDS->GetValue())
            aExceptions = uno::Sequence<OUString>{ u"EmbeddedDatabase"_ustr };
    }

    return CopyStoragesOfUnknownMediaType(GetStorage(), xStorage, aExceptions);
}

bool SfxObjectShell::SaveCompletedChildren()
{
    bool bResult = true;

    if ( pImpl->mxObjectContainer )
    {
        const uno::Sequence < OUString > aNames = GetEmbeddedObjectContainer().GetObjectNames();
        for ( const auto& rName : aNames )
        {
            uno::Reference < embed::XEmbeddedObject > xObj = GetEmbeddedObjectContainer().GetEmbeddedObject( rName );
            OSL_ENSURE( xObj.is(), "An empty entry in the embedded objects list!" );
            if ( xObj.is() )
            {
                uno::Reference< embed::XEmbedPersist > xPersist( xObj, uno::UNO_QUERY );
                if ( xPersist.is() )
                {
                    try
                    {
                        xPersist->saveCompleted( false/*bSuccess*/ );
                    }
                    catch( uno::Exception& )
                    {
                        // TODO/LATER: error handling
                        bResult = false;
                        break;
                    }
                }
            }
        }
    }

    return bResult;
}

bool SfxObjectShell::SwitchChildrenPersistence( const uno::Reference< embed::XStorage >& xStorage,
                                                    bool bForceNonModified )
{
    if ( !xStorage.is() )
    {
        // TODO/LATER: error handling
        return false;
    }

    if ( pImpl->mxObjectContainer )
        pImpl->mxObjectContainer->SetPersistentEntries(xStorage,bForceNonModified);

    return true;
}

// Never call this method directly, always use the DoSaveCompleted call
bool SfxObjectShell::SaveCompleted( const uno::Reference< embed::XStorage >& xStorage )
{
    bool bResult = false;
    bool bSendNotification = false;
    uno::Reference< embed::XStorage > xOldStorageHolder;

    // check for wrong creation of object container
    bool bHasContainer( pImpl->mxObjectContainer );

    if ( !xStorage.is() || xStorage == GetStorage() )
    {
        // no persistence change
        bResult = SaveCompletedChildren();
    }
    else
    {
        if ( pImpl->mxObjectContainer )
            GetEmbeddedObjectContainer().SwitchPersistence( xStorage );

        bResult = SwitchChildrenPersistence( xStorage, true );
    }

    if ( bResult )
    {
        if ( xStorage.is() && pImpl->m_xDocStorage != xStorage )
        {
            // make sure that until the storage is assigned the object
            // container is not created by accident!
            DBG_ASSERT( bHasContainer == (pImpl->mxObjectContainer != nullptr), "Wrong storage in object container!" );
            xOldStorageHolder = pImpl->m_xDocStorage;
            pImpl->m_xDocStorage = xStorage;
            bSendNotification = true;

            if ( IsEnableSetModified() )
                SetModified( false );
        }
    }
    else
    {
        if ( pImpl->mxObjectContainer )
            GetEmbeddedObjectContainer().SwitchPersistence( pImpl->m_xDocStorage );

        // let already successfully connected objects be switched back
        SwitchChildrenPersistence( pImpl->m_xDocStorage, true );
    }

    if ( bSendNotification )
    {
        SfxGetpApp()->NotifyEvent( SfxEventHint( SfxEventHintId::StorageChanged, GlobalEventConfig::GetEventName(GlobalEventId::STORAGECHANGED), this ) );
    }

    return bResult;
}

static bool StoragesOfUnknownMediaTypeAreCopied_Impl( const uno::Reference< embed::XStorage >& xSource,
                                                   const uno::Reference< embed::XStorage >& xTarget )
{
    OSL_ENSURE( xSource.is() && xTarget.is(), "Source and/or target storages are not available!" );
    if ( !xSource.is() || !xTarget.is() || xSource == xTarget )
        return true;

    try
    {
        const uno::Sequence< OUString > aSubElements = xSource->getElementNames();
        for ( const auto& rSubElement : aSubElements )
        {
            if ( xSource->isStorageElement( rSubElement ) )
            {
                OUString aMediaType;
                static constexpr OUString aMediaTypePropName( u"MediaType"_ustr  );
                bool bGotMediaType = false;

                try
                {
                    uno::Reference< embed::XOptimizedStorage > xOptStorage( xSource, uno::UNO_QUERY_THROW );
                    bGotMediaType =
                        ( xOptStorage->getElementPropertyValue( rSubElement, aMediaTypePropName ) >>= aMediaType );
                }
                catch( uno::Exception& )
                {}

                if ( !bGotMediaType )
                {
                    uno::Reference< embed::XStorage > xSubStorage;
                    try {
                        xSubStorage = xSource->openStorageElement( rSubElement, embed::ElementModes::READ );
                    } catch( uno::Exception& )
                    {}

                    if ( !xSubStorage.is() )
                    {
                        xSubStorage = ::comphelper::OStorageHelper::GetTemporaryStorage();
                        xSource->copyStorageElementLastCommitTo( rSubElement, xSubStorage );
                    }

                    uno::Reference< beans::XPropertySet > xProps( xSubStorage, uno::UNO_QUERY_THROW );
                    xProps->getPropertyValue( aMediaTypePropName ) >>= aMediaType;
                }

                // TODO/LATER: there should be a way to detect whether an object with such a MediaType can exist
                //             probably it should be placed in the MimeType-ClassID table or in standalone table
                if ( !aMediaType.isEmpty()
                  && aMediaType != "application/vnd.sun.star.oleobject" )
                {
                    css::datatransfer::DataFlavor aDataFlavor;
                    aDataFlavor.MimeType = aMediaType;
                    SotClipboardFormatId nFormat = SotExchange::GetFormat( aDataFlavor );

                    switch ( nFormat )
                    {
                        case SotClipboardFormatId::STARWRITER_60 :
                        case SotClipboardFormatId::STARWRITERWEB_60 :
                        case SotClipboardFormatId::STARWRITERGLOB_60 :
                        case SotClipboardFormatId::STARDRAW_60 :
                        case SotClipboardFormatId::STARIMPRESS_60 :
                        case SotClipboardFormatId::STARCALC_60 :
                        case SotClipboardFormatId::STARCHART_60 :
                        case SotClipboardFormatId::STARMATH_60 :
                        case SotClipboardFormatId::STARWRITER_8:
                        case SotClipboardFormatId::STARWRITERWEB_8:
                        case SotClipboardFormatId::STARWRITERGLOB_8:
                        case SotClipboardFormatId::STARDRAW_8:
                        case SotClipboardFormatId::STARIMPRESS_8:
                        case SotClipboardFormatId::STARCALC_8:
                        case SotClipboardFormatId::STARCHART_8:
                        case SotClipboardFormatId::STARMATH_8:
                            break;

                        default:
                        {
                            if ( !xTarget->hasByName( rSubElement ) )
                                return false;
                        }
                    }
                }
            }
        }
    }
    catch( uno::Exception& )
    {
        SAL_WARN( "sfx.doc", "Can not check storage consistency!" );
    }

    return true;
}

bool SfxObjectShell::SwitchPersistence( const uno::Reference< embed::XStorage >& xStorage )
{
    bool bResult = false;
    // check for wrong creation of object container
    bool bHasContainer( pImpl->mxObjectContainer );
    if ( xStorage.is() )
    {
        if ( pImpl->mxObjectContainer )
            GetEmbeddedObjectContainer().SwitchPersistence( xStorage );
        bResult = SwitchChildrenPersistence( xStorage );

        // TODO/LATER: substorages that have unknown mimetypes probably should be copied to the target storage here
        OSL_ENSURE( StoragesOfUnknownMediaTypeAreCopied_Impl( pImpl->m_xDocStorage, xStorage ),
                    "Some of substorages with unknown mimetypes is lost!" );
    }

    if ( bResult )
    {
        // make sure that until the storage is assigned the object container is not created by accident!
        DBG_ASSERT( bHasContainer == (pImpl->mxObjectContainer != nullptr), "Wrong storage in object container!" );
        if ( pImpl->m_xDocStorage != xStorage )
            DoSaveCompleted( new SfxMedium( xStorage, GetMedium()->GetBaseURL() ) );

        if ( IsEnableSetModified() )
            SetModified(); // ???
    }

    return bResult;
}

bool SfxObjectShell::CopyStoragesOfUnknownMediaType(const uno::Reference< embed::XStorage >& xSource,
                                                    const uno::Reference< embed::XStorage >& xTarget,
                                                    const uno::Sequence<OUString>& rExceptions)
{
    if (!xSource.is())
    {
        SAL_WARN( "sfx.doc", "SfxObjectShell::GetStorage() failed");
        return false;
    }

    // This method does not commit the target storage and should not do it
    bool bResult = true;

    try
    {
        const css::uno::Sequence<OUString> aSubElementNames = xSource->getElementNames();
        for (const OUString& rSubElement : aSubElementNames)
        {
            if (std::find(rExceptions.begin(), rExceptions.end(), rSubElement) != rExceptions.end())
                continue;

            if (rSubElement == "Configurations")
            {
                // The workaround for compatibility with SO7, "Configurations" substorage must be preserved
                if (xSource->isStorageElement(rSubElement))
                {
                    OSL_ENSURE(!xTarget->hasByName(rSubElement), "The target storage is an output "
                                                                 "storage, the element should not "
                                                                 "exist in the target!");

                    xSource->copyElementTo(rSubElement, xTarget, rSubElement);
                }
            }
            else if (xSource->isStorageElement(rSubElement))
            {
                OUString aMediaType;
                static constexpr OUString aMediaTypePropName( u"MediaType"_ustr  );
                bool bGotMediaType = false;

                try
                {
                    uno::Reference< embed::XOptimizedStorage > xOptStorage( xSource, uno::UNO_QUERY_THROW );
                    bGotMediaType = (xOptStorage->getElementPropertyValue(rSubElement, aMediaTypePropName)
                           >>= aMediaType);
                }
                catch( uno::Exception& )
                {}

                if ( !bGotMediaType )
                {
                    uno::Reference< embed::XStorage > xSubStorage;
                    try {
                        xSubStorage
                            = xSource->openStorageElement(rSubElement, embed::ElementModes::READ);
                    } catch( uno::Exception& )
                    {}

                    if ( !xSubStorage.is() )
                    {
                        // TODO/LATER: as optimization in future a substorage of target storage could be used
                        //             instead of the temporary storage; this substorage should be removed later
                        //             if the MimeType is wrong
                        xSubStorage = ::comphelper::OStorageHelper::GetTemporaryStorage();
                        xSource->copyStorageElementLastCommitTo(rSubElement, xSubStorage);
                    }

                    uno::Reference< beans::XPropertySet > xProps( xSubStorage, uno::UNO_QUERY_THROW );
                    xProps->getPropertyValue( aMediaTypePropName ) >>= aMediaType;
                }

                // TODO/LATER: there should be a way to detect whether an object with such a MediaType can exist
                //             probably it should be placed in the MimeType-ClassID table or in standalone table
                if ( !aMediaType.isEmpty()
                  && aMediaType != "application/vnd.sun.star.oleobject" )
                {
                    css::datatransfer::DataFlavor aDataFlavor;
                    aDataFlavor.MimeType = aMediaType;
                    SotClipboardFormatId nFormat = SotExchange::GetFormat( aDataFlavor );

                    switch ( nFormat )
                    {
                        case SotClipboardFormatId::STARWRITER_60 :
                        case SotClipboardFormatId::STARWRITERWEB_60 :
                        case SotClipboardFormatId::STARWRITERGLOB_60 :
                        case SotClipboardFormatId::STARDRAW_60 :
                        case SotClipboardFormatId::STARIMPRESS_60 :
                        case SotClipboardFormatId::STARCALC_60 :
                        case SotClipboardFormatId::STARCHART_60 :
                        case SotClipboardFormatId::STARMATH_60 :
                        case SotClipboardFormatId::STARWRITER_8:
                        case SotClipboardFormatId::STARWRITERWEB_8:
                        case SotClipboardFormatId::STARWRITERGLOB_8:
                        case SotClipboardFormatId::STARDRAW_8:
                        case SotClipboardFormatId::STARIMPRESS_8:
                        case SotClipboardFormatId::STARCALC_8:
                        case SotClipboardFormatId::STARCHART_8:
                        case SotClipboardFormatId::STARMATH_8:
                            break;

                        default:
                        {
                            OSL_ENSURE(rSubElement == "Configurations2"
                                           || nFormat == SotClipboardFormatId::STARBASE_8
                                           || !xTarget->hasByName(rSubElement),
                                       "The target storage is an output storage, the element "
                                       "should not exist in the target!");

                            if (!xTarget->hasByName(rSubElement))
                            {
                                xSource->copyElementTo(rSubElement, xTarget, rSubElement);
                            }
                        }
                    }
                }
            }
        }
    }
    catch( uno::Exception& )
    {
        bResult = false;
        // TODO/LATER: a specific error could be provided
    }

    return bResult;
}

bool SfxObjectShell::GenerateAndStoreThumbnail(bool bEncrypted, const uno::Reference<embed::XStorage>& xStorage)
{
    //optimize thumbnail generate and store procedure to improve odt saving performance, i120030
    bIsInGenerateThumbnail = true;

    bool bResult = false;

    try
    {
        uno::Reference<embed::XStorage> xThumbnailStorage = xStorage->openStorageElement(u"Thumbnails"_ustr, embed::ElementModes::READWRITE);

        if (xThumbnailStorage.is())
        {
            uno::Reference<io::XStream> xStream = xThumbnailStorage->openStreamElement(u"thumbnail.png"_ustr, embed::ElementModes::READWRITE);

            if (xStream.is() && WriteThumbnail(bEncrypted, xStream))
            {
                uno::Reference<embed::XTransactedObject> xTransactedObject(xThumbnailStorage, uno::UNO_QUERY);
                if (xTransactedObject)
                {
                    xTransactedObject->commit();
                    bResult = true;
                }
            }
        }
    }
    catch( uno::Exception& )
    {
    }

    //optimize thumbnail generate and store procedure to improve odt saving performance, i120030
    bIsInGenerateThumbnail = false;

    return bResult;
}

bool SfxObjectShell::WriteThumbnail(bool bEncrypted, const uno::Reference<io::XStream>& xStream)
{
    bool bResult = false;

    if (!xStream.is())
        return false;

    try
    {
        uno::Reference<io::XTruncate> xTruncate(xStream->getOutputStream(), uno::UNO_QUERY_THROW);
        xTruncate->truncate();

        uno::Reference <beans::XPropertySet> xSet(xStream, uno::UNO_QUERY);
        if (xSet.is())
            xSet->setPropertyValue(u"MediaType"_ustr, uno::Any(u"image/png"_ustr));
        if (bEncrypted)
        {
            const OUString sResID = GraphicHelper::getThumbnailReplacementIDByFactoryName_Impl(
                GetFactory().GetFactoryName());
            if (!sResID.isEmpty())
                bResult = GraphicHelper::getThumbnailReplacement_Impl(sResID, xStream);
        }
        else
        {
            BitmapEx bitmap = GetPreviewBitmap();
            if (!bitmap.IsEmpty())
            {
                bResult = GraphicHelper::getThumbnailFormatFromBitmap_Impl(bitmap, xStream);
            }
        }
    }
    catch(uno::Exception&)
    {}

    return bResult;
}

void SfxObjectShell::UpdateLinks()
{
}

bool SfxObjectShell::LoadExternal( SfxMedium& )
{
    // Not implemented. It's an error if the code path ever comes here.
    assert(false);
    return false;
}

bool SfxObjectShell::InsertGeneratedStream(SfxMedium&,
        uno::Reference<text::XTextRange> const&)
{
    // Not implemented. It's an error if the code path ever comes here.
    assert(false);
    return false;
}

bool SfxObjectShell::IsConfigOptionsChecked() const
{
    return pImpl->m_bConfigOptionsChecked;
}

void SfxObjectShell::SetConfigOptionsChecked( bool bChecked )
{
    pImpl->m_bConfigOptionsChecked = bChecked;
}

void SfxObjectShell::SetMacroCallsSeenWhileLoading()
{
    pImpl->m_bMacroCallsSeenWhileLoading = true;
}

bool SfxObjectShell::GetMacroCallsSeenWhileLoading() const
{
    if (comphelper::IsFuzzing() || officecfg::Office::Common::Security::Scripting::CheckDocumentEvents::get())
        return pImpl->m_bMacroCallsSeenWhileLoading;
    return false;
}

bool SfxObjectShell::QuerySaveSizeExceededModules_Impl( const uno::Reference< task::XInteractionHandler >& xHandler )
{
#if !HAVE_FEATURE_SCRIPTING
    (void) xHandler;
#else
    if ( !HasBasic() )
        return true;

    if ( !pImpl->aBasicManager.isValid() )
        GetBasicManager();
    std::vector< OUString > sModules;
    if ( xHandler.is() )
    {
        if( pImpl->aBasicManager.ImgVersion12PsswdBinaryLimitExceeded( sModules ) )
        {
            rtl::Reference<ModuleSizeExceeded> pReq =  new ModuleSizeExceeded( sModules );
            xHandler->handle( pReq );
            return pReq->isApprove();
        }
    }
#endif
    // No interaction handler, default is to continue to save
    return true;
}

bool SfxObjectShell::QueryAllowExoticFormat_Impl( const uno::Reference< task::XInteractionHandler >& xHandler, const OUString& rURL, const OUString& rFilterUIName )
{
    if ( SvtSecurityOptions::isTrustedLocationUri( rURL ) )
    {
        // Always load from trusted location
        return true;
    }
    if ( officecfg::Office::Common::Security::LoadExoticFileFormats::get() == 0 )
    {
        // Refuse loading without question
        return false;
    }
    else if ( officecfg::Office::Common::Security::LoadExoticFileFormats::get() == 2 )
    {
        // Always load without question
        return true;
    }
    else if ( officecfg::Office::Common::Security::LoadExoticFileFormats::get() == 1 && xHandler.is() )
    {
        // Display a warning and let the user decide
        rtl::Reference<ExoticFileLoadException> xException(new ExoticFileLoadException( rURL, rFilterUIName ));
        xHandler->handle( xException );
        return xException->isApprove();
    }
    // No interaction handler, default is to continue to load
    return true;
}

uno::Reference< task::XInteractionHandler > SfxObjectShell::getInteractionHandler() const
{
    uno::Reference< task::XInteractionHandler > xRet;
    if (SfxMedium* pRetrMedium = GetMedium())
        xRet = pRetrMedium->GetInteractionHandler();
    return xRet;
}

OUString SfxObjectShell::getDocumentBaseURL() const
{
    if (SfxMedium* pRetrMedium = GetMedium())
        return pRetrMedium->GetBaseURL();
    return OUString();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
