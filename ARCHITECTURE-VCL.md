# VCL (Visual Class Library) Architecture

## Overview

VCL is LibreOffice's cross-platform GUI toolkit and graphics abstraction layer. It provides a unified API for windowing, input handling, and rendering across Windows, macOS, Linux (GTK/Qt), and other platforms. VCL sits between the application layer and the native platform APIs.

## Core Architecture

### Layer Structure

```
┌─────────────────────────────────────┐
│     Application Code                │
├─────────────────────────────────────┤
│     VCL Public API                  │
├─────────────────────────────────────┤
│     VCL Core Implementation         │
├─────────────────────────────────────┤
│     SalInstance (Platform Layer)    │
├─────────────────────────────────────┤
│  Platform Backends (Win/Mac/GTK/Qt) │
├─────────────────────────────────────┤
│     Native Platform APIs            │
└─────────────────────────────────────┘
```

### Key Components

1. **OutputDevice**: Core rendering abstraction
2. **Window**: Base class for all UI elements  
3. **SalInstance**: Platform abstraction factory
4. **SalFrame**: Native window abstraction
5. **SalGraphics**: Native graphics context
6. **Application**: Main event loop and initialization

## Platform Abstraction (SAL)

### SalInstance

The `SalInstance` class is the factory for all platform-specific objects:

```cpp
class SalInstance
{
public:
    virtual SalFrame* CreateFrame(SalFrame* pParent, SalFrameStyleFlags nStyle) = 0;
    virtual SalVirtualDevice* CreateVirtualDevice(SalGraphics& rGraphics, ...) = 0;
    virtual SalPrinter* CreatePrinter(SalInfoPrinter* pInfoPrinter) = 0;
    virtual SalMenu* CreateMenu(bool bMenuBar, Menu* pMenu) = 0;
    // ... more factory methods
};
```

Platform implementations:
- **WinSalInstance**: Windows implementation
- **AquaSalInstance**: macOS implementation  
- **GtkSalInstance**: GTK3/4 backend
- **QtInstance**: Qt5/6 backend
- **SvpSalInstance**: Headless backend

### SalFrame

Represents a native window:

```cpp
class SalFrame
{
public:
    virtual void Show(bool bVisible, bool bNoActivate) = 0;
    virtual void SetTitle(const OUString& rTitle) = 0;
    virtual void SetPosSize(tools::Long nX, tools::Long nY, 
                           tools::Long nWidth, tools::Long nHeight,
                           sal_uInt16 nFlags) = 0;
    virtual void GetWorkArea(tools::Rectangle& rRect) = 0;
    // Event handling
    virtual bool PostEvent(std::unique_ptr<ImplSVEvent> pData) = 0;
};
```

### SalGraphics

Platform graphics context abstraction:

```cpp
class SalGraphics
{
public:
    // Drawing operations
    virtual void DrawLine(tools::Long nX1, tools::Long nY1, 
                         tools::Long nX2, tools::Long nY2) = 0;
    virtual void DrawRect(tools::Long nX, tools::Long nY, 
                         tools::Long nWidth, tools::Long nHeight) = 0;
    virtual void DrawPolygon(sal_uInt32 nPoints, const Point* pPtAry) = 0;
    
    // Text rendering
    virtual void DrawTextLayout(const GenericSalLayout&) = 0;
    
    // Bitmap operations
    virtual void DrawBitmap(const SalTwoRect& rPosAry, 
                           const SalBitmap& rSalBitmap) = 0;
};
```

## Window System

### Window Hierarchy

```
WorkWindow / Dialog / SystemWindow
        ↓
    Window (vcl::Window)
        ↓
Control (Button, Edit, ListBox, etc.)
```

### Window Types

1. **SystemWindow**: Top-level windows with system decorations
2. **WorkWindow**: Main application windows
3. **Dialog**: Modal/modeless dialogs
4. **DockingWindow**: Dockable tool windows
5. **Control**: Interactive widgets

### Window Management

Key aspects:
- Parent-child relationships
- Z-order management  
- Focus handling
- Layout management
- Event propagation

Example window creation:
```cpp
VclPtr<PushButton> pButton = VclPtr<PushButton>::Create(pParent);
pButton->SetText("Click Me");
pButton->SetClickHdl(LINK(this, MyClass, ClickHdl));
pButton->Show();
```

## Rendering System

### OutputDevice

The `OutputDevice` class is the core rendering abstraction:

```cpp
class OutputDevice
{
public:
    // State management
    void Push(PushFlags nFlags = PushFlags::ALL);
    void Pop();
    
    // Drawing primitives
    void DrawLine(const Point& rStartPt, const Point& rEndPt);
    void DrawRect(const tools::Rectangle& rRect);
    void DrawPolygon(const tools::Polygon& rPoly);
    void DrawText(const Point& rStartPt, const OUString& rStr);
    
    // Transformations
    void SetMapMode(const MapMode& rNewMapMode);
    void SetClipRegion(const Region& rRegion);
};
```

### Rendering Pipeline

```
Application Drawing Call
        ↓
OutputDevice API
        ↓
RenderContext Setup
        ↓
SalGraphics Call
        ↓
Platform Graphics API
        ↓
GPU/Display
```

### Double Buffering

VCL implements automatic double buffering:

```cpp
class VirtualDevice : public OutputDevice
{
    // Off-screen rendering buffer
    // Blitted to screen on demand
};
```

## Event System

### Event Types

```cpp
enum class VclEventId
{
    WindowActivate,
    WindowDeactivate,
    WindowClose,
    WindowResize,
    WindowMove,
    WindowPaint,
    WindowMouseMove,
    WindowMouseButtonDown,
    WindowMouseButtonUp,
    WindowKeyInput,
    WindowKeyUp,
    // ... many more
};
```

### Event Flow

```
Native Event (OS)
      ↓
SalFrame Handler
      ↓
ImplSVEvent Queue
      ↓
Application::Yield()
      ↓
Window Handler
      ↓
Application Code
```

### Event Handlers

```cpp
// Link-based handlers
void MyWindow::SetClickHdl(const Link<Button*,void>& rLink);

// Virtual method handlers  
virtual void MouseMove(const MouseEvent& rMEvt);
virtual void KeyInput(const KeyEvent& rKEvt);
virtual void Paint(vcl::RenderContext& rRenderContext, 
                  const tools::Rectangle& rRect);
```

## Layout Management

### Layout Types

1. **Fixed Layout**: Absolute positioning
2. **Box Layout**: Horizontal/vertical boxes
3. **Grid Layout**: Table-like arrangement
4. **Builder Layout**: UI defined in .ui files

### VclBuilder

Loads UI from Glade XML files:

```cpp
VclPtr<Dialog> pDialog = VclPtr<Dialog>::Create(pParent, "MyDialog", 
                                                "modules/mymodule/ui/mydialog.ui");
VclPtr<Button> pButton = pDialog->get<Button>("okbutton");
```

### Size Management

```cpp
class Window
{
    // Preferred size calculation
    virtual Size GetOptimalSize() const;
    
    // Layout requisition
    void queue_resize();
    
    // Size allocation
    virtual void setAllocation(const Size& rAllocation);
};
```

## Graphics Subsystems

### Font Handling

```cpp
class Font
{
    OUString            maFamilyName;
    Size                maSize;
    FontWeight          meWeight;
    FontItalic          meItalic;
    // Platform font handle via SalGraphics
};
```

Font subsystem components:
- **PhysicalFontCollection**: System font enumeration
- **LogicalFontInstance**: Font + size + attributes
- **SalLayout**: Text shaping and positioning
- **HarfBuzz**: Complex text layout

### Image/Bitmap Handling

```cpp
class Bitmap
{
    // Pixel data access
    BitmapReadAccess* AcquireReadAccess();
    BitmapWriteAccess* AcquireWriteAccess();
    
    // Format conversion
    bool Convert(BmpConversion eConversion);
    
    // Scaling/filtering
    bool Scale(const Size& rNewSize, BmpScaleFlag nScaleFlag);
};
```

Supported formats:
- Internal: 1, 4, 8, 24, 32 bit
- Import/Export: PNG, JPEG, BMP, GIF, etc.

### Printing

```cpp
class Printer : public OutputDevice
{
    // Job control
    bool StartJob(const OUString& rJobName);
    bool EndJob();
    
    // Page control
    void StartPage();
    void EndPage();
};
```

## Platform Backends

### Windows Backend (win/)

- Uses GDI+ and Direct2D
- Native Windows message handling
- COM integration for accessibility
- DirectWrite for text rendering

Key classes:
- `WinSalFrame`: HWND wrapper
- `WinSalGraphics`: HDC wrapper
- `WinSalBitmap`: HBITMAP wrapper

### macOS Backend (osx/)

- Objective-C++ implementation
- Cocoa/AppKit integration
- Core Graphics rendering
- Core Text for fonts

Key classes:
- `AquaSalFrame`: NSWindow wrapper
- `AquaSalGraphics`: CGContext wrapper  
- `AquaSalBitmap`: CGImage wrapper

### GTK Backend (unx/gtk3/)

- GTK3/GTK4 widget integration
- Cairo rendering
- Pango text layout
- GDK event handling

Key classes:
- `GtkSalFrame`: GtkWindow wrapper
- `GtkSalGraphics`: Cairo context
- `GtkSalBitmap`: Cairo surface

### Qt Backend (qt5/, qt6/)

- Qt5/Qt6 widget integration
- QPainter rendering
- Qt event loop integration
- Native dialogs

Key classes:
- `QtFrame`: QWidget wrapper
- `QtGraphics`: QPainter wrapper
- `QtBitmap`: QImage wrapper

## Threading Model

### Main Thread

- All UI operations must occur on main thread
- Event loop runs on main thread
- Window creation/destruction

### Thread Safety

```cpp
class SolarMutexGuard
{
    // Acquires the global VCL mutex
    // Required for all VCL operations
};
```

### Idle/Timer Handling

```cpp
class Idle : public Task
{
    // Low-priority background tasks
    virtual void Invoke() = 0;
};

class Timer : public Task  
{
    // Time-based callbacks
    void SetTimeout(sal_uInt64 nTimeoutMs);
    virtual void Invoke() = 0;
};
```

## Resource Management

### VclPtr Smart Pointer

Reference-counted pointer for VCL objects:

```cpp
template<class T>
class VclPtr
{
    // Automatic disposal handling
    // Breaks reference cycles
    // Safe for stack allocation
};
```

Usage:
```cpp
{
    VclPtr<PushButton> pButton = VclPtr<PushButton>::Create(pParent);
    // ... use button
} // Automatically disposed
```

### Lifecycle Management

1. **Creation**: Via VclPtr<T>::Create()
2. **Ownership**: Parent window owns children
3. **Disposal**: Explicit dispose() or automatic
4. **Cleanup**: Deferred until safe

## Accessibility

### Accessibility Bridge

```cpp
class AccessibleContext
{
    // Implements platform a11y APIs
    virtual sal_Int32 getAccessibleChildCount();
    virtual Reference<XAccessible> getAccessibleChild(sal_Int32 i);
    virtual sal_Int16 getAccessibleRole();
};
```

Platform implementations:
- **Windows**: IAccessible2/UIA
- **macOS**: NSAccessibility  
- **Linux**: ATK/AT-SPI2

## Performance Considerations

### Rendering Optimizations

1. **Damage tracking**: Only redraw changed regions
2. **Clipping**: Reduce overdraw
3. **Caching**: Font and image caches
4. **Hardware acceleration**: When available

### Best Practices

1. **Batch operations**: Reduce state changes
2. **Virtual devices**: Off-screen composition
3. **Lazy loading**: Defer resource loading
4. **Event coalescing**: Combine related events

## Debugging VCL

### Debug Options

Environment variables:
- `SAL_USE_VCLPLUGIN`: Force specific backend
- `SAL_VCL_DEBUG_DISABLE_PDFEXPORT`: Disable PDF export
- `VCL_DOUBLEBUFFERING_AVOID_PAINT`: Disable double buffering

### Common Issues

1. **Thread violations**: Not using SolarMutexGuard
2. **Lifecycle bugs**: Using disposed windows
3. **Platform differences**: Assuming specific behavior
4. **Resource leaks**: Not using VclPtr

## Future Directions

### Ongoing Work

1. **Skia integration**: Modern 2D graphics library
2. **Vulkan backend**: GPU acceleration
3. **Wayland support**: Native Wayland protocol
4. **HiDPI improvements**: Better scaling support

### Modernization Efforts

1. **C++17 adoption**: Modern C++ features
2. **Unit testing**: Better test coverage
3. **Performance**: Continued optimization
4. **Documentation**: Improved API docs

---

This documentation covers VCL's architecture in detail. For specific widget documentation, see the Doxygen-generated API documentation.