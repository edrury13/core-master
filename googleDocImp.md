# Google Docs Support Implementation Plan

Based on the git status, I can see you already have some Google Docs infrastructure started. Let me analyze what's needed:

## Current State Analysis

**Already present in your codebase:**
- `filter/source/config/fragments/filters/Google_*` - Filter definitions
- `filter/source/config/fragments/types/Google_*` - File type definitions  
- `ucb/source/ucp/gdocs/` - Universal Content Provider for Google Drive

## Implementation Strategy

### 1. **Google Docs Architecture Overview**
Google Docs isn't a file format - it's a cloud service. We need to:
- Use Google Drive API v3 to access documents
- Convert between Google's format and ODF
- Handle OAuth2 authentication

### 2. **Key Components Needed**

**A. Authentication Layer**
```cpp
// In ucb/source/ucp/gdocs/gdocs_auth.hxx
class GDocsAuth {
    // OAuth2 flow implementation
    // Token storage and refresh
    // Scopes: drive.file, docs.readonly
};
```

**B. Import Filter**
```cpp
// In filter/source/gdocs/GDocsImportFilter.cxx
// 1. Authenticate with Google
// 2. Use Google Docs API to export as .docx
// 3. Pass to existing DOCX import filter
// OR
// 2. Use Google Docs API to get document structure
// 3. Convert directly to Writer document model
```

**C. Export Filter**
```cpp
// In filter/source/gdocs/GDocsExportFilter.cxx
// 1. Convert ODF to Google Docs format
// 2. Upload using Google Drive API
// 3. Return document ID/URL
```

### 3. **Implementation Steps**

**Step 1: Check Existing UCP Implementation**
```bash
# Look at what's already there
ls -la ucb/source/ucp/gdocs/
cat ucb/source/ucp/gdocs/*.hxx
```

**Step 2: Create Authentication Module**
- Location: `ucb/source/ucp/gdocs/auth/`
- Implement OAuth2 flow
- Store refresh tokens securely
- Handle token expiration

**Step 3: Implement Import Filter**
- Location: `filter/source/gdocs/`
- Two approaches:
  1. **Simple**: Download as DOCX, use existing filter
  2. **Advanced**: Direct API conversion

**Step 4: Register Filters**
The .xcu files you have suggest filters are partially registered. Need to:
- Complete filter implementation
- Add UI elements for Google Docs in File → Open dialog

### 4. **Detailed Implementation Plan**

**Phase 1: Basic Import (Easiest)**
```cpp
// filter/source/gdocs/GDocsImportFilter.cxx
class GDocsImportFilter : public cppu::WeakImplHelper<XImportFilter>
{
    // 1. Parse Google Docs URL/ID from input
    // 2. Authenticate using stored credentials
    // 3. Call Google Export API:
    //    GET https://docs.google.com/feeds/download/documents/export/Export?id={DOC_ID}&exportFormat=docx
    // 4. Save temporary DOCX
    // 5. Call existing DOCX import filter
};
```

**Phase 2: Authentication UI**
- Add Google account management to Tools → Options
- Store credentials securely using LibreOffice's password storage

**Phase 3: Direct Integration**
- Implement Google Docs API v1 document structure parsing
- Convert directly to LibreOffice's document model
- Handle collaborative features (comments, suggestions)

### 5. **Required Google APIs**
1. **Google Drive API v3** - File listing and metadata
2. **Google Docs API v1** - Document structure access
3. **Google Sheets API v4** - Spreadsheet access
4. **Google Slides API v1** - Presentation access

### 6. **Configuration Needed**
```xml
<!-- In filter/source/config/fragments/filters/Google_Docs_Document.xcu -->
<node oor:name="Google Docs Document">
    <prop oor:name="FileFormatVersion"><value>0</value></prop>
    <prop oor:name="Type"><value>google_docs_document</value></prop>
    <prop oor:name="UIComponent"/>
    <prop oor:name="FilterService">
        <value>com.sun.star.comp.Writer.GDocsImportFilter</value>
    </prop>
    <prop oor:name="UIName">
        <value xml:lang="en-US">Google Docs Document</value>
    </prop>
</node>
```

## Quick Start Implementation

**For your demo, the fastest approach:**

1. **Implement Basic Import Only**
   - Download Google Doc as DOCX using export API
   - Pass to existing DOCX filter
   - ~500 lines of code

2. **Hardcode Authentication for Demo**
   - Use a service account or hardcoded OAuth token
   - Proper auth can come later

3. **Add Menu Item**
   - File → Open from Google Drive
   - Simple dialog with Doc ID input

Would you like me to start implementing the basic import filter that downloads as DOCX and reuses your existing filter?