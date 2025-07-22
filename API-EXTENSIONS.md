# LibreOffice Extension API Documentation

## Overview

LibreOffice extensions are packages that add functionality to LibreOffice. They can include new features, templates, galleries, autotext, and more. Extensions use the OXT (OpenOffice Extension) format and can be written in various languages using the UNO API.

## Extension Structure

### Basic Extension Layout

```
my-extension.oxt (ZIP archive)
├── META-INF/
│   └── manifest.xml          # Required: lists all files
├── description.xml           # Required: extension metadata
├── registration/
│   ├── license.txt          # Optional: license text
│   └── license_en.txt       # Localized licenses
├── icons/
│   ├── icon.png            # Extension icon
│   └── icon_hc.png         # High contrast icon
├── dialogs/
│   └── MyDialog.xdl        # Dialog definitions
├── Addons.xcu              # UI integration
├── ProtocolHandler.xcu     # Custom protocol handlers
├── CalcAddins.xcu          # Calc functions
└── libs/
    └── MyExtension.oxt     # Implementation libraries
```

## Extension Manifest

### META-INF/manifest.xml

```xml
<?xml version="1.0" encoding="UTF-8"?>
<manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0">
    <!-- Extension registration -->
    <manifest:file-entry manifest:media-type="application/vnd.sun.star.configuration-data"
                        manifest:full-path="Addons.xcu"/>
    
    <!-- Component registration -->
    <manifest:file-entry manifest:media-type="application/vnd.sun.star.uno-components"
                        manifest:full-path="components.xml"/>
    
    <!-- Python script -->
    <manifest:file-entry manifest:media-type="application/vnd.sun.star.script-python"
                        manifest:full-path="python/myscript.py"/>
    
    <!-- Dialog -->
    <manifest:file-entry manifest:media-type="application/vnd.sun.star.dialog-library"
                        manifest:full-path="dialogs/"/>
    
    <!-- Help files -->
    <manifest:file-entry manifest:media-type="application/vnd.sun.star.help"
                        manifest:full-path="help/"/>
</manifest:manifest>
```

## Extension Description

### description.xml

```xml
<?xml version="1.0" encoding="UTF-8"?>
<description xmlns="http://openoffice.org/extensions/description/2006"
             xmlns:xlink="http://www.w3.org/1999/xlink">
    
    <!-- Unique identifier -->
    <identifier value="com.example.myextension"/>
    
    <!-- Version -->
    <version value="1.2.0"/>
    
    <!-- Dependencies -->
    <dependencies>
        <OpenOffice.org-minimal-version value="3.3"/>
    </dependencies>
    
    <!-- Display name -->
    <display-name>
        <name lang="en-US">My Extension</name>
        <name lang="de">Meine Erweiterung</name>
    </display-name>
    
    <!-- Publisher -->
    <publisher>
        <name xlink:href="https://example.com" lang="en">Example Inc.</name>
    </publisher>
    
    <!-- Icon -->
    <icon>
        <default xlink:href="icons/icon.png"/>
        <high-contrast xlink:href="icons/icon_hc.png"/>
    </icon>
    
    <!-- Description -->
    <extension-description>
        <src xlink:href="description/desc_en.txt" lang="en"/>
        <src xlink:href="description/desc_de.txt" lang="de"/>
    </extension-description>
    
    <!-- License -->
    <registration>
        <simple-license accept-by="admin" default-license-id="en">
            <license-text xlink:href="registration/license_en.txt" lang="en"/>
            <license-text xlink:href="registration/license_de.txt" lang="de"/>
        </simple-license>
    </registration>
    
    <!-- Update information -->
    <update-information>
        <src xlink:href="https://example.com/update.xml"/>
    </update-information>
</description>
```

## UI Integration

### Addons.xcu - Menu and Toolbar Integration

```xml
<?xml version="1.0" encoding="UTF-8"?>
<oor:component-data xmlns:oor="http://openoffice.org/2001/registry"
                    xmlns:xs="http://www.w3.org/2001/XMLSchema"
                    oor:name="Addons" oor:package="org.openoffice.Office">
    <node oor:name="AddonUI">
        <!-- Menu items -->
        <node oor:name="OfficeMenuBar">
            <node oor:name="com.example.myextension.menu" oor:op="replace">
                <prop oor:name="Title" oor:type="xs:string">
                    <value xml:lang="en-US">My Extension</value>
                </prop>
                <node oor:name="Submenu">
                    <node oor:name="m1" oor:op="replace">
                        <prop oor:name="URL" oor:type="xs:string">
                            <value>vnd.sun.star.script:MyExtension.Module1.Function1?language=Basic&amp;location=application</value>
                        </prop>
                        <prop oor:name="Title" oor:type="xs:string">
                            <value xml:lang="en-US">Function 1</value>
                        </prop>
                        <prop oor:name="Target" oor:type="xs:string">
                            <value>_self</value>
                        </prop>
                        <prop oor:name="Context" oor:type="xs:string">
                            <value>com.sun.star.text.TextDocument</value>
                        </prop>
                    </node>
                </node>
            </node>
        </node>
        
        <!-- Toolbar items -->
        <node oor:name="OfficeToolBar">
            <node oor:name="com.example.myextension.toolbar" oor:op="replace">
                <node oor:name="t1" oor:op="replace">
                    <prop oor:name="URL" oor:type="xs:string">
                        <value>service:com.example.MyCommand</value>
                    </prop>
                    <prop oor:name="Title" oor:type="xs:string">
                        <value xml:lang="en-US">My Tool</value>
                    </prop>
                    <prop oor:name="Target" oor:type="xs:string">
                        <value>_self</value>
                    </prop>
                    <node oor:name="UserDefinedImages">
                        <prop oor:name="ImageSmallURL">
                            <value>%origin%/icons/tool_16.png</value>
                        </prop>
                        <prop oor:name="ImageBigURL">
                            <value>%origin%/icons/tool_26.png</value>
                        </prop>
                    </node>
                </node>
            </node>
        </node>
        
        <!-- Images -->
        <node oor:name="Images">
            <node oor:name="com.example.myextension.image1" oor:op="replace">
                <prop oor:name="URL">
                    <value>service:com.example.MyCommand</value>
                </prop>
                <node oor:name="UserDefinedImages">
                    <prop oor:name="ImageSmallURL">
                        <value>%origin%/icons/cmd_16.png</value>
                    </prop>
                </node>
            </node>
        </node>
    </node>
</oor:component-data>
```

## Component Implementation

### Python Extension Component

```python
# python/myextension.py
import uno
import unohelper
from com.sun.star.task import XJobExecutor
from com.sun.star.lang import XServiceInfo

# Implementation name (must match registration)
IMPLEMENTATION_NAME = "com.example.MyExtension"
SERVICE_NAMES = ("com.sun.star.task.Job",)

class MyExtension(unohelper.Base, XJobExecutor, XServiceInfo):
    def __init__(self, ctx):
        self.ctx = ctx
        
    # XJobExecutor
    def trigger(self, event):
        """Called when extension is triggered"""
        desktop = self.ctx.ServiceManager.createInstanceWithContext(
            "com.sun.star.frame.Desktop", self.ctx)
        
        # Get current document
        doc = desktop.getCurrentComponent()
        if doc and doc.supportsService("com.sun.star.text.TextDocument"):
            text = doc.getText()
            cursor = text.createTextCursor()
            text.insertString(cursor, "Hello from Extension!", False)
    
    # XServiceInfo
    def getImplementationName(self):
        return IMPLEMENTATION_NAME
    
    def supportsService(self, service_name):
        return service_name in SERVICE_NAMES
    
    def getSupportedServiceNames(self):
        return SERVICE_NAMES

# Component factory
def createInstance(ctx):
    return MyExtension(ctx)

g_ImplementationHelper = unohelper.ImplementationHelper()
g_ImplementationHelper.addImplementation(
    createInstance,
    IMPLEMENTATION_NAME,
    SERVICE_NAMES
)
```

### Java Extension Component

```java
// com/example/MyExtension.java
package com.example;

import com.sun.star.uno.XComponentContext;
import com.sun.star.lib.uno.helper.WeakBase;
import com.sun.star.task.XJobExecutor;
import com.sun.star.lang.XServiceInfo;
import com.sun.star.lang.XSingleComponentFactory;
import com.sun.star.registry.XRegistryKey;

public class MyExtension extends WeakBase 
    implements XJobExecutor, XServiceInfo {
    
    private final XComponentContext xContext;
    private static final String IMPLEMENTATION_NAME = 
        "com.example.MyExtension";
    private static final String[] SERVICE_NAMES = {
        "com.sun.star.task.Job"
    };
    
    public MyExtension(XComponentContext context) {
        this.xContext = context;
    }
    
    // XJobExecutor
    public void trigger(String event) {
        try {
            // Get desktop
            Object desktop = xContext.getServiceManager()
                .createInstanceWithContext(
                    "com.sun.star.frame.Desktop", xContext);
            
            XDesktop xDesktop = UnoRuntime.queryInterface(
                XDesktop.class, desktop);
            
            // Get current document
            XComponent doc = xDesktop.getCurrentComponent();
            XTextDocument textDoc = UnoRuntime.queryInterface(
                XTextDocument.class, doc);
                
            if (textDoc != null) {
                XText text = textDoc.getText();
                text.setString("Hello from Java Extension!");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    // XServiceInfo
    public String getImplementationName() {
        return IMPLEMENTATION_NAME;
    }
    
    public boolean supportsService(String serviceName) {
        for (String service : SERVICE_NAMES) {
            if (service.equals(serviceName)) {
                return true;
            }
        }
        return false;
    }
    
    public String[] getSupportedServiceNames() {
        return SERVICE_NAMES;
    }
    
    // Component factory
    public static XSingleComponentFactory __getComponentFactory(
        String implName) {
        
        XSingleComponentFactory xFactory = null;
        if (implName.equals(IMPLEMENTATION_NAME)) {
            xFactory = Factory.createComponentFactory(
                MyExtension.class, SERVICE_NAMES);
        }
        return xFactory;
    }
}
```

## Component Registration

### components.xml

```xml
<?xml version="1.0" encoding="UTF-8"?>
<components xmlns="http://openoffice.org/2010/uno-components">
    <!-- Python component -->
    <component loader="com.sun.star.loader.Python"
               uri="vnd.sun.star.expand:$UNO_USER_PACKAGES_CACHE/uno_packages/myextension.oxt/python/myextension.py">
        <implementation name="com.example.MyExtension">
            <service name="com.sun.star.task.Job"/>
        </implementation>
    </component>
    
    <!-- Java component -->
    <component loader="com.sun.star.loader.Java"
               uri="vnd.sun.star.expand:$UNO_USER_PACKAGES_CACHE/uno_packages/myextension.oxt/libs/myextension.jar">
        <implementation name="com.example.MyExtension">
            <service name="com.sun.star.task.Job"/>
        </implementation>
    </component>
    
    <!-- C++ component -->
    <component loader="com.sun.star.loader.SharedLibrary"
               uri="vnd.sun.star.expand:$UNO_USER_PACKAGES_CACHE/uno_packages/myextension.oxt/libs/myextension.so">
        <implementation name="com.example.MyExtension">
            <service name="com.sun.star.task.Job"/>
        </implementation>
    </component>
</components>
```

## Protocol Handlers

### ProtocolHandler.xcu

```xml
<?xml version="1.0" encoding="UTF-8"?>
<oor:component-data xmlns:oor="http://openoffice.org/2001/registry"
                    oor:name="ProtocolHandler"
                    oor:package="org.openoffice.Office">
    <node oor:name="HandlerSet">
        <node oor:name="com.example.MyProtocolHandler" oor:op="replace">
            <prop oor:name="Protocols" oor:type="oor:string-list">
                <value>myext:*</value>
            </prop>
        </node>
    </node>
</oor:component-data>
```

### Protocol Handler Implementation

```python
# Protocol handler implementation
class MyProtocolHandler(unohelper.Base, XDispatchProvider, XDispatch):
    def __init__(self, ctx):
        self.ctx = ctx
        self.listeners = []
    
    # XDispatchProvider
    def queryDispatch(self, url, target_frame_name, search_flags):
        if url.Protocol == "myext:":
            return self
        return None
    
    def queryDispatches(self, requests):
        return [self.queryDispatch(r.FeatureURL, r.FrameName, r.SearchFlags) 
                for r in requests]
    
    # XDispatch
    def dispatch(self, url, args):
        if url.Protocol == "myext:" and url.Path == "function1":
            self.execute_function1(args)
    
    def addStatusListener(self, control, url):
        self.listeners.append((control, url))
        # Send initial status
        event = FeatureStateEvent()
        event.FeatureURL = url
        event.IsEnabled = True
        control.statusChanged(event)
    
    def removeStatusListener(self, control, url):
        self.listeners.remove((control, url))
```

## Calc Add-in Functions

### CalcAddins.xcu

```xml
<?xml version="1.0" encoding="UTF-8"?>
<oor:component-data xmlns:oor="http://openoffice.org/2001/registry"
                    oor:name="CalcAddIns"
                    oor:package="org.openoffice.Office">
    <node oor:name="AddInInfo">
        <node oor:name="com.example.MyCalcAddin" oor:op="replace">
            <node oor:name="AddInFunctions">
                <node oor:name="myFunction" oor:op="replace">
                    <prop oor:name="DisplayName">
                        <value xml:lang="en">MYFUNCTION</value>
                    </prop>
                    <prop oor:name="Description">
                        <value xml:lang="en">My custom function</value>
                    </prop>
                    <prop oor:name="Category">
                        <value>Add-In</value>
                    </prop>
                    <node oor:name="Parameters">
                        <node oor:name="value" oor:op="replace">
                            <prop oor:name="DisplayName">
                                <value xml:lang="en">Value</value>
                            </prop>
                            <prop oor:name="Description">
                                <value xml:lang="en">Input value</value>
                            </prop>
                        </node>
                    </node>
                </node>
            </node>
        </node>
    </node>
</oor:component-data>
```

### Calc Add-in Implementation

```python
# Calc add-in implementation
from com.sun.star.sheet import XAddIn

class MyCalcAddin(unohelper.Base, XAddIn):
    def __init__(self, ctx):
        self.ctx = ctx
    
    # XAddIn
    def getProgrammaticFuntionName(self, display_name):
        if display_name == "MYFUNCTION":
            return "myFunction"
        return ""
    
    def getDisplayFunctionName(self, programmatic_name):
        if programmatic_name == "myFunction":
            return "MYFUNCTION"
        return ""
    
    def getFunctionDescription(self, programmatic_name):
        if programmatic_name == "myFunction":
            return "My custom function"
        return ""
    
    def getDisplayArgumentName(self, function_name, argument_index):
        if function_name == "myFunction" and argument_index == 0:
            return "Value"
        return ""
    
    def getArgumentDescription(self, function_name, argument_index):
        if function_name == "myFunction" and argument_index == 0:
            return "Input value"
        return ""
    
    def getProgrammaticCategoryName(self, programmatic_name):
        return "Add-In"
    
    def getDisplayCategoryName(self, programmatic_name):
        return "Add-In"
    
    # Actual function implementation
    def myFunction(self, value):
        """Custom Calc function"""
        return value * 2 + 10
```

## Options Dialog Integration

### OptionsDialog.xcu

```xml
<?xml version="1.0" encoding="UTF-8"?>
<oor:component-data xmlns:oor="http://openoffice.org/2001/registry"
                    xmlns:xs="http://www.w3.org/2001/XMLSchema"
                    oor:name="OptionsDialog"
                    oor:package="org.openoffice.Office">
    <node oor:name="Modules">
        <node oor:name="Writer">
            <node oor:name="Leaves">
                <node oor:name="com.example.MyExtension" oor:op="replace">
                    <prop oor:name="Id">
                        <value>com.example.MyExtension</value>
                    </prop>
                    <prop oor:name="Label">
                        <value xml:lang="en">My Extension</value>
                    </prop>
                    <prop oor:name="OptionsPage">
                        <value>%origin%/dialogs/Options.xdl</value>
                    </prop>
                    <prop oor:name="EventHandlerService">
                        <value>com.example.OptionsHandler</value>
                    </prop>
                </node>
            </node>
        </node>
    </node>
</oor:component-data>
```

## Building Extensions

### Build Script (Python)

```python
#!/usr/bin/env python3
# build_extension.py
import zipfile
import os
import shutil

def build_extension():
    # Extension name
    ext_name = "myextension.oxt"
    
    # Files to include
    files = [
        "META-INF/manifest.xml",
        "description.xml",
        "Addons.xcu",
        "python/myextension.py",
        "icons/icon.png",
        "registration/license.txt"
    ]
    
    # Create OXT (ZIP) file
    with zipfile.ZipFile(ext_name, 'w', zipfile.ZIP_DEFLATED) as oxt:
        for file in files:
            oxt.write(file)
    
    print(f"Extension built: {ext_name}")

if __name__ == "__main__":
    build_extension()
```

### Makefile

```makefile
# Makefile for extension
EXTENSION_NAME = myextension
VERSION = 1.0.0
OXT_FILE = $(EXTENSION_NAME)-$(VERSION).oxt

# Source files
SOURCES = META-INF/manifest.xml \
          description.xml \
          Addons.xcu \
          python/*.py \
          icons/*.png

all: $(OXT_FILE)

$(OXT_FILE): $(SOURCES)
	@echo "Building extension..."
	@zip -r $(OXT_FILE) $(SOURCES)
	@echo "Extension built: $(OXT_FILE)"

install: $(OXT_FILE)
	unopkg add $(OXT_FILE) --force

uninstall:
	unopkg remove $(EXTENSION_NAME)

clean:
	rm -f $(OXT_FILE)

.PHONY: all install uninstall clean
```

## Testing Extensions

### Unit Testing

```python
# test_extension.py
import unittest
import uno
from myextension import MyExtension

class TestMyExtension(unittest.TestCase):
    def setUp(self):
        self.ctx = uno.getComponentContext()
        self.extension = MyExtension(self.ctx)
    
    def test_implementation_name(self):
        self.assertEqual(
            self.extension.getImplementationName(),
            "com.example.MyExtension"
        )
    
    def test_service_support(self):
        self.assertTrue(
            self.extension.supportsService("com.sun.star.task.Job")
        )
    
    def test_functionality(self):
        # Test extension functionality
        result = self.extension.process_data([1, 2, 3])
        self.assertEqual(result, [2, 4, 6])

if __name__ == "__main__":
    unittest.main()
```

## Debugging Extensions

### Debug Output

```python
# Debug helper
def debug_log(message):
    """Log debug messages"""
    import datetime
    import os
    
    # Log file location
    log_file = os.path.expanduser("~/.config/libreoffice/4/user/myext.log")
    
    with open(log_file, "a") as f:
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"[{timestamp}] {message}\n")
```

### Remote Debugging

```python
# Enable remote debugging for Python extensions
def enable_debugging():
    import sys
    sys.path.append("/path/to/debugger")
    import pydevd
    pydevd.settrace('localhost', port=5678)
```

## Extension Management

### Update Mechanism

```xml
<!-- update.xml on server -->
<?xml version="1.0" encoding="UTF-8"?>
<description xmlns="http://openoffice.org/extensions/update/2006"
             xmlns:xlink="http://www.w3.org/1999/xlink">
    <identifier value="com.example.myextension"/>
    <version value="1.2.0"/>
    <update-download>
        <src xlink:href="https://example.com/myextension-1.2.0.oxt"/>
    </update-download>
    <release-notes>
        <src xlink:href="https://example.com/release-notes.txt" lang="en"/>
    </release-notes>
</description>
```

### Extension Configuration

```python
# Store extension configuration
def save_config(key, value):
    ctx = uno.getComponentContext()
    smgr = ctx.ServiceManager
    
    # Get configuration provider
    config_provider = smgr.createInstanceWithContext(
        "com.sun.star.configuration.ConfigurationProvider", ctx)
    
    # Access extension config node
    args = [PropertyValue("nodepath", 0,
        "/org.openoffice.Office.ExtensionManager/ExtensionProperties/"
        "com.example.myextension", 0)]
    
    config = config_provider.createInstanceWithArguments(
        "com.sun.star.configuration.ConfigurationUpdateAccess", args)
    
    # Set value
    config.setPropertyValue(key, value)
    config.commitChanges()
```

## Best Practices

### Performance

1. **Lazy Loading**: Load resources only when needed
2. **Caching**: Cache frequently used data
3. **Async Operations**: Use threading for long operations
4. **Resource Cleanup**: Properly dispose of resources

### Security

1. **Input Validation**: Always validate user input
2. **File Access**: Use proper file permissions
3. **Network Access**: Use HTTPS for network requests
4. **Code Signing**: Sign extensions for distribution

### Compatibility

1. **Version Checking**: Check LibreOffice version
2. **Service Availability**: Check if services exist
3. **Platform Testing**: Test on all target platforms
4. **Graceful Degradation**: Handle missing features

---

This documentation covers the complete extension development lifecycle for LibreOffice. Extensions provide a powerful way to enhance LibreOffice functionality while maintaining compatibility and user experience.