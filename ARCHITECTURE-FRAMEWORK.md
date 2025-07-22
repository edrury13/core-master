# Framework and SFX2 Architecture

## Overview

LibreOffice has two framework layers that work together:
- **Framework**: Modern UNO-based framework handling UI elements, commands, and dispatch
- **SFX2** (StarView Framework 2): Legacy framework providing document/view architecture

These frameworks bridge the gap between the application layer and the underlying platform, managing everything from menu bars to document loading.

## Framework Module

### Core Architecture

```
Framework Components
    ├── Dispatch Framework (Commands)
    ├── UI Element Framework (Toolbars, Menus)
    ├── Frame/Controller/Model Architecture
    ├── Service Implementations
    └── Configuration Management
```

### Frame-Controller-Model Pattern

```cpp
// Frame: Window container
interface XFrame : XComponent
{
    XController getController();
    void setComponent(XWindow xWindow, XController xController);
    XFramesSupplier getCreator();
    string getName();
};

// Controller: Connects view to model
interface XController : XComponent  
{
    XFrame getFrame();
    XModel getModel();
    void attachFrame(XFrame xFrame);
    boolean attachModel(XModel xModel);
};

// Model: Document
interface XModel : XComponent
{
    XController getCurrentController();
    string getURL();
    sequence<XController> getControllers();
};
```

### Dispatch Framework

Command routing system:

```cpp
// Dispatch provider
interface XDispatchProvider
{
    XDispatch queryDispatch(URL aURL, string sTargetFrameName, long nSearchFlags);
    sequence<XDispatch> queryDispatches(sequence<DispatchDescriptor> aRequests);
};

// Command dispatcher
interface XDispatch
{
    void dispatch(URL aURL, sequence<PropertyValue> aArgs);
    void addStatusListener(XStatusListener xControl, URL aURL);
    void removeStatusListener(XStatusListener xControl, URL aURL);
};
```

Command URL structure:
```
.uno:Save                    // UNO command
.uno:Paste?Format:string=RTF // With parameters  
slot:5502                    // Legacy slot ID
macro:///Standard.Module1.Main // Macro
```

### UI Element Framework

#### Toolbar Implementation

```cpp
class ToolBarManager : public XComponent, public XFrameActionListener
{
    // Toolbar configuration
    Reference<XUIConfigurationManager> m_xConfigManager;
    
    // VCL toolbar
    VclPtr<ToolBox> m_pToolBar;
    
    // Command mapping
    CommandToInfoMap m_aCommandMap;
    
    // Create buttons from configuration
    void FillToolbar(const Reference<XIndexAccess>& rItemContainer);
    
    // Handle clicks
    DECL_LINK(ClickHdl, ToolBox*, void);
};
```

#### Menu Implementation

```cpp
class MenuBarManager : public XComponent, public XFrameActionListener
{
    // Menu structure
    Reference<XMenu> m_xMenu;
    
    // Populate from configuration
    void FillMenuManager(Menu* pMenu, 
                        const Reference<XIndexAccess>& rItemContainer);
    
    // Dynamic menu updates
    void UpdateSpecialWindowMenu(Menu* pMenu);
};
```

### Service Manager Integration

Framework services registration:

```xml
<component loader="com.sun.star.loader.SharedLibrary">
  <implementation name="com.sun.star.comp.framework.Desktop">
    <service name="com.sun.star.frame.Desktop"/>
    <singleton name="com.sun.star.frame.theDesktop"/>
  </implementation>
  
  <implementation name="com.sun.star.comp.framework.Frame">
    <service name="com.sun.star.frame.Frame"/>
  </implementation>
</component>
```

### Configuration Access

UI configuration storage:

```cpp
class UIConfigurationManager : public XUIConfigurationManager
{
    // Storage for UI elements
    Reference<XStorage> m_xDocConfigStorage;
    
    // Element types
    enum ElementType {
        MENUBAR,
        TOOLBAR,
        STATUSBAR,
        ACCELERATOR
    };
    
    // Get configuration
    Reference<XIndexContainer> getSettings(const OUString& ResourceURL);
    
    // Modify configuration
    void replaceSettings(const OUString& ResourceURL,
                        const Reference<XIndexContainer>& aNewData);
};
```

## SFX2 Module

### Core Architecture

```
SFX2 Components
    ├── Document Framework (SfxObjectShell)
    ├── View Framework (SfxViewShell)
    ├── Controller Framework (SfxShell)
    ├── Slot/Command System
    ├── Document I/O (SfxMedium)
    └── Binding Framework
```

### Shell Hierarchy

```cpp
class SfxShell
{
    // Slot execution
    virtual void Execute(SfxRequest& rReq);
    virtual void GetState(SfxItemSet& rSet);
    
    // Interface registration
    SFX_DECL_INTERFACE(SFX_INTERFACE_SFXDOCSH)
    
    // Slot map
    static SfxSlot aSlotMap[];
};

// Hierarchy
SfxShell
    ├── SfxViewShell (View base)
    │   ├── SfxBaseViewShell
    │   └── Application views (SwView, ScTabViewShell, etc.)
    └── SfxObjectShell (Document base)
        └── Application documents (SwDocShell, ScDocShell, etc.)
```

### Slot System

Legacy command system using slot IDs:

```cpp
// Slot definition in .sdi file
SfxVoidItem Save SID_SAVEDOC
[
    AutoUpdate = FALSE,
    FastCall = FALSE,
    ReadOnly = FALSE,
    MenuConfig = TRUE,
    ToolBoxConfig = TRUE,
    AccelConfig = TRUE,
]

// Slot map in implementation
SFX_IMPL_INTERFACE(SwDocShell, SfxObjectShell)

void SwDocShell::InitInterface_Impl()
{
    GetStaticInterface()->RegisterStatusBar(StatusBarId::WriterStatusBar);
    GetStaticInterface()->RegisterObjectBar(SFX_OBJECTBAR_OBJECT,
                                           ToolbarId::WriteObjectBar);
}

// Slot execution
void SwDocShell::Execute(SfxRequest& rReq)
{
    switch(rReq.GetSlot())
    {
        case SID_SAVEDOC:
            Save();
            break;
    }
}
```

### Document I/O (SfxMedium)

File handling abstraction:

```cpp
class SfxMedium
{
    // Storage/Stream access
    Reference<XStorage> GetStorage();
    Reference<XInputStream> GetInputStream();
    
    // URL and filter
    OUString GetURLObject() const;
    std::shared_ptr<const SfxFilter> GetFilter() const;
    
    // Properties
    SfxItemSet* GetItemSet() const;
    
    // Transfer
    void Transfer_Impl();
    void DoSaveAs_Impl(const SfxItemSet& rItemSet);
};
```

### View Framework

View management and updates:

```cpp
class SfxViewFrame : public SfxShell
{
    // Current view
    SfxViewShell* m_pViewShell;
    
    // Object shell (document)
    SfxObjectShell* GetObjectShell();
    
    // Window
    vcl::Window& GetWindow() const;
    
    // Switching views
    bool SwitchToViewShell(sal_uInt16 nViewId);
};

class SfxViewShell : public SfxShell
{
    // Printing
    virtual SfxPrinter* GetPrinter();
    virtual sal_uInt16 SetPrinter(SfxPrinter* pNewPrinter);
    
    // View operations
    virtual void InnerResizePixel(const Point& rOfs, const Size& rSize);
    virtual void OuterResizePixel(const Point& rOfs, const Size& rSize);
};
```

### Binding Framework

Connects UI to document state:

```cpp
class SfxBindings
{
    // Registered controllers
    SfxControllerArray aControllers;
    
    // State cache
    std::unique_ptr<SfxStateCache> pCache;
    
    // Update methods
    void Invalidate(sal_uInt16 nId);
    void Update(sal_uInt16 nId);
    void UpdateAll();
    
    // Execution
    const SfxPoolItem* Execute(sal_uInt16 nSlot, 
                              const SfxPoolItem** pArgs = nullptr);
};
```

### Status Updates

Automatic UI updates:

```cpp
class SfxControllerItem
{
    // Binding connection
    SfxBindings& GetBindings();
    
    // State notification
    virtual void StateChanged(sal_uInt16 nSID, 
                            SfxItemState eState,
                            const SfxPoolItem* pState);
    
    // Registration
    void Bind(sal_uInt16 nSlotId, SfxBindings& rBindings);
};
```

## Integration Between Framework and SFX2

### Command Routing

```
User Action (Click/Key)
        ↓
Framework XDispatch
        ↓
DispatchHelper
        ↓
SfxDispatcher
        ↓
SfxShell::Execute
        ↓
Application Code
```

### Dispatch Provider Implementation

```cpp
class SfxDispatchProvider : public XDispatchProvider
{
    Reference<XDispatch> queryDispatch(const URL& aURL,
                                     const OUString& sTarget,
                                     sal_Int32 nFlags) override
    {
        // Convert .uno: command to slot ID
        sal_uInt16 nSlot = GetSlotId(aURL.Complete);
        
        // Create dispatcher
        return new SfxOfficeDispatch(pBindings, pDispatcher, nSlot);
    }
};
```

### State Synchronization

```cpp
class SfxOfficeDispatch : public XDispatch, public XStatusListener
{
    // Forward to SFX2
    void dispatch(const URL& aURL, 
                 const Sequence<PropertyValue>& aArgs) override
    {
        SfxRequest aReq(nSlotId, SfxCallMode::SYNCHRON, aArgs);
        pDispatcher->Execute(aReq);
    }
    
    // Status updates from SFX2
    void StateChanged(sal_uInt16 nSID,
                     SfxItemState eState,
                     const SfxPoolItem* pState) override
    {
        // Convert to UNO status event
        FeatureStateEvent aEvent;
        aEvent.FeatureURL = aURL;
        aEvent.IsEnabled = (eState != SfxItemState::DISABLED);
        
        // Notify listeners
        for (auto& xListener : aListeners)
            xListener->statusChanged(aEvent);
    }
};
```

## Module Loading

### Dynamic Module Loading

```cpp
class SfxModule : public SfxShell
{
    // Module initialization
    virtual void Construct();
    
    // Interface registration
    void RegisterInterface(SfxShell* pIFace);
    
    // Factories
    void RegisterChildWindow(std::unique_ptr<SfxChildWinFactory>);
    
    // Resources
    std::unique_ptr<SfxSlotPool> pSlotPool;
};
```

### Application Modules

```cpp
// Writer module
class SwModule : public SfxModule
{
    // Module-specific initialization
    virtual void InitInterface_Impl() override;
    
    // Global settings
    SwModuleOptions* GetModuleConfig();
};
```

## Docking Framework

### Docking Windows

```cpp
class SfxDockingWindow : public DockingWindow
{
    // Docking state
    bool IsFloatingMode() const;
    void SetFloatingMode(bool bFloat);
    
    // Alignment
    SfxChildAlignment GetAlignment() const;
    void SetAlignment(SfxChildAlignment eAlign);
    
    // State persistence
    virtual void FillInfo(SfxChildWinInfo& rInfo) const;
};
```

### Child Windows

```cpp
class SfxChildWindow
{
    // Factory pattern
    static std::unique_ptr<SfxChildWindow> CreateChildWindow(
        sal_uInt16 nId,
        vcl::Window* pParent,
        SfxBindings* pBindings,
        SfxChildWinInfo* pInfo);
    
    // Window access
    vcl::Window* GetWindow();
    
    // Alignment
    SfxChildAlignment GetAlignment() const;
};
```

## Task Pane Framework

### Sidebar Implementation

```cpp
class SidebarController
{
    // Panel management
    void SwitchToDeck(const OUString& rsDeckId);
    
    // Context updates
    void UpdateConfigurations();
    
    // Panel factory
    Reference<XUIElement> CreateUIElement(
        const OUString& rsImplementationURL,
        const bool bWantsCanvas);
};
```

## Print Framework

### Print Job Management

```cpp
class SfxPrinter : public Printer
{
    // Job setup
    std::unique_ptr<SfxItemSet> pOptions;
    
    // Page setup
    void SetPaperBin(sal_uInt16 nPaperBin);
    void SetOrientation(Orientation eOrient);
};

class SfxPrintHelper : public XPrintable
{
    // Print implementation
    void print(const Sequence<PropertyValue>& xOptions) override;
    
    // Print preview
    Sequence<PropertyValue> getPrinter() override;
};
```

## Configuration Management

### Toolbar/Menu Persistence

```cpp
class SfxConfigManager
{
    // Load configuration
    void LoadToolbars();
    void LoadMenubar();
    
    // Save modifications
    void StoreToolbars();
    void StoreMenubar();
    
    // Reset to default
    void ResetToolbars();
};
```

## Event Broadcasting

### Document Events

```cpp
class SfxBroadcaster
{
    // Listener management
    void AddListener(SfxListener& rListener);
    void RemoveListener(SfxListener& rListener);
    
    // Event dispatch
    void Broadcast(const SfxHint& rHint);
};

// Event types
class SfxEventHint : public SfxHint
{
    OUString aEventName;  // "OnSave", "OnPrint", etc.
    sal_uInt16 nEventId;
};
```

## Performance Considerations

### Lazy Loading

- UI elements created on demand
- Module libraries loaded when needed
- Configuration cached

### Update Optimization

```cpp
class SfxBindings
{
    // Batch updates
    void EnterRegistrations();
    void LeaveRegistrations();
    
    // Selective invalidation
    void InvalidateShell(const SfxShell& rShell);
};
```

## Future Directions

### Modernization Efforts

1. **Gradual SFX2 replacement**: Moving to pure UNO
2. **Async command execution**: Non-blocking UI
3. **Better configuration**: Simplified customization
4. **Modern C++**: Removing legacy code patterns

### Planned Improvements

- Unified command system
- Simplified view management
- Better extension integration
- Enhanced performance

---

This documentation covers the Framework and SFX2 architecture. These modules form the backbone of LibreOffice's application framework.