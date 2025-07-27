/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ustring.hxx>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/document/XImporter.hpp>
#include <com/sun/star/document/XExporter.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/presentation/XPresentationSupplier.hpp>
#include <cppuhelper/implbase.hxx>
#include <memory>
#include <vector>

namespace gdocs
{

struct GoogleSession;

/**
 * Converter for Google Slides to/from ODF Impress format
 * Handles conversion between Google Slides and LibreOffice Impress presentations
 */
class SlideConverter : public cppu::WeakImplHelper<
                              css::document::XFilter,
                              css::document::XImporter,
                              css::document::XExporter,
                              css::lang::XServiceInfo>
{
private:
    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    css::uno::Reference<css::lang::XComponent> m_xTargetComponent;
    
    // Google Slides export MIME types
    static constexpr const char* GSLIDE_EXPORT_PPTX = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    static constexpr const char* GSLIDE_EXPORT_ODP = "application/vnd.oasis.opendocument.presentation";
    static constexpr const char* GSLIDE_EXPORT_PDF = "application/pdf";
    static constexpr const char* GSLIDE_EXPORT_JPEG = "image/jpeg";
    static constexpr const char* GSLIDE_EXPORT_PNG = "image/png";
    
    // Helper methods
    bool exportGoogleSlideToOOXML(const std::string& fileId, 
                                  const std::shared_ptr<GoogleSession>& session,
                                  css::uno::Reference<css::io::XOutputStream>& xOutput);
    
    bool importOOXMLToGoogleSlide(css::uno::Reference<css::io::XInputStream>& xInput,
                                 const std::string& fileName,
                                 const std::shared_ptr<GoogleSession>& session,
                                 std::string& outFileId);
    
    bool convertOOXMLToODF(css::uno::Reference<css::io::XInputStream>& xOOXMLInput,
                          css::uno::Reference<css::io::XOutputStream>& xODFOutput);
    
    bool convertODFToOOXML(css::uno::Reference<css::io::XInputStream>& xODFInput,
                          css::uno::Reference<css::io::XOutputStream>& xOOXMLOutput);
    
    css::uno::Reference<css::io::XInputStream> createTempInputStream(const std::vector<sal_Int8>& data);
    std::vector<sal_Int8> readFromInputStream(css::uno::Reference<css::io::XInputStream>& xInput);
    
    // Presentation processing helpers
    bool processSlides(const css::uno::Reference<css::presentation::XPresentationSupplier>& xPresSupplier);
    bool processSlideMaster(const css::uno::Reference<css::drawing::XDrawPage>& xMasterPage);
    bool processSlideLayouts(const css::uno::Reference<css::drawing::XMasterPagesSupplier>& xMasterSupplier);
    bool processAnimations(const css::uno::Reference<css::drawing::XDrawPage>& xSlide);
    bool processTransitions(const css::uno::Reference<css::drawing::XDrawPage>& xSlide);
    bool processShapes(const css::uno::Reference<css::drawing::XDrawPage>& xSlide);
    bool processTextBoxes(const css::uno::Reference<css::drawing::XShape>& xShape);
    bool processImages(const css::uno::Reference<css::drawing::XDrawPage>& xSlide);
    bool processCharts(const css::uno::Reference<css::drawing::XDrawPage>& xSlide);
    
public:
    explicit SlideConverter(const css::uno::Reference<css::uno::XComponentContext>& xContext);
    virtual ~SlideConverter() override;
    
    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;
    
    // XFilter
    virtual sal_Bool SAL_CALL filter(const css::uno::Sequence<css::beans::PropertyValue>& aDescriptor) override;
    virtual void SAL_CALL cancel() override;
    
    // XImporter
    virtual void SAL_CALL setTargetDocument(const css::uno::Reference<css::lang::XComponent>& xDoc) override;
    
    // XExporter
    virtual void SAL_CALL setSourceDocument(const css::uno::Reference<css::lang::XComponent>& xDoc) override;
    
    // Public conversion methods
    bool importGoogleSlide(const std::string& fileId, 
                          const std::shared_ptr<GoogleSession>& session);
    
    bool exportToGoogleSlide(const std::string& fileName,
                            const std::shared_ptr<GoogleSession>& session,
                            const std::string& parentFolderId = "");
    
    // Utility methods
    static bool isGoogleSlideMimeType(const std::string& mimeType);
    static OUString getServiceName();
    
    // Slide-specific helpers
    bool processSlideNotes(const css::uno::Reference<css::drawing::XDrawPage>& xNotesPage);
    bool processSlideComments(const css::uno::Reference<css::drawing::XDrawPage>& xSlide);
    css::uno::Reference<css::drawing::XShape> createTextBox(
        const css::uno::Reference<css::drawing::XDrawPage>& xSlide,
        const css::awt::Rectangle& position,
        const OUString& text);
};

// Service factory functions
css::uno::Reference<css::uno::XInterface> SAL_CALL
SlideConverter_createInstance(const css::uno::Reference<css::uno::XComponentContext>& xContext);

css::uno::Sequence<OUString> SAL_CALL
SlideConverter_getSupportedServiceNames();

OUString SAL_CALL
SlideConverter_getImplementationName();

} // namespace gdocs

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */