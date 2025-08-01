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

#include <view/SlsInsertionIndicatorOverlay.hxx>

#include <SlideSorter.hxx>
#include <view/SlideSorterView.hxx>
#include <view/SlsLayouter.hxx>
#include <view/SlsPageObjectLayouter.hxx>
#include <view/SlsTheme.hxx>
#include "SlsFramePainter.hxx"
#include "SlsLayeredDevice.hxx"
#include <DrawDocShell.hxx>
#include <drawdoc.hxx>
#include <Window.hxx>

#include <o3tl/safeint.hxx>
#include <rtl/math.hxx>
#include <vcl/virdev.hxx>
#include <basegfx/range/b2drectangle.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>

namespace {

const double gnPreviewOffsetScale = 1.0 / 8.0;

::tools::Rectangle GrowRectangle (const ::tools::Rectangle& rBox, const sal_Int32 nOffset)
{
    return ::tools::Rectangle (
        rBox.Left() - nOffset,
        rBox.Top() - nOffset,
        rBox.Right() + nOffset,
        rBox.Bottom() + nOffset);
}

sal_Int32 RoundToInt (const double nValue) { return sal_Int32(::rtl::math::round(nValue)); }

} // end of anonymous namespace

namespace sd::slidesorter::view {

//=====  InsertionIndicatorOverlay  ===========================================

const sal_Int32 gnShadowBorder = 3;
const sal_Int32 gnLayerIndex = 2;

InsertionIndicatorOverlay::InsertionIndicatorOverlay (SlideSorter& rSlideSorter)
    : mrSlideSorter(rSlideSorter),
      mbIsVisible(false),
      mpShadowPainter(
          new FramePainter(mrSlideSorter.GetTheme()->GetIcon(Theme::Icon_RawInsertShadow)))
{
}

InsertionIndicatorOverlay::~InsertionIndicatorOverlay()
{
    // cid#1491947 silence Uncaught exception
    suppress_fun_call_w_exception(Hide());
}

void InsertionIndicatorOverlay::Create (const SdTransferable* pTransferable)
{
    if (pTransferable == nullptr)
        return;

    std::shared_ptr<controller::TransferableData> pData (
        controller::TransferableData::GetFromTransferable(pTransferable));
    if ( ! pData)
        return;
    sal_Int32 nSelectionCount (0);
    if (pTransferable->HasPageBookmarks())
        nSelectionCount = pTransferable->GetPageBookmarks().size();
    else
    {
        DrawDocShell* pDataDocShell = dynamic_cast<DrawDocShell*>(pTransferable->GetDocShell().get());
        if (pDataDocShell != nullptr)
        {
            SdDrawDocument* pDataDocument = pDataDocShell->GetDoc();
            if (pDataDocument != nullptr)
                nSelectionCount = pDataDocument->GetSdPageCount(PageKind::Standard);
        }
    }
    Create(pData->GetRepresentatives(), nSelectionCount);
}

void InsertionIndicatorOverlay::Create (
    const ::std::vector<controller::TransferableData::Representative>& rRepresentatives,
    const sal_Int32 nSelectionCount)
{
    view::Layouter& rLayouter (mrSlideSorter.GetView().GetLayouter());
    view::PageObjectLayouter* pPageObjectLayouter (
        rLayouter.GetPageObjectLayouter());
    std::shared_ptr<view::Theme> pTheme (mrSlideSorter.GetTheme());
    const Size aOriginalPreviewSize (pPageObjectLayouter->GetPreviewSize());

    const double nPreviewScale (0.5);
    const Size aPreviewSize (
        RoundToInt(aOriginalPreviewSize.Width()*nPreviewScale),
        RoundToInt(aOriginalPreviewSize.Height()*nPreviewScale));
    const sal_Int32 nOffset (
        RoundToInt(std::min(aPreviewSize.Width(),aPreviewSize.Height()) * gnPreviewOffsetScale));

    // Determine size and offset depending on the number of previews.
    sal_Int32 nCount (rRepresentatives.size());
    if (nCount > 0)
        --nCount;
    Size aIconSize(
        aPreviewSize.Width() + 2 * gnShadowBorder + nCount*nOffset,
        aPreviewSize.Height() + 2 * gnShadowBorder + nCount*nOffset);

    // Create virtual devices for bitmap and mask whose bitmaps later be
    // combined to form the BitmapEx of the icon.
    ScopedVclPtrInstance<VirtualDevice> pContent(
        *mrSlideSorter.GetContentWindow()->GetOutDev(), DeviceFormat::WITH_ALPHA);
    pContent->SetOutputSizePixel(aIconSize);

    pContent->SetFillColor();
    pContent->SetLineColor(pTheme->GetColor(Theme::Color_PreviewBorder));
    const Point aOffset = PaintRepresentatives(*pContent, aPreviewSize, nOffset, rRepresentatives);

    PaintPageCount(*pContent, nSelectionCount, aPreviewSize, aOffset);

    maIcon = pContent->GetBitmapEx(Point(0,0), aIconSize);
    maIcon.Scale(aIconSize);
}

Point InsertionIndicatorOverlay::PaintRepresentatives (
    OutputDevice& rContent,
    const Size& rPreviewSize,
    const sal_Int32 nOffset,
    const ::std::vector<controller::TransferableData::Representative>& rRepresentatives) const
{
    const Point aOffset (0,rRepresentatives.size()==1 ? -nOffset : 0);

    // Paint the pages.
    Point aPageOffset (0,0);
    double nTransparency (0);
    const BitmapEx aExclusionOverlay (mrSlideSorter.GetTheme()->GetIcon(Theme::Icon_HideSlideOverlay));
    for (sal_Int32 nIndex=2; nIndex>=0; --nIndex)
    {
        if (rRepresentatives.size() <= o3tl::make_unsigned(nIndex))
            continue;
        switch(nIndex)
        {
            case 0 :
                aPageOffset = Point(0, nOffset);
                nTransparency = 0.85;
                break;
            case 1:
                aPageOffset = Point(nOffset, 0);
                nTransparency = 0.75;
                break;
            case 2:
                aPageOffset = Point(2*nOffset, 2*nOffset);
                nTransparency = 0.65;
                break;
        }
        aPageOffset += aOffset;
        aPageOffset.AdjustX(gnShadowBorder );
        aPageOffset.AdjustY(gnShadowBorder );

        // Paint the preview.
        BitmapEx aPreview (rRepresentatives[nIndex].maBitmap);
        aPreview.Scale(rPreviewSize, BmpScaleFlag::BestQuality);
        rContent.DrawBitmapEx(aPageOffset, aPreview);

        // When the page is marked as excluded from the slide show then
        // paint an overlay that visualizes this.
        if (rRepresentatives[nIndex].mbIsExcluded)
        {
            const vcl::Region aSavedClipRegion (rContent.GetClipRegion());
            rContent.IntersectClipRegion(::tools::Rectangle(aPageOffset, rPreviewSize));
            // Paint bitmap tiled over the preview to mark it as excluded.
            const sal_Int32 nIconWidth (aExclusionOverlay.GetSizePixel().Width());
            const sal_Int32 nIconHeight (aExclusionOverlay.GetSizePixel().Height());
            if (nIconWidth>0 && nIconHeight>0)
            {
                for (::tools::Long nX=0; nX<rPreviewSize.Width(); nX+=nIconWidth)
                    for (::tools::Long nY=0; nY<rPreviewSize.Height(); nY+=nIconHeight)
                        rContent.DrawBitmapEx(Point(nX,nY)+aPageOffset, aExclusionOverlay);
            }
            rContent.SetClipRegion(aSavedClipRegion);
        }

        // Tone down the bitmap.  The further back the darker it becomes.
        ::tools::Rectangle aBox (
            aPageOffset.X(),
            aPageOffset.Y(),
            aPageOffset.X()+rPreviewSize.Width()-1,
            aPageOffset.Y()+rPreviewSize.Height()-1);
        rContent.SetFillColor(COL_BLACK);
        rContent.SetLineColor();
        rContent.DrawTransparent(
            basegfx::B2DHomMatrix(),
            ::basegfx::B2DPolyPolygon(::basegfx::utils::createPolygonFromRect(
                ::basegfx::B2DRectangle(aBox.Left(), aBox.Top(), aBox.Right()+1, aBox.Bottom()+1),
                0,
                0)),
            nTransparency);

        // Draw border around preview.
        ::tools::Rectangle aBorderBox (GrowRectangle(aBox, 1));
        rContent.SetLineColor(COL_GRAY);
        rContent.SetFillColor();
        rContent.DrawRect(aBorderBox);

        // Draw shadow around preview.
        mpShadowPainter->PaintFrame(rContent, aBorderBox);
    }

    return aPageOffset;
}

void InsertionIndicatorOverlay::PaintPageCount (
    OutputDevice& rDevice,
    const sal_Int32 nSelectionCount,
    const Size& rPreviewSize,
    const Point& rFirstPageOffset) const
{
    // Paint the number of slides.
    std::shared_ptr<view::Theme> pTheme (mrSlideSorter.GetTheme());
    std::shared_ptr<vcl::Font> pFont(Theme::GetFont(Theme::Font_PageCount, rDevice));
    if (!pFont)
        return;

    OUString sNumber (OUString::number(nSelectionCount));

    // Determine the size of the (painted) text and create a bounding
    // box that centers the text on the first preview.
    rDevice.SetFont(*pFont);
    ::tools::Rectangle aTextBox;
    rDevice.GetTextBoundRect(aTextBox, sNumber);
    Point aTextOffset (aTextBox.TopLeft());
    Size aTextSize (aTextBox.GetSize());
    // Place text inside the first page preview.
    Point aTextLocation(rFirstPageOffset);
    // Center the text.
    aTextLocation += Point(
        (rPreviewSize.Width()-aTextBox.GetWidth())/2,
        (rPreviewSize.Height()-aTextBox.GetHeight())/2);
    aTextBox = ::tools::Rectangle(aTextLocation, aTextSize);

    // Paint background, border and text.
    static const sal_Int32 nBorder = 5;
    rDevice.SetFillColor(pTheme->GetColor(Theme::Color_Selection));
    rDevice.SetLineColor(pTheme->GetColor(Theme::Color_Selection));
    rDevice.DrawRect(GrowRectangle(aTextBox, nBorder));

    rDevice.SetFillColor();
    rDevice.SetLineColor(pTheme->GetColor(Theme::Color_PageCountFontColor));
    rDevice.DrawRect(GrowRectangle(aTextBox, nBorder-1));

    rDevice.SetTextColor(pTheme->GetColor(Theme::Color_PageCountFontColor));
    rDevice.DrawText(aTextBox.TopLeft()-aTextOffset, sNumber);
}

void InsertionIndicatorOverlay::SetLocation (const Point& rLocation)
{
    const Point  aTopLeft (
        rLocation - Point(
            maIcon.GetSizePixel().Width()/2,
            maIcon.GetSizePixel().Height()/2));
    if (maLocation != aTopLeft)
    {
        const ::tools::Rectangle aOldBoundingBox (GetBoundingBox());

        maLocation = aTopLeft;

        if (mpLayerInvalidator && IsVisible())
        {
            mpLayerInvalidator->Invalidate(aOldBoundingBox);
            mpLayerInvalidator->Invalidate(GetBoundingBox());
        }
    }
}

void InsertionIndicatorOverlay::Paint (
    OutputDevice& rDevice,
    const ::tools::Rectangle&)
{
    if ( ! IsVisible())
        return;

    rDevice.DrawImage(maLocation, Image(maIcon));
}

void InsertionIndicatorOverlay::SetLayerInvalidator (std::unique_ptr<ILayerInvalidator> pInvalidator)
{
    mpLayerInvalidator = std::move(pInvalidator);

    if (mbIsVisible && mpLayerInvalidator)
        mpLayerInvalidator->Invalidate(GetBoundingBox());
}

void InsertionIndicatorOverlay::Show()
{
    if (  mbIsVisible)
        return;

    mbIsVisible = true;

    std::shared_ptr<LayeredDevice> pLayeredDevice (
        mrSlideSorter.GetView().GetLayeredDevice());
    if (pLayeredDevice)
    {
        pLayeredDevice->RegisterPainter(shared_from_this(), gnLayerIndex);
        if (mpLayerInvalidator)
            mpLayerInvalidator->Invalidate(GetBoundingBox());
    }
}

void InsertionIndicatorOverlay::Hide()
{
    if (!mbIsVisible)
        return;

    mbIsVisible = false;

    std::shared_ptr<LayeredDevice> pLayeredDevice (
        mrSlideSorter.GetView().GetLayeredDevice());
    if (pLayeredDevice)
    {
        if (mpLayerInvalidator)
            mpLayerInvalidator->Invalidate(GetBoundingBox());
        pLayeredDevice->RemovePainter(shared_from_this(), gnLayerIndex);
    }
}

::tools::Rectangle InsertionIndicatorOverlay::GetBoundingBox() const
{
    return ::tools::Rectangle(maLocation, maIcon.GetSizePixel());
}

Size InsertionIndicatorOverlay::GetSize() const
{
    return Size(
        maIcon.GetSizePixel().Width() + 10,
        maIcon.GetSizePixel().Height() + 10);
}

} // end of namespace ::sd::slidesorter::view

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
