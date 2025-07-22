# Import/Export Filter Framework Architecture

## Overview

LibreOffice's filter framework handles conversion between its native ODF formats and numerous foreign formats. The framework provides a flexible, extensible system for file format detection, import, and export across all LibreOffice applications.

## Core Architecture

### Filter Framework Components

```
Filter Framework
    ├── Type Detection
    ├── Filter Factory
    ├── Import Filters
    ├── Export Filters
    ├── Filter Configuration
    └── Stream/Storage Handling
```

### Key Interfaces

```cpp
// Type detection
interface XTypeDetection
{
    string queryTypeByURL(string URL);
    string queryTypeByDescriptor(sequence<PropertyValue> Descriptor, 
                                boolean bAllowDeep);
};

// Filter factory
interface XFilterFactory
{
    XFilter createInstance(string FilterName);
    boolean queryFilter(string FilterName, 
                       sequence<PropertyValue> Properties);
};

// Import filter
interface XImporter
{
    void setTargetDocument(XComponent Document);
};

// Export filter
interface XExporter
{
    void setSourceDocument(XComponent Document);
};

// Actual filtering
interface XFilter
{
    boolean filter(sequence<PropertyValue> MediaDescriptor);
    void cancel();
};
```

## Type Detection

### Detection Process

```cpp
class TypeDetection : public XTypeDetection
{
    // Detection by content
    OUString queryTypeByDescriptor(Sequence<PropertyValue>& rDescriptor,
                                  sal_Bool bAllowDeep)
    {
        // 1. Check URL pattern
        OUString sType = impl_detectTypeByURL(sURL);
        
        // 2. Check flat detection (file header)
        if (sType.isEmpty())
            sType = impl_detectTypeByPattern(xStream);
            
        // 3. Deep detection (parse content)
        if (sType.isEmpty() && bAllowDeep)
            sType = impl_detectTypeByContent(xStream);
            
        return sType;
    }
};
```

### File Type Registration

```xml
<!-- filter/source/config/fragments/types/writer_MS_Word_2007.xcu -->
<node oor:name="writer_MS_Word_2007" oor:op="replace">
    <prop oor:name="DetectService">
        <value>com.sun.star.comp.Writer.MSWordImportFilter</value>
    </prop>
    <prop oor:name="URLPattern"/>
    <prop oor:name="Extensions">
        <value>docx docm</value>
    </prop>
    <prop oor:name="MediaType">
        <value>application/vnd.openxmlformats-officedocument.wordprocessingml.document</value>
    </prop>
    <prop oor:name="Preferred">
        <value>false</value>
    </prop>
    <prop oor:name="ClipboardFormat">
        <value>MSWordDoc</value>
    </prop>
</node>
```

## Filter Configuration

### Filter Description

```xml
<!-- filter/source/config/fragments/filters/MS_Word_2007.xcu -->
<node oor:name="MS Word 2007" oor:op="replace">
    <prop oor:name="FileFormatVersion">
        <value>0</value>
    </prop>
    <prop oor:name="Type">
        <value>writer_MS_Word_2007</value>
    </prop>
    <prop oor:name="FilterService">
        <value>com.sun.star.comp.Writer.WriterFilter</value>
    </prop>
    <prop oor:name="UIComponent"/>
    <prop oor:name="UserData">
        <value>OXML</value>
    </prop>
    <prop oor:name="UIName">
        <value xml:lang="en-US">Word 2007-365</value>
    </prop>
    <prop oor:name="Flags">
        <value>IMPORT EXPORT ALIEN 3RDPARTYFILTER PREFERRED</value>
    </prop>
</node>
```

### Filter Flags

```cpp
enum FilterFlags
{
    IMPORT            = 0x00000001,  // Can import
    EXPORT            = 0x00000002,  // Can export
    TEMPLATE          = 0x00000004,  // Is a template
    INTERNAL          = 0x00000008,  // Internal format
    TEMPLATEPATH      = 0x00000010,  // Found in template path
    OWN               = 0x00000020,  // LibreOffice native
    ALIEN             = 0x00000040,  // Foreign format
    DEFAULT           = 0x00000100,  // Default for type
    SUPPORTSSELECTION = 0x00000200,  // Supports partial export
    NOTINFILEDLG      = 0x00001000,  // Hide from dialog
    NOTINCHOOSER      = 0x00002000,  // Hide from chooser
    READONLY          = 0x00010000,  // Read-only filter
    MUSTINSTALL       = 0x00020000,  // Must be installed
    CONSULTSERVICE    = 0x00040000,  // Ask service for UI
    STARONEFILTER     = 0x00080000,  // StarOffice filter
    PACKED            = 0x00100000,  // Packed format
    EXOTIC            = 0x00200000,  // Unusual format
    COMBINED          = 0x00800000,  // Combined filter
    ENCRYPTION        = 0x01000000,  // Supports encryption
};
```

## Import Filters

### Base Import Filter

```cpp
class ImportFilter : public XImporter, public XFilter
{
protected:
    Reference<XComponent> mxDocument;
    
public:
    // XImporter
    void SAL_CALL setTargetDocument(const Reference<XComponent>& xDoc) override
    {
        mxDocument = xDoc;
    }
    
    // XFilter
    sal_Bool SAL_CALL filter(const Sequence<PropertyValue>& rMediaDesc) override
    {
        // Get input stream
        Reference<XInputStream> xInputStream = GetInputStream(rMediaDesc);
        
        // Parse and import
        return ImportDocument(xInputStream, mxDocument);
    }
    
    virtual bool ImportDocument(const Reference<XInputStream>& xInput,
                               const Reference<XComponent>& xDoc) = 0;
};
```

### XML-Based Import

```cpp
class XmlImportFilter : public ImportFilter
{
protected:
    bool ImportDocument(const Reference<XInputStream>& xInput,
                       const Reference<XComponent>& xDoc) override
    {
        // Create SAX parser
        Reference<XParser> xParser = Parser::create(mxContext);
        
        // Set document handler
        Reference<XDocumentHandler> xHandler = CreateDocumentHandler(xDoc);
        xParser->setDocumentHandler(xHandler);
        
        // Parse
        InputSource aInputSource;
        aInputSource.aInputStream = xInput;
        xParser->parseStream(aInputSource);
        
        return true;
    }
};
```

### Binary Import Filters

```cpp
class BinaryImportFilter : public ImportFilter
{
protected:
    bool ImportDocument(const Reference<XInputStream>& xInput,
                       const Reference<XComponent>& xDoc) override
    {
        // Create stream
        SvStream* pStream = utl::UcbStreamHelper::CreateStream(xInput);
        
        // Read file header
        FileHeader aHeader;
        pStream->ReadBytes(&aHeader, sizeof(aHeader));
        
        // Validate and import
        if (ValidateHeader(aHeader))
            return ImportBinaryFormat(pStream, xDoc);
            
        return false;
    }
};
```

## Export Filters

### Base Export Filter

```cpp
class ExportFilter : public XExporter, public XFilter
{
protected:
    Reference<XComponent> mxDocument;
    
public:
    // XExporter
    void SAL_CALL setSourceDocument(const Reference<XComponent>& xDoc) override
    {
        mxDocument = xDoc;
    }
    
    // XFilter
    sal_Bool SAL_CALL filter(const Sequence<PropertyValue>& rMediaDesc) override
    {
        // Get output stream
        Reference<XOutputStream> xOutputStream = GetOutputStream(rMediaDesc);
        
        // Export document
        return ExportDocument(mxDocument, xOutputStream);
    }
    
    virtual bool ExportDocument(const Reference<XComponent>& xDoc,
                               const Reference<XOutputStream>& xOutput) = 0;
};
```

### XML Export

```cpp
class XmlExportFilter : public ExportFilter
{
protected:
    bool ExportDocument(const Reference<XComponent>& xDoc,
                       const Reference<XOutputStream>& xOutput) override
    {
        // Create SAX writer
        Reference<XWriter> xWriter = Writer::create(mxContext);
        xWriter->setOutputStream(xOutput);
        
        // Export
        Reference<XExportFilter> xExporter = CreateExporter();
        xExporter->exporter(xDoc, xWriter);
        
        return true;
    }
};
```

## Major Filter Implementations

### Writer Filters

```cpp
// DOCX Import (writerfilter/)
class WriterFilter : public ImportFilter
{
    bool ImportDocument(const Reference<XInputStream>& xInput,
                       const Reference<XComponent>& xDoc) override
    {
        // Create OOXML document
        writerfilter::ooxml::OOXMLDocument::Pointer_t pDocument = 
            writerfilter::ooxml::OOXMLDocumentFactory::createDocument(xInput);
            
        // Stream to domain mapper
        writerfilter::dmapper::DomainMapper aMapper(xDoc);
        pDocument->resolve(aMapper);
        
        return true;
    }
};

// RTF Import
class RTFFilter : public ImportFilter
{
    bool ImportDocument(const Reference<XInputStream>& xInput,
                       const Reference<XComponent>& xDoc) override
    {
        // RTF tokenizer
        RTFDocument::Pointer_t pDocument = 
            RTFDocumentFactory::createDocument(xInput);
            
        // Parse and map
        writerfilter::dmapper::DomainMapper aMapper(xDoc);
        pDocument->resolve(aMapper);
        
        return true;
    }
};
```

### Calc Filters

```cpp
// XLSX Import (sc/source/filter/oox/)
class XlsxImportFilter : public ImportFilter
{
    bool ImportDocument(const Reference<XInputStream>& xInput,
                       const Reference<XComponent>& xDoc) override
    {
        // Create workbook import
        oox::xls::ExcelFilter aFilter(mxContext);
        aFilter.setTargetDocument(xDoc);
        
        // Import
        return aFilter.importDocument();
    }
};

// CSV Import
class ScImportCSV
{
    bool Import(SvStream& rStream, ScDocument& rDoc)
    {
        // CSV parameters
        sal_Unicode cSeparator = ',';
        sal_Unicode cTextDelimiter = '"';
        
        // Parse line by line
        OUString aLine;
        while (rStream.ReadUniOrByteStringLine(aLine))
        {
            ParseCSVLine(aLine, cSeparator, cTextDelimiter);
        }
        
        return true;
    }
};
```

### Graphics Filters

```cpp
// Graphics import filter framework
class GraphicFilter
{
    Graphic ImportGraphic(SvStream& rStream, const OUString& rFilterName)
    {
        // Detect format
        OUString aFilterName = rFilterName;
        if (aFilterName.isEmpty())
            aFilterName = DetectFormat(rStream);
            
        // Import
        if (aFilterName == "PNG")
            return ImportPNG(rStream);
        else if (aFilterName == "JPEG")
            return ImportJPEG(rStream);
        else if (aFilterName == "SVG")
            return ImportSVG(rStream);
        // ... more formats
    }
};
```

### PDF Export

```cpp
class PDFExport
{
    bool Export(const Reference<XComponent>& xDoc,
               const Reference<XOutputStream>& xOutput)
    {
        // PDF writer
        vcl::PDFWriter aWriter(xOutput);
        
        // Export settings
        vcl::PDFWriter::PDFWriterContext aContext;
        aContext.Version = vcl::PDFWriter::PDFVersion::PDF_1_5;
        aContext.Tagged = true;  // PDF/UA
        
        // Render document
        for (sal_Int32 nPage = 0; nPage < nPageCount; ++nPage)
        {
            aWriter.NewPage();
            RenderPage(xDoc, nPage, aWriter);
        }
        
        aWriter.Emit();
        return true;
    }
};
```

## Filter Chains

### Alien Format Chains

Some formats require multiple conversion steps:

```cpp
// Example: Old Word format via WW8
class LegacyWordImport
{
    bool Import(const OUString& rFileName, Reference<XComponent>& xDoc)
    {
        // Step 1: Convert to WW8
        ConvertToWW8(rFileName, aTempFile);
        
        // Step 2: Use WW8 import filter
        WW8Import aImporter;
        return aImporter.Import(aTempFile, xDoc);
    }
};
```

### Transform Filters

```cpp
// XSLT-based filter
class XSLTFilter : public ImportFilter, public ExportFilter
{
    OUString msImportXSLT;
    OUString msExportXSLT;
    
    bool Transform(const Reference<XInputStream>& xInput,
                  const Reference<XOutputStream>& xOutput,
                  const OUString& rXSLT)
    {
        // Create transformer
        Reference<XXSLTTransformer> xTransformer = 
            XSLTTransformer::create(mxContext);
            
        // Set stylesheet
        xTransformer->setStyleSheet(rXSLT);
        
        // Transform
        xTransformer->setInputStream(xInput);
        xTransformer->setOutputStream(xOutput);
        xTransformer->start();
        
        return true;
    }
};
```

## Storage Handling

### Package/ZIP Support

```cpp
class ZipPackage : public XPackage
{
    // Access package streams
    Reference<XInputStream> GetStream(const OUString& rPath)
    {
        ZipEntry aEntry = FindEntry(rPath);
        return CreateInputStream(aEntry);
    }
    
    // Add stream to package
    void PutStream(const OUString& rPath, 
                  const Reference<XInputStream>& xStream)
    {
        ZipEntry aEntry;
        aEntry.sPath = rPath;
        AddEntry(aEntry, xStream);
    }
};
```

### OLE Storage

```cpp
class StgStorage : public BaseStorage
{
    // Access OLE streams
    BaseStorageStream* OpenStream(const OUString& rName, 
                                 StreamMode nMode)
    {
        StgDirEntry* pEntry = Find(rName);
        if (pEntry)
            return new StgStream(pEntry, nMode);
        return nullptr;
    }
};
```

## Performance Optimizations

### Streaming Import

```cpp
class StreamingImporter
{
    // Don't load entire file into memory
    bool ImportLargeFile(const Reference<XInputStream>& xInput)
    {
        // Create buffered stream
        Reference<XSeekable> xSeekable(xInput, UNO_QUERY);
        sal_Int64 nSize = xSeekable->getLength();
        
        // Process in chunks
        const sal_Int32 nChunkSize = 1024 * 1024; // 1MB
        Sequence<sal_Int8> aBuffer(nChunkSize);
        
        while (xInput->readBytes(aBuffer, nChunkSize) > 0)
        {
            ProcessChunk(aBuffer);
        }
        
        return true;
    }
};
```

### Parallel Processing

```cpp
class ParallelImporter
{
    // Import with multiple threads
    bool ImportWithThreads(const Reference<XInputStream>& xInput)
    {
        // Split work
        std::vector<WorkItem> aWorkItems = SplitWork(xInput);
        
        // Process in parallel
        std::vector<std::future<bool>> aFutures;
        for (const auto& rItem : aWorkItems)
        {
            aFutures.push_back(
                std::async(std::launch::async,
                          [this, rItem]() { return ProcessItem(rItem); }));
        }
        
        // Wait for completion
        bool bSuccess = true;
        for (auto& rFuture : aFutures)
            bSuccess &= rFuture.get();
            
        return bSuccess;
    }
};
```

## Error Handling

### Filter Exceptions

```cpp
class FilterException : public Exception
{
    FilterError meError;
    OUString msDetails;
    
    enum FilterError
    {
        UNSUPPORTED_FORMAT,
        CORRUPT_FILE,
        MISSING_PASSWORD,
        WRONG_PASSWORD,
        OUT_OF_MEMORY,
        IO_ERROR
    };
};
```

### Recovery Mechanisms

```cpp
class RobustImporter
{
    bool ImportWithRecovery(const Reference<XInputStream>& xInput)
    {
        try
        {
            return NormalImport(xInput);
        }
        catch (const FilterException& e)
        {
            if (e.GetError() == FilterError::CORRUPT_FILE)
                return RecoveryImport(xInput);
            throw;
        }
    }
};
```

## Extension Points

### Custom Filters

```cpp
// Register custom filter
class CustomFilterRegistration
{
    void RegisterFilter()
    {
        // Add type detection
        Sequence<PropertyValue> aType;
        aType[0].Name = "Type";
        aType[0].Value <<= OUString("custom_format");
        
        // Add filter description  
        Sequence<PropertyValue> aFilter;
        aFilter[0].Name = "Name";
        aFilter[0].Value <<= OUString("My Custom Filter");
        
        // Register with factory
        Reference<XFilterFactory> xFactory = GetFilterFactory();
        xFactory->insertByName("MyFilter", makeAny(aFilter));
    }
};
```

## Future Enhancements

### Planned Improvements

1. **Better Format Support**: More complete foreign format support
2. **Performance**: Faster import/export, especially for large files
3. **Streaming**: Better streaming support for cloud storage
4. **Fidelity**: Improved format conversion accuracy
5. **Modern Formats**: Support for emerging document formats

### Architecture Evolution

- Modular filter design
- Plugin-based architecture
- Better error recovery
- Format validation frameworks
- Machine learning for type detection

---

This documentation covers the LibreOffice filter framework architecture. The system's flexibility allows for easy addition of new formats while maintaining compatibility with existing ones.