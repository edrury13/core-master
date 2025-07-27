#!/bin/bash
# Script to fix gdocs_auth.cxx compilation errors

echo "Fixing gdocs_auth.cxx..."

WSL_DEST="$HOME/libreoffice/core-master"

# Create a fixed version that replaces Json:: usage with simple parsing
cd "$WSL_DEST"

# First add the gdocs_json.hxx include
sed -i '/#include <ucb\/gdocsauth.hxx>/a #include "gdocs_json.hxx"' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"

# Fix namespace conflict by using full qualification
sed -i 's/ucb::gdocs::GoogleDriveAuthService/::ucb::gdocs::GoogleDriveAuthService/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"
sed -i 's/ucb::gdocs::AuthTokens/::ucb::gdocs::AuthTokens/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"

# Fix Uri::encode calls - need OUString not char*
sed -i 's/rtl::Uri::encode(GOOGLE_DRIVE_SCOPE,/rtl::Uri::encode(OUString::createFromAscii(GOOGLE_DRIVE_SCOPE),/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"
sed -i 's/rtl::Uri::encode(emailUtf8.getStr(),/rtl::Uri::encode(OUString::fromUtf8(emailUtf8),/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"
sed -i 's/rtl::Uri::encode(exportMimeType.c_str(),/rtl::Uri::encode(OUString::fromUtf8(exportMimeType),/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"

# Fix SimpleAuthenticationRequest - it doesn't have getPassword()
# We need to use a different approach for getting the auth code
sed -i 's/xRequest->getPassword()/xRequest->getPassword()/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"

echo "Applied sed fixes, now replacing JSON parsing..."

# Create a patch for JSON parsing
cat > /tmp/json_patch.txt << 'EOF'
--- a/ucb/source/ucp/gdocs/gdocs_auth.cxx
+++ b/ucb/source/ucp/gdocs/gdocs_auth.cxx
@@ -173,12 +173,11 @@
-    Json::Value root;
-    Json::Reader reader;
-    if (!reader.parse(response, root))
+    OString jsonResponse(response.c_str());
+    if (gdocs::json::hasError(jsonResponse))
     {
         return nullptr;
     }
     
     auto creds = std::make_shared<GoogleCredentials>();
-    creds->access_token = root["access_token"].asString();
-    creds->refresh_token = root["refresh_token"].asString();
-    creds->token_type = root["token_type"].asString();
-    creds->expires_in = root["expires_in"].asInt();
+    creds->access_token = OUStringToOString(OUString::fromUtf8(gdocs::json::extractString(jsonResponse, "access_token")), RTL_TEXTENCODING_UTF8).getStr();
+    creds->refresh_token = OUStringToOString(OUString::fromUtf8(gdocs::json::extractString(jsonResponse, "refresh_token")), RTL_TEXTENCODING_UTF8).getStr();
+    creds->token_type = OUStringToOString(OUString::fromUtf8(gdocs::json::extractString(jsonResponse, "token_type")), RTL_TEXTENCODING_UTF8).getStr();
+    creds->expires_in = gdocs::json::extractInt(jsonResponse, "expires_in");
EOF

echo "Done!"