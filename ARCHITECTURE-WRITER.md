# Writer (SW Module) Architecture

## Overview

Writer is LibreOffice's word processor component, handling complex text documents with advanced formatting, layout, and features like mail merge, tables, and embedded objects. The SW module (from German "Schreibprogramm") implements a sophisticated document model and layout engine.

## Core Architecture

### Document Model Structure

```
SwDoc (Document)
    ├── SwNodes (Node Array)
    │   ├── SwStartNode
    │   ├── SwTextNode (Paragraphs)
    │   ├── SwTableNode
    │   ├── SwSectionNode
    │   └── SwEndNode
    ├── SwPageDesc (Page Styles)
    ├── SwNumRule (Numbering)
    ├── SwFieldTypes (Fields)
    └── SwFrameFormats (Frames/Objects)
```

### Key Classes

1. **SwDoc**: Central document class containing all content
2. **SwNodes**: Array storing document structure as nodes
3. **SwTextNode**: Represents a paragraph with text and formatting
4. **SwFrame**: Layout representation of document elements
5. **SwRootFrame**: Root of the layout tree
6. **SwViewShell**: View/controller connecting document to UI

## Document Model

### Node System

The document is stored as a doubly-linked array of nodes:

```cpp
class SwNode
{
    SwNodeType m_nNodeType;
    SwNodes& m_rNodes;
    
    // Node navigation
    SwNode* GetNext();
    SwNode* GetPrev();
    SwNodeIndex GetIndex() const;
};
```

Node types:
- **SwStartNode/SwEndNode**: Delimit sections
- **SwTextNode**: Contains paragraph text
- **SwTableNode**: Table structure
- **SwSectionNode**: Document sections
- **SwGrfNode**: Graphics
- **SwOLENode**: OLE objects

### Text Storage

Text nodes store content with attributes:

```cpp
class SwTextNode : public SwContentNode
{
    // Text content
    OUString m_Text;
    
    // Attribute management
    SwpHints* m_pSwpHints;  // Text attributes
    
    // Paragraph properties
    SwAttrSet m_aSet;
};
```

### Attributes and Formatting

Writer uses a sophisticated attribute system:

1. **Character attributes**: Applied to text ranges
   - Font, size, weight, color
   - Language, kerning, effects

2. **Paragraph attributes**: Applied to entire paragraphs
   - Alignment, spacing, indentation
   - Borders, numbering, tabs

3. **Automatic styles**: Formatting not in named styles

Attribute hierarchy:
```
Document defaults
    ↓
Page style
    ↓
Paragraph style
    ↓
Character style
    ↓
Direct formatting
```

## Layout Engine

### Layout Tree Structure

```
SwRootFrame (Root)
    ├── SwPageFrame (Page)
    │   ├── SwBodyFrame (Body)
    │   │   ├── SwTextFrame (Paragraph)
    │   │   ├── SwTabFrame (Table)
    │   │   └── SwSectionFrame (Section)
    │   ├── SwHeaderFrame
    │   └── SwFooterFrame
    └── SwPageFrame (Next Page)
        └── ...
```

### Frame Types

```cpp
class SwFrame
{
    // Position and size
    SwRect m_aFrame;
    SwRect m_aPrt;  // Print area
    
    // Tree structure
    SwFrame* m_pNext;
    SwFrame* m_pPrev;
    SwFrame* m_pUpper;
    SwFrame* m_pLower;
    
    // Layout calculation
    virtual void MakeAll(vcl::RenderContext* pRenderContext) = 0;
};
```

Frame categories:
- **Layout frames**: Page, body, column, section
- **Content frames**: Text, table, graphics
- **Floating frames**: Fly frames for positioned objects

### Layout Process

The layout engine performs:

1. **Frame creation**: Build frames from document model
2. **Size calculation**: Determine frame dimensions
3. **Positioning**: Place frames on pages
4. **Line breaking**: Break text into lines
5. **Page breaking**: Distribute content across pages

Layout triggers:
```cpp
// Document change
SwDoc::GetIDocumentLayoutAccess().SetLayouter(nullptr);

// View update  
SwViewShell::InvalidateLayout(true);

// Idle layout
SwLayIdle::DoIdleJob();
```

## Text Formatting

### Portion System

Text is broken into portions for rendering:

```cpp
class SwLinePortion
{
    sal_uInt16 nLineLength;  // Length in model
    sal_uInt16 nLineWidth;   // Width on screen
    
    virtual void Paint(const SwTextPaintInfo& rInf) const;
};
```

Portion types:
- **SwTextPortion**: Regular text
- **SwFieldPortion**: Fields
- **SwTabPortion**: Tab stops
- **SwLineBreakPortion**: Line breaks
- **SwHolePortion**: Spaces/gaps

### Line Layout

```cpp
class SwLineLayout : public SwTextPortion
{
    // Portions in this line
    SwLinePortion* m_pPortion;
    
    // Line metrics
    sal_uInt16 m_nRealHeight;
    sal_uInt16 m_nTextHeight;
};
```

### Hyphenation

Writer supports automatic hyphenation:

```cpp
class SwTextFormatter
{
    // Hyphenation decision
    bool Hyphenate(SwInterHyphInfo& rHyphInf);
    
    // Language-specific rules via ICU
    css::uno::Reference<css::linguistic2::XHyphenator> GetHyphenator();
};
```

## Tables

### Table Model

```
SwTable
    ├── SwTableLines (Rows)
    │   └── SwTableLine
    │       └── SwTableBoxes (Cells)
    │           └── SwTableBox
    │               └── SwStartNode
    │                   └── Content nodes
```

### Table Layout

Complex table layout features:
- Row/column span
- Nested tables
- Variable borders
- Auto-sizing
- Table splitting across pages

```cpp
class SwTabFrame : public SwLayoutFrame
{
    // Table layout algorithm
    void MakeAll(vcl::RenderContext* pRenderContext) override;
    
    // Handle row splitting
    bool Split(const SwTwips nCutPos, bool bTryToSplit);
};
```

## Fields and References

### Field System

```cpp
class SwField
{
    // Field type (date, page number, reference, etc.)
    SwFieldType* m_pType;
    
    // Calculate current value
    virtual OUString ExpandField(bool bCached) const;
    
    // Update on change
    virtual void UpdateFields();
};
```

Field categories:
- **Document fields**: Page number, page count
- **Date/time fields**: Current date, creation time
- **Reference fields**: Cross-references, bookmarks
- **Database fields**: Mail merge fields
- **User fields**: Variables, input fields

### Bookmarks and References

```cpp
class SwBookmark : public SwModify
{
    SwPosition m_aStartPos;
    SwPosition m_aEndPos;
    OUString m_aName;
};
```

## Lists and Numbering

### Numbering Rules

```cpp
class SwNumRule
{
    // Up to 10 levels (0-9)
    SwNumFormat m_aFormats[MAXLEVEL];
    
    // Numbering type
    SvxNumberType m_eNumberingType;
    
    // Continuous numbering
    bool m_bContinuousNumbering;
};
```

### List Implementation

Lists are implemented via paragraph attributes:

```cpp
class SwTextNode
{
    // List membership
    SwNumRule* GetNumRule() const;
    int GetActualListLevel() const;
    
    // List ID for continuous numbering
    OUString GetListId() const;
};
```

## Undo/Redo System

### Undo Architecture

```cpp
class SwUndo
{
    SwUndoId m_nId;
    
    virtual void UndoImpl(SwUndoIter& rIter) = 0;
    virtual void RedoImpl(SwUndoIter& rIter) = 0;
    virtual void RepeatImpl(SwUndoIter& rIter);
};
```

Undo types include:
- Text insertion/deletion
- Formatting changes
- Table operations
- Object insertion
- Style changes

### Undo Groups

Complex operations use undo groups:

```cpp
class SwUndoGroup : public SwUndo
{
    std::vector<std::unique_ptr<SwUndo>> m_aUndos;
    
    // Execute all contained undos
    void UndoImpl(SwUndoIter& rIter) override;
};
```

## View and Controller

### SwViewShell Hierarchy

```
SwViewShell (Base view)
    ├── SwCursorShell (Cursor handling)
    │   └── SwEditShell (Editing operations)
    │       └── SwWrtShell (UI integration)
    └── SwWebViewShell (HTML mode)
```

### Cursor System

```cpp
class SwCursor : public SwPaM
{
    // Position in document
    SwPosition m_aPoint;  // Current position
    SwPosition m_aMark;   // Selection anchor
    
    // Movement operations
    bool GoNextCell();
    bool GoPrevCell();
    bool GoNextWord();
};
```

## Redlining (Track Changes)

### Change Tracking

```cpp
class SwRangeRedline
{
    SwRedlineData m_aRedlineData;
    SwPosition m_aStart;
    SwPosition m_aEnd;
    
    // Type: Insert, Delete, Format
    RedlineType GetType() const;
    
    // Author and timestamp
    const OUString& GetAuthor() const;
    const DateTime& GetTimeStamp() const;
};
```

### Redline Display

Redlines affect layout:
- Deleted text can be hidden/shown
- Insertions highlighted
- Format changes marked

## Import/Export Filters

### Filter Architecture

Writer supports numerous formats:
- **Native**: ODT (ODF Text)
- **Microsoft**: DOC, DOCX, RTF
- **Web**: HTML, XHTML
- **Other**: TXT, PDF export

### DOCX Filter Structure

```
Document
    ↓
SwDoc Model
    ↓
DocxExport
    ↓
OOXML Parts (document.xml, styles.xml, etc.)
    ↓
ZIP Package
```

## Performance Optimizations

### Layout Cache

```cpp
class SwLayoutCache
{
    // Cache page breaks for faster reopening
    std::vector<sal_uInt16> m_aPageBreaks;
    
    // Apply cached layout
    bool Read(SwDoc* pDoc);
};
```

### Idle Processing

Background tasks during idle:
- Spell checking
- Grammar checking
- Layout optimization
- Field updates

### Smart Invalidation

Only invalidate affected portions:
```cpp
// Invalidate specific frame
pFrame->InvalidateSize();
pFrame->InvalidatePrt();
pFrame->InvalidatePos();

// Invalidate range
SwRect aRect(...);
pViewShell->InvalidateWindows(aRect);
```

## Mail Merge

### Database Integration

```cpp
class SwDBManager
{
    // Data source connections
    css::uno::Reference<css::sdbc::XConnection> GetConnection();
    
    // Merge operations
    bool MergeMailFiles(SwWrtShell* pWorkShell,
                       const SwMergeDescriptor& rMergeDesc);
};
```

### Merge Process

1. Connect to data source
2. Create merge documents
3. Replace fields with data
4. Generate output (print/PDF/email)

## Accessibility

### Document Structure

Exposed via accessibility APIs:
- Paragraphs as text blocks
- Tables with row/column info
- Lists with level information
- Headings for navigation

```cpp
class SwAccessibleParagraph : public SwAccessibleContext
{
    // Text access
    OUString GetString() override;
    
    // Text attributes
    css::uno::Sequence<css::beans::PropertyValue> 
        getCharacterAttributes(sal_Int32 nIndex);
};
```

## Future Enhancements

### Ongoing Development

1. **Performance**: Faster layout for large documents
2. **Collaboration**: Better change tracking and comments
3. **Styles**: Improved style handling and UI
4. **Compatibility**: Better DOCX fidelity
5. **Layout**: Advanced typography features

### Technical Debt

Areas for improvement:
- Simplify frame hierarchy
- Modernize attribute system
- Improve table algorithms
- Better separation of model/view

---

This documentation provides an in-depth look at Writer's architecture. For specific implementation details, consult the source code in the `sw/` directory.