/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_INC_WHISPER_WHISPERSETTINGSDIALOG_HXX
#define INCLUDED_SW_INC_WHISPER_WHISPERSETTINGSDIALOG_HXX

#include <swdllapi.h>
#include <vcl/weld.hxx>

namespace sw::whisper {

class WhisperConfig;

class SW_DLLPUBLIC WhisperSettingsDialog final : public weld::GenericDialogController
{
public:
    WhisperSettingsDialog(weld::Window* pParent, WhisperConfig& rConfig);
    virtual ~WhisperSettingsDialog() override;
    
private:
    WhisperConfig& m_rConfig;
    
    std::unique_ptr<weld::ComboBox> m_xModelCombo;
    std::unique_ptr<weld::ComboBox> m_xLanguageCombo;
    std::unique_ptr<weld::TextView> m_xPromptText;
    std::unique_ptr<weld::Scale> m_xTemperatureScale;
    std::unique_ptr<weld::Label> m_xApiKeyInfo;
    std::unique_ptr<weld::Button> m_xOKButton;
    
    void LoadSettings();
    void SaveSettings();
    
    DECL_LINK(OkHdl, weld::Button&, void);
};

} // namespace sw::whisper

#endif // INCLUDED_SW_INC_WHISPER_WHISPERSETTINGSDIALOG_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */