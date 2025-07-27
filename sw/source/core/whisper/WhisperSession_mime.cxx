/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <whisper/WhisperSession.hxx>
#include <whisper/WhisperConfig.hxx>
#include <sal/log.hxx>
#include <curl/curl.h>
#include <rtl/strbuf.hxx>
#include <rtl/ustrbuf.hxx>
#include <osl/thread.h>
#include <tools/json_writer.hxx>
#include <atomic>
#include <thread>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace sw::whisper {

class WhisperSession::Impl {
public:
    const WhisperConfig& m_rConfig;
    std::atomic<bool> m_bCancelled{false};
    std::thread m_thread;
    
    explicit Impl(const WhisperConfig& rConfig) : m_rConfig(rConfig) {}
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        auto* pBuffer = static_cast<OStringBuffer*>(userp);
        pBuffer->append(static_cast<const char*>(contents), totalSize);
        return totalSize;
    }
    
    void transcribe(const std::vector<sal_uInt8>& rAudioData,
                   std::function<void(const OUString&)> fnSuccess,
                   std::function<void(const OUString&)> fnError) {
        
        SAL_WARN("sw.whisper", "WhisperSession::transcribe - starting transcription, audio size: " << rAudioData.size());
        
        if (m_bCancelled)
            return;
            
        CURL* curl = curl_easy_init();
        if (!curl) {
            SAL_WARN("sw.whisper", "WhisperSession::transcribe - failed to initialize CURL");
            fnError("Failed to initialize CURL");
            return;
        }
        
        struct curl_slist* headers = nullptr;
        curl_mime* mime = nullptr;
        curl_mimepart* part = nullptr;
        
        try {
            // Set up headers
            OString sAuthHeader = "Authorization: Bearer " + 
                OUStringToOString(m_rConfig.getApiKey(), RTL_TEXTENCODING_UTF8);
            headers = curl_slist_append(headers, sAuthHeader.getStr());
            
            // Create mime form
            mime = curl_mime_init(curl);
            
            // Add file part
            part = curl_mime_addpart(mime);
            curl_mime_name(part, "file");
            curl_mime_filename(part, "audio.wav");
            curl_mime_data(part, reinterpret_cast<const char*>(rAudioData.data()), rAudioData.size());
            curl_mime_type(part, "audio/wav");
            
            // Add model part
            part = curl_mime_addpart(mime);
            curl_mime_name(part, "model");
            OString sModel = OUStringToOString(m_rConfig.getModel(), RTL_TEXTENCODING_UTF8);
            curl_mime_data(part, sModel.getStr(), sModel.getLength());
            
            // Add language part if specified
            if (!m_rConfig.getLanguage().isEmpty() && m_rConfig.getLanguage() != "auto") {
                part = curl_mime_addpart(mime);
                curl_mime_name(part, "language");
                OString sLang = OUStringToOString(m_rConfig.getLanguage(), RTL_TEXTENCODING_UTF8);
                curl_mime_data(part, sLang.getStr(), sLang.getLength());
            }
            
            // Add prompt part if specified
            if (!m_rConfig.getPrompt().isEmpty()) {
                part = curl_mime_addpart(mime);
                curl_mime_name(part, "prompt");
                OString sPrompt = OUStringToOString(m_rConfig.getPrompt(), RTL_TEXTENCODING_UTF8);
                curl_mime_data(part, sPrompt.getStr(), sPrompt.getLength());
            }
            
            // Add temperature part
            part = curl_mime_addpart(mime);
            curl_mime_name(part, "temperature");
            OString sTemp = OString::number(m_rConfig.getTemperature());
            curl_mime_data(part, sTemp.getStr(), sTemp.getLength());
            
            // Set CURL options
            curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/audio/transcriptions");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            
            OStringBuffer responseBuffer;
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
            
            // Timeout settings
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5 minutes max
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
            
            // SSL options
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            
            // Perform request
            SAL_WARN("sw.whisper", "WhisperSession::transcribe - sending request to OpenAI API");
            CURLcode res = curl_easy_perform(curl);
            
            if (m_bCancelled) {
                fnError("Cancelled");
                return;
            }
            
            if (res != CURLE_OK) {
                OUString sError = "CURL error: " + 
                    OUString::fromUtf8(curl_easy_strerror(res));
                fnError(sError);
                return;
            }
            
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            
            OString sResponse = responseBuffer.makeStringAndClear();
            
            SAL_WARN("sw.whisper", "WhisperSession::transcribe - HTTP response code: " << httpCode);
            SAL_WARN("sw.whisper", "WhisperSession::transcribe - Response: " << sResponse);
            
            if (httpCode != 200) {
                OUString sError = "HTTP error " + OUString::number(httpCode) + ": " +
                    OUString::fromUtf8(sResponse);
                fnError(sError);
                return;
            }
            
            // Parse JSON response using boost property tree
            try {
                boost::property_tree::ptree pt;
                std::stringstream ss(sResponse.getStr());
                boost::property_tree::read_json(ss, pt);
                
                // Get the text field
                std::string sText = pt.get<std::string>("text", "");
                
                if (sText.empty()) {
                    // Check if there's an error message
                    std::string sError = pt.get<std::string>("error.message", "");
                    if (!sError.empty()) {
                        fnError("API Error: " + OUString::fromUtf8(sError.c_str()));
                    } else {
                        fnError("Failed to parse response: no text found");
                    }
                } else {
                    SAL_WARN("sw.whisper", "WhisperSession::transcribe - transcription successful: " << sText.c_str());
                    fnSuccess(OUString::fromUtf8(sText.c_str()));
                }
            } catch (const boost::property_tree::json_parser_error& e) {
                fnError("JSON parse error: " + OUString::fromUtf8(e.what()));
            } catch (const std::exception& e) {
                fnError("Error parsing response: " + OUString::fromUtf8(e.what()));
            }
            
        } catch (const std::exception& e) {
            fnError(OUString::fromUtf8(e.what()));
        }
        
        // Cleanup
        curl_easy_cleanup(curl);
        if (headers)
            curl_slist_free_all(headers);
        if (mime)
            curl_mime_free(mime);
    }
};

WhisperSession::WhisperSession(const WhisperConfig& rConfig)
    : m_pImpl(std::make_unique<Impl>(rConfig))
{
}

WhisperSession::~WhisperSession() {
    cancel();
    if (m_pImpl->m_thread.joinable()) {
        m_pImpl->m_thread.join();
    }
}

void WhisperSession::transcribeAsync(
    const std::vector<sal_uInt8>& rAudioData,
    std::function<void(const OUString&)> fnSuccess,
    std::function<void(const OUString&)> fnError)
{
    m_pImpl->m_thread = std::thread([this, rAudioData, fnSuccess, fnError]() {
        m_pImpl->transcribe(rAudioData, fnSuccess, fnError);
    });
}

void WhisperSession::cancel() {
    m_pImpl->m_bCancelled = true;
}

} // namespace sw::whisper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */