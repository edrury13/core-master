# Google Docs UI Integration Implementation Plan

## Overview
Add a dedicated "Open from Google Drive" menu item and file browser to LibreOffice for improved user experience when accessing Google Docs, Sheets, and Slides.

## Phase 1: Menu Integration

### 1.1 Add Menu Item
**Location**: File menu, after "Open Remote..."
**Label**: "Open from Google Drive..."
**Command**: `.uno:OpenFromGoogleDrive`

**Files to modify:**
- `officecfg/registry/data/org/openoffice/Office/UI/GenericCommands.xcu`
  - Add command definition for OpenFromGoogleDrive
- `sw/uiconfig/swriter/menubar/menubar.xml` (Writer)
- `sc/uiconfig/scalc/menubar/menubar.xml` (Calc)
- `sd/uiconfig/simpress/menubar/menubar.xml` (Impress)
  - Add menu item to File menu in each application

### 1.2 Command Implementation
**Files to create/modify:**
- `sfx2/source/appl/appserv.cxx`
  - Add handler for SID_OPENFROMGOOGLEDRIVE
- `sfx2/inc/sfx2/sfxsids.hrc`
  - Define new SID_OPENFROMGOOGLEDRIVE constant
- `sfx2/sdi/sfx.sdi`
  - Add slot definition for the new command

## Phase 2: Google Drive File Browser Dialog

### 2.1 Dialog Design
**Features:**
- Tree view for folder navigation
- List view for files with columns: Name, Type, Modified Date, Owner
- Search box for finding files
- File type filter (Documents, Spreadsheets, Presentations, All)
- Preview pane (optional)
- Multi-select support
- Breadcrumb navigation

### 2.2 Dialog Implementation
**New files to create:**
- `cui/source/dialogs/GoogleDriveFilePicker.hxx`
- `cui/source/dialogs/GoogleDriveFilePicker.cxx`
  - Main dialog class inheriting from ModalDialog
  - Handle OAuth authentication if not already authenticated
  - List and navigate Google Drive contents
  - Return selected file(s) URL

- `cui/uiconfig/ui/googledrivepicker.ui`
  - Glade UI definition file
  - Layout: 
    ```
    +------------------------------------------+
    | Open from Google Drive                   |
    +------------------------------------------+
    | Search: [_______________] [Search]       |
    +------------------------------------------+
    | My Drive > Folder1 > Subfolder          |
    +----------+-------------------------------+
    | Folders  | Name    | Type | Modified    |
    | -------- | ------- | ---- | ----------- |
    | My Drive | Doc1    | Doc  | 2024-01-20  |
    | Shared   | Sheet1  | Sheet| 2024-01-19  |
    | Recent   | Pres1   | Slide| 2024-01-18  |
    +----------+-------------------------------+
    | Filter: [All Files     v]                |
    +------------------------------------------+
    | [Open] [Cancel]                          |
    +------------------------------------------+
    ```

### 2.3 Google Drive API Integration
**Files to modify:**
- `ucb/source/ucp/gdocs/gdocs_provider.hxx/cxx`
  - Add methods for listing drive contents
  - Support folder navigation
  - Implement search functionality

**New methods needed:**
- `listFolderContents(const OUString& folderId)`
- `searchFiles(const OUString& query)`
- `getFileMetadata(const OUString& fileId)`
- `getRootFolderId()`
- `getSharedWithMeFolderId()`

## Phase 3: Authentication Flow Integration

### 3.1 Authentication Status
**Files to modify:**
- `ucb/source/ucp/gdocs/gdocs_auth.hxx/cxx`
  - Add methods to check authentication status
  - Store tokens persistently (using LibreOffice's password manager)
  - Add method to get current user info

**New methods:**
- `isAuthenticated()`
- `getCurrentUserEmail()`
- `storeTokens(const GoogleCredentials& creds)`
- `loadStoredTokens()`

### 3.2 Authentication Dialog
**Files to create:**
- `cui/source/dialogs/GoogleAuthDialog.hxx/cxx`
  - Simple dialog showing authentication status
  - "Sign in with Google" button
  - Display current user if authenticated
  - "Sign out" option

## Phase 4: Integration with Existing Systems

### 4.1 Recent Documents
- Add Google Docs to recent documents list when opened
- Show Google Docs icon for Google Drive files

### 4.2 File Type Detection
**Files to modify:**
- `filter/source/config/fragments/types/Google_Docs_Document.xcu`
- Add MIME type associations for picker dialog

### 4.3 Progress Indication
- Show progress bar during file download
- Use LibreOffice's standard progress indication framework

## Phase 5: Implementation Details

### 5.1 Threading
- API calls should be on separate thread
- Use LibreOffice's threading helpers
- UI should remain responsive during loading

### 5.2 Caching
- Cache folder structure for performance
- Implement refresh mechanism
- Store thumbnails if preview is implemented

### 5.3 Error Handling
- Network connectivity issues
- Authentication failures
- API quota exceeded
- File access permissions

## Phase 6: Testing Requirements

### 6.1 Unit Tests
- Test Google Drive API wrapper methods
- Test authentication flow
- Test file listing and search

### 6.2 Integration Tests
- Test menu item in all applications
- Test dialog behavior
- Test file opening flow end-to-end

### 6.3 UI Tests
- Test keyboard navigation
- Test accessibility features
- Test different screen resolutions

## Implementation Order

1. **Week 1**: Menu integration and basic command handler
2. **Week 2**: Authentication improvements and token storage
3. **Week 3**: Google Drive API methods for listing/searching
4. **Week 4**: File picker dialog UI
5. **Week 5**: Dialog functionality and API integration
6. **Week 6**: Testing and polish

## Alternative Approaches Considered

### Option 1: Integrate with system file picker
- Use OS-native file picker with Google Drive extension
- Pros: Native look and feel
- Cons: Platform-specific implementation needed

### Option 2: Side panel integration
- Add Google Drive browser to sidebar
- Pros: Always accessible, doesn't block workflow
- Cons: Limited space, might clutter interface

### Option 3: Start Center integration
- Add Google Drive section to LibreOffice Start Center
- Pros: Discoverable for new users
- Cons: Only accessible at startup

## Recommendation
Implement the modal dialog approach (Phase 2) as it:
- Follows LibreOffice UI patterns
- Provides dedicated space for navigation
- Allows for future enhancements (preview, etc.)
- Is consistent across platforms