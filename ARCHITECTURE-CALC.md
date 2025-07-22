# Calc (SC Module) Architecture

## Overview

Calc is LibreOffice's spreadsheet application, providing powerful calculation capabilities, data analysis tools, and charting features. The SC module (from "Star Calc") implements a high-performance spreadsheet engine with support for large datasets, complex formulas, and various import/export formats.

## Core Architecture

### Document Structure

```
ScDocument
    ├── ScTable[] (Sheets)
    │   ├── ScColumn[] (Columns)
    │   │   └── ScColumnData
    │   │       ├── Cell Data (mdds containers)
    │   │       ├── Cell Attributes
    │   │       └── Cell Notes
    │   ├── ScPatternAttr (Formatting)
    │   ├── ScConditionalFormat
    │   └── ScDBData (Database ranges)
    ├── ScDocumentPool (Attribute pool)
    ├── ScStyleSheetPool (Styles)
    └── ScRangeName (Named ranges)
```

### Key Classes

1. **ScDocument**: Central document class containing all sheets
2. **ScTable**: Individual sheet/tab in the spreadsheet
3. **ScColumn**: Column data storage using mdds containers
4. **ScFormulaCell**: Formula cell with calculation engine
5. **ScInterpreter**: Formula calculation interpreter
6. **ScDocShell**: Document container connecting to framework

## Data Storage

### Column-Based Storage

Calc uses column-oriented storage for efficiency:

```cpp
class ScColumn
{
    // Multi-type data storage using mdds
    sc::CellStoreType maCells;          // Cell values
    sc::CellTextAttrStoreType maCellTextAttrs;  // Text attributes
    sc::CellNoteStoreType maCellNotes;  // Cell notes
    
    // Broadcaster for dependencies
    sc::BroadcasterStoreType maBroadcasters;
};
```

### Cell Types

```cpp
enum CellType
{
    CELLTYPE_NONE,
    CELLTYPE_VALUE,     // Numeric value
    CELLTYPE_STRING,    // Text
    CELLTYPE_FORMULA,   // Formula
    CELLTYPE_EDIT       // Rich text
};
```

### MDDS (Multi-Dimensional Data Structure)

Calc uses MDDS for efficient storage:
- **multi_type_vector**: Stores different types efficiently
- **flat_segment_tree**: For row heights, hidden rows
- **rectangle_set**: For merged cells tracking

Benefits:
- Memory efficiency for sparse data
- Fast bulk operations
- Type-safe storage

## Formula Engine

### Formula Compilation

```cpp
class ScCompiler
{
    // Tokenize formula string
    bool CompileString(const OUString& rFormula);
    
    // Generate RPN token array
    std::unique_ptr<ScTokenArray> CompileTokenArray();
    
    // Symbol recognition
    bool IsReference(const OUString& rName);
    bool IsFunction(const OUString& rName);
};
```

### Token Types

```cpp
class ScToken
{
    OpCode eOp;          // Operation code
    StackVar eType;      // Data type
    
    // Polymorphic data
    union {
        double fVal;     // Numeric value
        OUString* pStr;  // String value
        ScComplexRefData aRef;  // Cell reference
    };
};
```

### Formula Interpretation

```cpp
class ScInterpreter
{
    // Execution stack
    ScTokenStack aStack;
    
    // Main interpreter loop
    void Interpret();
    
    // Operation handlers
    void ScAdd();      // Addition
    void ScSum();      // SUM function
    void ScVLookup();  // VLOOKUP function
    // ... hundreds more
};
```

### Calculation Order

Dependency tracking ensures correct calculation:

```cpp
class ScFormulaCell
{
    // Dependencies
    ScFormulaCellGroup* pCellGroup;
    
    // Dirty flag management
    void SetDirty(bool bDirtyFlag = true);
    bool NeedsInterpret() const;
    
    // Calculation
    void Interpret();
};
```

## Reference System

### Cell Addressing

```cpp
struct ScAddress
{
    SCROW nRow;    // 0-1048575 (over 1 million rows)
    SCCOL nCol;    // 0-16383 (16K columns)
    SCTAB nTab;    // Sheet index
    
    // A1 or R1C1 notation
    OUString Format(ScRefFlags nFlags) const;
};
```

### Range References

```cpp
struct ScRange
{
    ScAddress aStart;
    ScAddress aEnd;
    
    // Operations
    bool Intersects(const ScRange& rRange) const;
    void ExtendTo(const ScRange& rRange);
};
```

### Reference Types

1. **Relative**: A1, B2 (adjust when copied)
2. **Absolute**: $A$1, $B$2 (fixed)
3. **Mixed**: $A1, A$1 (partially fixed)
4. **3D**: Sheet1.A1:Sheet3.C10
5. **External**: 'file:///path/file.ods'#Sheet1.A1

## Shared Formulas

### Formula Groups

Optimization for repeated formulas:

```cpp
class ScFormulaCellGroup
{
    // Shared compiled formula
    ScTokenArray* mpCode;
    
    // Length of group
    SCROW mnLength;
    
    // Group calculation
    bool InterpretInvariantGroup();
};
```

Benefits:
- Reduced memory usage
- Faster calculation
- Better CPU cache utilization

## Data Structures

### Database Ranges

```cpp
class ScDBData
{
    OUString aName;
    ScRange aDBRange;
    
    // Sort parameters
    ScSortParam aSortParam;
    
    // Filter criteria
    ScQueryParam aQueryParam;
    
    // Subtotals
    ScSubTotalParam aSubTotal;
};
```

### Pivot Tables (DataPilot)

```cpp
class ScDPObject
{
    // Source data
    ScRange aSourceRange;
    
    // Field configuration
    ScDPSaveData* pSaveData;
    
    // Output location
    ScRange aOutRange;
    
    // Refresh data
    void RefreshAfterLoad();
};
```

### Conditional Formatting

```cpp
class ScConditionalFormat
{
    // Range where applied
    ScRangeList aRange;
    
    // Format conditions
    std::vector<ScFormatEntry*> maEntries;
    
    // Evaluation
    ScRefCellValue GetCell(const ScAddress& rPos);
};
```

Condition types:
- Cell value conditions
- Color scales
- Data bars
- Icon sets
- Date conditions

## Import/Export

### File Format Support

Native and foreign formats:
- **ODS**: Native OpenDocument Spreadsheet
- **Excel**: XLS (binary), XLSX (XML)
- **CSV**: Configurable delimiters
- **HTML**: Table import/export
- **PDF**: Export with form controls

### Excel Compatibility

```cpp
class XclImpRoot
{
    // Excel version info
    XclBiff meBiff;
    
    // Import state
    ScDocument& rDoc;
    XclImpAddressConverter& rAddrConv;
    
    // Formula converter
    XclImpFormulaCompiler& rFmlaComp;
};
```

Special handling for:
- Excel-specific functions
- Different calculation modes
- Formatting differences
- Macro conversion (optional)

## Performance Features

### Multi-Threading

```cpp
class ScFormulaGroupInterpreter
{
    // Parallel calculation
    bool InterpretFormulaGroup(ScFormulaCellGroup& rGroup);
    
    // Thread pool
    static FormulaGroupInterpreter* GetFormulaGroupInterpreter();
};
```

Parallelization strategies:
- Column-based parallelization
- Formula group vectorization
- Independent range calculation

### OpenCL Acceleration

```cpp
namespace sc::opencl
{
    // OpenCL formula compiler
    class FormulaGroupInterpreterOpenCL
    {
        // Generate OpenCL kernels
        bool CompileKernel(ScTokenArray* pCode);
        
        // Execute on GPU
        bool ExecuteKernel(double* pResult);
    };
}
```

Supported operations:
- Basic arithmetic
- Statistical functions
- Matrix operations
- Custom OpenCL kernels

### Caching Mechanisms

1. **Formula results**: Cached until dependencies change
2. **Cell attributes**: Pooled to reduce memory
3. **Number formats**: Cached format strings
4. **Interpreted formulas**: Cached token arrays

## UI Integration

### Grid Window

```cpp
class ScGridWindow : public vcl::Window
{
    // Rendering
    void Paint(vcl::RenderContext& rRenderContext, 
               const tools::Rectangle& rRect) override;
    
    // Input handling
    void MouseButtonDown(const MouseEvent& rMEvt) override;
    void KeyInput(const KeyEvent& rKEvt) override;
    
    // Selection
    void UpdateSelectionRectangles();
};
```

### View Data

```cpp
class ScViewData
{
    // View state per sheet
    ScViewDataTable* pTabData[MAXTABCOUNT];
    
    // Active cell
    SCCOL nCurX;
    SCROW nCurY;
    
    // Zoom level
    Fraction aZoomX;
    Fraction aZoomY;
};
```

## Charts

### Chart Data Provider

```cpp
class ScChart2DataProvider : public chart2::data::XDataProvider
{
    // Create data sequences
    Reference<chart2::data::XDataSequence> 
        createDataSequenceByRangeRepresentation(
            const OUString& aRangeRepresentation);
    
    // Listen for updates
    void AddDataSequence(const ScRangeList& rRange);
};
```

### Live Updates

Charts update automatically when:
- Cell values change
- Ranges are modified
- Rows/columns inserted/deleted

## External References

### External Document Links

```cpp
class ScExternalRefManager
{
    // Document cache
    std::map<OUString, ScExternalRefCache::DocItem> maDocs;
    
    // Load external document
    ScDocument* GetCachedDocument(sal_uInt16 nFileId);
    
    // Update links
    void UpdateAllRefCells(sal_uInt16 nFileId);
};
```

### DDE Links

Dynamic Data Exchange support:
```cpp
class ScDdeLink : public sfx2::SvBaseLink
{
    // DDE connection
    OUString aAppl;    // Application
    OUString aTopic;   // Topic
    OUString aItem;    // Item
    
    // Update mechanism
    void DataChanged(const OUString& rMimeType,
                    const css::uno::Any& rValue) override;
};
```

## Macros and Scripting

### StarBasic Integration

```cpp
class ScMacroManager
{
    // Execute macro
    void ExecuteMacro(const OUString& rMacroName,
                     const css::uno::Sequence<css::uno::Any>& rArgs);
    
    // Event binding
    void BindMacroToEvent(const ScRange& rRange,
                         const OUString& rEventName,
                         const OUString& rMacroName);
};
```

### UNO API

Calc exposes comprehensive UNO interfaces:
- XSpreadsheetDocument
- XSpreadsheets
- XCellRange
- XFormulaCell
- XDataPilotTables

## Advanced Features

### Goal Seek

```cpp
class ScGoalSeek
{
    // Find input value for desired result
    bool Solve(const ScAddress& rFormulaCell,
              const ScAddress& rVariableCell,
              double fTargetValue);
};
```

### Solver

Linear and non-linear optimization:
```cpp
class ScSolver
{
    // Objective function
    ScAddress aObjectiveCell;
    
    // Variables
    ScRangeList aVariableCells;
    
    // Constraints
    std::vector<ScOptConstraint> aConstraints;
    
    // Solve
    bool Solve(ScDocument* pDoc);
};
```

### Scenarios

What-if analysis:
```cpp
class ScScenario
{
    OUString aName;
    OUString aComment;
    ScRange aRange;
    
    // Scenario data
    ScDocument* pScenarioDoc;
    
    // Apply scenario
    void Apply(ScDocument* pDoc);
};
```

## Memory Management

### Cell Pool

```cpp
class ScCellPool
{
    // Object pools for different cell types
    svl::SharedStringPool aStringPool;
    
    // Reuse cell objects
    ScFormulaCell* GetFormulaCell();
    void ReturnFormulaCell(ScFormulaCell* pCell);
};
```

### Sparse Storage

Optimizations for large, sparse sheets:
- Empty cells take no memory
- Shared strings for duplicates
- Compressed number storage
- Lazy column allocation

## Future Directions

### Planned Improvements

1. **Performance**: Better multi-core utilization
2. **Memory**: Further storage optimizations
3. **Compatibility**: Enhanced Excel compatibility
4. **Functions**: More Excel-compatible functions
5. **UI**: Modern spreadsheet UI features

### Technical Modernization

- C++20 adoption
- Better GPU utilization
- Improved formula engine
- Enhanced pivot tables
- Real-time collaboration

---

This documentation covers Calc's core architecture. For implementation details, see the source code in the `sc/` directory.