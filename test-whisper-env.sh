#!/bin/bash
# Test script to verify Whisper environment variable setup

echo -e "\033[36mTesting Whisper Environment Variable Setup\033[0m"
echo -e "\033[36m=========================================\033[0m"

# Check if the environment variable is set
echo -e "\n\033[33mChecking environment:\033[0m"
if [ -n "$LIBREOFFICE_OPENAI_API_KEY" ]; then
    echo -e "  \033[32mLIBREOFFICE_OPENAI_API_KEY is set (length: ${#LIBREOFFICE_OPENAI_API_KEY})\033[0m"
else
    echo -e "  \033[31mLIBREOFFICE_OPENAI_API_KEY is NOT set\033[0m"
fi

# Show how to set the variable
echo -e "\n\033[36mTo set the environment variable:\033[0m"
echo -e "  \033[33mTemporary (current session only):\033[0m"
echo '    export LIBREOFFICE_OPENAI_API_KEY="your-api-key-here"'
echo -e "  \033[33mPermanent (add to ~/.bashrc):\033[0m"
echo '    echo '\''export LIBREOFFICE_OPENAI_API_KEY="your-api-key-here"'\'' >> ~/.bashrc'

# Test running LibreOffice with the variable
echo -e "\n\033[36mTo run LibreOffice with the API key:\033[0m"
echo '  LIBREOFFICE_OPENAI_API_KEY="your-api-key-here" ./instdir/program/soffice'
echo "  OR if already exported:"
echo '  ./instdir/program/soffice'

# Check for logs
echo -e "\n\033[36mChecking for recent Whisper logs:\033[0m"
if [ -f "gdrive-debug.log" ]; then
    whisper_logs=$(grep "sw.whisper" gdrive-debug.log 2>/dev/null | tail -10)
    if [ -n "$whisper_logs" ]; then
        echo -e "\033[33mRecent Whisper log entries:\033[0m"
        echo "$whisper_logs" | while IFS= read -r line; do
            echo -e "  \033[90m$line\033[0m"
        done
    else
        echo -e "  \033[31mNo Whisper logs found in gdrive-debug.log\033[0m"
    fi
else
    echo -e "  \033[31mLog file gdrive-debug.log not found\033[0m"
fi

# Check if LibreOffice can see the variable
echo -e "\n\033[36mTesting if LibreOffice build can access environment:\033[0m"
if [ -x "./instdir/program/soffice" ]; then
    echo -e "  \033[32mLibreOffice executable found\033[0m"
else
    echo -e "  \033[31mLibreOffice executable not found at ./instdir/program/soffice\033[0m"
fi