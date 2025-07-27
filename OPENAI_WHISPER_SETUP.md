# OpenAI Whisper Speech-to-Text Setup for LibreOffice

## Prerequisites

You need an OpenAI API key to use the speech-to-text functionality. If you don't have one:
1. Create an account at https://platform.openai.com
2. Navigate to API Keys section
3. Create a new API key
4. Save it securely - you won't be able to see it again

## Setting up the API Key

### Windows (Command Prompt)
```cmd
setx LIBREOFFICE_OPENAI_API_KEY "your-api-key-here"
```

### Windows (PowerShell)
```powershell
[System.Environment]::SetEnvironmentVariable('LIBREOFFICE_OPENAI_API_KEY', 'your-api-key-here', 'User')
```

### Linux/macOS (Bash)
```bash
echo 'export LIBREOFFICE_OPENAI_API_KEY="your-api-key-here"' >> ~/.bashrc
source ~/.bashrc
```

### Linux/macOS (Zsh)
```zsh
echo 'export LIBREOFFICE_OPENAI_API_KEY="your-api-key-here"' >> ~/.zshrc
source ~/.zshrc
```

## Verifying the Setup

### Windows
```cmd
echo %LIBREOFFICE_OPENAI_API_KEY%
```

### Linux/macOS
```bash
echo $LIBREOFFICE_OPENAI_API_KEY
```

## Usage in LibreOffice Writer

1. **Start Recording**: Press `Ctrl+Shift+V` or click the microphone button in the toolbar
2. **Stop Recording**: Press `Ctrl+Shift+V` again or click the microphone button
3. **Settings**: Go to Tools → Speech Recognition Settings

## Troubleshooting

### API Key Not Found
- Make sure you've restarted LibreOffice after setting the environment variable
- On Windows, you may need to restart your computer for the environment variable to take effect

### Permission Denied
- On first use, you may need to grant microphone permissions to LibreOffice
- Windows: Check Settings → Privacy → Microphone
- macOS: System Preferences → Security & Privacy → Microphone
- Linux: Check your distribution's audio permissions

### Network Errors
- Ensure you have an active internet connection
- Check if your firewall is blocking LibreOffice
- Verify the API key is correct

## Privacy Notice

- Audio is sent to OpenAI's servers for transcription
- Audio is not stored by LibreOffice after processing
- Review OpenAI's privacy policy: https://openai.com/privacy/

## Pricing

- Whisper API costs $0.006 per minute of audio
- Check your usage at: https://platform.openai.com/usage