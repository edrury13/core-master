/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_INC_WHISPER_WHISPERCONFIG_HXX
#define INCLUDED_SW_INC_WHISPER_WHISPERCONFIG_HXX

#include <swdllapi.h>
#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Sequence.hxx>

namespace sw::whisper {

class SW_DLLPUBLIC WhisperConfig {
public:
    WhisperConfig();
    ~WhisperConfig();
    
    // API Key Management - from environment variable
    OUString getApiKey() const;
    bool hasApiKey() const;
    
    // Settings
    void setModel(const OUString& rModel);
    OUString getModel() const;
    
    void setLanguage(const OUString& rLang);
    OUString getLanguage() const;
    
    void setPrompt(const OUString& rPrompt);
    OUString getPrompt() const;
    
    void setTemperature(double fTemp);
    double getTemperature() const;
    
    // Audio settings
    OUString getAudioFormat() const { return "wav"; }
    sal_Int32 getSampleRate() const { return 16000; }
    
    // Placeholder methods for compatibility
    void Notify(const css::uno::Sequence<OUString>& aPropertyNames);
    void ImplCommit();
    
private:
    
    OUString m_sModel;
    OUString m_sLanguage;
    OUString m_sPrompt;
    double m_fTemperature;
};

} // namespace sw::whisper

#endif // INCLUDED_SW_INC_WHISPER_WHISPERCONFIG_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */