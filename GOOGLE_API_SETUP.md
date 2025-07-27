# Google API Setup for LibreOffice Google Docs Integration

## Overview
The Google Docs integration in LibreOffice uses OAuth2 authentication to access Google Drive. Instead of hardcoding API credentials in the source code, the implementation reads them from environment variables for better security.

## Files That Use Environment Variables
- `ucb/source/ucp/gdocs/gdocs_auth.cxx` - Reads `GOOGLE_CLIENT_ID` and `GOOGLE_CLIENT_SECRET`
- `ucb/source/ucp/gdocs/gdocs_auth.hxx` - Declares helper functions `getGoogleClientId()` and `getGoogleClientSecret()`

## Setup Instructions

### 1. Get Google OAuth2 Credentials
1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select an existing one
3. Enable Google Drive API:
   - Go to "APIs & Services" → "Library"
   - Search for "Google Drive API"
   - Click on it and press "Enable"
4. Create OAuth2 credentials:
   - Go to "APIs & Services" → "Credentials"
   - Click "Create Credentials" → "OAuth client ID"
   - Choose "Desktop app" as the application type
   - Give it a name (e.g., "LibreOffice Google Docs")
   - Copy the Client ID and Client Secret

### 2. Set Environment Variables

#### Windows (Command Prompt)
```cmd
set GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com
set GOOGLE_CLIENT_SECRET=your-client-secret
```

#### Windows (PowerShell)
```powershell
$env:GOOGLE_CLIENT_ID="569087325927-l2nds7oge01flbo100i9aqbl406hl77r.apps.googleusercontent.com"
$env:GOOGLE_CLIENT_SECRET="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"
```

#### Linux/WSL/macOS
```bash
export GOOGLE_CLIENT_ID="569087325927-l2nds7oge01flbo100i9aqbl406hl77r.apps.googleusercontent.com"
export GOOGLE_CLIENT_SECRET="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"
```

### 3. Make Environment Variables Persistent

#### Windows
1. Open System Properties (Win + Pause/Break)
2. Click "Advanced system settings"
3. Click "Environment Variables"
4. Under "User variables", click "New"
5. Add both `GOOGLE_CLIENT_ID` and `GOOGLE_CLIENT_SECRET`

#### Linux/WSL
Add to your `~/.bashrc` or `~/.profile`:
```bash
export GOOGLE_CLIENT_ID="your-client-id.apps.googleusercontent.com"
export GOOGLE_CLIENT_SECRET="your-client-secret"
```

Then reload:
```bash
source ~/.bashrc
```

## Testing the Integration

1. Start LibreOffice Writer with the environment variables set
2. Go to File → Open
3. Enter a Google Docs URL in one of these formats:
   - `https://docs.google.com/document/d/FILE_ID/edit`
   - `gdocs://user@gmail.com/FILE_ID`
4. A browser window should open for OAuth authentication
5. After authorizing, the document should open in Writer

## Troubleshooting

### Error: "GOOGLE_CLIENT_ID environment variable not set"
- Make sure you've set the environment variables in the same terminal/session where you're running LibreOffice
- On Windows, you may need to restart the terminal after setting system environment variables

### OAuth Error: "Invalid client"
- Double-check that your Client ID and Secret are copied correctly
- Ensure there are no extra spaces or quotes in the environment variables
- Verify the OAuth client type is "Desktop app" in Google Cloud Console

### Browser doesn't open
- Check if `xdg-open` (Linux) or default browser is properly configured
- Try manually copying the authorization URL from the error message

## Security Considerations

1. **Never commit credentials to version control**
   - Add `.env` to `.gitignore` if using .env files
   - Don't hardcode credentials in source files

2. **Use appropriate OAuth scopes**
   - Current implementation uses `https://www.googleapis.com/auth/drive` (full access)
   - Consider using `https://www.googleapis.com/auth/drive.file` for limited access

3. **Token storage**
   - The current implementation prompts for authorization each time
   - Future enhancement: Store refresh tokens securely in LibreOffice's password manager

## Implementation Details

The environment variables are read using LibreOffice's standard `o3tl::getEnvironment()` function, which provides cross-platform environment variable access. If the variables are not set, the functions throw a `uno::RuntimeException` with a descriptive error message.

### Code Example
```cpp
OUString getGoogleClientId()
{
    OUString clientId = o3tl::getEnvironment(u"GOOGLE_CLIENT_ID"_ustr);
    if (clientId.isEmpty())
    {
        throw uno::RuntimeException(
            "GOOGLE_CLIENT_ID environment variable not set. "
            "Please set it to your Google OAuth2 client ID.");
    }
    return clientId;
}
```

## Alternative Approach (Not Implemented)

An alternative approach would be to use LibreOffice's configuration system:
1. Create a configuration schema for Google Docs settings
2. Add UI in Tools → Options for entering credentials
3. Store encrypted credentials in the user profile

However, environment variables are simpler for development and follow common practices for OAuth credentials.