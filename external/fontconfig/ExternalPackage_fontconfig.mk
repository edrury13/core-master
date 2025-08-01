# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalPackage_ExternalPackage,fontconfig,fontconfig))

$(eval $(call gb_ExternalPackage_use_external_project,fontconfig,fontconfig))

ifeq ($(DISABLE_DYNLOADING),)
$(eval $(call gb_ExternalPackage_add_file,fontconfig,$(LIBO_LIB_FOLDER)/libfontconfig-lo.so.1.16.1,src/.libs/libfontconfig-lo.so.1.16.1))
endif

# vim: set noet sw=4 ts=4:
