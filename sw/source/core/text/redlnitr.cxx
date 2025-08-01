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

#include <string_view>

#include <hintids.hxx>
#include <o3tl/safeint.hxx>
#include <svl/whiter.hxx>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <scriptinfo.hxx>
#include <swmodule.hxx>
#include <redline.hxx>
#include <txatbase.hxx>
#include <docary.hxx>
#include "itratr.hxx"
#include <ndtxt.hxx>
#include <doc.hxx>
#include <IDocumentRedlineAccess.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <IDocumentMarkAccess.hxx>
#include <IMark.hxx>
#include <bookmark.hxx>
#include <rootfrm.hxx>
#include <breakit.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/settings.hxx>
#include <txtfrm.hxx>
#include <ftnfrm.hxx>
#include <vcl/svapp.hxx>
#include "redlnitr.hxx"
#include <extinput.hxx>
#include <fmtpdsc.hxx>
#include <editeng/charhiddenitem.hxx>
#include <editeng/colritem.hxx>
#include <editeng/crossedoutitem.hxx>
#include <editeng/formatbreakitem.hxx>
#include <editeng/udlnitem.hxx>
#include <officecfg/Office/Writer.hxx>

using namespace ::com::sun::star;

namespace {

class HideIterator
{
private:
    IDocumentRedlineAccess const& m_rIDRA;
    IDocumentMarkAccess const& m_rIDMA;
    bool const m_isHideRedlines;
    sw::FieldmarkMode const m_eFieldmarkMode;
    bool const m_isHideParagraphBreaks;
    SwPosition const m_Start;
    /// next redline
    SwRedlineTable::size_type m_RedlineIndex;
    /// next fieldmark
    std::pair<sw::mark::Fieldmark const*, std::optional<SwPosition>> m_Fieldmark;
    std::optional<SwPosition> m_oNextFieldmarkHide;
    /// previous paragraph break - because m_pStartPos/EndPos are non-owning
    std::optional<std::pair<SwPosition, SwPosition>> m_oParagraphBreak;
    /// current start/end pair
    SwPosition const* m_pStartPos;
    SwPosition const* m_pEndPos;
    SwNode const* m_pCurrentRedlineNode;

public:
    SwPosition const* GetStartPos() const { return m_pStartPos; }
    SwPosition const* GetEndPos() const { return m_pEndPos; }

    HideIterator(const SwTextNode & rTextNode,
            bool const isHideRedlines, sw::FieldmarkMode const eMode,
            sw::ParagraphBreakMode const ePBMode)
        : m_rIDRA(rTextNode.getIDocumentRedlineAccess())
        , m_rIDMA(*rTextNode.getIDocumentMarkAccess())
        , m_isHideRedlines(isHideRedlines)
        , m_eFieldmarkMode(eMode)
        , m_isHideParagraphBreaks(ePBMode == sw::ParagraphBreakMode::Hidden)
        , m_Start(rTextNode, 0)
        , m_RedlineIndex(isHideRedlines ? m_rIDRA.GetRedlinePos(rTextNode, RedlineType::Any) : SwRedlineTable::npos)
        , m_pStartPos(nullptr)
        , m_pEndPos(&m_Start)
        , m_pCurrentRedlineNode(&rTextNode)
    {
    }

    // delete redlines and fieldmarks can't overlap, due to sw::CalcBreaks()
    // and no combining of adjacent redlines
    // -> dummy chars are delete-redlined *iff* entire fieldmark is
    // Note: caller is responsible for checking for immediately adjacent hides
    bool Next()
    {
        SwPosition const* pNextRedlineHide(nullptr);
        assert(m_pEndPos);
        if (m_isHideRedlines)
        {
            // GetRedlinePos() returns npos if there is no redline on the
            // node but something else could have merged nodes so search again!
            if (m_RedlineIndex == SwRedlineTable::npos
                && &m_pEndPos->GetNode() != m_pCurrentRedlineNode)
            {
                m_RedlineIndex = m_rIDRA.GetRedlinePos(m_pEndPos->GetNode(), RedlineType::Any);
                m_pCurrentRedlineNode = &m_pEndPos->GetNode();
            }
            // position on current or next redline
            for (; m_RedlineIndex < m_rIDRA.GetRedlineTable().size(); ++m_RedlineIndex)
            {
                SwRangeRedline const*const pRed = m_rIDRA.GetRedlineTable()[m_RedlineIndex];

                if (m_pEndPos->GetNodeIndex() < pRed->Start()->GetNodeIndex())
                    break;

                if (pRed->GetType() != RedlineType::Delete)
                    continue;

                auto [pStart, pEnd] = pRed->StartEnd(); // SwPosition*
                if (*pStart == *pEnd)
                {   // only allowed while moving (either way?)
//                  assert(IDocumentRedlineAccess::IsHideChanges(rIDRA.GetRedlineFlags()));
                    continue;
                }
                if (pStart->GetNode().IsTableNode())
                {
                    assert(pEnd->GetNode() == m_Start.GetNode() && pEnd->GetContentIndex() == 0);
                    continue; // known pathology, ignore it
                }
                if (*m_pEndPos <= *pStart)
                {
                    pNextRedlineHide = pStart;
                    break; // the next one
                }
            }
        }

        // position on current or next fieldmark
        m_oNextFieldmarkHide.reset();
        if (m_eFieldmarkMode != sw::FieldmarkMode::ShowBoth)
        {
            sal_Unicode const magic(m_eFieldmarkMode == sw::FieldmarkMode::ShowResult
                    ? CH_TXT_ATR_FIELDSTART
                    : CH_TXT_ATR_FIELDSEP);
            SwTextNode* pTextNode = m_pEndPos->GetNode().GetTextNode();
            sal_Int32 const nPos = pTextNode ? pTextNode->GetText().indexOf(
                    magic, m_pEndPos->GetContentIndex()) : -1;
            if (nPos != -1)
            {
                m_oNextFieldmarkHide.emplace(*pTextNode, nPos);
                sw::mark::Fieldmark const*const pFieldmark(
                        m_eFieldmarkMode == sw::FieldmarkMode::ShowResult
                            ? m_rIDMA.getFieldmarkAt(*m_oNextFieldmarkHide)
                            : m_rIDMA.getInnerFieldmarkFor(*m_oNextFieldmarkHide));
                assert(pFieldmark);
                m_Fieldmark.first = pFieldmark;
                // for cursor travelling, there should be 2 visible chars;
                // whichever char is hidden, the cursor travelling needs to
                // be adapted in any case to skip in some situation or other;
                // always hide the CH_TXT_ATR_FIELDSEP for now
                if (m_eFieldmarkMode == sw::FieldmarkMode::ShowResult)
                {
                    m_Fieldmark.second.emplace(
                        sw::mark::FindFieldSep(*m_Fieldmark.first));
                    m_Fieldmark.second->AdjustContent(+1);
                    m_oNextFieldmarkHide->AdjustContent(+1); // skip start
                }
                else
                {
                    m_Fieldmark.second.emplace(pFieldmark->GetMarkEnd());
                    m_Fieldmark.second->AdjustContent(-1);
                }
            }
        }

        // == can happen only if redline starts inside field command, and in
        // that case redline will end before field separator
        assert(!pNextRedlineHide || !m_oNextFieldmarkHide
            || *pNextRedlineHide != *m_oNextFieldmarkHide
            || *m_rIDRA.GetRedlineTable()[m_RedlineIndex]->End() < *m_Fieldmark.second);
        if (pNextRedlineHide
            && (!m_oNextFieldmarkHide || *pNextRedlineHide < *m_oNextFieldmarkHide))
        {
            SwRangeRedline const*const pRed(m_rIDRA.GetRedlineTable()[m_RedlineIndex]);
            m_pStartPos = pRed->Start();
            m_pEndPos = pRed->End();
            ++m_RedlineIndex;
            return true;
        }
        else if (m_oNextFieldmarkHide)
        {
            assert(!pNextRedlineHide || *m_oNextFieldmarkHide <= *pNextRedlineHide);
            m_pStartPos = &*m_oNextFieldmarkHide;
            m_pEndPos = &*m_Fieldmark.second;
            return true;
        }
        else
        {
            assert(!pNextRedlineHide && !m_oNextFieldmarkHide);
            auto const hasHiddenItem = [](auto const& rNode) {
                auto const& rpSet(rNode.GetAttr(RES_PARATR_LIST_AUTOFMT).GetStyleHandle());
                return rpSet ? rpSet->Get(RES_CHRATR_HIDDEN).GetValue() : false;
            };
            auto const hasBreakBefore = [](SwTextNode const& rNode) {
                if (rNode.GetAttr(RES_PAGEDESC).GetPageDesc())
                {
                    return true;
                }
                switch (rNode.GetAttr(RES_BREAK).GetBreak())
                {
                    case SvxBreak::ColumnBefore:
                    case SvxBreak::ColumnBoth:
                    case SvxBreak::PageBefore:
                    case SvxBreak::PageBoth:
                        return true;
                    default:
                        break;
                }
                return false;
            };
            auto const hasBreakAfter = [](SwTextNode const& rNode) {
                switch (rNode.GetAttr(RES_BREAK).GetBreak())
                {
                    case SvxBreak::ColumnAfter:
                    case SvxBreak::ColumnBoth:
                    case SvxBreak::PageAfter:
                    case SvxBreak::PageBoth:
                        return true;
                    default:
                        break;
                }
                return false;
            };
            if (m_isHideParagraphBreaks
                && m_pEndPos->GetNode().IsTextNode() // ooo27109-1.sxw
                // only merge if next node is also text node
                && m_pEndPos->GetNodes()[m_pEndPos->GetNodeIndex()+1]->IsTextNode()
                && hasHiddenItem(*m_pEndPos->GetNode().GetTextNode())
                // no merge if there's a page break on any node
                && !hasBreakBefore(*m_pEndPos->GetNodes()[m_pEndPos->GetNodeIndex()+1]->GetTextNode())
                // first node, see SwTextFrame::GetBreak()
                && !hasBreakAfter(*m_Start.GetNode().GetTextNode()))
            {
                m_oParagraphBreak.emplace(
                    SwPosition(*m_pEndPos->GetNode().GetTextNode(), m_pEndPos->GetNode().GetTextNode()->Len()),
                    SwPosition(*m_pEndPos->GetNodes()[m_pEndPos->GetNodeIndex()+1]->GetTextNode(), 0));
                m_pStartPos = &m_oParagraphBreak->first;
                m_pEndPos = &m_oParagraphBreak->second;
                return true;
            }
            else // nothing
            {
                m_pStartPos = nullptr;
                m_pEndPos = nullptr;
                return false;
            }
        }
    }
};

}

namespace sw {

void FindParaPropsNodeIgnoreHidden(sw::MergedPara & rMerged,
        sw::ParagraphBreakMode const eMode, SwScriptInfo * pScriptInfo)
{
    if (eMode == sw::ParagraphBreakMode::Hidden)
    {
        ::std::optional<SwScriptInfo> oScriptInfo;
        if (pScriptInfo == nullptr)
        {
            oScriptInfo.emplace();
            pScriptInfo = &*oScriptInfo;
        }
        // always init: when called from SwTextFrame::SwClientNotify() it is stale!
        pScriptInfo->InitScriptInfoHidden(*rMerged.pFirstNode, &rMerged);
        TextFrameIndex nHiddenStart{COMPLETE_STRING};
        TextFrameIndex nHiddenEnd{0};
        pScriptInfo->GetBoundsOfHiddenRange(TextFrameIndex{0}, nHiddenStart, nHiddenEnd);
        if (TextFrameIndex{0} == nHiddenStart)
        {
            if (nHiddenEnd == TextFrameIndex{rMerged.mergedText.getLength()})
            {
                rMerged.pParaPropsNode = const_cast<SwTextNode*>(rMerged.pLastNode);
            }
            else
            {   // this requires MapViewToModel to never return a position at
                // the end of a node (when all its text is hidden)
                rMerged.pParaPropsNode = sw::MapViewToModel(rMerged, nHiddenEnd).first;
            }
            return;
        }
    }
    if (!rMerged.extents.empty())
    {   // para props from first node that isn't empty (OOo/LO compat)
        rMerged.pParaPropsNode = rMerged.extents.begin()->pNode;
    }
    else
    {   // if every node is empty, the last one wins (Word compat)
        // (OOo/LO historically used first one)
        rMerged.pParaPropsNode = const_cast<SwTextNode*>(rMerged.pLastNode);
    }
}

std::unique_ptr<sw::MergedPara>
CheckParaRedlineMerge(SwTextFrame & rFrame, SwTextNode & rTextNode,
       FrameMode const eMode)
{
    if (!rFrame.getRootFrame()->HasMergedParas())
    {
        return nullptr;
    }
    bool bHaveRedlines(false);
    std::vector<SwTextNode *> nodes{ &rTextNode };
    std::vector<SwTableNode *> tables;
    std::vector<SwSectionNode *> sections;
    std::vector<sw::Extent> extents;
    OUStringBuffer mergedText;
    SwTextNode * pNode(&rTextNode);
    sal_Int32 nLastEnd(0);
    for (auto iter = HideIterator(rTextNode,
                rFrame.getRootFrame()->IsHideRedlines(),
                rFrame.getRootFrame()->GetFieldmarkMode(),
                rFrame.getRootFrame()->GetParagraphBreakMode());
            iter.Next(); )
    {
        SwPosition const*const pStart(iter.GetStartPos());
        SwPosition const*const pEnd(iter.GetEndPos());
        bHaveRedlines = true;
        assert(pNode != &rTextNode || &pStart->GetNode() == &rTextNode); // detect calls with wrong start node
        if (pStart->GetContentIndex() != nLastEnd) // not 0 so we eliminate adjacent deletes
        {
            extents.emplace_back(pNode, nLastEnd, pStart->GetContentIndex());
            mergedText.append(pNode->GetText().subView(nLastEnd, pStart->GetContentIndex() - nLastEnd));
        }
        if (&pEnd->GetNode() != pNode)
        {
            if (pNode == &rTextNode)
            {
                pNode->SetRedlineMergeFlag(SwNode::Merge::First);
            } // else: was already set before
            int nLevel(0);
            for (SwNodeOffset j = pNode->GetIndex() + 1; j < pEnd->GetNodeIndex(); ++j)
            {
                SwNode *const pTmp(pNode->GetNodes()[j]);
                if (nLevel == 0)
                {
                    if (pTmp->IsTextNode())
                    {
                        nodes.push_back(pTmp->GetTextNode());
                    }
                    else if (pTmp->IsTableNode())
                    {
                        tables.push_back(pTmp->GetTableNode());
                    }
                    else if (pTmp->IsSectionNode())
                    {
                        sections.push_back(pTmp->GetSectionNode());
                    }
                }
                if (pTmp->IsStartNode())
                {
                    ++nLevel;
                }
                else if (pTmp->IsEndNode())
                {
                    --nLevel;
                }
                pTmp->SetRedlineMergeFlag(SwNode::Merge::Hidden);
            }
            // note: in DelLastPara() case, the end node is not actually merged
            // and is likely a SwTableNode!
            if (!pEnd->GetNode().IsTextNode())
            {
                assert(pEnd->GetNode() != pStart->GetNode());
                // must set pNode too because it will mark the last node
                pNode = nodes.back();
                assert(pNode == pNode->GetNodes()[pEnd->GetNodeIndex() - 1]);
                if (pNode != &rTextNode)
                {   // something might depend on last merged one being NonFirst?
                    pNode->SetRedlineMergeFlag(SwNode::Merge::NonFirst);
                }
                nLastEnd = pNode->Len();
            }
            else
            {
                pNode = pEnd->GetNode().GetTextNode();
                nodes.push_back(pNode);
                pNode->SetRedlineMergeFlag(SwNode::Merge::NonFirst);
                nLastEnd = pEnd->GetContentIndex();
            }
        }
        else
        {
            nLastEnd = pEnd->GetContentIndex();
        }
    }
    if (pNode == &rTextNode)
    {
        if (rTextNode.GetRedlineMergeFlag() != SwNode::Merge::None)
        {
            rTextNode.SetRedlineMergeFlag(SwNode::Merge::None);
        }
    }
    // Reset flag of the following text node since we know it's not merged;
    // also any table/sections in between.
    // * the following SwTextNode is in same nodes section as pNode (nLevel=0)
    // * the start nodes that don't have a SwTextNode before them
    //   on their level, and their corresponding end nodes
    // * the first SwTextNode inside each start node of the previous point
    // Other (non-first) SwTextNodes in nested sections shouldn't be reset!
    int nLevel(0);
    for (SwNodeOffset j = pNode->GetIndex() + 1; j < pNode->GetNodes().Count(); ++j)
    {
        SwNode *const pTmp(pNode->GetNodes()[j]);
        if (!pTmp->IsCreateFrameWhenHidingRedlines())
        {   // clear stale flag caused by editing with redlines shown
            pTmp->SetRedlineMergeFlag(SwNode::Merge::None);
        }
        if (pTmp->IsStartNode())
        {
            ++nLevel;
        }
        else if (pTmp->IsEndNode())
        {
            if (nLevel == 0)
            {
                break; // there is no following text node; avoid leaving section
            }
            --nLevel;
        }
        else if (pTmp->IsTextNode())
        {
            if (nLevel == 0)
            {
                break; // done
            }
            else
            {   // skip everything other than 1st text node in section!
                j = pTmp->EndOfSectionIndex() - 1; // will be incremented again
            }
        }
    }
    if (!bHaveRedlines)
    {
        if (rTextNode.IsInList() && !rTextNode.GetNum(rFrame.getRootFrame()))
        {
            rTextNode.AddToListRLHidden(); // try to add it...
        }
        return nullptr;
    }
    if (nLastEnd != pNode->Len())
    {
        extents.emplace_back(pNode, nLastEnd, pNode->Len());
        mergedText.append(pNode->GetText().subView(nLastEnd, pNode->Len() - nLastEnd));
    }
    if (extents.empty()) // there was no text anywhere
    {
        assert(mergedText.isEmpty());
    }
    else
    {
        assert(!mergedText.isEmpty());
    }
    auto pRet{std::make_unique<sw::MergedPara>(rFrame, std::move(extents),
                mergedText.makeStringAndClear(), &rTextNode, nodes.back())};
    FindParaPropsNodeIgnoreHidden(*pRet, rFrame.getRootFrame()->GetParagraphBreakMode(), nullptr);
    assert(pRet->pParaPropsNode);
    // keep lists up to date with visible nodes
    if (pRet->pParaPropsNode->IsInList() && !pRet->pParaPropsNode->GetNum(rFrame.getRootFrame()))
    {
        pRet->pParaPropsNode->AddToListRLHidden(); // try to add it...
    }
    for (auto const pTextNode : nodes)
    {
        if (pTextNode != pRet->pParaPropsNode)
        {
            pTextNode->RemoveFromListRLHidden();
        }
    }
    if (eMode == FrameMode::Existing)
    {
        // remove existing footnote frames for first node;
        // for non-first nodes with own frames, DelFrames will remove all
        // (could possibly call lcl_ChangeFootnoteRef, not sure if worth it)
        // note: must be done *before* changing listeners!
        // for non-first nodes that are already merged with this frame,
        // need to remove here too, otherwise footnotes can be removed only
        // by lucky accident, e.g. TruncLines().
        auto itExtent(pRet->extents.begin());
        for (auto const pTextNode : nodes)
        {
            sal_Int32 nLast(0);
            std::vector<std::pair<sal_Int32, sal_Int32>> hidden;
            for ( ; itExtent != pRet->extents.end(); ++itExtent)
            {
                if (itExtent->pNode != pTextNode)
                {
                    break;
                }
                if (itExtent->nStart != 0)
                {
                    assert(itExtent->nStart != nLast);
                    hidden.emplace_back(nLast, itExtent->nStart);
                }
                nLast = itExtent->nEnd;
            }
            if (nLast != pTextNode->Len())
            {
                hidden.emplace_back(nLast, pTextNode->Len());
            }
            sw::RemoveFootnotesForNode(*rFrame.getRootFrame(), *pTextNode, &hidden);
        }
        // unfortunately DelFrames() must be done before StartListening too,
        // otherwise footnotes cannot be deleted by SwTextFootnote::DelFrames!
        auto const end(--nodes.rend());
        for (auto iter = nodes.rbegin(); iter != end; ++iter)
        {
            (**iter).DelFrames(rFrame.getRootFrame());
        }
        // also delete tables & sections here; not necessary, but convenient
        for (auto const pTableNode : tables)
        {
            pTableNode->DelFrames(rFrame.getRootFrame());
        }
        for (auto const pSectionNode : sections)
        {
            pSectionNode->GetSection().GetFormat()->DelFrames(/*rFrame.getRootFrame()*/);
        }
    }
    for (SwTextNode * pTmp : nodes)
    {
        pRet->listener.StartListening(pTmp);
    }
    rFrame.EndListeningAll();
    return pRet;
}

} // namespace sw

void SwAttrIter::InitFontAndAttrHandler(
        SwTextNode const& rPropsNode,
        SwTextNode const& rTextNode,
        std::u16string_view aText,
        bool const*const pbVertLayout,
        bool const*const pbVertLayoutLRBT)
{
    // Build a font matching the default paragraph style:
    SwFontAccess aFontAccess( &rPropsNode.GetAnyFormatColl(), m_pViewShell );
    // It is possible that Init is called more than once, e.g., in a
    // SwTextFrame::FormatOnceMore situation or (since sw_redlinehide)
    // from SwAttrIter::Seek(); in the latter case SwTextSizeInfo::m_pFnt
    // is an alias of m_pFont so it must not be deleted!
    if (m_pFont)
    {
        *m_pFont = aFontAccess.Get()->GetFont();
    }
    else
    {
        m_pFont = new SwFont( aFontAccess.Get()->GetFont() );
    }

    // set font to vertical if frame layout is vertical
    // if it's a re-init, the vert flag never changes
    bool bVertLayoutLRBT = false;
    if (pbVertLayoutLRBT)
        bVertLayoutLRBT = *pbVertLayoutLRBT;
    if (pbVertLayout ? *pbVertLayout : m_aAttrHandler.IsVertLayout())
    {
        m_pFont->SetVertical(m_pFont->GetOrientation(), true, bVertLayoutLRBT);
    }

    // Initialize the default attribute of the attribute handler
    // based on the attribute array cached together with the font.
    // If any further attributes for the paragraph are given in pAttrSet
    // consider them during construction of the default array, and apply
    // them to the font
    m_aAttrHandler.Init(aFontAccess.Get()->GetDefault(), rTextNode.GetpSwAttrSet(),
           *rTextNode.getIDocumentSettingAccess(), m_pViewShell, *m_pFont,
           pbVertLayout ? *pbVertLayout : m_aAttrHandler.IsVertLayout(),
           bVertLayoutLRBT );

    m_aFontCacheIds[SwFontScript::Latin] = m_aFontCacheIds[SwFontScript::CJK] = m_aFontCacheIds[SwFontScript::CTL] = nullptr;

    assert(g_pBreakIt && g_pBreakIt->GetBreakIter().is());

    m_pFont->SetActual( m_pScriptInfo->WhichFont(TextFrameIndex(0)) );

    TextFrameIndex nChg(0);
    size_t nCnt = 0;

    do
    {
        if ( nCnt >= m_pScriptInfo->CountScriptChg() )
            break;
        nChg = m_pScriptInfo->GetScriptChg( nCnt );
        SwFontScript nTmp = SW_SCRIPTS;
        switch ( m_pScriptInfo->GetScriptType( nCnt++ ) ) {
            case i18n::ScriptType::ASIAN :
                if( !m_aFontCacheIds[SwFontScript::CJK] ) nTmp = SwFontScript::CJK;
                break;
            case i18n::ScriptType::COMPLEX :
                if( !m_aFontCacheIds[SwFontScript::CTL] ) nTmp = SwFontScript::CTL;
                break;
            default:
                if( !m_aFontCacheIds[SwFontScript::Latin ] ) nTmp = SwFontScript::Latin;
        }
        if( nTmp < SW_SCRIPTS )
        {
            m_pFont->CheckFontCacheId( m_pViewShell, nTmp );
            m_pFont->GetFontCacheId( m_aFontCacheIds[ nTmp ], m_aFontIdx[ nTmp ], nTmp );
        }
    }
    while (nChg < TextFrameIndex(aText.size()));
}

void SwAttrIter::CtorInitAttrIter(SwTextNode & rTextNode,
        SwScriptInfo & rScriptInfo, SwTextFrame const*const pFrame)
{
    // during HTML-Import it can happen, that no layout exists
    SwRootFrame* pRootFrame = rTextNode.getIDocumentLayoutAccess().GetCurrentLayout();
    m_pViewShell = pRootFrame ? pRootFrame->GetCurrShell() : nullptr;

    m_pScriptInfo = &rScriptInfo;

    // set font to vertical if frame layout is vertical
    bool bVertLayout = false;
    bool bVertLayoutLRBT = false;
    bool bRTL = false;
    if ( pFrame )
    {
        if ( pFrame->IsVertical() )
        {
            bVertLayout = true;
        }
        if (pFrame->IsVertLRBT())
        {
            bVertLayoutLRBT = true;
        }
        bRTL = pFrame->IsRightToLeft();
        m_pMergedPara = pFrame->GetMergedPara();
    }

    // determine script changes if not already done for current paragraph
    assert(m_pScriptInfo);
    if (m_pScriptInfo->GetInvalidityA() != TextFrameIndex(COMPLETE_STRING))
         m_pScriptInfo->InitScriptInfo(rTextNode, m_pMergedPara, bRTL);

    InitFontAndAttrHandler(
            m_pMergedPara ? *m_pMergedPara->pParaPropsNode : rTextNode,
            rTextNode,
            m_pMergedPara ? m_pMergedPara->mergedText : rTextNode.GetText(),
            & bVertLayout,
            & bVertLayoutLRBT);

    m_nStartIndex = m_nEndIndex = m_nPosition = m_nChgCnt = 0;
    m_nPropFont = 0;
    SwDoc& rDoc = rTextNode.GetDoc();
    const IDocumentRedlineAccess& rIDRA = rTextNode.getIDocumentRedlineAccess();

    // sw_redlinehide: this is a Ring - pExtInp is the first PaM that's inside
    // the node.  It's not clear whether there can be more than 1 PaM in the
    // Ring, and this code doesn't handle that case; neither did the old code.
    const SwExtTextInput* pExtInp = rDoc.GetExtTextInput( rTextNode );
    if (!pExtInp && m_pMergedPara)
    {
        SwTextNode const* pNode(&rTextNode);
        for (auto const& rExtent : m_pMergedPara->extents)
        {
            if (rExtent.pNode != pNode)
            {
                pNode = rExtent.pNode;
                pExtInp = rDoc.GetExtTextInput(*pNode);
                if (pExtInp)
                    break;
            }
        }
    }
    const bool bShow = IDocumentRedlineAccess::IsShowChanges(rIDRA.GetRedlineFlags())
        && pRootFrame && !pRootFrame->IsHideRedlines();
    if (!(pExtInp || m_pMergedPara || bShow))
        return;

    SwRedlineTable::size_type nRedlPos = rIDRA.GetRedlinePos( rTextNode, RedlineType::Any );
    if (SwRedlineTable::npos == nRedlPos && m_pMergedPara)
    {
        SwTextNode const* pNode(&rTextNode);
        for (auto const& rExtent : m_pMergedPara->extents)
        {   // note: have to search because extents based only on Delete
            if (rExtent.pNode != pNode)
            {
                pNode = rExtent.pNode;
                nRedlPos = rIDRA.GetRedlinePos(*pNode, RedlineType::Any);
                if (SwRedlineTable::npos != nRedlPos)
                    break;
            }
        }
        // TODO this is true initially but after delete ops it may be false... need to delete m_pMerged somewhere?
        // assert(SwRedlineTable::npos != nRedlPos);
        // false now with fieldmarks
        assert(!pRootFrame
            || pRootFrame->GetFieldmarkMode() != sw::FieldmarkMode::ShowBoth
            || SwRedlineTable::npos != nRedlPos || m_pMergedPara->extents.size() <= 1);
    }
    if (!(pExtInp || m_pMergedPara || SwRedlineTable::npos != nRedlPos))
        return;

    const std::vector<ExtTextInputAttr> *pArr = nullptr;
    if( pExtInp )
    {
        pArr = &pExtInp->GetAttrs();
        Seek( TextFrameIndex(0) );
    }

    m_pRedline.reset(new SwRedlineItr( rTextNode, *m_pFont, m_aAttrHandler, nRedlPos,
                    (pRootFrame && pRootFrame->IsHideRedlines())
                        ? SwRedlineItr::Mode::Hide
                        : bShow
                            ? SwRedlineItr::Mode::Show
                            : SwRedlineItr::Mode::Ignore,
                    pArr, pExtInp ? pExtInp->Start() : nullptr));

    if( m_pRedline->IsOn() )
        ++m_nChgCnt;
}

// The Redline-Iterator
// The following information/states exist in RedlineIterator:
//
// m_nFirst is the first index of RedlineTable, which overlaps with the paragraph.
//
// m_nAct is the currently active (if m_bOn is set) or the next possible index.
// m_nStart and m_nEnd give you the borders of the object within the paragraph.
//
// If m_bOn is set, the font has been manipulated according to it.
//
// If m_nAct is set to SwRedlineTable::npos (via Reset()), then currently no
// Redline is active, m_nStart and m_nEnd are invalid.
SwRedlineItr::SwRedlineItr( const SwTextNode& rTextNd, SwFont& rFnt,
                            SwAttrHandler& rAH, sal_Int32 nRed,
                            Mode const mode,
                            const std::vector<ExtTextInputAttr> *pArr,
                            SwPosition const*const pExtInputStart)
    : m_rDoc( rTextNd.GetDoc() )
    , m_rAttrHandler( rAH )
    , m_nNdIdx( rTextNd.GetIndex() )
    , m_nFirst( nRed )
    , m_nAct( SwRedlineTable::npos )
    , m_nStart( COMPLETE_STRING )
    , m_nEnd( COMPLETE_STRING )
    , m_bOn( false )
    , m_eMode( mode )
{
    if( pArr )
    {
        assert(pExtInputStart);
        m_pExt.reset( new SwExtend(*pArr, pExtInputStart->GetNodeIndex(),
                                     pExtInputStart->GetContentIndex()) );
    }
    else
        m_pExt = nullptr;
    assert(m_pExt || m_eMode != Mode::Ignore); // only create if necessary
    Seek(rFnt, m_nNdIdx, 0, COMPLETE_STRING);
}

SwRedlineItr::~SwRedlineItr() COVERITY_NOEXCEPT_FALSE
{
    Clear( nullptr );
    m_pExt.reset();
}

/// The return value of SwRedlineItr::Seek tells if the current font
/// has been manipulated by leaving (-1) or entering (+1) a range redline
short SwRedlineItr::Seek(SwFont& rFnt,
        SwNodeOffset const nNode, sal_Int32 const nNew, sal_Int32 const nOld)
{
    short nRet = 0;
    if( ExtOn() )
        return 0; // shortcut: if we're within an ExtendTextInputs
                  // there can't be other changes of attributes (not even by redlining)
    if (m_eMode == Mode::Show)
    {
        if (m_bOn)
        {
            if (nNew >= m_nEnd)
            {
                --nRet;
                Clear_( &rFnt );    // We go behind the current range
//                ++m_nAct; // don't increment, could be in next range too if overlap
            }
            else if (nNew < m_nStart)
            {
                --nRet;
                Clear_( &rFnt );    // We go before the current range
                if (m_nAct > m_nFirst)
                    m_nAct = m_nFirst;  // need to start over
                else
                    return nRet + EnterExtend(rFnt, nNode, nNew); // There's none prior to us
            }
            else
                return nRet + EnterExtend(rFnt, nNode, nNew); // We stayed in the same range
        }
        if (SwRedlineTable::npos == m_nAct || nOld > nNew)
            m_nAct = m_nFirst;

        m_nStart = COMPLETE_STRING;
        m_nEnd = COMPLETE_STRING;
        const SwRedlineTable& rTable = m_rDoc.getIDocumentRedlineAccess().GetRedlineTable();
        ::std::optional<decltype(m_nAct)> oFirstMatch;

        for ( ; m_nAct < rTable.size() ; ++m_nAct)
        {
            decltype(m_nStart) nStart;
            decltype(m_nEnd) nEnd;
            if (rTable[m_nAct]->CalcStartEnd(nNode, nStart, nEnd))
            { // previous redline intersected nNode but this one precedes it
                continue;
            }

            // redline table is sorted, but here it's not the complete redlines
            assert(m_nStart == COMPLETE_STRING || m_nStart <= nStart);
            assert(m_nStart == COMPLETE_STRING || m_nStart <= nEnd);
            if (oFirstMatch && nNew < nStart)
            {
                m_nEnd = std::min(m_nEnd, nStart);
                break;
            }
            if (nNew < nEnd)
            {
                m_nStart = nStart;
                m_nEnd = std::min(m_nEnd, nEnd);
                if (nStart <= nNew) // there can be a format and another redline...
                {
                    if (!oFirstMatch)
                    {
                        oFirstMatch.emplace(m_nAct);
                    }
                    const SwRangeRedline *pRed = rTable[ m_nAct ];

                    if (m_pSet)
                        m_pSet->ClearItem();
                    else
                    {
                        SwAttrPool& rPool =
                            const_cast<SwDoc&>(m_rDoc).GetAttrPool();
                        m_pSet = std::make_unique<SfxItemSetFixed<RES_CHRATR_BEGIN, RES_CHRATR_END-1>>(rPool);
                    }

                    if( 1 < pRed->GetStackCount() )
                        FillHints( pRed->GetAuthor( 1 ), pRed->GetType( 1 ) );
                    FillHints( pRed->GetAuthor(), pRed->GetType() );

                    SfxWhichIter aIter( *m_pSet );

                    // moved text: dark green with double underline or strikethrough
                    bool bDisplayMovedTextInGreen = officecfg::Office::Writer::Comparison::DisplayMovedTextInGreen::get();
                    if ( bDisplayMovedTextInGreen && pRed->IsMoved() )
                    {
                        m_pSet->Put(SvxColorItem( COL_GREEN, RES_CHRATR_COLOR ));
                        if (SfxItemState::SET == m_pSet->GetItemState(RES_CHRATR_CROSSEDOUT, true))
                            m_pSet->Put(SvxCrossedOutItem( STRIKEOUT_DOUBLE, RES_CHRATR_CROSSEDOUT ));
                        else
                            m_pSet->Put(SvxUnderlineItem( LINESTYLE_DOUBLE, RES_CHRATR_UNDERLINE ));
                    }

                    sal_uInt16 nWhich = aIter.FirstWhich();
                    while( nWhich )
                    {
                        const SfxPoolItem* pItem;
                        if( ( nWhich < RES_CHRATR_END ) &&
                            ( SfxItemState::SET == aIter.GetItemState( true, &pItem ) ) )
                        {
                            SwTextAttr* pAttr = MakeRedlineTextAttr(
                                const_cast<SwDoc&>(m_rDoc),
                                *const_cast<SfxPoolItem*>(pItem) );
                            pAttr->SetPriorityAttr( true );
                            m_Hints.push_back(pAttr);
                            m_rAttrHandler.PushAndChg( *pAttr, rFnt );
                        }
                        nWhich = aIter.NextWhich();
                    }
                }
                else
                {
                    break;
                }
            }
        }

        if (oFirstMatch)
        {
            m_bOn = true;
            m_nAct = *oFirstMatch; // rewind
            ++nRet; // increment only once per m_nStart/m_nEnd range
        }
    }
    else if (m_eMode == Mode::Hide)
    {   // ... just iterate to update m_nAct for GetNextRedln();
        // there is no need to care about formatting in this mode
        if (m_nAct == SwRedlineTable::npos || nOld == COMPLETE_STRING)
        {   // reset, or move backward
            m_nAct = m_nFirst;
        }
        for ( ; m_nAct < m_rDoc.getIDocumentRedlineAccess().GetRedlineTable().size(); ++m_nAct)
        {   // only Start matters in this mode
            // Seeks until it finds a RL that starts at or behind the seek pos.
            // - then update m_nStart/m_nEnd to the intersection of it with the
            // current node (if any).
            // The only way to skip to a different node is if there is a Delete
            // RL, so if there is no intersection we'll never skip again.
            // Note: here, assume that delete can't nest inside delete!
            SwRangeRedline const*const pRedline(
                m_rDoc.getIDocumentRedlineAccess().GetRedlineTable()[m_nAct]);
            SwPosition const*const pStart(pRedline->Start());
            if (pRedline->GetType() == RedlineType::Delete
                && (nNode < pStart->GetNodeIndex()
                    || (nNode == pStart->GetNodeIndex()
                        && nNew <= pStart->GetContentIndex())))
            {
                pRedline->CalcStartEnd(nNode, m_nStart, m_nEnd);
                break;
            }
            m_nStart = COMPLETE_STRING;
            m_nEnd = COMPLETE_STRING;
        }
    }
    return nRet + EnterExtend(rFnt, nNode, nNew);
}

void SwRedlineItr::FillHints( std::size_t nAuthor, RedlineType eType )
{
    switch ( eType )
    {
        case RedlineType::Insert:
            SwModule::get()->GetInsertAuthorAttr(nAuthor, *m_pSet);
            break;
        case RedlineType::Delete:
            SwModule::get()->GetDeletedAuthorAttr(nAuthor, *m_pSet);
            break;
        case RedlineType::Format:
        case RedlineType::FmtColl:
        case RedlineType::ParagraphFormat:
            SwModule::get()->GetFormatAuthorAttr(nAuthor, *m_pSet);
            break;
        default:
            break;
    }
}

void SwRedlineItr::ChangeTextAttr( SwFont* pFnt, SwTextAttr const &rHt, bool bChg )
{
    OSL_ENSURE( IsOn(), "SwRedlineItr::ChangeTextAttr: Off?" );

    if (m_eMode != Mode::Show && !m_pExt)
        return;

    if( bChg )
    {
        if (m_pExt && m_pExt->IsOn())
            m_rAttrHandler.PushAndChg( rHt, *m_pExt->GetFont() );
        else
            m_rAttrHandler.PushAndChg( rHt, *pFnt );
    }
    else
    {
        OSL_ENSURE( ! m_pExt || ! m_pExt->IsOn(), "Pop of attribute during opened extension" );
        m_rAttrHandler.PopAndChg( rHt, *pFnt );
    }
}

void SwRedlineItr::Clear_( SwFont* pFnt )
{
    OSL_ENSURE( m_bOn, "SwRedlineItr::Clear: Off?" );
    m_bOn = false;
    for (auto const& hint : m_Hints)
    {
        if( pFnt )
            m_rAttrHandler.PopAndChg( *hint, *pFnt );
        else
            m_rAttrHandler.Pop( *hint );
        SwTextAttr::Destroy(hint);
    }
    m_Hints.clear();
}

/// Ignore mode: does nothing.
/// Show mode: returns end of redline if currently in one, or start of next
/// Hide mode: returns start of next redline in current node, plus (if it's a
///            Delete) its end position and number of consecutive RLs
std::pair<sal_Int32, std::pair<SwRangeRedline const*, size_t>>
SwRedlineItr::GetNextRedln(sal_Int32 nNext, SwTextNode const*const pNode,
        SwRedlineTable::size_type & rAct)
{
    sal_Int32 nStart(m_nStart);
    sal_Int32 nEnd(m_nEnd);
    nNext = NextExtend(pNode->GetIndex(), nNext);
    if (m_eMode == Mode::Ignore || SwRedlineTable::npos == m_nFirst)
        return std::make_pair(nNext, std::make_pair(nullptr, 0));
    if (SwRedlineTable::npos == rAct)
    {
        rAct = m_nFirst;
    }
    if (rAct != m_nAct)
    {
        while (rAct < m_rDoc.getIDocumentRedlineAccess().GetRedlineTable().size())
        {
            SwRangeRedline const*const pRedline(
                    m_rDoc.getIDocumentRedlineAccess().GetRedlineTable()[rAct]);
            pRedline->CalcStartEnd(pNode->GetIndex(), nStart, nEnd);
            if (m_eMode != Mode::Hide
                || pRedline->GetType() == RedlineType::Delete)
            {
                break;
            }
            ++rAct; // Hide mode: search a Delete RL
        }
    }
    if (rAct == m_rDoc.getIDocumentRedlineAccess().GetRedlineTable().size())
    {
        return std::make_pair(nNext, std::make_pair(nullptr, 0)); // no Delete here
    }
    if (m_bOn || (m_eMode == Mode::Show && nStart == 0))
    {   // in Ignore mode, the end of redlines isn't relevant, except as returned in the second in the pair!
        if (nEnd < nNext)
            nNext = nEnd;
    }
    else if (nStart <= nNext)
    {
        if (m_eMode == Mode::Show)
        {
            nNext = nStart;
        }
        else
        {
            assert(m_eMode == Mode::Hide);
            SwRangeRedline const* pRedline(
                    m_rDoc.getIDocumentRedlineAccess().GetRedlineTable()[rAct]);
            assert(pRedline->GetType() == RedlineType::Delete); //?
            if (pRedline->GetType() == RedlineType::Delete)
            {
                nNext = nStart;
                size_t nSkipped(1); // (consecutive) candidates to be skipped
                while (rAct + nSkipped <
                       m_rDoc.getIDocumentRedlineAccess().GetRedlineTable().size())
                {
                    SwRangeRedline const*const pNext =
                        m_rDoc.getIDocumentRedlineAccess().GetRedlineTable()[rAct + nSkipped];
                    if (*pRedline->End() < *pNext->Start())
                    {
                        break; // done for now
                    }
                    else if (*pNext->Start() == *pRedline->End() &&
                            pNext->GetType() == RedlineType::Delete)
                    {
                        // consecutive delete - continue
                        pRedline = pNext;
                    }
                    ++nSkipped;
                }
                return std::make_pair(nNext, std::make_pair(pRedline, nSkipped));
            }
        }
    }
    return std::make_pair(nNext, std::make_pair(nullptr, 0));
}

bool SwRedlineItr::ChkSpecialUnderline_() const
{
    // If the underlining or the escapement is caused by redlining,
    // we always apply the SpecialUnderlining, i.e. the underlining
    // below the base line
    for (SwTextAttr* pHint : m_Hints)
    {
        const sal_uInt16 nWhich = pHint->Which();
        if( RES_CHRATR_UNDERLINE == nWhich ||
            RES_CHRATR_ESCAPEMENT == nWhich )
            return true;
    }
    return false;
}

bool SwRedlineItr::CheckLine(
        SwNodeOffset const nStartNode, sal_Int32 const nChkStart,
        SwNodeOffset const nEndNode, sal_Int32 nChkEnd, OUString& rRedlineText,
        bool& bRedlineEnd, RedlineType& eRedlineEnd, size_t* pAuthorAtPos)
{
    // note: previously this would return true in the (!m_bShow && m_pExt)
    // case, but surely that was a bug?
    if (m_nFirst == SwRedlineTable::npos || m_eMode != Mode::Show)
        return false;
    if( nChkEnd == nChkStart && pAuthorAtPos == nullptr ) // empty lines look one char further
        ++nChkEnd;
    sal_Int32 nOldStart = m_nStart;
    sal_Int32 nOldEnd = m_nEnd;
    SwRedlineTable::size_type const nOldAct = m_nAct;
    bool bRet = bRedlineEnd = false;
    eRedlineEnd = RedlineType::None;

    SwPosition const start(*m_rDoc.GetNodes()[nStartNode]->GetContentNode(), nChkStart);
    SwPosition const end(*m_rDoc.GetNodes()[nEndNode]->GetContentNode(), nChkEnd);
    SwRangeRedline const* pPrevRedline = nullptr;
    bool isBreak(false);
    for (m_nAct = m_nFirst; m_nAct < m_rDoc.getIDocumentRedlineAccess().GetRedlineTable().size(); ++m_nAct)
    {
        SwRangeRedline const*const pRedline(
            m_rDoc.getIDocumentRedlineAccess().GetRedlineTable()[ m_nAct ] );
        // collect text of the hidden redlines at the end of the line
        bool isExtendText(false);
        switch (ComparePosition(*pRedline->Start(), *pRedline->End(), start, end))
        {
            case SwComparePosition::Behind:
                isBreak = true;
                break;
            case SwComparePosition::OverlapBehind:
            case SwComparePosition::CollideStart:
            case SwComparePosition::Outside:
            case SwComparePosition::Equal:
                // store redlining at line end (for line break formatting)
                eRedlineEnd = pRedline->GetType();
                bRedlineEnd = true;
                isBreak = true;
                if (pAuthorAtPos)
                    *pAuthorAtPos = pRedline->GetAuthor();
                [[fallthrough]];
            case SwComparePosition::OverlapBefore:
            case SwComparePosition::CollideEnd:
            case SwComparePosition::Inside:
            {
                bRet = true;
                // start to collect text of invisible redlines for ChangesInMargin layout
                if (rRedlineText.isEmpty() && !pRedline->IsVisible())
                {
                    rRedlineText = pRedline->GetDescr(/*bSimplified=*/true);
                    pPrevRedline = pRedline;
                    isExtendText = true;
                }
                // join the text of the next invisible redlines in the same position
                // i.e. characters deleted by pressing backspace or delete
                else if (pPrevRedline && !pRedline->IsVisible() &&
                    *pRedline->Start() == *pPrevRedline->Start() && *pRedline->End() == *pPrevRedline->End() )
                {
                    OUString sExtendText(pRedline->GetDescr(/*bSimplified=*/true));
                    if (!sExtendText.isEmpty())
                    {
                        if (rRedlineText.getLength() < 12)
                        {
                            // TODO: remove extra space from GetDescr(true),
                            // but show deletion of paragraph or line break
                            rRedlineText = rRedlineText +
                                    pRedline->GetDescr(/*bSimplified=*/true).subView(1);
                        }
                        else
                            rRedlineText = OUString::Concat(rRedlineText.subView(0, rRedlineText.getLength() - 3)) + "...";
                    }
                    isExtendText = true;
                }
                break;
            }
            case SwComparePosition::Before:
                break; // -Werror=switch
        }
        if (isBreak && !isExtendText)
        {
            break;
        }
    }

    m_nStart = nOldStart;
    m_nEnd = nOldEnd;
    m_nAct = nOldAct;
    return bRet;
}

void SwExtend::ActualizeFont( SwFont &rFnt, ExtTextInputAttr nAttr )
{
    if ( nAttr & ExtTextInputAttr::Underline )
        rFnt.SetUnderline( LINESTYLE_SINGLE );
    else if ( nAttr & ExtTextInputAttr::DoubleUnderline )
        rFnt.SetUnderline( LINESTYLE_DOUBLE );
    else if ( nAttr & ExtTextInputAttr::BoldUnderline )
        rFnt.SetUnderline( LINESTYLE_BOLD );
    else if ( nAttr & ExtTextInputAttr::DottedUnderline )
        rFnt.SetUnderline( LINESTYLE_DOTTED );
    else if ( nAttr & ExtTextInputAttr::DashDotUnderline )
        rFnt.SetUnderline( LINESTYLE_DOTTED );

    if ( nAttr & ExtTextInputAttr::RedText )
        rFnt.SetColor( COL_RED );

    if ( nAttr & ExtTextInputAttr::Highlight )
    {
        const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();
        rFnt.SetColor( rStyleSettings.GetHighlightTextColor() );
        rFnt.SetBackColor( rStyleSettings.GetHighlightColor() );
    }
    if ( nAttr & ExtTextInputAttr::GrayWaveline )
        rFnt.SetGreyWave( true );
}

short SwExtend::Enter(SwFont& rFnt, SwNodeOffset const nNode, sal_Int32 const nNew)
{
    OSL_ENSURE( !m_pFont, "SwExtend: Enter with Font" );
    if (nNode != m_nNode)
        return 0;
    OSL_ENSURE( !Inside(), "SwExtend: Enter without Leave" );
    m_nPos = nNew;
    if( Inside() )
    {
        m_pFont.reset( new SwFont(rFnt) );
        ActualizeFont( rFnt, m_rArr[m_nPos - m_nStart] );
        return 1;
    }
    return 0;
}

bool SwExtend::Leave_(SwFont& rFnt, SwNodeOffset const nNode, sal_Int32 const nNew)
{
    OSL_ENSURE(nNode == m_nNode && Inside(), "SwExtend: Leave without Enter");
    if (nNode != m_nNode)
        return true;
    const ExtTextInputAttr nOldAttr = m_rArr[m_nPos - m_nStart];
    m_nPos = nNew;
    if( Inside() )
    {   // We stayed within the ExtendText-section
        const ExtTextInputAttr nAttr = m_rArr[m_nPos - m_nStart];
        if( nOldAttr != nAttr ) // Is there an (inner) change of attributes?
        {
            rFnt = *m_pFont;
            ActualizeFont( rFnt, nAttr );
        }
    }
    else
    {
        rFnt = *m_pFont;
        m_pFont.reset();
        return true;
    }
    return false;
}

sal_Int32 SwExtend::Next(SwNodeOffset const nNode, sal_Int32 nNext)
{
    if (nNode != m_nNode)
        return nNext;
    if (m_nPos < m_nStart)
    {
        if (nNext > m_nStart)
            nNext = m_nStart;
    }
    else if (m_nPos < m_nEnd)
    {
        sal_Int32 nIdx = m_nPos - m_nStart;
        const ExtTextInputAttr nAttr = m_rArr[ nIdx ];
        while (o3tl::make_unsigned(++nIdx) < m_rArr.size() && nAttr == m_rArr[nIdx])
            ; //nothing
        nIdx = nIdx + m_nStart;
        if( nNext > nIdx )
            nNext = nIdx;
    }
    return nNext;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
