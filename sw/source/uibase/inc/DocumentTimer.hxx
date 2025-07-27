/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/timer.hxx>
#include <tools/time.hxx>
#include <rtl/ustring.hxx>

class SwDocShell;
class SwView;

namespace sw
{

/** DocumentTimer tracks time spent working on a document
 *
 * Features:
 * - Tracks active time when document has focus
 * - Pauses automatically when idle or focus lost
 * - Saves time data with document
 * - Shows time in status bar
 */
class DocumentTimer
{
private:
    SwView* m_pView;
    SwDocShell* m_pDocShell;
    
    Timer m_aSecondTimer;       // Updates every second
    Timer m_aIdleTimer;         // Detects user idle
    Timer m_aAutoSaveTimer;     // Periodically saves time data
    
    sal_Int64 m_nTotalSeconds;  // Total time worked on document
    sal_Int64 m_nSessionSeconds; // Current session time
    tools::Time m_aSessionStart;  // When current session started
    
    bool m_bActive;             // Timer currently running
    bool m_bIdle;               // User is idle
    bool m_bModified;           // Time data changed since last save
    
    static constexpr sal_Int32 IDLE_TIMEOUT_MS = 300000; // 5 minutes
    static constexpr sal_Int32 AUTOSAVE_INTERVAL_MS = 60000; // 1 minute
    
    DECL_LINK(SecondTimerHdl, Timer*, void);
    DECL_LINK(IdleTimerHdl, Timer*, void);
    DECL_LINK(AutoSaveTimerHdl, Timer*, void);
    
    void UpdateDisplay();
    void SaveTimeData();
    
public:
    explicit DocumentTimer(SwView* pView);
    ~DocumentTimer();
    
    // Timer control
    void Start();
    void Stop();
    void Pause();
    bool IsActive() const { return m_bActive && !m_bIdle; }
    
    // Time data
    sal_Int64 GetTotalSeconds() const { return m_nTotalSeconds + GetCurrentSessionSeconds(); }
    sal_Int64 GetCurrentSessionSeconds() const;
    OUString GetTimeString() const;
    
    // Document integration
    void LoadFromDocument();
    void SaveToDocument();
    void OnDocumentSave();
    
    // Activity tracking
    void OnUserActivity();
    void OnFocusChanged(bool bHasFocus);
};

} // namespace sw

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */