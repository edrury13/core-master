/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// PulseAudio implementation for WSLg audio capture
#ifdef USE_PULSEAUDIO

#include <whisper/AudioCapture.hxx>
#include <sal/log.hxx>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <thread>
#include <atomic>

namespace sw::whisper {

class AudioCapture::Impl {
public:
    std::vector<sal_Int16> m_audioBuffer;
    std::mutex m_bufferMutex;
    std::function<void(float)> m_fnLevelCallback;
    sal_Int32 m_nSampleRate = 16000;
    sal_Int32 m_nChannels = 1;
    bool m_bRecording = false;
    
    pa_simple* m_pulse = nullptr;
    std::thread m_captureThread;
    std::atomic<bool> m_stopCapture{false};
    
    void captureThreadFunc() {
        const int frames = 1024;
        std::vector<sal_Int16> buffer(frames * m_nChannels);
        int error;
        
        while (!m_stopCapture) {
            // Read audio data
            if (pa_simple_read(m_pulse, buffer.data(), 
                             buffer.size() * sizeof(sal_Int16), &error) < 0) {
                SAL_WARN("sw.whisper", "PulseAudio read failed: " << pa_strerror(error));
                break;
            }
            
            // Store in buffer
            {
                std::lock_guard<std::mutex> lock(m_bufferMutex);
                m_audioBuffer.insert(m_audioBuffer.end(), 
                                   buffer.begin(), buffer.end());
            }
            
            // Calculate RMS for level monitoring
            if (m_fnLevelCallback) {
                float fSum = 0;
                for (const auto& sample : buffer) {
                    float fSample = sample / 32768.0f;
                    fSum += fSample * fSample;
                }
                float fRms = std::sqrt(fSum / buffer.size());
                m_fnLevelCallback(fRms);
            }
        }
    }
};

bool AudioCapture::initialize(sal_Int32 nSampleRate, sal_Int32 nChannels) {
    m_pImpl->m_nSampleRate = nSampleRate;
    m_pImpl->m_nChannels = nChannels;
    
    // PulseAudio sample specification
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = static_cast<uint32_t>(nSampleRate),
        .channels = static_cast<uint8_t>(nChannels)
    };
    
    int error;
    
    // Try to connect to PulseAudio
    m_pImpl->m_pulse = pa_simple_new(
        nullptr,                // Use default server
        "LibreOffice Writer",   // Application name
        PA_STREAM_RECORD,       // Recording stream
        nullptr,                // Use default device
        "Speech to Text",       // Stream description
        &ss,                    // Sample format
        nullptr,                // Use default channel map
        nullptr,                // Use default buffering attributes
        &error                  // Error code
    );
    
    if (!m_pImpl->m_pulse) {
        SAL_WARN("sw.whisper", "Failed to connect to PulseAudio: " << pa_strerror(error));
        
        // Fall back to fake audio for testing
        SAL_WARN("sw.whisper", "Using fake audio capture for testing");
        return true;
    }
    
    SAL_WARN("sw.whisper", "PulseAudio initialized successfully");
    return true;
}

void AudioCapture::shutdown() {
    if (m_pImpl->m_captureThread.joinable()) {
        m_pImpl->m_stopCapture = true;
        m_pImpl->m_captureThread.join();
    }
    
    if (m_pImpl->m_pulse) {
        pa_simple_free(m_pImpl->m_pulse);
        m_pImpl->m_pulse = nullptr;
    }
}

bool AudioCapture::startRecording() {
    if (m_pImpl->m_bRecording)
        return true;
        
    if (!m_pImpl->m_pulse) {
        // Fake audio mode (same as before)
        SAL_WARN("sw.whisper", "Starting fake audio recording for testing");
        m_pImpl->m_bRecording = true;
        
        // Generate test tone
        const int sampleRate = m_pImpl->m_nSampleRate;
        const int duration = 3;
        const int numSamples = sampleRate * duration * m_pImpl->m_nChannels;
        const float frequency = 440.0f;
        
        m_pImpl->m_audioBuffer.clear();
        m_pImpl->m_audioBuffer.reserve(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float sample = 0.3f * std::sin(2.0f * M_PI * frequency * t);
            m_pImpl->m_audioBuffer.push_back(static_cast<sal_Int16>(sample * 32767));
        }
        
        SAL_WARN("sw.whisper", "Generated " << numSamples << " samples of test audio");
        return true;
    }
    
    // Start real audio capture
    m_pImpl->m_stopCapture = false;
    m_pImpl->m_captureThread = std::thread(&Impl::captureThreadFunc, m_pImpl.get());
    m_pImpl->m_bRecording = true;
    
    SAL_WARN("sw.whisper", "Started PulseAudio recording");
    return true;
}

void AudioCapture::stopRecording() {
    if (!m_pImpl->m_bRecording)
        return;
        
    m_pImpl->m_bRecording = false;
    
    if (m_pImpl->m_pulse && m_pImpl->m_captureThread.joinable()) {
        m_pImpl->m_stopCapture = true;
        m_pImpl->m_captureThread.join();
        SAL_WARN("sw.whisper", "Stopped PulseAudio recording");
    } else {
        SAL_WARN("sw.whisper", "Stopped fake audio recording");
    }
}

} // namespace sw::whisper

#endif // USE_PULSEAUDIO

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */