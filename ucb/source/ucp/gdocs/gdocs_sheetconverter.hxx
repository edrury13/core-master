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
#include <com/sun/star/sheet/XSpreadsheetDocument.hpp>
#include <cppuhelper/implbase.hxx>
#include <memory>
#include <vector>

namespace gdocs
{

struct GoogleSession;

/**
 * Converter for Google Sheets to/from ODF Calc format
 * Handles conversion between Google Sheets and LibreOffice Calc spreadsheets
 */
class SheetConverter : public cppu::WeakImplHelper<
                              css::document::XFilter,
                              css::document::XImporter,
                              css::document::XExporter,
                              css::lang::XServiceInfo>
{
private:
    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    css::uno::Reference<css::lang::XComponent> m_xTargetComponent;
    
    // Google Sheets export MIME types
    static constexpr const char* GSHEET_EXPORT_XLSX = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    static constexpr const char* GSHEET_EXPORT_ODS = "application/vnd.oasis.opendocument.spreadsheet";
    static constexpr const char* GSHEET_EXPORT_CSV = "text/csv";
    static constexpr const char* GSHEET_EXPORT_PDF = "application/pdf";
    
    // Helper methods
    bool exportGoogleSheetToOOXML(const std::string& fileId, 
                                  const std::shared_ptr<GoogleSession>& session,
                                  css::uno::Reference<css::io::XOutputStream>& xOutput);
    
    bool importOOXMLToGoogleSheet(css::uno::Reference<css::io::XInputStream>& xInput,
                                 const std::string& fileName,
                                 const std::shared_ptr<GoogleSession>& session,
                                 std::string& outFileId);
    
    bool convertOOXMLToODF(css::uno::Reference<css::io::XInputStream>& xOOXMLInput,
                          css::uno::Reference<css::io::XOutputStream>& xODFOutput);
    
    bool convertODFToOOXML(css::uno::Reference<css::io::XInputStream>& xODFInput,
                          css::uno::Reference<css::io::XOutputStream>& xOOXMLOutput);
    
    css::uno::Reference<css::io::XInputStream> createTempInputStream(const std::vector<sal_Int8>& data);
    std::vector<sal_Int8> readFromInputStream(css::uno::Reference<css::io::XInputStream>& xInput);
    
    // Spreadsheet processing helpers
    bool processSheets(const css::uno::Reference<css::sheet::XSpreadsheetDocument>& xSpreadDoc);
    bool processFormulas(const css::uno::Reference<css::sheet::XSpreadsheet>& xSheet);
    bool processCharts(const css::uno::Reference<css::sheet::XSpreadsheet>& xSheet);
    bool processCellStyles(const css::uno::Reference<css::sheet::XSpreadsheet>& xSheet);
    bool processConditionalFormatting(const css::uno::Reference<css::sheet::XSpreadsheet>& xSheet);
    bool processPivotTables(const css::uno::Reference<css::sheet::XSpreadsheetDocument>& xSpreadDoc);
    
public:
    explicit SheetConverter(const css::uno::Reference<css::uno::XComponentContext>& xContext);
    virtual ~SheetConverter() override;
    
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
    bool importGoogleSheet(const std::string& fileId, 
                          const std::shared_ptr<GoogleSession>& session);
    
    bool exportToGoogleSheet(const std::string& fileName,
                            const std::shared_ptr<GoogleSession>& session,
                            const std::string& parentFolderId = "");
    
    // Utility methods
    static bool isGoogleSheetMimeType(const std::string& mimeType);
    static OUString getServiceName();
    
    // Range and formula helpers
    bool convertExcelFormulasToCalc(const css::uno::Reference<css::sheet::XSpreadsheet>& xSheet);
    bool convertCalcFormulasToExcel(const css::uno::Reference<css::sheet::XSpreadsheet>& xSheet);
    OUString convertRangeReference(const OUString& excelRef, bool toCalc = true);
};

// Service factory functions
css::uno::Reference<css::uno::XInterface> SAL_CALL
SheetConverter_createInstance(const css::uno::Reference<css::uno::XComponentContext>& xContext);

css::uno::Sequence<OUString> SAL_CALL
SheetConverter_getSupportedServiceNames();

OUString SAL_CALL
SheetConverter_getImplementationName();

} // namespace gdocs

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */