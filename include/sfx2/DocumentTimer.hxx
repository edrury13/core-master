/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/dllapi.h>
#include <vcl/timer.hxx>
#include <tools/time.hxx>
#include <rtl/ustring.hxx>
#include <svl/lstner.hxx>
#include <memory>

class SfxViewShell;
class SfxObjectShell;
class SfxHint;

namespace sfx2
{

/** DocumentTimer tracks time spent working on a document
 *
 * Features:
 * - Tracks active time when document has focus
 * - Saves time data with document
 * - Shows time in status bar
 * - Shared implementation for all LibreOffice applications
 */
class SFX2_DLLPUBLIC DocumentTimer : public SfxListener
{
private:
    SfxViewShell* m_pViewShell;
    SfxObjectShell* m_pDocShell;
    
    Timer m_aSecondTimer;       // Updates every second
    Timer m_aAutoSaveTimer;     // Periodically saves time data
    
    sal_Int64 m_nTotalSeconds;  // Total time worked on document
    sal_Int64 m_nSessionSeconds; // Current session time
    tools::Time m_aSessionStart;  // When current session started
    
    bool m_bActive;             // Timer currently running
    bool m_bModified;           // Time data changed since last save
    
    static constexpr sal_Int32 AUTOSAVE_INTERVAL_MS = 60000; // 1 minute
    
    DECL_LINK(SecondTimerHdl, Timer*, void);
    DECL_LINK(AutoSaveTimerHdl, Timer*, void);
    
    void UpdateDisplay();
    void SaveTimeData();
    sal_Int64 GetCurrentSessionSeconds() const;
    
public:
    DocumentTimer(SfxViewShell* pViewShell);
    ~DocumentTimer();
    
    // Control methods
    void Start();
    void Stop();
    void Toggle() { IsActive() ? Stop() : Start(); }
    
    // State queries
    bool IsActive() const { return m_bActive; }
    sal_Int64 GetTotalSeconds() const { return m_nTotalSeconds + GetCurrentSessionSeconds(); }
    OUString GetTimeString() const;
    
    // Document integration
    void LoadFromDocument();
    void SaveToDocument();
    void OnDocumentSave();
    
    // SfxListener override
    virtual void Notify(SfxBroadcaster& rBC, const SfxHint& rHint) override;
};

} // namespace sfx2

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */