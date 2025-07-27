# Google Docs Authentication Setup

## Overview

The LibreOffice Google Docs integration uses OAuth2 for authentication. To protect sensitive credentials, the client ID and client secret are now read from environment variables instead of being hardcoded.

## Setting Up Google OAuth2 Credentials

1. **Create a Google Cloud Project**
   - Go to [Google Cloud Console](https://console.cloud.google.com/)
   - Create a new project or select an existing one
   - Note the project ID for later use

2. **Enable Google Drive API**
   - In the Google Cloud Console, go to "APIs & Services" > "Library"
   - Search for "Google Drive API"
   - Click on it and press "Enable"

3. **Create OAuth2 Credentials**
   - Go to "APIs & Services" > "Credentials"
   - Click "Create Credentials" > "OAuth client ID"
   - Choose "Desktop app" as the application type
   - Give it a meaningful name (e.g., "LibreOffice Google Docs Integration")
   - Download the credentials or copy the Client ID and Client Secret

## Setting Environment Variables

### Windows

Open Command Prompt or PowerShell and run:
```cmd
set GOOGLE_CLIENT_ID=your_client_id_here.apps.googleusercontent.com
set GOOGLE_CLIENT_SECRET=your_client_secret_here
```

For permanent settings, add them through System Properties > Environment Variables.

### Linux/macOS

Add to your shell configuration file (~/.bashrc, ~/.zshrc, etc.):
```bash
export GOOGLE_CLIENT_ID="your_client_id_here.apps.googleusercontent.com"
export GOOGLE_CLIENT_SECRET="your_client_secret_here"
```

Then reload your shell configuration:
```bash
source ~/.bashrc  # or ~/.zshrc
```

## Error Handling

If the environment variables are not set, the authentication functions will throw a `uno::RuntimeException` with a descriptive error message:
- "GOOGLE_CLIENT_ID environment variable not set. Please set it to your Google OAuth2 client ID."
- "GOOGLE_CLIENT_SECRET environment variable not set. Please set it to your Google OAuth2 client secret."

## Security Considerations

- Never commit actual credentials to version control
- Use the provided `.env.example` file as a template
- Consider using a password manager or secure vault for production environments
- Rotate credentials regularly
- Restrict OAuth2 scopes to minimum required permissions

## Testing

To verify your setup:
1. Set the environment variables as described above
2. Run LibreOffice
3. Try to access Google Docs through the file picker
4. You should be redirected to Google's OAuth2 consent screen

## Troubleshooting

- **Environment variables not found**: Make sure you've set them in the same terminal/environment where LibreOffice is running
- **Authentication fails**: Verify that your Client ID and Secret are correct and that the Google Drive API is enabled
- **Redirect URI mismatch**: The code uses `urn:ietf:wg:oauth:2.0:oob` as the redirect URI, which is suitable for desktop applications