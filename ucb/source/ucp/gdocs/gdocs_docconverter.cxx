/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>
#include <sal/log.hxx>

#include "gdocs_docconverter.hxx"
#include "gdocs_auth.hxx"

#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/io/XTruncate.hpp>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/document/FilterOptionsRequest.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/text/XTextContent.hpp>
#include <com/sun/star/text/XTextCursor.hpp>
#include <com/sun/star/style/XStyleFamiliesSupplier.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/table/XTableRows.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XDesktop2.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/text/XTextGraphicObjectsSupplier.hpp>
#include <com/sun/star/text/XTextTablesSupplier.hpp>
#include <com/sun/star/text/XTextTable.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertysequence.hxx>
#include <comphelper/storagehelper.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <tools/stream.hxx>
#include <unotools/streamwrap.hxx>
#include <unotools/tempfile.hxx>
#include <rtl/string.hxx>
#include <rtl/ustrbuf.hxx>
#include <com/sun/star/io/XSeekableInputStream.hpp>
#include <svtools/sfxecode.hxx>
#include <vcl/svapp.hxx>

using namespace css;

namespace gdocs
{

DocConverter::DocConverter(const uno::Reference<uno::XComponentContext>& xContext)
    : m_xContext(xContext)
{
}

DocConverter::~DocConverter()
{
}

// XServiceInfo
OUString SAL_CALL DocConverter::getImplementationName()
{
    return DocConverter_getImplementationName();
}

sal_Bool SAL_CALL DocConverter::supportsService(const OUString& ServiceName)
{
    const uno::Sequence<OUString> aServiceNames = getSupportedServiceNames();
    for (const OUString& rService : aServiceNames)
    {
        if (ServiceName == rService)
            return true;
    }
    return false;
}

uno::Sequence<OUString> SAL_CALL DocConverter::getSupportedServiceNames()
{
    return DocConverter_getSupportedServiceNames();
}

// XFilter
sal_Bool SAL_CALL DocConverter::filter(const uno::Sequence<beans::PropertyValue>& aDescriptor)
{
    comphelper::SequenceAsHashMap aMap(aDescriptor);
    
    // Extract file parameters
    OUString sURL = aMap.getUnpackedValueOrDefault("URL", OUString());
    uno::Reference<io::XInputStream> xInputStream = aMap.getUnpackedValueOrDefault("InputStream", uno::Reference<io::XInputStream>());
    uno::Reference<io::XOutputStream> xOutputStream = aMap.getUnpackedValueOrDefault("OutputStream", uno::Reference<io::XOutputStream>());
    
    // Check if this is a Google Docs URL
    if (sURL.startsWith("gdocs://") || sURL.startsWith("https://docs.google.com/document/"))
    {
        // Parse Google Docs URL to extract file ID and session info
        OUString sFileId;
        std::shared_ptr<GoogleSession> pSession;
        
        if (sURL.startsWith("https://docs.google.com/document/"))
        {
            // Extract file ID from Google Docs URL
            // Format: https://docs.google.com/document/d/FILE_ID/edit
            sal_Int32 nStart = sURL.indexOf("/d/");
            if (nStart >= 0)
            {
                nStart += 3; // Skip "/d/"
                sal_Int32 nEnd = sURL.indexOf('/', nStart);
                if (nEnd > nStart)
                {
                    sFileId = sURL.copy(nStart, nEnd - nStart);
                }
                else
                {
                    sFileId = sURL.copy(nStart);
                }
            }
            
            // Create session with empty user (will prompt for auth)
            pSession = createGoogleSession(OUString());
        }
        else
        {
            // Extract file ID from custom URL (format: gdocs://user@domain/file_id)
            sal_Int32 nSlashPos = sURL.lastIndexOf('/');
            if (nSlashPos > 0)
            {
                sFileId = sURL.copy(nSlashPos + 1);
                OUString sUserInfo = sURL.copy(8, nSlashPos - 8); // Skip "gdocs://"
                
                // Create or get session for user
                pSession = createGoogleSession(sUserInfo);
            }
        }
        
        if (pSession && !sFileId.isEmpty() && xOutputStream.is())
        {
            return exportGoogleDocToOOXML(OUStringToOString(sFileId, RTL_TEXTENCODING_UTF8).getStr(), 
                                        pSession, xOutputStream);
        }
    }
    else if (xInputStream.is())
    {
        // Import from stream - convert OOXML to ODF
        if (xOutputStream.is())
        {
            return convertOOXMLToODF(xInputStream, xOutputStream);
        }
    }
    
    return false;
}

void SAL_CALL DocConverter::cancel()
{
    // Implementation for cancelling ongoing operations
}

// XImporter
void SAL_CALL DocConverter::setTargetDocument(const uno::Reference<lang::XComponent>& xDoc)
{
    m_xTargetComponent = xDoc;
}

// XExporter
void SAL_CALL DocConverter::setSourceDocument(const uno::Reference<lang::XComponent>& xDoc)
{
    m_xTargetComponent = xDoc;
}

bool DocConverter::exportGoogleDocToOOXML(const std::string& fileId, 
                                         const std::shared_ptr<GoogleSession>& session,
                                         uno::Reference<io::XOutputStream>& xOutput)
{
    try
    {
        // Use Google Drive API to export document in DOCX format
        std::vector<char> docxData = exportGoogleDocToMemory(*session, fileId, EXPORT_DOCX_MIMETYPE);
        
        // Write data to output stream
        if (!docxData.empty())
        {
            uno::Sequence<sal_Int8> aSeq(reinterpret_cast<const sal_Int8*>(docxData.data()), docxData.size());
            xOutput->writeBytes(aSeq);
            
            // Now we need to use the existing DOCX import filter to convert to ODF
            // Create a temporary stream with the DOCX data
            uno::Reference<io::XSeekableInputStream> xTempInput = 
                uno::Reference<io::XSeekableInputStream>(
                    new ::utl::OSeekableInputStreamWrapper(
                        new SvMemoryStream(docxData.data(), docxData.size(), StreamMode::READ)));
            
            // Use the DOCX import filter
            uno::Reference<document::XFilter> xDocxFilter(
                m_xContext->getServiceManager()->createInstanceWithContext(
                    "com.sun.star.comp.Writer.WriterFilter", m_xContext),
                uno::UNO_QUERY);
            
            if (xDocxFilter.is() && m_xTargetComponent.is())
            {
                uno::Reference<document::XImporter> xImporter(xDocxFilter, uno::UNO_QUERY);
                if (xImporter.is())
                {
                    xImporter->setTargetDocument(m_xTargetComponent);
                    
                    // Create filter descriptor
                    uno::Sequence<beans::PropertyValue> aFilterDesc(comphelper::InitPropertySequence({
                        {"InputStream", uno::Any(xTempInput)},
                        {"FilterName", uno::Any(OUString("MS Word 2007 XML"))}
                    }));
                    
                    return xDocxFilter->filter(aFilterDesc);
                }
            }
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error exporting Google Doc: " << e.Message);
    }
    
    return false;
}

bool DocConverter::importOOXMLToGoogleDoc(uno::Reference<io::XInputStream>& xInput,
                                         const std::string& fileName,
                                         const std::shared_ptr<GoogleSession>& session,
                                         std::string& outFileId)
{
    try
    {
        // Read OOXML data from input stream
        std::vector<sal_Int8> ooxmlData = readFromInputStream(xInput);
        
        if (!ooxmlData.empty())
        {
            // Upload to Google Drive as DOCX, which will be converted to Google Docs format
            outFileId = uploadFile(*session, "", fileName, GDOC_EXPORT_DOCX, "");
            return !outFileId.empty();
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error importing to Google Doc: " << e.Message);
    }
    
    return false;
}

bool DocConverter::convertOOXMLToODF(uno::Reference<io::XInputStream>& xOOXMLInput,
                                    uno::Reference<io::XOutputStream>& xODFOutput)
{
    try
    {
        // Use LibreOffice's built-in OOXML import and ODF export filters
        uno::Reference<frame::XDesktop2> xDesktop = frame::Desktop::create(m_xContext);
        
        // Create load properties for OOXML
        uno::Sequence<beans::PropertyValue> aLoadProps = comphelper::InitPropertySequence({
            {"InputStream", uno::Any(xOOXMLInput)},
            {"FilterName", uno::Any(OUString("MS Word 2007 XML"))},
            {"Hidden", uno::Any(true)}
        });
        
        // Load OOXML document
        uno::Reference<lang::XComponent> xDoc = xDesktop->loadComponentFromURL(
            "private:stream", "_blank", 0, aLoadProps);
            
        if (xDoc.is())
        {
            // Get document model
            uno::Reference<frame::XModel> xModel(xDoc, uno::UNO_QUERY);
            if (xModel.is())
            {
                // Create export properties for ODF
                uno::Sequence<beans::PropertyValue> aStoreProps = comphelper::InitPropertySequence({
                    {"OutputStream", uno::Any(xODFOutput)},
                    {"FilterName", uno::Any(OUString("writer8"))},
                    {"Overwrite", uno::Any(true)}
                });
                
                // Export as ODF
                uno::Reference<frame::XStorable> xStorable(xModel, uno::UNO_QUERY);
                if (xStorable.is())
                {
                    xStorable->storeToURL("private:stream", aStoreProps);
                    
                    // Process document content if needed
                    uno::Reference<text::XTextDocument> xTextDoc(xDoc, uno::UNO_QUERY);
                    if (xTextDoc.is())
                    {
                        processTextContent(xTextDoc);
                    }
                    
                    xDoc->dispose();
                    return true;
                }
            }
            xDoc->dispose();
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error converting OOXML to ODF: " << e.Message);
    }
    
    return false;
}

bool DocConverter::convertODFToOOXML(uno::Reference<io::XInputStream>& xODFInput,
                                    uno::Reference<io::XOutputStream>& xOOXMLOutput)
{
    try
    {
        uno::Reference<frame::XDesktop2> xDesktop = frame::Desktop::create(m_xContext);
        
        // Load ODF document
        uno::Sequence<beans::PropertyValue> aLoadProps = comphelper::InitPropertySequence({
            {"InputStream", uno::Any(xODFInput)},
            {"Hidden", uno::Any(true)}
        });
        
        uno::Reference<lang::XComponent> xDoc = xDesktop->loadComponentFromURL(
            "private:stream", "_blank", 0, aLoadProps);
            
        if (xDoc.is())
        {
            uno::Reference<frame::XModel> xModel(xDoc, uno::UNO_QUERY);
            if (xModel.is())
            {
                // Export as OOXML
                uno::Sequence<beans::PropertyValue> aStoreProps = comphelper::InitPropertySequence({
                    {"OutputStream", uno::Any(xOOXMLOutput)},
                    {"FilterName", uno::Any(OUString("MS Word 2007 XML"))},
                    {"Overwrite", uno::Any(true)}
                });
                
                uno::Reference<frame::XStorable> xStorable(xModel, uno::UNO_QUERY);
                if (xStorable.is())
                {
                    xStorable->storeToURL("private:stream", aStoreProps);
                    xDoc->dispose();
                    return true;
                }
            }
            xDoc->dispose();
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error converting ODF to OOXML: " << e.Message);
    }
    
    return false;
}

uno::Reference<io::XInputStream> DocConverter::createTempInputStream(const std::vector<sal_Int8>& data)
{
    uno::Reference<io::XInputStream> xResult;
    
    try
    {
        SvMemoryStream* pMemoryStream = new SvMemoryStream(
            const_cast<void*>(reinterpret_cast<const void*>(data.data())), 
            data.size(), 
            StreamMode::READ);
        xResult = new utl::OInputStreamWrapper(pMemoryStream, true);
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error creating temp input stream: " << e.Message);
    }
    
    return xResult;
}

std::vector<sal_Int8> DocConverter::readFromInputStream(uno::Reference<io::XInputStream>& xInput)
{
    std::vector<sal_Int8> result;
    
    try
    {
        const sal_Int32 nBufferSize = 8192;
        uno::Sequence<sal_Int8> aBuffer(nBufferSize);
        sal_Int32 nBytesRead = 0;
        
        do
        {
            nBytesRead = xInput->readBytes(aBuffer, nBufferSize);
            if (nBytesRead > 0)
            {
                result.insert(result.end(), aBuffer.getConstArray(), 
                             aBuffer.getConstArray() + nBytesRead);
            }
        }
        while (nBytesRead == nBufferSize);
        
        xInput->closeInput();
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error reading from input stream: " << e.Message);
    }
    
    return result;
}

bool DocConverter::processTextContent(const uno::Reference<text::XTextDocument>& xTextDoc)
{
    try
    {
        if (!xTextDoc.is())
            return false;
            
        uno::Reference<text::XText> xText = xTextDoc->getText();
        if (!xText.is())
            return false;
            
        // Get text cursor for document content
        uno::Reference<text::XTextCursor> xCursor = xText->createTextCursor();
        if (!xCursor.is())
            return false;
            
        // Process document structure, styles, etc.
        uno::Reference<style::XStyleFamiliesSupplier> xStyleSupplier(xTextDoc, uno::UNO_QUERY);
        if (xStyleSupplier.is())
        {
            processStyles(xStyleSupplier);
        }
        
        // Process images
        processImages(xTextDoc);
        
        // Process tables
        processTables(xTextDoc);
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing text content: " << e.Message);
    }
    
    return false;
}

bool DocConverter::processStyles(const uno::Reference<style::XStyleFamiliesSupplier>& xStyleSupplier)
{
    try
    {
        uno::Reference<container::XNameAccess> xStyleFamilies = xStyleSupplier->getStyleFamilies();
        if (!xStyleFamilies.is())
            return false;
            
        // Process paragraph styles
        if (xStyleFamilies->hasByName("ParagraphStyles"))
        {
            uno::Reference<container::XNameAccess> xParaStyles;
            xStyleFamilies->getByName("ParagraphStyles") >>= xParaStyles;
            
            if (xParaStyles.is())
            {
                uno::Sequence<OUString> aStyleNames = xParaStyles->getElementNames();
                for (const OUString& rStyleName : aStyleNames)
                {
                    uno::Reference<beans::XPropertySet> xStyle;
                    xParaStyles->getByName(rStyleName) >>= xStyle;
                    
                    // Process style properties as needed
                    if (xStyle.is())
                    {
                        // Handle font, formatting, etc.
                    }
                }
            }
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing styles: " << e.Message);
    }
    
    return false;
}

bool DocConverter::processImages(const uno::Reference<text::XTextDocument>& xTextDoc)
{
    try
    {
        uno::Reference<text::XTextGraphicObjectsSupplier> xGraphicSupplier(xTextDoc, uno::UNO_QUERY);
        if (!xGraphicSupplier.is())
            return true; // No images to process
            
        uno::Reference<container::XNameAccess> xGraphics = xGraphicSupplier->getTextGraphicObjects();
        if (!xGraphics.is())
            return true;
            
        uno::Sequence<OUString> aGraphicNames = xGraphics->getElementNames();
        for (const OUString& rGraphicName : aGraphicNames)
        {
            uno::Reference<beans::XPropertySet> xGraphic;
            xGraphics->getByName(rGraphicName) >>= xGraphic;
            
            if (xGraphic.is())
            {
                // Process image properties, links, etc.
            }
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing images: " << e.Message);
    }
    
    return false;
}

bool DocConverter::processTables(const uno::Reference<text::XTextDocument>& xTextDoc)
{
    try
    {
        uno::Reference<text::XTextTablesSupplier> xTableSupplier(xTextDoc, uno::UNO_QUERY);
        if (!xTableSupplier.is())
            return true; // No tables to process
            
        uno::Reference<container::XNameAccess> xTables = xTableSupplier->getTextTables();
        if (!xTables.is())
            return true;
            
        uno::Sequence<OUString> aTableNames = xTables->getElementNames();
        for (const OUString& rTableName : aTableNames)
        {
            uno::Reference<text::XTextTable> xTable;
            xTables->getByName(rTableName) >>= xTable;
            
            if (xTable.is())
            {
                // Process table structure, cell content, formatting, etc.
                uno::Reference<table::XCellRange> xCellRange(xTable, uno::UNO_QUERY);
                if (xCellRange.is())
                {
                    // Handle table cells
                }
            }
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing tables: " << e.Message);
    }
    
    return false;
}

bool DocConverter::importGoogleDoc(const std::string& fileId, 
                                  const std::shared_ptr<GoogleSession>& session)
{
    // Implementation for importing Google Doc
    return false;
}

bool DocConverter::exportToGoogleDoc(const std::string& fileName,
                                    const std::shared_ptr<GoogleSession>& session,
                                    const std::string& parentFolderId)
{
    // Implementation for exporting to Google Doc
    return false;
}

bool DocConverter::isGoogleDocMimeType(const std::string& mimeType)
{
    return mimeType == "application/vnd.google-apps.document";
}

OUString DocConverter::getServiceName()
{
    return "com.sun.star.ucb.GoogleDocsConverter";
}

// Service factory functions
uno::Reference<uno::XInterface> SAL_CALL
DocConverter_createInstance(const uno::Reference<uno::XComponentContext>& xContext)
{
    return static_cast<cppu::OWeakObject*>(new DocConverter(xContext));
}

uno::Sequence<OUString> SAL_CALL
DocConverter_getSupportedServiceNames()
{
    return { DocConverter::getServiceName() };
}

OUString SAL_CALL
DocConverter_getImplementationName()
{
    return "com.sun.star.comp.ucb.GoogleDocsConverter";
}

} // namespace gdocs

// Component factory function for UNO service registration
extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
gdocs_DocConverter_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new gdocs::DocConverter(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */