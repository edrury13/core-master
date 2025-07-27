/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ustring.hxx>
#include <memory>
#include <vector>
#include <map>
#include <chrono>

namespace gdocs
{

struct FileInfo
{
    OUString id;
    OUString name;
    OUString mimeType;
    OUString webViewLink;
    OUString webContentLink;
    bool isFolder;
    sal_Int64 size;
    std::chrono::system_clock::time_point modifiedTime;
    std::vector<OUString> parents;
};

class Session
{
private:
    OUString m_sUrl;
    OUString m_sUsername;
    OUString m_sAccessToken;
    OUString m_sRefreshToken;
    std::chrono::system_clock::time_point m_aTokenExpiry;
    
    static constexpr const char* GOOGLE_DRIVE_API_BASE = "https://www.googleapis.com/drive/v3";
    
    bool refreshAccessToken();
    OUString makeApiRequest( const OUString& sEndpoint, const OUString& sMethod = "GET",
                            const OUString& sBody = OUString() );

public:
    Session( const OUString& sUrl, const OUString& sUsername );
    ~Session();
    
    void setTokens( const OUString& sAccessToken, const OUString& sRefreshToken,
                   std::chrono::seconds nExpiresIn );
    
    bool isAuthenticated() const;
    
    // Google Drive API operations
    std::vector<FileInfo> listChildren( const OUString& sFolderId = "root" );
    FileInfo getFileInfo( const OUString& sFileId );
    FileInfo createFolder( const OUString& sName, const OUString& sParentId = "root" );
    FileInfo uploadFile( const OUString& sName, const OUString& sMimeType,
                        const std::vector<sal_Int8>& rData, const OUString& sParentId = "root" );
    std::vector<sal_Int8> downloadFile( const OUString& sFileId );
    bool deleteFile( const OUString& sFileId );
    FileInfo updateFile( const OUString& sFileId, const OUString& sNewName = OUString(),
                        const std::vector<sal_Int8>& rNewData = std::vector<sal_Int8>() );
    
    const OUString& getUsername() const { return m_sUsername; }
    const OUString& getUrl() const { return m_sUrl; }
};

} // namespace gdocs

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */