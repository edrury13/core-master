/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/stbitem.hxx>

class ScTimerStatusBarControl final : public SfxStatusBarControl
{
public:
    SFX_DECL_STATUSBAR_CONTROL();

    ScTimerStatusBarControl(sal_uInt16 nSlotId, sal_uInt16 nId, StatusBar& rStb);
    virtual ~ScTimerStatusBarControl() override;

    virtual void StateChangedAtStatusBarControl(sal_uInt16 nSID, SfxItemState eState,
                                                const SfxPoolItem* pState) override;
    virtual void Click() override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */