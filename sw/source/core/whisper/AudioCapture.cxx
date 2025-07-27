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
#include <cmath>
#include <thread>
#include <atomic>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Platform-specific audio includes
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#elif defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#elif defined(__linux__)
#include <alsa/asoundlib.h>
#include <poll.h>
#include <errno.h>
#endif

namespace sw::whisper {

class AudioCapture::Impl {
public:
    std::vector<sal_Int16> m_audioBuffer;
    std::mutex m_bufferMutex;
    std::function<void(float)> m_fnLevelCallback;
    sal_Int32 m_nSampleRate = 16000;
    sal_Int32 m_nChannels = 1;
    bool m_bRecording = false;
    
#ifdef _WIN32
    HWAVEIN m_hWaveIn = nullptr;
    static constexpr int BUFFER_COUNT = 4;
    static constexpr int BUFFER_SIZE = 4096;
    std::vector<WAVEHDR> m_waveHeaders;
    std::vector<std::vector<char>> m_waveBuffers;
    
    static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, 
                                   DWORD_PTR dwInstance, 
                                   DWORD_PTR dwParam1, 
                                   DWORD_PTR dwParam2);
#elif defined(__APPLE__)
    AudioQueueRef m_audioQueue = nullptr;
    AudioQueueBufferRef m_audioBuffers[3];
    static constexpr int BUFFER_COUNT = 3;
    static constexpr int BUFFER_SIZE = 4096;
    
    static void audioQueueCallback(void* inUserData,
                                  AudioQueueRef inAQ,
                                  AudioQueueBufferRef inBuffer,
                                  const AudioTimeStamp* inStartTime,
                                  UInt32 inNumPackets,
                                  const AudioStreamPacketDescription* inPacketDesc);
#elif defined(__linux__)
    snd_pcm_t* m_pcmHandle = nullptr;
    std::thread m_captureThread;
    std::atomic<bool> m_stopCapture{false};
    
    void captureThreadFunc();
#endif
};

#ifdef _WIN32
void CALLBACK AudioCapture::Impl::waveInProc(
    HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, 
    DWORD_PTR dwParam1, DWORD_PTR)
{
    if (uMsg != WIM_DATA)
        return;
        
    auto* pImpl = reinterpret_cast<Impl*>(dwInstance);
    WAVEHDR* pHdr = reinterpret_cast<WAVEHDR*>(dwParam1);
    
    if (!pImpl->m_bRecording || pHdr->dwBytesRecorded == 0)
        return;
        
    // Process audio data
    auto* pData = reinterpret_cast<sal_Int16*>(pHdr->lpData);
    size_t nSamples = pHdr->dwBytesRecorded / sizeof(sal_Int16);
    
    {
        std::lock_guard<std::mutex> lock(pImpl->m_bufferMutex);
        pImpl->m_audioBuffer.insert(pImpl->m_audioBuffer.end(), 
                                   pData, pData + nSamples);
    }
    
    // Calculate RMS for level monitoring
    if (pImpl->m_fnLevelCallback) {
        float fSum = 0;
        for (size_t i = 0; i < nSamples; ++i) {
            float fSample = pData[i] / 32768.0f;
            fSum += fSample * fSample;
        }
        float fRms = std::sqrt(fSum / nSamples);
        pImpl->m_fnLevelCallback(fRms);
    }
    
    // Re-queue buffer
    waveInAddBuffer(hwi, pHdr, sizeof(WAVEHDR));
}
#elif defined(__APPLE__)
void AudioCapture::Impl::audioQueueCallback(
    void* inUserData,
    AudioQueueRef inAQ,
    AudioQueueBufferRef inBuffer,
    const AudioTimeStamp*,
    UInt32,
    const AudioStreamPacketDescription*)
{
    auto* pImpl = static_cast<Impl*>(inUserData);
    
    if (!pImpl->m_bRecording || inBuffer->mAudioDataByteSize == 0)
        return;
        
    // Process audio data
    auto* pData = reinterpret_cast<sal_Int16*>(inBuffer->mAudioData);
    size_t nSamples = inBuffer->mAudioDataByteSize / sizeof(sal_Int16);
    
    {
        std::lock_guard<std::mutex> lock(pImpl->m_bufferMutex);
        pImpl->m_audioBuffer.insert(pImpl->m_audioBuffer.end(), 
                                   pData, pData + nSamples);
    }
    
    // Calculate RMS for level monitoring
    if (pImpl->m_fnLevelCallback) {
        float fSum = 0;
        for (size_t i = 0; i < nSamples; ++i) {
            float fSample = pData[i] / 32768.0f;
            fSum += fSample * fSample;
        }
        float fRms = std::sqrt(fSum / nSamples);
        pImpl->m_fnLevelCallback(fRms);
    }
    
    // Re-enqueue buffer
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, nullptr);
}
#elif defined(__linux__)
void AudioCapture::Impl::captureThreadFunc() {
    SAL_WARN("sw.whisper", "Audio capture thread started");
    const int frames = 256;
    std::vector<sal_Int16> buffer(frames * m_nChannels);
    
    // Use blocking mode with poll for interruptible reads
    snd_pcm_nonblock(m_pcmHandle, 0);
    
    while (!m_stopCapture) {
        // Use poll to wait for data with timeout
        struct pollfd pfd;
        snd_pcm_poll_descriptors(m_pcmHandle, &pfd, 1);
        
        int err = poll(&pfd, 1, 100); // 100ms timeout
        if (err < 0) {
            SAL_WARN("sw.whisper", "Poll error: " << strerror(errno));
            break;
        } else if (err == 0) {
            // Timeout - check if we should stop
            continue;
        }
        
        int rc = snd_pcm_readi(m_pcmHandle, buffer.data(), frames);
        if (rc == -EAGAIN) {
            // No data available, continue
            continue;
        } else if (rc == -EPIPE) {
            // Overrun occurred
            snd_pcm_prepare(m_pcmHandle);
        } else if (rc == -ESTRPIPE) {
            // Stream suspended, exit
            SAL_WARN("sw.whisper", "Stream suspended, exiting capture thread");
            break;
        } else if (rc == -EBADFD || rc == -ENOTTY || rc == -ENODEV) {
            // Device closed or dropped, exit cleanly
            SAL_WARN("sw.whisper", "Device closed, exiting capture thread");
            break;
        } else if (rc < 0) {
            SAL_WARN("sw.whisper", "Error from read: " << snd_strerror(rc));
            // For critical errors, exit the loop
            if (rc == -EIO || rc == -ENOENT) {
                break;
            }
        } else if (rc > 0) {
            // Process any audio data we got (even if less than requested)
            {
                std::lock_guard<std::mutex> lock(m_bufferMutex);
                m_audioBuffer.insert(m_audioBuffer.end(), 
                                   buffer.begin(), buffer.begin() + rc * m_nChannels);
            }
            
            static int logCount = 0;
            static int totalReads = 0;
            totalReads++;
            if (++logCount % 50 == 0) { // Log every 50 buffers due to short reads
                SAL_WARN("sw.whisper", "Captured " << m_audioBuffer.size() << " samples so far (from " << totalReads << " reads)");
            }
            
            // Calculate RMS for level monitoring
            if (m_fnLevelCallback) {
                float fSum = 0;
                for (int i = 0; i < rc * m_nChannels; ++i) {
                    float fSample = buffer[i] / 32768.0f;
                    fSum += fSample * fSample;
                }
                float fRms = std::sqrt(fSum / (rc * m_nChannels));
                m_fnLevelCallback(fRms);
            }
        }
    }
    SAL_WARN("sw.whisper", "Audio capture thread exiting, total samples: " << m_audioBuffer.size());
}
#endif

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
    
    SAL_WARN("sw.whisper", "AudioCapture::initialize - Sample rate: " << nSampleRate 
             << ", Channels: " << nChannels);
    
#ifdef _WIN32
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = nChannels;
    wfx.nSamplesPerSec = nSampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    
    // Open wave input device
    MMRESULT result = waveInOpen(
        &m_pImpl->m_hWaveIn,
        WAVE_MAPPER,
        &wfx,
        reinterpret_cast<DWORD_PTR>(Impl::waveInProc),
        reinterpret_cast<DWORD_PTR>(m_pImpl.get()),
        CALLBACK_FUNCTION
    );
    
    if (result != MMSYSERR_NOERROR) {
        SAL_WARN("sw.whisper", "Failed to open wave input device: " << result);
        return false;
    }
    
    // Prepare buffers
    m_pImpl->m_waveHeaders.resize(Impl::BUFFER_COUNT);
    m_pImpl->m_waveBuffers.resize(Impl::BUFFER_COUNT);
    
    for (int i = 0; i < Impl::BUFFER_COUNT; ++i) {
        m_pImpl->m_waveBuffers[i].resize(Impl::BUFFER_SIZE);
        
        WAVEHDR& hdr = m_pImpl->m_waveHeaders[i];
        hdr.lpData = m_pImpl->m_waveBuffers[i].data();
        hdr.dwBufferLength = Impl::BUFFER_SIZE;
        hdr.dwBytesRecorded = 0;
        hdr.dwUser = 0;
        hdr.dwFlags = 0;
        hdr.dwLoops = 0;
        
        result = waveInPrepareHeader(m_pImpl->m_hWaveIn, &hdr, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            SAL_WARN("sw.whisper", "Failed to prepare wave header: " << result);
            shutdown();
            return false;
        }
        
        result = waveInAddBuffer(m_pImpl->m_hWaveIn, &hdr, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            SAL_WARN("sw.whisper", "Failed to add wave buffer: " << result);
            shutdown();
            return false;
        }
    }
    
    return true;
#elif defined(__APPLE__)
    // Set up audio format
    AudioStreamBasicDescription format = {};
    format.mSampleRate = nSampleRate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mFramesPerPacket = 1;
    format.mChannelsPerFrame = nChannels;
    format.mBitsPerChannel = 16;
    format.mBytesPerPacket = format.mBytesPerFrame = format.mChannelsPerFrame * 2;
    
    // Create audio queue
    OSStatus status = AudioQueueNewInput(
        &format,
        Impl::audioQueueCallback,
        m_pImpl.get(),
        nullptr,
        kCFRunLoopCommonModes,
        0,
        &m_pImpl->m_audioQueue
    );
    
    if (status != noErr) {
        SAL_WARN("sw.whisper", "Failed to create audio queue: " << status);
        return false;
    }
    
    // Allocate buffers
    for (int i = 0; i < Impl::BUFFER_COUNT; ++i) {
        status = AudioQueueAllocateBuffer(
            m_pImpl->m_audioQueue,
            Impl::BUFFER_SIZE,
            &m_pImpl->m_audioBuffers[i]
        );
        
        if (status != noErr) {
            SAL_WARN("sw.whisper", "Failed to allocate audio buffer: " << status);
            shutdown();
            return false;
        }
        
        status = AudioQueueEnqueueBuffer(
            m_pImpl->m_audioQueue,
            m_pImpl->m_audioBuffers[i],
            0,
            nullptr
        );
        
        if (status != noErr) {
            SAL_WARN("sw.whisper", "Failed to enqueue audio buffer: " << status);
            shutdown();
            return false;
        }
    }
    
    return true;
#elif defined(__linux__)
    int rc;
    snd_pcm_hw_params_t* params;
    
    // Try PulseAudio first (for WSL)
    rc = snd_pcm_open(&m_pImpl->m_pcmHandle, "pulse", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        SAL_WARN("sw.whisper", "Unable to open PulseAudio device: " << snd_strerror(rc));
        
        // Try default ALSA device
        rc = snd_pcm_open(&m_pImpl->m_pcmHandle, "default", SND_PCM_STREAM_CAPTURE, 0);
        if (rc < 0) {
            SAL_WARN("sw.whisper", "Unable to open default PCM device: " << snd_strerror(rc));
            // In WSL or environments without audio, use fake audio for testing
            SAL_WARN("sw.whisper", "Using fake audio capture for testing");
            return true; // Continue with fake audio
        }
        SAL_WARN("sw.whisper", "Opened default ALSA device");
    } else {
        SAL_WARN("sw.whisper", "Opened PulseAudio device successfully");
    }
    
    // Allocate hardware parameters
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(m_pImpl->m_pcmHandle, params);
    
    // Set parameters
    snd_pcm_hw_params_set_access(m_pImpl->m_pcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(m_pImpl->m_pcmHandle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(m_pImpl->m_pcmHandle, params, nChannels);
    
    unsigned int sampleRate = nSampleRate;
    snd_pcm_hw_params_set_rate_near(m_pImpl->m_pcmHandle, params, &sampleRate, 0);
    
    // Write parameters
    rc = snd_pcm_hw_params(m_pImpl->m_pcmHandle, params);
    if (rc < 0) {
        SAL_WARN("sw.whisper", "Unable to set hw parameters: " << snd_strerror(rc));
        shutdown();
        return false;
    }
    
    return true;
#else
    SAL_WARN("sw.whisper", "Audio capture not implemented for this platform");
    // For now, return true to allow testing without actual audio
    return true;
#endif
}

void AudioCapture::shutdown() {
#ifdef _WIN32
    if (m_pImpl->m_hWaveIn) {
        waveInStop(m_pImpl->m_hWaveIn);
        waveInReset(m_pImpl->m_hWaveIn);
        
        for (auto& hdr : m_pImpl->m_waveHeaders) {
            waveInUnprepareHeader(m_pImpl->m_hWaveIn, &hdr, sizeof(WAVEHDR));
        }
        
        waveInClose(m_pImpl->m_hWaveIn);
        m_pImpl->m_hWaveIn = nullptr;
    }
#elif defined(__APPLE__)
    if (m_pImpl->m_audioQueue) {
        AudioQueueStop(m_pImpl->m_audioQueue, true);
        
        for (int i = 0; i < Impl::BUFFER_COUNT; ++i) {
            if (m_pImpl->m_audioBuffers[i]) {
                AudioQueueFreeBuffer(m_pImpl->m_audioQueue, m_pImpl->m_audioBuffers[i]);
            }
        }
        
        AudioQueueDispose(m_pImpl->m_audioQueue, true);
        m_pImpl->m_audioQueue = nullptr;
    }
#elif defined(__linux__)
    if (m_pImpl->m_captureThread.joinable()) {
        m_pImpl->m_stopCapture = true;
        m_pImpl->m_captureThread.join();
    }
    
    if (m_pImpl->m_pcmHandle) {
        snd_pcm_close(m_pImpl->m_pcmHandle);
        m_pImpl->m_pcmHandle = nullptr;
    }
#endif
}

bool AudioCapture::startRecording() {
    if (m_pImpl->m_bRecording)
        return true;
        
#ifdef _WIN32
    if (!m_pImpl->m_hWaveIn)
        return false;
        
    MMRESULT result = waveInStart(m_pImpl->m_hWaveIn);
    if (result != MMSYSERR_NOERROR) {
        SAL_WARN("sw.whisper", "Failed to start recording: " << result);
        return false;
    }
    
    m_pImpl->m_bRecording = true;
    return true;
#elif defined(__APPLE__)
    if (!m_pImpl->m_audioQueue)
        return false;
        
    OSStatus status = AudioQueueStart(m_pImpl->m_audioQueue, nullptr);
    if (status != noErr) {
        SAL_WARN("sw.whisper", "Failed to start recording: " << status);
        return false;
    }
    
    m_pImpl->m_bRecording = true;
    return true;
#elif defined(__linux__)
    if (!m_pImpl->m_pcmHandle) {
        // Fake audio mode - generate test data
        SAL_WARN("sw.whisper", "Starting fake audio recording for testing");
        m_pImpl->m_bRecording = true;
        
        // Generate 3 seconds of test audio with a simple tone
        const int sampleRate = m_pImpl->m_nSampleRate;
        const int duration = 3; // seconds
        const int numSamples = sampleRate * duration * m_pImpl->m_nChannels;
        const float frequency = 440.0f; // A4 note
        
        m_pImpl->m_audioBuffer.clear();
        m_pImpl->m_audioBuffer.reserve(numSamples);
        
        // Generate a simple sine wave tone
        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float sample = 0.3f * std::sin(2.0f * M_PI * frequency * t);
            m_pImpl->m_audioBuffer.push_back(static_cast<sal_Int16>(sample * 32767));
        }
        
        SAL_WARN("sw.whisper", "Generated " << numSamples << " samples of test audio");
        return true;
    }
        
    m_pImpl->m_stopCapture = false;
    m_pImpl->m_captureThread = std::thread(&Impl::captureThreadFunc, m_pImpl.get());
    m_pImpl->m_bRecording = true;
    SAL_WARN("sw.whisper", "Started audio capture thread for PulseAudio");
    return true;
#else
    // For testing without actual audio
    m_pImpl->m_bRecording = true;
    SAL_WARN("sw.whisper", "Audio capture started (stub mode)");
    return true;
#endif
}

void AudioCapture::stopRecording() {
    if (!m_pImpl->m_bRecording)
        return;
        
    m_pImpl->m_bRecording = false;
    
#ifdef _WIN32
    if (m_pImpl->m_hWaveIn) {
        waveInStop(m_pImpl->m_hWaveIn);
    }
#elif defined(__APPLE__)
    if (m_pImpl->m_audioQueue) {
        AudioQueueStop(m_pImpl->m_audioQueue, false);
    }
#elif defined(__linux__)
    if (m_pImpl->m_pcmHandle && m_pImpl->m_captureThread.joinable()) {
        SAL_WARN("sw.whisper", "Stopping audio capture thread");
        m_pImpl->m_stopCapture = true;
        
        // Drop the PCM stream to interrupt any blocking reads
        snd_pcm_drop(m_pImpl->m_pcmHandle);
        
        // Wait for thread to finish
        m_pImpl->m_captureThread.join();
        SAL_WARN("sw.whisper", "Audio capture thread stopped, captured " << m_pImpl->m_audioBuffer.size() << " samples");
    } else {
        SAL_WARN("sw.whisper", "Stopped fake audio recording");
    }
#endif
}

bool AudioCapture::isRecording() const {
    return m_pImpl->m_bRecording;
}

std::vector<sal_Int16> AudioCapture::getAudioData() const {
    std::lock_guard<std::mutex> lock(m_pImpl->m_bufferMutex);
    SAL_WARN("sw.whisper", "AudioCapture::getAudioData - returning " << m_pImpl->m_audioBuffer.size() << " samples");
    return m_pImpl->m_audioBuffer;
}

void AudioCapture::clearBuffer() {
    std::lock_guard<std::mutex> lock(m_pImpl->m_bufferMutex);
    m_pImpl->m_audioBuffer.clear();
}

void AudioCapture::setAudioLevelCallback(std::function<void(float)> callback) {
    m_pImpl->m_fnLevelCallback = callback;
}

std::vector<sal_uInt8> AudioCapture::convertToWav() const {
    std::lock_guard<std::mutex> lock(m_pImpl->m_bufferMutex);
    
    SAL_WARN("sw.whisper", "AudioCapture::convertToWav - converting " << m_pImpl->m_audioBuffer.size() << " samples to WAV");
    
    // WAV file header structure
    struct WavHeader {
        char chunkId[4] = {'R', 'I', 'F', 'F'};
        sal_uInt32 chunkSize;
        char format[4] = {'W', 'A', 'V', 'E'};
        char subchunk1Id[4] = {'f', 'm', 't', ' '};
        sal_uInt32 subchunk1Size = 16;
        sal_uInt16 audioFormat = 1; // PCM
        sal_uInt16 numChannels;
        sal_uInt32 sampleRate;
        sal_uInt32 byteRate;
        sal_uInt16 blockAlign;
        sal_uInt16 bitsPerSample = 16;
        char subchunk2Id[4] = {'d', 'a', 't', 'a'};
        sal_uInt32 subchunk2Size;
    };
    
    WavHeader header;
    header.numChannels = m_pImpl->m_nChannels;
    header.sampleRate = m_pImpl->m_nSampleRate;
    header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    header.subchunk2Size = m_pImpl->m_audioBuffer.size() * sizeof(sal_Int16);
    header.chunkSize = 36 + header.subchunk2Size;
    
    std::vector<sal_uInt8> wavData;
    wavData.reserve(sizeof(WavHeader) + header.subchunk2Size);
    
    // Write header
    const sal_uInt8* pHeader = reinterpret_cast<const sal_uInt8*>(&header);
    wavData.insert(wavData.end(), pHeader, pHeader + sizeof(WavHeader));
    
    // Write audio data
    const sal_uInt8* pAudioData = reinterpret_cast<const sal_uInt8*>(m_pImpl->m_audioBuffer.data());
    wavData.insert(wavData.end(), pAudioData, 
                   pAudioData + m_pImpl->m_audioBuffer.size() * sizeof(sal_Int16));
    
    return wavData;
}

double AudioCapture::getRecordingDuration() const {
    std::lock_guard<std::mutex> lock(m_pImpl->m_bufferMutex);
    return static_cast<double>(m_pImpl->m_audioBuffer.size()) / m_pImpl->m_nSampleRate;
}

} // namespace sw::whisper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */