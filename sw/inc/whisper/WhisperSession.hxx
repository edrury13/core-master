/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_INC_WHISPER_WHISPERSESSION_HXX
#define INCLUDED_SW_INC_WHISPER_WHISPERSESSION_HXX

#include <swdllapi.h>
#include <rtl/ustring.hxx>
#include <vector>
#include <functional>
#include <memory>

namespace sw::whisper {

class WhisperConfig;

class SW_DLLPUBLIC WhisperSession {
public:
    explicit WhisperSession(const WhisperConfig& rConfig);
    ~WhisperSession();
    
    void transcribeAsync(
        const std::vector<sal_uInt8>& rAudioData,
        std::function<void(const OUString&)> fnSuccess,
        std::function<void(const OUString&)> fnError);
        
    void cancel();
    
private:
    class Impl;
    std::unique_ptr<Impl> m_pImpl;
};

} // namespace sw::whisper

#endif // INCLUDED_SW_INC_WHISPER_WHISPERSESSION_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */