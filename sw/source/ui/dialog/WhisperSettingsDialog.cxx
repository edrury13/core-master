/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <whisper/WhisperSettingsDialog.hxx>
#include <whisper/WhisperConfig.hxx>
#include <strings.hrc>
#include <swtypes.hxx>

namespace sw::whisper {

WhisperSettingsDialog::WhisperSettingsDialog(weld::Window* pParent, WhisperConfig& rConfig)
    : weld::GenericDialogController(pParent, "modules/swriter/ui/whispersettingsdialog.ui", 
                                   "WhisperSettingsDialog")
    , m_rConfig(rConfig)
    , m_xModelCombo(m_xBuilder->weld_combo_box("model"))
    , m_xLanguageCombo(m_xBuilder->weld_combo_box("language"))
    , m_xPromptText(m_xBuilder->weld_text_view("prompt"))
    , m_xTemperatureScale(m_xBuilder->weld_scale("temperature"))
    , m_xApiKeyInfo(m_xBuilder->weld_label("api_key_info"))
{
    // Set default response
    m_xDialog->set_default_response(RET_OK);
    
    LoadSettings();
}

WhisperSettingsDialog::~WhisperSettingsDialog()
{
}

void WhisperSettingsDialog::LoadSettings()
{
    // Model
    OUString sModel = m_rConfig.getModel();
    if (sModel == "whisper-1")
        m_xModelCombo->set_active(0);
    else
        m_xModelCombo->set_active_text(sModel);
    
    // Language
    OUString sLang = m_rConfig.getLanguage();
    if (sLang.isEmpty() || sLang == "auto")
        m_xLanguageCombo->set_active_id("auto");
    else
        m_xLanguageCombo->set_active_id(sLang);
    
    // Prompt
    m_xPromptText->set_text(m_rConfig.getPrompt());
    
    // Temperature
    m_xTemperatureScale->set_value(m_rConfig.getTemperature());
    
    // API Key status
    OUString sApiKeyStatus;
    if (m_rConfig.hasApiKey()) {
        sApiKeyStatus = SwResId(STR_WHISPER_API_KEY_CONFIGURED);
    } else {
        sApiKeyStatus = SwResId(STR_WHISPER_API_KEY_NOT_CONFIGURED);
    }
    m_xApiKeyInfo->set_label(sApiKeyStatus);
}

void WhisperSettingsDialog::SaveSettings()
{
    // Model
    m_rConfig.setModel(m_xModelCombo->get_active_text());
    
    // Language
    OUString sLangId = m_xLanguageCombo->get_active_id();
    if (sLangId == "auto")
        m_rConfig.setLanguage("");
    else
        m_rConfig.setLanguage(sLangId);
    
    // Prompt
    m_rConfig.setPrompt(m_xPromptText->get_text());
    
    // Temperature
    m_rConfig.setTemperature(m_xTemperatureScale->get_value());
}

IMPL_LINK_NOARG(WhisperSettingsDialog, OkHdl, weld::Button&, void)
{
    SaveSettings();
    m_xDialog->response(RET_OK);
}

short WhisperSettingsDialog::run()
{
    // Get OK button and connect handler
    auto xOkBtn = m_xDialog->weld_button_for_response(RET_OK);
    if (xOkBtn)
        xOkBtn->connect_clicked(LINK(this, WhisperSettingsDialog, OkHdl));
    
    return GenericDialogController::run();
}

} // namespace sw::whisper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */