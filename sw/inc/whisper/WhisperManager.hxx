/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_INC_WHISPER_WHISPERMANAGER_HXX
#define INCLUDED_SW_INC_WHISPER_WHISPERMANAGER_HXX

#include <swdllapi.h>
#include <rtl/ustring.hxx>
#include <memory>
#include <functional>

namespace sw::whisper {

enum class WhisperState {
    Idle,
    Recording,
    Processing,
    Error
};

class WhisperSession;
class WhisperConfig;
class AudioCapture;

class SW_DLLPUBLIC WhisperManager final {
public:
    static WhisperManager& getInstance();
    
    // Main API
    void startRecording();
    void stopRecording();
    void cancelRecording();
    
    // Configuration
    WhisperConfig& getConfig();
    bool isConfigured() const;
    
    // State
    WhisperState getState() const { return m_eState; }
    void setStateChangeCallback(std::function<void(WhisperState)> callback);
    
    // Results
    void setTextInsertCallback(std::function<void(const OUString&)> callback);
    
    // Error handling
    OUString getLastError() const { return m_sLastError; }
    
private:
    WhisperManager();
    ~WhisperManager();
    WhisperManager(const WhisperManager&) = delete;
    WhisperManager& operator=(const WhisperManager&) = delete;
    
    void ensureConfigInitialized() const;
    
    std::unique_ptr<WhisperSession> m_pSession;
    std::unique_ptr<WhisperConfig> m_pConfig;
    std::unique_ptr<AudioCapture> m_pAudioCapture;
    
    WhisperState m_eState;
    OUString m_sLastError;
    std::function<void(WhisperState)> m_fnStateCallback;
    std::function<void(const OUString&)> m_fnTextCallback;
};

} // namespace sw::whisper

#endif // INCLUDED_SW_INC_WHISPER_WHISPERMANAGER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */