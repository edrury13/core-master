/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <whisper/WhisperManager.hxx>
#include <whisper/WhisperSession.hxx>
#include <whisper/WhisperConfig.hxx>
#include <whisper/AudioCapture.hxx>
#include <vcl/svapp.hxx>
#include <sal/log.hxx>

namespace sw::whisper {

WhisperManager& WhisperManager::getInstance() {
    static WhisperManager aInstance;
    return aInstance;
}

WhisperManager::WhisperManager()
    : m_pConfig(nullptr)
    , m_pAudioCapture(std::make_unique<AudioCapture>())
    , m_eState(WhisperState::Idle)
{
    SAL_WARN("sw.whisper", "WhisperManager created - using lazy initialization");
}

WhisperManager::~WhisperManager() = default;

void WhisperManager::startRecording() {
    SAL_WARN("sw.whisper", "WhisperManager::startRecording called - current state: " << static_cast<int>(m_eState));
    
    if (m_eState != WhisperState::Idle) {
        SAL_WARN("sw.whisper", "WhisperManager::startRecording - not idle, returning");
        return;
    }
    
    if (!isConfigured()) {
        SAL_WARN("sw.whisper", "WhisperManager::startRecording - not configured");
        m_eState = WhisperState::Error;
        m_sLastError = "No OpenAI API key configured. Please set LIBREOFFICE_OPENAI_API_KEY environment variable.";
        if (m_fnStateCallback) {
            m_fnStateCallback(m_eState);
        }
        return;
    }
    
    SAL_WARN("sw.whisper", "WhisperManager::startRecording - initializing audio capture");
    try {
        // Initialize audio capture with config settings
        ensureConfigInitialized();
        if (!m_pAudioCapture->initialize(
            m_pConfig->getSampleRate(), 
            1 // mono
        )) {
            throw std::runtime_error("Failed to initialize audio capture");
        }
        
        // Start recording
        SAL_WARN("sw.whisper", "WhisperManager::startRecording - starting audio capture");
        if (!m_pAudioCapture->startRecording()) {
            throw std::runtime_error("Failed to start recording");
        }
        
        SAL_WARN("sw.whisper", "WhisperManager::startRecording - recording started successfully");
        m_eState = WhisperState::Recording;
        if (m_fnStateCallback) {
            m_fnStateCallback(m_eState);
        }
        
        // Set up audio level monitoring
        m_pAudioCapture->setAudioLevelCallback([this](float /*fLevel*/) {
            // Update UI with audio level
            SolarMutexGuard aGuard;
            // Trigger UI update - will be implemented later
        });
        
    } catch (const std::exception& e) {
        SAL_WARN("sw.whisper", "Failed to start recording: " << e.what());
        m_eState = WhisperState::Error;
        m_sLastError = OUString::fromUtf8(e.what());
        if (m_fnStateCallback) {
            m_fnStateCallback(m_eState);
        }
    }
}

void WhisperManager::stopRecording() {
    SAL_WARN("sw.whisper", "WhisperManager::stopRecording called - current state: " << static_cast<int>(m_eState));
    
    if (m_eState != WhisperState::Recording) {
        return;
    }
    
    m_pAudioCapture->stopRecording();
    m_eState = WhisperState::Processing;
    
    SAL_WARN("sw.whisper", "WhisperManager::stopRecording - stopped audio capture, state now: Processing");
    
    if (m_fnStateCallback) {
        m_fnStateCallback(m_eState);
    }
    
    // Get audio data
    SAL_WARN("sw.whisper", "WhisperManager::stopRecording - getting audio data");
    auto audioData = m_pAudioCapture->convertToWav();
    double duration = m_pAudioCapture->getRecordingDuration();
    SAL_WARN("sw.whisper", "WhisperManager::stopRecording - got audio data");
    
    SAL_WARN("sw.whisper", "WhisperManager::stopRecording - audio data size: " << audioData.size() 
             << " bytes, duration: " << duration << " seconds");
    
    // Log WAV header info for debugging
    if (audioData.size() >= 44) {
        sal_uInt32 sampleRate = *reinterpret_cast<const sal_uInt32*>(&audioData[24]);
        sal_uInt16 channels = *reinterpret_cast<const sal_uInt16*>(&audioData[22]);
        sal_uInt16 bitsPerSample = *reinterpret_cast<const sal_uInt16*>(&audioData[34]);
        SAL_WARN("sw.whisper", "WAV format: " << sampleRate << "Hz, " 
                 << channels << " channels, " << bitsPerSample << " bits");
    }
    
    if (audioData.empty() || audioData.size() < 100) {
        SAL_WARN("sw.whisper", "WhisperManager::stopRecording - no audio data captured");
        m_eState = WhisperState::Error;
        m_sLastError = "No audio data captured";
        if (m_fnStateCallback) {
            m_fnStateCallback(m_eState);
        }
        return;
    }
    
    // Create session and process audio
    ensureConfigInitialized();
    m_pSession = std::make_unique<WhisperSession>(*m_pConfig);
    
    SAL_WARN("sw.whisper", "WhisperManager::stopRecording - sending audio to WhisperSession for transcription");
    
    m_pSession->transcribeAsync(
        audioData,
        [this](const OUString& rText) {
            // Success callback
            SAL_WARN("sw.whisper", "WhisperManager - transcription success: " << rText);
            SolarMutexGuard aGuard;
            m_eState = WhisperState::Idle;
            
            if (m_fnTextCallback) {
                SAL_WARN("sw.whisper", "WhisperManager - calling text callback");
                m_fnTextCallback(rText);
            }
            
            if (m_fnStateCallback) {
                m_fnStateCallback(m_eState);
            }
            
            m_pAudioCapture->clearBuffer();
        },
        [this](const OUString& rError) {
            // Error callback
            SolarMutexGuard aGuard;
            m_eState = WhisperState::Error;
            m_sLastError = rError;
            
            if (m_fnStateCallback) {
                m_fnStateCallback(m_eState);
            }
            
            SAL_WARN("sw.whisper", "WhisperManager - transcription error: " << rError);
        }
    );
}

void WhisperManager::cancelRecording() {
    if (m_eState == WhisperState::Recording) {
        m_pAudioCapture->stopRecording();
        m_pAudioCapture->clearBuffer();
    }
    
    if (m_pSession) {
        m_pSession->cancel();
    }
    
    m_eState = WhisperState::Idle;
    if (m_fnStateCallback) {
        m_fnStateCallback(m_eState);
    }
}

void WhisperManager::ensureConfigInitialized() const {
    if (!m_pConfig) {
        SAL_WARN("sw.whisper", "WhisperManager::ensureConfigInitialized - creating WhisperConfig");
        try {
            const_cast<WhisperManager*>(this)->m_pConfig = std::make_unique<WhisperConfig>();
            SAL_WARN("sw.whisper", "WhisperManager::ensureConfigInitialized - WhisperConfig created successfully");
        } catch (const std::exception& e) {
            SAL_WARN("sw.whisper", "WhisperManager::ensureConfigInitialized - Failed to create WhisperConfig: " << e.what());
            throw;
        } catch (...) {
            SAL_WARN("sw.whisper", "WhisperManager::ensureConfigInitialized - Failed to create WhisperConfig: unknown exception");
            throw;
        }
    }
}

WhisperConfig& WhisperManager::getConfig() {
    ensureConfigInitialized();
    return *m_pConfig;
}

bool WhisperManager::isConfigured() const {
    try {
        ensureConfigInitialized();
        bool hasKey = m_pConfig->hasApiKey();
        SAL_WARN("sw.whisper", "WhisperManager::isConfigured - hasApiKey: " << hasKey);
        return hasKey;
    } catch (...) {
        SAL_WARN("sw.whisper", "WhisperManager::isConfigured - Failed to initialize config, returning false");
        return false;
    }
}

void WhisperManager::setStateChangeCallback(std::function<void(WhisperState)> callback) {
    m_fnStateCallback = callback;
}

void WhisperManager::setTextInsertCallback(std::function<void(const OUString&)> callback) {
    m_fnTextCallback = callback;
}

} // namespace sw::whisper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */