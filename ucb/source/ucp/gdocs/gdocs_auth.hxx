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
#include <string>
#include <memory>
#include <vector>

namespace gdocs
{

// Google OAuth2 configuration
const char* const GOOGLE_AUTH_URL = "https://accounts.google.com/o/oauth2/v2/auth";
const char* const GOOGLE_TOKEN_URL = "https://oauth2.googleapis.com/token";
const char* const GOOGLE_REDIRECT_URI = "urn:ietf:wg:oauth:2.0:oob";

// Helper functions to get OAuth2 credentials from environment
OUString getGoogleClientId();
OUString getGoogleClientSecret();

// Google Drive API scopes
const char* const GOOGLE_DRIVE_SCOPE = "https://www.googleapis.com/auth/drive";
const char* const GOOGLE_DRIVE_FILE_SCOPE = "https://www.googleapis.com/auth/drive.file";

struct GoogleCredentials
{
    std::string access_token;
    std::string refresh_token;
    std::string token_type;
    int expires_in;
    std::string user_email;
};

struct GoogleSession
{
    GoogleCredentials credentials;
    std::string user_email;
    
    bool isValid() const
    {
        return !credentials.access_token.empty();
    }
};

struct GoogleFile
{
    std::string id;
    std::string name;
    std::string mimeType;
    std::string parents;
    std::string modifiedTime;
    std::string createdTime;
    long size;
    bool isFolder;
    
    GoogleFile() : size(0), isFolder(false) {}
};

// Export formats for Google Workspace documents
const char* const GOOGLE_DOCS_MIMETYPE = "application/vnd.google-apps.document";
const char* const GOOGLE_SHEETS_MIMETYPE = "application/vnd.google-apps.spreadsheet";
const char* const GOOGLE_SLIDES_MIMETYPE = "application/vnd.google-apps.presentation";

const char* const EXPORT_DOCX_MIMETYPE = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
const char* const EXPORT_XLSX_MIMETYPE = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
const char* const EXPORT_PPTX_MIMETYPE = "application/vnd.openxmlformats-officedocument.presentationml.presentation";

// Authentication helper functions
std::string buildAuthorizationUrl(const OUString& userEmail);
std::shared_ptr<GoogleCredentials> exchangeAuthCodeForToken(const std::string& authCode);
std::shared_ptr<GoogleCredentials> refreshAccessToken(const std::string& refreshToken);
std::shared_ptr<GoogleSession> createGoogleSession(const OUString& userEmail);

// Google Drive API helper functions
std::vector<GoogleFile> listFiles(const GoogleSession& session, const std::string& folderId = "");
std::shared_ptr<GoogleFile> getFile(const GoogleSession& session, const std::string& fileId);
bool downloadFile(const GoogleSession& session, const std::string& fileId, const std::string& destPath);
std::string uploadFile(const GoogleSession& session, const std::string& filePath, 
                      const std::string& fileName, const std::string& mimeType,
                      const std::string& parentId = "");
bool updateFile(const GoogleSession& session, const std::string& fileId, 
                const std::string& filePath, const std::string& mimeType);
bool deleteFile(const GoogleSession& session, const std::string& fileId);
std::string createFolder(const GoogleSession& session, const std::string& folderName, 
                        const std::string& parentId = "");

// Export Google Workspace document as Office format
bool exportGoogleDocument(const GoogleSession& session, const std::string& fileId, 
                         const std::string& exportMimeType, const std::string& destPath);
                         
// Helper to export Google Doc as DOCX to memory
std::vector<char> exportGoogleDocToMemory(const GoogleSession& session, const std::string& fileId,
                                          const std::string& exportMimeType);

// Google Drive API methods for file browsing
struct DriveItem
{
    OUString id;
    OUString name;
    OUString mimeType;
    OUString modifiedTime;
    OUString iconLink;
    bool isFolder;
    
    DriveItem() : isFolder(false) {}
};

// List files in a folder
std::vector<DriveItem> listFolderContents(
    const GoogleSession& session,
    const OUString& folderId = "root");

// Search for files
std::vector<DriveItem> searchFiles(
    const GoogleSession& session,
    const OUString& query);

// Get file metadata
DriveItem getFileMetadata(
    const GoogleSession& session,
    const OUString& fileId);

// Get current user info
OUString getCurrentUserEmail(const GoogleSession& session);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */