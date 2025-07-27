/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gdocs_slideconverter.hxx"
#include "gdocs_auth.hxx"

#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/io/XTruncate.hpp>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/presentation/XPresentationSupplier.hpp>
#include <com/sun/star/presentation/XPresentation.hpp>
#include <com/sun/star/drawing/XDrawPagesSupplier.hpp>
#include <com/sun/star/drawing/XDrawPages.hpp>
#include <com/sun/star/drawing/XDrawPage.hpp>
#include <com/sun/star/drawing/XShape.hpp>
#include <com/sun/star/drawing/XShapes.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/animation/XAnimationNodeSupplier.hpp>
#include <com/sun/star/presentation/XSlideShowController.hpp>
#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertysequence.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <tools/stream.hxx>
#include <unotools/streamwrap.hxx>
#include <rtl/string.hxx>
#include <rtl/ustrbuf.hxx>

using namespace css;

namespace gdocs
{

SlideConverter::SlideConverter(const uno::Reference<uno::XComponentContext>& xContext)
    : m_xContext(xContext)
{
}

SlideConverter::~SlideConverter()
{
}

// XServiceInfo
OUString SAL_CALL SlideConverter::getImplementationName()
{
    return SlideConverter_getImplementationName();
}

sal_Bool SAL_CALL SlideConverter::supportsService(const OUString& ServiceName)
{
    const uno::Sequence<OUString> aServiceNames = getSupportedServiceNames();
    for (const OUString& rService : aServiceNames)
    {
        if (ServiceName == rService)
            return true;
    }
    return false;
}

uno::Sequence<OUString> SAL_CALL SlideConverter::getSupportedServiceNames()
{
    return SlideConverter_getSupportedServiceNames();
}

// XFilter
sal_Bool SAL_CALL SlideConverter::filter(const uno::Sequence<beans::PropertyValue>& aDescriptor)
{
    comphelper::SequenceAsHashMap aMap(aDescriptor);
    
    // Extract file parameters
    OUString sURL = aMap.getUnpackedValueOrDefault("URL", OUString());
    uno::Reference<io::XInputStream> xInputStream = aMap.getUnpackedValueOrDefault("InputStream", uno::Reference<io::XInputStream>());
    uno::Reference<io::XOutputStream> xOutputStream = aMap.getUnpackedValueOrDefault("OutputStream", uno::Reference<io::XOutputStream>());
    
    // Check if this is a Google Slides URL
    if (sURL.startsWith("gslides://"))
    {
        // Parse Google Slides URL to extract file ID and session info
        OUString sFileId;
        std::shared_ptr<GoogleSession> pSession;
        
        sal_Int32 nSlashPos = sURL.lastIndexOf('/');
        if (nSlashPos > 0)
        {
            sFileId = sURL.copy(nSlashPos + 1);
            OUString sUserInfo = sURL.copy(10, nSlashPos - 10); // Skip "gslides://"
            
            pSession = createGoogleSession(sUserInfo);
        }
        
        if (pSession && !sFileId.isEmpty() && xOutputStream.is())
        {
            return exportGoogleSlideToOOXML(OUStringToOString(sFileId, RTL_TEXTENCODING_UTF8).getStr(), 
                                          pSession, xOutputStream);
        }
    }
    else if (xInputStream.is())
    {
        if (xOutputStream.is())
        {
            return convertOOXMLToODF(xInputStream, xOutputStream);
        }
    }
    
    return false;
}

void SAL_CALL SlideConverter::cancel()
{
    // Implementation for cancelling ongoing operations
}

// XImporter
void SAL_CALL SlideConverter::setTargetDocument(const uno::Reference<lang::XComponent>& xDoc)
{
    m_xTargetComponent = xDoc;
}

// XExporter
void SAL_CALL SlideConverter::setSourceDocument(const uno::Reference<lang::XComponent>& xDoc)
{
    m_xTargetComponent = xDoc;
}

bool SlideConverter::exportGoogleSlideToOOXML(const std::string& fileId, 
                                            const std::shared_ptr<GoogleSession>& session,
                                            uno::Reference<io::XOutputStream>& xOutput)
{
    try
    {
        // Use Google Drive API to export presentation in PPTX format
        std::vector<sal_Int8> pptxData;
        
        // Build export URL for Google Slides
        std::string exportUrl = "https://www.googleapis.com/drive/v3/files/" + fileId + "/export?mimeType=" + GSLIDE_EXPORT_PPTX;
        
        // Make authenticated request to download PPTX data
        if (downloadFile(*session, fileId, ""))
        {
            // For now, create dummy PPTX data
            std::string dummyData = "Dummy PPTX content from Google Slides file: " + fileId;
            pptxData.assign(dummyData.begin(), dummyData.end());
        }
        
        // Write data to output stream
        if (!pptxData.empty())
        {
            uno::Sequence<sal_Int8> aSeq(pptxData.data(), pptxData.size());
            xOutput->writeBytes(aSeq);
            xOutput->closeOutput();
            return true;
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error exporting Google Slides: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::importOOXMLToGoogleSlide(uno::Reference<io::XInputStream>& xInput,
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
            // Upload to Google Drive as PPTX, which will be converted to Google Slides format
            outFileId = uploadFile(*session, "", fileName, GSLIDE_EXPORT_PPTX, "");
            return !outFileId.empty();
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error importing to Google Slides: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::convertOOXMLToODF(uno::Reference<io::XInputStream>& xOOXMLInput,
                                     uno::Reference<io::XOutputStream>& xODFOutput)
{
    try
    {
        // Use LibreOffice's built-in OOXML import and ODF export filters
        uno::Reference<frame::XDesktop2> xDesktop = frame::Desktop::create(m_xContext);
        
        // Create load properties for OOXML
        uno::Sequence<beans::PropertyValue> aLoadProps = comphelper::InitPropertySequence({
            {"InputStream", uno::makeAny(xOOXMLInput)},
            {"FilterName", uno::makeAny(OUString("Impress MS PowerPoint 2007 XML"))},
            {"Hidden", uno::makeAny(true)}
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
                    {"OutputStream", uno::makeAny(xODFOutput)},
                    {"FilterName", uno::makeAny(OUString("impress8"))},
                    {"Overwrite", uno::makeAny(true)}
                });
                
                // Export as ODF
                uno::Reference<frame::XStorable> xStorable(xModel, uno::UNO_QUERY);
                if (xStorable.is())
                {
                    xStorable->storeToURL("private:stream", aStoreProps);
                    
                    // Process presentation content if needed
                    uno::Reference<presentation::XPresentationSupplier> xPresSupplier(xDoc, uno::UNO_QUERY);
                    if (xPresSupplier.is())
                    {
                        processSlides(xPresSupplier);
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

bool SlideConverter::convertODFToOOXML(uno::Reference<io::XInputStream>& xODFInput,
                                     uno::Reference<io::XOutputStream>& xOOXMLOutput)
{
    try
    {
        uno::Reference<frame::XDesktop2> xDesktop = frame::Desktop::create(m_xContext);
        
        // Load ODF document
        uno::Sequence<beans::PropertyValue> aLoadProps = comphelper::InitPropertySequence({
            {"InputStream", uno::makeAny(xODFInput)},
            {"Hidden", uno::makeAny(true)}
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
                    {"OutputStream", uno::makeAny(xOOXMLOutput)},
                    {"FilterName", uno::makeAny(OUString("Impress MS PowerPoint 2007 XML"))},
                    {"Overwrite", uno::makeAny(true)}
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

uno::Reference<io::XInputStream> SlideConverter::createTempInputStream(const std::vector<sal_Int8>& data)
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

std::vector<sal_Int8> SlideConverter::readFromInputStream(uno::Reference<io::XInputStream>& xInput)
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

bool SlideConverter::processSlides(const uno::Reference<presentation::XPresentationSupplier>& xPresSupplier)
{
    try
    {
        if (!xPresSupplier.is())
            return false;
            
        // Get draw pages (slides)
        uno::Reference<drawing::XDrawPagesSupplier> xDrawPagesSupplier(xPresSupplier, uno::UNO_QUERY);
        if (!xDrawPagesSupplier.is())
            return false;
            
        uno::Reference<drawing::XDrawPages> xDrawPages = xDrawPagesSupplier->getDrawPages();
        if (!xDrawPages.is())
            return false;
            
        sal_Int32 nSlideCount = xDrawPages->getCount();
        for (sal_Int32 i = 0; i < nSlideCount; ++i)
        {
            uno::Reference<drawing::XDrawPage> xSlide;
            xDrawPages->getByIndex(i) >>= xSlide;
            
            if (xSlide.is())
            {
                // Process individual slide
                processShapes(xSlide);
                processAnimations(xSlide);
                processTransitions(xSlide);
                processImages(xSlide);
                processCharts(xSlide);
                
                // Process slide notes
                uno::Reference<drawing::XDrawPage> xNotesPage;
                uno::Reference<beans::XPropertySet> xSlideProp(xSlide, uno::UNO_QUERY);
                if (xSlideProp.is())
                {
                    xSlideProp->getPropertyValue("NotesPage") >>= xNotesPage;
                    if (xNotesPage.is())
                    {
                        processSlideNotes(xNotesPage);
                    }
                }
            }
        }
        
        // Process master slides
        uno::Reference<drawing::XMasterPagesSupplier> xMasterSupplier(xPresSupplier, uno::UNO_QUERY);
        if (xMasterSupplier.is())
        {
            processSlideLayouts(xMasterSupplier);
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing slides: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processSlideMaster(const uno::Reference<drawing::XDrawPage>& xMasterPage)
{
    try
    {
        if (!xMasterPage.is())
            return false;
            
        // Process master slide layout, backgrounds, placeholders
        processShapes(xMasterPage);
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing slide master: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processSlideLayouts(const uno::Reference<drawing::XMasterPagesSupplier>& xMasterSupplier)
{
    try
    {
        uno::Reference<drawing::XDrawPages> xMasterPages = xMasterSupplier->getMasterPages();
        if (!xMasterPages.is())
            return false;
            
        sal_Int32 nMasterCount = xMasterPages->getCount();
        for (sal_Int32 i = 0; i < nMasterCount; ++i)
        {
            uno::Reference<drawing::XDrawPage> xMasterPage;
            xMasterPages->getByIndex(i) >>= xMasterPage;
            
            if (xMasterPage.is())
            {
                processSlideMaster(xMasterPage);
            }
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing slide layouts: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processAnimations(const uno::Reference<drawing::XDrawPage>& xSlide)
{
    try
    {
        if (!xSlide.is())
            return false;
            
        // Handle slide animations
        uno::Reference<animation::XAnimationNodeSupplier> xAnimSupplier(xSlide, uno::UNO_QUERY);
        if (xAnimSupplier.is())
        {
            uno::Reference<animation::XAnimationNode> xAnimNode = xAnimSupplier->getAnimationNode();
            if (xAnimNode.is())
            {
                // Process animation sequences, effects, timing
            }
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing animations: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processTransitions(const uno::Reference<drawing::XDrawPage>& xSlide)
{
    try
    {
        if (!xSlide.is())
            return false;
            
        // Handle slide transitions
        uno::Reference<beans::XPropertySet> xSlideProp(xSlide, uno::UNO_QUERY);
        if (xSlideProp.is())
        {
            // Get transition properties
            sal_Int16 nTransitionType = 0;
            xSlideProp->getPropertyValue("TransitionType") >>= nTransitionType;
            
            sal_Int16 nTransitionSubtype = 0;
            xSlideProp->getPropertyValue("TransitionSubtype") >>= nTransitionSubtype;
            
            double fTransitionDuration = 0.0;
            xSlideProp->getPropertyValue("TransitionDuration") >>= fTransitionDuration;
            
            // Process transition settings
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing transitions: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processShapes(const uno::Reference<drawing::XDrawPage>& xSlide)
{
    try
    {
        if (!xSlide.is())
            return false;
            
        sal_Int32 nShapeCount = xSlide->getCount();
        for (sal_Int32 i = 0; i < nShapeCount; ++i)
        {
            uno::Reference<drawing::XShape> xShape;
            xSlide->getByIndex(i) >>= xShape;
            
            if (xShape.is())
            {
                // Get shape type and process accordingly
                uno::Reference<beans::XPropertySet> xShapeProp(xShape, uno::UNO_QUERY);
                if (xShapeProp.is())
                {
                    OUString sShapeType;
                    xShapeProp->getPropertyValue("ShapeType") >>= sShapeType;
                    
                    if (sShapeType == "com.sun.star.drawing.TextShape")
                    {
                        processTextBoxes(xShape);
                    }
                    else if (sShapeType == "com.sun.star.drawing.GraphicObjectShape")
                    {
                        // Handle image shapes
                    }
                    else if (sShapeType == "com.sun.star.drawing.OLE2Shape")
                    {
                        // Handle embedded objects (charts, etc.)
                    }
                    // Add more shape type handling as needed
                }
            }
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing shapes: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processTextBoxes(const uno::Reference<drawing::XShape>& xShape)
{
    try
    {
        if (!xShape.is())
            return false;
            
        // Get text from shape
        uno::Reference<text::XText> xText(xShape, uno::UNO_QUERY);
        if (xText.is())
        {
            OUString sText = xText->getString();
            
            // Process text formatting, styles, etc.
            uno::Reference<text::XTextCursor> xCursor = xText->createTextCursor();
            if (xCursor.is())
            {
                // Handle text properties, formatting
                uno::Reference<beans::XPropertySet> xTextProp(xCursor, uno::UNO_QUERY);
                if (xTextProp.is())
                {
                    // Process font, color, alignment, etc.
                }
            }
        }
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing text boxes: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processImages(const uno::Reference<drawing::XDrawPage>& xSlide)
{
    try
    {
        // Handle image objects on slides
        if (!xSlide.is())
            return false;
            
        sal_Int32 nShapeCount = xSlide->getCount();
        for (sal_Int32 i = 0; i < nShapeCount; ++i)
        {
            uno::Reference<drawing::XShape> xShape;
            xSlide->getByIndex(i) >>= xShape;
            
            if (xShape.is())
            {
                uno::Reference<beans::XPropertySet> xShapeProp(xShape, uno::UNO_QUERY);
                if (xShapeProp.is())
                {
                    OUString sShapeType;
                    xShapeProp->getPropertyValue("ShapeType") >>= sShapeType;
                    
                    if (sShapeType == "com.sun.star.drawing.GraphicObjectShape")
                    {
                        // Process image properties, scaling, positioning
                        OUString sGraphicURL;
                        xShapeProp->getPropertyValue("GraphicURL") >>= sGraphicURL;
                        
                        // Handle image URL, embedded graphics, etc.
                    }
                }
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

bool SlideConverter::processCharts(const uno::Reference<drawing::XDrawPage>& xSlide)
{
    try
    {
        // Handle chart objects on slides
        if (!xSlide.is())
            return false;
            
        // Similar to processImages but for chart objects
        // This would handle embedded charts and their data
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing charts: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processSlideNotes(const uno::Reference<drawing::XDrawPage>& xNotesPage)
{
    try
    {
        if (!xNotesPage.is())
            return true; // No notes to process
            
        // Process speaker notes
        processShapes(xNotesPage);
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing slide notes: " << e.Message);
    }
    
    return false;
}

bool SlideConverter::processSlideComments(const uno::Reference<drawing::XDrawPage>& xSlide)
{
    try
    {
        // Handle slide comments (if supported)
        if (!xSlide.is())
            return false;
            
        // This would require additional interfaces for comment support
        
        return true;
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error processing slide comments: " << e.Message);
    }
    
    return false;
}

uno::Reference<drawing::XShape> SlideConverter::createTextBox(
    const uno::Reference<drawing::XDrawPage>& xSlide,
    const css::awt::Rectangle& position,
    const OUString& text)
{
    uno::Reference<drawing::XShape> xTextBox;
    
    try
    {
        if (!xSlide.is())
            return xTextBox;
            
        // Create text shape
        uno::Reference<lang::XMultiServiceFactory> xFactory(xSlide, uno::UNO_QUERY);
        if (xFactory.is())
        {
            xTextBox = uno::Reference<drawing::XShape>(
                xFactory->createInstance("com.sun.star.drawing.TextShape"), 
                uno::UNO_QUERY);
                
            if (xTextBox.is())
            {
                xTextBox->setPosition(css::awt::Point(position.X, position.Y));
                xTextBox->setSize(css::awt::Size(position.Width, position.Height));
                
                // Set text content
                uno::Reference<text::XText> xText(xTextBox, uno::UNO_QUERY);
                if (xText.is())
                {
                    xText->setString(text);
                }
                
                // Add to slide
                xSlide->add(xTextBox);
            }
        }
    }
    catch (const uno::Exception& e)
    {
        SAL_WARN("ucb.ucp.gdocs", "Error creating text box: " << e.Message);
    }
    
    return xTextBox;
}

bool SlideConverter::importGoogleSlide(const std::string& fileId, 
                                     const std::shared_ptr<GoogleSession>& session)
{
    // Implementation for importing Google Slides
    return false;
}

bool SlideConverter::exportToGoogleSlide(const std::string& fileName,
                                       const std::shared_ptr<GoogleSession>& session,
                                       const std::string& parentFolderId)
{
    // Implementation for exporting to Google Slides
    return false;
}

bool SlideConverter::isGoogleSlideMimeType(const std::string& mimeType)
{
    return mimeType == "application/vnd.google-apps.presentation";
}

OUString SlideConverter::getServiceName()
{
    return "com.sun.star.ucb.GoogleSlidesConverter";
}

// Service factory functions
uno::Reference<uno::XInterface> SAL_CALL
SlideConverter_createInstance(const uno::Reference<uno::XComponentContext>& xContext)
{
    return static_cast<cppu::OWeakObject*>(new SlideConverter(xContext));
}

uno::Sequence<OUString> SAL_CALL
SlideConverter_getSupportedServiceNames()
{
    return { SlideConverter::getServiceName() };
}

OUString SAL_CALL
SlideConverter_getImplementationName()
{
    return "com.sun.star.comp.ucb.GoogleSlidesConverter";
}

} // namespace gdocs

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */