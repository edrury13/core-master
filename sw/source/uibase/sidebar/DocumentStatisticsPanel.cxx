/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "DocumentStatisticsPanel.hxx"
#include <sfx2/dispatch.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/sidebar/Panel.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/sfxsids.hrc>
#include <svl/stritem.hxx>
#include <view.hxx>
#include <wrtsh.hxx>
#include <docsh.hxx>
#include <doc.hxx>
#include <IDocumentStatistics.hxx>
#include <strings.hrc>
#include <cmdid.h>
#include <com/sun/star/frame/XController.hpp>
#include <sfx2/sidebar/SidebarController.hxx>
#include <sfx2/sidebar/TabBar.hxx>

using namespace css;
using namespace css::uno;

namespace sw::sidebar {

std::unique_ptr<PanelLayout> DocumentStatisticsPanel::Create(
    weld::Widget* pParent,
    SfxBindings* pBindings)
{
    if (pParent == nullptr)
        throw css::lang::IllegalArgumentException(u"no parent window given to DocumentStatisticsPanel::Create"_ustr, nullptr, 0);
    if (pBindings == nullptr)
        throw css::lang::IllegalArgumentException(u"no SfxBindings given to DocumentStatisticsPanel::Create"_ustr, nullptr, 0);

    return std::make_unique<DocumentStatisticsPanel>(pParent, pBindings);
}

DocumentStatisticsPanel::DocumentStatisticsPanel(
    weld::Widget* pParent,
    SfxBindings* pBindings)
    : PanelLayout(pParent, u"DocumentStatisticsPanel"_ustr, u"modules/swriter/ui/documentstatisticspanel.ui"_ustr)
    , mpBindings(pBindings)
    , mxPageCount(m_xBuilder->weld_label(u"pages"_ustr))
    , mxWordCount(m_xBuilder->weld_label(u"words"_ustr))
    , mxCharCount(m_xBuilder->weld_label(u"chars"_ustr))
    , mxCharExcludingSpacesCount(m_xBuilder->weld_label(u"chars_no_spaces"_ustr))
    , mxParagraphCount(m_xBuilder->weld_label(u"paragraphs"_ustr))
    , mxTableCount(m_xBuilder->weld_label(u"tables"_ustr))
    , mxImageCount(m_xBuilder->weld_label(u"images"_ustr))
    , mxObjectCount(m_xBuilder->weld_label(u"objects"_ustr))
    , mxCommentCount(m_xBuilder->weld_label(u"comments"_ustr))
    , mxTimeToRead(m_xBuilder->weld_label(u"time_to_read"_ustr))
    , maDocStatController(FN_STAT_WORDCOUNT, *pBindings, *this)
    , maUpdateTimer("sw:DocumentStatisticsPanel maUpdateTimer")
{
    Initialize();
}

DocumentStatisticsPanel::~DocumentStatisticsPanel()
{
    maUpdateTimer.Stop();
    maDocStatController.dispose();
}

void DocumentStatisticsPanel::Initialize()
{
    maUpdateTimer.SetTimeout(500);
    maUpdateTimer.SetInvokeHandler(LINK(this, DocumentStatisticsPanel, UpdateTimerHdl));
    
    Update();
    mpBindings->Invalidate(FN_STAT_WORDCOUNT);
}

SwDocShell* DocumentStatisticsPanel::GetDocShell()
{
    SfxObjectShell* pObjectShell = SfxObjectShell::Current();
    return dynamic_cast<SwDocShell*>(pObjectShell);
}

void DocumentStatisticsPanel::Update()
{
    SwDocShell* pDocShell = GetDocShell();
    if (!pDocShell)
        return;

    SwDoc* pDoc = pDocShell->GetDoc();
    if (!pDoc)
        return;

    const SwDocStat& aDocStat = pDoc->getIDocumentStatistics().GetDocStat();

    if (aDocStat.bModified || 
        aDocStat.nPage != maLastDocStat.nPage ||
        aDocStat.nWord != maLastDocStat.nWord ||
        aDocStat.nChar != maLastDocStat.nChar ||
        aDocStat.nCharExcludingSpaces != maLastDocStat.nCharExcludingSpaces ||
        aDocStat.nPara != maLastDocStat.nPara ||
        aDocStat.nTable != maLastDocStat.nTable ||
        aDocStat.nGrf != maLastDocStat.nGrf ||
        aDocStat.nOLE != maLastDocStat.nOLE ||
        aDocStat.nComments != maLastDocStat.nComments)
    {
        mxPageCount->set_label(OUString::number(aDocStat.nPage));
        mxWordCount->set_label(OUString::number(aDocStat.nWord));
        mxCharCount->set_label(OUString::number(aDocStat.nChar));
        mxCharExcludingSpacesCount->set_label(OUString::number(aDocStat.nCharExcludingSpaces));
        mxParagraphCount->set_label(OUString::number(aDocStat.nPara));
        mxTableCount->set_label(OUString::number(aDocStat.nTable));
        mxImageCount->set_label(OUString::number(aDocStat.nGrf));
        mxObjectCount->set_label(OUString::number(aDocStat.nOLE));
        mxCommentCount->set_label(OUString::number(aDocStat.nComments));
        
        // Calculate reading time (average reading speed is 200 words per minute)
        const sal_uLong nWords = aDocStat.nWord;
        const sal_uLong nMinutes = (nWords + 199) / 200; // Round up
        OUString sReadingTime;
        if (nMinutes == 0)
            sReadingTime = u"< 1 min"_ustr;
        else if (nMinutes == 1)
            sReadingTime = u"1 min"_ustr;
        else
            sReadingTime = OUString::number(nMinutes) + u" min"_ustr;
        mxTimeToRead->set_label(sReadingTime);
        
        maLastDocStat = aDocStat;
    }
}

void DocumentStatisticsPanel::NotifyItemUpdate(
    const sal_uInt16 nSId,
    const SfxItemState /*eState*/,
    const SfxPoolItem* /*pState*/)
{
    if (nSId == FN_STAT_WORDCOUNT)
    {
        maUpdateTimer.Start();
    }
}


IMPL_LINK_NOARG(DocumentStatisticsPanel, UpdateTimerHdl, Timer*, void)
{
    Update();
}

// Make the panel visible when the command is triggered
void ShowDocumentStatisticsPanel()
{
    // For now, just ensure the sidebar is visible
    // The panel will be available in the Properties deck
    if (SfxViewFrame* pViewFrame = SfxViewFrame::Current())
    {
        pViewFrame->ShowChildWindow(SID_SIDEBAR);
        // The user can then navigate to the Document Statistics panel
        // in the Properties deck
    }
}

} // end of namespace sw::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
