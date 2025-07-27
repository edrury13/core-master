#!/bin/bash
# Run LibreOffice with Whisper support

echo "Starting LibreOffice with Whisper support..."
echo "=========================================="

# Set the OpenAI API key - REPLACE WITH YOUR ACTUAL KEY
export LIBREOFFICE_OPENAI_API_KEY="your-openai-api-key-here"

# Enable debug logging for Whisper
export SAL_LOG="+INFO.sw.whisper+WARN.sw.whisper"

cd ~/libreoffice/core-master

if [ -z "$LIBREOFFICE_OPENAI_API_KEY" ] || [ "$LIBREOFFICE_OPENAI_API_KEY" = "your-openai-api-key-here" ]; then
    echo "WARNING: LIBREOFFICE_OPENAI_API_KEY is not set!"
    echo "Please edit this script and add your OpenAI API key."
    echo ""
fi

echo "Starting LibreOffice Writer..."
./instdir/program/soffice --writer