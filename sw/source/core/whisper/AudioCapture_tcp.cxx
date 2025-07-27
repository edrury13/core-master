/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// TCP-based audio capture for WSL - receives audio from Windows host
#ifdef USE_TCP_AUDIO

#include <whisper/AudioCapture.hxx>
#include <sal/log.hxx>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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
    
    int m_socket = -1;
    std::thread m_captureThread;
    std::atomic<bool> m_stopCapture{false};
    
    void captureThreadFunc() {
        const int bufferSize = 4096;
        std::vector<sal_Int16> buffer(bufferSize / sizeof(sal_Int16));
        
        while (!m_stopCapture) {
            // Receive audio data from Windows
            ssize_t received = recv(m_socket, buffer.data(), 
                                  buffer.size() * sizeof(sal_Int16), 0);
            
            if (received <= 0) {
                if (received < 0) {
                    SAL_WARN("sw.whisper", "TCP receive error");
                }
                break;
            }
            
            size_t samples = received / sizeof(sal_Int16);
            
            // Store in buffer
            {
                std::lock_guard<std::mutex> lock(m_bufferMutex);
                m_audioBuffer.insert(m_audioBuffer.end(), 
                                   buffer.begin(), buffer.begin() + samples);
            }
            
            // Calculate RMS for level monitoring
            if (m_fnLevelCallback && samples > 0) {
                float fSum = 0;
                for (size_t i = 0; i < samples; ++i) {
                    float fSample = buffer[i] / 32768.0f;
                    fSum += fSample * fSample;
                }
                float fRms = std::sqrt(fSum / samples);
                m_fnLevelCallback(fRms);
            }
        }
        
        close(m_socket);
        m_socket = -1;
    }
};

bool AudioCapture::initialize(sal_Int32 nSampleRate, sal_Int32 nChannels) {
    m_pImpl->m_nSampleRate = nSampleRate;
    m_pImpl->m_nChannels = nChannels;
    
    // Check for audio bridge environment variable
    const char* audioPort = getenv("LIBREOFFICE_AUDIO_BRIDGE_PORT");
    if (!audioPort) {
        SAL_WARN("sw.whisper", "LIBREOFFICE_AUDIO_BRIDGE_PORT not set, using fake audio");
        return true;
    }
    
    int port = atoi(audioPort);
    if (port <= 0 || port > 65535) {
        SAL_WARN("sw.whisper", "Invalid audio bridge port: " << audioPort);
        return true;
    }
    
    // Create socket
    m_pImpl->m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_pImpl->m_socket < 0) {
        SAL_WARN("sw.whisper", "Failed to create socket");
        return true;
    }
    
    // Connect to Windows audio bridge
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Get Windows host IP (usually the default gateway in WSL)
    const char* hostIP = getenv("LIBREOFFICE_AUDIO_BRIDGE_HOST");
    if (!hostIP) {
        // Try to detect Windows host IP
        FILE* fp = popen("ip route | grep default | awk '{print $3}'", "r");
        if (fp) {
            char ipBuffer[64];
            if (fgets(ipBuffer, sizeof(ipBuffer), fp)) {
                ipBuffer[strcspn(ipBuffer, "\n")] = 0;
                hostIP = ipBuffer;
            }
            pclose(fp);
        }
    }
    
    if (!hostIP || inet_pton(AF_INET, hostIP, &addr.sin_addr) <= 0) {
        SAL_WARN("sw.whisper", "Invalid host IP for audio bridge");
        close(m_pImpl->m_socket);
        m_pImpl->m_socket = -1;
        return true;
    }
    
    // Try to connect
    if (connect(m_pImpl->m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        SAL_WARN("sw.whisper", "Failed to connect to audio bridge at " << hostIP << ":" << port);
        close(m_pImpl->m_socket);
        m_pImpl->m_socket = -1;
        return true;
    }
    
    SAL_WARN("sw.whisper", "Connected to Windows audio bridge at " << hostIP << ":" << port);
    return true;
}

bool AudioCapture::startRecording() {
    if (m_pImpl->m_bRecording)
        return true;
        
    if (m_pImpl->m_socket < 0) {
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
    
    // Send start command to Windows
    const char* cmd = "START\n";
    send(m_pImpl->m_socket, cmd, strlen(cmd), 0);
    
    // Start receiving thread
    m_pImpl->m_stopCapture = false;
    m_pImpl->m_captureThread = std::thread(&Impl::captureThreadFunc, m_pImpl.get());
    m_pImpl->m_bRecording = true;
    
    SAL_WARN("sw.whisper", "Started TCP audio recording");
    return true;
}

void AudioCapture::stopRecording() {
    if (!m_pImpl->m_bRecording)
        return;
        
    m_pImpl->m_bRecording = false;
    
    if (m_pImpl->m_socket >= 0) {
        // Send stop command
        const char* cmd = "STOP\n";
        send(m_pImpl->m_socket, cmd, strlen(cmd), 0);
        
        m_pImpl->m_stopCapture = true;
        if (m_pImpl->m_captureThread.joinable()) {
            m_pImpl->m_captureThread.join();
        }
        SAL_WARN("sw.whisper", "Stopped TCP audio recording");
    } else {
        SAL_WARN("sw.whisper", "Stopped fake audio recording");
    }
}

} // namespace sw::whisper

#endif // USE_TCP_AUDIO

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */