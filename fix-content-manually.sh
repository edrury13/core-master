#!/bin/bash
# Manual fix for gdocs_content.cxx

WSL_DEST="$HOME/libreoffice/core-master"

echo "Manually fixing gdocs_content.cxx..."

# Find the queryCreatableContentsInfo function and fix it
sed -i '/seq\[0\]\.Type = GDOCS_FOLDER_TYPE;/{
s/seq\[0\]/seq.getArray()[0]/g
}' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx"

sed -i '/seq\[0\]\.Attributes/{
s/seq\[0\]/seq.getArray()[0]/g
}' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx"

sed -i '/seq\[0\]\.Properties/{
s/seq\[0\]/seq.getArray()[0]/g
}' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx"

sed -i '/seq\[1\]\.Type = GDOCS_FILE_TYPE;/{
s/seq\[1\]/seq.getArray()[1]/g
}' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx"

sed -i '/seq\[1\]\.Attributes/{
s/seq\[1\]/seq.getArray()[1]/g
}' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx"

sed -i '/seq\[1\]\.Properties/{
s/seq\[1\]/seq.getArray()[1]/g
}' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx"

echo "Done!"