/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ucb/gdocsauth.hxx>
#include <com/sun/star/task/PasswordContainer.hpp>
#include <com/sun/star/task/UrlRecord.hpp>
#include <com/sun/star/uno/Exception.hpp>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <rtl/uri.hxx>
#include <osl/file.hxx>
#include <unotools/bootstrap.hxx>
#include <curl/curl.h>
#include "gdocs_json.hxx"

using namespace com::sun::star;

namespace ucb::gdocs {

namespace {
    const char* GOOGLE_TOKEN_URL = "https://oauth2.googleapis.com/token";
    const OUString GDRIVE_AUTH_URL = u"com.sun.star.auth.gdrive"_ustr;
    
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
    {
        userp->append(static_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }
    
    OUString getGoogleClientId()
    {
        OUString sClientId;
        osl::File aFile(u"${BRAND_BASE_DIR}/program/google_client_id.txt"_ustr);
        if (aFile.open(osl_File_OpenFlag_Read) == osl::FileBase::E_None)
        {
            sal_uInt64 nSize = 0;
            aFile.getSize(nSize);
            if (nSize > 0 && nSize < 1024)
            {
                std::vector<char> buffer(nSize + 1, 0);
                sal_uInt64 nRead = 0;
                aFile.read(buffer.data(), nSize, nRead);
                sClientId = OUString::fromUtf8(std::string_view(buffer.data(), nRead));
                sClientId = sClientId.trim();
            }
            aFile.close();
        }
        return sClientId.isEmpty() ? u"YOUR_CLIENT_ID"_ustr : sClientId;
    }
    
    OUString getGoogleClientSecret()
    {
        OUString sClientSecret;
        osl::File aFile(u"${BRAND_BASE_DIR}/program/google_client_secret.txt"_ustr);
        if (aFile.open(osl_File_OpenFlag_Read) == osl::FileBase::E_None)
        {
            sal_uInt64 nSize = 0;
            aFile.getSize(nSize);
            if (nSize > 0 && nSize < 1024)
            {
                std::vector<char> buffer(nSize + 1, 0);
                sal_uInt64 nRead = 0;
                aFile.read(buffer.data(), nSize, nRead);
                sClientSecret = OUString::fromUtf8(std::string_view(buffer.data(), nRead));
                sClientSecret = sClientSecret.trim();
            }
            aFile.close();
        }
        return sClientSecret.isEmpty() ? u"YOUR_CLIENT_SECRET"_ustr : sClientSecret;
    }
}

GoogleDriveAuthService& GoogleDriveAuthService::getInstance()
{
    static GoogleDriveAuthService instance;
    return instance;
}

void GoogleDriveAuthService::storeTokens(const AuthTokens& tokens)
{
    SAL_INFO("ucb.ucp.gdocs", "Storing Google Drive tokens for user: " << tokens.user_email);
    
    try
    {
        uno::Reference<task::XPasswordContainer2> xContainer = 
            task::PasswordContainer::create(comphelper::getProcessComponentContext());
        
        if (xContainer.is())
        {
            // Store access token
            xContainer->addPersistent(
                GDRIVE_AUTH_URL,
                tokens.user_email,
                tokens.access_token,
                uno::Reference<task::XInteractionHandler>());
            
            // Store refresh token with a different key
            OUString sRefreshUrl = GDRIVE_AUTH_URL + u".refresh"_ustr;
            xContainer->addPersistent(
                sRefreshUrl,
                tokens.user_email,
                tokens.refresh_token,
                uno::Reference<task::XInteractionHandler>());
            
            SAL_INFO("ucb.ucp.gdocs", "Tokens stored successfully");
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Failed to store tokens: " << e.Message);
    }
}

AuthTokens GoogleDriveAuthService::getStoredTokens()
{
    AuthTokens tokens;
    
    try
    {
        uno::Reference<task::XPasswordContainer2> xContainer = 
            task::PasswordContainer::create(comphelper::getProcessComponentContext());
        
        if (xContainer.is())
        {
            // Get all stored credentials for our URL
            uno::Sequence<task::UrlRecord> aRecords = xContainer->getAllPersistent(
                uno::Reference<task::XInteractionHandler>());
            
            for (const auto& record : aRecords)
            {
                if (record.Url == GDRIVE_AUTH_URL && record.UserList.hasElements())
                {
                    // Get the first user's credentials
                    const auto& userRecord = record.UserList[0];
                    tokens.user_email = userRecord.UserName;
                    tokens.access_token = userRecord.Passwords[0];
                    
                    SAL_INFO("ucb.ucp.gdocs", "Found stored tokens for user: " << tokens.user_email);
                }
                else if (record.Url == GDRIVE_AUTH_URL + u".refresh"_ustr && record.UserList.hasElements())
                {
                    // Get refresh token
                    const auto& userRecord = record.UserList[0];
                    tokens.refresh_token = userRecord.Passwords[0];
                }
            }
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Failed to retrieve tokens: " << e.Message);
    }
    
    return tokens;
}

bool GoogleDriveAuthService::hasValidTokens()
{
    AuthTokens tokens = getStoredTokens();
    
    if (tokens.access_token.isEmpty())
        return false;
    
    // Try a simple API call to verify the token
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;
    
    std::string response;
    std::string url = "https://www.googleapis.com/oauth2/v2/userinfo";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    struct curl_slist* headers = nullptr;
    OString authHeader = "Authorization: Bearer " + OUStringToOString(tokens.access_token, RTL_TEXTENCODING_UTF8);
    headers = curl_slist_append(headers, authHeader.getStr());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    bool valid = (res == CURLE_OK && http_code == 200);
    SAL_INFO("ucb.ucp.gdocs", "Token validation result: " << valid);
    
    return valid;
}

bool GoogleDriveAuthService::refreshAccessToken()
{
    AuthTokens tokens = getStoredTokens();
    
    if (tokens.refresh_token.isEmpty())
    {
        SAL_WARN("ucb.ucp.gdocs", "No refresh token available");
        return false;
    }
    
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;
    
    std::string response;
    OString clientId = OUStringToOString(getGoogleClientId(), RTL_TEXTENCODING_UTF8);
    OString clientSecret = OUStringToOString(getGoogleClientSecret(), RTL_TEXTENCODING_UTF8);
    OString refreshToken = OUStringToOString(tokens.refresh_token, RTL_TEXTENCODING_UTF8);
    
    std::string postData = "refresh_token=" + std::string(refreshToken.getStr()) +
                          "&client_id=" + std::string(clientId.getStr()) +
                          "&client_secret=" + std::string(clientSecret.getStr()) +
                          "&grant_type=refresh_token";
    
    curl_easy_setopt(curl, CURLOPT_URL, GOOGLE_TOKEN_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK)
    {
        OString jsonResponse(response.c_str());
        if (!gdocs::json::hasError(jsonResponse))
        {
            tokens.access_token = OUString::fromUtf8(gdocs::json::extractString(jsonResponse, "access_token"));
            tokens.expires_in = gdocs::json::extractInt(jsonResponse, "expires_in");
            
            // Store the new access token
            storeTokens(tokens);
            
            SAL_INFO("ucb.ucp.gdocs", "Successfully refreshed access token");
            return true;
        }
    }
    
    SAL_WARN("ucb.ucp.gdocs", "Failed to refresh access token");
    return false;
}

void GoogleDriveAuthService::clearTokens()
{
    try
    {
        uno::Reference<task::XPasswordContainer2> xContainer = 
            task::PasswordContainer::create(comphelper::getProcessComponentContext());
        
        if (xContainer.is())
        {
            xContainer->removeAllPersistent();
            SAL_INFO("ucb.ucp.gdocs", "All tokens cleared");
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Failed to clear tokens: " << e.Message);
    }
}

} // namespace ucb::gdocs

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */