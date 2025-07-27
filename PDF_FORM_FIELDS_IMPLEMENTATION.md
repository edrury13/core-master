# PDF Form Fields Implementation for LibreOffice Draw

## Overview
This implementation adds PDF form field support to LibreOffice Draw, allowing users to create interactive PDF forms with various control types.

## Implementation Details

### 1. Form Control Shape Classes
- **Location**: `sd/inc/formcontrolshape.hxx` and `sd/source/core/formcontrolshape.cxx`
- **Class**: `SdFormControlShape` - Inherits from `SdrRectObj`
- **Supported Controls**:
  - Push Button
  - Check Box
  - Radio Button
  - Text Field
  - List Box
  - Combo Box

### 2. UI Integration
- **Commands Added**: 
  - `SID_INSERT_PUSHBUTTON` through `SID_INSERT_COMBOBOX` (IDs 1250-1255)
  - Defined in `include/svx/svxids.hrc`
- **Menu Integration**: 
  - Added "Form Controls" submenu in Insert menu
  - Location: `sd/uiconfig/simpress/menubar/menubar.xml`
- **Command Handlers**: 
  - `DrawViewShell::InsertFormControl()` in `sd/source/ui/view/drviews_form.cxx`

### 3. PDF Export
- **Export Handler**: `sd/source/ui/unoidl/unomodel_pdfexport.cxx`
- **Function**: `ExportFormControlsToPDF()`
- **Features**:
  - Converts form control shapes to PDF widgets
  - Preserves all properties (text, colors, fonts, etc.)
  - Handles special properties per control type

### 4. Build System Integration
- Updated `sd/Library_sd.mk` to include:
  - `sd/source/core/formcontrolshape`
  - `sd/source/ui/view/drviews_form`
  - `sd/source/ui/unoidl/unomodel_pdfexport`

## Usage

### Creating Form Fields
1. Open LibreOffice Draw
2. Go to Insert → Form Controls
3. Select the desired control type
4. The control is inserted at default position with default properties
5. Edit properties through the Properties panel (pending implementation)

### Exporting to PDF
1. Create form controls in your Draw document
2. File → Export as PDF
3. Form controls are automatically converted to interactive PDF form fields

## Pending Features

### 1. Property Panel (TODO)
- Create a dedicated panel for form field properties
- Allow editing of:
  - Field name and description
  - Visual properties (colors, fonts, borders)
  - Behavior properties (read-only, required, max length)
  - List items for list/combo boxes

### 2. PDF Import Enhancement (TODO)
- Extend `sdext/source/pdfimport/` to recognize PDF form widgets
- Convert PDF form annotations to `SdFormControlShape` objects
- Preserve all form field properties during import

## Technical Notes

### Form Control Properties
Each form control maintains:
- Name (unique identifier)
- Description (tooltip/help text)
- Visual properties (colors, fonts, borders)
- Type-specific properties (checked state, list items, etc.)

### PDF Widget Mapping
| Draw Control | PDF Widget Type |
|--------------|----------------|
| PushButton | PDFWriter::PushButton |
| CheckBox | PDFWriter::CheckBox |
| RadioButton | PDFWriter::RadioButton |
| TextField | PDFWriter::Edit |
| ListBox | PDFWriter::ListBox |
| ComboBox | PDFWriter::ComboBox |

### Integration Points
- **Shape Recognition**: Custom `SdrObjKind` (OBJ_MAXI + 100)
- **PDF Export Hook**: Integrate with existing PDF export in `filter/source/pdf/`
- **UNO API**: Properties accessible via `css::beans::PropertyValue` sequences

## Building
```bash
make sd.clean
make sd
```

## Testing
1. Create a Draw document with various form controls
2. Export to PDF
3. Open PDF in a reader that supports forms (Adobe Reader, Foxit, etc.)
4. Verify all form fields are interactive and functional

## Future Enhancements
- JavaScript actions for form controls
- Form validation rules
- Submit button with URL actions
- Digital signature fields
- Barcode fields
- Better visual feedback during design time