/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <timerctrl.hxx>
#include <svl/stritem.hxx>
#include <vcl/status.hxx>
#include <vcl/event.hxx>
#include <sfx2/dispatch.hxx>
#include <cmdid.h>
#include <sal/log.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>

SFX_IMPL_STATUSBAR_CONTROL(SwTimerStatusBarControl, SfxStringItem);

SwTimerStatusBarControl::SwTimerStatusBarControl(
        sal_uInt16 _nSlotId,
        sal_uInt16 _nId,
        StatusBar& rStb) :
    SfxStatusBarControl(_nSlotId, _nId, rStb)
{
}

SwTimerStatusBarControl::~SwTimerStatusBarControl()
{
}

void SwTimerStatusBarControl::StateChangedAtStatusBarControl(
    sal_uInt16 /*nSID*/, SfxItemState eState, const SfxPoolItem* pState )
{
    if (eState == SfxItemState::DEFAULT) // Can access pState
    {
        GetStatusBar().SetItemText( GetId(), static_cast<const SfxStringItem*>(pState)->GetValue() );
    }
    else
    {
        GetStatusBar().SetItemText(GetId(), u""_ustr);
    }
}

void SwTimerStatusBarControl::Click()
{
    SAL_WARN("sw.ui", "SwTimerStatusBarControl::Click() called, SlotId=" << GetSlotId());
    
    // Only use the parent implementation - don't execute twice!
    SfxStatusBarControl::Click();
    
    SAL_WARN("sw.ui", "SwTimerStatusBarControl::Click() completed");
}

bool SwTimerStatusBarControl::MouseButtonDown(const MouseEvent& rEvt)
{
    SAL_WARN("sw.ui", "SwTimerStatusBarControl::MouseButtonDown() called, button=" << rEvt.GetButtons());
    if (rEvt.GetButtons() == MOUSE_LEFT && rEvt.GetClicks() == 1)
    {
        SAL_WARN("sw.ui", "Left mouse button clicked on timer control");
        // Return false to allow normal processing
        return false;
    }
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */