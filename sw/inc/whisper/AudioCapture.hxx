/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_INC_WHISPER_AUDIOCAPTURE_HXX
#define INCLUDED_SW_INC_WHISPER_AUDIOCAPTURE_HXX

#include <swdllapi.h>
#include <vector>
#include <memory>
#include <functional>
#include <sal/types.h>

namespace sw::whisper {

class SW_DLLPUBLIC AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();
    
    // Platform-specific initialization
    bool initialize(sal_Int32 nSampleRate = 16000, sal_Int32 nChannels = 1);
    void shutdown();
    
    // Recording control
    bool startRecording();
    void stopRecording();
    bool isRecording() const;
    
    // Get captured audio data
    std::vector<sal_Int16> getAudioData() const;
    void clearBuffer();
    
    // Callbacks
    void setAudioLevelCallback(std::function<void(float)> callback);
    
    // Audio format conversion
    std::vector<sal_uInt8> convertToWav() const;
    
    // Get recording duration in seconds
    double getRecordingDuration() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> m_pImpl;
};

} // namespace sw::whisper

#endif // INCLUDED_SW_INC_WHISPER_AUDIOCAPTURE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */