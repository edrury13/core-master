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

namespace sd
{

SFX_IMPL_STATUSBAR_CONTROL(SdTimerStatusBarControl, SfxStringItem);

SdTimerStatusBarControl::SdTimerStatusBarControl(
        sal_uInt16 _nSlotId,
        sal_uInt16 _nId,
        StatusBar& rStb) :
    SfxStatusBarControl(_nSlotId, _nId, rStb)
{
    // Set initial text
    GetStatusBar().SetItemText(GetId(), u"Timer: 00:00:00 [Stopped]"_ustr);
    SAL_INFO("sd.ui", "SdTimerStatusBarControl created with slot " << _nSlotId);
}

SdTimerStatusBarControl::~SdTimerStatusBarControl()
{
}

void SdTimerStatusBarControl::StateChangedAtStatusBarControl(
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

void SdTimerStatusBarControl::Click()
{
    // Call the parent implementation to execute the command
    SfxStatusBarControl::Click();
}

} // namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */