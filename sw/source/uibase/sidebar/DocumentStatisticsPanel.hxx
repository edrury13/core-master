/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef INCLUDED_SW_SOURCE_UIBASE_SIDEBAR_DOCUMENTSTATISTICSPANEL_HXX
#define INCLUDED_SW_SOURCE_UIBASE_SIDEBAR_DOCUMENTSTATISTICSPANEL_HXX

#include <sfx2/sidebar/PanelLayout.hxx>
#include <sfx2/sidebar/ControllerItem.hxx>
#include <vcl/timer.hxx>
#include <docstat.hxx>

class SwView;
class SwDocShell;

namespace sw::sidebar {

class DocumentStatisticsPanel
    : public PanelLayout,
      public ::sfx2::sidebar::ControllerItem::ItemUpdateReceiverInterface
{
public:
    static std::unique_ptr<PanelLayout> Create(
        weld::Widget* pParent,
        SfxBindings* pBindings);

    virtual void NotifyItemUpdate(
        const sal_uInt16 nSId,
        const SfxItemState eState,
        const SfxPoolItem* pState) override;

    virtual void GetControlState(
        const sal_uInt16 /*nSId*/,
        boost::property_tree::ptree& /*rState*/) override {}

    DocumentStatisticsPanel(
        weld::Widget* pParent,
        SfxBindings* pBindings);
    virtual ~DocumentStatisticsPanel() override;

private:
    SfxBindings* mpBindings;
    
    std::unique_ptr<weld::Label> mxPageCount;
    std::unique_ptr<weld::Label> mxWordCount;
    std::unique_ptr<weld::Label> mxCharCount;
    std::unique_ptr<weld::Label> mxCharExcludingSpacesCount;
    std::unique_ptr<weld::Label> mxParagraphCount;
    std::unique_ptr<weld::Label> mxTableCount;
    std::unique_ptr<weld::Label> mxImageCount;
    std::unique_ptr<weld::Label> mxObjectCount;
    std::unique_ptr<weld::Label> mxCommentCount;
    std::unique_ptr<weld::Label> mxTimeToRead;
    
    ::sfx2::sidebar::ControllerItem maDocStatController;
    
    Timer maUpdateTimer;
    SwDocStat maLastDocStat;
    
    void Initialize();
    void Update();
    SwDocShell* GetDocShell();
    
    DECL_LINK(UpdateTimerHdl, Timer*, void);
};

// Global function to show the Document Statistics panel
void ShowDocumentStatisticsPanel();

} // end of namespace sw::sidebar

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
