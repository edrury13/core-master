#!/bin/bash
# Fix content provider compilation errors

WSL_DEST="$HOME/libreoffice/core-master"

# Add missing include for cppu::queryInterface
sed -i '/#include "gdocs_provider.hxx"/a #include <cppuhelper/queryinterface.hxx>' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_provider.cxx"

# Add missing include for cppu::OTypeCollection  
sed -i '/#include <cppuhelper\/queryinterface.hxx>/a #include <cppuhelper/typeprovider.hxx>' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_provider.cxx"

# Fix getCommands pure virtual function in gdocs_content.hxx
sed -i '/ContentRefList getChildren/a\
    virtual css::uno::Sequence< css::ucb::CommandInfo >\
    getCommands( const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv ) override;' "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_content.hxx"

echo "Fixed content provider issues"