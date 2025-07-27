/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_UCB_GDOCSAUTH_HXX
#define INCLUDED_UCB_GDOCSAUTH_HXX

#include <rtl/ustring.hxx>
#include <memory>

namespace ucb::gdocs {

struct AuthTokens
{
    OUString access_token;
    OUString refresh_token;
    OUString user_email;
    sal_Int32 expires_in = 0;
};

class GoogleDriveAuthService
{
public:
    static GoogleDriveAuthService& getInstance();
    
    // Store tokens in secure storage
    void storeTokens(const AuthTokens& tokens);
    
    // Retrieve tokens from secure storage
    AuthTokens getStoredTokens();
    
    // Check if we have valid stored tokens
    bool hasValidTokens();
    
    // Refresh access token using refresh token
    bool refreshAccessToken();
    
    // Clear all stored tokens (sign out)
    void clearTokens();
    
private:
    GoogleDriveAuthService() = default;
    ~GoogleDriveAuthService() = default;
    
    // Use LibreOffice's password container for secure storage
    void storeInPasswordContainer(const AuthTokens& tokens);
    AuthTokens retrieveFromPasswordContainer();
};

} // namespace ucb::gdocs

#endif // INCLUDED_UCB_GDOCSAUTH_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */