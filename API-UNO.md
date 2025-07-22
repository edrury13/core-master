# UNO API Documentation

## Overview

The UNO (Universal Network Objects) API is LibreOffice's comprehensive programming interface that enables developers to control and extend LibreOffice functionality. It provides language-independent access to all document types, UI elements, and system services.

## API Structure

### Service Hierarchy

```
com.sun.star (root namespace)
    ├── frame           # Application framework
    ├── text            # Writer API
    ├── sheet           # Calc API  
    ├── drawing         # Draw API
    ├── presentation    # Impress API
    ├── beans           # Properties
    ├── lang            # Core services
    ├── util            # Utilities
    └── ...many more
```

## Core Services

### Desktop Service

The main entry point for UNO programming:

```python
# Python example
import uno

# Get desktop
local_context = uno.getComponentContext()
resolver = local_context.ServiceManager.createInstanceWithContext(
    "com.sun.star.bridge.UnoUrlResolver", local_context)
context = resolver.resolve(
    "uno:socket,host=localhost,port=2002;urp;StarOffice.ComponentContext")
desktop = context.ServiceManager.createInstanceWithContext(
    "com.sun.star.frame.Desktop", context)

# Load document
document = desktop.loadComponentFromURL(
    "file:///path/to/document.odt", "_blank", 0, ())
```

### Service Manager

Central registry for all UNO services:

```cpp
// C++ example
Reference<XComponentContext> xContext = comphelper::getProcessComponentContext();
Reference<XMultiServiceFactory> xFactory(xContext->getServiceManager(), UNO_QUERY);

// Create service
Reference<XSpreadsheetDocument> xCalcDoc(
    xFactory->createInstance("com.sun.star.sheet.SpreadsheetDocument"),
    UNO_QUERY);
```

## Document APIs

### Writer API

Text document manipulation:

```java
// Java example
// Get text
XTextDocument xTextDoc = UnoRuntime.queryInterface(
    XTextDocument.class, document);
XText xText = xTextDoc.getText();

// Insert text
XTextCursor xCursor = xText.createTextCursor();
xText.insertString(xCursor, "Hello World!", false);

// Apply formatting
XPropertySet xCursorProps = UnoRuntime.queryInterface(
    XPropertySet.class, xCursor);
xCursorProps.setPropertyValue("CharWeight", FontWeight.BOLD);
xCursorProps.setPropertyValue("CharColor", 0xFF0000); // Red

// Insert table
XMultiServiceFactory xDocFactory = UnoRuntime.queryInterface(
    XMultiServiceFactory.class, xTextDoc);
XTextTable xTable = UnoRuntime.queryInterface(
    XTextTable.class,
    xDocFactory.createInstance("com.sun.star.text.TextTable"));
xTable.initialize(4, 3); // 4 rows, 3 columns
xText.insertTextContent(xCursor, xTable, false);
```

### Calc API

Spreadsheet manipulation:

```python
# Python example
# Get sheet
sheets = document.getSheets()
sheet = sheets.getByIndex(0)

# Set cell values
cell = sheet.getCellByPosition(0, 0)  # A1
cell.setValue(42)
cell = sheet.getCellByPosition(1, 0)  # B1
cell.setString("Hello")

# Set formula
cell = sheet.getCellByPosition(2, 0)  # C1
cell.setFormula("=A1*2")

# Get cell range
range = sheet.getCellRangeByPosition(0, 0, 2, 10)  # A1:C11
range.setPropertyValue("CellBackColor", 0xFFFF00)  # Yellow

# Create chart
charts = sheet.getCharts()
rect = Rectangle()
rect.X = 5000
rect.Y = 1000
rect.Width = 10000
rect.Height = 7000
charts.addNewByName("MyChart", rect, [range.getRangeAddress()], True, True)
```

### Draw/Impress API

Graphics and presentation manipulation:

```cpp
// C++ example
// Get draw page
Reference<XDrawPagesSupplier> xDrawPagesSupplier(document, UNO_QUERY);
Reference<XDrawPages> xDrawPages = xDrawPagesSupplier->getDrawPages();
Reference<XDrawPage> xDrawPage(xDrawPages->getByIndex(0), UNO_QUERY);

// Create shape
Reference<XMultiServiceFactory> xFactory(document, UNO_QUERY);
Reference<XShape> xShape(
    xFactory->createInstance("com.sun.star.drawing.RectangleShape"),
    UNO_QUERY);

// Set position and size
xShape->setPosition(awt::Point(1000, 1000));
xShape->setSize(awt::Size(5000, 3000));

// Add to page
xDrawPage->add(xShape);

// Set properties
Reference<XPropertySet> xShapeProps(xShape, UNO_QUERY);
xShapeProps->setPropertyValue("FillColor", Any(sal_Int32(0x00FF00)));
xShapeProps->setPropertyValue("LineColor", Any(sal_Int32(0x0000FF)));
```

## Dialog and Forms API

### Creating Dialogs

```python
# Python example
def create_dialog():
    # Create dialog model
    dialog_model = smgr.createInstanceWithContext(
        "com.sun.star.awt.UnoControlDialogModel", ctx)
    dialog_model.Width = 200
    dialog_model.Height = 150
    dialog_model.Title = "My Dialog"
    
    # Add button
    button_model = dialog_model.createInstance(
        "com.sun.star.awt.UnoControlButtonModel")
    button_model.Name = "OKButton"
    button_model.Label = "OK"
    button_model.PositionX = 75
    button_model.PositionY = 100
    button_model.Width = 50
    button_model.Height = 20
    dialog_model.insertByName("OKButton", button_model)
    
    # Create dialog control
    dialog = smgr.createInstanceWithContext(
        "com.sun.star.awt.UnoControlDialog", ctx)
    dialog.setModel(dialog_model)
    
    # Add action listener
    action_listener = ActionListener()
    dialog.getControl("OKButton").addActionListener(action_listener)
    
    # Execute dialog
    dialog.setVisible(True)
    dialog.execute()
```

### Form Controls

```java
// Java example - Form controls in document
// Get form
XFormsSupplier xFormsSupplier = UnoRuntime.queryInterface(
    XFormsSupplier.class, drawPage);
XNameContainer xForms = xFormsSupplier.getForms();
XForm xForm = UnoRuntime.queryInterface(
    XForm.class, xForms.getByName("Standard"));

// Create text field
XMultiServiceFactory xServiceFactory = UnoRuntime.queryInterface(
    XMultiServiceFactory.class, document);
Object textField = xServiceFactory.createInstance(
    "com.sun.star.form.component.TextField");

// Set properties
XPropertySet xTextFieldProps = UnoRuntime.queryInterface(
    XPropertySet.class, textField);
xTextFieldProps.setPropertyValue("Name", "UserName");
xTextFieldProps.setPropertyValue("DefaultText", "Enter name");

// Add to form
XIndexContainer xFormContainer = UnoRuntime.queryInterface(
    XIndexContainer.class, xForm);
xFormContainer.insertByIndex(0, textField);
```

## Database API

### Database Connection

```python
# Python example
# Create database context
db_context = smgr.createInstanceWithContext(
    "com.sun.star.sdb.DatabaseContext", ctx)

# Connect to database
data_source = db_context.getByName("Bibliography")
connection = data_source.getConnection("", "")

# Execute query
statement = connection.createStatement()
result_set = statement.executeQuery(
    "SELECT * FROM biblio WHERE Author LIKE '%Smith%'")

# Process results
while result_set.next():
    author = result_set.getString(1)
    title = result_set.getString(2)
    print(f"{author}: {title}")
```

### Data-Aware Forms

```cpp
// C++ example
// Bind form to data
Reference<XPropertySet> xFormProps(xForm, UNO_QUERY);
xFormProps->setPropertyValue("DataSourceName", Any(OUString("Bibliography")));
xFormProps->setPropertyValue("Command", Any(OUString("biblio")));
xFormProps->setPropertyValue("CommandType", Any(CommandType::TABLE));

// Bind control to field
Reference<XPropertySet> xControlProps(textField, UNO_QUERY);
xControlProps->setPropertyValue("DataField", Any(OUString("Author")));

// Load data
Reference<XLoadable> xLoadable(xForm, UNO_QUERY);
xLoadable->load();
```

## Event Handling

### Document Events

```java
// Java example
public class DocumentEventListener implements XDocumentEventListener {
    public void documentEventOccured(DocumentEvent event) {
        switch (event.EventName) {
            case "OnSave":
                System.out.println("Document saved");
                break;
            case "OnPrint":
                System.out.println("Document printed");
                break;
            case "OnModifyChanged":
                System.out.println("Modification state changed");
                break;
        }
    }
    
    public void disposing(EventObject event) {
        // Cleanup
    }
}

// Register listener
XDocumentEventBroadcaster broadcaster = UnoRuntime.queryInterface(
    XDocumentEventBroadcaster.class, document);
broadcaster.addDocumentEventListener(new DocumentEventListener());
```

### Mouse and Keyboard Events

```python
# Python example
class MouseListener(unohelper.Base, XMouseListener):
    def mousePressed(self, event):
        print(f"Mouse pressed at {event.X}, {event.Y}")
        
    def mouseReleased(self, event):
        pass
        
    def mouseEntered(self, event):
        pass
        
    def mouseExited(self, event):
        pass
        
    def disposing(self, event):
        pass

# Add to control
control.addMouseListener(MouseListener())
```

## Configuration API

### Reading Configuration

```cpp
// C++ example
// Get configuration provider
Reference<XMultiServiceFactory> xConfigProvider =
    theDefaultProvider::get(xContext);

// Access configuration node
Sequence<Any> aArguments(1);
PropertyValue aProperty;
aProperty.Name = "nodepath";
aProperty.Value <<= OUString("/org.openoffice.Office.Writer/Print");
aArguments[0] <<= aProperty;

Reference<XNameAccess> xAccess(
    xConfigProvider->createInstanceWithArguments(
        "com.sun.star.configuration.ConfigurationAccess",
        aArguments),
    UNO_QUERY);

// Read value
Any aValue = xAccess->getByName("PrinterName");
OUString sPrinterName;
aValue >>= sPrinterName;
```

### Writing Configuration

```python
# Python example
# Get configuration provider
config_provider = smgr.createInstanceWithContext(
    "com.sun.star.configuration.ConfigurationProvider", ctx)

# Get updateable access
args = [PropertyValue("nodepath", 0, 
    "/org.openoffice.Office.Common/Misc", 0)]
config_access = config_provider.createInstanceWithArguments(
    "com.sun.star.configuration.ConfigurationUpdateAccess", args)

# Update value
config_access.setPropertyValue("ShowTipOfTheDay", False)

# Commit changes
config_access.commitChanges()
```

## Dispatch API

### Executing Commands

```java
// Java example
// Get dispatcher
XDispatchProvider xDispatchProvider = UnoRuntime.queryInterface(
    XDispatchProvider.class, frame);

// Create URL
URL[] aURL = new URL[1];
aURL[0] = new URL();
aURL[0].Complete = ".uno:Bold";

// Transform URL
XURLTransformer xURLTransformer = UnoRuntime.queryInterface(
    XURLTransformer.class,
    xServiceManager.createInstance("com.sun.star.util.URLTransformer"));
xURLTransformer.parseSmart(aURL, ".uno:");

// Get dispatch
XDispatch xDispatch = xDispatchProvider.queryDispatch(
    aURL[0], "", 0);

// Execute with arguments
PropertyValue[] args = new PropertyValue[1];
args[0] = new PropertyValue();
args[0].Name = "Bold";
args[0].Value = Boolean.TRUE;

xDispatch.dispatch(aURL[0], args);
```

## Printing API

### Print Document

```python
# Python example
# Get printable interface
printable = document

# Set printer options
printer_opts = []
opt = PropertyValue()
opt.Name = "Name"
opt.Value = "My Printer"
printer_opts.append(opt)

printable.setPrinter(printer_opts)

# Print options
print_opts = []
opt = PropertyValue()
opt.Name = "Pages"
opt.Value = "1-5"
print_opts.append(opt)

opt = PropertyValue()
opt.Name = "Copies"
opt.Value = 2
print_opts.append(opt)

# Print
printable.print(print_opts)
```

## Best Practices

### Error Handling

```python
# Python example
try:
    # UNO operations
    document = desktop.loadComponentFromURL(url, "_blank", 0, ())
    if document is None:
        raise Exception("Failed to load document")
        
except NoSuchElementException as e:
    print(f"Element not found: {e}")
except PropertyVetoException as e:
    print(f"Property change vetoed: {e}")
except Exception as e:
    print(f"Error: {e}")
finally:
    # Cleanup
    if document:
        document.close(True)
```

### Performance Tips

1. **Cache service instances**: Don't create services repeatedly
2. **Use batch operations**: Update multiple properties at once
3. **Minimize API calls**: Retrieve data in chunks
4. **Use listeners wisely**: Remove when not needed
5. **Handle disposal**: Clean up resources properly

### Threading

```cpp
// C++ example - Thread-safe access
class SafeUNOAccess
{
    SolarMutexGuard aGuard;  // Acquire solar mutex
    
    void ModifyDocument()
    {
        // All UNO calls must be protected
        Reference<XText> xText = xTextDoc->getText();
        xText->setString("Thread-safe modification");
    }
};
```

## Common Patterns

### Model-View-Controller

```python
# Model (Document)
model = document

# View (Current controller's view)
controller = model.getCurrentController()
view = controller

# Controller operations
controller.getViewCursor().goToStart(False)
controller.getViewCursor().goToEnd(True)  # Select all
```

### Property Iteration

```java
// Enumerate all properties
XPropertySetInfo xInfo = xPropertySet.getPropertySetInfo();
Property[] properties = xInfo.getProperties();
for (Property prop : properties) {
    try {
        Object value = xPropertySet.getPropertyValue(prop.Name);
        System.out.println(prop.Name + " = " + value);
    } catch (Exception e) {
        // Some properties might not be accessible
    }
}
```

---

This documentation covers the essential UNO API concepts and usage patterns. For complete API reference, see https://api.libreoffice.org/