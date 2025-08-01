/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/util/URL.hpp>
#include <com/sun/star/util/URLTransformer.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/system/SystemShellExecuteException.hpp>
#include <com/sun/star/document/XTypeDetection.hpp>
#include <com/sun/star/document/MacroExecMode.hpp>
#include <com/sun/star/document/UpdateDocMode.hpp>
#include <com/sun/star/task/ErrorCodeRequest.hpp>
#include <com/sun/star/task/InteractionHandler.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/packages/WrongPasswordException.hpp>
#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>

#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/SetFlagContextHelper.hxx>
#include <comphelper/storagehelper.hxx>
#include <comphelper/string.hxx>
#include <comphelper/synchronousdispatch.hxx>

#include <svl/intitem.hxx>
#include <svl/stritem.hxx>
#include <svl/eitem.hxx>
#include <sfx2/doctempl.hxx>
#include <svtools/sfxecode.hxx>
#include <preventduplicateinteraction.hxx>
#include <svtools/ehdl.hxx>
#include <unotools/pathoptions.hxx>
#include <unotools/securityoptions.hxx>
#include <unotools/moduleoptions.hxx>
#include <unotools/extendedsecurityoptions.hxx>
#include <comphelper/docpasswordhelper.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <com/sun/star/ui/dialogs/XFilePicker3.hpp>
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>

#include <sfx2/app.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/objitem.hxx>
#include <sfx2/objsh.hxx>
#include <svl/slstitm.hxx>
#include <appopen.hxx>
#include <sfx2/request.hxx>
#include <sfx2/sfxresid.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/strings.hrc>
#include <sfx2/viewfrm.hxx>
#include <sfx2/sfxuno.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/filedlghelper.hxx>
#include <sfx2/templatedlg.hxx>
#include <sfx2/sfxsids.hrc>
#include <o3tl/string_view.hxx>
#include <openuriexternally.hxx>

#include <officecfg/Office/ProtocolHandler.hxx>
#include <officecfg/Office/Security.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::container;
using namespace ::cppu;
using namespace ::sfx2;

void SetTemplate_Impl( std::u16string_view rFileName,
                        const OUString &rLongName,
                        SfxObjectShell *pDoc)
{
    // write TemplateName to DocumentProperties of document
    // TemplateDate stays as default (=current date)
    pDoc->ResetFromTemplate( rLongName, rFileName );
}

namespace {

class SfxDocPasswordVerifier : public ::comphelper::IDocPasswordVerifier
{
public:
    explicit SfxDocPasswordVerifier(SfxMedium& rMedium)
        : m_rMedium(rMedium)
        , mxStorage(rMedium.GetStorage())
    {
    }

    virtual ::comphelper::DocPasswordVerifierResult
                        verifyPassword( const OUString& rPassword, uno::Sequence< beans::NamedValue >& o_rEncryptionData ) override;
    virtual ::comphelper::DocPasswordVerifierResult
                        verifyEncryptionData( const uno::Sequence< beans::NamedValue >& rEncryptionData ) override;

private:
    SfxMedium & m_rMedium;
    Reference< embed::XStorage > mxStorage;
};

}

::comphelper::DocPasswordVerifierResult SfxDocPasswordVerifier::verifyPassword( const OUString& rPassword, uno::Sequence< beans::NamedValue >& o_rEncryptionData )
{
    o_rEncryptionData = ::comphelper::OStorageHelper::CreatePackageEncryptionData( rPassword );
    return verifyEncryptionData( o_rEncryptionData );
}


::comphelper::DocPasswordVerifierResult SfxDocPasswordVerifier::verifyEncryptionData( const uno::Sequence< beans::NamedValue >& rEncryptionData )
{
    ::comphelper::DocPasswordVerifierResult eResult = ::comphelper::DocPasswordVerifierResult::WrongPassword;
    try
    {
        // check the encryption data
        // if the data correct is the stream will be opened successfully
        // and immediately closed
        ::comphelper::OStorageHelper::SetCommonStorageEncryptionData( mxStorage, rEncryptionData );

        // for new ODF encryption, try to extract the encrypted inner package
        // (it will become the SfxObjectShell storage)
        if (!m_rMedium.TryEncryptedInnerPackage(mxStorage))
        {   // ... old ODF encryption:
            mxStorage->openStreamElement(
                u"content.xml"_ustr,
                embed::ElementModes::READ | embed::ElementModes::NOCREATE );
        }

        // no exception -> success
        eResult = ::comphelper::DocPasswordVerifierResult::OK;
    }
    catch( const packages::WrongPasswordException& )
    {
        eResult = ::comphelper::DocPasswordVerifierResult::WrongPassword;
    }
    catch( const uno::Exception& )
    {
        // unknown error, report it as wrong password
        // TODO/LATER: we need an additional way to report unknown problems in this case
        eResult = ::comphelper::DocPasswordVerifierResult::WrongPassword;
    }
    return eResult;
}


ErrCode CheckPasswd_Impl
(
    SfxObjectShell*  pDoc,
    SfxMedium*       pFile      // the Medium and its Password should be obtained
)

/*  [Description]

    Ask for the password for a medium, only works if it concerns storage.
    If the password flag is set in the Document Info, then the password is
    requested through a user dialogue and the set at the Set of the medium.
    If the set does not exist the it is created.
*/
{
    ErrCode nRet = ERRCODE_NONE;

    if( !pFile->GetFilter() || pFile->IsStorage() )
    {
        uno::Reference< embed::XStorage > xStorage = pFile->GetStorage();
        if( xStorage.is() )
        {
            uno::Reference< beans::XPropertySet > xStorageProps( xStorage, uno::UNO_QUERY );
            if ( xStorageProps.is() )
            {
                bool bIsEncrypted = false;
                uno::Sequence< uno::Sequence< beans::NamedValue > > aGpgProperties;
                try {
                    xStorageProps->getPropertyValue(u"HasEncryptedEntries"_ustr)
                        >>= bIsEncrypted;
                    xStorageProps->getPropertyValue(u"EncryptionGpGProperties"_ustr)
                        >>= aGpgProperties;
                } catch( uno::Exception& )
                {
                    // TODO/LATER:
                    // the storage either has no encrypted elements or it's just
                    // does not allow to detect it, probably it should be implemented later
                }

                if ( bIsEncrypted )
                {
                    css::uno::Reference<css::awt::XWindow> xWin(pDoc ? pDoc->GetDialogParent(pFile) : nullptr);
                    if (xWin)
                        xWin->setVisible(true);

                    nRet = ERRCODE_SFX_CANTGETPASSWD;

                    SfxItemSet& rSet = pFile->GetItemSet();
                    Reference< css::task::XInteractionHandler > xInteractionHandler = pFile->GetInteractionHandler();
                    if( xInteractionHandler.is() )
                    {
                        // use the comphelper password helper to request a password
                        OUString aPassword;
                        const SfxStringItem* pPasswordItem = rSet.GetItem(SID_PASSWORD, false);
                        if ( pPasswordItem )
                            aPassword = pPasswordItem->GetValue();

                        uno::Sequence< beans::NamedValue > aEncryptionData;
                        const SfxUnoAnyItem* pEncryptionDataItem = rSet.GetItem(SID_ENCRYPTIONDATA, false);
                        if ( pEncryptionDataItem )
                            pEncryptionDataItem->GetValue() >>= aEncryptionData;

                        // try if one of the public key entries is
                        // decryptable, then extract session key
                        // from it
                        if ( !aEncryptionData.hasElements() && aGpgProperties.hasElements() )
                            aEncryptionData = ::comphelper::DocPasswordHelper::decryptGpgSession(aGpgProperties);

                        // tdf#93389: if recovering a document, encryption data should contain
                        // entries for the real filter, not only for recovery ODF, to keep it
                        // encrypted. Pass this in encryption data.
                        // TODO: pass here the real filter (from AutoRecovery::implts_openDocs)
                        // to marshal this to requestAndVerifyDocPassword
                        if (rSet.GetItemState(SID_DOC_SALVAGE, false) == SfxItemState::SET)
                        {
                            aEncryptionData = comphelper::concatSequences(
                                aEncryptionData, std::initializer_list<beans::NamedValue>{
                                                     { u"ForSalvage"_ustr, css::uno::Any(true) } });
                        }

                        SfxDocPasswordVerifier aVerifier(*pFile);
                        aEncryptionData = ::comphelper::DocPasswordHelper::requestAndVerifyDocPassword(
                            aVerifier, aEncryptionData, aPassword, xInteractionHandler, pFile->GetOrigURL(), comphelper::DocPasswordRequestType::Standard );

                        rSet.ClearItem( SID_PASSWORD );
                        rSet.ClearItem( SID_ENCRYPTIONDATA );

                        if ( aEncryptionData.hasElements() )
                        {
                            rSet.Put( SfxUnoAnyItem( SID_ENCRYPTIONDATA, uno::Any( aEncryptionData ) ) );

                            try
                            {
                                // update the version list of the medium using the new password
                                pFile->GetVersionList();
                            }
                            catch( uno::Exception& )
                            {
                                // TODO/LATER: set the error code
                            }

                            nRet = ERRCODE_NONE;
                        }
                        else
                            nRet = ERRCODE_IO_ABORT;
                    }
                }
            }
            else
            {
                OSL_FAIL( "A storage must implement XPropertySet interface!" );
                nRet = ERRCODE_SFX_CANTGETPASSWD;
            }
        }
    }

    return nRet;
}


ErrCodeMsg SfxApplication::LoadTemplate( SfxObjectShellLock& xDoc, const OUString &rFileName, std::unique_ptr<SfxItemSet> pSet )
{
    std::shared_ptr<const SfxFilter> pFilter;
    SfxMedium aMedium( rFileName,  ( StreamMode::READ | StreamMode::SHARE_DENYNONE ) );

    if ( !aMedium.GetStorage( false ).is() )
        aMedium.GetInStream();

    if ( aMedium.GetErrorIgnoreWarning() )
    {
        return aMedium.GetErrorCode();
    }

    aMedium.UseInteractionHandler( true );
    ErrCode nErr = GetFilterMatcher().GuessFilter( aMedium, pFilter, SfxFilterFlags::TEMPLATE, SfxFilterFlags::NONE );
    if ( ERRCODE_NONE != nErr)
    {
        return ERRCODE_SFX_NOTATEMPLATE;
    }

    if( !pFilter || !pFilter->IsAllowedAsTemplate() )
    {
        return ERRCODE_SFX_NOTATEMPLATE;
    }

    if ( pFilter->GetFilterFlags() & SfxFilterFlags::STARONEFILTER )
    {
        DBG_ASSERT( !xDoc.Is(), "Sorry, not implemented!" );
        SfxStringItem aName( SID_FILE_NAME, rFileName );
        SfxStringItem aReferer( SID_REFERER, u"private:user"_ustr );
        SfxStringItem aFlags( SID_OPTIONS, u"T"_ustr );
        SfxBoolItem aHidden( SID_HIDDEN, true );
        const SfxPoolItemHolder aRet(GetDispatcher_Impl()->ExecuteList(
            SID_OPENDOC, SfxCallMode::SYNCHRON,
            { &aName, &aHidden, &aReferer, &aFlags } ));
        const SfxObjectItem* pObj(dynamic_cast<const SfxObjectItem*>(aRet.getItem()));
        if ( pObj )
            xDoc = dynamic_cast<SfxObjectShell*>( pObj->GetShell()  );
        else
        {
            const SfxViewFrameItem* pView(dynamic_cast<const SfxViewFrameItem*>(aRet.getItem()));
            if ( pView )
            {
                SfxViewFrame *pFrame = pView->GetFrame();
                if ( pFrame )
                    xDoc = pFrame->GetObjectShell();
            }
        }

        if ( !xDoc.Is() )
            return ERRCODE_SFX_DOLOADFAILED;
    }
    else
    {
        if ( !xDoc.Is() )
            xDoc = SfxObjectShell::CreateObject( pFilter->GetServiceName() );

        //pMedium takes ownership of pSet
        SfxMedium *pMedium = new SfxMedium(rFileName, StreamMode::STD_READ, std::move(pFilter), std::move(pSet));
        if(!xDoc->DoLoad(pMedium))
        {
            ErrCodeMsg nErrCode = xDoc->GetErrorCode();
            xDoc->DoClose();
            xDoc.Clear();
            return nErrCode;
        }
    }

    try
    {
        // TODO: introduce error handling

        uno::Reference< embed::XStorage > xTempStorage = ::comphelper::OStorageHelper::GetTemporaryStorage();
        if( !xTempStorage.is() )
            throw uno::RuntimeException();

        xDoc->GetStorage()->copyToStorage( xTempStorage );

        if ( !xDoc->DoSaveCompleted( new SfxMedium( xTempStorage, OUString() ) ) )
            throw uno::RuntimeException();
    }
    catch( uno::Exception& )
    {
        xDoc->DoClose();
        xDoc.Clear();

        // TODO: transfer correct error outside
        return ERRCODE_SFX_GENERAL;
    }

    SetTemplate_Impl( rFileName, OUString(), xDoc );

    xDoc->SetNoName();
    xDoc->InvalidateName();
    xDoc->SetModified(false);
    xDoc->ResetError();

    css::uno::Reference< css::frame::XModel >  xModel = xDoc->GetModel();
    if ( xModel.is() )
    {
        std::unique_ptr<SfxItemSet> pNew = xDoc->GetMedium()->GetItemSet().Clone();
        pNew->ClearItem( SID_PROGRESS_STATUSBAR_CONTROL );
        pNew->ClearItem( SID_FILTER_NAME );
        css::uno::Sequence< css::beans::PropertyValue > aArgs;
        TransformItems( SID_OPENDOC, *pNew, aArgs );
        sal_Int32 nLength = aArgs.getLength();
        aArgs.realloc( nLength + 1 );
        auto pArgs = aArgs.getArray();
        pArgs[nLength].Name = "Title";
        pArgs[nLength].Value <<= xDoc->GetTitle( SFX_TITLE_DETECT );
        xModel->attachResource( OUString(), aArgs );
    }

    return xDoc->GetErrorCode();
}


void SfxApplication::NewDocDirectExec_Impl( SfxRequest& rReq )
{
    const SfxStringItem* pFactoryItem = rReq.GetArg<SfxStringItem>(SID_NEWDOCDIRECT);
    OUString aFactName;
    if ( pFactoryItem )
        aFactName = pFactoryItem->GetValue();
    else
        aFactName = SvtModuleOptions().GetDefaultModuleName();

    SfxRequest aReq( SID_OPENDOC, SfxCallMode::SYNCHRON, GetPool() );
    aReq.AppendItem( SfxStringItem( SID_FILE_NAME, "private:factory/" + aFactName ) );
    aReq.AppendItem( SfxFrameItem( SID_DOCFRAME, GetFrame() ) );
    aReq.AppendItem( SfxStringItem( SID_TARGETNAME, u"_default"_ustr ) );

    // TODO/LATER: Should the other arguments be transferred as well?
    const SfxStringItem* pDefaultPathItem = rReq.GetArg<SfxStringItem>(SID_DEFAULTFILEPATH);
    if ( pDefaultPathItem )
        aReq.AppendItem( *pDefaultPathItem );
    const SfxStringItem* pDefaultNameItem = rReq.GetArg<SfxStringItem>(SID_DEFAULTFILENAME);
    if ( pDefaultNameItem )
        aReq.AppendItem( *pDefaultNameItem );

    SfxGetpApp()->ExecuteSlot( aReq );
    const SfxViewFrameItem* pItem(dynamic_cast<const SfxViewFrameItem*>(aReq.GetReturnValue().getItem()));
    if (nullptr != pItem)
        rReq.SetReturnValue(SfxFrameItem(0, pItem->GetFrame()));
}

void SfxApplication::NewDocDirectState_Impl( SfxItemSet &rSet )
{
    rSet.Put(SfxStringItem(SID_NEWDOCDIRECT, "private:factory/" + SvtModuleOptions().GetDefaultModuleName()));
}

void SfxApplication::NewDocExec_Impl( SfxRequest& rReq )
{
    // No Parameter from BASIC only Factory given?
    const SfxStringItem* pTemplNameItem = rReq.GetArg<SfxStringItem>(SID_TEMPLATE_NAME);
    const SfxStringItem* pTemplFileNameItem = rReq.GetArg<SfxStringItem>(SID_FILE_NAME);
    const SfxStringItem* pTemplRegionNameItem = rReq.GetArg<SfxStringItem>(SID_TEMPLATE_REGIONNAME);

    SfxObjectShellLock xDoc;

    OUString  aTemplateRegion, aTemplateName, aTemplateFileName;
    bool    bDirect = false; // through FileName instead of Region/Template
    SfxErrorContext aEc(ERRCTX_SFX_NEWDOC);
    if ( !pTemplNameItem && !pTemplFileNameItem )
    {
        bool bNewWin = false;
        weld::Window* pTopWin = GetTopWindow();

        SfxObjectShell* pCurrentShell = SfxObjectShell::Current();
        Reference<XModel> xModel;
        if(pCurrentShell)
            xModel = pCurrentShell->GetModel();

        SfxTemplateManagerDlg aTemplDlg(rReq.GetFrameWeld());

        if (xModel.is())
            aTemplDlg.setDocumentModel(xModel);

        int nRet = aTemplDlg.run();
        if ( nRet == RET_OK )
        {
            rReq.Done();
            if ( pTopWin != GetTopWindow() )
            {
                // the dialogue opens a document -> a new TopWindow appears
                pTopWin = GetTopWindow();
                bNewWin = true;
            }
        }

        if (bNewWin && pTopWin)
        {
            // after the destruction of the dialogue its parent comes to top,
            // but we want that the new document is on top
            pTopWin->present();
        }

        return;
    }
    else
    {
        // Template-Name
        if ( pTemplNameItem )
            aTemplateName = pTemplNameItem->GetValue();

        // Template-Region
        if ( pTemplRegionNameItem )
            aTemplateRegion = pTemplRegionNameItem->GetValue();

        // Template-File-Name
        if ( pTemplFileNameItem )
        {
            aTemplateFileName = pTemplFileNameItem->GetValue();
            bDirect = true;
        }
    }

    ErrCode lErr = ERRCODE_NONE;
    if ( !bDirect )
    {
        SfxDocumentTemplates aTmpFac;
        if( aTemplateFileName.isEmpty() )
            aTmpFac.GetFull( aTemplateRegion, aTemplateName, aTemplateFileName );

        if( aTemplateFileName.isEmpty() )
            lErr = ERRCODE_SFX_TEMPLATENOTFOUND;
    }

    INetURLObject aObj( aTemplateFileName );
    SfxErrorContext aEC( ERRCTX_SFX_LOADTEMPLATE, aObj.PathToFileName() );

    if ( lErr != ERRCODE_NONE )
    {
        ErrCode lFatalErr = lErr.IgnoreWarning();
        if ( lFatalErr )
            ErrorHandler::HandleError(lErr);
    }
    else
    {
        SfxCallMode eMode = SfxCallMode::SYNCHRON;
        SfxPoolItemHolder aResult;
        SfxStringItem aReferer( SID_REFERER, u"private:user"_ustr );
        SfxStringItem aTarget( SID_TARGETNAME, u"_default"_ustr );
        if ( !aTemplateFileName.isEmpty() )
        {
            DBG_ASSERT( aObj.GetProtocol() != INetProtocol::NotValid, "Illegal URL!" );

            SfxStringItem aName( SID_FILE_NAME, aObj.GetMainURL( INetURLObject::DecodeMechanism::NONE ) );
            SfxStringItem aTemplName( SID_TEMPLATE_NAME, aTemplateName );
            SfxStringItem aTemplRegionName( SID_TEMPLATE_REGIONNAME, aTemplateRegion );
            aResult = GetDispatcher_Impl()->ExecuteList(SID_OPENDOC, eMode,
                {&aName, &aTarget, &aReferer, &aTemplName, &aTemplRegionName});
        }
        else
        {
            SfxStringItem aName( SID_FILE_NAME, u"private:factory"_ustr );
            aResult = GetDispatcher_Impl()->ExecuteList(SID_OPENDOC, eMode,
                    { &aName, &aTarget, &aReferer } );
        }

        if (aResult)
            rReq.SetReturnValue( *aResult.getItem() );
    }
}


namespace {

/**
 * Check if a given filter type should open the hyperlinked document
 * natively.
 *
 * @param rFilter filter object
 */
bool lcl_isFilterNativelySupported(const SfxFilter& rFilter)
{
    if (rFilter.IsOwnFormat())
        return true;

    const OUString& aName = rFilter.GetFilterName();
    // We can handle all Excel variants natively.
    return aName.startsWith("MS Excel");
}

}

void SfxApplication::OpenDocExec_Impl( SfxRequest& rReq )
{
    OUString aDocService;
    const SfxStringItem* pDocSrvItem = rReq.GetArg<SfxStringItem>(SID_DOC_SERVICE);
    if (pDocSrvItem)
        aDocService = pDocSrvItem->GetValue();

    sal_uInt16 nSID = rReq.GetSlot();
    const SfxStringItem* pFileNameItem = rReq.GetArg<SfxStringItem>(SID_FILE_NAME);
    if ( pFileNameItem )
    {
        OUString aCommand( pFileNameItem->GetValue() );
        const SfxSlot* pSlot = GetInterface()->GetSlot( aCommand );
        if ( pSlot )
        {
            pFileNameItem = nullptr;
        }
        else
        {
            if ( aCommand.startsWith("slot:") )
            {
                sal_uInt16 nSlotId = static_cast<sal_uInt16>(o3tl::toInt32(aCommand.subView(5)));
                if ( nSlotId == SID_OPENDOC )
                    pFileNameItem = nullptr;
            }
        }
    }

    if ( !pFileNameItem )
    {
        // get FileName from dialog
        css::uno::Sequence<OUString> aURLList;
        OUString aFilter;
        std::optional<SfxAllItemSet> pSet;
        OUString aPath;
        const SfxStringItem* pFolderNameItem = rReq.GetArg<SfxStringItem>(SID_PATH);
        if ( pFolderNameItem )
            aPath = pFolderNameItem->GetValue();
        else if ( nSID == SID_OPENTEMPLATE )
        {
            aPath = SvtPathOptions().GetTemplatePath();
            if (!aPath.isEmpty())                             // if not empty then get last token
                aPath = aPath.copy(aPath.lastIndexOf(';')+1); // lastIndexOf+copy works whether separator (';') is there or not
        }

        sal_Int16 nDialog = SFX2_IMPL_DIALOG_CONFIG;
        const SfxBoolItem* pSystemDialogItem = rReq.GetArg<SfxBoolItem>(SID_FILE_DIALOG);
        if ( pSystemDialogItem )
            nDialog = pSystemDialogItem->GetValue() ? SFX2_IMPL_DIALOG_SYSTEM : SFX2_IMPL_DIALOG_OOO;

        const SfxBoolItem* pRemoteDialogItem = rReq.GetArg<SfxBoolItem>(SID_REMOTE_DIALOG);
        if ( pRemoteDialogItem && pRemoteDialogItem->GetValue())
            nDialog = SFX2_IMPL_DIALOG_REMOTE;

        sal_Int16 nDialogType = ui::dialogs::TemplateDescription::FILEOPEN_READONLY_VERSION_FILTEROPTIONS;
        FileDialogFlags eDialogFlags = FileDialogFlags::MultiSelection;
        const SfxBoolItem* pSignPDFItem = rReq.GetArg<SfxBoolItem>(SID_SIGNPDF);
        if (pSignPDFItem && pSignPDFItem->GetValue())
        {
            eDialogFlags |= FileDialogFlags::SignPDF;
            nDialogType = ui::dialogs::TemplateDescription::FILEOPEN_SIMPLE;
        }

        css::uno::Sequence< OUString >  aDenyList;

        const SfxStringListItem* pDenyListItem = rReq.GetArg<SfxStringListItem>(SID_DENY_LIST);
        if ( pDenyListItem )
            pDenyListItem->GetStringList( aDenyList );

        std::optional<bool> bShowFilterDialog;
        weld::Window* pTopWindow = GetTopWindow();
        ErrCode nErr = sfx2::FileOpenDialog_Impl(pTopWindow,
                nDialogType,
                eDialogFlags, aURLList,
                aFilter, pSet, &aPath, nDialog, aDenyList, bShowFilterDialog);

        if ( nErr == ERRCODE_ABORT )
        {
            return;
        }

        rReq.SetArgs( *pSet );
        if ( !aFilter.isEmpty() )
            rReq.AppendItem( SfxStringItem( SID_FILTER_NAME, aFilter ) );
        rReq.AppendItem( SfxStringItem( SID_TARGETNAME, u"_default"_ustr ) );
        rReq.AppendItem( SfxStringItem( SID_REFERER, u"private:user"_ustr ) );
        pSet.reset();

        if (aURLList.hasElements())
        {
            if ( nSID == SID_OPENTEMPLATE )
                rReq.AppendItem( SfxBoolItem( SID_TEMPLATE, false ) );

            // This helper wraps an existing (or may new created InteractionHandler)
            // intercept all incoming interactions and provide useful information
            // later if the following transaction was finished.

            rtl::Reference<sfx2::PreventDuplicateInteraction> pHandler = new sfx2::PreventDuplicateInteraction(comphelper::getProcessComponentContext());
            uno::Reference<task::XInteractionHandler> xWrappedHandler;

            // wrap existing handler or create new UUI handler
            const SfxUnoAnyItem* pInteractionItem = rReq.GetArg<SfxUnoAnyItem>(SID_INTERACTIONHANDLER);
            if (pInteractionItem)
            {
                pInteractionItem->GetValue() >>= xWrappedHandler;
                rReq.RemoveItem( SID_INTERACTIONHANDLER );
            }
            if (xWrappedHandler.is())
                pHandler->setHandler(xWrappedHandler);
            else
                pHandler->useDefaultUUIHandler();
            rReq.AppendItem( SfxUnoAnyItem(SID_INTERACTIONHANDLER,css::uno::Any(uno::Reference<task::XInteractionHandler>(pHandler))) );

            // define rules for this handler
            css::uno::Type aInteraction = ::cppu::UnoType<css::task::ErrorCodeRequest>::get();
            ::sfx2::PreventDuplicateInteraction::InteractionInfo aRule(aInteraction);
            pHandler->addInteractionRule(aRule);

            if (!aDocService.isEmpty())
            {
                rReq.RemoveItem(SID_DOC_SERVICE);
                rReq.AppendItem(SfxStringItem(SID_DOC_SERVICE, aDocService));
            }

            // Passes the checkbox state of "Edit Filter Settings" to filter dialogs through multiple layers of code.
            // Since some layers use a published API and cannot be modified directly, we use a context layer instead.
            // This is a one-time flag and is not stored in any configuration.
            // For an example of how it's used, see ScFilterOptionsObj::execute.
            std::optional<css::uno::ContextLayer> oLayer;
            if (bShowFilterDialog.has_value())
            {
                oLayer.emplace(comphelper::NewFlagContext(u"ShowFilterDialog"_ustr,
                                                          bShowFilterDialog.value()));
            }

            for (auto const& url : aURLList)
            {
                rReq.RemoveItem( SID_FILE_NAME );
                rReq.AppendItem( SfxStringItem( SID_FILE_NAME, url ) );

                // Run synchronous, so that not the next document is loaded
                // when rescheduling
                // TODO/LATER: use URLList argument and always remove one document after another, each step in asynchronous execution, until finished
                // but only if reschedule is a problem
                GetDispatcher_Impl()->Execute( SID_OPENDOC, SfxCallMode::SYNCHRON, *rReq.GetArgs() );

                // check for special interaction "NO MORE DOCUMENTS ALLOWED" and
                // break loop then. Otherwise we risk showing the same interaction more than once.
                if ( pHandler->getInteractionInfo(aInteraction, &aRule) )
                {
                    if (aRule.m_nCallCount > 0)
                    {
                        if (aRule.m_xRequest.is())
                        {
                            css::task::ErrorCodeRequest aRequest;
                            if (aRule.m_xRequest->getRequest() >>= aRequest)
                            {
                                if (aRequest.ErrCode == sal_Int32(sal_uInt32(ERRCODE_SFX_NOMOREDOCUMENTSALLOWED)))
                                    break;
                            }
                        }
                    }
                }
            }

            return;
        }
    }

    bool bHyperlinkUsed = false;

    if ( SID_OPENURL == nSID )
    {
        // SID_OPENURL does the same as SID_OPENDOC!
        rReq.SetSlot( SID_OPENDOC );
    }
    else if ( nSID == SID_OPENTEMPLATE )
    {
        rReq.AppendItem( SfxBoolItem( SID_TEMPLATE, false ) );
    }
    // pass URL to OS by using ShellExecuter or open it internal
    // if it seems to be an own format.
    /* Attention!
            There exist two possibilities to open hyperlinks:
            a) using SID_OPENHYPERLINK (new)
            b) using SID_BROWSE        (old)
     */
    else if ( nSID == SID_OPENHYPERLINK )
    {
        rReq.SetSlot( SID_OPENDOC );
        bHyperlinkUsed = true;
    }

    // no else here! It's optional ...
    if (!bHyperlinkUsed)
    {
        const SfxBoolItem* pHyperLinkUsedItem = rReq.GetArg<SfxBoolItem>(SID_BROWSE);
        if ( pHyperLinkUsedItem )
            bHyperlinkUsed = pHyperLinkUsedItem->GetValue();
        // no "official" item, so remove it from ItemSet before using UNO-API
        rReq.RemoveItem( SID_BROWSE );
    }

    const SfxStringItem* pFileName = rReq.GetArg<SfxStringItem>(SID_FILE_NAME);
    assert(pFileName && "SID_FILE_NAME is required");
    OUString aFileName = pFileName->GetValue();

    OUString aReferer;
    const SfxStringItem* pRefererItem = rReq.GetArg<SfxStringItem>(SID_REFERER);
    if ( pRefererItem )
        aReferer = pRefererItem->GetValue();

    const SfxStringItem* pFileFlagsItem = rReq.GetArg<SfxStringItem>(SID_OPTIONS);
    if ( pFileFlagsItem )
    {
        const OUString aFileFlags = pFileFlagsItem->GetValue().toAsciiUpperCase();
        if ( aFileFlags.indexOf('T') >= 0 )
        {
            rReq.RemoveItem( SID_TEMPLATE );
            rReq.AppendItem( SfxBoolItem( SID_TEMPLATE, true ) );
        }

        if ( aFileFlags.indexOf('H') >= 0 )
        {
            rReq.RemoveItem( SID_HIDDEN );
            rReq.AppendItem( SfxBoolItem( SID_HIDDEN, true ) );
        }

        if ( aFileFlags.indexOf('R') >= 0 )
        {
            rReq.RemoveItem( SID_DOC_READONLY );
            rReq.AppendItem( SfxBoolItem( SID_DOC_READONLY, true ) );
        }

        if ( aFileFlags.indexOf('B') >= 0 )
        {
            rReq.RemoveItem( SID_PREVIEW );
            rReq.AppendItem( SfxBoolItem( SID_PREVIEW, true ) );
        }

        rReq.RemoveItem( SID_OPTIONS );
    }

    // Mark without URL cannot be handled by hyperlink code
    if ( bHyperlinkUsed && !aFileName.isEmpty() && aFileName[0] != '#' )
    {
        uno::Reference<document::XTypeDetection> xTypeDetection(
            comphelper::getProcessServiceFactory()->createInstance(u"com.sun.star.document.TypeDetection"_ustr), UNO_QUERY);

        if ( xTypeDetection.is() )
        {
            URL             aURL;

            aURL.Complete = aFileName;
            Reference< util::XURLTransformer > xTrans( util::URLTransformer::create( ::comphelper::getProcessComponentContext() ) );
            xTrans->parseStrict( aURL );

            INetProtocol aINetProtocol = INetURLObject( aURL.Complete ).GetProtocol();
            auto eMode = officecfg::Office::Security::Hyperlinks::Open::get();

            if ( eMode == SvtExtendedSecurityOptions::OPEN_NEVER && aINetProtocol != INetProtocol::VndSunStarHelp )
            {
                SolarMutexGuard aGuard;
                weld::Window *pWindow = SfxGetpApp()->GetTopWindow();

                std::unique_ptr<weld::MessageDialog> xSecurityWarningBox(Application::CreateMessageDialog(pWindow,
                                                                         VclMessageType::Warning, VclButtonsType::Ok, SfxResId(STR_SECURITY_WARNING_NO_HYPERLINKS)));
                xSecurityWarningBox->set_title(SfxResId(RID_SECURITY_WARNING_TITLE));
                xSecurityWarningBox->run();
                return;
            }

            std::shared_ptr<const SfxFilter> pFilter{};

            // attempt loading native documents only if they are from a known protocol
            // it might be sensible to limit the set of protocols even further, but that
            // may cause regressions, needs further testing
            // see tdf#136427 for details
            if (aINetProtocol != INetProtocol::NotValid) {
                const OUString aTypeName { xTypeDetection->queryTypeByURL( aURL.Main ) };
                SfxFilterMatcher& rMatcher = SfxGetpApp()->GetFilterMatcher();
                pFilter = rMatcher.GetFilter4EA( aTypeName );
            }

            bool bStartPresentation = false;
            if (pFilter)
            {
                const SfxUInt16Item* pSlide = rReq.GetArg<SfxUInt16Item>(SID_DOC_STARTPRESENTATION);
                if (pSlide
                    && (pFilter->GetWildcard().Matches(u".pptx")
                        || pFilter->GetWildcard().Matches(u".ppt")
                        || pFilter->GetWildcard().Matches(u".ppsx")
                        || pFilter->GetWildcard().Matches(u".pps")))
                {
                    bStartPresentation = true;
                }
            }

            if (!pFilter || (!lcl_isFilterNativelySupported(*pFilter) && !bStartPresentation))
            {
                // hyperlink does not link to own type => special handling (http, ftp) browser and (other external protocols) OS
                if ( aINetProtocol == INetProtocol::Mailto )
                {
                    // don't dispatch mailto hyperlink to desktop dispatcher
                    rReq.RemoveItem( SID_TARGETNAME );
                    rReq.AppendItem( SfxStringItem( SID_TARGETNAME, u"_self"_ustr ) );
                }
                else if ( aINetProtocol == INetProtocol::Ftp ||
                     aINetProtocol == INetProtocol::Http ||
                     aINetProtocol == INetProtocol::Https )
                {
                    sfx2::openUriExternally(aURL.Complete, true, rReq.GetFrameWeld());
                    return;
                }
                else
                {
                    // check for "internal" protocols that should not be forwarded to the system
                    // add special protocols that always should be treated as internal
                    std::vector < OUString > aProtocols { u"private:*"_ustr, u"vnd.sun.star.*"_ustr };

                    // get registered protocol handlers from configuration
                    Reference < XNameAccess > xAccess(officecfg::Office::ProtocolHandler::HandlerSet::get());
                    const Sequence < OUString > aNames = xAccess->getElementNames();
                    for ( const auto& rName : aNames )
                    {
                        Reference < XPropertySet > xSet;
                        Any aRet = xAccess->getByName( rName );
                        aRet >>= xSet;
                        if ( xSet.is() )
                        {
                            // copy protocols
                            aRet = xSet->getPropertyValue(u"Protocols"_ustr);
                            Sequence < OUString > aTmp;
                            aRet >>= aTmp;

                            aProtocols.insert(aProtocols.end(),std::cbegin(aTmp),std::cend(aTmp));
                        }
                    }

                    bool bFound = false;
                    for (const OUString & rProtocol : aProtocols)
                    {
                        WildCard aPattern(rProtocol);
                        if ( aPattern.Matches( aURL.Complete ) )
                        {
                            bFound = true;
                            break;
                        }
                    }

                    if ( !bFound )
                    {
                        bool bLoadInternal = false;
                        try
                        {
                            sfx2::openUriExternally(
                                aURL.Complete, pFilter == nullptr, rReq.GetFrameWeld());
                        }
                        catch ( css::system::SystemShellExecuteException& )
                        {
                            rReq.RemoveItem( SID_TARGETNAME );
                            rReq.AppendItem( SfxStringItem( SID_TARGETNAME, u"_default"_ustr ) );
                            bLoadInternal = true;
                        }
                        if ( !bLoadInternal )
                            return;
                    }
                }
            }
            else
            {
                // hyperlink document must be loaded into a new frame
                rReq.RemoveItem( SID_TARGETNAME );
                rReq.AppendItem( SfxStringItem( SID_TARGETNAME, u"_default"_ustr ) );
            }
        }
    }

    if (!SvtSecurityOptions::isSecureMacroUri(aFileName, aReferer))
    {
        SfxErrorContext aCtx( ERRCTX_SFX_OPENDOC, aFileName );
        ErrorHandler::HandleError( ERRCODE_IO_ACCESSDENIED );
        return;
    }

    SfxFrame* pTargetFrame = nullptr;
    Reference< XFrame > xTargetFrame;

    const SfxFrameItem* pFrameItem = rReq.GetArg<SfxFrameItem>(SID_DOCFRAME);
    if ( pFrameItem )
        pTargetFrame = pFrameItem->GetFrame();

    if ( !pTargetFrame )
    {
        const SfxUnoFrameItem* pUnoFrameItem = rReq.GetArg<SfxUnoFrameItem>(SID_FILLFRAME);
        if ( pUnoFrameItem )
            xTargetFrame = pUnoFrameItem->GetFrame();
    }

    if (!pTargetFrame && !xTargetFrame.is())
    {
        if (const SfxViewFrame* pViewFrame = SfxViewFrame::Current())
            pTargetFrame = &pViewFrame->GetFrame();
    }

    // check if caller has set a callback
    std::unique_ptr<SfxLinkItem> pLinkItem;

    // remove from Itemset, because it confuses the parameter transformation
    if (auto pParamLinkItem = rReq.GetArg<SfxLinkItem>(SID_DONELINK))
        pLinkItem.reset(pParamLinkItem->Clone());

    rReq.RemoveItem( SID_DONELINK );

    // check if the view must be hidden
    bool bHidden = false;
    const SfxBoolItem* pHidItem = rReq.GetArg<SfxBoolItem>(SID_HIDDEN);
    if ( pHidItem )
        bHidden = pHidItem->GetValue();

    // This request is a UI call. We have to set the right values inside the MediaDescriptor
    // for: InteractionHandler, StatusIndicator, MacroExecutionMode and DocTemplate.
    // But we have to look for already existing values or for real hidden requests.
    const SfxBoolItem* pPreviewItem = rReq.GetArg<SfxBoolItem>(SID_PREVIEW);
    if (!bHidden && ( !pPreviewItem || !pPreviewItem->GetValue() ) )
    {
        const SfxUnoAnyItem* pInteractionItem = rReq.GetArg<SfxUnoAnyItem>(SID_INTERACTIONHANDLER);
        const SfxUInt16Item* pMacroExecItem = rReq.GetArg<SfxUInt16Item>(SID_MACROEXECMODE);
        const SfxUInt16Item* pDocTemplateItem = rReq.GetArg<SfxUInt16Item>(SID_UPDATEDOCMODE);

        if (!pInteractionItem)
        {
            Reference < task::XInteractionHandler2 > xHdl = task::InteractionHandler::createWithParent( ::comphelper::getProcessComponentContext(), nullptr );
            rReq.AppendItem( SfxUnoAnyItem(SID_INTERACTIONHANDLER,css::uno::Any(xHdl)) );
        }
        if (!pMacroExecItem)
            rReq.AppendItem( SfxUInt16Item(SID_MACROEXECMODE,css::document::MacroExecMode::USE_CONFIG) );
        if (!pDocTemplateItem)
            rReq.AppendItem( SfxUInt16Item(SID_UPDATEDOCMODE,css::document::UpdateDocMode::ACCORDING_TO_CONFIG) );
    }

    // extract target name
    OUString aTarget;
    const SfxStringItem* pTargetItem = rReq.GetArg<SfxStringItem>(SID_TARGETNAME);
    if ( pTargetItem )
        aTarget = pTargetItem->GetValue();
    else
    {
        const SfxBoolItem* pNewViewItem = rReq.GetArg<SfxBoolItem>(SID_OPEN_NEW_VIEW);
        if ( pNewViewItem && pNewViewItem->GetValue() )
            aTarget = "_blank" ;
    }

    if ( bHidden )
    {
        aTarget = "_blank";
        DBG_ASSERT( rReq.IsSynchronCall() || pLinkItem, "Hidden load process must be done synchronously!" );
    }

    Reference < XController > xController;
    // if a frame is given, it must be used for the starting point of the targeting mechanism
    // this code is also used if asynchronous loading is possible, because loadComponent always is synchron
    if ( !xTargetFrame.is() )
    {
        if ( pTargetFrame )
        {
            xTargetFrame = pTargetFrame->GetFrameInterface();
        }
        else
        {
            xTargetFrame = Desktop::create(::comphelper::getProcessComponentContext());
        }
    }

    // make URL ready
    const SfxStringItem* pURLItem = rReq.GetArg<SfxStringItem>(SID_FILE_NAME);
    aFileName = pURLItem->GetValue();
    if( aFileName.startsWith("#") ) // Mark without URL
    {
        SfxViewFrame *pView = pTargetFrame ? pTargetFrame->GetCurrentViewFrame() : nullptr;
        if (!pView)
            pView = SfxViewFrame::Current();
        if (pView)
            pView->GetViewShell()->JumpToMark( aFileName.copy(1) );
        rReq.SetReturnValue( SfxViewFrameItem( pView ) );
        return;
    }

    // convert items to properties for framework API calls
    Sequence < PropertyValue > aArgs;
    TransformItems( SID_OPENDOC, *rReq.GetArgs(), aArgs );
    // Any Referer (that was relevant in the above call to
    // SvtSecurityOptions::isSecureMacroUri) is no longer relevant, assuming
    // this "open" request is initiated directly by the user:
    auto pArg = std::find_if(std::cbegin(aArgs), std::cend(aArgs),
        [](const PropertyValue& rArg) { return rArg.Name == "Referer"; });
    if (pArg != std::cend(aArgs))
    {
        auto nIndex = static_cast<sal_Int32>(std::distance(std::cbegin(aArgs), pArg));
        comphelper::removeElementAt(aArgs, nIndex);
    }

    // TODO/LATER: either remove LinkItem or create an asynchronous process for it
    if( bHidden || pLinkItem || rReq.IsSynchronCall() )
    {
        // if loading must be done synchron, we must wait for completion to get a return value
        // find frame by myself; I must know the exact frame to get the controller for the return value from it
        Reference < XComponent > xComp;

        try
        {
            xComp = ::comphelper::SynchronousDispatch::dispatch( xTargetFrame, aFileName, aTarget, aArgs );
        }
        catch(const RuntimeException&)
        {
            throw;
        }
        catch(const css::uno::Exception&)
        {
        }

        Reference < XModel > xModel( xComp, UNO_QUERY );
        if ( xModel.is() )
            xController = xModel->getCurrentController();
        else
            xController.set( xComp, UNO_QUERY );

    }
    else
    {
        URL aURL;
        aURL.Complete = aFileName;
        Reference< util::XURLTransformer > xTrans( util::URLTransformer::create( ::comphelper::getProcessComponentContext() ) );
        xTrans->parseStrict( aURL );

        Reference < XDispatchProvider > xProv( xTargetFrame, UNO_QUERY );
        Reference < XDispatch > xDisp = xProv.is() ? xProv->queryDispatch( aURL, aTarget, FrameSearchFlag::ALL ) : Reference < XDispatch >();
        if ( xDisp.is() )
            xDisp->dispatch( aURL, aArgs );
    }

    if ( xController.is() )
    {
        // try to find the SfxFrame for the controller
        SfxFrame* pCntrFrame = nullptr;
        for ( SfxViewShell* pShell = SfxViewShell::GetFirst( false ); pShell; pShell = SfxViewShell::GetNext( *pShell, false ) )
        {
            if ( pShell->GetController() == xController )
            {
                pCntrFrame = &pShell->GetViewFrame().GetFrame();
                break;
            }
        }

        if ( pCntrFrame )
        {
            SfxObjectShell* pSh = pCntrFrame->GetCurrentDocument();
            DBG_ASSERT( pSh, "Controller without ObjectShell ?!" );

            rReq.SetReturnValue( SfxViewFrameItem( pCntrFrame->GetCurrentViewFrame() ) );
        }
    }

    if (pLinkItem)
    {
        const SfxPoolItem* pRetValue(rReq.GetReturnValue().getItem());
        if (pRetValue)
        {
            pLinkItem->GetValue().Call(pRetValue);
        }
    }
}

void SfxApplication::OpenRemoteExec_Impl( SfxRequest& rReq )
{
    rReq.AppendItem( SfxBoolItem( SID_REMOTE_DIALOG, true ) );
    GetDispatcher_Impl()->Execute( SID_OPENDOC, SfxCallMode::SYNCHRON, *rReq.GetArgs() );
}

void SfxApplication::OpenFromGoogleDriveExec_Impl( SfxRequest& rReq )
{
    SAL_WARN("sfx.appl", "OpenFromGoogleDriveExec_Impl called");
    fprintf(stderr, "\n\n*** GOOGLE DRIVE MENU CLICKED ***\n\n");
    
    FILE* menuLog = fopen("/tmp/gdrive_menu_clicked.log", "w");
    if (menuLog) {
        fprintf(menuLog, "Google Drive menu item clicked at %s\n", __TIME__);
        fclose(menuLog);
    }
    
    // Show Google Drive file picker dialog
    try
    {
        uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
        uno::Reference<lang::XMultiComponentFactory> xFactory = xContext->getServiceManager();
        
        // First ensure CUI library is loaded by creating the GetCreateDialogFactoryService
        // This is necessary because GoogleDriveFilePicker is in the CUI module
        try {
            uno::Reference<uno::XInterface> xCuiLoader(
                xFactory->createInstanceWithContext(
                    u"com.sun.star.cui.GetCreateDialogFactoryService"_ustr,
                    xContext));
        } catch (...) {
            // Ignore - this is just to ensure the library is loaded
        }
        
        // Create the Google Drive picker service
        SAL_WARN("sfx.appl", "Creating GoogleDriveFilePicker service...");
        uno::Reference<ui::dialogs::XFilePicker3> xPicker(
            xFactory->createInstanceWithArgumentsAndContext(
                u"com.sun.star.ui.dialogs.GoogleDriveFilePicker"_ustr,
                uno::Sequence<uno::Any>(),
                xContext), 
            uno::UNO_QUERY);
        
        if (xPicker.is())
        {
            SAL_WARN("sfx.appl", "GoogleDriveFilePicker service created successfully");
            // Execute the picker
            if (xPicker->execute() == ui::dialogs::ExecutableDialogResults::OK)
            {
                // Get selected file and open it
                uno::Sequence<OUString> aFiles = xPicker->getSelectedFiles();
                if (aFiles.hasElements())
                {
                    // Open the selected file
                    SfxStringItem aFileItem(SID_FILE_NAME, aFiles[0]);
                    SfxBoolItem aAsyncItem(SID_ASYNCHRON, false);
                    rReq.SetArgs(SfxAllItemSet(SfxGetpApp()->GetPool()));
                    rReq.AppendItem(aFileItem);
                    rReq.AppendItem(aAsyncItem);
                    GetDispatcher_Impl()->Execute(SID_OPENDOC, SfxCallMode::SYNCHRON, *rReq.GetArgs());
                }
            }
        }
        else
        {
            // Service not available - show error
            SAL_WARN("sfx.appl", "GoogleDriveFilePicker service not found - com.sun.star.ui.dialogs.GoogleDriveFilePicker");
            fprintf(stderr, "\n*** GoogleDriveFilePicker service NOT FOUND ***\n");
            weld::Window* pParent = GetTopWindow();
            std::unique_ptr<weld::MessageDialog> xErrorBox(
                Application::CreateMessageDialog(pParent,
                                               VclMessageType::Error, VclButtonsType::Ok,
                                               u"Google Drive file picker service not available.\n"
                                               "Please check your installation."_ustr));
            xErrorBox->run();
        }
    }
    catch (const uno::Exception& ex)
    {
        // Handle errors - show error dialog with detailed information
        SAL_WARN("sfx.appl", "GoogleDriveFilePicker exception: " << ex.Message);
        weld::Window* pParent = GetTopWindow();
        std::unique_ptr<weld::MessageDialog> xErrorBox(
            Application::CreateMessageDialog(pParent,
                                           VclMessageType::Error, VclButtonsType::Ok,
                                           OUString("Failed to open Google Drive picker: ") + ex.Message));
        xErrorBox->run();
    }
}

void SfxApplication::SignPDFExec_Impl(SfxRequest& rReq)
{
    rReq.AppendItem(SfxBoolItem(SID_SIGNPDF, true));
    GetDispatcher_Impl()->Execute(SID_OPENDOC, SfxCallMode::SYNCHRON, *rReq.GetArgs());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
