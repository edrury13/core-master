/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SFX2_GDRIVESYNC_HXX
#define INCLUDED_SFX2_GDRIVESYNC_HXX

#include <sfx2/dllapi.h>
#include <rtl/ustring.hxx>
#include <map>
#include <memory>

namespace sfx2 {

struct GoogleDriveFileInfo
{
    OUString fileId;
    OUString localPath;
    OUString mimeType;
    OUString originalName;
    bool needsUpload = false;
};

class SFX2_DLLPUBLIC GoogleDriveSync
{
public:
    static GoogleDriveSync& getInstance();
    
    // Register a downloaded file
    void registerFile(const OUString& localPath, const OUString& fileId, 
                      const OUString& mimeType, const OUString& originalName);
    
    // Check if a file is from Google Drive
    bool isGoogleDriveFile(const OUString& localPath) const;
    
    // Get Google Drive info for a local file
    std::shared_ptr<GoogleDriveFileInfo> getFileInfo(const OUString& localPath) const;
    
    // Mark file as needing upload
    void markForUpload(const OUString& localPath);
    
    // Upload file back to Google Drive
    bool uploadFile(const OUString& localPath);
    
    // Remove file from tracking (on close)
    void unregisterFile(const OUString& localPath);
    
private:
    GoogleDriveSync() = default;
    ~GoogleDriveSync() = default;
    
    std::map<OUString, std::shared_ptr<GoogleDriveFileInfo>> m_fileMap;
};

} // namespace sfx2

#endif // INCLUDED_SFX2_GDRIVESYNC_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */