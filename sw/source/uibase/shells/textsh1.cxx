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

#include <sal/config.h>

#include <config_features.h>

#include <com/sun/star/i18n/WordType.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/linguistic2/XSearchableDictionaryList.hpp>
#include <com/sun/star/linguistic2/XThesaurus.hpp>
#include <com/sun/star/text/XContentControlsSupplier.hpp>

#include <hintids.hxx>
#include <cmdid.h>
#include <comphelper/lok.hxx>
#include <comphelper/propertysequence.hxx>

#include <i18nutil/unicode.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <svtools/langtab.hxx>
#include <svl/numformat.hxx>
#include <svl/slstitm.hxx>
#include <svl/stritem.hxx>
#include <sfx2/htmlmode.hxx>
#include <svl/whiter.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/namedcolor.hxx>
#include <sfx2/viewfrm.hxx>
#include <vcl/unohelp2.hxx>
#include <vcl/weld.hxx>
#include <sfx2/lokhelper.hxx>
#include <sfx2/request.hxx>
#include <svl/eitem.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/colritem.hxx>
#include <editeng/tstpitem.hxx>
#include <editeng/brushitem.hxx>
#include <editeng/svxacorr.hxx>
#include <svl/cjkoptions.hxx>
#include <svl/ctloptions.hxx>
#include <IDocumentDrawModelAccess.hxx>
#include <IDocumentSettingAccess.hxx>
#include <charfmt.hxx>
#include <svx/SmartTagItem.hxx>
#include <svx/xflgrit.hxx>
#include <svx/xflhtit.hxx>
#include <svx/xfillit0.hxx>
#include <fmtinfmt.hxx>
#include <wrtsh.hxx>
#include <wview.hxx>
#include <swmodule.hxx>
#include <viewopt.hxx>
#include <uitool.hxx>
#include <textsh.hxx>
#include <IMark.hxx>
#include <swdtflvr.hxx>
#include <swundo.hxx>
#include <reffld.hxx>
#include <textcontentcontrol.hxx>
#include <txatbase.hxx>
#include <docsh.hxx>
#include <inputwin.hxx>
#include <chrdlgmodes.hxx>
#include <fmtcol.hxx>
#include <cellatr.hxx>
#include <edtwin.hxx>
#include <fldmgr.hxx>
#include <ndtxt.hxx>
#include <strings.hrc>
#include <paratr.hxx>
#include <vcl/svapp.hxx>
#include <sfx2/app.hxx>
#include <breakit.hxx>
#include <SwSmartTagMgr.hxx>
#include <editeng/acorrcfg.hxx>
#include <swabstdlg.hxx>
#include <sfx2/sfxdlg.hxx>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/style/XStyleFamiliesSupplier.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/linguistic2/ProofreadingResult.hpp>
#include <com/sun/star/linguistic2/XDictionary.hpp>
#include <whisper/WhisperManager.hxx>
#include <whisper/WhisperSettingsDialog.hxx>
#include <com/sun/star/linguistic2/XSpellAlternatives.hpp>
#include <editeng/unolingu.hxx>
#include <doc.hxx>
#include <drawdoc.hxx>
#include <view.hxx>
#include <pam.hxx>
#include <sfx2/objface.hxx>
#include <langhelper.hxx>
#include <uiitems.hxx>
#include <svx/nbdtmgfact.hxx>
#include <svx/nbdtmg.hxx>
#include <SwRewriter.hxx>
#include <svx/drawitem.hxx>
#include <numrule.hxx>
#include <memory>
#include <xmloff/odffields.hxx>
#include <bookmark.hxx>
#include <linguistic/misc.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <comphelper/scopeguard.hxx>
#include <authfld.hxx>
#include <config_wasm_strip.h>
#if HAVE_FEATURE_CURL && !ENABLE_WASM_STRIP_EXTRA
#include <officecfg/Office/Common.hxx>
#include <officecfg/Office/Linguistic.hxx>
#include <svl/visitem.hxx>
#include <translatelangselect.hxx>
#endif // HAVE_FEATURE_CURL && ENABLE_WASM_STRIP_EXTRA
#include <translatehelper.hxx>
#include <IDocumentContentOperations.hxx>
#include <IDocumentUndoRedo.hxx>
#include <fmtcntnt.hxx>
#include <fmtrfmrk.hxx>
#include <cntfrm.hxx>
#include <flyfrm.hxx>
#include <unoprnms.hxx>
#include <boost/property_tree/json_parser.hpp>
#include <formatcontentcontrol.hxx>
#include <rtl/uri.hxx>
#include <unotxdoc.hxx>
#include <expfld.hxx>
#include <sax/tools/converter.hxx>

#include <com/sun/star/text/XTextEmbeddedObjectsSupplier.hpp>
#include <com/sun/star/chart2/XInternalDataProvider.hpp>
#include <com/sun/star/chart2/XChartDocument.hpp>
#include <com/sun/star/chart/XChartDocument.hpp>
#include <com/sun/star/chart/XChartDataArray.hpp>
#include <com/sun/star/chart2/XTitle.hpp>
#include <com/sun/star/chart2/XTitled.hpp>
#include <com/sun/star/chart/ChartDataRowSource.hpp>
#include <com/sun/star/util/XModifiable.hpp>

#include <com/sun/star/chart2/XCoordinateSystemContainer.hpp>
#include <com/sun/star/chart2/XChartTypeContainer.hpp>
#include <com/sun/star/chart2/XDataSeriesContainer.hpp>
#include <com/sun/star/util/XCloneable.hpp>

#include <com/sun/star/util/SearchAlgorithms2.hpp>
#include <com/sun/star/document/XDocumentProperties2.hpp>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>

#include <com/sun/star/beans/XPropertyAccess.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>

using namespace ::com::sun::star;
using namespace com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace com::sun::star::style;
using namespace svx::sidebar;

static void sw_CharDialogResult(const SfxItemSet* pSet, SwWrtShell &rWrtSh, std::shared_ptr<SfxItemSet> const & pCoreSet, bool bSel,
                                bool bSelectionPut, bool bApplyToParagraph, SfxRequest *pReq);

static void sw_CharDialog(SwWrtShell& rWrtSh, bool bUseDialog, bool bApplyToParagraph,
                          sal_uInt16 nSlot, const SfxItemSet* pArgs, SfxRequest* pReq)
{
    FieldUnit eMetric = ::GetDfltMetric(dynamic_cast<SwWebView*>( &rWrtSh.GetView()) != nullptr );
    SwModule::get()->PutItem(SfxUInt16Item(SID_ATTR_METRIC, static_cast<sal_uInt16>(eMetric)));
    auto pCoreSet = std::make_shared<SfxItemSetFixed<
            RES_CHRATR_BEGIN, RES_CHRATR_END - 1,
            RES_TXTATR_INETFMT, RES_TXTATR_INETFMT,
            RES_BACKGROUND, RES_SHADOW,
            SID_ATTR_BORDER_INNER, SID_ATTR_BORDER_INNER,
            SID_HTML_MODE, SID_HTML_MODE,
            SID_ATTR_CHAR_WIDTH_FIT_TO_LINE, SID_ATTR_CHAR_WIDTH_FIT_TO_LINE,
            FN_PARAM_SELECTION, FN_PARAM_SELECTION>> ( rWrtSh.GetView().GetPool() );
    rWrtSh.GetCurAttr(*pCoreSet);

    bool bSel = rWrtSh.HasSelection();
    bool bSelectionPut = false;
    if(bSel || rWrtSh.IsInWord())
    {
        if(!bSel)
        {
            rWrtSh.StartAction();
            rWrtSh.Push();
            if(!rWrtSh.SelectTextAttr( RES_TXTATR_INETFMT ))
                rWrtSh.SelWrd();
        }
        pCoreSet->Put(SfxStringItem(FN_PARAM_SELECTION, rWrtSh.GetSelText()));
        bSelectionPut = true;
        if(!bSel)
        {
            rWrtSh.Pop(SwCursorShell::PopMode::DeleteCurrent);
            rWrtSh.EndAction();
        }
    }
    pCoreSet->Put(SfxUInt16Item(SID_ATTR_CHAR_WIDTH_FIT_TO_LINE, rWrtSh.GetScalingOfSelectedText()));

    ::ConvertAttrCharToGen(*pCoreSet);

    // Setting the BoxInfo
    ::PrepareBoxInfo(*pCoreSet, rWrtSh);

    pCoreSet->Put(SfxUInt16Item(SID_HTML_MODE, ::GetHtmlMode(rWrtSh.GetView().GetDocShell())));
    VclPtr<SfxAbstractTabDialog> pDlg;
    if ( bUseDialog && GetActiveView() )
    {
        SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
        pDlg.reset(pFact->CreateSwCharDlg(rWrtSh.GetView().GetFrameWeld(), rWrtSh.GetView(), *pCoreSet, SwCharDlgMode::Std));

        if (nSlot == SID_CHAR_DLG_EFFECT)
            pDlg->SetCurPageId(u"fonteffects"_ustr);
        else if (nSlot == SID_CHAR_DLG_POSITION)
            pDlg->SetCurPageId(u"position"_ustr);
        else if (nSlot == SID_CHAR_DLG_FOR_PARAGRAPH)
            pDlg->SetCurPageId(u"font"_ustr);
        else if (pReq)
        {
            const SfxStringItem* pItem = (*pReq).GetArg<SfxStringItem>(FN_PARAM_1);
            if (pItem)
                pDlg->SetCurPageId(pItem->GetValue());
        }
    }

    if (bUseDialog)
    {
        std::shared_ptr<SfxRequest> pRequest;
        if (pReq)
        {
            pRequest = std::make_shared<SfxRequest>(*pReq);
            pReq->Ignore(); // the 'old' request is not relevant any more
        }
        pDlg->StartExecuteAsync([pDlg, &rWrtSh, pCoreSet=std::move(pCoreSet), bSel,
                                 bSelectionPut, bApplyToParagraph,
                                 pRequest=std::move(pRequest)](sal_Int32 nResult){
            if (nResult == RET_OK)
            {
                sw_CharDialogResult(pDlg->GetOutputItemSet(), rWrtSh, pCoreSet, bSel, bSelectionPut,
                                    bApplyToParagraph, pRequest.get());
            }
            pDlg->disposeOnce();
        });
    }
    else if (pArgs)
    {
        sw_CharDialogResult(pArgs, rWrtSh, pCoreSet, bSel, bSelectionPut, bApplyToParagraph, pReq);
    }
}

static void sw_CharDialogResult(const SfxItemSet* pSet, SwWrtShell& rWrtSh, std::shared_ptr<SfxItemSet> const & pCoreSet, bool bSel,
                                bool bSelectionPut, bool bApplyToParagraph, SfxRequest* pReq)
{
    SfxItemSet aTmpSet( *pSet );
    ::ConvertAttrGenToChar(aTmpSet, *pCoreSet);

    const bool bWasLocked = rWrtSh.IsViewLocked();
    if (bApplyToParagraph)
    {
        rWrtSh.StartAction();
        rWrtSh.LockView(true);
        rWrtSh.Push();
        SwLangHelper::SelectCurrentPara(rWrtSh);
    }

    const SfxStringItem* pSelectionItem;
    bool bInsert = false;
    sal_Int32 nInsert = 0;

    // The old item is for unknown reasons back in the set again.
    if( !bSelectionPut && (pSelectionItem = aTmpSet.GetItemIfSet(FN_PARAM_SELECTION, false)) )
    {
        const OUString& sInsert = pSelectionItem->GetValue();
        bInsert = !sInsert.isEmpty();
        if(bInsert)
        {
            nInsert = sInsert.getLength();
            rWrtSh.StartAction();
            rWrtSh.Insert( sInsert );
            rWrtSh.SetMark();
            rWrtSh.ExtendSelection(false, sInsert.getLength());
            SfxRequest aReq(rWrtSh.GetView().GetViewFrame(), FN_INSERT_STRING);
            aReq.AppendItem( SfxStringItem( FN_INSERT_STRING, sInsert ) );
            aReq.Done();
            SfxRequest aReq1(rWrtSh.GetView().GetViewFrame(), FN_CHAR_LEFT);
            aReq1.AppendItem( SfxInt32Item(FN_PARAM_MOVE_COUNT, nInsert) );
            aReq1.AppendItem( SfxBoolItem(FN_PARAM_MOVE_SELECTION, true) );
            aReq1.Done();
        }
    }
    aTmpSet.ClearItem(FN_PARAM_SELECTION);

    SwTextFormatColl* pColl = rWrtSh.GetCurTextFormatColl();
    if(bSel && rWrtSh.IsSelFullPara() && pColl && pColl->IsAutoUpdateOnDirectFormat())
    {
        rWrtSh.AutoUpdatePara(pColl, aTmpSet);
    }
    else
        rWrtSh.SetAttrSet( aTmpSet );
    if (pReq)
        pReq->Done(aTmpSet);
    if(bInsert)
    {
        SfxRequest aReq1(rWrtSh.GetView().GetViewFrame(), FN_CHAR_RIGHT);
        aReq1.AppendItem( SfxInt32Item(FN_PARAM_MOVE_COUNT, nInsert) );
        aReq1.AppendItem( SfxBoolItem(FN_PARAM_MOVE_SELECTION, false) );
        aReq1.Done();
        rWrtSh.SwapPam();
        rWrtSh.ClearMark();
        rWrtSh.DontExpandFormat();
        rWrtSh.EndAction();
    }

    if (bApplyToParagraph)
    {
        rWrtSh.Pop(SwCursorShell::PopMode::DeleteCurrent);
        rWrtSh.LockView(bWasLocked);
        rWrtSh.EndAction();
    }
}


static void sw_ParagraphDialogResult(SfxItemSet* pSet, SwWrtShell &rWrtSh, SfxRequest& rReq, SwPaM* pPaM)
{
    if (!pSet)
        return;

    rReq.Done( *pSet );
    ::SfxToSwPageDescAttr( rWrtSh, *pSet );
    // #i56253#
    // enclose all undos.
    // Thus, check conditions, if actions will be performed.
    const bool bUndoNeeded( pSet->Count() ||
            SfxItemState::SET == pSet->GetItemState(FN_NUMBER_NEWSTART) ||
            SfxItemState::SET == pSet->GetItemState(FN_NUMBER_NEWSTART_AT) );
    if ( bUndoNeeded )
    {
        rWrtSh.StartUndo( SwUndoId::INSATTR );
    }
    if( pSet->Count() )
    {
        rWrtSh.StartAction();
        if ( const SfxStringItem* pDropTextItem = pSet->GetItemIfSet(FN_DROP_TEXT, false) )
        {
            if ( !pDropTextItem->GetValue().isEmpty() )
                rWrtSh.ReplaceDropText(pDropTextItem->GetValue(), pPaM);
        }
        rWrtSh.SetAttrSet(*pSet, SetAttrMode::DEFAULT, pPaM);
        rWrtSh.EndAction();
        SwTextFormatColl* pColl = rWrtSh.GetPaMTextFormatColl(pPaM);
        if(pColl && pColl->IsAutoUpdateOnDirectFormat())
        {
            rWrtSh.AutoUpdatePara(pColl, *pSet, pPaM);
        }
    }

    if( SfxItemState::SET == pSet->GetItemState(FN_NUMBER_NEWSTART) )
    {
        //SetNumRuleStart(true) restarts the numbering at the value
        //that is defined at the starting point of the numbering level
        //otherwise the SetNodeNumStart() value determines the start
        //if it's set to something different than USHRT_MAX

        bool bStart = static_cast<const SfxBoolItem&>(pSet->Get(FN_NUMBER_NEWSTART)).GetValue();

        // Default value for restart value has to be USHRT_MAX
        // in order to indicate that the restart value of the list
        // style has to be used on restart.
        sal_uInt16 nNumStart = USHRT_MAX;
        if( SfxItemState::SET == pSet->GetItemState(FN_NUMBER_NEWSTART_AT) )
        {
            nNumStart = pSet->Get(FN_NUMBER_NEWSTART_AT).GetValue();
        }
        rWrtSh.SetNumRuleStart(bStart, pPaM);
        rWrtSh.SetNodeNumStart(nNumStart);
    }
    else if( SfxItemState::SET == pSet->GetItemState(FN_NUMBER_NEWSTART_AT) )
    {
        rWrtSh.SetNodeNumStart(pSet->Get(FN_NUMBER_NEWSTART_AT).GetValue());
        rWrtSh.SetNumRuleStart(false, pPaM);
    }
    // #i56253#
    if ( bUndoNeeded )
    {
        rWrtSh.EndUndo( SwUndoId::INSATTR );
    }
}

namespace {

void InsertBreak(SwWrtShell& rWrtSh,
                 sal_uInt16 nKind,
                 ::std::optional<sal_uInt16> oPageNumber,
                 const UIName& rTemplateName, std::optional<SwLineBreakClear> oClear)
{
    switch ( nKind )
    {
        case 1 :
            rWrtSh.InsertLineBreak(oClear);
            break;
        case 2 :
            rWrtSh.InsertColumnBreak(); break;
        case 3 :
        {
            rWrtSh.StartAllAction();
            if( !rTemplateName.isEmpty() )
                rWrtSh.InsertPageBreak( &rTemplateName, oPageNumber );
            else
                rWrtSh.InsertPageBreak();
            rWrtSh.EndAllAction();
        }
    }
}

OUString GetLocalURL(const SwWrtShell& rSh)
{
    SwField* pField = rSh.GetCurField();
    if (!pField)
    {
        return OUString();
    }

    if (pField->GetTyp()->Which() != SwFieldIds::TableOfAuthorities)
    {
        return OUString();
    }

    const auto& rAuthorityField = *static_cast<const SwAuthorityField*>(pField);
    SwAuthEntry* pAuthEntry = rAuthorityField.GetAuthEntry();
    if (!pAuthEntry)
    {
        return OUString();
    }

    const OUString& rLocalURL = pAuthEntry->GetAuthorField(AUTH_FIELD_LOCAL_URL);
    return rLocalURL;
}

void UpdateSections(const SfxRequest& rReq, SwWrtShell& rWrtSh)
{
    OUString aSectionNamePrefix;
    const SfxStringItem* pSectionNamePrefix = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
    if (pSectionNamePrefix)
    {
        aSectionNamePrefix = pSectionNamePrefix->GetValue();
    }

    uno::Sequence<beans::PropertyValues> aSections;
    const SfxUnoAnyItem* pSections = rReq.GetArg<SfxUnoAnyItem>(FN_PARAM_2);
    if (pSections)
    {
        pSections->GetValue() >>= aSections;
    }

    rWrtSh.GetDoc()->GetIDocumentUndoRedo().StartUndo(SwUndoId::UPDATE_SECTIONS, nullptr);
    rWrtSh.StartAction();

    SwDoc* pDoc = rWrtSh.GetDoc();
    sal_Int32 nSectionIndex = 0;
    const SwSectionFormats& rFormats = pDoc->GetSections();
    IDocumentContentOperations& rIDCO = pDoc->getIDocumentContentOperations();
    for (size_t i = 0; i < rFormats.size(); ++i)
    {
        const SwSectionFormat* pFormat = rFormats[i];
        if (!pFormat->GetName().toString().startsWith(aSectionNamePrefix))
        {
            continue;
        }

        if (nSectionIndex >= aSections.getLength())
        {
            break;
        }

        comphelper::SequenceAsHashMap aMap(aSections[nSectionIndex++]);
        UIName aSectionName( aMap[u"RegionName"_ustr].get<OUString>() );
        if (aSectionName != pFormat->GetName())
        {
            const_cast<SwSectionFormat*>(pFormat)->SetFormatName(aSectionName, /*bBroadcast=*/true);
            SwSectionData aSectionData(*pFormat->GetSection());
            aSectionData.SetSectionName(aSectionName);
            pDoc->UpdateSection(i, aSectionData);
        }

        const SwFormatContent& rContent = pFormat->GetContent();
        const SwNodeIndex* pContentNodeIndex = rContent.GetContentIdx();
        if (pContentNodeIndex)
        {
            SwPaM aSectionStart(SwPosition{*pContentNodeIndex});
            aSectionStart.Move(fnMoveForward, GoInContent);
            SwPaM* pCursorPos = rWrtSh.GetCursor();
            *pCursorPos = aSectionStart;
            rWrtSh.EndOfSection(/*bSelect=*/true);
            rIDCO.DeleteAndJoin(*pCursorPos);
            rWrtSh.EndSelect();

            OUString aSectionText = aMap[u"Content"_ustr].get<OUString>();
            SwTranslateHelper::PasteHTMLToPaM(rWrtSh, pCursorPos, aSectionText.toUtf8());
        }
    }

    rWrtSh.EndAction();
    rWrtSh.GetDoc()->GetIDocumentUndoRedo().EndUndo(SwUndoId::UPDATE_SECTIONS, nullptr);
}

void DeleteSections(const SfxRequest& rReq, SwWrtShell& rWrtSh)
{
    OUString aSectionNamePrefix;
    const SfxStringItem* pSectionNamePrefix = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
    if (pSectionNamePrefix)
    {
        aSectionNamePrefix = pSectionNamePrefix->GetValue();
    }

    rWrtSh.GetDoc()->GetIDocumentUndoRedo().StartUndo(SwUndoId::DELETE_SECTIONS, nullptr);
    rWrtSh.StartAction();
    comphelper::ScopeGuard g(
        [&rWrtSh]
        {
            rWrtSh.EndAction();
            rWrtSh.GetDoc()->GetIDocumentUndoRedo().EndUndo(SwUndoId::DELETE_SECTIONS, nullptr);
        });

    SwDoc* pDoc = rWrtSh.GetDoc();
    std::vector<SwSectionFormat*> aRemovals;
    for (SwSectionFormat* pFormat : pDoc->GetSections())
        if (aSectionNamePrefix.isEmpty() || pFormat->GetName().toString().startsWith(aSectionNamePrefix))
            aRemovals.push_back(pFormat);

    for (const auto& pFormat : aRemovals)
    {
        // Just delete the format, not the content of the section.
        pDoc->DelSectionFormat(pFormat);
    }
}

void DeleteContentControl( const SwWrtShell& rWrtSh )
{
    SwTextContentControl* pTextContentControl = rWrtSh.CursorInsideContentControl();
    if (pTextContentControl) {
        const SwFormatContentControl& rFormatContentControl = pTextContentControl->GetContentControl();
        const std::shared_ptr<SwContentControl>& pContentControl = rFormatContentControl.GetContentControl();
        pContentControl->SetReadWrite(true);
        pTextContentControl->Delete(true);
    }
}


void UpdateBookmarks(const SfxRequest& rReq, SwWrtShell& rWrtSh)
{
    if (rWrtSh.getIDocumentSettingAccess().get(DocumentSettingId::PROTECT_BOOKMARKS))
    {
        return;
    }

    OUString aBookmarkNamePrefix;
    const SfxStringItem* pBookmarkNamePrefix = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
    if (pBookmarkNamePrefix)
    {
        aBookmarkNamePrefix = pBookmarkNamePrefix->GetValue();
    }

    uno::Sequence<beans::PropertyValues> aBookmarks;
    const SfxUnoAnyItem* pBookmarks = rReq.GetArg<SfxUnoAnyItem>(FN_PARAM_2);
    if (pBookmarks)
    {
        pBookmarks->GetValue() >>= aBookmarks;
    }

    rWrtSh.GetDoc()->GetIDocumentUndoRedo().StartUndo(SwUndoId::UPDATE_BOOKMARKS, nullptr);
    rWrtSh.StartAction();

    IDocumentMarkAccess& rIDMA = *rWrtSh.GetDoc()->getIDocumentMarkAccess();
    sal_Int32 nBookmarkIndex = 0;
    bool bSortMarks = false;
    for (auto it = rIDMA.getBookmarksBegin(); it != rIDMA.getBookmarksEnd(); ++it)
    {
        sw::mark::Bookmark* pMark = *it;
        assert(pMark);
        if (!pMark->GetName().toString().startsWith(aBookmarkNamePrefix))
        {
            continue;
        }

        if (aBookmarks.getLength() <= nBookmarkIndex)
        {
            continue;
        }

        comphelper::SequenceAsHashMap aMap(aBookmarks[nBookmarkIndex++]);
        if (aMap[u"Bookmark"_ustr].get<OUString>() != pMark->GetName())
        {
            rIDMA.renameMark(pMark, SwMarkName(aMap[u"Bookmark"_ustr].get<OUString>()));
        }

        OUString aBookmarkText = aMap[u"BookmarkText"_ustr].get<OUString>();

        // Insert markers to remember where the paste positions are.
        SwPaM aMarkers(pMark->GetMarkEnd());
        IDocumentContentOperations& rIDCO = rWrtSh.GetDoc()->getIDocumentContentOperations();
        bool bSuccess = rIDCO.InsertString(aMarkers, u"XY"_ustr);
        if (bSuccess)
        {
            SwPaM aPasteEnd(pMark->GetMarkEnd());
            aPasteEnd.Move(fnMoveForward, GoInContent);

            // Paste HTML content.
            SwPaM* pCursorPos = rWrtSh.GetCursor();
            *pCursorPos = aPasteEnd;
            SwTranslateHelper::PasteHTMLToPaM(rWrtSh, pCursorPos, aBookmarkText.toUtf8());

            // Update the bookmark to point to the new content.
            SwPaM aPasteStart(pMark->GetMarkEnd());
            aPasteStart.Move(fnMoveForward, GoInContent);
            SwPaM aStartMarker(pMark->GetMarkStart(), *aPasteStart.GetPoint());
            SwPaM aEndMarker(*aPasteEnd.GetPoint(), *aPasteEnd.GetPoint());
            aEndMarker.GetMark()->AdjustContent(1);
            pMark->SetMarkPos(*aPasteStart.GetPoint());
            pMark->SetOtherMarkPos(*aPasteEnd.GetPoint());
            bSortMarks = true;

            // Remove markers. the start marker includes the old content as well.
            rIDCO.DeleteAndJoin(aStartMarker);
            rIDCO.DeleteAndJoin(aEndMarker);
        }
    }
    if (bSortMarks)
    {
        rIDMA.assureSortedMarkContainers();
    }

    rWrtSh.EndAction();
    rWrtSh.GetDoc()->GetIDocumentUndoRedo().EndUndo(SwUndoId::UPDATE_BOOKMARKS, nullptr);
}

void UpdateBookmark(const SfxRequest& rReq, SwWrtShell& rWrtSh)
{
    if (rWrtSh.getIDocumentSettingAccess().get(DocumentSettingId::PROTECT_BOOKMARKS))
    {
        return;
    }

    OUString aBookmarkNamePrefix;
    const SfxStringItem* pBookmarkNamePrefix = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
    if (pBookmarkNamePrefix)
    {
        aBookmarkNamePrefix = pBookmarkNamePrefix->GetValue();
    }

    uno::Sequence<beans::PropertyValue> aBookmark;
    const SfxUnoAnyItem* pBookmarks = rReq.GetArg<SfxUnoAnyItem>(FN_PARAM_2);
    if (pBookmarks)
    {
        pBookmarks->GetValue() >>= aBookmark;
    }

    IDocumentMarkAccess& rIDMA = *rWrtSh.GetDoc()->getIDocumentMarkAccess();
    SwPosition& rCursor = *rWrtSh.GetCursor()->GetPoint();
    sw::mark::Bookmark* pBookmark = rIDMA.getOneInnermostBookmarkFor(rCursor);
    if (!pBookmark || !pBookmark->GetName().toString().startsWith(aBookmarkNamePrefix))
    {
        return;
    }

    SwRewriter aRewriter;
    aRewriter.AddRule(UndoArg1, pBookmark->GetName());
    rWrtSh.GetDoc()->GetIDocumentUndoRedo().StartUndo(SwUndoId::UPDATE_BOOKMARK, &aRewriter);
    rWrtSh.StartAction();
    comphelper::ScopeGuard g(
        [&rWrtSh, &aRewriter]
        {
            rWrtSh.EndAction();
            rWrtSh.GetDoc()->GetIDocumentUndoRedo().EndUndo(SwUndoId::UPDATE_BOOKMARK, &aRewriter);
        });


    comphelper::SequenceAsHashMap aMap(aBookmark);
    if (aMap[u"Bookmark"_ustr].get<OUString>() != pBookmark->GetName())
    {
        rIDMA.renameMark(pBookmark, SwMarkName(aMap[u"Bookmark"_ustr].get<OUString>()));
    }

    // Insert markers to remember where the paste positions are.
    SwPaM aMarkers(pBookmark->GetMarkEnd());
    IDocumentContentOperations& rIDCO = rWrtSh.GetDoc()->getIDocumentContentOperations();
    if (!rIDCO.InsertString(aMarkers, u"XY"_ustr))
    {
        return;
    }

    SwPaM aPasteEnd(pBookmark->GetMarkEnd());
    aPasteEnd.Move(fnMoveForward, GoInContent);

    OUString aBookmarkText = aMap[u"BookmarkText"_ustr].get<OUString>();

    // Paste HTML content.
    SwPaM* pCursorPos = rWrtSh.GetCursor();
    *pCursorPos = aPasteEnd;
    SwTranslateHelper::PasteHTMLToPaM(rWrtSh, pCursorPos, aBookmarkText.toUtf8());

    // Update the bookmark to point to the new content.
    SwPaM aPasteStart(pBookmark->GetMarkEnd());
    aPasteStart.Move(fnMoveForward, GoInContent);
    SwPaM aStartMarker(pBookmark->GetMarkStart(), *aPasteStart.GetPoint());
    SwPaM aEndMarker(*aPasteEnd.GetPoint(), *aPasteEnd.GetPoint());
    aEndMarker.GetMark()->AdjustContent(1);
    pBookmark->SetMarkPos(*aPasteStart.GetPoint());
    pBookmark->SetOtherMarkPos(*aPasteEnd.GetPoint());

    // Remove markers. the start marker includes the old content as well.
    rIDCO.DeleteAndJoin(aStartMarker);
    rIDCO.DeleteAndJoin(aEndMarker);
    rIDMA.assureSortedMarkContainers();
}

void DeleteBookmarks(const SfxRequest& rReq, SwWrtShell& rWrtSh)
{
    if (rWrtSh.getIDocumentSettingAccess().get(DocumentSettingId::PROTECT_BOOKMARKS))
    {
        return;
    }

    OUString aBookmarkNamePrefix;
    const SfxStringItem* pBookmarkNamePrefix = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
    if (pBookmarkNamePrefix)
    {
        aBookmarkNamePrefix = pBookmarkNamePrefix->GetValue();
    }

    rWrtSh.GetDoc()->GetIDocumentUndoRedo().StartUndo(SwUndoId::DELETE_BOOKMARKS, nullptr);
    rWrtSh.StartAction();
    comphelper::ScopeGuard g(
        [&rWrtSh]
        {
            rWrtSh.EndAction();
            rWrtSh.GetDoc()->GetIDocumentUndoRedo().EndUndo(SwUndoId::DELETE_BOOKMARKS, nullptr);
        });

    IDocumentMarkAccess* pMarkAccess = rWrtSh.GetDoc()->getIDocumentMarkAccess();
    std::vector<sw::mark::MarkBase*> aRemovals;
    for (auto it = pMarkAccess->getBookmarksBegin(); it != pMarkAccess->getBookmarksEnd(); ++it)
    {
        sw::mark::Bookmark* pBookmark = *it;
        assert(pBookmark);

        if (!aBookmarkNamePrefix.isEmpty())
        {
            if (!pBookmark->GetName().toString().startsWith(aBookmarkNamePrefix))
            {
                continue;
            }
        }

        aRemovals.push_back(pBookmark);
    }

    for (const auto& pMark : aRemovals)
    {
        pMarkAccess->deleteMark(pMark);
    }
}

void DeleteFields(const SfxRequest& rReq, SwWrtShell& rWrtSh)
{
    const SfxStringItem* pTypeName = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
    if (!pTypeName || pTypeName->GetValue() != "SetRef")
    {
        // This is implemented so far only for reference marks.
        return;
    }

    OUString aNamePrefix;
    const SfxStringItem* pNamePrefix = rReq.GetArg<SfxStringItem>(FN_PARAM_2);
    if (pNamePrefix)
    {
        aNamePrefix = pNamePrefix->GetValue();
    }

    SwDoc* pDoc = rWrtSh.GetDoc();
    pDoc->GetIDocumentUndoRedo().StartUndo(SwUndoId::DELETE_FIELDS, nullptr);
    rWrtSh.StartAction();
    comphelper::ScopeGuard g(
        [&rWrtSh]
        {
            rWrtSh.EndAction();
            rWrtSh.GetDoc()->GetIDocumentUndoRedo().EndUndo(SwUndoId::DELETE_FIELDS, nullptr);
        });

    std::vector<const SwFormatRefMark*> aRemovals;
    for (sal_uInt16 i = 0; i < pDoc->GetRefMarks(); ++i)
    {
        const SwFormatRefMark* pRefMark = pDoc->GetRefMark(i);
        if (!aNamePrefix.isEmpty())
        {
            if (!pRefMark->GetRefName().toString().startsWith(aNamePrefix))
            {
                continue;
            }
        }

        aRemovals.push_back(pRefMark);
    }

    for (const auto& pMark : aRemovals)
    {
        pDoc->DeleteFormatRefMark(pMark);
    }
}

void lcl_LogWarning(std::string sWarning)
{
    LOK_WARN("sw.transform",  sWarning);
}

bool lcl_ChangeChartColumnCount(const uno::Reference<chart2::XChartDocument>& xChartDoc, sal_Int32 nId,
                                bool bInsert, bool bResize = false)
{
    uno::Reference<chart2::XDiagram> xDiagram = xChartDoc->getFirstDiagram();
    if (!xDiagram.is())
        return false;
    uno::Reference<chart2::XCoordinateSystemContainer> xCooSysContainer(xDiagram, uno::UNO_QUERY);
    if (!xCooSysContainer.is())
        return false;
    uno::Sequence<uno::Reference<chart2::XCoordinateSystem>> xCooSysSequence(
        xCooSysContainer->getCoordinateSystems());
    if (xCooSysSequence.getLength() <= 0)
        return false;
    uno::Reference<chart2::XChartTypeContainer> xChartTypeContainer(xCooSysSequence[0],
                                                                    uno::UNO_QUERY);
    if (!xChartTypeContainer.is())
        return false;
    uno::Sequence<uno::Reference<chart2::XChartType>> xChartTypeSequence(
        xChartTypeContainer->getChartTypes());
    if (xChartTypeSequence.getLength() <= 0)
        return false;
    uno::Reference<chart2::XDataSeriesContainer> xDSContainer(xChartTypeSequence[0],
                                                              uno::UNO_QUERY);
    if (!xDSContainer.is())
        return false;

    uno::Reference<chart2::XInternalDataProvider> xIDataProvider(xChartDoc->getDataProvider(),
                                                                 uno::UNO_QUERY);
    if (!xIDataProvider.is())
        return false;

    uno::Sequence<uno::Reference<chart2::XDataSeries>> aSeriesSeq(xDSContainer->getDataSeries());

    int nSeriesCount = aSeriesSeq.getLength();

    if (bResize)
    {
        // Resize is actually some inserts, or deletes
        if (nId > nSeriesCount)
        {
            bInsert = true;
        }
        else if (nId < nSeriesCount)
        {
            bInsert = false;
        }
        else
        {
            // Resize to the same size. No change needed
            return true;
        }
    }

    // insert or delete
    if (bInsert)
    {
        // insert
        if (nId > nSeriesCount && !bResize)
            return false;

        int nInsertCount = bResize ? nId - nSeriesCount : 1;

        // call dialog code
        if (bResize)
        {
            for (int i = 0; i < nInsertCount; i++)
            {
                xIDataProvider->insertDataSeries(nSeriesCount);
            }
            return true;
        }

        xIDataProvider->insertDataSeries(nId);
    }
    else
    {
        // delete 1 or more columns
        if (nId >= nSeriesCount)
            return false;
        int nDeleteCount = bResize ? nSeriesCount - nId : 1;
        for (int i = 0; i < nDeleteCount; i++)
        {
            xDSContainer->removeDataSeries(aSeriesSeq[nId]);
        }
    }
    return true;
}

bool lcl_ResizeChartColumns(const uno::Reference<chart2::XChartDocument>& xChartDoc, sal_Int32 nSize)
{
    return lcl_ChangeChartColumnCount(xChartDoc, nSize, false, true);
}

bool lcl_InsertChartColumns(const uno::Reference<chart2::XChartDocument>& xChartDoc, sal_Int32 nId)
{
    return lcl_ChangeChartColumnCount(xChartDoc, nId, true);
}

bool lcl_DeleteChartColumns(const uno::Reference<chart2::XChartDocument>& xChartDoc, sal_Int32 nId)
{
    return lcl_ChangeChartColumnCount(xChartDoc, nId, false);
}
}

static bool AddWordToWordbook(const uno::Reference<linguistic2::XDictionary>& xDictionary, SwWrtShell &rWrtSh)
{
    if (!xDictionary)
        return false;

    SwRect aToFill;
    uno::Reference<linguistic2::XSpellAlternatives>  xSpellAlt(rWrtSh.GetCorrection(nullptr, aToFill));
    if (!xSpellAlt.is())
        return false;

    OUString sWord = xSpellAlt->getWord();
    linguistic::DictionaryError nAddRes = linguistic::AddEntryToDic(xDictionary, sWord, false, OUString());
    if (linguistic::DictionaryError::NONE != nAddRes && xDictionary.is() && !xDictionary->getEntry(sWord).is())
    {
        SvxDicError(rWrtSh.GetView().GetFrameWeld(), nAddRes);
        return false;
    }
    return true;
}

void SwTextShell::Execute(SfxRequest &rReq)
{
    bool bUseDialog = true;
    const SfxItemSet *pArgs = rReq.GetArgs();
    SwWrtShell& rWrtSh = GetShell();
    const SfxPoolItem* pItem = nullptr;
    const sal_uInt16 nSlot = rReq.GetSlot();
    if(pArgs)
        pArgs->GetItemState(GetPool().GetWhichIDFromSlotID(nSlot), false, &pItem);
    switch( nSlot )
    {
        case SID_UNICODE_NOTATION_TOGGLE:
        {
            tools::Long nMaxUnits = 256;
            sal_Int32 nSelLength = rWrtSh.GetSelText().getLength();
            if( rWrtSh.IsSelection() && !rWrtSh.IsMultiSelection() && (nSelLength < nMaxUnits) )
                nMaxUnits = nSelLength;

            tools::Long index = 0;
            ToggleUnicodeCodepoint aToggle;
            while( nMaxUnits-- && aToggle.AllowMoreInput(rWrtSh.GetChar(true, index-1)) )
                --index;

            OUString sReplacement = aToggle.ReplacementString();
            if( !sReplacement.isEmpty() )
            {
                if (rWrtSh.HasReadonlySel() && !rWrtSh.CursorInsideInputField())
                {
                    // Only break if there's something to do; don't nag with the dialog otherwise
                    rWrtSh.InfoReadOnlyDialog(false);
                    break;
                }
                OUString stringToReplace = aToggle.StringToReplace();
                SwRewriter aRewriter;
                aRewriter.AddRule( UndoArg1, stringToReplace );
                aRewriter.AddRule( UndoArg2, SwResId(STR_YIELDS) );
                aRewriter.AddRule( UndoArg3, sReplacement );
                rWrtSh.StartUndo(SwUndoId::REPLACE, &aRewriter);
                rWrtSh.GetCursor()->Normalize(false);

                rWrtSh.ClearMark();
                if( rWrtSh.IsInSelect() )  // cancel any in-progress keyboard selection as well
                    rWrtSh.EndSelect();
                // Select exactly what was chosen for replacement
                rWrtSh.GetCursor()->SetMark();
                rWrtSh.GetCursor()->GetPoint()->AdjustContent(-stringToReplace.getLength());
                rWrtSh.DelLeft();
                rWrtSh.Insert2( sReplacement );
                rWrtSh.EndUndo(SwUndoId::REPLACE, &aRewriter);
            }
        }
        break;

        case SID_LANGUAGE_STATUS:
        {
            // get the language
            OUString aNewLangText;
            const SfxStringItem* pItem2 = rReq.GetArg<SfxStringItem>(SID_LANGUAGE_STATUS);
            if (pItem2)
                aNewLangText = pItem2->GetValue();

            //!! Remember the view frame right now...
            //!! (call to GetView().GetViewFrame() will break if the
            //!! SwTextShell got destroyed meanwhile.)
            SfxViewFrame& rViewFrame = GetView().GetViewFrame();

            if (aNewLangText == "*")
            {
                // open the dialog "Tools/Options/Languages and Locales - General"
                // to set the documents default language
                SfxAbstractDialogFactory* pFact = SfxAbstractDialogFactory::Create();
                ScopedVclPtr<VclAbstractDialog> pDlg(pFact->CreateVclDialog(GetView().GetFrameWeld(), SID_LANGUAGE_OPTIONS));
                pDlg->Execute();
            }
            else
            {
                //!! We have to use StartAction / EndAction bracketing in
                //!! order to prevent possible destruction of the SwTextShell
                //!! due to the selection changes coming below.
                rWrtSh.StartAction();
                // prevent view from jumping because of (temporary) selection changes
                rWrtSh.LockView( true );

                // setting the new language...
                if (!aNewLangText.isEmpty())
                {
                    static constexpr OUString aSelectionLangPrefix(u"Current_"_ustr);
                    static constexpr OUString aParagraphLangPrefix(u"Paragraph_"_ustr);
                    static constexpr OUString aDocumentLangPrefix(u"Default_"_ustr);

                    SfxItemSetFixed<RES_CHRATR_LANGUAGE, RES_CHRATR_LANGUAGE,
                                    RES_CHRATR_CJK_LANGUAGE, RES_CHRATR_CJK_LANGUAGE,
                                    RES_CHRATR_CTL_LANGUAGE, RES_CHRATR_CTL_LANGUAGE,
                                    RES_CHRATR_SCRIPT_HINT, RES_CHRATR_SCRIPT_HINT>
                        aCoreSet(GetPool());

                    sal_Int32 nPos = 0;
                    bool bForSelection = true;
                    bool bForParagraph = false;
                    if (-1 != (nPos = aNewLangText.indexOf( aSelectionLangPrefix )))
                    {
                        // ... for the current selection
                        aNewLangText = aNewLangText.replaceAt(nPos, aSelectionLangPrefix.getLength(), u"");
                        bForSelection = true;
                    }
                    else if (-1 != (nPos = aNewLangText.indexOf(aParagraphLangPrefix)))
                    {
                        // ... for the current paragraph language
                        aNewLangText = aNewLangText.replaceAt(nPos, aParagraphLangPrefix.getLength(), u"");
                        bForSelection = true;
                        bForParagraph = true;
                    }
                    else if (-1 != (nPos = aNewLangText.indexOf(aDocumentLangPrefix)))
                    {
                        // ... as default document language
                        aNewLangText = aNewLangText.replaceAt(nPos, aDocumentLangPrefix.getLength(), u"");
                        bForSelection = false;
                    }

                    if (bForParagraph || !bForSelection)
                    {
                        rWrtSh.Push(); // save selection for later restoration
                        rWrtSh.ClearMark(); // fdo#67796: invalidate table crsr
                    }

                    if (bForParagraph)
                        SwLangHelper::SelectCurrentPara( rWrtSh );

                    if (!bForSelection) // document language to be changed...
                    {
                        rWrtSh.SelAll();
                        rWrtSh.ExtendedSelectAll();
                    }

                    rWrtSh.StartUndo( ( !bForParagraph && !bForSelection ) ? SwUndoId::SETDEFTATTR : SwUndoId::EMPTY );
                    if (aNewLangText == "LANGUAGE_NONE")
                        SwLangHelper::SetLanguage_None( rWrtSh, bForSelection, aCoreSet );
                    else if (aNewLangText == "RESET_LANGUAGES")
                        SwLangHelper::ResetLanguages( rWrtSh );
                    else
                        SwLangHelper::SetLanguage( rWrtSh, aNewLangText, bForSelection, aCoreSet );
                    rWrtSh.EndUndo();

                    if (bForParagraph || !bForSelection)
                    {
                        rWrtSh.Pop(SwCursorShell::PopMode::DeleteCurrent); // restore selection...
                    }
                }

                rWrtSh.LockView( false );
                rWrtSh.EndAction();
            }

            // invalidate slot to get the new language displayed
            rViewFrame.GetBindings().Invalidate( nSlot );

            rReq.Done();
            break;
        }

        case SID_THES:
        {
            // replace word/selection with text from selected sub menu entry
            OUString aReplaceText;
            const SfxStringItem* pItem2 = rReq.GetArg(FN_PARAM_THES_WORD_REPLACE);
            if (pItem2)
                aReplaceText = pItem2->GetValue();
            if (!aReplaceText.isEmpty())
            {
                SwView &rView2 = rWrtSh.GetView();
                const bool bSelection = rWrtSh.HasSelection();
                const OUString aLookUpText = rView2.GetThesaurusLookUpText( bSelection );
                rView2.InsertThesaurusSynonym( aReplaceText, aLookUpText, bSelection );
            }
        }
        break;

        case SID_CHARMAP:
        {
            InsertSymbol( rReq );
        }
        break;
        case FN_INSERT_FOOTNOTE:
        case FN_INSERT_ENDNOTE:
        {
            OUString aStr;
            const SfxStringItem* pFont = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
            const SfxStringItem* pNameItem = rReq.GetArg<SfxStringItem>(nSlot);
            if ( pNameItem )
                aStr = pNameItem->GetValue();
            bool bFont = pFont && !pFont->GetValue().isEmpty();
            rWrtSh.StartUndo( SwUndoId::UI_INSERT_FOOTNOTE );
            rWrtSh.InsertFootnote( aStr, nSlot == FN_INSERT_ENDNOTE, !bFont );
            if ( bFont )
            {
                rWrtSh.Left( SwCursorSkipMode::Chars, true, 1, false );
                SfxItemSetFixed<RES_CHRATR_FONT, RES_CHRATR_FONT> aSet( rWrtSh.GetAttrPool() );
                rWrtSh.GetCurAttr( aSet );
                rWrtSh.SetAttrSet( aSet, SetAttrMode::DONTEXPAND );
                rWrtSh.ResetSelect(nullptr, false, ScrollSizeMode::ScrollSizeDefault);
                rWrtSh.EndSelect();
                rWrtSh.GotoFootnoteText();
            }
            rWrtSh.EndUndo( SwUndoId::UI_INSERT_FOOTNOTE );
            rReq.Done();
        }
        break;
        case FN_INSERT_FOOTNOTE_DLG:
        {
            SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            VclPtr<AbstractInsFootNoteDlg> pDlg(pFact->CreateInsFootNoteDlg(
                GetView().GetFrameWeld(), rWrtSh));
            pDlg->SetHelpId(GetStaticInterface()->GetSlot(nSlot)->GetCommand());
            pDlg->StartExecuteAsync(
                [this, pDlg] (sal_Int32 nResult)->void
                {
                    if ( nResult == RET_OK )
                    {
                        pDlg->Apply();
                        const sal_uInt16 nId = pDlg->IsEndNote() ? FN_INSERT_ENDNOTE : FN_INSERT_FOOTNOTE;
                        SfxRequest aReq(GetView().GetViewFrame(), nId);
                        if ( !pDlg->GetStr().isEmpty() )
                            aReq.AppendItem( SfxStringItem( nId, pDlg->GetStr() ) );
                        if ( !pDlg->GetFontName().isEmpty() )
                            aReq.AppendItem( SfxStringItem( FN_PARAM_1, pDlg->GetFontName() ) );
                        ExecuteSlot( aReq );
                    }
                    pDlg->disposeOnce();
                }
            );
            rReq.Ignore();
        }
        break;
        case FN_FORMAT_FOOTNOTE_DLG:
        case FN_FORMAT_CURRENT_FOOTNOTE_DLG:
        {
            GetView().ExecFormatFootnote();
            break;
        }
        case SID_INSERTDOC:
        {
            GetView().ExecuteInsertDoc( rReq, pItem );
            break;
        }
        case FN_FORMAT_RESET:
        {
            // #i78856, reset all attributes but not the language attributes
            // (for this build an array of all relevant attributes and
            // remove the languages from that)
            o3tl::sorted_vector<sal_uInt16> aAttribs;

            static constexpr std::pair<sal_uInt16, sal_uInt16> aResetableSetRange[] = {
                // tdf#40496: we don't want to change writing direction, so exclude RES_FRAMEDIR:
                { RES_FRMATR_BEGIN, RES_FRAMEDIR - 1 },
                { RES_FRAMEDIR + 1, RES_FRMATR_END - 1 },
                { RES_CHRATR_BEGIN, RES_CHRATR_LANGUAGE - 1 },
                { RES_CHRATR_LANGUAGE + 1, RES_CHRATR_CJK_LANGUAGE - 1 },
                { RES_CHRATR_CJK_LANGUAGE + 1, RES_CHRATR_CTL_LANGUAGE - 1 },
                { RES_CHRATR_CTL_LANGUAGE + 1, RES_CHRATR_END - 1 },
                { RES_PARATR_BEGIN, RES_PARATR_END - 1 },
                { RES_PARATR_LIST_AUTOFMT, RES_PARATR_LIST_AUTOFMT },
                { RES_TXTATR_UNKNOWN_CONTAINER, RES_TXTATR_UNKNOWN_CONTAINER },
                { RES_UNKNOWNATR_BEGIN, RES_UNKNOWNATR_END - 1 },
            };
            for (const auto& [nBegin, nEnd] : aResetableSetRange)
            {
                for (sal_uInt16 i = nBegin; i <= nEnd; ++i)
                    aAttribs.insert( i );
            }

            // also clear the direct formatting flag inside SwTableBox(es)
            if (SwFEShell* pFEShell = GetView().GetDocShell()->GetFEShell())
                pFEShell->UpdateTableStyleFormatting(nullptr, true);

            // tdf#160801 fix crash by delaying resetting of attributes
            // Calling SwWrtShell::ResetAttr() will sometimes delete the
            // current SwTextShell instance so call it after clearing the
            // direct formatting flag.
            rWrtSh.ResetAttr( aAttribs );

            rReq.Done();
            break;
        }
        case FN_INSERT_BREAK_DLG:
        {
            if ( pItem )
            {
                ::std::optional<sal_uInt16> oPageNumber;
                std::optional<SwLineBreakClear> oClear;
                UIName aTemplateName;
                sal_uInt16 nKind = static_cast<const SfxInt16Item*>(pItem)->GetValue();
                const SfxStringItem* pTemplate = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
                const SfxUInt16Item* pNumber = rReq.GetArg<SfxUInt16Item>(FN_PARAM_2);
                const SfxBoolItem* pIsNumberFilled = rReq.GetArg<SfxBoolItem>(FN_PARAM_3);
                if ( pTemplate )
                    aTemplateName = UIName(pTemplate->GetValue());
                if ( pNumber && pIsNumberFilled && pIsNumberFilled->GetValue() )
                    oPageNumber = pNumber->GetValue();

                InsertBreak(rWrtSh, nKind, oPageNumber, aTemplateName, oClear);
            }
            else
            {
                SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();

                std::shared_ptr<AbstractSwBreakDlg> pAbstractDialog(pFact->CreateSwBreakDlg(GetView().GetFrameWeld(), rWrtSh));
                std::shared_ptr<weld::DialogController> pDialogController(pAbstractDialog->getDialogController());

                weld::DialogController::runAsync(pDialogController,
                    [pAbstractDialog=std::move(pAbstractDialog), &rWrtSh] (sal_Int32 nResult) {
                        if( RET_OK == nResult )
                        {
                            sal_uInt16 nKind = pAbstractDialog->GetKind();
                            OUString aTemplateName = pAbstractDialog->GetTemplateName();
                            ::std::optional<sal_uInt16> oPageNumber = pAbstractDialog->GetPageNumber();
                            std::optional<SwLineBreakClear> oClear = pAbstractDialog->GetClear();

                            InsertBreak(rWrtSh, nKind, oPageNumber, UIName(aTemplateName), oClear);
                        }
                    });
            }

            break;
        }
        case FN_INSERT_BOOKMARK:
        {
            const SfxStringItem* pBookmarkText = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
            SwPaM* pCursorPos = rWrtSh.GetCursor();
            if ( pItem )
            {
                rWrtSh.StartAction();
                OUString sName = static_cast<const SfxStringItem*>(pItem)->GetValue();

                if (pBookmarkText)
                {
                    OUString aBookmarkText = pBookmarkText->GetValue();
                    // Split node to remember where the start position is.
                    bool bSuccess = rWrtSh.GetDoc()->getIDocumentContentOperations().SplitNode(
                        *pCursorPos->GetPoint(), /*bChkTableStart=*/false);
                    if (bSuccess)
                    {
                        SwPaM aBookmarkPam(*pCursorPos->GetPoint());
                        aBookmarkPam.Move(fnMoveBackward, GoInContent);

                        // Paste HTML content.
                        SwTranslateHelper::PasteHTMLToPaM(
                            rWrtSh, pCursorPos, aBookmarkText.toUtf8());
                        if (pCursorPos->GetPoint()->GetContentIndex() == 0)
                        {
                            // The paste created a last empty text node, remove it.
                            SwPaM aPam(*pCursorPos->GetPoint());
                            aPam.SetMark();
                            aPam.Move(fnMoveBackward, GoInContent);
                            rWrtSh.GetDoc()->getIDocumentContentOperations().DeleteAndJoin(aPam);
                        }

                        // Undo the above SplitNode().
                        aBookmarkPam.SetMark();
                        aBookmarkPam.Move(fnMoveForward, GoInContent);
                        rWrtSh.GetDoc()->getIDocumentContentOperations().DeleteAndJoin(
                            aBookmarkPam);
                        *aBookmarkPam.GetMark() = *pCursorPos->GetPoint();
                        *pCursorPos = aBookmarkPam;
                    }
                }

                rWrtSh.SetBookmark( vcl::KeyCode(), SwMarkName(sName) );
                if (pBookmarkText)
                {
                    pCursorPos->DeleteMark();
                }
                rWrtSh.EndAction();
                break;
            }
            [[fallthrough]];
        }
        case FN_EDIT_BOOKMARK:
        {
            ::std::optional<OUString> oName;
            if (pItem)
            {
                oName.emplace(static_cast<const SfxStringItem*>(pItem)->GetValue());
            }
            {
                SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
                ScopedVclPtr<VclAbstractDialog> pDlg(pFact->CreateSwInsertBookmarkDlg(GetView().GetFrameWeld(), rWrtSh, oName ? &*oName : nullptr));
                VclAbstractDialog::AsyncContext aContext;
                aContext.maEndDialogFn = [](sal_Int32){};
                pDlg->StartExecuteAsync(aContext);
            }

            break;
        }
        case FN_UPDATE_BOOKMARKS:
        {
            // This updates all bookmarks in the document that match the conditions specified in
            // rReq.
            UpdateBookmarks(rReq, rWrtSh);
            break;
        }
        case FN_UPDATE_BOOKMARK:
        {
            // This updates the bookmark under the cursor.
            UpdateBookmark(rReq, rWrtSh);
            break;
        }
        case FN_DELETE_BOOKMARK:
        {
            // This deletes a bookmark with the specified name.
            if (pItem && !rWrtSh.getIDocumentSettingAccess().get(DocumentSettingId::PROTECT_BOOKMARKS))
            {
                IDocumentMarkAccess* const pMarkAccess = rWrtSh.getIDocumentMarkAccess();
                pMarkAccess->deleteMark(pMarkAccess->findMark(SwMarkName(static_cast<const SfxStringItem*>(pItem)->GetValue())), false);
            }
            break;
        }
        case FN_DELETE_BOOKMARKS:
        {
            // This deletes all bookmarks in the document matching a specified prefix.
            DeleteBookmarks(rReq, rWrtSh);
            break;
        }
        case FN_DELETE_FIELDS:
        {
            // This deletes all fields in the document matching a specified type & prefix.
            DeleteFields(rReq, rWrtSh);
            break;
        }
        case FN_UPDATE_SECTIONS:
        {
            UpdateSections(rReq, rWrtSh);
            break;
        }
        case FN_DELETE_SECTIONS:
        {
            // This deletes all sections in the document matching a specified prefix. Note that the
            // section is deleted, but not its contents.
            DeleteSections(rReq, rWrtSh);
            break;
        }
        case FN_DELETE_CONTENT_CONTROL:
        {
            DeleteContentControl( rWrtSh );
            break;
        }
        case FN_SET_REMINDER:
        {
            // collect and sort navigator reminder names
            IDocumentMarkAccess* const pMarkAccess = rWrtSh.getIDocumentMarkAccess();
            std::vector< SwMarkName > vNavMarkNames;
            for(auto ppMark = pMarkAccess->getAllMarksBegin();
                ppMark != pMarkAccess->getAllMarksEnd();
                ++ppMark)
            {
                if( IDocumentMarkAccess::GetType(**ppMark) == IDocumentMarkAccess::MarkType::NAVIGATOR_REMINDER )
                    vNavMarkNames.push_back((*ppMark)->GetName());
            }
            std::sort(vNavMarkNames.begin(), vNavMarkNames.end());

            // we are maxed out so delete the first one
            // this assumes that IDocumentMarkAccess generates Names in ascending order
            if(vNavMarkNames.size() == MAX_MARKS)
                pMarkAccess->deleteMark(pMarkAccess->findMark(vNavMarkNames[0]), false);

            rWrtSh.SetBookmark(vcl::KeyCode(), SwMarkName(), IDocumentMarkAccess::MarkType::NAVIGATOR_REMINDER);
            SwView::SetActMark(vNavMarkNames.size() < MAX_MARKS ? vNavMarkNames.size() : MAX_MARKS-1);

            break;
        }
        case FN_AUTOFORMAT_REDLINE_APPLY:
        {
            SvxSwAutoFormatFlags aFlags(SvxAutoCorrCfg::Get().GetAutoCorrect()->GetSwFlags());
            // This must always be false for the postprocessing.
            aFlags.bAFormatByInput = false;
            aFlags.bWithRedlining = true;
            rWrtSh.AutoFormat( &aFlags, false );
            aFlags.bWithRedlining = false;

            SfxViewFrame& rVFrame = GetView().GetViewFrame();
            if (rVFrame.HasChildWindow(FN_REDLINE_ACCEPT))
                rVFrame.ToggleChildWindow(FN_REDLINE_ACCEPT);

            SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            auto xRequest = std::make_shared<SfxRequest>(rReq);
            rReq.Ignore(); // the 'old' request is not relevant any more
            VclPtr<AbstractSwModalRedlineAcceptDlg> pDlg(pFact->CreateSwModalRedlineAcceptDlg(GetView().GetEditWin().GetFrameWeld()));
            pDlg->StartExecuteAsync(
                [pDlg, xRequest=std::move(xRequest)] (sal_Int32 /*nResult*/)->void
                {
                    pDlg->disposeOnce();
                    xRequest->Done();
                }
            );
        }
        break;

        case FN_AUTOFORMAT_APPLY:
        {
            SvxSwAutoFormatFlags aFlags(SvxAutoCorrCfg::Get().GetAutoCorrect()->GetSwFlags());
            // This must always be false for the postprocessing.
            aFlags.bAFormatByInput = false;
            rWrtSh.AutoFormat( &aFlags, false );
            rReq.Done();
        }
        break;
        case FN_AUTOFORMAT_AUTO:
        {
            SvxAutoCorrCfg& rACfg = SvxAutoCorrCfg::Get();
            bool bSet = pItem ? static_cast<const SfxBoolItem*>(pItem)->GetValue() : !rACfg.IsAutoFormatByInput();
            if( bSet != rACfg.IsAutoFormatByInput() )
            {
                rACfg.SetAutoFormatByInput( bSet );
                rACfg.Commit();
                GetView().GetViewFrame().GetBindings().Invalidate( nSlot );
                if ( !pItem )
                    rReq.AppendItem( SfxBoolItem( GetPool().GetWhichIDFromSlotID(nSlot), bSet ) );
                rReq.Done();
            }
        }
        break;
        case FN_AUTO_CORRECT:
        {
            // At first set to blank as default.
            rWrtSh.AutoCorrect( *SvxAutoCorrCfg::Get().GetAutoCorrect(), ' ' );
            rReq.Done();
        }
        break;
        case FN_TABLE_SORT_DIALOG:
        case FN_SORTING_DLG:
        {
            SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            VclPtr<AbstractSwSortDlg> pDlg(pFact->CreateSwSortingDialog(GetView().GetFrameWeld(), rWrtSh));
            auto xRequest = std::make_shared<SfxRequest>(rReq);
            rReq.Ignore(); // the 'old' request is not relevant any more
            pDlg->StartExecuteAsync(
                [pDlg, xRequest=std::move(xRequest)] (sal_Int32 nResult)->void
                {
                    if (nResult == RET_OK)
                        pDlg->Apply();
                    pDlg->disposeOnce();
                    xRequest->Done();
                }
            );
        }
        break;
        case FN_NUMBERING_OUTLINE_DLG:
        {
            GetView().ExecNumberingOutline(GetPool());
            rReq.Done();
        }
            break;
        case FN_CALCULATE:
            {
                rtl::Reference<SwTransferable> pTransfer = new SwTransferable( rWrtSh );
                pTransfer->CalculateAndCopy();
                rReq.Done();
            }
            break;
        case FN_GOTO_REFERENCE:
        {
            SwField *pField = rWrtSh.GetCurField();
            if(pField && pField->GetTypeId() == SwFieldTypesEnum::GetRef)
            {
                rWrtSh.StartAllAction();
                rWrtSh.SwCursorShell::GotoRefMark( static_cast<SwGetRefField*>(pField)->GetSetRefName(),
                                    static_cast<SwGetRefField*>(pField)->GetSubType(),
                                    static_cast<SwGetRefField*>(pField)->GetSeqNo(),
                                    static_cast<SwGetRefField*>(pField)->GetFlags() );
                rWrtSh.EndAllAction();
                rReq.Done();
            }
        }
            break;
        case FN_EDIT_FORMULA:
        {
            const sal_uInt16 nId = SwInputChild::GetChildWindowId();
            SfxViewFrame& rVFrame = GetView().GetViewFrame();
            if(pItem)
            {
                //if the ChildWindow is active it has to be removed
                if( rVFrame.HasChildWindow( nId ) )
                {
                    rVFrame.ToggleChildWindow( nId );
                    rVFrame.GetBindings().InvalidateAll( true );
                }

                OUString sFormula(static_cast<const SfxStringItem*>(pItem)->GetValue());
                SwFieldMgr aFieldMgr;
                rWrtSh.StartAllAction();
                bool bDelSel = rWrtSh.HasSelection();
                if( bDelSel )
                {
                    rWrtSh.StartUndo( SwUndoId::START );
                    rWrtSh.DelRight();
                }
                else
                {
                    rWrtSh.EnterStdMode();
                }

                if( !bDelSel && aFieldMgr.GetCurField() && SwFieldTypesEnum::Formel == aFieldMgr.GetCurTypeId() )
                    aFieldMgr.UpdateCurField( static_cast<SwGetExpField*>(aFieldMgr.GetCurField())->GetFormat(), OUString(), sFormula );
                else if( !sFormula.isEmpty() )
                {
                    if( rWrtSh.IsCursorInTable() )
                    {
                        SfxItemSetFixed<RES_BOXATR_FORMULA, RES_BOXATR_FORMULA> aSet( rWrtSh.GetAttrPool() );
                        aSet.Put( SwTableBoxFormula( sFormula ));
                        rWrtSh.SetTableBoxFormulaAttrs( aSet );
                        rWrtSh.UpdateTable();
                    }
                    else
                    {
                        SvNumberFormatter* pFormatter = rWrtSh.GetNumberFormatter();
                        const sal_uInt32 nSysNumFormat = pFormatter->GetFormatIndex( NF_NUMBER_STANDARD, LANGUAGE_SYSTEM);
                        SwInsertField_Data aData(SwFieldTypesEnum::Formel, static_cast<sal_uInt16>(SwGetSetExpType::Formula), OUString(), sFormula, nSysNumFormat);
                        aFieldMgr.InsertField(aData);
                    }
                }

                if( bDelSel )
                    rWrtSh.EndUndo( SwUndoId::END );
                rWrtSh.EndAllAction();
                rReq.Done();
            }
            else
            {
                rWrtSh.EndAllTableBoxEdit();
                rVFrame.ToggleChildWindow( nId );
                if( !rVFrame.HasChildWindow( nId ) )
                    rVFrame.GetBindings().InvalidateAll( true );
                rReq.Ignore();
            }
        }

        break;
        case FN_TABLE_UNSET_READ_ONLY:
        {
            rWrtSh.UnProtectTables();
        }
        break;
        case SID_EDIT_HYPERLINK:
        {
            if (!rWrtSh.HasSelection())
            {
                SfxItemSetFixed<RES_TXTATR_INETFMT, RES_TXTATR_INETFMT> aSet(GetPool());
                rWrtSh.GetCurAttr(aSet);
                if (SfxItemState::SET > aSet.GetItemState(RES_TXTATR_INETFMT))
                {
                    // Didn't find a hyperlink to edit yet.

                    // If the cursor is just before an unselected hyperlink,
                    // the dialog will not know that it should edit that hyperlink,
                    // so in this case, first select it so the dialog will find the hyperlink.
                    // The dialog would leave the hyperlink selected anyway after a successful edit
                    // (although it isn't normally selected after a cancel, but oh well).
                    if (!rWrtSh.SelectTextAttr(RES_TXTATR_INETFMT))
                        break;
                }
            }

            GetView().GetViewFrame().SetChildWindow(SID_HYPERLINK_DIALOG, true);
        }
        break;
        case SID_REMOVE_HYPERLINK:
        {
            bool bSel = rWrtSh.HasSelection();
            if(!bSel)
            {
                rWrtSh.StartAction();
                rWrtSh.Push();
                if(!rWrtSh.SelectTextAttr( RES_TXTATR_INETFMT ))
                    rWrtSh.SelWrd();
            }
            //now remove the attribute
            rWrtSh.ResetAttr({ RES_TXTATR_INETFMT });
            if(!bSel)
            {
                rWrtSh.Pop(SwCursorShell::PopMode::DeleteCurrent);
                rWrtSh.EndAction();
            }
        }
        break;
        case SID_ATTR_BRUSH_CHAR :
        case SID_ATTR_CHAR_SCALEWIDTH :
        case SID_ATTR_CHAR_ROTATED :
        case FN_TXTATR_INET :
        {
            const sal_uInt16 nWhich = GetPool().GetWhichIDFromSlotID( nSlot );
            if ( pArgs && pArgs->GetItemState( nWhich ) == SfxItemState::SET )
                bUseDialog = false;
            [[fallthrough]];
        }
        case SID_CHAR_DLG:
        case SID_CHAR_DLG_EFFECT:
        case SID_CHAR_DLG_POSITION:
        {
            sw_CharDialog(rWrtSh, bUseDialog, /*ApplyToParagraph*/false, nSlot, pArgs, &rReq);
        }
        break;
        case SID_CHAR_DLG_FOR_PARAGRAPH:
        {
            sw_CharDialog(rWrtSh, /*UseDialog*/true, /*ApplyToParagraph*/true, nSlot, pArgs, &rReq);
        }
        break;
        case SID_ATTR_LRSPACE :
        case SID_ATTR_ULSPACE :
        case SID_ATTR_BRUSH :
        case SID_PARA_VERTALIGN :
        case SID_ATTR_PARA_NUMRULE :
        case SID_ATTR_PARA_REGISTER :
        case SID_ATTR_PARA_PAGENUM :
        case FN_FORMAT_LINENUMBER :
        case FN_NUMBER_NEWSTART :
        case FN_NUMBER_NEWSTART_AT :
        case FN_FORMAT_DROPCAPS :
        case FN_DROP_TEXT:
        case SID_ATTR_PARA_LRSPACE:
        {
            const sal_uInt16 nWhich = GetPool().GetWhichIDFromSlotID( nSlot );
            if ( pArgs && pArgs->GetItemState( nWhich ) == SfxItemState::SET )
                bUseDialog = false;
            [[fallthrough]];
        }
        case SID_PARA_DLG:
        {
            SwPaM* pPaM = nullptr;

            if ( pArgs )
            {
                const SwPaMItem* pPaMItem = pArgs->GetItemIfSet( GetPool().GetWhichIDFromSlotID( FN_PARAM_PAM ), false );
                if ( pPaMItem )
                    pPaM = pPaMItem->GetValue( );
            }

            if ( !pPaM )
                pPaM = rWrtSh.GetCursor();

            FieldUnit eMetric = ::GetDfltMetric( dynamic_cast<SwWebView*>( &GetView()) != nullptr );
            SwModule* mod = SwModule::get();
            mod->PutItem(SfxUInt16Item(SID_ATTR_METRIC, static_cast<sal_uInt16>(eMetric)));

            bool bApplyCharUnit = ::HasCharUnit( dynamic_cast<SwWebView*>( &GetView()) != nullptr  );
            mod->PutItem(SfxBoolItem(SID_ATTR_APPLYCHARUNIT, bApplyCharUnit));

            SfxItemSetFixed<
                    RES_PARATR_BEGIN, RES_FRMATR_END - 1,
                    // FillAttribute support:
                    XATTR_FILL_FIRST, XATTR_FILL_LAST,
                    // Includes SID_ATTR_TABSTOP_POS:
                    SID_ATTR_TABSTOP_DEFAULTS, SID_ATTR_TABSTOP_OFFSET,
                    SID_ATTR_BORDER_INNER, SID_ATTR_BORDER_INNER,
                    SID_ATTR_PARA_MODEL, SID_ATTR_PARA_KEEP,
                    // Items to hand over XPropertyList things like XColorList,
                    // XHatchList, XGradientList, and XBitmapList to the Area
                    // TabPage:
                    SID_COLOR_TABLE, SID_PATTERN_LIST,
                    SID_HTML_MODE, SID_HTML_MODE,
                    SID_ATTR_PARA_PAGENUM, SID_ATTR_PARA_PAGENUM,
                    FN_PARAM_1, FN_PARAM_1,
                    FN_NUMBER_NEWSTART, FN_NUMBER_NEWSTART_AT,
                    FN_DROP_TEXT, FN_DROP_CHAR_STYLE_NAME>  aCoreSet( GetPool() );

            // get also the list level indent values merged as LR-SPACE item, if needed.
            rWrtSh.GetPaMAttr( pPaM, aCoreSet, true );

            // create needed items for XPropertyList entries from the DrawModel so that
            // the Area TabPage can access them
            // Do this after GetCurAttr, this resets the ItemSet content again
            SwDrawModel* pDrawModel = GetView().GetDocShell()->GetDoc()->getIDocumentDrawModelAccess().GetDrawModel();

            aCoreSet.Put(SvxColorListItem(pDrawModel->GetColorList(), SID_COLOR_TABLE));
            aCoreSet.Put(SvxGradientListItem(pDrawModel->GetGradientList(), SID_GRADIENT_LIST));
            aCoreSet.Put(SvxHatchListItem(pDrawModel->GetHatchList(), SID_HATCH_LIST));
            aCoreSet.Put(SvxBitmapListItem(pDrawModel->GetBitmapList(), SID_BITMAP_LIST));
            aCoreSet.Put(SvxPatternListItem(pDrawModel->GetPatternList(), SID_PATTERN_LIST));
            aCoreSet.Put(SfxUInt16Item(SID_HTML_MODE,
                            ::GetHtmlMode(GetView().GetDocShell())));

            // Tabulators: Put DefaultTabs into ItemSet
            const SvxTabStopItem& rDefTabs =
                            GetPool().GetUserOrPoolDefaultItem(RES_PARATR_TABSTOP);

            const sal_uInt16 nDefDist = o3tl::narrowing<sal_uInt16>(::GetTabDist( rDefTabs ));
            SfxUInt16Item aDefDistItem( SID_ATTR_TABSTOP_DEFAULTS, nDefDist );
            aCoreSet.Put( aDefDistItem );

            // Current tabulator
            SfxUInt16Item aTabPos( SID_ATTR_TABSTOP_POS, 0 );
            aCoreSet.Put( aTabPos );

            // Left border as offset
            //#i24363# tab stops relative to indent
            const tools::Long nOff
                = rWrtSh.getIDocumentSettingAccess().get(DocumentSettingId::TABS_RELATIVE_TO_INDENT)
                      ? aCoreSet.Get(RES_MARGIN_TEXTLEFT).ResolveTextLeft({})
                      : 0;
            SfxInt32Item aOff( SID_ATTR_TABSTOP_OFFSET, nOff );
            aCoreSet.Put( aOff );

            // Setting the BoxInfo
            ::PrepareBoxInfo( aCoreSet, rWrtSh );

            // Current page format
            ::SwToSfxPageDescAttr( aCoreSet );

            // Properties of numbering
            if (rWrtSh.GetNumRuleAtCurrCursorPos())
            {
                SfxBoolItem aStart( FN_NUMBER_NEWSTART, rWrtSh.IsNumRuleStart( pPaM ) );
                aCoreSet.Put(aStart);
                SfxUInt16Item aStartAt( FN_NUMBER_NEWSTART_AT,
                                        rWrtSh.GetNodeNumStart( pPaM ) );
                aCoreSet.Put(aStartAt);
            }
            VclPtr<SfxAbstractTabDialog> pDlg;

            if ( bUseDialog && GetActiveView() )
            {
                OUString sDefPage;
                if (pItem)
                    sDefPage = static_cast<const SfxStringItem*>(pItem)->GetValue();

                SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
                pDlg.reset(pFact->CreateSwParaDlg(GetView().GetFrameWeld(), GetView(), aCoreSet, false, sDefPage));
            }

            if ( !bUseDialog )
            {
                if ( nSlot == SID_ATTR_PARA_LRSPACE)
                {
                    SvxLRSpaceItem aParaMargin(static_cast<const SvxLRSpaceItem&>(pArgs->Get(nSlot)));
                    SvxFirstLineIndentItem firstLine(RES_MARGIN_FIRSTLINE);
                    SvxTextLeftMarginItem leftMargin(RES_MARGIN_TEXTLEFT);
                    SvxRightMarginItem rightMargin(RES_MARGIN_RIGHT);
                    firstLine.SetTextFirstLineOffset(aParaMargin.GetTextFirstLineOffset(),
                                                     aParaMargin.GetPropTextFirstLineOffset());
                    firstLine.SetAutoFirst(aParaMargin.IsAutoFirst());
                    leftMargin.SetTextLeft(aParaMargin.GetTextLeft(), aParaMargin.GetPropLeft());
                    rightMargin.SetRight(aParaMargin.GetRight(), aParaMargin.GetPropRight());
                    aCoreSet.Put(firstLine);
                    aCoreSet.Put(leftMargin);
                    aCoreSet.Put(rightMargin);

                    sw_ParagraphDialogResult(&aCoreSet, rWrtSh, rReq, pPaM);
                }
                else
                    sw_ParagraphDialogResult(const_cast<SfxItemSet*>(pArgs), rWrtSh, rReq, pPaM);
            }
            else if (pDlg)
            {
                auto pRequest = std::make_shared<SfxRequest>(rReq);
                rReq.Ignore(); // the 'old' request is not relevant any more

                auto vCursors = CopyPaMRing(*pPaM); // tdf#134439 make a copy to use at later apply
                pDlg->StartExecuteAsync([pDlg, &rWrtSh, pDrawModel, pRequest=std::move(pRequest), nDefDist, vCursors=std::move(vCursors)](sal_Int32 nResult){
                    if (nResult == RET_OK)
                    {
                        // Apply defaults if necessary.
                        SfxItemSet* pSet = const_cast<SfxItemSet*>(pDlg->GetOutputItemSet());
                        sal_uInt16 nNewDist;
                        const SfxUInt16Item* pDefaultsItem = pSet->GetItemIfSet(SID_ATTR_TABSTOP_DEFAULTS, false);
                        if (pDefaultsItem && nDefDist != (nNewDist = pDefaultsItem->GetValue()) )
                        {
                            SvxTabStopItem aDefTabs( 0, 0, SvxTabAdjust::Default, RES_PARATR_TABSTOP );
                            MakeDefTabs( nNewDist, aDefTabs );
                            rWrtSh.SetDefault( aDefTabs );
                            pSet->ClearItem( SID_ATTR_TABSTOP_DEFAULTS );
                        }

                        const SfxPoolItem* pItem2 = nullptr;
                        if (SfxItemState::SET == pSet->GetItemState(FN_PARAM_1, false, &pItem2))
                        {
                            pSet->Put(SfxStringItem(FN_DROP_TEXT, static_cast<const SfxStringItem*>(pItem2)->GetValue()));
                            pSet->ClearItem(FN_PARAM_1);
                        }

                        if (const SwFormatDrop* pDropItem = pSet->GetItemIfSet(RES_PARATR_DROP, false))
                        {
                            UIName sCharStyleName;
                            if (pDropItem->GetCharFormat())
                                sCharStyleName = pDropItem->GetCharFormat()->GetName();
                            pSet->Put(SfxStringItem(FN_DROP_CHAR_STYLE_NAME, sCharStyleName.toString()));
                        }

                        const XFillStyleItem* pFS = pSet->GetItem<XFillStyleItem>(XATTR_FILLSTYLE);
                        bool bSet = pFS && pFS->GetValue() == drawing::FillStyle_GRADIENT;
                        const XFillGradientItem* pTempGradItem
                            = bSet ? pSet->GetItem<XFillGradientItem>(XATTR_FILLGRADIENT) : nullptr;
                        if (pTempGradItem && pTempGradItem->GetName().isEmpty())
                        {
                            // MigrateItemSet guarantees unique gradient names
                            SfxItemSetFixed<XATTR_FILLGRADIENT, XATTR_FILLGRADIENT> aMigrateSet(rWrtSh.GetView().GetPool());
                            aMigrateSet.Put(XFillGradientItem(u"gradient"_ustr, pTempGradItem->GetGradientValue()));
                            SdrModel::MigrateItemSet(&aMigrateSet, pSet, *pDrawModel);
                        }

                        bSet = pFS && pFS->GetValue() == drawing::FillStyle_HATCH;
                        const XFillHatchItem* pTempHatchItem
                            = bSet ? pSet->GetItem<XFillHatchItem>(XATTR_FILLHATCH) : nullptr;
                        if (pTempHatchItem && pTempHatchItem->GetName().isEmpty())
                        {
                            SfxItemSetFixed<XATTR_FILLHATCH, XATTR_FILLHATCH> aMigrateSet(rWrtSh.GetView().GetPool());
                            aMigrateSet.Put(XFillHatchItem(u"hatch"_ustr, pTempHatchItem->GetHatchValue()));
                            SdrModel::MigrateItemSet(&aMigrateSet, pSet, *pDrawModel);
                        }

                        sw_ParagraphDialogResult(pSet, rWrtSh, *pRequest, vCursors->front().get());
                    }
                    pDlg->disposeOnce();
                });
            }
        }
        break;
        case FN_NUM_CONTINUE:
        {
            OUString sContinuedListId;
            const SwNumRule* pRule =
                rWrtSh.SearchNumRule( true, sContinuedListId );
            // #i86492#
            // Search also for bullet list
            if ( !pRule )
            {
                pRule = rWrtSh.SearchNumRule( false, sContinuedListId );
            }
            if ( pRule )
            {
                rWrtSh.SetCurNumRule( *pRule, false, sContinuedListId );
            }
        }
        break;

        case FN_SELECT_PARA:
        {
            if ( !rWrtSh.IsSttOfPara() )
                rWrtSh.SttPara();
            else
                rWrtSh.EnterStdMode();
            rWrtSh.EndPara( true );
        }
        break;

        case SID_DEC_INDENT:
        case SID_INC_INDENT:
        //According to the requirement, modified the behavior when user
        //using the indent button on the toolbar. Now if we increase/decrease indent for a
        //paragraph which has bullet style it will increase/decrease the bullet level.
        {
            //If the current paragraph has bullet call the function to
            //increase or decrease the bullet level.
            //Why could I know whether a paragraph has bullet or not by checking the below conditions?
            //Please refer to the "case KEY_TAB:" section in SwEditWin::KeyInput(..) :
            //      if( rSh.GetCurNumRule() && rSh.IsSttOfPara() &&
            //                  !rSh.HasReadonlySel() )
            //              eKeyState = KS_NumDown;
            //Above code demonstrates that when the cursor is at the start of a paragraph which has bullet,
            //press TAB will increase the bullet level.
            //So I copied from that ^^
            if ( rWrtSh.GetNumRuleAtCurrCursorPos() && !rWrtSh.HasReadonlySel() )
            {
                rWrtSh.NumUpDown( SID_INC_INDENT == nSlot );
            }
            else                //execute the original processing functions
            {
                //below is copied of the old codes
                rWrtSh.MoveLeftMargin( SID_INC_INDENT == nSlot, rReq.GetModifier() != KEY_MOD1 );
            }
        }
        rReq.Done();
        break;

        case FN_DEC_INDENT_OFFSET:
        case FN_INC_INDENT_OFFSET:
            rWrtSh.MoveLeftMargin( FN_INC_INDENT_OFFSET == nSlot, rReq.GetModifier() == KEY_MOD1 );
            rReq.Done();
            break;

        case SID_ATTR_CHAR_COLOR2:
        {
            std::unique_ptr<const SvxColorItem> pRecentColor; // manage lifecycle scope
            if (!pItem)
            {
                // no color provided: use the pre-selected color shown in the toolbar/sidebar
                const std::optional<NamedColor> oColor
                    = GetView().GetDocShell()->GetRecentColor(SID_ATTR_CHAR_COLOR);
                if (oColor.has_value())
                {
                    const model::ComplexColor aCol = (*oColor).getComplexColor();
                    pRecentColor = std::make_unique<const SvxColorItem>(
                        aCol.getFinalColor(), aCol, RES_CHRATR_COLOR);
                    pItem = pRecentColor.get();
                }
            }

            if (pItem)
            {
                auto* pColorItem = static_cast<const SvxColorItem*>(pItem);
                SwEditWin& rEditWin = GetView().GetEditWin();
                rEditWin.SetWaterCanTextColor(pColorItem->GetValue());
                SwApplyTemplate* pApply = rEditWin.GetApplyTemplate();

                // If there is a selection, then set the color on it
                // otherwise, it'll be the color for the next text to be typed
                if (!pApply || pApply->nColor != SID_ATTR_CHAR_COLOR_EXT)
                {
                    rWrtSh.SetAttrItem(SvxColorItem(pColorItem->GetValue(), pColorItem->getComplexColor(), RES_CHRATR_COLOR));
                }

                rReq.Done();
            }
        }
        break;
        case SID_ATTR_CHAR_BACK_COLOR:
        case SID_ATTR_CHAR_COLOR_BACKGROUND: // deprecated
        case SID_ATTR_CHAR_COLOR_EXT:
        {
            Color aColor = COL_TRANSPARENT;
            model::ComplexColor aComplexColor;

            if (pItem)
            {
                auto* pColorItem = static_cast<const SvxColorItem*>(pItem);
                aColor = pColorItem->GetValue();
                aComplexColor = pColorItem->getComplexColor();
            }
            else if (nSlot == SID_ATTR_CHAR_BACK_COLOR)
            {
                // no color provided: use the pre-selected color shown in the toolbar/sidebar
                const std::optional<NamedColor> oColor
                    = GetView().GetDocShell()->GetRecentColor(nSlot);
                if (oColor.has_value())
                {
                    aComplexColor = (*oColor).getComplexColor();
                    aColor = aComplexColor.getFinalColor();
                }
            }

            SwEditWin& rEdtWin = GetView().GetEditWin();
            if (nSlot != SID_ATTR_CHAR_COLOR_EXT)
                rEdtWin.SetWaterCanTextBackColor(aColor);
            else if (pItem)
                rEdtWin.SetWaterCanTextColor(aColor);

            SwApplyTemplate* pApply = rEdtWin.GetApplyTemplate();
            SwApplyTemplate aTempl;
            if (!pApply && (rWrtSh.HasSelection() || rReq.IsAPI()))
            {
                if (nSlot != SID_ATTR_CHAR_COLOR_EXT)
                {
                    SfxItemSetFixed<RES_CHRATR_BACKGROUND, RES_CHRATR_BACKGROUND> aCoreSet( rWrtSh.GetView().GetPool() );

                    rWrtSh.GetCurAttr(aCoreSet);

                    // Remove highlight if already set of the same color
                    const SvxBrushItem& rBrushItem = aCoreSet.Get(RES_CHRATR_BACKGROUND);
                    if (aColor == rBrushItem.GetColor())
                    {
                        aComplexColor = model::ComplexColor();
                        aColor = COL_TRANSPARENT;
                    }
                    ApplyCharBackground(aColor, aComplexColor, rWrtSh);
                }
                else
                    rWrtSh.SetAttrItem(SvxColorItem(aColor, aComplexColor, RES_CHRATR_COLOR));
            }
            else
            {
                if(!pApply || pApply->nColor != nSlot)
                    aTempl.nColor = nSlot;
                rEdtWin.SetApplyTemplate(aTempl);
            }

            rReq.Done();
        }
        break;

        case FN_NUM_BULLET_MOVEDOWN:
            if (!rWrtSh.IsAddMode())
                rWrtSh.MoveParagraph();
            rReq.Done();
            break;

        case FN_NUM_BULLET_MOVEUP:
            if (!rWrtSh.IsAddMode())
                rWrtSh.MoveParagraph(SwNodeOffset(-1));
            rReq.Done();
            break;
        case SID_INSERT_HYPERLINK:
        {
            SfxRequest aReq(SID_HYPERLINK_DIALOG, SfxCallMode::SLOT, SfxGetpApp()->GetPool());
            GetView().GetViewFrame().ExecuteSlot( aReq);
            rReq.Ignore();
        }
        break;
        case SID_RUBY_DIALOG:
        case SID_HYPERLINK_DIALOG:
        {
            SfxRequest aReq(nSlot, SfxCallMode::SLOT, SfxGetpApp()->GetPool());
            GetView().GetViewFrame().ExecuteSlot( aReq);
            rReq.Ignore();
        }
        break;
    case FN_INSERT_PAGEHEADER:
    case FN_INSERT_PAGEFOOTER:
    if(pArgs && pArgs->Count())
    {
        OUString sStyleName;
        if(pItem)
            sStyleName = static_cast<const SfxStringItem*>(pItem)->GetValue();
        bool bOn = true;
        if( SfxItemState::SET == pArgs->GetItemState(FN_PARAM_1, false, &pItem))
            bOn = static_cast<const SfxBoolItem*>(pItem)->GetValue();
        rWrtSh.ChangeHeaderOrFooter(UIName(sStyleName), FN_INSERT_PAGEHEADER == nSlot, bOn, !rReq.IsAPI());
        rReq.Done();
    }
    break;
    case FN_READONLY_SELECTION_MODE :
        if(GetView().GetDocShell()->IsReadOnly())
        {
            rWrtSh.SetReadonlySelectionOption(
                !rWrtSh.GetViewOptions()->IsSelectionInReadonly());
            rWrtSh.ShowCursor();
        }
    break;
    case FN_SELECTION_MODE_DEFAULT:
    case FN_SELECTION_MODE_BLOCK :
    {
        bool bSetBlockMode = !rWrtSh.IsBlockMode();
        if( pArgs && SfxItemState::SET == pArgs->GetItemState(nSlot, false, &pItem))
            bSetBlockMode = static_cast<const SfxBoolItem*>(pItem)->GetValue();
        if( ( nSlot == FN_SELECTION_MODE_DEFAULT ) != bSetBlockMode )
            rWrtSh.EnterBlockMode();
        else
            rWrtSh.EnterStdMode();
        SfxBindings &rBnd = GetView().GetViewFrame().GetBindings();
        rBnd.Invalidate(FN_STAT_SELMODE);
        rBnd.Update(FN_STAT_SELMODE);
    }
    break;
    case SID_OPEN_HYPERLINK:
    case SID_COPY_HYPERLINK_LOCATION:
    {
        SfxItemSetFixed<RES_TXTATR_INETFMT, RES_TXTATR_INETFMT> aSet(GetPool());
        rWrtSh.GetCurAttr(aSet);

        const SwFormatINetFormat* pINetFormat = nullptr;
        if(SfxItemState::SET <= aSet.GetItemState( RES_TXTATR_INETFMT ))
            pINetFormat = &aSet.Get(RES_TXTATR_INETFMT);
        else if (!rWrtSh.HasSelection())
        {
            // is the cursor at the beginning of a hyperlink?
            const SwTextNode* pTextNd = rWrtSh.GetCursor()->GetPointNode().GetTextNode();
            if (pTextNd)
            {
                const sal_Int32 nIndex = rWrtSh.GetCursor()->Start()->GetContentIndex();
                const SwTextAttr* pINetFmt = pTextNd->GetTextAttrAt(nIndex, RES_TXTATR_INETFMT);
                if (pINetFmt && !pINetFmt->GetINetFormat().GetValue().isEmpty())
                    pINetFormat = &pINetFmt->GetINetFormat();
            }
        }

        if (pINetFormat)
        {
            if (nSlot == SID_OPEN_HYPERLINK)
            {
                rWrtSh.ClickToINetAttr(*pINetFormat);
            }
            else if (nSlot == SID_COPY_HYPERLINK_LOCATION)
            {
                OUString hyperlinkLocation = pINetFormat->GetValue();
                ::uno::Reference< datatransfer::clipboard::XClipboard > xClipboard = GetView().GetEditWin().GetClipboard();
                vcl::unohelper::TextDataObject::CopyStringTo(hyperlinkLocation, xClipboard, SfxViewShell::Current());
            }
        }
        else
        {
            SwField* pField = rWrtSh.GetCurField();
            if (pField && pField->GetTyp()->Which() == SwFieldIds::TableOfAuthorities)
            {
                const auto& rAuthorityField = *static_cast<const SwAuthorityField*>(pField);
                OUString targetURL = u""_ustr;

                if (auto targetType = rAuthorityField.GetTargetType();
                    targetType == SwAuthorityField::TargetType::UseDisplayURL
                    || targetType == SwAuthorityField::TargetType::UseTargetURL)
                {
                    // Bibliography entry with URL also provides a hyperlink.
                    targetURL = rAuthorityField.GetAbsoluteURL();
                }

                if (targetURL.getLength() > 0)
                {
                    if (nSlot == SID_OPEN_HYPERLINK)
                    {
                        ::LoadURL(rWrtSh, targetURL, LoadUrlFlags::NewView, /*rTargetFrameName=*/OUString());
                    }
                    else if (nSlot == SID_COPY_HYPERLINK_LOCATION)
                    {
                        ::uno::Reference< datatransfer::clipboard::XClipboard > xClipboard = GetView().GetEditWin().GetClipboard();
                        vcl::unohelper::TextDataObject::CopyStringTo(targetURL, xClipboard, SfxViewShell::Current());
                    }
                }
            }
        }
    }
    break;
    case FN_OPEN_LOCAL_URL:
    {
        OUString aLocalURL = GetLocalURL(rWrtSh);
        if (!aLocalURL.isEmpty())
        {
            ::LoadURL(rWrtSh, aLocalURL, LoadUrlFlags::NewView, /*rTargetFrameName=*/OUString());
        }
    }
    break;
    case SID_OPEN_XML_FILTERSETTINGS:
    {
        HandleOpenXmlFilterSettings(rReq);
    }
    break;
    case FN_FORMAT_APPLY_HEAD1:
    {
    }
    break;
    case FN_FORMAT_APPLY_HEAD2:
    {
    }
    break;
    case FN_FORMAT_APPLY_HEAD3:
    {
    }
    break;
    case FN_FORMAT_APPLY_DEFAULT:
    {
    }
    break;
    case FN_FORMAT_APPLY_TEXTBODY:
    {
    }
    break;
    case FN_WORDCOUNT_DIALOG:
    {
        GetView().UpdateWordCount(this, nSlot);
    }
    break;
    case FN_PROTECT_FIELDS:
    case FN_PROTECT_BOOKMARKS:
    {
        IDocumentSettingAccess& rIDSA = rWrtSh.getIDocumentSettingAccess();
        DocumentSettingId aSettingId = nSlot == FN_PROTECT_FIELDS
                                           ? DocumentSettingId::PROTECT_FIELDS
                                           : DocumentSettingId::PROTECT_BOOKMARKS;
        rIDSA.set(aSettingId, !rIDSA.get(aSettingId));
        // Invalidate so that toggle state gets updated
        SfxViewFrame& rViewFrame = GetView().GetViewFrame();
        rViewFrame.GetBindings().Invalidate(nSlot);
        rViewFrame.GetBindings().Update(nSlot);
    }
    break;
    case SID_FM_CTL_PROPERTIES:
    {
        SwPosition aPos(*GetShell().GetCursor()->GetPoint());
        sw::mark::Fieldmark* pFieldBM = GetShell().getIDocumentMarkAccess()->getInnerFieldmarkFor(aPos);
        if ( !pFieldBM )
        {
            aPos.AdjustContent(-1);
            pFieldBM = GetShell().getIDocumentMarkAccess()->getInnerFieldmarkFor(aPos);
        }

        if ( pFieldBM && pFieldBM->GetFieldname() == ODF_FORMDROPDOWN
             && !(rWrtSh.GetCurrSection() && rWrtSh.GetCurrSection()->IsProtect()) )
        {
            SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            VclPtr<AbstractDropDownFormFieldDialog> pDlg(pFact->CreateDropDownFormFieldDialog(rWrtSh.GetView().GetFrameWeld(), pFieldBM));
            auto xRequest = std::make_shared<SfxRequest>(rReq);
            rReq.Ignore(); // the 'old' request is not relevant any more
            auto pWrtSh = &rWrtSh;
            pDlg->StartExecuteAsync(
                [pDlg, pFieldBM, pWrtSh, xRequest=std::move(xRequest)] (sal_Int32 nResult)->void
                {
                    if (nResult == RET_OK)
                    {
                        pDlg->Apply();
                        pFieldBM->Invalidate();
                        pWrtSh->InvalidateWindows( SwRect(pWrtSh->GetView().GetVisArea()) );
                        pWrtSh->UpdateCursor(); // cursor position might be invalid
                    }
                    pDlg->disposeOnce();
                    xRequest->Done();
                }
            );
        }
        else if ( pFieldBM && pFieldBM->GetFieldname() == ODF_FORMDATE )
        {
            SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            sw::mark::DateFieldmark* pDateField = &dynamic_cast<sw::mark::DateFieldmark&>(*pFieldBM);
            VclPtr<AbstractDateFormFieldDialog> pDlg(pFact->CreateDateFormFieldDialog(rWrtSh.GetView().GetFrameWeld(), pDateField, *GetView().GetDocShell()->GetDoc()));
            auto pWrtSh = &rWrtSh;
            auto xRequest = std::make_shared<SfxRequest>(rReq);
            rReq.Ignore(); // the 'old' request is not relevant any more
            pDlg->StartExecuteAsync(
                [pDlg, pWrtSh, pDateField, xRequest=std::move(xRequest)] (sal_Int32 nResult)->void
                {
                    if (nResult == RET_OK)
                    {
                        pDlg->Apply();
                        pDateField->Invalidate();
                        pWrtSh->InvalidateWindows( SwRect(pWrtSh->GetView().GetVisArea()) );
                        pWrtSh->UpdateCursor(); // cursor position might be invalid
                    }
                    pDlg->disposeOnce();
                    xRequest->Done();
                }
            );
        }
        else
        {
            SfxRequest aReq(GetView().GetViewFrame(), SID_FM_CTL_PROPERTIES);
            aReq.AppendItem( SfxBoolItem( SID_FM_CTL_PROPERTIES, true ) );
            rWrtSh.GetView().GetFormShell()->Execute( aReq );
        }
    }
    break;
    case SID_FM_TRANSLATE:
    {
#if HAVE_FEATURE_CURL && !ENABLE_WASM_STRIP_EXTRA
        const SfxPoolItem* pTargetLangStringItem = nullptr;
        if (pArgs && SfxItemState::SET == pArgs->GetItemState(SID_ATTR_TARGETLANG_STR, false, &pTargetLangStringItem))
        {
            std::optional<OUString> oDeeplAPIUrl = officecfg::Office::Linguistic::Translation::Deepl::ApiURL::get();
            std::optional<OUString> oDeeplKey = officecfg::Office::Linguistic::Translation::Deepl::AuthKey::get();
            if (!oDeeplAPIUrl || oDeeplAPIUrl->isEmpty() || !oDeeplKey || oDeeplKey->isEmpty())
            {
                SAL_WARN("sw.ui", "SID_FM_TRANSLATE: API options are not set");
                break;
            }
            const OString aAPIUrl = OUStringToOString(rtl::Concat2View(*oDeeplAPIUrl + "?tag_handling=html"), RTL_TEXTENCODING_UTF8).trim();
            const OString aAuthKey = OUStringToOString(*oDeeplKey, RTL_TEXTENCODING_UTF8).trim();
            OString aTargetLang = OUStringToOString(static_cast<const SfxStringItem*>(pTargetLangStringItem)->GetValue(), RTL_TEXTENCODING_UTF8);
            SwTranslateHelper::TranslateAPIConfig aConfig({aAPIUrl, aAuthKey, aTargetLang});
            SwTranslateHelper::TranslateDocument(rWrtSh, aConfig);
        }
        else
        {
            SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            std::shared_ptr<AbstractSwTranslateLangSelectDlg> pAbstractDialog(pFact->CreateSwTranslateLangSelectDlg(GetView().GetFrameWeld(), rWrtSh));
            std::shared_ptr<weld::DialogController> pDialogController(pAbstractDialog->getDialogController());
            weld::DialogController::runAsync(pDialogController, [] (sal_Int32 /*nResult*/) { });
        }
#endif // HAVE_FEATURE_CURL && ENABLE_WASM_STRIP_EXTRA
    }
    break;
    case SID_SPELLCHECK_IGNORE:
    {
        SwPaM *pPaM = rWrtSh.GetCursor();
        if (pPaM)
            SwEditShell::IgnoreGrammarErrorAt( *pPaM );
    }
    break;
    case SID_SPELLCHECK_IGNORE_ALL:
    {
        OUString sApplyText;
        const SfxStringItem* pItem2 = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
        if (pItem2)
            sApplyText = pItem2->GetValue();

        if(sApplyText == "Grammar")
        {
            linguistic2::ProofreadingResult aGrammarCheckRes;
            sal_Int32 nErrorInResult = -1;
            uno::Sequence< OUString > aSuggestions;
            sal_Int32 nErrorPosInText = -1;
            SwRect aToFill;
            bool bCorrectionRes = rWrtSh.GetGrammarCorrection( aGrammarCheckRes, nErrorPosInText, nErrorInResult, aSuggestions, nullptr, aToFill );
            if(bCorrectionRes)
            {
                try {
                    uno::Reference< linguistic2::XDictionary > xDictionary = LinguMgr::GetIgnoreAllList();
                    aGrammarCheckRes.xProofreader->ignoreRule(
                        aGrammarCheckRes.aErrors[ nErrorInResult ].aRuleIdentifier,
                            aGrammarCheckRes.aLocale );
                    // refresh the layout of the actual paragraph (faster)
                    SwPaM *pPaM = rWrtSh.GetCursor();
                    if (pPaM)
                        SwEditShell::IgnoreGrammarErrorAt( *pPaM );
                    if (xDictionary.is() && pPaM)
                    {
                        linguistic::AddEntryToDic( xDictionary, pPaM->GetText(), false, OUString() );
                        // refresh the layout of all paragraphs (workaround to launch a dictionary event)
                        xDictionary->setActive(false);
                        xDictionary->setActive(true);
                    }
                }
                catch( const uno::Exception& )
                {
                }
            }
        }
        else if (sApplyText == "Spelling")
        {
            AddWordToWordbook(LinguMgr::GetIgnoreAllList(), rWrtSh);
        }
    }
    break;
    case SID_ADD_TO_WORDBOOK:
    {
        OUString aDicName;
        if (const SfxStringItem* pItem1 = rReq.GetArg<SfxStringItem>(FN_PARAM_1))
            aDicName = pItem1->GetValue();

        uno::Reference<linguistic2::XSearchableDictionaryList> xDicList(LinguMgr::GetDictionaryList());
        uno::Reference<linguistic2::XDictionary> xDic = xDicList.is() ? xDicList->getDictionaryByName(aDicName) : nullptr;
        if (AddWordToWordbook(xDic, rWrtSh))
        {
            // save modified user-dictionary if it is persistent
            uno::Reference<frame::XStorable> xSavDic(xDic, uno::UNO_QUERY);
            if (xSavDic.is())
                xSavDic->store();
        }
        break;
    }
    break;
    case SID_SPELLCHECK_APPLY_SUGGESTION:
    {
        OUString sApplyText;
        const SfxStringItem* pItem2 = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
        if (pItem2)
            sApplyText = pItem2->GetValue();

        static constexpr OUString sSpellingRule(u"Spelling_"_ustr);
        static constexpr OUString sGrammarRule(u"Grammar_"_ustr);

        bool bGrammar = false;
        sal_Int32 nPos = 0;
        uno::Reference< linguistic2::XSpellAlternatives >  xSpellAlt;
        if(-1 != (nPos = sApplyText.indexOf( sGrammarRule )))
        {
            sApplyText = sApplyText.replaceAt(nPos, sGrammarRule.getLength(), u"");
            bGrammar = true;
        }
        else if (-1 != (nPos = sApplyText.indexOf( sSpellingRule )))
        {
            sApplyText = sApplyText.replaceAt(nPos, sSpellingRule.getLength(), u"");
            SwRect aToFill;
            xSpellAlt.set(rWrtSh.GetCorrection(nullptr, aToFill));
            bGrammar = false;
        }

        if (!bGrammar && !xSpellAlt.is())
            return;

        bool bOldIns = rWrtSh.IsInsMode();
        rWrtSh.SetInsMode();

        OUString aTmp( sApplyText );
        OUString aOrig( bGrammar ? OUString() : xSpellAlt->getWord() );

        // if original word has a trailing . (likely the end of a sentence)
        // and the replacement text hasn't, then add it to the replacement
        if (!aTmp.isEmpty() && !aOrig.isEmpty() &&
            aOrig.endsWith(".") && /* !IsAlphaNumeric ??*/
            !aTmp.endsWith("."))
        {
            aTmp += ".";
        }

        SwRewriter aRewriter;

        aRewriter.AddRule(UndoArg1, rWrtSh.GetCursorDescr()
            // don't show the hidden control character of the comment
            .replaceAll(OUStringChar(CH_TXTATR_INWORD), "") );
        aRewriter.AddRule(UndoArg2, SwResId(STR_YIELDS));

        OUString aTmpStr = SwResId(STR_START_QUOTE) +
            aTmp + SwResId(STR_END_QUOTE);
        aRewriter.AddRule(UndoArg3, aTmpStr);

        rWrtSh.StartUndo(SwUndoId::UI_REPLACE, &aRewriter);
        rWrtSh.StartAction();

        // keep comments at the end of the replacement in case spelling correction is
        // invoked via the context menu. The spell check dialog does the correction in edlingu.cxx.
        rWrtSh.ReplaceKeepComments(aTmp);

        rWrtSh.EndAction();
        rWrtSh.EndUndo();

        rWrtSh.SetInsMode( bOldIns );
    }
    break;
        case FN_TRANSFORM_DOCUMENT_STRUCTURE:
        {
            // get the parameter, what to transform
            OUString aDataJson;
            const SfxStringItem* pDataJson = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
            if (pDataJson)
            {
                aDataJson = pDataJson->GetValue();
                aDataJson = rtl::Uri::decode(aDataJson, rtl_UriDecodeStrict, RTL_TEXTENCODING_UTF8);
            }

            // parse the JSON got prom parameter
            boost::property_tree::ptree aTree;
            std::stringstream aStream(
                (std::string(OUStringToOString(aDataJson, RTL_TEXTENCODING_UTF8))));
            try
            {
                boost::property_tree::read_json(aStream, aTree);
            }
            catch (...)
            {
                lcl_LogWarning("FillApi Transform parameter, Wrong JSON format. ");
                throw;
            }

            // get the loaded content controls
            uno::Reference<text::XContentControlsSupplier> xCCSupplier(
                GetView().GetDocShell()->GetModel(), uno::UNO_QUERY);
            if (!xCCSupplier.is())
                break;

            uno::Reference<container::XIndexAccess> xContentControls
                = xCCSupplier->getContentControls();
            int iCCcount = xContentControls->getCount();

            enum class ContentFilterType
            {
                ERROR = -1,
                INDEX,
                TAG,
                ALIAS,
                ID
            };
            std::vector<std::string> aIdTexts = { ".ByIndex.", ".ByTag.", ".ByAlias.", ".ById." };

            // get charts
            uno::Reference<text::XTextEmbeddedObjectsSupplier> xEOS(
                GetView().GetDocShell()->GetModel(), uno::UNO_QUERY);
            if (!xEOS.is())
                break;
            uno::Reference<container::XIndexAccess> xEmbeddeds(xEOS->getEmbeddedObjects(),
                                                               uno::UNO_QUERY);
            if (!xEmbeddeds.is())
                break;

            sal_Int32 nEOcount = xEmbeddeds->getCount();

            enum class ChartFilterType
            {
                ERROR = -1,
                INDEX,
                NAME,
                TITLE,
                SUBTITLE
            };
            std::vector<std::string> aIdChartTexts
                = { ".ByEmbedIndex.", ".ByEmbedName.", ".ByTitle.", ".BySubTitle." };

            // Iterate through the JSON data loaded into a tree structure
            for (const auto& aItem : aTree)
            {
                if (aItem.first == "Transforms")
                {
                    // Handle all transformations
                    for (const auto& aItem2Obj : aItem.second)
                    {
                        // handle `"Transforms": { `  and `"Transforms": [` cases as well
                        // if an element is an object `{...}`, then get the first element of the object
                        const auto& aItem2
                            = aItem2Obj.first == "" ? *aItem2Obj.second.ordered_begin() : aItem2Obj;

                        if (aItem2.first == "DocumentProperties")
                        {
                            uno::Reference<document::XDocumentPropertiesSupplier>
                                xDocumentPropsSupplier(GetView().GetDocShell()->GetModel(),
                                                       uno::UNO_QUERY);
                            if (!xDocumentPropsSupplier.is())
                                continue;
                            uno::Reference<document::XDocumentProperties2> xDocProps(
                                xDocumentPropsSupplier->getDocumentProperties(), uno::UNO_QUERY);
                            if (!xDocProps.is())
                                continue;

                            for (const auto& aItem3 : aItem2.second)
                            {
                                if (aItem3.first == "Author")
                                {
                                    xDocProps->setAuthor(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "Generator")
                                {
                                    xDocProps->setGenerator(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "CreationDate")
                                {
                                    util::DateTime aDateTime;
                                    sax::Converter::parseDateTime(
                                        aDateTime,
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                    xDocProps->setCreationDate(aDateTime);
                                }
                                else if (aItem3.first == "Title")
                                {
                                    xDocProps->setTitle(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "Subject")
                                {
                                    xDocProps->setSubject(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "Description")
                                {
                                    xDocProps->setDescription(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "Keywords")
                                {
                                    uno::Sequence<OUString> aStringSeq(aItem3.second.size());
                                    auto aStringArray = aStringSeq.getArray();
                                    int nId = 0;
                                    for (const auto& aItem4 : aItem3.second)
                                    {
                                        aStringArray[nId++] = OStringToOUString(aItem4.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8);
                                    }
                                    xDocProps->setKeywords(aStringSeq);
                                }
                                else if (aItem3.first == "Language")
                                {
                                    OUString aLanguageStr
                                        = OStringToOUString(aItem3.second.get_value<std::string>(),
                                                            RTL_TEXTENCODING_UTF8);
                                    lang::Locale aLanguageLang
                                        = LanguageTag::convertToLocale(aLanguageStr);
                                    xDocProps->setLanguage(aLanguageLang);
                                }
                                else if (aItem3.first == "ModifiedBy")
                                {
                                    xDocProps->setModifiedBy(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "ModificationDate")
                                {
                                    util::DateTime aDateTime;
                                    sax::Converter::parseDateTime(
                                        aDateTime,
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                    xDocProps->setModificationDate(aDateTime);
                                }
                                else if (aItem3.first == "PrintedBy")
                                {
                                    xDocProps->setPrintedBy(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "PrintDate")
                                {
                                    util::DateTime aDateTime;
                                    sax::Converter::parseDateTime(
                                        aDateTime,
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                    xDocProps->setPrintDate(aDateTime);
                                }
                                else if (aItem3.first == "TemplateName")
                                {
                                    xDocProps->setTemplateName(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "TemplateURL")
                                {
                                    xDocProps->setTemplateURL(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "TemplateDate")
                                {
                                    util::DateTime aDateTime;
                                    sax::Converter::parseDateTime(
                                        aDateTime,
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                    xDocProps->setTemplateDate(aDateTime);
                                }
                                else if (aItem3.first == "AutoloadURL")
                                {
                                    // Warning: wrong data here, can freeze LO.
                                    xDocProps->setAutoloadURL(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "AutoloadSecs")
                                {
                                    //sal_Int32
                                    xDocProps->setAutoloadSecs(aItem3.second.get_value<int>());
                                }
                                else if (aItem3.first == "DefaultTarget")
                                {
                                    xDocProps->setDefaultTarget(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "DocumentStatistics")
                                {
                                    uno::Sequence<beans::NamedValue> aNamedValueSeq(
                                        aItem3.second.size());
                                    auto aNamedValueArray = aNamedValueSeq.getArray();
                                    int nId = 0;
                                    for (const auto& aItem4 : aItem3.second)
                                    {
                                        OUString aName = OStringToOUString(aItem4.first,
                                                                           RTL_TEXTENCODING_UTF8);
                                        sal_Int32 nValue = aItem4.second.get_value<int>();
                                        aNamedValueArray[nId].Name = aName;
                                        aNamedValueArray[nId].Value <<= nValue;
                                        nId++;
                                    }
                                    xDocProps->setDocumentStatistics(aNamedValueSeq);
                                }
                                else if (aItem3.first == "EditingCycles")
                                {
                                    //sal_Int16
                                    xDocProps->setEditingCycles(aItem3.second.get_value<int>());
                                }
                                else if (aItem3.first == "EditingDuration")
                                {
                                    //sal_Int32
                                    xDocProps->setEditingDuration(aItem3.second.get_value<int>());
                                }
                                else if (aItem3.first == "Contributor")
                                {
                                    uno::Sequence<OUString> aStringSeq(aItem3.second.size());
                                    auto aStringArray = aStringSeq.getArray();
                                    int nId = 0;
                                    for (const auto& aItem4 : aItem3.second)
                                    {
                                        aStringArray[nId++] = OStringToOUString(
                                            aItem4.second.get_value<std::string>(),
                                            RTL_TEXTENCODING_UTF8);
                                    }
                                    xDocProps->setContributor(aStringSeq);
                                }
                                else if (aItem3.first == "Coverage")
                                {
                                    xDocProps->setCoverage(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "Identifier")
                                {
                                    xDocProps->setIdentifier(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "Publisher")
                                {
                                    uno::Sequence<OUString> aStringSeq(aItem3.second.size());
                                    auto aStringArray = aStringSeq.getArray();
                                    int nId = 0;
                                    for (const auto& aItem4 : aItem3.second)
                                    {
                                        aStringArray[nId++] = OStringToOUString(
                                            aItem4.second.get_value<std::string>(),
                                            RTL_TEXTENCODING_UTF8);
                                    }
                                    xDocProps->setPublisher(aStringSeq);
                                }
                                else if (aItem3.first == "Relation")
                                {
                                    uno::Sequence<OUString> aStringSeq(aItem3.second.size());
                                    auto aStringArray = aStringSeq.getArray();
                                    int nId = 0;
                                    for (const auto& aItem4 : aItem3.second)
                                    {
                                        aStringArray[nId++] = OStringToOUString(
                                            aItem4.second.get_value<std::string>(),
                                            RTL_TEXTENCODING_UTF8);
                                    }
                                    xDocProps->setRelation(aStringSeq);
                                }
                                else if (aItem3.first == "Rights")
                                {
                                    xDocProps->setRights(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "Source")
                                {
                                    xDocProps->setSource(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "Type")
                                {
                                    xDocProps->setType(
                                        OStringToOUString(aItem3.second.get_value<std::string>(),
                                                          RTL_TEXTENCODING_UTF8));
                                }
                                else if (aItem3.first == "UserDefinedProperties")
                                {
                                    const uno::Reference<beans::XPropertyContainer> xUserProps
                                        = xDocProps->getUserDefinedProperties();
                                    if (!xUserProps.is())
                                        continue;
                                    uno::Reference<beans::XPropertyAccess> xUserPropsAccess(
                                        xDocProps->getUserDefinedProperties(), uno::UNO_QUERY);
                                    if (!xUserPropsAccess.is())
                                        continue;

                                    for (const auto& aItem4Obj : aItem3.second)
                                    {
                                        // handle [{},{}...] case as well as {}...}
                                        const auto& aItem4 = aItem4Obj.first == ""
                                                                 ? *aItem4Obj.second.ordered_begin()
                                                                 : aItem4Obj;

                                        if (aItem4.first == "Delete")
                                        {
                                            std::string aPropName
                                                = aItem4.second.get_value<std::string>();
                                            try
                                            {
                                                xUserProps->removeProperty(OStringToOUString(
                                                    aPropName, RTL_TEXTENCODING_UTF8));
                                            }
                                            catch (...)
                                            {
                                                lcl_LogWarning("FillApi DocumentProperties "
                                                               "UserDefinedPropertieschart, failed "
                                                               "to delete property: '"
                                                               + aPropName + "'");
                                            }
                                        }
                                        else if (aItem4.first.starts_with("Add."))
                                        {
                                            std::string aPropName = aItem4.first.substr(4);

                                            comphelper::SequenceAsHashMap aUserDefinedProperties(
                                                xUserPropsAccess->getPropertyValues());
                                            comphelper::SequenceAsHashMap::iterator it
                                                = aUserDefinedProperties.find(OStringToOUString(
                                                    aPropName, RTL_TEXTENCODING_UTF8));
                                            bool bToDelete = (it != aUserDefinedProperties.end());

                                            try
                                            {
                                                std::stringstream aStreamPart;
                                                aStreamPart << "{\n\"" << aPropName << "\" : ";
                                                boost::property_tree::json_parser::write_json(
                                                    aStreamPart, aItem4.second);
                                                aStreamPart << "}";

                                                OString aJSONPart(aStreamPart.str());
                                                std::vector<beans::PropertyValue> aPropVec
                                                    = comphelper::JsonToPropertyValues(aJSONPart);

                                                if (bToDelete)
                                                    xUserProps->removeProperty(aPropVec[0].Name);

                                                xUserProps->addProperty(
                                                    aPropVec[0].Name,
                                                    beans::PropertyAttribute::REMOVABLE,
                                                    aPropVec[0].Value);
                                            }
                                            catch(...)
                                            {
                                                lcl_LogWarning("FillApi DocumentProperties "
                                                               "UserDefinedPropertieschart, failed "
                                                               "to add property: '"
                                                               + aPropName + "'");
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (aItem2.first.starts_with("Charts"))
                        {
                            std::string aTextEnd = aItem2.first.substr(6);
                            std::string aValue = "";
                            ChartFilterType iKeyId = ChartFilterType::ERROR;
                            // Find how the chart is identified: ByIndex, ByTitle...
                            for (size_t i = 0; i < aIdChartTexts.size(); i++)
                            {
                                if (aTextEnd.starts_with(aIdChartTexts[i]))
                                {
                                    iKeyId = static_cast<ChartFilterType>(i);
                                    aValue = aTextEnd.substr(aIdChartTexts[i].length());
                                    break;
                                }
                            }
                            if (iKeyId != ChartFilterType::ERROR)
                            {
                                // A chart transformation filter can match multiple charts
                                // In that case every matching charts will be transformed
                                // If no chart match to the filter, then we show warning
                                bool bChartFound = false;
                                for (int i = 0; i < nEOcount; ++i)
                                {
                                    uno::Reference<beans::XPropertySet> xShapeProps(
                                        xEmbeddeds->getByIndex(i), uno::UNO_QUERY);
                                    if (!xShapeProps.is())
                                        continue;

                                    uno::Reference<frame::XModel> xDocModel;
                                    xShapeProps->getPropertyValue(u"Model"_ustr) >>= xDocModel;
                                    if (!xDocModel.is())
                                        continue;

                                    uno::Reference<chart2::XChartDocument> xChartDoc(
                                        xDocModel, uno::UNO_QUERY);
                                    if (!xChartDoc.is())
                                        continue;

                                    uno::Reference<chart2::data::XDataProvider> xDataProvider(
                                        xChartDoc->getDataProvider());
                                    if (!xDataProvider.is())
                                        continue;

                                    uno::Reference<chart::XChartDataArray> xDataArray(
                                        xChartDoc->getDataProvider(), uno::UNO_QUERY);
                                    if (!xDataArray.is())
                                        continue;

                                    uno::Reference<chart2::XInternalDataProvider> xIDataProvider(
                                        xChartDoc->getDataProvider(), uno::UNO_QUERY);
                                    if (!xIDataProvider.is())
                                        continue;

                                    uno::Reference<util::XModifiable> xModi(xDocModel,
                                                                            uno::UNO_QUERY);
                                    if (!xModi.is())
                                        continue;

                                    switch (iKeyId)
                                    {
                                        case ChartFilterType::INDEX:
                                        {
                                            if (stoi(aValue) != i)
                                                continue;
                                        }
                                        break;
                                        case ChartFilterType::NAME:
                                        {
                                            uno::Reference<container::XNamed> xNamedShape(
                                                xEmbeddeds->getByIndex(i), uno::UNO_QUERY);
                                            if (xNamedShape.is())
                                            {
                                                OUString aName;
                                                aName = xNamedShape->getName();
                                                if (OStringToOUString(aValue, RTL_TEXTENCODING_UTF8)
                                                    != aName)
                                                    continue;
                                            }
                                        }
                                        break;
                                        case ChartFilterType::TITLE:
                                        {
                                            uno::Reference<chart2::XTitled> xTitled(
                                                xChartDoc, uno::UNO_QUERY_THROW);
                                            if (!xTitled.is())
                                                continue;
                                            uno::Reference<chart2::XTitle> xTitle
                                                = xTitled->getTitleObject();
                                            if (!xTitle.is())
                                                continue;

                                            OUString aTitle;
                                            const uno::Sequence<
                                                uno::Reference<chart2::XFormattedString>>
                                                aFSSeq = xTitle->getText();
                                            for (auto const& fs : aFSSeq)
                                                aTitle += fs->getString();
                                            if (OStringToOUString(aValue, RTL_TEXTENCODING_UTF8)
                                                != aTitle)
                                                continue;
                                        }
                                        break;
                                        case ChartFilterType::SUBTITLE:
                                        {
                                            uno::Reference<chart2::XDiagram> xDiagram
                                                = xChartDoc->getFirstDiagram();
                                            if (!xDiagram.is())
                                                continue;

                                            uno::Reference<chart2::XTitled> xTitled(
                                                xDiagram, uno::UNO_QUERY_THROW);
                                            if (!xTitled.is())
                                                continue;

                                            uno::Reference<chart2::XTitle> xSubTitle(
                                                xTitled->getTitleObject());
                                            if (!xSubTitle.is())
                                                continue;

                                            OUString aSubTitle;
                                            const uno::Sequence<
                                                uno::Reference<chart2::XFormattedString>>
                                                aFSSeq = xSubTitle->getText();
                                            for (auto const& fs : aFSSeq)
                                                aSubTitle += fs->getString();
                                            if (OStringToOUString(aValue, RTL_TEXTENCODING_UTF8)
                                                != aSubTitle)
                                                continue;
                                        }
                                        break;
                                        default:
                                            continue;
                                    }

                                    // We have a match, this chart need to be transformed
                                    // Set all the values (of the chart) what is needed
                                    bChartFound = true;

                                    // Check if the InternalDataProvider is row or column based.
                                    bool bChartUseColumns = false;
                                    uno::Sequence<beans::PropertyValue> aArguments(
                                        xDataProvider->detectArguments(nullptr));
                                    for (sal_Int32 j = 0; j < aArguments.getLength(); ++j)
                                    {
                                        if (aArguments[j].Name == "DataRowSource")
                                        {
                                            css::chart::ChartDataRowSource eRowSource;
                                            if (aArguments[j].Value >>= eRowSource)
                                                bChartUseColumns
                                                    = (eRowSource
                                                       == css::chart::ChartDataRowSource_COLUMNS);
                                            break;
                                        }
                                    }

                                    for (const auto& aItem3Obj : aItem2.second)
                                    {
                                        //handle [] and {} cases
                                        const auto& aItem3 = aItem3Obj.first == ""
                                                                 ? *aItem3Obj.second.ordered_begin()
                                                                 : aItem3Obj;

                                        if (aItem3.first.starts_with("deletecolumn.")
                                            || aItem3.first.starts_with("deleterow.")
                                            || aItem3.first.starts_with("insertcolumn.")
                                            || aItem3.first.starts_with("insertrow.")
                                            || aItem3.first.starts_with("modifycolumn.")
                                            || aItem3.first.starts_with("modifyrow."))
                                        {
                                            // delete insert, or modify a row, or column
                                            // column, or row?
                                            bool bSetColumn = (aItem3.first[6] == 'c');
                                            int nId = stoi(aItem3.first.substr(bSetColumn ? 13 : 10));
                                            bool bDelete = aItem3.first.starts_with("delete");
                                            // delete/insert a row/column if needed
                                            if (!aItem3.first.starts_with("modify"))
                                            {
                                                if (bChartUseColumns == bSetColumn)
                                                {
                                                    if (bDelete)
                                                    {
                                                        if (!lcl_DeleteChartColumns(xChartDoc, nId))
                                                            continue;
                                                        xIDataProvider->deleteSequence(nId);
                                                    }
                                                    else
                                                    {
                                                        if (!lcl_InsertChartColumns(xChartDoc, nId))
                                                            continue;
                                                    }
                                                }
                                                else
                                                {
                                                    if (bDelete)
                                                    {
                                                        xIDataProvider
                                                            ->deleteDataPointForAllSequences(nId);
                                                    }
                                                    else
                                                    {
                                                        xIDataProvider
                                                            ->insertDataPointForAllSequences(nId
                                                                                             - 1);
                                                    }
                                                }
                                            }
                                            // set values also, if needed
                                            if (!bDelete && aItem3.second.size() > 0)
                                            {
                                                uno::Sequence<uno::Sequence<double>> aData
                                                    = xDataArray->getData();
                                                uno::Sequence<double>* pRows = aData.getArray();

                                                int nIndex = 0;
                                                int nX = nId;
                                                int nY = nId;
                                                bool bIndexWarning = false;
                                                for (const auto& aItem4 : aItem3.second)
                                                {
                                                    if (bSetColumn)
                                                    {
                                                        nY = nIndex;
                                                    }
                                                    else
                                                    {
                                                        nX = nIndex;
                                                    }
                                                    if (nY < aData.getLength() && nY >= 0
                                                        && nX < pRows[nY].getLength() && nX >= 0)
                                                    {
                                                        double* pCols = pRows[nY].getArray();
                                                        pCols[nX]
                                                            = aItem4.second.get_value<double>();
                                                    }
                                                    else
                                                    {
                                                        bIndexWarning = true;
                                                    }

                                                    nIndex++;
                                                }
                                                if (bIndexWarning)
                                                {
                                                    std::string sValues = "";
                                                    for (const auto& atemp : aItem3.second)
                                                    {
                                                        if (sValues != "")
                                                        {
                                                            sValues += ", ";
                                                        }
                                                        sValues += atemp.second
                                                                       .get_value<std::string>();
                                                    }
                                                    lcl_LogWarning(
                                                        "FillApi chart: Invalid Cell Index at: '"
                                                        + aItem3.first + ": " + sValues
                                                        + "' (probably too many parameters)");
                                                }

                                                xDataArray->setData(aData);
                                            }
                                        }
                                        else if (aItem3.first.starts_with("setrowdesc"))
                                        {
                                            // set row descriptions
                                            uno::Sequence<OUString> aRowDesc
                                                = xDataArray->getRowDescriptions();
                                            OUString* aRowdata = aRowDesc.getArray();

                                            if (aItem3.first.starts_with("setrowdesc."))
                                            {
                                                // set only 1 description
                                                int nValue = stoi(aItem3.first.substr(11));
                                                if (nValue >= 0 && nValue < aRowDesc.getLength())
                                                {
                                                    aRowdata[nValue] = OStringToOUString(
                                                        aItem3.second.get_value<std::string>(),
                                                        RTL_TEXTENCODING_UTF8);
                                                }
                                                else
                                                {
                                                    lcl_LogWarning("FillApi chart setrowdesc: "
                                                                   "invalid Index at: '"
                                                                   + aItem3.first + "'");
                                                }
                                            }
                                            else
                                            {
                                                // set an array of description at once
                                                int nIndex = 0;
                                                for (const auto& aItem4 : aItem3.second)
                                                {
                                                    if (nIndex >= aRowDesc.getLength())
                                                    {
                                                        lcl_LogWarning("FillApi chart setrowdesc: "
                                                                       "too many params");
                                                        break;
                                                    }
                                                    aRowdata[nIndex] = OStringToOUString(
                                                        aItem4.second.get_value<std::string>(),
                                                        RTL_TEXTENCODING_UTF8);
                                                    nIndex++;
                                                }
                                            }
                                            xDataArray->setRowDescriptions(aRowDesc);
                                        }
                                        else if (aItem3.first.starts_with("setcolumndesc"))
                                        {
                                            // set column descriptions
                                            uno::Sequence<OUString> aColDesc
                                                = xDataArray->getColumnDescriptions();
                                            OUString* aColdata = aColDesc.getArray();

                                            if (aItem3.first.starts_with("setcolumndesc."))
                                            {
                                                int nValue = stoi(aItem3.first.substr(14));
                                                if (nValue >= 0 && nValue < aColDesc.getLength())
                                                {
                                                    aColdata[nValue] = OStringToOUString(
                                                        aItem3.second.get_value<std::string>(),
                                                        RTL_TEXTENCODING_UTF8);
                                                }
                                                else
                                                {
                                                    lcl_LogWarning("FillApi chart setcolumndesc: "
                                                                   "invalid Index at: '"
                                                                   + aItem3.first + "'");
                                                }
                                            }
                                            else
                                            {
                                                int nIndex = 0;
                                                for (const auto& aItem4 : aItem3.second)
                                                {
                                                    if (nIndex >= aColDesc.getLength())
                                                    {
                                                        lcl_LogWarning(
                                                            "FillApi chart setcolumndesc:"
                                                            " too many parameters");
                                                        break;
                                                    }
                                                    aColdata[nIndex] = OStringToOUString(
                                                        aItem4.second.get_value<std::string>(),
                                                        RTL_TEXTENCODING_UTF8);
                                                    nIndex++;
                                                }
                                            }
                                            xDataArray->setColumnDescriptions(aColDesc);
                                        }
                                        else if (aItem3.first.starts_with("resize"))
                                        {
                                            if (aItem3.second.size() >= 2)
                                            {
                                                auto aItem4 = aItem3.second.begin();
                                                int nY = aItem4->second.get_value<int>();
                                                int nX = (++aItem4)->second.get_value<int>();

                                                if (nX < 1 || nY < 1)
                                                {
                                                    lcl_LogWarning(
                                                        "FillApi chart resize: wrong param"
                                                        " (Needed: x,y >= 1)");
                                                    continue;
                                                }
                                                // here we need to use the new insert column thing
                                                if (!lcl_ResizeChartColumns(xChartDoc, nX))
                                                    continue;

                                                uno::Sequence<uno::Sequence<double>> aData
                                                    = xDataArray->getData();
                                                if (aData.getLength() != nY)
                                                    aData.realloc(nY);

                                                for (sal_Int32 j = 0; j < nY; ++j)
                                                {
                                                    uno::Sequence<double>* pRows = aData.getArray();
                                                    // resize row if needed
                                                    if (pRows[j].getLength() != nX)
                                                    {
                                                        pRows[j].realloc(nX);
                                                    }
                                                }
                                                xDataArray->setData(aData);
                                            }
                                            else
                                            {
                                                lcl_LogWarning(
                                                    "FillApi chart resize: not enough parameters"
                                                    " (x,y is needed)");
                                            }
                                        }
                                        else if (aItem3.first.starts_with("data"))
                                        {
                                            // set table data values
                                            uno::Sequence<uno::Sequence<double>> aData
                                                = xDataArray->getData();

                                            // set only 1 cell data
                                            if (aItem3.first.starts_with("datayx."))
                                            {
                                                int nPoint = aItem3.first.find('.', 7);
                                                int nY = stoi(aItem3.first.substr(7, nPoint - 7));
                                                int nX = stoi(aItem3.first.substr(nPoint + 1));
                                                bool bValidIndex = false;
                                                if (nY < aData.getLength() && nY >= 0)
                                                {
                                                    uno::Sequence<double>* pRows = aData.getArray();
                                                    if (nX < pRows[nY].getLength() && nX >= 0)
                                                    {
                                                        double* pCols = pRows[nY].getArray();
                                                        pCols[nX]
                                                            = aItem3.second.get_value<double>();
                                                        bValidIndex = true;
                                                    }
                                                }
                                                if (!bValidIndex)
                                                {
                                                    lcl_LogWarning(
                                                        "FillApi chart datayx: invalid Index at: '"
                                                        + aItem3.first + "'");
                                                }
                                            }
                                            else
                                            {
                                                // set the whole data table
                                                // resize if needed
                                                int nRowsCount = aItem3.second.size();
                                                int nColsCount = 0;

                                                for (const auto& aItem4 : aItem3.second)
                                                {
                                                    if (nColsCount
                                                        < static_cast<int>(aItem4.second.size()))
                                                    {
                                                        nColsCount = aItem4.second.size();
                                                    }
                                                }

                                                if (nColsCount > 0)
                                                {
                                                    // here we need to use the new insert column thing
                                                    if(!lcl_ResizeChartColumns(xChartDoc, nColsCount))
                                                        continue;

                                                    if (aData.getLength() != nRowsCount)
                                                        aData.realloc(nRowsCount);

                                                    // set all the rows
                                                    sal_Int32 nY = 0;
                                                    for (const auto& aItem4 : aItem3.second)
                                                    {
                                                        uno::Sequence<double>* pRows
                                                            = aData.getArray();
                                                        // resize row if needed
                                                        if (pRows[nY].getLength() != nColsCount)
                                                        {
                                                            pRows[nY].realloc(nColsCount);
                                                        }
                                                        double* pCols = pRows[nY].getArray();
                                                        // set all values in the row
                                                        sal_Int32 nX = 0;
                                                        for (const auto& aItem5 : aItem4.second)
                                                        {
                                                            if (nX >= nColsCount)
                                                            {
                                                                // This should never happen
                                                                break;
                                                            }
                                                            pCols[nX]
                                                                = aItem5.second.get_value<double>();
                                                            nX++;
                                                        }
                                                        nY++;
                                                    }
                                                }
                                            }
                                            xDataArray->setData(aData);
                                        }
                                        else
                                        {
                                            lcl_LogWarning("FillApi chart command not recognised: '"
                                                           + aItem3.first + "'");
                                        }
                                        xModi->setModified(true);
                                    }
                                }
                                if (!bChartFound)
                                {
                                    lcl_LogWarning("FillApi: No chart match the filter: '"
                                                   + aItem2.first + "'");
                                }
                            }
                            else
                            {
                                lcl_LogWarning("FillApi chart filter type not recognised: '"
                                               + aItem2.first + "'");
                            }
                        }

                        if (aItem2.first.starts_with("ContentControls"))
                        {
                            std::string aTextEnd = aItem2.first.substr(15);
                            std::string aValue = "";
                            ContentFilterType iKeyId = ContentFilterType::ERROR;
                            // Find how the content control is identified: ByIndex, ByAlias...
                            for (size_t i = 0; i < aIdTexts.size(); i++)
                            {
                                if (aTextEnd.starts_with(aIdTexts[i]))
                                {
                                    iKeyId = static_cast<ContentFilterType>(i);
                                    aValue = aTextEnd.substr(aIdTexts[i].length());
                                    break;
                                }
                            }
                            if (iKeyId != ContentFilterType::ERROR)
                            {
                                // Check all the content controls, if they match
                                bool bCCFound = false;
                                for (int i = 0; i < iCCcount; ++i)
                                {
                                    uno::Reference<text::XTextContent> xContentControl;
                                    xContentControls->getByIndex(i) >>= xContentControl;

                                    uno::Reference<beans::XPropertySet> xContentControlProps(
                                        xContentControl, uno::UNO_QUERY);
                                    if (!xContentControlProps.is())
                                        continue;

                                    // Compare the loaded and the actual identifier
                                    switch (iKeyId)
                                    {
                                        case ContentFilterType::INDEX:
                                        {
                                            if (stoi(aValue) != i)
                                                continue;
                                        }
                                        break;
                                        case ContentFilterType::ID:
                                        {
                                            sal_Int32 iID = -1;
                                            xContentControlProps->getPropertyValue(UNO_NAME_ID)
                                                >>= iID;
                                            if (stoi(aValue) != iID)
                                                continue;
                                        }
                                        break;
                                        case ContentFilterType::ALIAS:
                                        {
                                            OUString aAlias;
                                            xContentControlProps->getPropertyValue(UNO_NAME_ALIAS)
                                                >>= aAlias;
                                            if (OStringToOUString(aValue, RTL_TEXTENCODING_UTF8)
                                                != aAlias)
                                                continue;
                                        }
                                        break;
                                        case ContentFilterType::TAG:
                                        {
                                            OUString aTag;
                                            xContentControlProps->getPropertyValue(UNO_NAME_TAG)
                                                >>= aTag;
                                            if (OStringToOUString(aValue, RTL_TEXTENCODING_UTF8)
                                                != aTag)
                                                continue;
                                        }
                                        break;
                                        default:
                                            continue;
                                    }

                                    // We have a match, this content control need to be transformed
                                    // Set all the values (of the content control) what is needed
                                    bCCFound = true;
                                    for (const auto& aItem3 : aItem2.second)
                                    {
                                        if (aItem3.first == "content")
                                        {
                                            std::string aContent
                                                = aItem3.second.get_value<std::string>();

                                            uno::Reference<text::XText> xContentControlText(
                                                xContentControl, uno::UNO_QUERY);
                                            if (!xContentControlText.is())
                                                continue;

                                            xContentControlText->setString(
                                                OStringToOUString(aContent, RTL_TEXTENCODING_UTF8));

                                            sal_Int32 iType = 0;
                                            xContentControlProps->getPropertyValue(
                                                UNO_NAME_CONTENT_CONTROL_TYPE)
                                                >>= iType;
                                            SwContentControlType aType
                                                = static_cast<SwContentControlType>(iType);

                                            // if we set the content of a checkbox, then we
                                            // also set the checked state based on the content
                                            if (aType == SwContentControlType::CHECKBOX)
                                            {
                                                OUString aCheckedContent;
                                                xContentControlProps->getPropertyValue(
                                                    UNO_NAME_CHECKED_STATE)
                                                    >>= aCheckedContent;
                                                bool bChecked = false;
                                                if (aCheckedContent
                                                    == OStringToOUString(
                                                        aItem3.second.get_value<std::string>(),
                                                        RTL_TEXTENCODING_UTF8))
                                                    bChecked = true;
                                                xContentControlProps->setPropertyValue(
                                                    UNO_NAME_CHECKED, uno::Any(bChecked));
                                            }
                                            else if (aType == SwContentControlType::PLAIN_TEXT
                                                     || aType == SwContentControlType::RICH_TEXT
                                                     || aType == SwContentControlType::DATE
                                                     || aType == SwContentControlType::COMBO_BOX
                                                     || aType
                                                            == SwContentControlType::DROP_DOWN_LIST)
                                            {
                                                // Set the placeholder
                                                bool bPlaceHolder = aContent == "" ? true : false;
                                                xContentControlProps->setPropertyValue(
                                                    UNO_NAME_SHOWING_PLACE_HOLDER,
                                                    uno::Any(bPlaceHolder));
                                                if (bPlaceHolder)
                                                {
                                                    OUString aPlaceHolderText;
                                                    switch (aType)
                                                    {
                                                        case SwContentControlType::PLAIN_TEXT:
                                                        case SwContentControlType::RICH_TEXT:
                                                        {
                                                            aPlaceHolderText = SwResId(
                                                                STR_CONTENT_CONTROL_PLACEHOLDER);
                                                        }
                                                        break;
                                                        case SwContentControlType::COMBO_BOX:
                                                        case SwContentControlType::DROP_DOWN_LIST:
                                                        {
                                                            aPlaceHolderText = SwResId(
                                                                STR_DROPDOWN_CONTENT_CONTROL_PLACEHOLDER);
                                                        }
                                                        break;
                                                        case SwContentControlType::DATE:
                                                        {
                                                            aPlaceHolderText = SwResId(
                                                                STR_DATE_CONTENT_CONTROL_PLACEHOLDER);
                                                        }
                                                        break;
                                                        default: // do nothing for picture and checkbox
                                                        break;
                                                    }
                                                    if (!aPlaceHolderText.isEmpty())
                                                        xContentControlText->setString(
                                                            aPlaceHolderText);
                                                }
                                            }
                                        }
                                        else if (aItem3.first == "checked")
                                        {
                                            bool bChecked
                                                = (aItem3.second.get_value<std::string>() == "true")
                                                      ? true
                                                      : false;
                                            xContentControlProps->setPropertyValue(
                                                UNO_NAME_CHECKED,
                                                uno::Any(bChecked));

                                            OUString aCheckContent;
                                            xContentControlProps->getPropertyValue(
                                                bChecked ? UNO_NAME_CHECKED_STATE
                                                         : UNO_NAME_UNCHECKED_STATE)
                                                >>= aCheckContent;
                                            uno::Reference<text::XText> xContentControlText(
                                                xContentControl, uno::UNO_QUERY);
                                            if (!xContentControlText.is())
                                                continue;
                                            xContentControlText->setString(aCheckContent);
                                        }
                                        else if (aItem3.first == "date")
                                        {
                                            std::string aDate
                                                = aItem3.second.get_value<std::string>();
                                            xContentControlProps->setPropertyValue(
                                                UNO_NAME_CURRENT_DATE,
                                                uno::Any(OStringToOUString(aDate,
                                                                           RTL_TEXTENCODING_UTF8)));
                                        }
                                        else if (aItem3.first == "alias")
                                        {
                                            xContentControlProps->setPropertyValue(
                                                UNO_NAME_ALIAS,
                                                uno::Any(OStringToOUString(
                                                    aItem3.second.get_value<std::string>(),
                                                    RTL_TEXTENCODING_UTF8)));
                                        }
                                        else
                                        {
                                            lcl_LogWarning(
                                                "FillApi contentControl command not recognised: '"
                                                + aItem3.first + "'");
                                        }
                                    }
                                }
                                if (!bCCFound)
                                {
                                    lcl_LogWarning("FillApi: No contentControl match the filter: '"
                                                   + aItem2.first + "'");
                                }
                            }
                            else
                            {
                                lcl_LogWarning(
                                    "FillApi contentControl filter type not recognised: '"
                                    + aItem2.first + "'");
                            }
                        }
                    }
                }
            }
        }
        break;
    case FN_INSERT_WHISPER_TEXT:
    case FN_WHISPER_START_STOP:
    case FN_WHISPER_SETTINGS:
        ExecWhisper(rReq);
        break;
        
    default:
        OSL_ENSURE(false, "wrong dispatcher");
        return;
    }
}

void SwTextShell::GetState( SfxItemSet &rSet )
{
    SwWrtShell &rSh = GetShell();
    SfxWhichIter aIter( rSet );
    sal_uInt16 nWhich = aIter.FirstWhich();
    
    // Debug: log all state queries
    static int nCallCount = 0;
    nCallCount++;
    if (nCallCount < 50) // Limit logging to avoid spam
    {
        SAL_WARN("sw.whisper", "SwTextShell::GetState called (#" << nCallCount << "), first which: " << nWhich);
        // Log if this is near our whisper commands (21444-21446)
        if (nWhich >= 21440 && nWhich <= 21450)
        {
            SAL_WARN("sw.whisper", "*** NEAR WHISPER RANGE: which=" << nWhich);
        }
    }
    // Log all which values when iterating
    static bool bLoggedAll = false;
    if (!bLoggedAll && nCallCount == 1)
    {
        SAL_WARN("sw.whisper", "=== Logging all which values in first GetState call ===");
        SfxWhichIter aDebugIter(rSet);
        sal_uInt16 nDebugWhich = aDebugIter.FirstWhich();
        while (nDebugWhich)
        {
            SAL_WARN("sw.whisper", "  Which: " << nDebugWhich);
            nDebugWhich = aDebugIter.NextWhich();
        }
        bLoggedAll = true;
    }
    
    while ( nWhich )
    {
        const sal_uInt16 nSlotId = GetPool().GetSlotId(nWhich);
        
        // Debug specific whisper commands
        if (nSlotId == FN_INSERT_WHISPER_TEXT || nSlotId == FN_WHISPER_START_STOP || nSlotId == FN_WHISPER_SETTINGS ||
            nWhich == FN_INSERT_WHISPER_TEXT || nWhich == FN_WHISPER_START_STOP || nWhich == FN_WHISPER_SETTINGS)
        {
            SAL_WARN("sw.whisper", "WHISPER COMMAND FOUND! nWhich=" << nWhich << ", nSlotId=" << nSlotId 
                << " (FN_INSERT_WHISPER_TEXT=" << FN_INSERT_WHISPER_TEXT 
                << ", FN_WHISPER_START_STOP=" << FN_WHISPER_START_STOP
                << ", FN_WHISPER_SETTINGS=" << FN_WHISPER_SETTINGS << ")");
        }
        
        switch (nSlotId)
        {
        case FN_FORMAT_CURRENT_FOOTNOTE_DLG:
            if( !rSh.IsCursorInFootnote() )
                rSet.DisableItem( nWhich );
        break;

        case SID_LANGUAGE_STATUS:
            {
                // the value of used script types
                OUString aScriptTypesInUse( OUString::number( static_cast<int>(rSh.GetScriptType()) ) );

                // get keyboard language
                OUString aKeyboardLang;
                SwEditWin& rEditWin = GetView().GetEditWin();
                LanguageType nLang = rEditWin.GetInputLanguage();
                if (nLang != LANGUAGE_DONTKNOW && nLang != LANGUAGE_SYSTEM)
                    aKeyboardLang = SvtLanguageTable::GetLanguageString( nLang );

                // get the language that is in use
                OUString aCurrentLang = u"*"_ustr;
                nLang = SwLangHelper::GetCurrentLanguage( rSh );
                if (nLang != LANGUAGE_DONTKNOW)
                {
                    aCurrentLang = SvtLanguageTable::GetLanguageString( nLang );
                    if (comphelper::LibreOfficeKit::isActive())
                    {
                        if (nLang == LANGUAGE_NONE)
                        {
                            aCurrentLang += ";-";
                        }
                        else
                        {
                            aCurrentLang += ";" + LanguageTag(nLang).getBcp47(false);
                        }
                    }
                }

                // build sequence for status value
                uno::Sequence< OUString > aSeq{ aCurrentLang,
                                                aScriptTypesInUse,
                                                aKeyboardLang,
                                                SwLangHelper::GetTextForLanguageGuessing( rSh ) };

                // set sequence as status value
                SfxStringListItem aItem( SID_LANGUAGE_STATUS );
                aItem.SetStringList( aSeq );
                rSet.Put( aItem );
            }
        break;

        case SID_THES:
        {
            // is there a valid selection to get text from?
            OUString aText;
            bool bValid = !rSh.HasSelection() ||
                    (rSh.IsSelOnePara() && !rSh.IsMultiSelection());
            // prevent context menu from showing when cursor is not in or at the end of a word
            // (GetCurWord will return the next word if there is none at the current position...)
            const sal_Int16 nWordType = ::i18n::WordType::DICTIONARY_WORD;
            bool bWord = rSh.IsInWord( nWordType ) || rSh.IsStartWord( nWordType ) || rSh.IsEndWord( nWordType );
            if (bValid && bWord)
               aText = rSh.HasSelection()? rSh.GetSelText() : rSh.GetCurWord();

            LanguageType nLang = rSh.GetCurLang();
            LanguageTag aLanguageTag( nLang);
            const lang::Locale& aLocale( aLanguageTag.getLocale());

            // disable "Thesaurus" context menu entry if there is nothing to look up
            uno::Reference< linguistic2::XThesaurus >  xThes( ::GetThesaurus() );
            if (aText.isEmpty() ||
                !xThes.is() || nLang == LANGUAGE_NONE || !xThes->hasLocale( aLocale ))
                rSet.DisableItem( SID_THES );
            else
            {
                // set word and locale to look up as status value
                OUString aStatusVal = aText + "#" + aLanguageTag.getBcp47();
                rSet.Put( SfxStringItem( SID_THES, aStatusVal ) );
            }
        }
        break;

        case FN_NUMBER_NEWSTART :
            if(!rSh.GetNumRuleAtCurrCursorPos())
                    rSet.DisableItem(nWhich);
            else
                rSet.Put(SfxBoolItem(FN_NUMBER_NEWSTART,
                    rSh.IsNumRuleStart()));
        break;

        case FN_EDIT_FORMULA:
        case SID_CHARMAP:
        case SID_CHARMAP_CONTROL:
            {
                const SelectionType nType = rSh.GetSelectionType();
                if (!(nType & SelectionType::Text) &&
                    !(nType & SelectionType::Table) &&
                    !(nType & SelectionType::NumberList))
                {
                    rSet.DisableItem(nWhich);
                }
                else if ( nWhich == FN_EDIT_FORMULA
                          && rSh.CursorInsideInputField() )
                {
                    rSet.DisableItem( nWhich );
                }
            }
            break;

        case FN_INSERT_ENDNOTE:
        case FN_INSERT_FOOTNOTE:
        case FN_INSERT_FOOTNOTE_DLG:
            {
                const FrameTypeFlags nNoType =
                    FrameTypeFlags::FLY_ANY | FrameTypeFlags::HEADER | FrameTypeFlags::FOOTER | FrameTypeFlags::FOOTNOTE;
                FrameTypeFlags eType = rSh.GetFrameType(nullptr, true);
                bool bSplitFly = false;
                if (eType & FrameTypeFlags::FLY_ATCNT)
                {
                    SwContentFrame* pContentFrame = rSh.GetCurrFrame(/*bCalcFrame=*/false);
                    if (pContentFrame)
                    {
                        SwFlyFrame* pFlyFrame = pContentFrame->FindFlyFrame();
                        bSplitFly = pFlyFrame && pFlyFrame->IsFlySplitAllowed();
                    }
                }
                if (eType & nNoType && !bSplitFly)
                    rSet.DisableItem(nWhich);

                if ( rSh.CursorInsideInputField() )
                {
                    rSet.DisableItem( nWhich );
                }
            }
            break;

        case SID_INSERTDOC:
        case FN_INSERT_GLOSSARY:
        case FN_EXPAND_GLOSSARY:
            if ( rSh.CursorInsideInputField() )
            {
                rSet.DisableItem( nWhich );
            }
            break;

        case FN_INSERT_TABLE:
            if ( rSh.CursorInsideInputField()
                 || rSh.GetTableFormat()
                 || (rSh.GetFrameType(nullptr,true) & FrameTypeFlags::FOOTNOTE) )
            {
                rSet.DisableItem( nWhich );
            }
            break;

        case FN_CALCULATE:
            if ( !rSh.IsSelection() )
                rSet.DisableItem(nWhich);
            break;
        case FN_GOTO_REFERENCE:
            {
                SwField *pField = rSh.GetCurField();
                if ( !pField || (pField->GetTypeId() != SwFieldTypesEnum::GetRef) )
                    rSet.DisableItem(nWhich);
            }
            break;
        case FN_AUTOFORMAT_AUTO:
            {
                rSet.Put( SfxBoolItem( nWhich, SvxAutoCorrCfg::Get().IsAutoFormatByInput() ));
            }
            break;

        case SID_DEC_INDENT:
        case SID_INC_INDENT:
        {
            //if the paragraph has bullet we'll do the following things:
            //1: if the bullet level is the first level, disable the decrease-indent button
            //2: if the bullet level is the last level, disable the increase-indent button
            if ( rSh.GetNumRuleAtCurrCursorPos() && !rSh.HasReadonlySel() )
            {
                const sal_uInt8 nLevel = rSh.GetNumLevel();
                if ( ( nLevel == ( MAXLEVEL - 1 ) && nWhich == SID_INC_INDENT )
                     || ( nLevel == 0 && nWhich == SID_DEC_INDENT ) )
                {
                    rSet.DisableItem( nWhich );
                }
            }
            else
            {
                sal_uInt16 nHtmlMode = ::GetHtmlMode( GetView().GetDocShell() );
                nHtmlMode &= HTMLMODE_ON | HTMLMODE_SOME_STYLES;
                if ( ( nHtmlMode == HTMLMODE_ON )
                     || !rSh.IsMoveLeftMargin( SID_INC_INDENT == nWhich ) )
                {
                    rSet.DisableItem( nWhich );
                }
            }
        }
        break;

        case FN_DEC_INDENT_OFFSET:
        case FN_INC_INDENT_OFFSET:
            {
                sal_uInt16 nHtmlMode = ::GetHtmlMode(GetView().GetDocShell());
                nHtmlMode &= HTMLMODE_ON|HTMLMODE_SOME_STYLES;
                if( (nHtmlMode == HTMLMODE_ON) ||
                    !rSh.IsMoveLeftMargin( FN_INC_INDENT_OFFSET == nWhich,
                                            false ))
                    rSet.DisableItem( nWhich );
            }
            break;

        case SID_ATTR_CHAR_COLOR2:
            {
                SfxItemSet aSet( GetPool() );
                rSh.GetCurAttr( aSet );
                const SvxColorItem& aColorItem = aSet.Get(RES_CHRATR_COLOR);
                rSet.Put( aColorItem.CloneSetWhich(SID_ATTR_CHAR_COLOR2) );
            }
            break;
        case SID_ATTR_CHAR_BACK_COLOR:
        case SID_ATTR_CHAR_COLOR_BACKGROUND:
            {
                // Always use the visible background
                SfxItemSet aSet( GetPool() );
                rSh.GetCurAttr( aSet );
                const SvxBrushItem& aBrushItem = aSet.Get(RES_CHRATR_HIGHLIGHT);
                if( aBrushItem.GetColor() != COL_TRANSPARENT )
                {
                    rSet.Put(SvxColorItem(aBrushItem.GetColor(), aBrushItem.getComplexColor(), nWhich));
                }
                else
                {
                    const SvxBrushItem& aBrushItem2 = aSet.Get(RES_CHRATR_BACKGROUND);
                    rSet.Put(SvxColorItem(aBrushItem2.GetColor(), aBrushItem2.getComplexColor(), nWhich));
                }
            }
            break;
        case SID_ATTR_CHAR_COLOR_BACKGROUND_EXT:
            {
                SwEditWin& rEdtWin = GetView().GetEditWin();
                SwApplyTemplate* pApply = rEdtWin.GetApplyTemplate();
                const sal_uInt32 nColWhich = pApply ? pApply->nColor : 0;
                const bool bUseTemplate = nColWhich == SID_ATTR_CHAR_BACK_COLOR
                                          || nColWhich == SID_ATTR_CHAR_COLOR_BACKGROUND;
                rSet.Put(SfxBoolItem(nWhich, bUseTemplate));
            }
            break;
        case SID_ATTR_CHAR_COLOR_EXT:
            {
                SwEditWin& rEdtWin = GetView().GetEditWin();
                SwApplyTemplate* pApply = rEdtWin.GetApplyTemplate();
                rSet.Put(SfxBoolItem(nWhich, pApply && pApply->nColor == nWhich));
            }
            break;
        case FN_SET_REMINDER:
        case FN_INSERT_BOOKMARK:
            if( rSh.IsTableMode()
                || rSh.CursorInsideInputField() )
            {
                rSet.DisableItem( nWhich );
            }
            break;

        case FN_INSERT_BREAK:
            if ( rSh.HasReadonlySel()
                 && !rSh.CursorInsideInputField() )
            {
                rSet.DisableItem( nWhich );
            }
            break;

        case FN_INSERT_BREAK_DLG:
        case FN_INSERT_COLUMN_BREAK:
        case FN_INSERT_PAGEBREAK:
            if( rSh.CursorInsideInputField() || rSh.CursorInsideContentControl() )
            {
                rSet.DisableItem( nWhich );
            }
            break;

        case FN_INSERT_PAGEHEADER:
        case FN_INSERT_PAGEFOOTER:
            if (comphelper::LibreOfficeKit::isActive())
            {
                bool bState = false;
                bool bAllState = true;
                bool bIsPhysical = false;

                OUString aStyleName;
                std::vector<OUString> aList;
                static constexpr OUStringLiteral sPhysical(u"IsPhysical");
                static constexpr OUStringLiteral sDisplay(u"DisplayName");
                const OUString sHeaderOn(nWhich == FN_INSERT_PAGEHEADER ? u"HeaderIsOn"_ustr : u"FooterIsOn"_ustr);

                rtl::Reference< SwXTextDocument > xSupplier(GetView().GetDocShell()->GetBaseModel());
                if (xSupplier.is())
                {
                    uno::Reference< XNameContainer > xContainer;
                    uno::Reference< XNameAccess > xFamilies = xSupplier->getStyleFamilies();
                    if (xFamilies->getByName(u"PageStyles"_ustr) >>= xContainer)
                    {
                        const uno::Sequence< OUString > aSeqNames = xContainer->getElementNames();
                        for (const auto& rName : aSeqNames)
                        {
                            aStyleName = rName;
                            uno::Reference<XPropertySet> xPropSet(xContainer->getByName(aStyleName), uno::UNO_QUERY);
                            if (xPropSet.is() && (xPropSet->getPropertyValue(sPhysical) >>= bIsPhysical) && bIsPhysical)
                            {
                                xPropSet->getPropertyValue(sDisplay) >>= aStyleName;
                                if ((xPropSet->getPropertyValue(sHeaderOn)>>= bState) && bState)
                                    aList.push_back(aStyleName);
                                else
                                    bState = false;

                                // Check if all entries have the same state
                                bAllState &= bState;
                            }
                            else
                                bIsPhysical = false;
                        }
                    }
                }

                if (bAllState && aList.size() > 1)
                    aList.push_back(u"_ALL_"_ustr);

                rSet.Put(SfxStringListItem(nWhich, &aList));
            }
            else
            {
                rSet.Put( SfxObjectShellItem( nWhich, GetView().GetDocShell() ));
            }
            break;
            case FN_TABLE_SORT_DIALOG:
            case FN_SORTING_DLG:
                if(!rSh.HasSelection() ||
                        (FN_TABLE_SORT_DIALOG == nWhich && !rSh.GetTableFormat()))
                    rSet.DisableItem( nWhich );
            break;

            case SID_RUBY_DIALOG:
                {
                    if( !SvtCJKOptions::IsRubyEnabled()
                        || rSh.CursorInsideInputField() )
                    {
                        GetView().GetViewFrame().GetBindings().SetVisibleState( nWhich, false );
                        rSet.DisableItem(nWhich);
                    }
                    else
                        GetView().GetViewFrame().GetBindings().SetVisibleState( nWhich, true );
                }
                break;

            case SID_FM_TRANSLATE:
                {
#if HAVE_FEATURE_CURL && !ENABLE_WASM_STRIP_EXTRA
                    if (!officecfg::Office::Common::Misc::ExperimentalMode::get()
                        && !comphelper::LibreOfficeKit::isActive())
                    {
                        rSet.Put(SfxVisibilityItem(nWhich, false));
                        break;
                    }
                    std::optional<OUString> oDeeplAPIUrl = officecfg::Office::Linguistic::Translation::Deepl::ApiURL::get();
                    std::optional<OUString> oDeeplKey = officecfg::Office::Linguistic::Translation::Deepl::AuthKey::get();
                    if (!oDeeplAPIUrl || oDeeplAPIUrl->isEmpty() || !oDeeplKey || oDeeplKey->isEmpty())
                    {
                        rSet.DisableItem(nWhich);
                    }
#endif
                }
                break;

            case SID_HYPERLINK_DIALOG:
                if( GetView().GetDocShell()->IsReadOnly()
                    || ( !GetView().GetViewFrame().HasChildWindow(nWhich)
                         && rSh.HasReadonlySel() )
                    || rSh.CursorInsideInputField() )
                {
                    rSet.DisableItem(nWhich);
                }
                else
                {
                    rSet.Put(SfxBoolItem( nWhich, nullptr != GetView().GetViewFrame().GetChildWindow( nWhich ) ));
                }
                break;

            case SID_EDIT_HYPERLINK:
                {
                    if (!rSh.HasReadonlySel())
                    {
                        SfxItemSetFixed<RES_TXTATR_INETFMT, RES_TXTATR_INETFMT> aSet(GetPool());
                        rSh.GetCurAttr(aSet);
                        if (SfxItemState::SET <= aSet.GetItemState(RES_TXTATR_INETFMT))
                            break;

                        // is the cursor at the beginning of a hyperlink?
                        const SwTextNode* pTextNd = rSh.GetCursor()->GetPointNode().GetTextNode();
                        if (pTextNd && !rSh.HasSelection())
                        {
                            const sal_Int32 nIndex = rSh.GetCursor()->Start()->GetContentIndex();
                            const SwTextAttr* pINetFmt
                                = pTextNd->GetTextAttrAt(nIndex, RES_TXTATR_INETFMT);
                            if (pINetFmt && !pINetFmt->GetINetFormat().GetValue().isEmpty())
                                break;
                        }
                    }
                    rSet.DisableItem(nWhich);
                }
            break;
            case SID_INSERT_HYPERLINK:
            {
                if (!rSh.HasSelection())
                {
                    rSet.DisableItem(nWhich);
                    break;
                }
                if (!rSh.HasReadonlySel())
                {
                    SfxItemSetFixed<RES_TXTATR_INETFMT, RES_TXTATR_INETFMT> aSet(GetPool());
                    rSh.GetCurAttr(aSet);

                    // If a hyperlink is selected, either alone or along with other text...
                    if (SfxItemState::SET <= aSet.GetItemState(RES_TXTATR_INETFMT)
                        || aSet.GetItemState(RES_TXTATR_INETFMT) == SfxItemState::INVALID)
                    {
                        rSet.DisableItem(nWhich);
                    }

                    // is the cursor at the beginning of a hyperlink?
                    const SwTextNode* pTextNd = rSh.GetCursor()->GetPointNode().GetTextNode();
                    if (pTextNd && !rSh.HasSelection())
                    {
                        const sal_Int32 nIndex = rSh.GetCursor()->Start()->GetContentIndex();
                        const SwTextAttr* pINetFmt
                            = pTextNd->GetTextAttrAt(nIndex, RES_TXTATR_INETFMT);
                        if (pINetFmt && !pINetFmt->GetINetFormat().GetValue().isEmpty())
                            rSet.DisableItem(nWhich);
                    }
                }
            }
            break;
            case SID_REMOVE_HYPERLINK:
            {
                if (!rSh.HasReadonlySel())
                {
                    SfxItemSetFixed<RES_TXTATR_INETFMT, RES_TXTATR_INETFMT> aSet(GetPool());
                    rSh.GetCurAttr(aSet);

                    // If a hyperlink is selected, either alone or along with other text...
                    if (SfxItemState::SET <= aSet.GetItemState(RES_TXTATR_INETFMT)
                        || aSet.GetItemState(RES_TXTATR_INETFMT) == SfxItemState::INVALID)
                    {
                        break;
                    }

                    // is the cursor at the beginning of a hyperlink?
                    const SwTextNode* pTextNd = rSh.GetCursor()->GetPointNode().GetTextNode();
                    if (pTextNd && !rSh.HasSelection())
                    {
                        const sal_Int32 nIndex = rSh.GetCursor()->Start()->GetContentIndex();
                        const SwTextAttr* pINetFmt
                            = pTextNd->GetTextAttrAt(nIndex, RES_TXTATR_INETFMT);
                        if (pINetFmt && !pINetFmt->GetINetFormat().GetValue().isEmpty())
                            break;
                    }
                }
                rSet.DisableItem(nWhich);
            }
            break;
            case SID_TRANSLITERATE_HALFWIDTH:
            case SID_TRANSLITERATE_FULLWIDTH:
            case SID_TRANSLITERATE_HIRAGANA:
            case SID_TRANSLITERATE_KATAKANA:
            {
                if(!SvtCJKOptions::IsChangeCaseMapEnabled())
                {
                    GetView().GetViewFrame().GetBindings().SetVisibleState( nWhich, false );
                    rSet.DisableItem(nWhich);
                }
                else
                    GetView().GetViewFrame().GetBindings().SetVisibleState( nWhich, true );
            }
            break;
            case FN_READONLY_SELECTION_MODE :
                if(!GetView().GetDocShell()->IsReadOnly())
                    rSet.DisableItem( nWhich );
                else
                {
                    rSet.Put(SfxBoolItem(nWhich, rSh.GetViewOptions()->IsSelectionInReadonly()));
                }
            break;
            case FN_SELECTION_MODE_DEFAULT:
            case FN_SELECTION_MODE_BLOCK :
                    rSet.Put(SfxBoolItem(nWhich, (nWhich == FN_SELECTION_MODE_DEFAULT) != rSh.IsBlockMode()));
            break;
            case SID_COPY_HYPERLINK_LOCATION:
            case SID_OPEN_HYPERLINK:
            {
                SfxItemSetFixed<RES_TXTATR_INETFMT, RES_TXTATR_INETFMT> aSet(GetPool());
                rSh.GetCurAttr(aSet);
                if (SfxItemState::SET <= aSet.GetItemState(RES_TXTATR_INETFMT, false))
                    break;

                // is the cursor at the beginning of a hyperlink?
                const SwTextNode* pTextNd = rSh.GetCursor()->GetPointNode().GetTextNode();
                if (pTextNd && !rSh.HasSelection())
                {
                    const sal_Int32 nIndex = rSh.GetCursor()->Start()->GetContentIndex();
                    const SwTextAttr* pINetFmt = pTextNd->GetTextAttrAt(nIndex, RES_TXTATR_INETFMT);
                    if (pINetFmt && !pINetFmt->GetINetFormat().GetValue().isEmpty())
                        break;
                }

                SwField* pField = rSh.GetCurField();
                if (pField && pField->GetTyp()->Which() == SwFieldIds::TableOfAuthorities)
                {
                    const auto& rAuthorityField = *static_cast<const SwAuthorityField*>(pField);
                    if (auto targetType = rAuthorityField.GetTargetType();
                        targetType == SwAuthorityField::TargetType::UseDisplayURL
                        || targetType == SwAuthorityField::TargetType::UseTargetURL)
                    {
                        // Check if the Bibliography entry has a target URL
                        if (rAuthorityField.GetAbsoluteURL().getLength() > 0)
                            break;
                    }
                }

                rSet.DisableItem(nWhich);
            }
            break;
            case FN_OPEN_LOCAL_URL:
            {
                if (GetLocalURL(rSh).isEmpty())
                {
                    rSet.DisableItem(nWhich);
                }
            }
            break;
            case  SID_OPEN_SMARTTAGMENU:
            {
                 std::vector< OUString > aSmartTagTypes;
                 uno::Sequence< uno::Reference< container::XStringKeyMap > > aStringKeyMaps;
                 uno::Reference<text::XTextRange> xRange;

                 rSh.GetSmartTagTerm( aSmartTagTypes, aStringKeyMaps, xRange );

                 if ( xRange.is() && !aSmartTagTypes.empty() )
                 {
                     uno::Sequence < uno::Sequence< uno::Reference< smarttags::XSmartTagAction > > > aActionComponentsSequence;
                     uno::Sequence < uno::Sequence< sal_Int32 > > aActionIndicesSequence;

                     const SmartTagMgr& rSmartTagMgr = SwSmartTagMgr::Get();
                     rSmartTagMgr.GetActionSequences( aSmartTagTypes,
                                                      aActionComponentsSequence,
                                                      aActionIndicesSequence );

                     uno::Reference <frame::XController> xController = GetView().GetController();
                     lang::Locale aLocale( SW_BREAKITER()->GetLocale( GetAppLanguageTag() ) );
                     const OUString& aApplicationName( rSmartTagMgr.GetApplicationName() );
                     const OUString aRangeText = xRange->getString();

                     const SvxSmartTagItem aItem( SID_OPEN_SMARTTAGMENU,
                                                  aActionComponentsSequence,
                                                  aActionIndicesSequence,
                                                  aStringKeyMaps,
                                                  xRange,
                                                  std::move(xController),
                                                  std::move(aLocale),
                                                  aApplicationName,
                                                  aRangeText );

                     rSet.Put( aItem );
                 }
                 else
                     rSet.DisableItem(nWhich);
            }
            break;

            case FN_NUM_NUMBERING_ON:
                rSet.Put(SfxBoolItem(FN_NUM_NUMBERING_ON,rSh.SelectionHasNumber()));
            break;

            case FN_NUM_BULLET_ON:
                rSet.Put(SfxBoolItem(FN_NUM_BULLET_ON,rSh.SelectionHasBullet()));
            break;

            case FN_NUM_BULLET_OFF:
                rSet.Put(SfxBoolItem(FN_NUM_BULLET_OFF, !rSh.GetNumRuleAtCurrCursorPos() &&
                                     !rSh.GetNumRuleAtCurrentSelection()));
            break;

            case FN_SVX_SET_OUTLINE:
            {
                NBOTypeMgrBase* pOutline = NBOutlineTypeMgrFact::CreateInstance(NBOType::Outline);
                auto pCurRule = const_cast<SwNumRule*>(rSh.GetNumRuleAtCurrCursorPos());
                if (pOutline && pCurRule)
                {
                    SvxNumRule aSvxRule = pCurRule->MakeSvxNumRule();
                    const sal_uInt16 nIndex = pOutline->GetNBOIndexForNumRule(aSvxRule, 0);
                    rSet.Put(SfxBoolItem(FN_SVX_SET_OUTLINE, nIndex < USHRT_MAX));
                }
                break;
            }
            case FN_BUL_NUM_RULE_INDEX:
            case FN_NUM_NUM_RULE_INDEX:
            case FN_OUTLINE_RULE_INDEX:
        {
            SwNumRule* pCurRule = const_cast<SwNumRule*>(GetShell().GetNumRuleAtCurrCursorPos());
            if( pCurRule )
            {
                sal_uInt16 nActNumLvl = GetShell().GetNumLevel();
                if( nActNumLvl < MAXLEVEL )
                {
                    nActNumLvl = 1<<nActNumLvl;
                }
                SvxNumRule aSvxRule = pCurRule->MakeSvxNumRule();
                if ( GetShell().HasBullet())
                {
                    rSet.Put(SfxUInt16Item(FN_BUL_NUM_RULE_INDEX, USHRT_MAX));
                    rSet.Put(SfxUInt16Item(FN_NUM_NUM_RULE_INDEX, USHRT_MAX));
                    NBOTypeMgrBase* pBullets = NBOutlineTypeMgrFact::CreateInstance(NBOType::Bullets);
                    if ( pBullets )
                    {
                        const sal_uInt16 nBulIndex = pBullets->GetNBOIndexForNumRule(aSvxRule,nActNumLvl);
                        rSet.Put(SfxUInt16Item(FN_BUL_NUM_RULE_INDEX,nBulIndex));
                    }
                }else if ( GetShell().HasNumber() )
                {
                    rSet.Put(SfxUInt16Item(FN_BUL_NUM_RULE_INDEX, USHRT_MAX));
                    rSet.Put(SfxUInt16Item(FN_NUM_NUM_RULE_INDEX, USHRT_MAX));
                    NBOTypeMgrBase* pNumbering = NBOutlineTypeMgrFact::CreateInstance(NBOType::Numbering);
                    if ( pNumbering )
                    {
                        const sal_uInt16 nBulIndex = pNumbering->GetNBOIndexForNumRule(aSvxRule,nActNumLvl);
                        rSet.Put(SfxUInt16Item(FN_NUM_NUM_RULE_INDEX,nBulIndex));
                    }
                }

                if ( nWhich == FN_OUTLINE_RULE_INDEX )
                {
                    rSet.Put(SfxUInt16Item(FN_OUTLINE_RULE_INDEX, USHRT_MAX));
                    NBOTypeMgrBase* pOutline = NBOutlineTypeMgrFact::CreateInstance(NBOType::Outline);
                    if ( pOutline )
                    {
                        const sal_uInt16 nIndex = pOutline->GetNBOIndexForNumRule(aSvxRule,nActNumLvl);
                        rSet.Put(SfxUInt16Item(FN_OUTLINE_RULE_INDEX,nIndex));
                    }
                }
            }
        }
            break;
            case FN_BUL_GET_DOC_BULLETS:
            {
                std::set<OUString> aBulletsSet = rSh.GetUsedBullets();
                std::vector<OUString> aBullets;
                std::copy(aBulletsSet.begin(), aBulletsSet.end(), std::back_inserter(aBullets));
                SfxStringListItem aItem(FN_BUL_GET_DOC_BULLETS);
                uno::Sequence<OUString> aSeq(aBullets.data(),
                                             static_cast<sal_Int32>(aBullets.size()));
                aItem.SetStringList(aSeq);
                rSet.Put(aItem);
            }
            break;
            case FN_NUM_CONTINUE:
            {
                // #i86492#
                // Search also for bullet list
                OUString aDummy;
                const SwNumRule* pRule =
                        rSh.SearchNumRule( true, aDummy );
                if ( !pRule )
                {
                    pRule = rSh.SearchNumRule( false, aDummy );
                }
                if ( !pRule )
                    rSet.DisableItem(nWhich);
            }
            break;
            case SID_INSERT_RLM :
            case SID_INSERT_LRM :
            {
                bool bEnabled = SvtCTLOptions::IsCTLFontEnabled();
                GetView().GetViewFrame().GetBindings().SetVisibleState( nWhich, bEnabled );
                if(!bEnabled)
                    rSet.DisableItem(nWhich);
            }
            break;
            case SID_FM_CTL_PROPERTIES:
            {
                bool bDisable = false;

                // First get the state from the form shell
                SfxItemSetFixed<SID_FM_CTL_PROPERTIES, SID_FM_CTL_PROPERTIES> aSet(GetShell().GetAttrPool());
                aSet.Put(SfxBoolItem( SID_FM_CTL_PROPERTIES, true ));
                GetShell().GetView().GetFormShell()->GetState( aSet );

                if(SfxItemState::DISABLED == aSet.GetItemState(SID_FM_CTL_PROPERTIES))
                {
                    bDisable = true;
                }

                // Enable it if we have a valid object other than what form shell knows
                SwPosition aPos(*GetShell().GetCursor()->GetPoint());
                sw::mark::Fieldmark* pFieldBM = GetShell().getIDocumentMarkAccess()->getInnerFieldmarkFor(aPos);
                if ( !pFieldBM && aPos.GetContentIndex() > 0)
                {
                    aPos.AdjustContent(-1);
                    pFieldBM = GetShell().getIDocumentMarkAccess()->getInnerFieldmarkFor(aPos);
                }
                if ( pFieldBM && (pFieldBM->GetFieldname() == ODF_FORMDROPDOWN || pFieldBM->GetFieldname() == ODF_FORMDATE) )
                {
                    bDisable = false;
                }

                if(bDisable)
                    rSet.DisableItem(nWhich);
            }
            break;
            case SID_COPY:
            case SID_CUT:
            {
                if (GetObjectShell()->isContentExtractionLocked())
                    rSet.DisableItem(nWhich);
                break;
            }
            case FN_PROTECT_FIELDS:
            case FN_PROTECT_BOOKMARKS:
            {
                DocumentSettingId aSettingId = nWhich == FN_PROTECT_FIELDS
                                                   ? DocumentSettingId::PROTECT_FIELDS
                                                   : DocumentSettingId::PROTECT_BOOKMARKS;
                bool bProtected = rSh.getIDocumentSettingAccess().get(aSettingId);
                rSet.Put(SfxBoolItem(nWhich, bProtected));
            }
            break;
            case FN_DELETE_CONTENT_CONTROL:
            case FN_CONTENT_CONTROL_PROPERTIES:
            {
                if (!GetShell().CursorInsideContentControl())
                {
                    rSet.DisableItem(nWhich);
                }
            }
            break;
            
        case FN_INSERT_WHISPER_TEXT:
        case FN_WHISPER_START_STOP:
        case FN_WHISPER_SETTINGS:
            SAL_WARN("sw.whisper", "GetState: Whisper command detected: " << nSlotId);
            GetWhisperState(rSet);
            break;
        }
        nWhich = aIter.NextWhich();
    }
    
    // Check if any whisper commands were missed
    if (rSet.GetItemState(FN_INSERT_WHISPER_TEXT) == SfxItemState::INVALID)
    {
        SAL_WARN("sw.whisper", "FN_INSERT_WHISPER_TEXT not handled, adding manually");
        GetWhisperState(rSet);
    }
    if (rSet.GetItemState(FN_WHISPER_START_STOP) == SfxItemState::INVALID)
    {
        SAL_WARN("sw.whisper", "FN_WHISPER_START_STOP not handled, adding manually");
        GetWhisperState(rSet);
    }
    if (rSet.GetItemState(FN_WHISPER_SETTINGS) == SfxItemState::INVALID)
    {
        SAL_WARN("sw.whisper", "FN_WHISPER_SETTINGS not handled, adding manually");
        GetWhisperState(rSet);
    }
}

void SwTextShell::ExecWhisper(SfxRequest& rReq)
{
    SAL_WARN("sw.whisper", "ExecWhisper called with slot: " << rReq.GetSlot());
    
    SwWrtShell& rSh = GetShell();
    auto& rWhisperMgr = sw::whisper::WhisperManager::getInstance();
    
    switch (rReq.GetSlot())
    {
        case FN_INSERT_WHISPER_TEXT:
        {
            SAL_WARN("sw.whisper", "FN_INSERT_WHISPER_TEXT clicked - current state: " << static_cast<int>(rWhisperMgr.getState()));
            // Toggle recording
            if (rWhisperMgr.getState() == sw::whisper::WhisperState::Idle)
            {
                SAL_WARN("sw.whisper", "Starting recording with text insert callback");
                rWhisperMgr.setTextInsertCallback([&rSh](const OUString& rText) {
                    rSh.Insert(rText);
                });
                rWhisperMgr.startRecording();
            }
            else if (rWhisperMgr.getState() == sw::whisper::WhisperState::Recording)
            {
                SAL_WARN("sw.whisper", "Stopping recording");
                rWhisperMgr.stopRecording();
            }
        }
        break;
        
        case FN_WHISPER_START_STOP:
        {
            SAL_WARN("sw.whisper", "FN_WHISPER_START_STOP clicked - current state: " << static_cast<int>(rWhisperMgr.getState()));
            // Toggle recording without auto-insert
            if (rWhisperMgr.getState() == sw::whisper::WhisperState::Idle)
            {
                SAL_WARN("sw.whisper", "Starting recording with text insert");
                // Set up text insertion callback - capture view pointer
                SwView* pView = &GetView();
                rWhisperMgr.setTextInsertCallback([pView](const OUString& rText) {
                    SAL_WARN("sw.whisper", "Text insert callback called with: " << rText);
                    if (pView) {
                        SwWrtShell& rWrtSh = pView->GetWrtShell();
                        // Insert the transcribed text at cursor position
                        rWrtSh.Insert(rText);
                    }
                });
                rWhisperMgr.startRecording();
            }
            else if (rWhisperMgr.getState() == sw::whisper::WhisperState::Recording)
            {
                SAL_WARN("sw.whisper", "Stopping recording");
                rWhisperMgr.stopRecording();
            }
        }
        break;
        
        case FN_WHISPER_SETTINGS:
        {
            SAL_WARN("sw.whisper", "FN_WHISPER_SETTINGS clicked - opening settings dialog");
            try {
                auto& rWhisperConfig = rWhisperMgr.getConfig();
                sw::whisper::WhisperSettingsDialog aDlg(GetView().GetFrameWeld(), rWhisperConfig);
                aDlg.run();
            } catch (const std::exception& e) {
                SAL_WARN("sw.whisper", "Failed to open settings dialog: " << e.what());
            } catch (...) {
                SAL_WARN("sw.whisper", "Failed to open settings dialog: unknown exception");
            }
        }
        break;
    }
}

void SwTextShell::GetWhisperState(SfxItemSet& rSet)
{
    auto& rWhisperMgr = sw::whisper::WhisperManager::getInstance();
    
    SfxWhichIter aIter(rSet);
    sal_uInt16 nWhich = aIter.FirstWhich();
    
    SAL_WARN("sw.whisper", "GetWhisperState called, isConfigured: " << rWhisperMgr.isConfigured());
    
    // Check if we have a valid shell and document
    SwWrtShell& rSh = GetShell();
    if (rSh.IsSelFrameMode())
    {
        SAL_WARN("sw.whisper", "Disabling whisper - frame mode");
        // Disable all whisper commands when in frame mode
        while (nWhich)
        {
            if (nWhich == FN_INSERT_WHISPER_TEXT || 
                nWhich == FN_WHISPER_START_STOP || 
                nWhich == FN_WHISPER_SETTINGS)
            {
                rSet.DisableItem(nWhich);
            }
            nWhich = aIter.NextWhich();
        }
        return;
    }
    
    while (nWhich)
    {
        switch (nWhich)
        {
            case FN_INSERT_WHISPER_TEXT:
            case FN_WHISPER_START_STOP:
            {
                SAL_WARN("sw.whisper", "Checking whisper command " << nWhich << ", isConfigured: " << rWhisperMgr.isConfigured());
                if (!rWhisperMgr.isConfigured())
                {
                    rSet.DisableItem(nWhich);
                    SAL_WARN("sw.whisper", "Disabled command " << nWhich << " - not configured");
                }
                else
                {
                    // Set the toggle state based on recording state
                    bool bRecording = rWhisperMgr.getState() == sw::whisper::WhisperState::Recording;
                    rSet.Put(SfxBoolItem(nWhich, bRecording));
                    SAL_WARN("sw.whisper", "Enabled command " << nWhich << " - recording: " << bRecording);
                }
            }
            break;
            
            case FN_WHISPER_SETTINGS:
            {
                // Settings is always enabled (unless in frame mode, handled above)
                SAL_WARN("sw.whisper", "Settings menu item should be enabled (command: " << nWhich << ")");
            }
            break;
            
            default:
                SAL_WARN("sw.whisper", "Unknown whisper command in GetWhisperState: " << nWhich);
            break;
        }
        
        nWhich = aIter.NextWhich();
    }
}

// Add global initialization check
static bool g_bWhisperDebugInit = []() {
    SAL_WARN("sw.whisper", "textsh1.cxx loaded - Whisper debugging active");
    return true;
}();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
