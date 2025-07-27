/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <whisper/WhisperConfig.hxx>
#include <o3tl/environment.hxx>
#include <sal/log.hxx>

namespace sw::whisper {

namespace {
    constexpr OUString ENV_VAR_NAME = u"LIBREOFFICE_OPENAI_API_KEY"_ustr;
}

WhisperConfig::WhisperConfig()
    : m_sModel(u"whisper-1"_ustr)
    , m_sLanguage(u"auto"_ustr)
    , m_fTemperature(0.2)
    , m_sPrompt(u""_ustr)
{
    SAL_WARN("sw.whisper", "WhisperConfig constructor - simplified version without ConfigItem");
}

WhisperConfig::~WhisperConfig() = default;

void WhisperConfig::Notify(const css::uno::Sequence<OUString>&) {
    // Not used in simplified version
}

void WhisperConfig::ImplCommit() {
    // Not used in simplified version
}

OUString WhisperConfig::getApiKey() const {
    OUString apiKey = o3tl::getEnvironment(ENV_VAR_NAME);
    SAL_WARN("sw.whisper", "WhisperConfig::getApiKey - API key length: " << apiKey.getLength());
    return apiKey;
}

OUString WhisperConfig::getModel() const {
    return m_sModel;
}

void WhisperConfig::setModel(const OUString& rModel) {
    m_sModel = rModel;
}

OUString WhisperConfig::getLanguage() const {
    return m_sLanguage;
}

void WhisperConfig::setLanguage(const OUString& rLanguage) {
    m_sLanguage = rLanguage;
}

double WhisperConfig::getTemperature() const {
    return m_fTemperature;
}

void WhisperConfig::setTemperature(double fTemperature) {
    m_fTemperature = fTemperature;
}

OUString WhisperConfig::getPrompt() const {
    return m_sPrompt;
}

void WhisperConfig::setPrompt(const OUString& rPrompt) {
    m_sPrompt = rPrompt;
}

bool WhisperConfig::hasApiKey() const {
    OUString apiKey = getApiKey();
    bool hasKey = !apiKey.isEmpty();
    SAL_WARN("sw.whisper", "WhisperConfig::hasApiKey - result: " << hasKey);
    return hasKey;
}

} // namespace sw::whisper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */