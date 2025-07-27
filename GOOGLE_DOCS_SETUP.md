# Google Docs Integration Setup

## Quick Setup

1. **Get Google OAuth2 Credentials**
   - Go to https://console.cloud.google.com/
   - Create a new project or select an existing one
   - Enable Google Drive API
   - Create OAuth2 credentials (Desktop application type)
   - Copy the Client ID and Client Secret

2. **Set Environment Variables**

   **Windows (Command Prompt):**
   ```cmd
   set GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com
   set GOOGLE_CLIENT_SECRET=your-client-secret
   ```

   **Windows (PowerShell):**
   ```powershell
   $env:GOOGLE_CLIENT_ID="your-client-id.apps.googleusercontent.com"
   $env:GOOGLE_CLIENT_SECRET="your-client-secret"
   ```

   **WSL/Linux:**
   ```bash
   export GOOGLE_CLIENT_ID="your-client-id.apps.googleusercontent.com"
   export GOOGLE_CLIENT_SECRET="your-client-secret"
   ```

3. **Test the Integration**
   - Run LibreOffice Writer
   - Try to open a Google Docs URL: File → Open → Enter URL like:
     - `https://docs.google.com/document/d/FILE_ID/edit`
     - Or `gdocs://user@gmail.com/FILE_ID`
   - A browser should open for OAuth authentication
   - After authorization, the document should open in Writer

## Persistent Environment Variables

**Windows:**
- System Properties → Advanced → Environment Variables
- Add GOOGLE_CLIENT_ID and GOOGLE_CLIENT_SECRET as user variables

**Linux/WSL:**
- Add to `~/.bashrc` or `~/.profile`:
  ```bash
  export GOOGLE_CLIENT_ID="your-client-id.apps.googleusercontent.com"
  export GOOGLE_CLIENT_SECRET="your-client-secret"
  ```

## Security Notes
- Never commit actual credentials to version control
- Use `.env` files only for local development
- For production, use secure credential storage