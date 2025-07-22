# LibreOffice Scripting API Documentation

## Overview

LibreOffice supports scripting in multiple languages, allowing users to automate tasks, create custom functions, and extend functionality. The scripting framework provides a unified interface for different scripting languages while maintaining security and performance.

## Supported Languages

### Language Support Matrix

| Language | Integrated | External | IDE Support | Debugger |
|----------|-----------|----------|-------------|----------|
| StarBasic | ✓ | ✗ | Built-in | ✓ |
| Python | ✓ | ✓ | External | ✓ |
| JavaScript | ✓ | ✗ | Basic | ✗ |
| BeanShell | ✓ | ✗ | Basic | ✗ |
| Java | ✗ | ✓ | External | ✓ |

## StarBasic/LibreOffice Basic

### Basic IDE

```vb
' StarBasic example
Sub HelloWorld
    Dim oDoc As Object
    Dim oText As Object
    
    ' Get current document
    oDoc = ThisComponent
    
    ' Check if it's a text document
    If oDoc.supportsService("com.sun.star.text.TextDocument") Then
        oText = oDoc.getText()
        oText.setString("Hello from StarBasic!")
    Else
        MsgBox "This macro works only with text documents"
    End If
End Sub
```

### Document Manipulation

```vb
' Complex document operations
Sub ProcessDocument
    Dim oDoc As Object, oSheets As Object, oSheet As Object
    Dim oCell As Object, i As Integer, sum As Double
    
    oDoc = ThisComponent
    oSheets = oDoc.getSheets()
    oSheet = oSheets.getByIndex(0)
    
    ' Calculate sum of column A
    sum = 0
    For i = 0 To 99
        oCell = oSheet.getCellByPosition(0, i)
        If oCell.getType() = com.sun.star.table.CellContentType.VALUE Then
            sum = sum + oCell.getValue()
        End If
    Next i
    
    ' Put result in B1
    oSheet.getCellByPosition(1, 0).setValue(sum)
End Sub
```

### Dialog Creation

```vb
' Create and show dialog
Sub ShowCustomDialog
    Dim oDialog As Object
    Dim oDialogModel As Object
    Dim oButton As Object
    Dim oLabel As Object
    
    ' Create dialog
    oDialog = CreateUnoDialog(DialogLibraries.Standard.Dialog1)
    
    ' Access controls
    oButton = oDialog.getControl("CommandButton1")
    oLabel = oDialog.getControl("Label1")
    
    ' Set properties
    oLabel.Text = "Enter your name:"
    
    ' Show dialog
    If oDialog.execute() = 1 Then
        MsgBox "OK pressed"
    End If
    
    oDialog.dispose()
End Sub
```

### Event Handling

```vb
' Form control events
Sub Button_Click(oEvent As Object)
    Dim oButton As Object
    Dim oForm As Object
    Dim oTextField As Object
    
    oButton = oEvent.Source
    oForm = oButton.getParent()
    oTextField = oForm.getByName("TextField1")
    
    MsgBox "You entered: " & oTextField.Text
End Sub

' Document events
Sub OnDocumentOpen(oEvent As Object)
    ' Called when document opens
    ThisComponent.DocumentProperties.Subject = "Opened: " & Now()
End Sub
```

## Python Scripting

### Script Organization

Python scripts location:
```
~/.config/libreoffice/4/user/Scripts/python/     # User scripts
/usr/lib/libreoffice/share/Scripts/python/       # System scripts
<document>/Scripts/python/                        # Document embedded
```

### Basic Python Script

```python
# Python macro example
import uno

def hello_world(*args):
    """Basic Python macro"""
    # Get desktop
    desktop = XSCRIPTCONTEXT.getDesktop()
    
    # Get current document
    model = desktop.getCurrentComponent()
    
    # Check if Writer document
    if model.supportsService("com.sun.star.text.TextDocument"):
        text = model.getText()
        cursor = text.createTextCursor()
        text.insertString(cursor, "Hello from Python!", False)
    else:
        # Show message box
        ctx = XSCRIPTCONTEXT.getComponentContext()
        smgr = ctx.ServiceManager
        toolkit = smgr.createInstanceWithContext(
            "com.sun.star.awt.Toolkit", ctx)
        
        msgbox = toolkit.createMessageBox(
            None,
            "infobox",
            1,
            "Python Macro",
            "This macro works only with Writer documents"
        )
        msgbox.execute()

# Required for macro discovery
g_exportedScripts = (hello_world,)
```

### Advanced Python Operations

```python
# Complex document processing
import uno
from com.sun.star.beans import PropertyValue

def process_spreadsheet(*args):
    """Process Calc spreadsheet with Python"""
    doc = XSCRIPTCONTEXT.getDocument()
    
    # Check if Calc document
    if not doc.supportsService("com.sun.star.sheet.SpreadsheetDocument"):
        return
    
    sheets = doc.getSheets()
    sheet = sheets.getByIndex(0)
    
    # Process data
    data = []
    for row in range(10):
        row_data = []
        for col in range(5):
            cell = sheet.getCellByPosition(col, row)
            row_data.append(cell.getValue())
        data.append(row_data)
    
    # Analyze data
    import statistics
    for col in range(5):
        col_data = [data[row][col] for row in range(10)]
        mean = statistics.mean(col_data)
        stdev = statistics.stdev(col_data)
        
        # Write results
        sheet.getCellByPosition(col, 12).setValue(mean)
        sheet.getCellByPosition(col, 13).setValue(stdev)
    
    # Add chart
    create_chart(sheet, "A1:E10")

def create_chart(sheet, range_address):
    """Create chart from data range"""
    charts = sheet.getCharts()
    
    # Chart position and size
    rect = uno.createUnoStruct("com.sun.star.awt.Rectangle")
    rect.X = 1000
    rect.Y = 15000
    rect.Width = 10000
    rect.Height = 7000
    
    # Create chart
    charts.addNewByName("DataChart", rect, [range_address], True, True)

g_exportedScripts = (process_spreadsheet,)
```

### Python Dialog Handling

```python
# Dialog creation and handling
import uno
import unohelper
from com.sun.star.awt import XActionListener

class ButtonListener(unohelper.Base, XActionListener):
    def __init__(self, dialog):
        self.dialog = dialog
        
    def actionPerformed(self, event):
        """Handle button click"""
        text_field = self.dialog.getControl("TextField1")
        user_input = text_field.Text
        
        # Process input
        result = f"You entered: {user_input.upper()}"
        
        # Update label
        label = self.dialog.getControl("Label2")
        label.Text = result

def show_python_dialog(*args):
    """Create and show dialog from Python"""
    ctx = XSCRIPTCONTEXT.getComponentContext()
    smgr = ctx.ServiceManager
    
    # Create dialog model
    dialog_model = smgr.createInstanceWithContext(
        "com.sun.star.awt.UnoControlDialogModel", ctx)
    
    dialog_model.Width = 200
    dialog_model.Height = 100
    dialog_model.Title = "Python Dialog"
    
    # Add text field
    text_model = dialog_model.createInstance(
        "com.sun.star.awt.UnoControlEditModel")
    text_model.Name = "TextField1"
    text_model.PositionX = 10
    text_model.PositionY = 10
    text_model.Width = 180
    text_model.Height = 20
    dialog_model.insertByName("TextField1", text_model)
    
    # Add button
    button_model = dialog_model.createInstance(
        "com.sun.star.awt.UnoControlButtonModel")
    button_model.Name = "Button1"
    button_model.Label = "Process"
    button_model.PositionX = 75
    button_model.PositionY = 40
    button_model.Width = 50
    button_model.Height = 20
    dialog_model.insertByName("Button1", button_model)
    
    # Add result label
    label_model = dialog_model.createInstance(
        "com.sun.star.awt.UnoControlFixedTextModel")
    label_model.Name = "Label2"
    label_model.PositionX = 10
    label_model.PositionY = 70
    label_model.Width = 180
    label_model.Height = 20
    dialog_model.insertByName("Label2", label_model)
    
    # Create dialog
    dialog = smgr.createInstanceWithContext(
        "com.sun.star.awt.UnoControlDialog", ctx)
    dialog.setModel(dialog_model)
    
    # Add listener
    listener = ButtonListener(dialog)
    dialog.getControl("Button1").addActionListener(listener)
    
    # Create window
    toolkit = smgr.createInstanceWithContext(
        "com.sun.star.awt.Toolkit", ctx)
    dialog.setVisible(True)
    dialog.createPeer(toolkit, None)
    
    # Execute dialog
    dialog.execute()
    dialog.dispose()

g_exportedScripts = (show_python_dialog,)
```

## JavaScript Scripting

### Basic JavaScript

```javascript
// JavaScript macro
importClass(Packages.com.sun.star.uno.UnoRuntime);
importClass(Packages.com.sun.star.text.XTextDocument);

function helloJavaScript() {
    // Get document
    oDoc = XSCRIPTCONTEXT.getDocument();
    
    // Check if text document
    xTextDoc = UnoRuntime.queryInterface(XTextDocument, oDoc);
    if (xTextDoc != null) {
        xText = xTextDoc.getText();
        xText.setString("Hello from JavaScript!");
    }
}
```

### Rhino JavaScript Features

```javascript
// More complex JavaScript example
function processData() {
    var doc = XSCRIPTCONTEXT.getDocument();
    var sheets = doc.getSheets();
    var sheet = sheets.getByIndex(0);
    
    // JavaScript array processing
    var data = [];
    for (var i = 0; i < 10; i++) {
        var row = [];
        for (var j = 0; j < 5; j++) {
            var cell = sheet.getCellByPosition(j, i);
            row.push(cell.getValue());
        }
        data.push(row);
    }
    
    // Use JavaScript features
    var totals = data.map(function(row) {
        return row.reduce(function(a, b) { return a + b; }, 0);
    });
    
    // Write results
    totals.forEach(function(total, index) {
        sheet.getCellByPosition(6, index).setValue(total);
    });
}
```

## BeanShell Scripting

### BeanShell Example

```java
// BeanShell script
import com.sun.star.uno.UnoRuntime;
import com.sun.star.text.XTextDocument;
import com.sun.star.text.XText;

// BeanShell allows Java-like syntax
XTextDocument textDoc = (XTextDocument) UnoRuntime.queryInterface(
    XTextDocument.class, XSCRIPTCONTEXT.getDocument());

if (textDoc != null) {
    XText text = textDoc.getText();
    text.setString("Hello from BeanShell!");
    
    // Can use Java libraries
    java.util.Date now = new java.util.Date();
    text.insertString(text.getEnd(), "\nCurrent time: " + now, false);
}
```

## Script Provider Framework

### Script URI Format

```
vnd.sun.star.script:ScriptName?language=Language&location=Location

Examples:
vnd.sun.star.script:Standard.Module1.Main?language=Basic&location=document
vnd.sun.star.script:pythonscript.py$function_name?language=Python&location=user
vnd.sun.star.script:HelloWorld.js?language=JavaScript&location=share
```

### Script Execution

```python
# Execute script from Python
def execute_macro(script_uri):
    """Execute any macro by URI"""
    ctx = uno.getComponentContext()
    smgr = ctx.ServiceManager
    
    # Get script provider
    script_provider = smgr.createInstanceWithContext(
        "com.sun.star.script.provider.MasterScriptProviderFactory", ctx)
    
    master_provider = script_provider.createScriptProvider("")
    script = master_provider.getScript(script_uri)
    
    # Execute
    result = script.invoke((), (), ())
    return result[0]
```

## Security

### Macro Security Levels

```python
# Check macro security
def check_security():
    ctx = uno.getComponentContext()
    smgr = ctx.ServiceManager
    
    # Get security configuration
    config_provider = smgr.createInstanceWithContext(
        "com.sun.star.configuration.ConfigurationProvider", ctx)
    
    args = [PropertyValue("nodepath", 0, 
        "/org.openoffice.Office.Common/Security/Scripting", 0)]
    
    config = config_provider.createInstanceWithArguments(
        "com.sun.star.configuration.ConfigurationAccess", args)
    
    macro_security = config.getPropertyValue("MacroSecurityLevel")
    # 0 = Low, 1 = Medium, 2 = High, 3 = Very High
    return macro_security
```

### Signed Scripts

```xml
<!-- META-INF/manifest.xml for signed scripts -->
<manifest:manifest>
    <manifest:file-entry 
        manifest:full-path="Scripts/python/myscript.py"
        manifest:media-type="application/binary"/>
    <manifest:file-entry
        manifest:full-path="META-INF/documentsignatures.xml"
        manifest:media-type="application/vnd.sun.star.package-bundle-signature"/>
</manifest:manifest>
```

## Deployment

### Script Locations

1. **User Scripts**: `~/.config/libreoffice/4/user/Scripts/`
2. **Shared Scripts**: `/usr/share/libreoffice/Scripts/`
3. **Document Scripts**: Embedded in `.odt`, `.ods`, etc.
4. **Extension Scripts**: Deployed via `.oxt` packages

### Extension Packaging

```xml
<!-- description.xml for script extension -->
<description>
    <identifier value="com.example.myscripts"/>
    <version value="1.0"/>
    <display-name>
        <name lang="en">My Script Collection</name>
    </display-name>
    <publisher>
        <name lang="en">Your Name</name>
    </publisher>
    <extension-description>
        <src lang="en" xlink:href="description.txt"/>
    </extension-description>
</description>
```

## Performance Optimization

### Best Practices

```python
# Efficient script example
def optimized_processing():
    doc = XSCRIPTCONTEXT.getDocument()
    sheet = doc.getSheets().getByIndex(0)
    
    # Batch operations
    # Bad: Individual cell access
    # for i in range(1000):
    #     cell = sheet.getCellByPosition(0, i)
    #     cell.setValue(i * 2)
    
    # Good: Range operations
    cell_range = sheet.getCellRangeByPosition(0, 0, 0, 999)
    data = [[i * 2] for i in range(1000)]
    cell_range.setDataArray(tuple(data))
    
    # Disable screen updating
    doc.lockControllers()
    try:
        # Perform operations
        process_data()
    finally:
        doc.unlockControllers()
```

### Profiling Scripts

```python
# Performance profiling
import time

def profile_script():
    start_time = time.time()
    
    # Your code here
    perform_operations()
    
    end_time = time.time()
    execution_time = end_time - start_time
    
    # Log results
    with open("/tmp/script_profile.log", "a") as f:
        f.write(f"Execution time: {execution_time:.2f} seconds\n")
```

## Debugging

### Debug Output

```python
# Debugging helpers
def debug_print(message):
    """Print debug message to console"""
    import sys
    
    # Method 1: Console output (if available)
    print(f"DEBUG: {message}", file=sys.stderr)
    
    # Method 2: Message box
    ctx = XSCRIPTCONTEXT.getComponentContext()
    smgr = ctx.ServiceManager
    toolkit = smgr.createInstanceWithContext(
        "com.sun.star.awt.Toolkit", ctx)
    
    msgbox = toolkit.createMessageBox(
        None, "infobox", 1, "Debug", str(message))
    msgbox.execute()
    
    # Method 3: Log to file
    with open("/tmp/libreoffice_debug.log", "a") as f:
        f.write(f"{message}\n")
```

### Remote Debugging

```python
# Enable Python remote debugging
def enable_remote_debugging():
    import sys
    sys.path.append("/path/to/pycharm-debug.egg")
    import pydevd
    pydevd.settrace('localhost', port=5678, stdoutToServer=True, 
                    stderrToServer=True)
```

## Integration Examples

### Database Integration

```python
# Connect to database from script
def database_query():
    ctx = XSCRIPTCONTEXT.getComponentContext()
    smgr = ctx.ServiceManager
    
    # Database context
    db_context = smgr.createInstanceWithContext(
        "com.sun.star.sdb.DatabaseContext", ctx)
    
    # Connect
    data_source = db_context.getByName("Bibliography")
    connection = data_source.getConnection("", "")
    
    # Query
    statement = connection.createStatement()
    result_set = statement.executeQuery(
        "SELECT * FROM biblio WHERE Year > 2000")
    
    # Process results
    doc = XSCRIPTCONTEXT.getDocument()
    sheet = doc.getSheets().getByIndex(0)
    row = 0
    
    while result_set.next():
        sheet.getCellByPosition(0, row).setString(
            result_set.getString(1))
        sheet.getCellByPosition(1, row).setValue(
            result_set.getInt(2))
        row += 1
```

### Web Service Integration

```python
# Call web service from script
def fetch_web_data():
    import urllib.request
    import json
    
    # Fetch data
    url = "https://api.example.com/data"
    response = urllib.request.urlopen(url)
    data = json.loads(response.read())
    
    # Insert into document
    doc = XSCRIPTCONTEXT.getDocument()
    if doc.supportsService("com.sun.star.text.TextDocument"):
        text = doc.getText()
        cursor = text.createTextCursor()
        
        for item in data:
            text.insertString(cursor, f"{item['name']}: {item['value']}\n", False)
```

---

This documentation covers the scripting capabilities in LibreOffice. Each language offers different features and integration levels, allowing developers to choose the best tool for their automation needs.