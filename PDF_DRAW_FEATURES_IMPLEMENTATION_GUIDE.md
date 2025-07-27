# PDF Draw Features Implementation Guide

This guide provides step-by-step instructions for implementing three PDF export features in LibreOffice Draw:
1. PDF Measurement Tools Support
2. PDF Page Rotation  
3. PDF Grid/Guide Export

## Feature 1: PDF Measurement Tools Support

### Overview
Add scale/measurement metadata to exported PDFs so technical drawings can use PDF viewer measurement tools with correct units.

### Implementation Steps

#### Step 1: Add UI Options to PDF Export Dialog
1. Modify `filter/source/pdf/impdialog.hxx`:
   - Add checkbox member: `std::unique_ptr<weld::CheckButton> m_xCbExportMeasurementInfo;`
   - Add scale/unit combo boxes

2. Modify `filter/source/pdf/impdialog.cxx`:
   - Initialize new controls in constructor
   - Add handlers for measurement export options
   - Store settings in FilterData

3. Update `filter/uiconfig/ui/pdfgeneralpage.ui`:
   - Add "Export measurement information" checkbox
   - Add unit selection dropdown (mm, cm, m, inch, ft)
   - Add scale ratio input fields (e.g., 1:100)

#### Step 2: Extend PDF Export Options Structure
1. Modify `filter/source/pdf/pdfexport.hxx`:
   ```cpp
   struct PDFExportOptions {
       // existing members...
       bool mbExportMeasurementInfo;
       OUString msDrawingUnit;
       double mfScaleNumerator;
       double mfScaleDenominator;
   };
   ```

2. Update `filter/source/pdf/pdfexport.cxx`:
   - Parse new options from FilterData
   - Pass to PDF writer

#### Step 3: Implement Measurement Dictionary in PDF Writer
1. Modify `vcl/source/gdi/pdfwriter_impl.hxx`:
   ```cpp
   class PDFWriterImpl {
       // Add method
       void emitMeasurementDictionary(const tools::Rectangle& rPageRect,
                                     const OUString& rUnit,
                                     double fScaleRatio);
   };
   ```

2. Implement in `vcl/source/gdi/pdfwriter_impl.cxx`:
   ```cpp
   void PDFWriterImpl::emitMeasurementDictionary(/*...*/) {
       // Create Measure dictionary
       // PDF Reference 1.6, Section 8.7.2
       aLine.append("/Type /Measure\n");
       aLine.append("/Subtype /RL\n"); // Rectilinear measure
       
       // Coordinate system
       aLine.append("/R \"");
       aLine.append(OUStringToOString(rUnit, RTL_TEXTENCODING_ASCII_US));
       aLine.append("\"\n");
       
       // Scale ratio
       aLine.append("/X [ ");
       aLine.append(fScaleRatio);
       aLine.append(" 0 0 ");
       aLine.append(fScaleRatio);
       aLine.append(" 0 0 ]\n");
   }
   ```

#### Step 4: Apply to Draw Pages
1. Modify `sd/source/ui/unoidl/unomodel.cxx`:
   - In PDF export method, check if document is Draw type
   - If measurement export enabled, call measurement setup
   - Apply to each page's viewport dictionary

## Feature 2: PDF Page Rotation

### Overview
Allow per-page rotation settings during PDF export (0°, 90°, 180°, 270°).

### Implementation Steps

#### Step 1: Add Page Rotation UI
1. Create new dialog `sd/uiconfig/sdraw/ui/pdfpagerotationdialog.ui`:
   - List of pages with rotation dropdown for each
   - "Apply to all" option
   - Preview area

2. Add button to PDF export dialog to open rotation settings

#### Step 2: Store Rotation Settings
1. Add to `filter/source/pdf/pdfexport.hxx`:
   ```cpp
   struct PageRotationInfo {
       sal_Int32 nPageNum;
       sal_Int32 nRotation; // 0, 90, 180, 270
   };
   std::vector<PageRotationInfo> maPageRotations;
   ```

#### Step 3: Implement Rotation in PDF Writer
1. Modify `vcl/source/gdi/pdfwriter_impl.cxx`:
   ```cpp
   void PDFWriterImpl::emitPage(sal_Int32 nPageId) {
       // Existing code...
       
       // Add rotation if specified
       sal_Int32 nRotation = getPageRotation(nPageId);
       if (nRotation != 0) {
           aLine.append("/Rotate ");
           aLine.append(nRotation);
           aLine.append("\n");
       }
   }
   ```

#### Step 4: Handle Content Transformation
1. Apply transformation matrix when rotation is set:
   ```cpp
   void PDFWriterImpl::beginPage(sal_Int32 nRotation) {
       if (nRotation == 90) {
           // Rotate 90 degrees clockwise
           setTransformation(0, -1, 1, 0, 0, pageHeight);
       } else if (nRotation == 180) {
           setTransformation(-1, 0, 0, -1, pageWidth, pageHeight);
       } else if (nRotation == 270) {
           setTransformation(0, 1, -1, 0, pageWidth, 0);
       }
   }
   ```

## Feature 3: PDF Grid/Guide Export

### Overview
Export Draw's grid lines and guides as non-printing PDF annotations.

### Implementation Steps

#### Step 1: Add Grid/Guide Export Options
1. Modify PDF export dialog:
   - Add "Export grid as annotations" checkbox
   - Add "Export guides as annotations" checkbox
   - Add opacity slider for grid lines

#### Step 2: Access Draw Grid/Guide Data
1. In `sd/source/ui/view/sdview.cxx`:
   ```cpp
   void SdView::GetGridSettings(bool& rbVisible, 
                               Size& rGridSize,
                               bool& rbSnapToGrid) {
       const SdrPageView* pPV = GetSdrPageView();
       if (pPV) {
           rbVisible = pPV->IsGridVisible();
           rGridSize = pPV->GetGridSize();
           rbSnapToGrid = pPV->IsGridSnap();
       }
   }
   ```

2. Get guide lines:
   ```cpp
   void SdView::GetGuideLines(std::vector<sal_Int32>& rHorizontal,
                             std::vector<sal_Int32>& rVertical) {
       const SdrPage* pPage = GetPage();
       if (pPage) {
           // Get helplines
           const SdrHelpLineList& rHelpLines = pPage->GetHelpLineList();
           for (size_t i = 0; i < rHelpLines.GetCount(); ++i) {
               const SdrHelpLine& rLine = rHelpLines[i];
               if (rLine.GetKind() == SdrHelpLineKind::Vertical) {
                   rVertical.push_back(rLine.GetPos().X());
               } else if (rLine.GetKind() == SdrHelpLineKind::Horizontal) {
                   rHorizontal.push_back(rLine.GetPos().Y());
               }
           }
       }
   }
   ```

#### Step 3: Create PDF Annotations for Grid/Guides
1. In PDF writer, create line annotations:
   ```cpp
   void PDFWriterImpl::emitGridAnnotation(const Point& rStart, 
                                         const Point& rEnd,
                                         double fOpacity) {
       // Create line annotation
       beginObject(nAnnotId);
       aLine.append("/Type /Annot\n");
       aLine.append("/Subtype /Line\n");
       aLine.append("/NM (Grid Line)\n");
       aLine.append("/F 4\n"); // Hidden, printable flag off
       
       // Line coordinates
       aLine.append("/L [ ");
       appendPoint(rStart, aLine);
       aLine.append(" ");
       appendPoint(rEnd, aLine);
       aLine.append(" ]\n");
       
       // Appearance
       aLine.append("/CA "); // Opacity
       aLine.append(fOpacity);
       aLine.append("\n");
       
       aLine.append("/C [ 0.5 0.5 0.5 ]\n"); // Gray color
       endObject();
   }
   ```

#### Step 4: Integration in Draw PDF Export
1. Modify `sd/source/ui/unoidl/unomodel.cxx`:
   ```cpp
   void SdXImpressDocument::exportGridAndGuides(vcl::PDFExtOutDevData& rPDFData) {
       SdDrawDocument* pDoc = GetDoc();
       SdView* pView = /* get current view */;
       
       if (mbExportGrid) {
           // Get grid settings
           bool bVisible;
           Size aGridSize;
           bool bSnapToGrid;
           pView->GetGridSettings(bVisible, aGridSize, bSnapToGrid);
           
           if (bVisible) {
               // Calculate grid lines for page
               for (long x = 0; x < pageWidth; x += aGridSize.Width()) {
                   rPDFData.CreateGridAnnotation(Point(x, 0), 
                                               Point(x, pageHeight),
                                               0.3); // 30% opacity
               }
               // Similar for horizontal lines
           }
       }
       
       if (mbExportGuides) {
           // Export guide lines
           std::vector<sal_Int32> aHGuides, aVGuides;
           pView->GetGuideLines(aHGuides, aVGuides);
           
           for (sal_Int32 x : aVGuides) {
               rPDFData.CreateGuideAnnotation(Point(x, 0),
                                            Point(x, pageHeight),
                                            0.5); // 50% opacity
           }
           // Similar for horizontal guides
       }
   }
   ```

### Testing Each Feature

1. **Measurement Tools**:
   - Export technical drawing with known dimensions
   - Open in Adobe Acrobat/Reader
   - Use measurement tool to verify scale

2. **Page Rotation**:
   - Create multi-page document with mixed orientations
   - Export with different rotation settings
   - Verify correct display in PDF viewer

3. **Grid/Guide Export**:
   - Enable grid and add guides in Draw
   - Export with grid/guide options enabled
   - Check annotations panel in PDF viewer
   - Verify they don't print

### Build Configuration
Add new files to makefiles:
- `sd/Library_sd.mk`
- `filter/Library_pdffilter.mk`
- `vcl/Library_vcl.mk`

### Localization
Add translatable strings to:
- `sd/inc/strings.hrc`
- `filter/source/pdf/strings.hrc`