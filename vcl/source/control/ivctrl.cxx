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


#include <utility>
#include <vcl/toolkit/ivctrl.hxx>
#include "imivctl.hxx"
#include <vcl/bitmapex.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/mnemonic.hxx>
#include <vcl/settings.hxx>
#include <vcl/tabctrl.hxx>
#include <vcl/vclevent.hxx>
#include <vcl/uitest/uiobject.hxx>
#include <vcl/uitest/logger.hxx>
#include <vcl/uitest/eventdescription.hxx>
#include <accessibility/accessibleiconchoicectrl.hxx>
#include <verticaltabctrl.hxx>

using namespace ::com::sun::star::accessibility;

namespace
{
void collectUIInformation( const OUString& aID, const OUString& aPos)
{
    EventDescription aDescription;
    aDescription.aID = aID;
    aDescription.aParameters = {{ "POS" ,  aPos}};
    aDescription.aAction = "SELECT";
    aDescription.aKeyWord = "VerticalTab";
    UITestLogger::getInstance().logEvent(aDescription);
}
}

SvxIconChoiceCtrlEntry::SvxIconChoiceCtrlEntry( OUString _aText,
                                                Image _aImage )
    : aImage(std::move(_aImage))
    , aText(std::move(_aText))
    , nX(0)
    , nY(0)
    , nFlags(SvxIconViewFlags::NONE)
{
}

OUString SvxIconChoiceCtrlEntry::GetDisplayText() const
{
    return MnemonicGenerator::EraseAllMnemonicChars( aText );
}

SvtIconChoiceCtrl::SvtIconChoiceCtrl( vcl::Window* pParent, WinBits nWinStyle ) :

     // WB_CLIPCHILDREN on, as ScrollBars lie on the window!
    Control( pParent, nWinStyle | WB_CLIPCHILDREN ),

    _pImpl           ( new SvxIconChoiceCtrl_Impl( this, nWinStyle ) ),
    m_nWidth(-1)
{
    GetOutDev()->SetLineColor();
    _pImpl->InitSettings();
}

SvtIconChoiceCtrl::~SvtIconChoiceCtrl()
{
    disposeOnce();
}

void SvtIconChoiceCtrl::dispose()
{
    if (_pImpl)
    {
        _pImpl->CallEventListeners( VclEventId::ObjectDying, nullptr );
        _pImpl.reset();
    }
    Control::dispose();
}

SvxIconChoiceCtrlEntry* SvtIconChoiceCtrl::InsertEntry( const OUString& rText, const Image& rImage  )
{
    SvxIconChoiceCtrlEntry* pEntry = new SvxIconChoiceCtrlEntry( rText, rImage);

    _pImpl->InsertEntry(std::unique_ptr<SvxIconChoiceCtrlEntry>(pEntry), _pImpl->GetEntryCount());

    return pEntry;
}

void SvtIconChoiceCtrl::RemoveEntry(sal_Int32 nIndex)
{
    _pImpl->RemoveEntry(nIndex);
}

void SvtIconChoiceCtrl::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect)
{
    _pImpl->Paint(rRenderContext, rRect);
}

void SvtIconChoiceCtrl::MouseButtonDown( const MouseEvent& rMEvt )
{
    if( !_pImpl->MouseButtonDown( rMEvt ) )
        Control::MouseButtonDown( rMEvt );
}

void SvtIconChoiceCtrl::MouseMove( const MouseEvent& rMEvt )
{
    if( !_pImpl->MouseMove( rMEvt ) )
        Control::MouseMove( rMEvt );
}
void SvtIconChoiceCtrl::ArrangeIcons()
{
    Size aFullSize;
    tools::Rectangle aEntryRect;

    for ( sal_Int32 i = 0; i < GetEntryCount(); i++ )
    {
        SvxIconChoiceCtrlEntry* pEntry = GetEntry ( i );
        aEntryRect = _pImpl->GetEntryBoundRect ( pEntry );

        aFullSize.setHeight ( aFullSize.getHeight()+aEntryRect.GetHeight() );
    }

    _pImpl->Arrange(aFullSize.getHeight());

    _pImpl->Arrange(1000);
}

long SvtIconChoiceCtrl::AdjustWidth(const long nWidth)
{
    const long cMargin = 9;

    if (nWidth + cMargin > m_nWidth)
    {
        m_nWidth = nWidth + cMargin;
        this->set_width_request(m_nWidth);
        _pImpl->SetGrid(Size(m_nWidth, 32));
    }
    return m_nWidth - cMargin;
}

void SvtIconChoiceCtrl::Resize()
{
    _pImpl->Resize();
    Control::Resize();
}

void SvtIconChoiceCtrl::GetFocus()
{
    _pImpl->GetFocus();
    Control::GetFocus();
}

void SvtIconChoiceCtrl::LoseFocus()
{
    if (_pImpl)
        _pImpl->LoseFocus();
    Control::LoseFocus();
}

void SvtIconChoiceCtrl::SetFont(const vcl::Font& rFont)
{
    if (rFont != GetFont())
    {
        Control::SetFont(rFont);
        _pImpl->FontModified();
    }
}

void SvtIconChoiceCtrl::SetPointFont(const vcl::Font& rFont)
{
    if (rFont != GetPointFont(*GetOutDev())) //FIXME
    {
        Control::SetPointFont(*GetOutDev(), rFont); //FIXME
        _pImpl->FontModified();
    }
}

void SvtIconChoiceCtrl::Command(const CommandEvent& rCEvt)
{
    _pImpl->Command( rCEvt );
    //pass at least alt press/release to parent impl
    if (rCEvt.GetCommand() == CommandEventId::ModKeyChange)
        Control::Command(rCEvt);
}

sal_Int32 SvtIconChoiceCtrl::GetEntryCount() const
{
    return _pImpl ? _pImpl->GetEntryCount() : 0;
}

SvxIconChoiceCtrlEntry* SvtIconChoiceCtrl::GetEntry( sal_Int32 nPos ) const
{
    return _pImpl ? _pImpl->GetEntry( nPos ) : nullptr;
}

SvxIconChoiceCtrlEntry* SvtIconChoiceCtrl::GetSelectedEntry() const
{
    return _pImpl ? _pImpl->GetFirstSelectedEntry() : nullptr;
}

void SvtIconChoiceCtrl::ClickIcon()
{
    GetSelectedEntry();
    _aClickIconHdl.Call( this );
}

void SvtIconChoiceCtrl::KeyInput( const KeyEvent& rKEvt )
{
    bool bKeyUsed = DoKeyInput( rKEvt );
    if ( !bKeyUsed )
    {
        Control::KeyInput( rKEvt );
    }
}
bool SvtIconChoiceCtrl::DoKeyInput( const KeyEvent& rKEvt )
{
    return _pImpl->KeyInput( rKEvt );
}
sal_Int32 SvtIconChoiceCtrl::GetEntryListPos( SvxIconChoiceCtrlEntry const * pEntry ) const
{
    return _pImpl->GetEntryListPos( pEntry );
}
SvxIconChoiceCtrlEntry* SvtIconChoiceCtrl::GetCursor( ) const
{
    return _pImpl->GetCurEntry( );
}
void SvtIconChoiceCtrl::SetCursor( SvxIconChoiceCtrlEntry* pEntry )
{
    _pImpl->SetCursor( pEntry );
}

void SvtIconChoiceCtrl::DataChanged( const DataChangedEvent& rDCEvt )
{
    if ( ((rDCEvt.GetType() == DataChangedEventType::SETTINGS) ||
         (rDCEvt.GetType() == DataChangedEventType::FONTSUBSTITUTION) ||
         (rDCEvt.GetType() == DataChangedEventType::FONTS) ) &&
         (rDCEvt.GetFlags() & AllSettingsFlags::STYLE) )
    {
        _pImpl->InitSettings();
        Invalidate(InvalidateFlags::NoChildren);
    }
    else
        Control::DataChanged( rDCEvt );
}

void SvtIconChoiceCtrl::SetBackground( const Wallpaper& rPaper )
{
    if( rPaper == GetBackground() )
        return;

    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
    // if it is the default (empty) wallpaper
    if (rPaper.IsEmpty())
    {
        Control::SetBackground( rStyleSettings.GetFieldColor() );
    }
    else
    {
        Wallpaper aBackground( rPaper );
        // HACK, as background might be transparent!
        if( !aBackground.IsBitmap() )
            aBackground.SetStyle( WallpaperStyle::Tile );

        WallpaperStyle eStyle = aBackground.GetStyle();
        Color aBack( aBackground.GetColor());
        if( aBack == COL_TRANSPARENT &&
            (!aBackground.IsBitmap() ||
             aBackground.GetBitmap().IsAlpha() ||
             (eStyle != WallpaperStyle::Tile && eStyle != WallpaperStyle::Scale)) )
        {
            aBackground.SetColor( rStyleSettings.GetFieldColor() );
        }
        if( aBackground.IsScrollable() )
        {
            tools::Rectangle aRect;
            aRect.SetSize( Size(32765, 32765) );
            aBackground.SetRect( aRect );
        }
        else
        {
            tools::Rectangle aRect( _pImpl->GetOutputRect() );
            aBackground.SetRect( aRect );
        }
        Control::SetBackground( aBackground );
    }

    // If text colors are attributed "hard," don't use automatism to select
    // a readable text color.
    vcl::Font aFont( GetFont() );
    aFont.SetColor( rStyleSettings.GetFieldTextColor() );
    SetFont( aFont );

    Invalidate(InvalidateFlags::NoChildren);
}

void SvtIconChoiceCtrl::RequestHelp( const HelpEvent& rHEvt )
{
    if ( !_pImpl->RequestHelp( rHEvt ) )
        Control::RequestHelp( rHEvt );
}

tools::Rectangle SvtIconChoiceCtrl::GetBoundingBox( SvxIconChoiceCtrlEntry* pEntry ) const
{
    return _pImpl->GetEntryBoundRect( pEntry );
}

void SvtIconChoiceCtrl::FillLayoutData() const
{
    CreateLayoutData();
    const_cast<SvtIconChoiceCtrl*>(this)->Invalidate();
}

tools::Rectangle SvtIconChoiceCtrl::GetEntryCharacterBounds( const sal_Int32 _nEntryPos, const sal_Int32 _nCharacterIndex ) const
{
    tools::Rectangle aRect;

    Pair aEntryCharacterRange = GetLineStartEnd( _nEntryPos );
    if ( aEntryCharacterRange.A() + _nCharacterIndex < aEntryCharacterRange.B() )
    {
        aRect = GetCharacterBounds( aEntryCharacterRange.A() + _nCharacterIndex );
    }

    return aRect;
}

void SvtIconChoiceCtrl::CallImplEventListeners(VclEventId nEvent, void* pData)
{
    CallEventListeners(nEvent, pData);
}
css::uno::Reference< XAccessible > SvtIconChoiceCtrl::CreateAccessible()
{
    return new AccessibleIconChoiceCtrl(*this);
}

struct VerticalTabPageData
{
    OUString sId;
    SvxIconChoiceCtrlEntry* pEntry;
    VclPtr<vcl::Window> xPage;      ///< the TabPage itself
};

VerticalTabControl::VerticalTabControl(vcl::Window* pParent, bool bWithIcons)
    : VclHBox(pParent)
    , m_xChooser(VclPtr<SvtIconChoiceCtrl>::Create(this, WB_3DLOOK | (bWithIcons ?  WB_ICON : WB_SMALLICON) |
#ifdef MACOSX
                                                         WB_NOBORDER |
#else
                                                         WB_BORDER |
#endif
                                                         WB_NOCOLUMNHEADER |
                                                         WB_NODRAGSELECTION | WB_TABSTOP | WB_CLIPCHILDREN |
                                                         WB_NOHSCROLL))
    , m_xBox(VclPtr<VclVBox>::Create(this))
{
    SetStyle(GetStyle() | WB_DIALOGCONTROL);
    SetType(WindowType::VERTICALTABCONTROL);
    m_xChooser->SetClickHdl(LINK(this, VerticalTabControl, ChosePageHdl_Impl));
    m_xChooser->set_width_request(150);
    m_xChooser->set_height_request(400);
    m_xChooser->SetSizePixel(Size(150, 400));
    m_xBox->set_vexpand(true);
    m_xBox->set_hexpand(true);
    m_xBox->set_expand(true);
    m_xBox->Show();
    m_xChooser->Show();
}

VerticalTabControl::~VerticalTabControl()
{
    disposeOnce();
}

void VerticalTabControl::dispose()
{
    m_xChooser.disposeAndClear();
    m_xBox.disposeAndClear();
    VclHBox::dispose();
}

IMPL_LINK_NOARG(VerticalTabControl, ChosePageHdl_Impl, SvtIconChoiceCtrl*, void)
{
    SvxIconChoiceCtrlEntry *pEntry = m_xChooser->GetSelectedEntry();
    if (!pEntry)
        pEntry = m_xChooser->GetCursor();

    VerticalTabPageData* pData = GetPageData(pEntry);

    if (pData->sId != m_sCurrentPageId)
        SetCurPageId(pData->sId);
}

bool VerticalTabControl::EventNotify(NotifyEvent& rNEvt)
{
    if (rNEvt.GetType() == NotifyEventType::KEYINPUT)
    {
        sal_uInt16 nCode = rNEvt.GetKeyEvent()->GetKeyCode().GetCode();
        if (nCode == KEY_PAGEUP || nCode == KEY_PAGEDOWN)
        {
            m_xChooser->DoKeyInput(*(rNEvt.GetKeyEvent()));
            return true;
        }
    }
    return VclHBox::EventNotify(rNEvt);
}

void VerticalTabControl::ActivatePage()
{
    m_aActivateHdl.Call( this );
}

bool VerticalTabControl::DeactivatePage()
{
    return !m_aDeactivateHdl.IsSet() || m_aDeactivateHdl.Call(this);
}

VerticalTabPageData* VerticalTabControl::GetPageData(const SvxIconChoiceCtrlEntry* pEntry) const
{
    VerticalTabPageData* pRet = nullptr;
    for (auto & pData : maPageList)
    {
        if (pData->pEntry == pEntry)
        {
            pRet = pData.get();
            break;
        }
    }
    return pRet;
}

VerticalTabPageData* VerticalTabControl::GetPageData(std::u16string_view rId) const
{
    VerticalTabPageData* pRet = nullptr;
    for (auto & pData : maPageList)
    {
        if (pData->sId == rId)
        {
            pRet = pData.get();
            break;
        }
    }
    return pRet;
}

void VerticalTabControl::SetCurPageId(const OUString& rId)
{
    OUString sOldPageId = GetCurPageId();
    if (sOldPageId == rId)
        return;

    VerticalTabPageData* pOldData = GetPageData(sOldPageId);
    if (pOldData && pOldData->xPage)
    {
        if (!DeactivatePage())
            return;
        pOldData->xPage->Hide();
    }

    m_sCurrentPageId = "";

    VerticalTabPageData* pNewData = GetPageData(rId);
    if (pNewData && pNewData->xPage)
    {
        m_sCurrentPageId = rId;
        m_xChooser->SetCursor(pNewData->pEntry);

        ActivatePage();
        pNewData->xPage->Show();
    }
    collectUIInformation(get_id(),m_sCurrentPageId);
}

const OUString & VerticalTabControl::GetPageId(sal_uInt16 nIndex) const
{
    return maPageList[nIndex]->sId;
}

void VerticalTabControl::InsertPage(const rtl::OUString &rIdent, const rtl::OUString& rLabel, const Image& rImage,
                                    const rtl::OUString& rTooltip, VclPtr<vcl::Window> xPage, int nPos)
{
    SvxIconChoiceCtrlEntry* pEntry = m_xChooser->InsertEntry(rLabel, rImage);
    pEntry->SetQuickHelpText(rTooltip);
    m_xChooser->ArrangeIcons();
    VerticalTabPageData* pNew;
    if (nPos == -1)
    {
        maPageList.emplace_back(new VerticalTabPageData);
        pNew = maPageList.back().get();
    }
    else
    {
        maPageList.emplace(maPageList.begin() + nPos, new VerticalTabPageData);
        pNew = maPageList[nPos].get();
    }
    pNew->sId = rIdent;
    pNew->pEntry = pEntry;
    pNew->xPage = xPage;
    Size aOrigPrefSize(m_xBox->get_preferred_size());
    Size aPagePrefSize(xPage->get_preferred_size());
    m_xBox->set_width_request(std::max(aOrigPrefSize.Width(), aPagePrefSize.Width()));
    m_xBox->set_height_request(std::max(aOrigPrefSize.Height(), aPagePrefSize.Height()));
    pNew->xPage->Hide();
}

void VerticalTabControl::RemovePage(std::u16string_view rPageId)
{
    for (auto it = maPageList.begin(), end = maPageList.end(); it != end; ++it)
    {
        VerticalTabPageData* pData = it->get();
        if (pData->sId == rPageId)
        {
            sal_Int32 nEntryListPos = m_xChooser->GetEntryListPos(pData->pEntry);
            assert(nEntryListPos >= 0);
            m_xChooser->RemoveEntry(nEntryListPos);
            m_xChooser->ArrangeIcons();
            maPageList.erase(it);
            break;
        }
    }
}

sal_uInt16 VerticalTabControl::GetPagePos(std::u16string_view rPageId) const
{
    VerticalTabPageData* pData = GetPageData(rPageId);
    if (!pData)
        return TAB_PAGE_NOTFOUND;
    return m_xChooser->GetEntryListPos(pData->pEntry);
}

VclPtr<vcl::Window> VerticalTabControl::GetPage(std::u16string_view rPageId) const
{
    VerticalTabPageData* pData = GetPageData(rPageId);
    if (!pData)
        return nullptr;
    return pData->xPage;
}

OUString VerticalTabControl::GetPageText(std::u16string_view rPageId) const
{
    VerticalTabPageData* pData = GetPageData(rPageId);
    if (!pData)
        return OUString();
    return pData->pEntry->GetText();
}

void VerticalTabControl::SetPageText(std::u16string_view rPageId, const OUString& rText)
{
    VerticalTabPageData* pData = GetPageData(rPageId);
    if (!pData)
        return;
    pData->pEntry->SetText(rText);
}

Size VerticalTabControl::GetOptimalSize() const
{
    // re-calculate size - we might have replaced dummy tab pages with
    // actual content
    Size aOptimalPageSize(m_xBox->get_preferred_size());

    for (auto const& item : maPageList)
    {
        Size aPagePrefSize(item->xPage->get_preferred_size());
        if (aPagePrefSize.Width() > aOptimalPageSize.Width())
            aOptimalPageSize.setWidth( aPagePrefSize.Width() );
        if (aPagePrefSize.Height() > aOptimalPageSize.Height())
            aOptimalPageSize.setHeight( aPagePrefSize.Height() );
    }

    Size aChooserSize(m_xChooser->get_preferred_size());
    return Size(aChooserSize.Width() + aOptimalPageSize.Width(),
                std::max(aChooserSize.Height(), aOptimalPageSize.Height()));
}

void VerticalTabControl::DumpAsPropertyTree(tools::JsonWriter& rJsonWriter)
{
    rJsonWriter.put("id", get_id());
    rJsonWriter.put("type", "tabcontrol");
    rJsonWriter.put("vertical", true);
    rJsonWriter.put("selected", GetCurPageId());

    {
        auto childrenNode = rJsonWriter.startArray("children");
        for (int i = 0; i < GetPageCount(); i++)
        {
            VclPtr<vcl::Window> pChild = GetPage(GetPageId(i));

            if (pChild)
            {
                if (!pChild->GetChildCount())
                    continue;

                auto aChildNode = rJsonWriter.startStruct();
                pChild->DumpAsPropertyTree(rJsonWriter);
            }
        }
    }
    {
        auto tabsNode = rJsonWriter.startArray("tabs");
        for(int i = 0; i < GetPageCount(); i++)
        {
            VclPtr<vcl::Window> pChild = GetPage(GetPageId(i));

            if (pChild)
            {
                if (!pChild->GetChildCount())
                    continue;

                auto aTabNode = rJsonWriter.startStruct();
                auto sId = GetPageId(i);
                rJsonWriter.put("text", GetPageText(sId));
                rJsonWriter.put("id", sId);
                rJsonWriter.put("name", GetPageText(sId));
            }
        }
    }
}

FactoryFunction VerticalTabControl::GetUITestFactory() const
{
    return VerticalTabControlUIObject::create;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
