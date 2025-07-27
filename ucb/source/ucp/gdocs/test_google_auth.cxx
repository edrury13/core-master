/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * Test program for Google Docs authentication with environment variables
 * 
 * This is a simple test to verify that:
 * 1. Environment variables are read correctly
 * 2. Error handling works when variables are not set
 * 3. Authorization URL is built correctly
 * 
 * To compile and run:
 * - Set GOOGLE_CLIENT_ID and GOOGLE_CLIENT_SECRET environment variables
 * - Build LibreOffice with this module
 * - Run the test
 */

#include <iostream>
#include <cstdlib>
#include "gdocs_auth.hxx"
#include <rtl/ustring.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>

using namespace gdocs;

void testEnvironmentVariables()
{
    std::cout << "Testing environment variable reading..." << std::endl;
    
    try
    {
        OUString clientId = getGoogleClientId();
        std::cout << "✓ GOOGLE_CLIENT_ID is set" << std::endl;
        
        // Don't print the actual value for security
        if (!clientId.isEmpty() && clientId.indexOf(".apps.googleusercontent.com") > 0)
        {
            std::cout << "✓ Client ID appears to be in correct format" << std::endl;
        }
        else
        {
            std::cout << "⚠ Client ID format may be incorrect" << std::endl;
        }
    }
    catch (const com::sun::star::uno::RuntimeException& e)
    {
        std::cout << "✗ Error reading GOOGLE_CLIENT_ID: " 
                  << OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() 
                  << std::endl;
    }
    
    try
    {
        OUString clientSecret = getGoogleClientSecret();
        std::cout << "✓ GOOGLE_CLIENT_SECRET is set" << std::endl;
        
        // Just verify it's not empty
        if (!clientSecret.isEmpty())
        {
            std::cout << "✓ Client Secret is not empty" << std::endl;
        }
    }
    catch (const com::sun::star::uno::RuntimeException& e)
    {
        std::cout << "✗ Error reading GOOGLE_CLIENT_SECRET: " 
                  << OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() 
                  << std::endl;
    }
}

void testAuthorizationUrl()
{
    std::cout << "\nTesting authorization URL building..." << std::endl;
    
    try
    {
        OUString testEmail("test@example.com");
        std::string authUrl = buildAuthorizationUrl(testEmail);
        
        if (!authUrl.empty())
        {
            std::cout << "✓ Authorization URL built successfully" << std::endl;
            
            // Check for required parameters
            if (authUrl.find("client_id=") != std::string::npos)
                std::cout << "✓ Contains client_id parameter" << std::endl;
            
            if (authUrl.find("redirect_uri=") != std::string::npos)
                std::cout << "✓ Contains redirect_uri parameter" << std::endl;
                
            if (authUrl.find("scope=") != std::string::npos)
                std::cout << "✓ Contains scope parameter" << std::endl;
                
            if (authUrl.find("login_hint=test%40example.com") != std::string::npos)
                std::cout << "✓ Contains properly encoded login_hint" << std::endl;
        }
    }
    catch (const com::sun::star::uno::RuntimeException& e)
    {
        std::cout << "✗ Error building authorization URL: " 
                  << OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr() 
                  << std::endl;
    }
}

int main()
{
    std::cout << "Google Docs Authentication Test\n" 
              << "==============================\n" << std::endl;
    
    testEnvironmentVariables();
    testAuthorizationUrl();
    
    std::cout << "\nTest completed." << std::endl;
    
    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */