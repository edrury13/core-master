#!/bin/bash
# Fix Library_ucpgdocs1.mk

WSL_DEST="$HOME/libreoffice/core-master"

echo "Fixing Library_ucpgdocs1.mk..."

# Remove the o3tl line we added
sed -i '/o3tl \\/d' "$WSL_DEST/ucb/Library_ucpgdocs1.mk"

# o3tl is header-only, we don't need to link against it
# Just make sure we have all the libraries we actually need

cat > "$WSL_DEST/ucb/Library_ucpgdocs1.mk" << 'EOF'
# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,ucpgdocs1))

$(eval $(call gb_Library_set_componentfile,ucpgdocs1,ucb/source/ucp/gdocs/ucpgdocs1))

$(eval $(call gb_Library_use_externals,ucpgdocs1,\
	boost_headers \
	curl \
))

$(eval $(call gb_Library_use_libraries,ucpgdocs1,\
	comphelper \
	cppu \
	cppuhelper \
	sal \
	salhelper \
	tl \
	ucbhelper \
	utl \
	vcl \
))

$(eval $(call gb_Library_use_sdk_api,ucpgdocs1))

$(eval $(call gb_Library_add_exception_objects,ucpgdocs1,\
	ucb/source/ucp/gdocs/gdocs_provider \
	ucb/source/ucp/gdocs/gdocs_content \
	ucb/source/ucp/gdocs/gdocs_auth \
	ucb/source/ucp/gdocs/gdocs_authservice \
	ucb/source/ucp/gdocs/gdocs_datasupplier \
	ucb/source/ucp/gdocs/gdocs_docconverter \
	ucb/source/ucp/gdocs/gdocs_sheetconverter \
	ucb/source/ucp/gdocs/gdocs_slideconverter \
))

# vim: set noet sw=4 ts=4:
EOF

echo "Done!"