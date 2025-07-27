/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/weld.hxx>
#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <vector>
#include <memory>

namespace com::sun::star::uno { class XComponentContext; }

struct GoogleDriveItem
{
    OUString id;
    OUString name;
    OUString mimeType;
    OUString modifiedTime;
    OUString iconLink;
    sal_Int64 size;
    bool isFolder;
    
    GoogleDriveItem() : size(0), isFolder(false) {}
};

class GoogleDriveFilePicker : public weld::GenericDialogController
{
private:
    // UI elements
    std::unique_ptr<weld::Entry> m_xSearchEntry;
    std::unique_ptr<weld::Button> m_xSearchButton;
    std::unique_ptr<weld::TreeView> m_xFolderTree;
    std::unique_ptr<weld::TreeView> m_xFileList;
    std::unique_ptr<weld::ComboBox> m_xFilterCombo;
    std::unique_ptr<weld::Label> m_xPathLabel;
    std::unique_ptr<weld::Button> m_xOpenButton;
    std::unique_ptr<weld::Button> m_xCancelButton;
    std::unique_ptr<weld::Button> m_xAuthButton;
    std::unique_ptr<weld::Label> m_xAuthStatus;
    std::unique_ptr<weld::Box> m_xAuthUrlBox;
    std::unique_ptr<weld::TextView> m_xAuthUrlText;
    std::unique_ptr<weld::Entry> m_xAuthCodeEntry;
    std::unique_ptr<weld::Button> m_xAuthSubmitButton;
    
    // Data
    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    std::vector<GoogleDriveItem> m_aCurrentItems;
    std::vector<OUString> m_aFolderStack;
    OUString m_sSelectedFileId;
    OUString m_sSelectedFileUrl;
    bool m_bIsAuthenticated;
    
    // OAuth data
    OUString m_sClientId;
    OUString m_sClientSecret;
    OUString m_sAccessToken;
    OUString m_sRefreshToken;
    OUString m_sUserEmail;
    OUString m_sAuthCode;
    sal_Int32 m_nTokenExpiry;
    
    // Methods
    void LoadRootFolder();
    void LoadFolderContents(const OUString& folderId);
    void RefreshFileList();
    void UpdatePathLabel();
    void CheckAuthentication();
    void PerformSearch();
    
    // OAuth methods
    void StartOAuthFlow();
    bool ExchangeCodeForToken(const OUString& sCode);
    bool RefreshAccessToken();
    bool IsTokenValid();
    void LoadStoredTokens();
    void SaveStoredTokens();
    
    // HTTP methods
    OUString PerformHttpGet(const OUString& sUrl);
    OUString PerformHttpPost(const OUString& sUrl, const OUString& sPostData);
    static OUString EncodeURLComponent(const OUString& sComponent);
    
    // JSON parsing
    void ParseFileListResponse(const OUString& sResponse);
    void ParseTokenResponse(const OUString& sResponse);
    void ParseUserInfoResponse(const OUString& sResponse);
    
    // Helper methods
    OUString GetRecentDate();
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    OUString ExtractJsonString(const OUString& sJson, const OUString& sKey);
    sal_Int32 FindMatchingBracket(const OUString& sStr, sal_Int32 nStart, sal_Unicode cOpen, sal_Unicode cClose);
    
    // Download/Upload methods
    OUString DownloadFile(const OUString& fileId, const OUString& fileName, 
                         const OUString& mimeType);
    bool UploadFile(const OUString& localPath, const OUString& fileId,
                    const OUString& mimeType);
    
    // Event handlers
    DECL_LINK(SearchHdl, weld::Button&, void);
    DECL_LINK(FolderSelectHdl, weld::TreeView&, void);
    DECL_LINK(FileSelectHdl, weld::TreeView&, void);
    DECL_LINK(FileDoubleClickHdl, weld::TreeView&, bool);
    DECL_LINK(FilterChangedHdl, weld::ComboBox&, void);
    DECL_LINK(OpenHdl, weld::Button&, void);
    DECL_LINK(AuthenticateHdl, weld::Button&, void);
    DECL_LINK(AuthSubmitHdl, weld::Button&, void);
    
public:
    GoogleDriveFilePicker(weld::Window* pParent, 
                         const css::uno::Reference<css::uno::XComponentContext>& xContext);
    virtual ~GoogleDriveFilePicker() override;
    
    OUString GetSelectedFileUrl() const { return m_sSelectedFileUrl; }
    OUString GetSelectedFileId() const { return m_sSelectedFileId; }
    
    // Static helper for uploading files back to Google Drive
    static bool UploadFileToGoogleDrive(const OUString& localPath, const OUString& fileId,
                                        const OUString& mimeType);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */