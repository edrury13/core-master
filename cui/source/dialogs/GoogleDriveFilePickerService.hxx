/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/ui/dialogs/XFilePicker3.hpp>
#include <com/sun/star/ui/dialogs/XFilePickerControlAccess.hpp>
#include <com/sun/star/ui/dialogs/XFilePreview.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/beans/StringPair.hpp>
#include <cppuhelper/compbase.hxx>
#include <memory>

namespace weld { class Window; }

typedef cppu::WeakComponentImplHelper<
    css::ui::dialogs::XFilePicker3,
    css::ui::dialogs::XFilePickerControlAccess,
    css::ui::dialogs::XFilePreview,
    css::lang::XInitialization,
    css::lang::XServiceInfo> GoogleDriveFilePickerServiceBase;

class GoogleDriveFilePickerService : public GoogleDriveFilePickerServiceBase
{
private:
    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    css::uno::Sequence<OUString> m_aSelectedFiles;
    OUString m_sDisplayDirectory;
    sal_Int16 m_nDialogResult;
    osl::Mutex m_aMutex;
    
public:
    explicit GoogleDriveFilePickerService(const css::uno::Reference<css::uno::XComponentContext>& rxContext);
    virtual ~GoogleDriveFilePickerService() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    // XInitialization
    virtual void SAL_CALL initialize(const css::uno::Sequence<css::uno::Any>& aArguments) override;

    // XFilePicker
    virtual void SAL_CALL setMultiSelectionMode(sal_Bool bMode) override;
    virtual void SAL_CALL setDefaultName(const OUString& aName) override;
    virtual void SAL_CALL setDisplayDirectory(const OUString& aDirectory) override;
    virtual OUString SAL_CALL getDisplayDirectory() override;
    virtual css::uno::Sequence<OUString> SAL_CALL getFiles() override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSelectedFiles() override;

    // XExecutableDialog
    virtual void SAL_CALL setTitle(const OUString& aTitle) override;
    virtual sal_Int16 SAL_CALL execute() override;

    // XFilePicker2
    virtual void SAL_CALL cancel() override;

    // XFilePickerNotifier
    virtual void SAL_CALL addFilePickerListener(
        const css::uno::Reference<css::ui::dialogs::XFilePickerListener>& xListener) override;
    virtual void SAL_CALL removeFilePickerListener(
        const css::uno::Reference<css::ui::dialogs::XFilePickerListener>& xListener) override;

    // XFilterManager
    virtual void SAL_CALL appendFilter(const OUString& aTitle, const OUString& aFilter) override;
    virtual void SAL_CALL setCurrentFilter(const OUString& aTitle) override;
    virtual OUString SAL_CALL getCurrentFilter() override;

    // XFilterGroupManager  
    virtual void SAL_CALL appendFilterGroup(const OUString& sGroupTitle,
        const css::uno::Sequence<css::beans::StringPair>& aFilters) override;

    // XFilePickerControlAccess
    virtual void SAL_CALL setValue(sal_Int16 nControlId, sal_Int16 nControlAction, 
                                  const css::uno::Any& aValue) override;
    virtual css::uno::Any SAL_CALL getValue(sal_Int16 nControlId, sal_Int16 nControlAction) override;
    virtual void SAL_CALL enableControl(sal_Int16 nControlId, sal_Bool bEnable) override;
    virtual void SAL_CALL setLabel(sal_Int16 nControlId, const OUString& aLabel) override;
    virtual OUString SAL_CALL getLabel(sal_Int16 nControlId) override;

    // XFilePreview
    virtual css::uno::Sequence<sal_Int16> SAL_CALL getSupportedImageFormats() override;
    virtual sal_Int32 SAL_CALL getTargetColorDepth() override;
    virtual sal_Int32 SAL_CALL getAvailableWidth() override;
    virtual sal_Int32 SAL_CALL getAvailableHeight() override;
    virtual void SAL_CALL setImage(sal_Int16 aImageFormat, const css::uno::Any& aImage) override;
    virtual sal_Bool SAL_CALL setShowState(sal_Bool bShowState) override;
    virtual sal_Bool SAL_CALL getShowState() override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */