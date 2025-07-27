#!/bin/bash
# Comprehensive fix for UCB build errors

WSL_DEST="$HOME/libreoffice/core-master"
cd "$WSL_DEST"

echo "Applying comprehensive UCB fixes..."

# Fix 1: gdocs_authservice.cxx - namespace issue
echo "Fixing gdocs_authservice.cxx namespace..."
sed -i 's/gdocs::json::/::gdocs::json::/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_authservice.cxx"

# Fix 2: gdocs_auth.cxx - RuntimeException needs OUString
echo "Fixing RuntimeException in gdocs_auth.cxx..."
sed -i 's/throw uno::RuntimeException("HTTP request failed: " + std::string(curl_easy_strerror(res)));/throw uno::RuntimeException(OUString::createFromAscii("HTTP request failed: ") + OUString::createFromAscii(curl_easy_strerror(res)));/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"

# Fix 3: Add missing includes to gdocs_content.cxx
echo "Adding missing includes to gdocs_content.cxx..."
sed -i '/#include "gdocs_content.hxx"/a\
#include <osl/file.hxx>\
#include <com/sun/star/ucb/UnsupportedCommandException.hpp>\
#include <com/sun/star/ucb/MissingInputStreamException.hpp>\
#include <com/sun/star/beans/IllegalTypeException.hpp>' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx"

# Fix 4: Fix ContentInfo assignment issues
echo "Fixing ContentInfo assignments..."
# Create a patch for the queryCreatableContentsInfo function
cat > /tmp/content_fix.patch << 'EOF'
--- a/ucb/source/ucp/gdocs/gdocs_content.cxx
+++ b/ucb/source/ucp/gdocs/gdocs_content.cxx
@@ -319,8 +319,9 @@ uno::Sequence< ucb::ContentInfo > Content::queryCreatableContentsInfo()
     if ( isFolder() )
     {
         uno::Sequence< ucb::ContentInfo > seq( 2 );
-        seq[0].Type = GDOCS_FOLDER_TYPE;
-        seq[0].Attributes = ucb::ContentInfoAttribute::KIND_FOLDER;
+        ucb::ContentInfo& folder = seq.getArray()[0];
+        folder.Type = GDOCS_FOLDER_TYPE;
+        folder.Attributes = ucb::ContentInfoAttribute::KIND_FOLDER;
         
         uno::Sequence< beans::Property > props( 1 );
         props[0] = beans::Property(
@@ -328,11 +329,12 @@ uno::Sequence< ucb::ContentInfo > Content::queryCreatableContentsInfo()
             -1,
             cppu::UnoType<OUString>::get(),
             beans::PropertyAttribute::BOUND );
-        seq[0].Properties = props;
+        folder.Properties = props;
         
-        seq[1].Type = GDOCS_FILE_TYPE;
-        seq[1].Attributes = ucb::ContentInfoAttribute::KIND_DOCUMENT;
-        seq[1].Properties = props;
+        ucb::ContentInfo& file = seq.getArray()[1];
+        file.Type = GDOCS_FILE_TYPE;
+        file.Attributes = ucb::ContentInfoAttribute::KIND_DOCUMENT;
+        file.Properties = props;
         
         return seq;
     }
EOF

# Apply the patch
patch -p1 < /tmp/content_fix.patch

# Fix 5: Add getCommands implementation to gdocs_content.cxx
echo "Adding getCommands implementation..."
cat >> "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx" << 'EOF'

uno::Sequence< ucb::CommandInfo > Content::getCommands(
    const uno::Reference< ucb::XCommandEnvironment > & /*xEnv*/ )
{
    static const ucb::CommandInfo aCommandInfoTable[] =
    {
        // Required commands
        ucb::CommandInfo(
            "getCommandInfo",
            -1,
            cppu::UnoType<void>::get()
        ),
        ucb::CommandInfo(
            "getPropertySetInfo",
            -1,
            cppu::UnoType<void>::get()
        ),
        ucb::CommandInfo(
            "getPropertyValues",
            -1,
            cppu::UnoType<uno::Sequence< beans::Property >>::get()
        ),
        ucb::CommandInfo(
            "setPropertyValues",
            -1,
            cppu::UnoType<uno::Sequence< beans::PropertyValue >>::get()
        ),
        // Optional standard commands
        ucb::CommandInfo(
            "open",
            -1,
            cppu::UnoType<ucb::OpenCommandArgument2>::get()
        ),
        ucb::CommandInfo(
            "insert",
            -1,
            cppu::UnoType<ucb::InsertCommandArgument>::get()
        ),
        ucb::CommandInfo(
            "delete",
            -1,
            cppu::UnoType<bool>::get()
        ),
        ucb::CommandInfo(
            "transfer",
            -1,
            cppu::UnoType<ucb::TransferInfo>::get()
        ),
        ucb::CommandInfo(
            "createNewContent",
            -1,
            cppu::UnoType<ucb::ContentInfo>::get()
        )
    };

    return uno::Sequence< ucb::CommandInfo >(
        aCommandInfoTable,
        sizeof( aCommandInfoTable ) / sizeof( aCommandInfoTable[ 0 ] ) );
}
EOF

# Fix 6: Fix DataSupplier access to private methods
echo "Making Content methods public for DataSupplier..."
sed -i '/std::shared_ptr<GoogleSession> getSession/,/std::shared_ptr<GoogleFile> getGoogleFile/{s/private:/public:/}' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.hxx"

# Fix 7: Add getContext method to Content
sed -i '/ContentRefList getChildren/a\
    css::uno::Reference< css::uno::XComponentContext > getContext() const { return m_xContext; }' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.hxx"

# Fix 8: Fix OpenCommandArgument2 - it doesn't have Environment member
echo "Fixing OpenCommandArgument2 usage..."
sed -i 's/, m_xEnv( rCommand.Environment )//' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_datasupplier.cxx"

# Fix 9: Fix getBadArgExcept() usage
echo "Fixing getBadArgExcept usage..."
sed -i 's/aRetRange\[ n \] <<= getBadArgExcept();/aRetRange[ n ] = uno::Any(getBadArgExcept());/g' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.cxx"

echo "All fixes applied!"