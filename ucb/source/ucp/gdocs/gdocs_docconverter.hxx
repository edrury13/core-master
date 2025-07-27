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
#include <cppuhelper/implbase.hxx>
#include <memory>
#include <vector>

namespace gdocs
{

struct GoogleSession;

/**
 * Converter for Google Docs to/from ODF Writer format
 * Handles conversion between Google Docs and LibreOffice Writer documents
 */
class DocConverter : public cppu::WeakImplHelper<
                            css::document::XFilter,
                            css::document::XImporter,
                            css::document::XExporter,
                            css::lang::XServiceInfo>
{
private:
    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    css::uno::Reference<css::lang::XComponent> m_xTargetComponent;
    
    // Google Drive export MIME types for different formats
    static constexpr const char* GDOC_EXPORT_DOCX = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    static constexpr const char* GDOC_EXPORT_ODT = "application/vnd.oasis.opendocument.text";
    static constexpr const char* GDOC_EXPORT_RTF = "application/rtf";
    static constexpr const char* GDOC_EXPORT_PDF = "application/pdf";
    
    // Helper methods
    bool exportGoogleDocToOOXML(const std::string& fileId, 
                                const std::shared_ptr<GoogleSession>& session,
                                css::uno::Reference<css::io::XOutputStream>& xOutput);
    
    bool importOOXMLToGoogleDoc(css::uno::Reference<css::io::XInputStream>& xInput,
                               const std::string& fileName,
                               const std::shared_ptr<GoogleSession>& session,
                               std::string& outFileId);
    
    bool convertOOXMLToODF(css::uno::Reference<css::io::XInputStream>& xOOXMLInput,
                          css::uno::Reference<css::io::XOutputStream>& xODFOutput);
    
    bool convertODFToOOXML(css::uno::Reference<css::io::XInputStream>& xODFInput,
                          css::uno::Reference<css::io::XOutputStream>& xOOXMLOutput);
    
    css::uno::Reference<css::io::XInputStream> createTempInputStream(const std::vector<sal_Int8>& data);
    std::vector<sal_Int8> readFromInputStream(css::uno::Reference<css::io::XInputStream>& xInput);
    
    // Document processing helpers
    bool processTextContent(const css::uno::Reference<css::text::XTextDocument>& xTextDoc);
    bool processStyles(const css::uno::Reference<css::style::XStyleFamiliesSupplier>& xStyleSupplier);
    bool processImages(const css::uno::Reference<css::text::XTextDocument>& xTextDoc);
    bool processTables(const css::uno::Reference<css::text::XTextDocument>& xTextDoc);
    
public:
    explicit DocConverter(const css::uno::Reference<css::uno::XComponentContext>& xContext);
    virtual ~DocConverter() override;
    
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
    bool importGoogleDoc(const std::string& fileId, 
                        const std::shared_ptr<GoogleSession>& session);
    
    bool exportToGoogleDoc(const std::string& fileName,
                          const std::shared_ptr<GoogleSession>& session,
                          const std::string& parentFolderId = "");
    
    // Utility methods
    static bool isGoogleDocMimeType(const std::string& mimeType);
    static OUString getServiceName();
};

// Service factory functions
css::uno::Reference<css::uno::XInterface> SAL_CALL
DocConverter_createInstance(const css::uno::Reference<css::uno::XComponentContext>& xContext);

css::uno::Sequence<OUString> SAL_CALL
DocConverter_getSupportedServiceNames();

OUString SAL_CALL
DocConverter_getImplementationName();

} // namespace gdocs

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */