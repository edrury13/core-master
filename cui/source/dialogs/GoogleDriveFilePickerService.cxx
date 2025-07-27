/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "GoogleDriveFilePickerService.hxx"
#include "GoogleDriveFilePicker.hxx"
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>
#include <com/sun/star/ui/dialogs/CommonFilePickerElementIds.hpp>
#include <cppuhelper/supportsservice.hxx>
#include <vcl/svapp.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <cstdio>

using namespace css;
using namespace css::uno;
using namespace css::ui::dialogs;

GoogleDriveFilePickerService::GoogleDriveFilePickerService(const Reference<XComponentContext>& rxContext)
    : GoogleDriveFilePickerServiceBase(m_aMutex)
    , m_xContext(rxContext)
    , m_nDialogResult(ExecutableDialogResults::CANCEL)
{
    fprintf(stderr, "\n*** GoogleDriveFilePickerService CONSTRUCTOR ***\n");
    FILE* f = fopen("/tmp/gdrive_service.log", "w");
    if (f) {
        fprintf(f, "GoogleDriveFilePickerService created\n");
        fclose(f);
    }
}

GoogleDriveFilePickerService::~GoogleDriveFilePickerService()
{
}

// XServiceInfo
OUString SAL_CALL GoogleDriveFilePickerService::getImplementationName()
{
    return u"com.sun.star.comp.cui.GoogleDriveFilePicker"_ustr;
}

sal_Bool SAL_CALL GoogleDriveFilePickerService::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

Sequence<OUString> SAL_CALL GoogleDriveFilePickerService::getSupportedServiceNames()
{
    return { u"com.sun.star.ui.dialogs.GoogleDriveFilePicker"_ustr };
}

// XInitialization
void SAL_CALL GoogleDriveFilePickerService::initialize(const Sequence<Any>& /*aArguments*/)
{
    // Nothing to initialize for now
}

// XFilePicker
void SAL_CALL GoogleDriveFilePickerService::setMultiSelectionMode(sal_Bool /*bMode*/)
{
    // Google Drive picker currently supports single selection only
}

void SAL_CALL GoogleDriveFilePickerService::setDefaultName(const OUString& /*aName*/)
{
    // Not applicable for Google Drive picker
}

void SAL_CALL GoogleDriveFilePickerService::setDisplayDirectory(const OUString& aDirectory)
{
    m_sDisplayDirectory = aDirectory;
}

OUString SAL_CALL GoogleDriveFilePickerService::getDisplayDirectory()
{
    return m_sDisplayDirectory;
}

Sequence<OUString> SAL_CALL GoogleDriveFilePickerService::getFiles()
{
    return getSelectedFiles();
}

Sequence<OUString> SAL_CALL GoogleDriveFilePickerService::getSelectedFiles()
{
    return m_aSelectedFiles;
}

// XExecutableDialog
void SAL_CALL GoogleDriveFilePickerService::setTitle(const OUString& /*aTitle*/)
{
    // Title is fixed for Google Drive picker
}

sal_Int16 SAL_CALL GoogleDriveFilePickerService::execute()
{
    SAL_WARN("cui.dialogs", "GoogleDriveFilePickerService::execute() called");
    fprintf(stderr, "\n*** GoogleDriveFilePickerService::execute() CALLED ***\n");
    SolarMutexGuard aGuard;
    
    try
    {
        // Get parent window
        weld::Window* pParentWindow = Application::GetFrameWeld(
            css::uno::Reference<css::awt::XWindow>());
        
        // Create and run the dialog
        SAL_WARN("cui.dialogs", "About to create GoogleDriveFilePicker dialog");
        GoogleDriveFilePicker aDlg(pParentWindow, m_xContext);
        SAL_WARN("cui.dialogs", "GoogleDriveFilePicker dialog created, about to run");
        
        m_nDialogResult = aDlg.run();
        
        if (m_nDialogResult == RET_OK)
        {
            OUString sFileUrl = aDlg.GetSelectedFileUrl();
            if (!sFileUrl.isEmpty())
            {
                m_aSelectedFiles = { sFileUrl };
                return ExecutableDialogResults::OK;
            }
        }
    }
    catch (const Exception& e)
    {
        SAL_WARN("cui.dialogs", "GoogleDriveFilePicker execute failed: " << e.Message);
    }
    
    return ExecutableDialogResults::CANCEL;
}

// XFilePicker2
void SAL_CALL GoogleDriveFilePickerService::cancel()
{
    // TODO: Implement cancellation if dialog is running
}

// XFilePickerNotifier
void SAL_CALL GoogleDriveFilePickerService::addFilePickerListener(
    const Reference<XFilePickerListener>& /*xListener*/)
{
    // Not implemented for now
}

void SAL_CALL GoogleDriveFilePickerService::removeFilePickerListener(
    const Reference<XFilePickerListener>& /*xListener*/)
{
    // Not implemented for now
}

// XFilterManager
void SAL_CALL GoogleDriveFilePickerService::appendFilter(const OUString& /*aTitle*/, 
                                                        const OUString& /*aFilter*/)
{
    // Google Drive handles its own filtering
}

void SAL_CALL GoogleDriveFilePickerService::setCurrentFilter(const OUString& /*aTitle*/)
{
    // Google Drive handles its own filtering
}

OUString SAL_CALL GoogleDriveFilePickerService::getCurrentFilter()
{
    return OUString();
}

// XFilterGroupManager
void SAL_CALL GoogleDriveFilePickerService::appendFilterGroup(const OUString& /*sGroupTitle*/,
    const Sequence<beans::StringPair>& /*aFilters*/)
{
    // Google Drive handles its own filtering
}

// XFilePickerControlAccess
void SAL_CALL GoogleDriveFilePickerService::setValue(sal_Int16 /*nControlId*/, 
                                                    sal_Int16 /*nControlAction*/, 
                                                    const Any& /*aValue*/)
{
    // Not implemented for Google Drive picker
}

Any SAL_CALL GoogleDriveFilePickerService::getValue(sal_Int16 /*nControlId*/, 
                                                   sal_Int16 /*nControlAction*/)
{
    return Any();
}

void SAL_CALL GoogleDriveFilePickerService::enableControl(sal_Int16 /*nControlId*/, 
                                                         sal_Bool /*bEnable*/)
{
    // Not implemented for Google Drive picker
}

void SAL_CALL GoogleDriveFilePickerService::setLabel(sal_Int16 /*nControlId*/, 
                                                    const OUString& /*aLabel*/)
{
    // Not implemented for Google Drive picker
}

OUString SAL_CALL GoogleDriveFilePickerService::getLabel(sal_Int16 /*nControlId*/)
{
    return OUString();
}

// XFilePreview
Sequence<sal_Int16> SAL_CALL GoogleDriveFilePickerService::getSupportedImageFormats()
{
    return Sequence<sal_Int16>();
}

sal_Int32 SAL_CALL GoogleDriveFilePickerService::getTargetColorDepth()
{
    return 0;
}

sal_Int32 SAL_CALL GoogleDriveFilePickerService::getAvailableWidth()
{
    return 0;
}

sal_Int32 SAL_CALL GoogleDriveFilePickerService::getAvailableHeight()
{
    return 0;
}

void SAL_CALL GoogleDriveFilePickerService::setImage(sal_Int16 /*aImageFormat*/, 
                                                    const Any& /*aImage*/)
{
    // Not implemented for Google Drive picker
}

sal_Bool SAL_CALL GoogleDriveFilePickerService::setShowState(sal_Bool /*bShowState*/)
{
    return false;
}

sal_Bool SAL_CALL GoogleDriveFilePickerService::getShowState()
{
    return false;
}

// Factory function
extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
cui_GoogleDriveFilePicker_get_implementation(
    css::uno::XComponentContext* context,
    css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new GoogleDriveFilePickerService(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */