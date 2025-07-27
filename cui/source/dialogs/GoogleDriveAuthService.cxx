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
#include <com/sun/star/task/XPasswordContainer2.hpp>
#include <com/sun/star/task/UrlRecord.hpp>
#include <comphelper/processfactory.hxx>
#include <sal/log.hxx>
#include <o3tl/environment.hxx>
#include <curl/curl.h>
#include <rtl/strbuf.hxx>

using namespace com::sun::star;

namespace ucb::gdocs {

namespace {
    const OUString GDRIVE_AUTH_URL = u"https://www.google.com/drive/auth"_ustr;
    
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t realsize = size * nmemb;
        OStringBuffer* pBuffer = static_cast<OStringBuffer*>(userp);
        pBuffer->append(static_cast<const char*>(contents), realsize);
        return realsize;
    }
    
    OUString ExtractJsonString(const OUString& sJson, const OUString& sKey)
    {
        OUString sSearch = "\"" + sKey + "\":";
        sal_Int32 nKeyPos = sJson.indexOf(sSearch);
        if (nKeyPos == -1)
            return OUString();
            
        sal_Int32 nValueStart = nKeyPos + sSearch.getLength();
        
        // Skip whitespace
        while (nValueStart < sJson.getLength() && 
               (sJson[nValueStart] == ' ' || sJson[nValueStart] == '\t'))
            nValueStart++;
            
        if (nValueStart >= sJson.getLength())
            return OUString();
            
        // Check if it's a string value
        if (sJson[nValueStart] == '"')
        {
            nValueStart++;
            sal_Int32 nValueEnd = sJson.indexOf('"', nValueStart);
            if (nValueEnd == -1)
                return OUString();
            return sJson.copy(nValueStart, nValueEnd - nValueStart);
        }
        else
        {
            // Numeric value
            sal_Int32 nValueEnd = nValueStart;
            while (nValueEnd < sJson.getLength() && 
                   sJson[nValueEnd] != ',' && 
                   sJson[nValueEnd] != '}')
                nValueEnd++;
            return sJson.copy(nValueStart, nValueEnd - nValueStart).trim();
        }
    }
}

GoogleDriveAuthService& GoogleDriveAuthService::getInstance()
{
    static GoogleDriveAuthService aInstance;
    return aInstance;
}

void GoogleDriveAuthService::storeTokens(const AuthTokens& tokens)
{
    try
    {
        storeInPasswordContainer(tokens);
        SAL_INFO("ucb.gdocs", "Tokens stored successfully");
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.gdocs", "Failed to store tokens: " << e.Message);
    }
}

AuthTokens GoogleDriveAuthService::getStoredTokens()
{
    try
    {
        return retrieveFromPasswordContainer();
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.gdocs", "Failed to retrieve tokens: " << e.Message);
        return AuthTokens();
    }
}

bool GoogleDriveAuthService::hasValidTokens()
{
    AuthTokens tokens = getStoredTokens();
    return !tokens.access_token.isEmpty();
}

bool GoogleDriveAuthService::refreshAccessToken()
{
    AuthTokens tokens = getStoredTokens();
    if (tokens.refresh_token.isEmpty())
    {
        SAL_WARN("ucb.gdocs", "No refresh token available");
        return false;
    }
    
    OUString sClientId = o3tl::getEnvironment(u"GOOGLE_CLIENT_ID"_ustr);
    OUString sClientSecret = o3tl::getEnvironment(u"GOOGLE_CLIENT_SECRET"_ustr);
    
    if (sClientId.isEmpty() || sClientSecret.isEmpty())
    {
        SAL_WARN("ucb.gdocs", "Google OAuth credentials not configured");
        return false;
    }
    
    // Prepare refresh request
    OUString sPostData = "client_id=" + sClientId +
                        "&client_secret=" + sClientSecret +
                        "&refresh_token=" + tokens.refresh_token +
                        "&grant_type=refresh_token";
    
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;
        
    OStringBuffer sResponse;
    OString sPostDataUtf8 = OUStringToOString(sPostData, RTL_TEXTENCODING_UTF8);
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostDataUtf8.getStr());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sResponse);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK)
    {
        SAL_WARN("ucb.gdocs", "Token refresh failed: " << curl_easy_strerror(res));
        return false;
    }
    
    OUString sResponseStr = OStringToOUString(sResponse, RTL_TEXTENCODING_UTF8);
    OUString sNewAccessToken = ExtractJsonString(sResponseStr, "access_token");
    
    if (sNewAccessToken.isEmpty())
    {
        SAL_WARN("ucb.gdocs", "Failed to extract new access token from response");
        return false;
    }
    
    // Update tokens
    tokens.access_token = sNewAccessToken;
    
    OUString sExpiresIn = ExtractJsonString(sResponseStr, "expires_in");
    if (!sExpiresIn.isEmpty())
        tokens.expires_in = sExpiresIn.toInt32();
        
    storeTokens(tokens);
    
    SAL_INFO("ucb.gdocs", "Access token refreshed successfully");
    return true;
}

void GoogleDriveAuthService::clearTokens()
{
    try
    {
        uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
        uno::Reference<task::XPasswordContainer2> xPasswordContainer = 
            task::PasswordContainer::create(xContext);
            
        xPasswordContainer->removeUrl(GDRIVE_AUTH_URL);
        SAL_INFO("ucb.gdocs", "Tokens cleared successfully");
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.gdocs", "Failed to clear tokens: " << e.Message);
    }
}

void GoogleDriveAuthService::storeInPasswordContainer(const AuthTokens& tokens)
{
    uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
    uno::Reference<task::XPasswordContainer2> xPasswordContainer = 
        task::PasswordContainer::create(xContext);
        
    // Store tokens as username/password pairs
    uno::Sequence<OUString> aPasswords{
        tokens.access_token,
        tokens.refresh_token,
        OUString::number(tokens.expires_in)
    };
    
    xPasswordContainer->addPersistent(
        GDRIVE_AUTH_URL,
        tokens.user_email.isEmpty() ? "default" : tokens.user_email,
        aPasswords,
        uno::Reference<task::XInteractionHandler>()
    );
}

AuthTokens GoogleDriveAuthService::retrieveFromPasswordContainer()
{
    AuthTokens tokens;
    
    uno::Reference<uno::XComponentContext> xContext = comphelper::getProcessComponentContext();
    uno::Reference<task::XPasswordContainer2> xPasswordContainer = 
        task::PasswordContainer::create(xContext);
        
    try
    {
        // Get all URLs stored in password container
        uno::Sequence<task::UrlRecord> aRecords = xPasswordContainer->getAllPersistent(
            uno::Reference<task::XInteractionHandler>()
        );
        
        // Find our URL
        for (const auto& aRecord : aRecords)
        {
            if (aRecord.Url == GDRIVE_AUTH_URL && aRecord.UserList.hasElements())
            {
                // Use the first user found
                if (aRecord.UserList[0].Passwords.getLength() >= 2)
                {
                    tokens.user_email = aRecord.UserList[0].UserName;
                    tokens.access_token = aRecord.UserList[0].Passwords[0];
                    tokens.refresh_token = aRecord.UserList[0].Passwords[1];
                    
                    if (aRecord.UserList[0].Passwords.getLength() >= 3)
                        tokens.expires_in = aRecord.UserList[0].Passwords[2].toInt32();
                }
                break;
            }
        }
    }
    catch (const uno::Exception&)
    {
        // No stored tokens
    }
    
    return tokens;
}

} // namespace ucb::gdocs

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */