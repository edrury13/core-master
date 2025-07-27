/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "GoogleDriveFilePicker.hxx"
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/ucb/XContentAccess.hpp>
#include <com/sun/star/ucb/XContentProvider.hpp>
#include <com/sun/star/ucb/XContentProviderManager.hpp>
#include <com/sun/star/ucb/XUniversalContentBroker.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/task/InteractionHandler.hpp>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/system/SystemShellExecute.hpp>
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#include <ucbhelper/content.hxx>
#include <ucbhelper/commandenvironment.hxx>
#include <ucbhelper/simpleauthenticationrequest.hxx>
#include <tools/urlobj.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <sal/log.hxx>
#include <o3tl/environment.hxx>
#include <comphelper/processfactory.hxx>
#include <officecfg/Office/Common.hxx>
#include <osl/file.hxx>
#include <unotools/configmgr.hxx>
#include <thread>
#include <future>
#include <curl/curl.h>
#include <rtl/strbuf.hxx>
#include <osl/file.hxx>
#include <osl/security.hxx>
#include <cstdio>
#include <ucb/gdocsauth.hxx>
#include <sfx2/gdrivesync.hxx>
#include <unotools/tempfile.hxx>

using namespace com::sun::star;

GoogleDriveFilePicker::GoogleDriveFilePicker(weld::Window* pParent,
                                           const uno::Reference<uno::XComponentContext>& xContext)
    : GenericDialogController(pParent, "cui/ui/googledrivepicker.ui", "GoogleDriveDialog")
    , m_xSearchEntry(m_xBuilder->weld_entry("searchEntry"))
    , m_xSearchButton(m_xBuilder->weld_button("searchButton"))
    , m_xFolderTree(m_xBuilder->weld_tree_view("folderTree"))
    , m_xFileList(m_xBuilder->weld_tree_view("fileList"))
    , m_xFilterCombo(m_xBuilder->weld_combo_box("filterCombo"))
    , m_xPathLabel(m_xBuilder->weld_label("pathLabel"))
    , m_xOpenButton(m_xBuilder->weld_button("ok"))
    , m_xCancelButton(m_xBuilder->weld_button("cancel"))
    , m_xAuthButton(m_xBuilder->weld_button("authButton"))
    , m_xAuthStatus(m_xBuilder->weld_label("authStatus"))
    , m_xAuthUrlBox(m_xBuilder->weld_box("authUrlBox"))
    , m_xAuthUrlText(m_xBuilder->weld_text_view("authUrlText"))
    , m_xAuthCodeEntry(m_xBuilder->weld_entry("authCodeEntry"))
    , m_xAuthSubmitButton(m_xBuilder->weld_button("authSubmitButton"))
    , m_xContext(xContext)
    , m_bIsAuthenticated(false)
    , m_nTokenExpiry(0)
{
    SAL_WARN("cui.dialogs", "\n\n=== GoogleDriveFilePicker CONSTRUCTOR CALLED ===\n\n");
    fprintf(stderr, "\n\n*** GoogleDriveFilePicker CONSTRUCTOR - If you see this, code is executing ***\n\n");
    
    // Write to a debug file to ensure we're executing
    FILE* debugFile = fopen("/tmp/gdrive_debug.log", "a");
    if (debugFile) {
        fprintf(debugFile, "\n=== GoogleDriveFilePicker CONSTRUCTOR at %s ===\n", __TIME__);
        fprintf(debugFile, "Dialog created successfully\n");
        fclose(debugFile);
    }
    
    // Set up UI
    m_xFileList->set_size_request(400, 300);
    m_xFolderTree->set_size_request(150, 300);
    
    // Add columns to file list
    std::vector<int> aWidths = { 200, 80, 100 };
    m_xFileList->set_column_fixed_widths(aWidths);
    
    // Set up filters
    m_xFilterCombo->append_text("All Files");
    m_xFilterCombo->append_text("Documents");
    m_xFilterCombo->append_text("Spreadsheets");
    m_xFilterCombo->append_text("Presentations");
    m_xFilterCombo->set_active(0);
    
    // Connect signals
    m_xSearchButton->connect_clicked(LINK(this, GoogleDriveFilePicker, SearchHdl));
    m_xFolderTree->connect_selection_changed(LINK(this, GoogleDriveFilePicker, FolderSelectHdl));
    m_xFileList->connect_selection_changed(LINK(this, GoogleDriveFilePicker, FileSelectHdl));
    m_xFileList->connect_row_activated(LINK(this, GoogleDriveFilePicker, FileDoubleClickHdl));
    m_xFilterCombo->connect_changed(LINK(this, GoogleDriveFilePicker, FilterChangedHdl));
    m_xOpenButton->connect_clicked(LINK(this, GoogleDriveFilePicker, OpenHdl));
    m_xAuthButton->connect_clicked(LINK(this, GoogleDriveFilePicker, AuthenticateHdl));
    m_xAuthSubmitButton->connect_clicked(LINK(this, GoogleDriveFilePicker, AuthSubmitHdl));
    
    // Initially disable open button
    m_xOpenButton->set_sensitive(false);
    
    // Check authentication status
    CheckAuthentication();
    
    // If authenticated, load root folder
    if (m_bIsAuthenticated)
    {
        LoadRootFolder();
    }
}

GoogleDriveFilePicker::~GoogleDriveFilePicker()
{
}

void GoogleDriveFilePicker::CheckAuthentication()
{
    SAL_WARN("cui.dialogs", "CheckAuthentication called");
    // Check if we have OAuth credentials set
    m_sClientId = o3tl::getEnvironment(u"GOOGLE_CLIENT_ID"_ustr);
    m_sClientSecret = o3tl::getEnvironment(u"GOOGLE_CLIENT_SECRET"_ustr);
    SAL_WARN("cui.dialogs", "Client ID from env: " << (m_sClientId.isEmpty() ? "EMPTY" : "SET"));
    SAL_WARN("cui.dialogs", "Client Secret from env: " << (m_sClientSecret.isEmpty() ? "EMPTY" : "SET"));
    
    if (!m_sClientId.isEmpty() && !m_sClientSecret.isEmpty())
    {
        // Use unified auth service
        SAL_WARN("cui.dialogs", "Checking authentication with unified service");
        auto& authService = ::ucb::gdocs::GoogleDriveAuthService::getInstance();
        
        if (authService.hasValidTokens())
        {
            auto tokens = authService.getStoredTokens();
            m_sAccessToken = tokens.access_token;
            m_sRefreshToken = tokens.refresh_token;
            m_sUserEmail = tokens.user_email;
            
            SAL_WARN("cui.dialogs", "Valid tokens found from auth service");
            m_bIsAuthenticated = true;
            m_xAuthStatus->set_label("Authenticated as: " + m_sUserEmail);
            m_xAuthButton->set_label("Sign out");
        }
        else if (!authService.getStoredTokens().refresh_token.isEmpty())
        {
            // Try to refresh the token
            SAL_WARN("cui.dialogs", "Token expired, trying to refresh");
            if (authService.refreshAccessToken())
            {
                auto tokens = authService.getStoredTokens();
                m_sAccessToken = tokens.access_token;
                m_sRefreshToken = tokens.refresh_token;
                m_sUserEmail = tokens.user_email;
                
                SAL_WARN("cui.dialogs", "Token refresh successful");
                m_bIsAuthenticated = true;
                m_xAuthStatus->set_label("Authenticated as: " + m_sUserEmail);
                m_xAuthButton->set_label("Sign out");
            }
            else
            {
                SAL_WARN("cui.dialogs", "Token refresh failed");
                m_bIsAuthenticated = false;
                m_xAuthStatus->set_label("Authentication expired");
                m_xAuthButton->set_label("Sign in with Google");
            }
        }
        else
        {
            SAL_WARN("cui.dialogs", "No stored tokens found");
            m_bIsAuthenticated = false;
            m_xAuthStatus->set_label("Not authenticated");
            m_xAuthButton->set_label("Sign in with Google");
        }
    }
    else
    {
        m_xAuthStatus->set_label("Google API credentials not configured");
        m_xAuthButton->set_sensitive(false);
    }
}

void GoogleDriveFilePicker::LoadRootFolder()
{
    SAL_WARN("cui.dialogs", "LoadRootFolder called");
    // Clear folder tree
    m_xFolderTree->clear();
    
    // Add root folders
    m_xFolderTree->append(u"root"_ustr, u"My Drive"_ustr);
    m_xFolderTree->append(u"shared"_ustr, u"Shared with me"_ustr);
    m_xFolderTree->append(u"recent"_ustr, u"Recent"_ustr);
    
    // Select My Drive by default
    m_xFolderTree->select(0);
    
    // Load My Drive contents
    LoadFolderContents("root");
}

void GoogleDriveFilePicker::LoadFolderContents(const OUString& folderId)
{
    SAL_WARN("cui.dialogs", "GoogleDriveFilePicker::LoadFolderContents() called with folderId: " << folderId);
    m_aCurrentItems.clear();
    
    if (!m_bIsAuthenticated)
    {
        SAL_WARN("cui.dialogs", "Not authenticated, skipping folder load");
        RefreshFileList();
        UpdatePathLabel();
        return;
    }
    
    try
    {
        // Create Google Drive API request
        OUString sUrl = "https://www.googleapis.com/drive/v3/files";
        OUString sQuery;
        
        if (folderId == "root" || folderId.isEmpty())
        {
            sQuery = "'root' in parents and trashed = false";
        }
        else if (folderId == "shared")
        {
            sQuery = "sharedWithMe and trashed = false";
        }
        else if (folderId == "recent")
        {
            // Get files modified in last 7 days
            sQuery = "trashed = false and modifiedTime > '" + GetRecentDate() + "'";
        }
        else
        {
            sQuery = "'" + folderId + "' in parents and trashed = false";
        }
        
        sUrl += "?q=" + EncodeURLComponent(sQuery);
        sUrl += "&fields=files(id,name,mimeType,modifiedTime,size,iconLink,parents)";
        sUrl += "&orderBy=folder,name";
        sUrl += "&pageSize=1000";
        
        SAL_WARN("cui.dialogs", "Making Google Drive API request to: " << sUrl);
        SAL_WARN("cui.dialogs", "Access token available: " << (!m_sAccessToken.isEmpty() ? "YES" : "NO"));
        SAL_WARN("cui.dialogs", "Access token length: " << m_sAccessToken.getLength());
        
        OUString sResponse = PerformHttpGet(sUrl);
        SAL_WARN("cui.dialogs", "API Response length: " << sResponse.getLength());
        
        if (!sResponse.isEmpty())
        {
            ParseFileListResponse(sResponse);
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("cui.dialogs", "Failed to load folder contents: " << e.Message);
    }
    
    SAL_WARN("cui.dialogs", "LoadFolderContents completed with " << m_aCurrentItems.size() << " items");
    
    RefreshFileList();
    UpdatePathLabel();
}

void GoogleDriveFilePicker::RefreshFileList()
{
    m_xFileList->clear();
    
    OUString filterType = m_xFilterCombo->get_active_text();
    
    for (const auto& item : m_aCurrentItems)
    {
        // Apply filter
        if (filterType != "All Files")
        {
            if (filterType == "Documents" && 
                item.mimeType.indexOf("document") == -1 &&
                item.mimeType.indexOf("text") == -1)
                continue;
            if (filterType == "Spreadsheets" && 
                item.mimeType.indexOf("spreadsheet") == -1)
                continue;
            if (filterType == "Presentations" && 
                item.mimeType.indexOf("presentation") == -1)
                continue;
        }
        
        // Add to list
        OUString sType = item.isFolder ? u"Folder"_ustr : u"File"_ustr;
        m_xFileList->append(item.id, item.name);
        m_xFileList->set_text(m_xFileList->n_children() - 1, sType, 1);
        m_xFileList->set_text(m_xFileList->n_children() - 1, item.modifiedTime, 2);
    }
}

void GoogleDriveFilePicker::UpdatePathLabel()
{
    OUString sPath = "My Drive";
    if (!m_aFolderStack.empty())
    {
        for (const auto& folder : m_aFolderStack)
        {
            sPath += " > " + folder;
        }
    }
    m_xPathLabel->set_label(sPath);
}

void GoogleDriveFilePicker::PerformSearch()
{
    OUString searchTerm = m_xSearchEntry->get_text();
    if (searchTerm.isEmpty())
    {
        RefreshFileList();
        return;
    }
    
    // TODO: Implement actual search using Google Drive API
    // For now, just filter current items
    m_xFileList->clear();
    
    for (const auto& item : m_aCurrentItems)
    {
        if (item.name.toAsciiLowerCase().indexOf(searchTerm.toAsciiLowerCase()) != -1)
        {
            OUString sType = item.isFolder ? u"Folder"_ustr : u"File"_ustr;
            m_xFileList->append(item.id, item.name);
            m_xFileList->set_text(m_xFileList->n_children() - 1, sType, 1);
            m_xFileList->set_text(m_xFileList->n_children() - 1, item.modifiedTime, 2);
        }
    }
}

IMPL_LINK_NOARG(GoogleDriveFilePicker, SearchHdl, weld::Button&, void)
{
    PerformSearch();
}

IMPL_LINK_NOARG(GoogleDriveFilePicker, FolderSelectHdl, weld::TreeView&, void)
{
    OUString selectedId = m_xFolderTree->get_selected_id();
    if (!selectedId.isEmpty())
    {
        LoadFolderContents(selectedId);
    }
}

IMPL_LINK_NOARG(GoogleDriveFilePicker, FileSelectHdl, weld::TreeView&, void)
{
    int nSelected = m_xFileList->get_selected_index();
    if (nSelected != -1)
    {
        m_sSelectedFileId = m_xFileList->get_id(nSelected);
        
        // Find the selected item
        for (const auto& item : m_aCurrentItems)
        {
            if (item.id == m_sSelectedFileId)
            {
                m_xOpenButton->set_sensitive(!item.isFolder);
                if (!item.isFolder)
                {
                    // Store file info for later download
                    m_sSelectedFileId = item.id;
                    // URL will be set after download
                    m_sSelectedFileUrl.clear();
                }
                break;
            }
        }
    }
    else
    {
        m_xOpenButton->set_sensitive(false);
    }
}

IMPL_LINK_NOARG(GoogleDriveFilePicker, FileDoubleClickHdl, weld::TreeView&, bool)
{
    int nSelected = m_xFileList->get_selected_index();
    if (nSelected != -1)
    {
        OUString selectedId = m_xFileList->get_id(nSelected);
        
        // Find the selected item
        for (const auto& item : m_aCurrentItems)
        {
            if (item.id == selectedId)
            {
                if (item.isFolder)
                {
                    // Navigate into folder
                    m_aFolderStack.push_back(item.name);
                    LoadFolderContents(item.id);
                }
                else
                {
                    // Download file and open
                    m_sSelectedFileId = item.id;
                    OUString sTempPath = DownloadFile(item.id, item.name, item.mimeType);
                    if (!sTempPath.isEmpty())
                    {
                        m_sSelectedFileUrl = sTempPath;
                        m_xDialog->response(RET_OK);
                    }
                    else
                    {
                        // Show error message
                        std::unique_ptr<weld::MessageDialog> xErrorBox(Application::CreateMessageDialog(m_xDialog.get(),
                            VclMessageType::Error, VclButtonsType::Ok,
                            "Failed to download file from Google Drive"));
                        xErrorBox->run();
                    }
                }
                break;
            }
        }
    }
    return true;
}

IMPL_LINK_NOARG(GoogleDriveFilePicker, FilterChangedHdl, weld::ComboBox&, void)
{
    RefreshFileList();
}

IMPL_LINK_NOARG(GoogleDriveFilePicker, OpenHdl, weld::Button&, void)
{
    // Download the selected file if not already downloaded
    if (m_sSelectedFileUrl.isEmpty() && !m_sSelectedFileId.isEmpty())
    {
        // Find the selected item to get its details
        for (const auto& item : m_aCurrentItems)
        {
            if (item.id == m_sSelectedFileId)
            {
                OUString sTempPath = DownloadFile(item.id, item.name, item.mimeType);
                if (!sTempPath.isEmpty())
                {
                    m_sSelectedFileUrl = sTempPath;
                    m_xDialog->response(RET_OK);
                }
                else
                {
                    // Show error message
                    std::unique_ptr<weld::MessageDialog> xErrorBox(Application::CreateMessageDialog(m_xDialog.get(),
                        VclMessageType::Error, VclButtonsType::Ok,
                        "Failed to download file from Google Drive"));
                    xErrorBox->run();
                }
                return;
            }
        }
    }
    
    m_xDialog->response(RET_OK);
}

IMPL_LINK_NOARG(GoogleDriveFilePicker, AuthenticateHdl, weld::Button&, void)
{
    if (m_bIsAuthenticated)
    {
        // Sign out using unified auth service
        auto& authService = ::ucb::gdocs::GoogleDriveAuthService::getInstance();
        authService.clearTokens();
        
        m_sAccessToken.clear();
        m_sRefreshToken.clear();
        m_sUserEmail.clear();
        m_bIsAuthenticated = false;
        
        m_xAuthStatus->set_label("Not authenticated");
        m_xAuthButton->set_label("Sign in with Google");
        
        m_aCurrentItems.clear();
        RefreshFileList();
    }
    else
    {
        // Start OAuth flow
        StartOAuthFlow();
    }
}

IMPL_LINK_NOARG(GoogleDriveFilePicker, AuthSubmitHdl, weld::Button&, void)
{
    SAL_WARN("cui.dialogs", "AuthSubmitHdl called - user submitted auth code");
    OUString sAuthCode = m_xAuthCodeEntry->get_text().trim();
    SAL_WARN("cui.dialogs", "Auth code received, length: " << sAuthCode.getLength());
    if (!sAuthCode.isEmpty())
    {
        // Clear the code field immediately (codes are single-use)
        m_xAuthCodeEntry->set_text("");
        
        SAL_WARN("cui.dialogs", "Calling ExchangeCodeForToken");
        if (ExchangeCodeForToken(sAuthCode))
        {
            SAL_WARN("cui.dialogs", "Token exchange successful");
            
            // Save tokens using unified auth service
            auto& authService = ::ucb::gdocs::GoogleDriveAuthService::getInstance();
            ::ucb::gdocs::AuthTokens tokens;
            tokens.access_token = m_sAccessToken;
            tokens.refresh_token = m_sRefreshToken;
            tokens.user_email = m_sUserEmail;
            authService.storeTokens(tokens);
            
            // Hide auth URL box and update status
            m_xAuthUrlBox->set_visible(false);
            m_bIsAuthenticated = true;
            m_xAuthStatus->set_label("Authenticated as: " + m_sUserEmail);
            m_xAuthButton->set_label("Sign out");
            
            // Load root folder
            LoadRootFolder();
        }
        else
        {
            SAL_WARN("cui.dialogs", "Token exchange failed");
            // Show error
            m_xAuthStatus->set_label("Authentication failed. Get a new code and try again.");
            // Keep the URL box visible so user can get a new code
        }
    }
}

// OAuth implementation
void GoogleDriveFilePicker::StartOAuthFlow()
{
    SAL_WARN("cui.dialogs", "GoogleDriveFilePicker::StartOAuthFlow() called");
    try
    {
        // Build authorization URL
        OUString sAuthUrl = "https://accounts.google.com/o/oauth2/v2/auth";
        sAuthUrl += "?client_id=" + EncodeURLComponent(m_sClientId);
        sAuthUrl += "&redirect_uri=urn:ietf:wg:oauth:2.0:oob";
        sAuthUrl += "&response_type=code";
        sAuthUrl += "&scope=" + EncodeURLComponent("https://www.googleapis.com/auth/drive.readonly https://www.googleapis.com/auth/userinfo.email");
        sAuthUrl += "&access_type=offline";
        sAuthUrl += "&prompt=consent";
        
        // Show the URL in the UI
        m_xAuthUrlText->set_text(sAuthUrl);
        m_xAuthUrlBox->set_visible(true);
        m_xAuthCodeEntry->set_text("");
        m_xAuthCodeEntry->grab_focus();
        
        // Try to open browser
        try
        {
            uno::Reference<system::XSystemShellExecute> xSystemShell(
                system::SystemShellExecute::create(m_xContext));
            xSystemShell->execute(sAuthUrl, OUString(), 
                system::SystemShellExecuteFlags::URIS_ONLY);
        }
        catch (...)
        {
            // Browser launch failed - not a problem, user can copy URL manually
            SAL_WARN("cui.dialogs", "Failed to open browser for OAuth - user can copy URL manually");
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("cui.dialogs", "OAuth flow failed: " << e.Message);
        std::unique_ptr<weld::MessageDialog> xErrorBox(
            Application::CreateMessageDialog(m_xDialog.get(),
                VclMessageType::Error, VclButtonsType::Ok,
                u"Authentication failed: "_ustr + e.Message));
        xErrorBox->run();
    }
}

bool GoogleDriveFilePicker::ExchangeCodeForToken(const OUString& sCode)
{
    SAL_WARN("cui.dialogs", "ExchangeCodeForToken called with code length: " << sCode.getLength());
    OUString sUrl = "https://oauth2.googleapis.com/token";
    OUString sPostData = "code=" + EncodeURLComponent(sCode);
    sPostData += "&client_id=" + EncodeURLComponent(m_sClientId);
    sPostData += "&client_secret=" + EncodeURLComponent(m_sClientSecret);
    sPostData += "&redirect_uri=urn:ietf:wg:oauth:2.0:oob";
    sPostData += "&grant_type=authorization_code";
    
    SAL_WARN("cui.dialogs", "Exchanging auth code for token...");
    SAL_WARN("cui.dialogs", "URL: " << sUrl);
    SAL_WARN("cui.dialogs", "POST data: " << sPostData);
    
    OUString sResponse = PerformHttpPost(sUrl, sPostData);
    SAL_WARN("cui.dialogs", "Token exchange response: " << sResponse);
    
    if (!sResponse.isEmpty())
    {
        ParseTokenResponse(sResponse);
        SAL_WARN("cui.dialogs", "Access token: " << (m_sAccessToken.isEmpty() ? "EMPTY" : "RECEIVED"));
        
        // Get user info
        if (!m_sAccessToken.isEmpty())
        {
            OUString sUserInfoUrl = "https://www.googleapis.com/oauth2/v2/userinfo";
            SAL_WARN("cui.dialogs", "Getting user info from: " << sUserInfoUrl);
            OUString sUserInfo = PerformHttpGet(sUserInfoUrl);
            SAL_WARN("cui.dialogs", "User info response: " << sUserInfo);
            
            if (!sUserInfo.isEmpty())
            {
                ParseUserInfoResponse(sUserInfo);
                SAL_WARN("cui.dialogs", "User email: " << m_sUserEmail);
            }
            
            // Save tokens to the auth service
            auto& authService = ::ucb::gdocs::GoogleDriveAuthService::getInstance();
            ::ucb::gdocs::AuthTokens tokens;
            tokens.access_token = m_sAccessToken;
            tokens.refresh_token = m_sRefreshToken;
            tokens.user_email = m_sUserEmail;
            authService.storeTokens(tokens);
            
            // Mark as authenticated
            m_bIsAuthenticated = true;
            
            return true;
        }
    }
    return false;
}

bool GoogleDriveFilePicker::RefreshAccessToken()
{
    if (m_sRefreshToken.isEmpty())
        return false;
        
    OUString sUrl = "https://oauth2.googleapis.com/token";
    OUString sPostData = "refresh_token=" + EncodeURLComponent(m_sRefreshToken);
    sPostData += "&client_id=" + EncodeURLComponent(m_sClientId);
    sPostData += "&client_secret=" + EncodeURLComponent(m_sClientSecret);
    sPostData += "&grant_type=refresh_token";
    
    OUString sResponse = PerformHttpPost(sUrl, sPostData);
    if (!sResponse.isEmpty())
    {
        ParseTokenResponse(sResponse);
        
        // Save tokens using unified auth service
        auto& authService = ::ucb::gdocs::GoogleDriveAuthService::getInstance();
        ::ucb::gdocs::AuthTokens tokens;
        tokens.access_token = m_sAccessToken;
        tokens.refresh_token = m_sRefreshToken;
        tokens.user_email = m_sUserEmail;
        authService.storeTokens(tokens);
        
        return !m_sAccessToken.isEmpty();
    }
    return false;
}

bool GoogleDriveFilePicker::IsTokenValid()
{
    SAL_WARN("cui.dialogs", "IsTokenValid called");
    // Simple check - try to get user info
    OUString sUrl = "https://www.googleapis.com/oauth2/v2/userinfo";
    SAL_WARN("cui.dialogs", "Checking token validity with userinfo API");
    OUString sResponse = PerformHttpGet(sUrl);
    bool bValid = !sResponse.isEmpty() && sResponse.indexOf("error") == -1;
    SAL_WARN("cui.dialogs", "Token valid: " << bValid << ", response: " << sResponse.copy(0, 200));
    return bValid;
}

void GoogleDriveFilePicker::LoadStoredTokens()
{
    SAL_WARN("cui.dialogs", "LoadStoredTokens called");
    // Load tokens from user's config directory
    OUString sConfigPath;
    osl::Security aSecurity;
    aSecurity.getConfigDir(sConfigPath);
    sConfigPath += "/libreoffice/gdrive_tokens.dat";
    
    osl::File aFile(sConfigPath);
    if (aFile.open(osl_File_OpenFlag_Read) == osl::FileBase::E_None)
    {
        sal_uInt64 nSize = 0;
        aFile.getSize(nSize);
        if (nSize > 0 && nSize < 10000) // Sanity check
        {
            std::vector<sal_uInt8> aBuffer(nSize);
            sal_uInt64 nRead = 0;
            if (aFile.read(aBuffer.data(), nSize, nRead) == osl::FileBase::E_None)
            {
                OString sData(reinterpret_cast<const char*>(aBuffer.data()), nRead);
                // Simple format: access_token|refresh_token|email
                sal_Int32 nPos1 = sData.indexOf('|');
                sal_Int32 nPos2 = sData.indexOf('|', nPos1 + 1);
                if (nPos1 > 0 && nPos2 > nPos1)
                {
                    m_sAccessToken = OUString::fromUtf8(sData.subView(0, nPos1));
                    m_sRefreshToken = OUString::fromUtf8(sData.subView(nPos1 + 1, nPos2 - nPos1 - 1));
                    m_sUserEmail = OUString::fromUtf8(sData.subView(nPos2 + 1));
                }
            }
        }
        aFile.close();
    }
}

void GoogleDriveFilePicker::SaveStoredTokens()
{
    // Save tokens to user's config directory
    OUString sConfigPath;
    osl::Security aSecurity;
    aSecurity.getConfigDir(sConfigPath);
    
    // Create directory if needed
    OUString sDir = sConfigPath + "/libreoffice";
    osl::Directory::create(sDir);
    
    sConfigPath = sDir + "/gdrive_tokens.dat";
    
    osl::File aFile(sConfigPath);
    if (aFile.open(osl_File_OpenFlag_Write | osl_File_OpenFlag_Create) == osl::FileBase::E_None)
    {
        // Simple format: access_token|refresh_token|email
        OString sData = OUStringToOString(m_sAccessToken, RTL_TEXTENCODING_UTF8) + "|" +
                       OUStringToOString(m_sRefreshToken, RTL_TEXTENCODING_UTF8) + "|" +
                       OUStringToOString(m_sUserEmail, RTL_TEXTENCODING_UTF8);
        
        sal_uInt64 nWritten = 0;
        aFile.write(sData.getStr(), sData.getLength(), nWritten);
        aFile.close();
    }
}

// HTTP implementation
OUString GoogleDriveFilePicker::PerformHttpGet(const OUString& sUrl)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return OUString();
    
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);
    OStringBuffer sResponse;
    
    curl_easy_setopt(curl, CURLOPT_URL, sUrlUtf8.getStr());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sResponse);
    
    // SSL settings - in production, these should be enabled
    // For development/testing, you may need to set CURLOPT_SSL_VERIFYPEER to 0L
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disable for development
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // Disable for development
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    
    // Set user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LibreOffice/1.0");
    
    // Force IPv4 to avoid IPv6 issues in WSL
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    
    // Add auth header if we have access token
    struct curl_slist* headers = nullptr;
    if (!m_sAccessToken.isEmpty())
    {
        OString sAuthHeader = "Authorization: Bearer " + OUStringToOString(m_sAccessToken, RTL_TEXTENCODING_UTF8);
        headers = curl_slist_append(headers, sAuthHeader.getStr());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    CURLcode res = curl_easy_perform(curl);
    
    // Get HTTP response code
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    if (headers)
        curl_slist_free_all(headers);
    
    OUString sResult;
    if (res == CURLE_OK)
    {
        OString sResponseStr = sResponse.makeStringAndClear();
        SAL_WARN("cui.dialogs", "HTTP response code: " << http_code << ", length: " << sResponseStr.getLength());
        if (http_code >= 400)
        {
            SAL_WARN("cui.dialogs", "HTTP error response: " << sResponseStr);
        }
        sResult = OUString::fromUtf8(sResponseStr);
    }
    else
    {
        SAL_WARN("cui.dialogs", "CURL error: " << curl_easy_strerror(res));
    }
    
    curl_easy_cleanup(curl);
    return sResult;
}

OUString GoogleDriveFilePicker::PerformHttpPost(const OUString& sUrl, const OUString& sPostData)
{
    SAL_WARN("cui.dialogs", "PerformHttpPost called with URL: " << sUrl);
    CURL* curl = curl_easy_init();
    if (!curl)
        return OUString();
    
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);
    OString sPostDataUtf8 = OUStringToOString(sPostData, RTL_TEXTENCODING_UTF8);
    OStringBuffer sResponse;
    
    curl_easy_setopt(curl, CURLOPT_URL, sUrlUtf8.getStr());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostDataUtf8.getStr());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sResponse);
    
    // Disable SSL verification for development
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L); // Increase timeout to 2 minutes
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L); // Increase connection timeout
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LibreOffice/1.0");
    
    // Disable verbose output
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    
    // Add DNS cache timeout
    curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 60L);
    
    // Force IPv4 to avoid IPv6 connection issues in WSL
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    // Get HTTP response code
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    
    OUString sResult;
    if (res == CURLE_OK)
    {
        OString sResponseStr = sResponse.makeStringAndClear();
        SAL_WARN("cui.dialogs", "HTTP POST response code: " << http_code << ", length: " << sResponseStr.getLength());
        if (http_code >= 400)
        {
            SAL_WARN("cui.dialogs", "HTTP POST error response: " << sResponseStr);
        }
        sResult = OUString::fromUtf8(sResponseStr);
    }
    else
    {
        SAL_WARN("cui.dialogs", "CURL error: " << curl_easy_strerror(res));
    }
    
    curl_easy_cleanup(curl);
    return sResult;
}

OUString GoogleDriveFilePicker::EncodeURLComponent(const OUString& sComponent)
{
    // Simple URL encoding
    OUStringBuffer sResult;
    for (sal_Int32 i = 0; i < sComponent.getLength(); ++i)
    {
        sal_Unicode c = sComponent[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
        {
            sResult.append(c);
        }
        else if (c == ' ')
        {
            sResult.append("+");
        }
        else
        {
            sResult.append("%");
            sResult.append(OUString::number(static_cast<sal_uInt16>(c), 16).toAsciiUpperCase());
        }
    }
    return sResult.makeStringAndClear();
}

// Helper to extract JSON string value
OUString GoogleDriveFilePicker::ExtractJsonString(const OUString& sJson, const OUString& sKey)
{
    // First try with string value: "key":"value"
    OUString sSearch = "\"" + sKey + "\":\"";
    sal_Int32 nPos = sJson.indexOf(sSearch);
    
    if (nPos != -1)
    {
        // Found string value
        nPos += sSearch.getLength();
        sal_Int32 nEnd = sJson.indexOf('\"', nPos);
        if (nEnd != -1)
        {
            return sJson.copy(nPos, nEnd - nPos);
        }
    }
    
    // Try without quotes for null/boolean/number values: "key":value
    sSearch = "\"" + sKey + "\":";
    nPos = sJson.indexOf(sSearch);
    if (nPos == -1)
        return OUString();
    
    nPos += sSearch.getLength();
    
    // Skip whitespace
    while (nPos < sJson.getLength() && 
           (sJson[nPos] == ' ' || sJson[nPos] == '\t' || sJson[nPos] == '\n' || sJson[nPos] == '\r'))
        nPos++;
    
    if (nPos >= sJson.getLength())
        return OUString();
    
    // Find end of value
    sal_Int32 nEnd = nPos;
    while (nEnd < sJson.getLength() && 
           sJson[nEnd] != ',' && 
           sJson[nEnd] != '}' && 
           sJson[nEnd] != ']')
        nEnd++;
    
    return sJson.copy(nPos, nEnd - nPos).trim();
}

// JSON parsing
void GoogleDriveFilePicker::ParseFileListResponse(const OUString& sResponse)
{
    SAL_WARN("cui.dialogs", "ParseFileListResponse called, response length: " << sResponse.getLength());
    
    // Log first 500 chars of response for debugging
    if (sResponse.getLength() > 0)
    {
        SAL_WARN("cui.dialogs", "Response preview: " << sResponse.copy(0, std::min(sal_Int32(500), sResponse.getLength())));
    }
    
    sal_Int32 nFilesStart = sResponse.indexOf("\"files\":");
    if (nFilesStart == -1)
    {
        SAL_WARN("cui.dialogs", "No 'files' field found in response");
        return;
    }
        
    sal_Int32 nArrayStart = sResponse.indexOf('[', nFilesStart);
    sal_Int32 nArrayEnd = FindMatchingBracket(sResponse, nArrayStart, '[', ']');
    
    if (nArrayStart == -1 || nArrayEnd == -1)
        return;
        
    // Parse each file object
    sal_Int32 nPos = nArrayStart + 1;
    while (nPos < nArrayEnd)
    {
        sal_Int32 nObjStart = sResponse.indexOf('{', nPos);
        if (nObjStart == -1 || nObjStart >= nArrayEnd)
            break;
            
        sal_Int32 nObjEnd = FindMatchingBracket(sResponse, nObjStart, '{', '}');
        if (nObjEnd == -1)
            break;
            
        OUString sObj = sResponse.copy(nObjStart, nObjEnd - nObjStart + 1);
        
        GoogleDriveItem item;
        item.id = ExtractJsonString(sObj, "id");
        item.name = ExtractJsonString(sObj, "name");
        item.mimeType = ExtractJsonString(sObj, "mimeType");
        item.modifiedTime = ExtractJsonString(sObj, "modifiedTime");
        
        // Debug: Check if quotes are being included
        if (item.id.startsWith("\"") && item.id.endsWith("\""))
        {
            SAL_WARN("cui.dialogs", "WARNING: ID contains quotes, stripping them");
            item.id = item.id.copy(1, item.id.getLength() - 2);
        }
        if (item.name.startsWith("\"") && item.name.endsWith("\""))
        {
            SAL_WARN("cui.dialogs", "WARNING: Name contains quotes, stripping them");
            item.name = item.name.copy(1, item.name.getLength() - 2);
        }
        
        SAL_WARN("cui.dialogs", "Parsed item - ID: " << item.id << ", Name: " << item.name << 
                 ", MimeType: " << item.mimeType);
        
        // Format modified time to show just date
        if (item.modifiedTime.getLength() >= 10)
            item.modifiedTime = item.modifiedTime.copy(0, 10);
        
        item.isFolder = (item.mimeType == "application/vnd.google-apps.folder");
        
        if (!item.id.isEmpty() && !item.name.isEmpty())
        {
            m_aCurrentItems.push_back(item);
            SAL_WARN("cui.dialogs", "Added item to list: " << item.name);
        }
        else
        {
            SAL_WARN("cui.dialogs", "Skipped item - empty ID or name");
        }
        
        nPos = nObjEnd + 1;
    }
    
    SAL_WARN("cui.dialogs", "ParseFileListResponse completed - total items parsed: " << m_aCurrentItems.size());
}

sal_Int32 GoogleDriveFilePicker::FindMatchingBracket(const OUString& sStr, sal_Int32 nStart, sal_Unicode cOpen, sal_Unicode cClose)
{
    if (nStart < 0 || nStart >= sStr.getLength())
        return -1;
        
    sal_Int32 nLevel = 1;
    for (sal_Int32 i = nStart + 1; i < sStr.getLength(); ++i)
    {
        sal_Unicode c = sStr[i];
        if (c == cOpen)
            nLevel++;
        else if (c == cClose)
        {
            nLevel--;
            if (nLevel == 0)
                return i;
        }
    }
    return -1;
}

void GoogleDriveFilePicker::ParseTokenResponse(const OUString& sResponse)
{
    m_sAccessToken = ExtractJsonString(sResponse, "access_token");
    m_sRefreshToken = ExtractJsonString(sResponse, "refresh_token");
    
    OUString sExpiry = ExtractJsonString(sResponse, "expires_in");
    if (!sExpiry.isEmpty())
        m_nTokenExpiry = sExpiry.toInt32();
}

void GoogleDriveFilePicker::ParseUserInfoResponse(const OUString& sResponse)
{
    m_sUserEmail = ExtractJsonString(sResponse, "email");
}

OUString GoogleDriveFilePicker::GetRecentDate()
{
    // Get date 7 days ago in RFC3339 format
    // For demo, return fixed date
    return u"2024-01-13T00:00:00Z"_ustr;
}

size_t GoogleDriveFilePicker::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    OStringBuffer* pBuffer = static_cast<OStringBuffer*>(userp);
    pBuffer->append(static_cast<const char*>(contents), realsize);
    return realsize;
}

OUString GoogleDriveFilePicker::DownloadFile(const OUString& fileId, const OUString& fileName,
                                           const OUString& mimeType)
{
    SAL_WARN("cui.dialogs", "DownloadFile called for: " << fileName << " (ID: " << fileId << ")");
    
    // Strip quotes if present (shouldn't happen, but just in case)
    OUString cleanFileId = fileId;
    OUString cleanFileName = fileName;
    
    if (cleanFileId.startsWith("\"") && cleanFileId.endsWith("\""))
    {
        SAL_WARN("cui.dialogs", "ERROR: fileId contains quotes, stripping them");
        cleanFileId = cleanFileId.copy(1, cleanFileId.getLength() - 2);
    }
    if (cleanFileName.startsWith("\"") && cleanFileName.endsWith("\""))
    {
        SAL_WARN("cui.dialogs", "ERROR: fileName contains quotes, stripping them");
        cleanFileName = cleanFileName.copy(1, cleanFileName.getLength() - 2);
    }
    
    if (cleanFileId.isEmpty() || !IsTokenValid())
    {
        SAL_WARN("cui.dialogs", "Invalid file ID or token");
        return OUString();
    }
    
    // Determine export URL based on mime type
    OUString sDownloadUrl;
    OUString sExportMimeType;
    
    if (mimeType == "application/vnd.google-apps.document")
    {
        sExportMimeType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        sDownloadUrl = "https://www.googleapis.com/drive/v3/files/" + cleanFileId + 
                      "/export?mimeType=" + EncodeURLComponent(sExportMimeType);
    }
    else if (mimeType == "application/vnd.google-apps.spreadsheet")
    {
        sExportMimeType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        sDownloadUrl = "https://www.googleapis.com/drive/v3/files/" + cleanFileId + 
                      "/export?mimeType=" + EncodeURLComponent(sExportMimeType);
    }
    else if (mimeType == "application/vnd.google-apps.presentation")
    {
        sExportMimeType = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
        sDownloadUrl = "https://www.googleapis.com/drive/v3/files/" + cleanFileId + 
                      "/export?mimeType=" + EncodeURLComponent(sExportMimeType);
    }
    else
    {
        // Regular file download
        sDownloadUrl = "https://www.googleapis.com/drive/v3/files/" + cleanFileId + "?alt=media";
    }
    
    // Determine extension
    OUString sExtension;
    if (mimeType == "application/vnd.google-apps.document")
        sExtension = ".docx";
    else if (mimeType == "application/vnd.google-apps.spreadsheet")
        sExtension = ".xlsx";
    else if (mimeType == "application/vnd.google-apps.presentation")
        sExtension = ".pptx";
    else
    {
        sal_Int32 nDotPos = cleanFileName.lastIndexOf('.');
        if (nDotPos != -1)
            sExtension = cleanFileName.copy(nDotPos);
    }
    
    // Create temporary file with proper name
    OUString sPrefix = "gdrive_" + cleanFileName;
    if (sExtension.isEmpty() && cleanFileName.indexOf('.') == -1)
    {
        // Add extension if not present
        if (mimeType == "application/vnd.google-apps.document")
            sPrefix += ".docx";
        else if (mimeType == "application/vnd.google-apps.spreadsheet")
            sPrefix += ".xlsx";
        else if (mimeType == "application/vnd.google-apps.presentation")
            sPrefix += ".pptx";
    }
    
    utl::TempFileNamed aTempFile(sPrefix);
    // Don't enable killing file - we need it to persist after this function returns
    // aTempFile.EnableKillingFile();
    OUString sTempPath = aTempFile.GetURL();
    
    SAL_WARN("cui.dialogs", "Downloading to temp file: " << sTempPath);
    SAL_WARN("cui.dialogs", "Download URL: " << sDownloadUrl);
    
    // Download file content
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        SAL_WARN("cui.dialogs", "Failed to initialize CURL");
        return OUString();
    }
    
    // Get system path from URL
    OUString sSysPath;
    osl::FileBase::getSystemPathFromFileURL(aTempFile.GetURL(), sSysPath);
    OString sSysPathUtf8 = OUStringToOString(sSysPath, RTL_TEXTENCODING_UTF8);
    
    SAL_WARN("cui.dialogs", "Opening file for writing: " << sSysPath);
    
    FILE* fp = fopen(sSysPathUtf8.getStr(), "wb");
    if (!fp)
    {
        SAL_WARN("cui.dialogs", "Failed to open temp file for writing: " << sSysPath);
        curl_easy_cleanup(curl);
        return OUString();
    }
    
    OString sUrlUtf8 = OUStringToOString(sDownloadUrl, RTL_TEXTENCODING_UTF8);
    OString sAuth = "Authorization: Bearer " + OUStringToOString(m_sAccessToken, RTL_TEXTENCODING_UTF8);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, sAuth.getStr());
    
    curl_easy_setopt(curl, CURLOPT_URL, sUrlUtf8.getStr());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    SAL_WARN("cui.dialogs", "Download result: " << curl_easy_strerror(res) << " HTTP code: " << http_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK || http_code != 200)
    {
        SAL_WARN("cui.dialogs", "Download failed: " << curl_easy_strerror(res) << " HTTP: " << http_code);
        return OUString();
    }
    
    // Register file with GoogleDriveSync
    sfx2::GoogleDriveSync::getInstance().registerFile(sTempPath, cleanFileId, mimeType, cleanFileName);
    
    SAL_WARN("cui.dialogs", "File downloaded successfully to: " << sTempPath);
    return sTempPath;
}

bool GoogleDriveFilePicker::UploadFile(const OUString& localPath, const OUString& fileId,
                                      const OUString& mimeType)
{
    SAL_WARN("cui.dialogs", "UploadFile called for: " << localPath << " (ID: " << fileId << ")");
    
    if (fileId.isEmpty() || localPath.isEmpty() || !IsTokenValid())
    {
        SAL_WARN("cui.dialogs", "Invalid parameters or token");
        return false;
    }
    
    // Open the local file
    OString sLocalPathUtf8 = OUStringToOString(localPath, RTL_TEXTENCODING_UTF8);
    FILE* fp = fopen(sLocalPathUtf8.getStr(), "rb");
    if (!fp)
    {
        SAL_WARN("cui.dialogs", "Failed to open local file: " << localPath);
        return false;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // Read file content
    std::vector<char> fileContent(fileSize);
    size_t bytesRead = fread(fileContent.data(), 1, fileSize, fp);
    fclose(fp);
    
    if (bytesRead != static_cast<size_t>(fileSize))
    {
        SAL_WARN("cui.dialogs", "Failed to read file content");
        return false;
    }
    
    // Prepare upload URL
    OUString sUploadUrl = "https://www.googleapis.com/upload/drive/v3/files/" + fileId + "?uploadType=media";
    
    // Determine content type for upload
    OUString sContentType;
    if (mimeType == "application/vnd.google-apps.document")
        sContentType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    else if (mimeType == "application/vnd.google-apps.spreadsheet")
        sContentType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    else if (mimeType == "application/vnd.google-apps.presentation")
        sContentType = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    else
        sContentType = "application/octet-stream";
    
    // Perform upload
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        SAL_WARN("cui.dialogs", "Failed to initialize CURL");
        return false;
    }
    
    OString sUrlUtf8 = OUStringToOString(sUploadUrl, RTL_TEXTENCODING_UTF8);
    OString sAuth = "Authorization: Bearer " + OUStringToOString(m_sAccessToken, RTL_TEXTENCODING_UTF8);
    OString sContentTypeHeader = "Content-Type: " + OUStringToOString(sContentType, RTL_TEXTENCODING_UTF8);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, sAuth.getStr());
    headers = curl_slist_append(headers, sContentTypeHeader.getStr());
    
    OStringBuffer responseBuffer;
    
    curl_easy_setopt(curl, CURLOPT_URL, sUrlUtf8.getStr());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fileContent.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, fileSize);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK || http_code != 200)
    {
        SAL_WARN("cui.dialogs", "Upload failed: " << curl_easy_strerror(res) << " HTTP: " << http_code);
        SAL_WARN("cui.dialogs", "Response: " << responseBuffer.toString());
        return false;
    }
    
    SAL_WARN("cui.dialogs", "File uploaded successfully");
    return true;
}

// Static helper method
bool GoogleDriveFilePicker::UploadFileToGoogleDrive(const OUString& localPath, const OUString& fileId,
                                                   const OUString& mimeType)
{
    SAL_WARN("cui.dialogs", "Static UploadFileToGoogleDrive called for: " << localPath);
    
    // Get authentication from unified service
    auto& authService = ::ucb::gdocs::GoogleDriveAuthService::getInstance();
    if (!authService.hasValidTokens())
    {
        SAL_WARN("cui.dialogs", "No valid tokens for upload");
        return false;
    }
    
    auto tokens = authService.getStoredTokens();
    if (tokens.access_token.isEmpty())
    {
        SAL_WARN("cui.dialogs", "Empty access token");
        return false;
    }
    
    // Open the local file
    OString sLocalPathUtf8 = OUStringToOString(localPath, RTL_TEXTENCODING_UTF8);
    FILE* fp = fopen(sLocalPathUtf8.getStr(), "rb");
    if (!fp)
    {
        SAL_WARN("cui.dialogs", "Failed to open local file: " << localPath);
        return false;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // Read file content
    std::vector<char> fileContent(fileSize);
    size_t bytesRead = fread(fileContent.data(), 1, fileSize, fp);
    fclose(fp);
    
    if (bytesRead != static_cast<size_t>(fileSize))
    {
        SAL_WARN("cui.dialogs", "Failed to read file content");
        return false;
    }
    
    // Prepare upload URL
    OUString sUploadUrl = "https://www.googleapis.com/upload/drive/v3/files/" + fileId + "?uploadType=media";
    
    // Determine content type for upload
    OUString sContentType;
    if (mimeType == "application/vnd.google-apps.document")
        sContentType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    else if (mimeType == "application/vnd.google-apps.spreadsheet")
        sContentType = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    else if (mimeType == "application/vnd.google-apps.presentation")
        sContentType = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    else
        sContentType = "application/octet-stream";
    
    // Perform upload
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        SAL_WARN("cui.dialogs", "Failed to initialize CURL");
        return false;
    }
    
    OString sUrlUtf8 = OUStringToOString(sUploadUrl, RTL_TEXTENCODING_UTF8);
    OString sAuth = "Authorization: Bearer " + OUStringToOString(tokens.access_token, RTL_TEXTENCODING_UTF8);
    OString sContentTypeHeader = "Content-Type: " + OUStringToOString(sContentType, RTL_TEXTENCODING_UTF8);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, sAuth.getStr());
    headers = curl_slist_append(headers, sContentTypeHeader.getStr());
    
    OStringBuffer responseBuffer;
    
    curl_easy_setopt(curl, CURLOPT_URL, sUrlUtf8.getStr());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fileContent.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, fileSize);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK || http_code != 200)
    {
        SAL_WARN("cui.dialogs", "Static upload failed: " << curl_easy_strerror(res) << " HTTP: " << http_code);
        SAL_WARN("cui.dialogs", "Response: " << responseBuffer.toString());
        return false;
    }
    
    SAL_WARN("cui.dialogs", "File uploaded successfully via static method");
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */