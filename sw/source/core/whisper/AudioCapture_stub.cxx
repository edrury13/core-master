/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <whisper/AudioCapture.hxx>
#include <sal/log.hxx>
#include <memory>
#include <vector>
#include <mutex>

namespace sw::whisper {

class AudioCapture::Impl {
public:
    std::vector<sal_Int16> m_audioBuffer;
    std::mutex m_bufferMutex;
    std::function<void(float)> m_fnLevelCallback;
    sal_Int32 m_nSampleRate = 16000;
    sal_Int32 m_nChannels = 1;
    bool m_bRecording = false;
    double m_fStartTime = 0.0;
};

AudioCapture::AudioCapture()
    : m_pImpl(std::make_unique<Impl>())
{
}

AudioCapture::~AudioCapture() {
    shutdown();
}

bool AudioCapture::initialize(sal_Int32 nSampleRate, sal_Int32 nChannels) {
    m_pImpl->m_nSampleRate = nSampleRate;
    m_pImpl->m_nChannels = nChannels;
    
    // Stub implementation - always succeed
    SAL_INFO("sw.whisper", "AudioCapture stub initialized");
    return true;
}

bool AudioCapture::startRecording() {
    if (m_pImpl->m_bRecording)
        return true;
        
    m_pImpl->m_bRecording = true;
    m_pImpl->m_audioBuffer.clear();
    m_pImpl->m_fStartTime = 0.0; // Would use system time in real implementation
    
    SAL_INFO("sw.whisper", "AudioCapture stub recording started");
    return true;
}

void AudioCapture::stopRecording() {
    if (!m_pImpl->m_bRecording)
        return;
        
    m_pImpl->m_bRecording = false;
    
    SAL_INFO("sw.whisper", "AudioCapture stub recording stopped");
}

void AudioCapture::shutdown() {
    stopRecording();
    SAL_INFO("sw.whisper", "AudioCapture stub shutdown");
}

std::vector<sal_Int16> AudioCapture::getAudioData() const {
    std::lock_guard<std::mutex> lock(m_pImpl->m_bufferMutex);
    return m_pImpl->m_audioBuffer;
}

void AudioCapture::clearBuffer() {
    std::lock_guard<std::mutex> lock(m_pImpl->m_bufferMutex);
    m_pImpl->m_audioBuffer.clear();
}

bool AudioCapture::isRecording() const {
    return m_pImpl->m_bRecording;
}

void AudioCapture::setAudioLevelCallback(std::function<void(float)> callback) {
    m_pImpl->m_fnLevelCallback = std::move(callback);
}

std::vector<sal_uInt8> AudioCapture::convertToWav() const {
    // Stub implementation - return empty WAV
    std::vector<sal_uInt8> wavData;
    
    // Would generate proper WAV header and convert audio data in real implementation
    SAL_INFO("sw.whisper", "AudioCapture stub convertToWav called");
    
    return wavData;
}

double AudioCapture::getRecordingDuration() const {
    if (!m_pImpl->m_audioBuffer.empty()) {
        return static_cast<double>(m_pImpl->m_audioBuffer.size()) / 
               (m_pImpl->m_nSampleRate * m_pImpl->m_nChannels);
    }
    return 0.0;
}

} // namespace sw::whisper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */