# LibreOfficeKit API Documentation

## Overview

LibreOfficeKit (LOK) is a C/C++ API that allows embedding LibreOffice technology into other applications. It provides a simple, stable interface for rendering documents and handling user interaction without requiring a full LibreOffice installation. LOK is used in LibreOffice Online, mobile apps, and other embedding scenarios.

## Architecture

### Component Structure

```
LibreOfficeKit
    ├── Core API (C interface)
    ├── Document Rendering
    ├── User Input Handling
    ├── Callbacks System
    ├── View Management
    └── Platform Abstraction
```

### Initialization Flow

```
Application
    ↓
lok_init() → LibreOfficeKit instance
    ↓
lok_DocView (GTK) or custom implementation
    ↓
Document loading and rendering
    ↓
User interaction via callbacks
```

## Core API

### Initialization

```c
// Include the header
#include <LibreOfficeKit/LibreOfficeKit.h>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

// Initialize LibreOfficeKit
LibreOfficeKit* pKit = lok_init("/path/to/libreoffice/install");
if (!pKit) {
    fprintf(stderr, "Failed to initialize LibreOfficeKit\n");
    return 1;
}

// Get version information
char* version = pKit->pClass->getVersionInfo(pKit);
printf("LOK Version: %s\n", version);

// Set optional features
pKit->pClass->setOptionalFeatures(pKit, 
    LOK_FEATURE_DOCUMENT_PASSWORD |
    LOK_FEATURE_DOCUMENT_PASSWORD_TO_MODIFY |
    LOK_FEATURE_PART_IN_INVALIDATION_CALLBACK);
```

### Document Loading

```c
// Load a document
LibreOfficeKitDocument* pDocument = pKit->pClass->documentLoad(
    pKit, 
    "/path/to/document.odt"
);

if (!pDocument) {
    char* error = pKit->pClass->getError(pKit);
    fprintf(stderr, "Failed to load document: %s\n", error);
    free(error);
    return 1;
}

// Get document type
LibreOfficeKitDocumentType eType = pDocument->pClass->getDocumentType(pDocument);
switch (eType) {
    case LOK_DOCTYPE_TEXT:
        printf("Text document\n");
        break;
    case LOK_DOCTYPE_SPREADSHEET:
        printf("Spreadsheet\n");
        break;
    case LOK_DOCTYPE_PRESENTATION:
        printf("Presentation\n");
        break;
    case LOK_DOCTYPE_DRAWING:
        printf("Drawing\n");
        break;
}
```

### Document Rendering

```c
// Get document size
long nWidth, nHeight;
pDocument->pClass->getDocumentSize(pDocument, &nWidth, &nHeight);
printf("Document size: %ld x %ld\n", nWidth, nHeight);

// Create a bitmap buffer (RGBA)
int nCanvasWidth = 1024;
int nCanvasHeight = 768;
unsigned char* pBuffer = malloc(nCanvasWidth * nCanvasHeight * 4);

// Paint document to buffer
pDocument->pClass->paintTile(
    pDocument,
    pBuffer,
    nCanvasWidth, nCanvasHeight,  // Canvas size
    0, 0,                          // Tile position
    nWidth, nHeight                // Tile size in document coordinates
);

// Buffer now contains the rendered document
// Save to file or display in UI
```

### Advanced Rendering

```c
// Render specific part/page
int nParts = pDocument->pClass->getParts(pDocument);
printf("Document has %d parts\n", nParts);

// Select a part (e.g., slide in presentation)
pDocument->pClass->setPart(pDocument, 1);  // Second slide/sheet

// Get part name
char* partName = pDocument->pClass->getPartName(pDocument, 1);
printf("Part name: %s\n", partName);
free(partName);

// Render with custom DPI
int nTileTwipsWidth = 3000;   // Width in TWIPS (1/20 of a point)
int nTileTwipsHeight = 2000;  // Height in TWIPS
int nTilePixelWidth = 300;    // Output width in pixels
int nTilePixelHeight = 200;   // Output height in pixels

pDocument->pClass->paintTile(
    pDocument,
    pBuffer,
    nTilePixelWidth, nTilePixelHeight,
    0, 0,  // Position in document
    nTileTwipsWidth, nTileTwipsHeight
);
```

## Callback System

### Registering Callbacks

```c
// Callback function
static void documentCallback(int nType, const char* pPayload, void* pData) {
    printf("Callback: type=%d, payload=%s\n", nType, pPayload ? pPayload : "null");
    
    switch (nType) {
        case LOK_CALLBACK_INVALIDATE_TILES:
            // Part of document needs repainting
            // Payload: "x, y, width, height" or "EMPTY"
            break;
            
        case LOK_CALLBACK_INVALIDATE_VISIBLE_CURSOR:
            // Cursor position changed
            // Payload: "x, y, width, height"
            break;
            
        case LOK_CALLBACK_TEXT_SELECTION:
            // Text selection changed
            // Payload: selection rectangles
            break;
            
        case LOK_CALLBACK_CURSOR_VISIBLE:
            // Cursor visibility changed
            // Payload: "true" or "false"
            break;
            
        case LOK_CALLBACK_STATE_CHANGED:
            // UI state changed (toolbar items, etc.)
            // Payload: ".uno:Bold=true" or similar
            break;
    }
}

// Register callback
pDocument->pClass->registerCallback(
    pDocument, 
    documentCallback, 
    NULL  // User data
);
```

### Callback Types

```c
// Common callback types
enum LibreOfficeKitCallbackType {
    LOK_CALLBACK_INVALIDATE_TILES = 0,
    LOK_CALLBACK_INVALIDATE_VISIBLE_CURSOR = 1,
    LOK_CALLBACK_TEXT_SELECTION = 2,
    LOK_CALLBACK_CURSOR_VISIBLE = 3,
    LOK_CALLBACK_SEARCH_NOT_FOUND = 4,
    LOK_CALLBACK_SEARCH_RESULT_SELECTION = 5,
    LOK_CALLBACK_DOCUMENT_SIZE_CHANGED = 6,
    LOK_CALLBACK_SET_PART = 7,
    LOK_CALLBACK_STATE_CHANGED = 8,
    LOK_CALLBACK_STATUS_INDICATOR_START = 9,
    LOK_CALLBACK_STATUS_INDICATOR_SET_VALUE = 10,
    LOK_CALLBACK_STATUS_INDICATOR_FINISH = 11,
    LOK_CALLBACK_ERROR = 12,
    LOK_CALLBACK_CONTEXT_MENU = 13,
    LOK_CALLBACK_INVALIDATE_VIEW_CURSOR = 14,
    LOK_CALLBACK_TEXT_VIEW_SELECTION = 15,
    LOK_CALLBACK_CELL_VIEW_CURSOR = 16,
    // ... many more
};
```

## User Input

### Mouse Events

```c
// Mouse button press
pDocument->pClass->postMouseEvent(
    pDocument,
    LOK_MOUSEEVENT_BUTTONDOWN,
    100, 200,  // x, y in document coordinates
    1,         // Click count
    MOUSE_LEFT,
    0          // Modifier keys
);

// Mouse move
pDocument->pClass->postMouseEvent(
    pDocument,
    LOK_MOUSEEVENT_MOVE,
    150, 250,
    1,
    MOUSE_LEFT,
    0
);

// Mouse button release
pDocument->pClass->postMouseEvent(
    pDocument,
    LOK_MOUSEEVENT_BUTTONUP,
    150, 250,
    1,
    MOUSE_LEFT,
    0
);
```

### Keyboard Events

```c
// Key press
pDocument->pClass->postKeyEvent(
    pDocument,
    LOK_KEYEVENT_KEYINPUT,
    'A',           // Character code
    KEY_A          // Key code
);

// Key with modifiers
pDocument->pClass->postKeyEvent(
    pDocument,
    LOK_KEYEVENT_KEYINPUT,
    'C',
    KEY_C | KEY_MOD1  // Ctrl+C
);

// Special keys
pDocument->pClass->postKeyEvent(
    pDocument,
    LOK_KEYEVENT_KEYINPUT,
    0,
    KEY_RETURN
);
```

### Text Input

```c
// Insert text at cursor
pDocument->pClass->postUnoCommand(
    pDocument,
    ".uno:InsertText",
    "{\"Text\":{\"type\":\"string\",\"value\":\"Hello World\"}}",
    false  // Don't notify
);

// Paste from clipboard
pDocument->pClass->paste(
    pDocument,
    "text/plain;charset=utf-8",
    "Content to paste",
    strlen("Content to paste")
);
```

## View Management

### Multiple Views

```c
// Create a new view
int nViewId = pDocument->pClass->createView(pDocument);
printf("Created view: %d\n", nViewId);

// Get current view
int nCurrentView = pDocument->pClass->getView(pDocument);

// Set active view
pDocument->pClass->setView(pDocument, nViewId);

// Destroy view
pDocument->pClass->destroyView(pDocument, nViewId);

// Get all views
int* pViewIds = NULL;
int nViewCount = pDocument->pClass->getViewsCount(pDocument);
if (nViewCount > 0) {
    pViewIds = malloc(sizeof(int) * nViewCount);
    pDocument->pClass->getViewIds(pDocument, pViewIds, nViewCount);
    
    for (int i = 0; i < nViewCount; i++) {
        printf("View ID: %d\n", pViewIds[i]);
    }
    free(pViewIds);
}
```

### View Callbacks

```c
// View-specific callback
static void viewCallback(int nType, const char* pPayload, void* pData) {
    int nViewId = (int)(intptr_t)pData;
    printf("View %d callback: type=%d\n", nViewId, nType);
    
    switch (nType) {
        case LOK_CALLBACK_INVALIDATE_VIEW_CURSOR:
            // Other view's cursor moved
            break;
        case LOK_CALLBACK_TEXT_VIEW_SELECTION:
            // Other view's selection changed
            break;
        case LOK_CALLBACK_VIEW_CURSOR_VISIBLE:
            // Other view's cursor visibility
            break;
    }
}

// Register view callback
pDocument->pClass->registerCallback(
    pDocument, 
    viewCallback, 
    (void*)(intptr_t)nViewId
);
```

## UNO Commands

### Executing Commands

```c
// Execute UNO command without parameters
pDocument->pClass->postUnoCommand(
    pDocument,
    ".uno:Bold",
    NULL,
    false
);

// Execute with parameters (JSON format)
const char* params = "{"
    "\"FontHeight.Height\":{"
        "\"type\":\"float\","
        "\"value\":\"18\""
    "}"
"}";

pDocument->pClass->postUnoCommand(
    pDocument,
    ".uno:FontHeight",
    params,
    false
);

// Common commands
pDocument->pClass->postUnoCommand(pDocument, ".uno:Save", NULL, false);
pDocument->pClass->postUnoCommand(pDocument, ".uno:Undo", NULL, false);
pDocument->pClass->postUnoCommand(pDocument, ".uno:Redo", NULL, false);
pDocument->pClass->postUnoCommand(pDocument, ".uno:Cut", NULL, false);
pDocument->pClass->postUnoCommand(pDocument, ".uno:Copy", NULL, false);
pDocument->pClass->postUnoCommand(pDocument, ".uno:Paste", NULL, false);
```

### Getting Command State

```c
// Get state of a command
char* pState = pDocument->pClass->getCommandValues(
    pDocument,
    ".uno:Bold"
);

if (pState) {
    printf("Bold state: %s\n", pState);
    // Parse JSON response
    free(pState);
}

// Get multiple states at once
const char* commands = "{"
    "\"commands\":["
        "\".uno:Bold\","
        "\".uno:Italic\","
        "\".uno:Underline\""
    "]"
"}";

char* pStates = pDocument->pClass->getCommandValues(
    pDocument,
    ".uno:CommandStatus",
    commands
);
```

## GTK+ Integration

### LOKDocView Widget

```c
#include <gtk/gtk.h>
#include <LibreOfficeKitGtk/LibreOfficeKitGtk.h>

// Create LOKDocView widget
GtkWidget* pDocView = lok_doc_view_new(NULL, NULL, NULL);

// Load document
lok_doc_view_open_document(
    LOK_DOC_VIEW(pDocView),
    "/path/to/document.odt",
    NULL,  // Password
    NULL,  // Cancellable
    openDocumentCallback,
    NULL   // User data
);

// Handle signals
g_signal_connect(pDocView, "load-changed", 
                 G_CALLBACK(onLoadChanged), NULL);
g_signal_connect(pDocView, "command-changed", 
                 G_CALLBACK(onCommandChanged), NULL);
g_signal_connect(pDocView, "search-not-found", 
                 G_CALLBACK(onSearchNotFound), NULL);

// Add to container
gtk_container_add(GTK_CONTAINER(window), pDocView);
gtk_widget_show_all(window);
```

### LOKDocView Signals

```c
// Document loaded signal
static void onLoadChanged(LOKDocView* pDocView, 
                         gdouble fProgress, 
                         gpointer pData) {
    if (fProgress == 1.0) {
        printf("Document loaded\n");
        
        // Get LibreOfficeKitDocument
        LibreOfficeKitDocument* pDocument = 
            lok_doc_view_get_document(pDocView);
    }
}

// Command state changed
static void onCommandChanged(LOKDocView* pDocView,
                            gchar* pCommand,
                            gpointer pData) {
    printf("Command changed: %s\n", pCommand);
}

// Part changed (page/slide/sheet)
static void onPartChanged(LOKDocView* pDocView,
                         gint nPart,
                         gpointer pData) {
    printf("Switched to part: %d\n", nPart);
}
```

## Mobile Platform Integration

### iOS Integration

```objc
// Objective-C wrapper
@interface LOKDocument : NSObject
@property (nonatomic, readonly) LibreOfficeKitDocument* document;

- (instancetype)initWithPath:(NSString*)path;
- (UIImage*)renderTileAtZoom:(CGFloat)zoom 
                      region:(CGRect)region 
                        size:(CGSize)size;
- (void)postTouchEvent:(UITouch*)touch;
@end

@implementation LOKDocument
- (UIImage*)renderTileAtZoom:(CGFloat)zoom 
                      region:(CGRect)region 
                        size:(CGSize)size {
    // Create bitmap context
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        NULL, size.width, size.height, 8, 0,
        colorSpace, kCGImageAlphaPremultipliedLast);
    
    // Get buffer
    void* buffer = CGBitmapContextGetData(context);
    
    // Render tile
    self.document->pClass->paintTile(
        self.document,
        buffer,
        size.width, size.height,
        region.origin.x, region.origin.y,
        region.size.width, region.size.height
    );
    
    // Create UIImage
    CGImageRef cgImage = CGBitmapContextCreateImage(context);
    UIImage* image = [UIImage imageWithCGImage:cgImage];
    
    // Cleanup
    CGImageRelease(cgImage);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    
    return image;
}
@end
```

### Android Integration

```java
// Java wrapper using JNI
public class LOKDocument {
    private long nativeHandle;
    
    static {
        System.loadLibrary("lo-native");
    }
    
    public LOKDocument(String path) {
        nativeHandle = nativeOpen(path);
    }
    
    public Bitmap renderTile(int width, int height, 
                           float x, float y, 
                           float tileWidth, float tileHeight) {
        Bitmap bitmap = Bitmap.createBitmap(
            width, height, Bitmap.Config.ARGB_8888);
        nativeRenderTile(nativeHandle, bitmap, 
                        x, y, tileWidth, tileHeight);
        return bitmap;
    }
    
    // Native methods
    private native long nativeOpen(String path);
    private native void nativeRenderTile(long handle, Bitmap bitmap,
                                       float x, float y, 
                                       float tw, float th);
    private native void nativeDestroy(long handle);
}
```

## Advanced Features

### Collaborative Editing

```c
// Enable collaborative features
pDocument->pClass->setViewLanguage(pDocument, nViewId, "en-US");

// Set author information
pDocument->pClass->setAuthor(pDocument, "John Doe");

// Track changes by different views
static void collaborativeCallback(int nType, const char* pPayload, void* pData) {
    switch (nType) {
        case LOK_CALLBACK_VIEW_CURSOR_VISIBLE:
            // Parse: {"viewId": "1", "visible": "true"}
            break;
            
        case LOK_CALLBACK_VIEW_LOCK:
            // Parse: {"viewId": "1", "rectangle": "x,y,w,h"}
            break;
            
        case LOK_CALLBACK_TEXT_VIEW_SELECTION:
            // Parse: {"viewId": "1", "selection": "rectangles"}
            break;
    }
}
```

### Comments and Annotations

```c
// Add comment
const char* commentParams = "{"
    "\"Text\": {"
        "\"type\": \"string\","
        "\"value\": \"This needs review\""
    "},"
    "\"Author\": {"
        "\"type\": \"string\","
        "\"value\": \"John Doe\""
    "}"
"}";

pDocument->pClass->postUnoCommand(
    pDocument,
    ".uno:InsertAnnotation",
    commentParams,
    false
);

// Get all comments
char* pComments = pDocument->pClass->getCommandValues(
    pDocument,
    ".uno:ViewAnnotations"
);
```

### Form Controls

```c
// Interact with form controls
void handleFormField(const char* pPayload) {
    // Parse JSON payload
    // {"action": "show", "type": "drop-down", 
    //  "items": ["Option1", "Option2"], "selected": "0"}
    
    // Show dropdown to user and handle selection
    int nSelected = showDropdown(items);
    
    // Send selection back
    char response[256];
    snprintf(response, sizeof(response),
             "{\"type\": \"drop-down\", \"selected\": \"%d\"}", 
             nSelected);
    
    pDocument->pClass->sendFormFieldEvent(
        pDocument,
        "drop-down",
        response
    );
}
```

## Performance Optimization

### Tile Caching

```c
typedef struct {
    int x, y, width, height;
    unsigned char* buffer;
    time_t timestamp;
} TileCache;

// Simple tile cache implementation
TileCache* tileCache[CACHE_SIZE];
int cacheIndex = 0;

unsigned char* getTile(LibreOfficeKitDocument* pDoc,
                      int x, int y, int w, int h) {
    // Check cache
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (tileCache[i] && 
            tileCache[i]->x == x && 
            tileCache[i]->y == y &&
            tileCache[i]->width == w && 
            tileCache[i]->height == h) {
            return tileCache[i]->buffer;
        }
    }
    
    // Render new tile
    unsigned char* buffer = malloc(w * h * 4);
    pDoc->pClass->paintTile(pDoc, buffer, w, h, x, y, w, h);
    
    // Add to cache
    if (tileCache[cacheIndex]) {
        free(tileCache[cacheIndex]->buffer);
        free(tileCache[cacheIndex]);
    }
    
    tileCache[cacheIndex] = malloc(sizeof(TileCache));
    tileCache[cacheIndex]->x = x;
    tileCache[cacheIndex]->y = y;
    tileCache[cacheIndex]->width = w;
    tileCache[cacheIndex]->height = h;
    tileCache[cacheIndex]->buffer = buffer;
    tileCache[cacheIndex]->timestamp = time(NULL);
    
    cacheIndex = (cacheIndex + 1) % CACHE_SIZE;
    
    return buffer;
}
```

### Async Operations

```c
// Async rendering with threads
typedef struct {
    LibreOfficeKitDocument* pDocument;
    int x, y, width, height;
    void (*callback)(unsigned char* buffer, void* data);
    void* userData;
} RenderTask;

void* renderThread(void* arg) {
    RenderTask* task = (RenderTask*)arg;
    
    unsigned char* buffer = malloc(task->width * task->height * 4);
    task->pDocument->pClass->paintTile(
        task->pDocument,
        buffer,
        task->width, task->height,
        task->x, task->y,
        task->width, task->height
    );
    
    task->callback(buffer, task->userData);
    free(task);
    
    return NULL;
}

void renderTileAsync(LibreOfficeKitDocument* pDoc,
                    int x, int y, int w, int h,
                    void (*callback)(unsigned char*, void*),
                    void* userData) {
    RenderTask* task = malloc(sizeof(RenderTask));
    task->pDocument = pDoc;
    task->x = x;
    task->y = y;
    task->width = w;
    task->height = h;
    task->callback = callback;
    task->userData = userData;
    
    pthread_t thread;
    pthread_create(&thread, NULL, renderThread, task);
    pthread_detach(thread);
}
```

## Error Handling

### Error Checking

```c
// Comprehensive error handling
typedef enum {
    LOK_ERROR_NONE = 0,
    LOK_ERROR_INIT_FAILED,
    LOK_ERROR_DOCUMENT_LOAD_FAILED,
    LOK_ERROR_INVALID_PARAMETER,
    LOK_ERROR_OUT_OF_MEMORY,
    LOK_ERROR_UNKNOWN
} LOKError;

typedef struct {
    LOKError code;
    char message[256];
} LOKErrorInfo;

LOKErrorInfo lastError = {LOK_ERROR_NONE, ""};

void setError(LOKError code, const char* message) {
    lastError.code = code;
    strncpy(lastError.message, message, sizeof(lastError.message) - 1);
}

LibreOfficeKitDocument* safeDocumentLoad(LibreOfficeKit* pKit, 
                                        const char* pPath) {
    if (!pKit || !pPath) {
        setError(LOK_ERROR_INVALID_PARAMETER, 
                "Invalid kit or path");
        return NULL;
    }
    
    LibreOfficeKitDocument* pDoc = 
        pKit->pClass->documentLoad(pKit, pPath);
    
    if (!pDoc) {
        char* error = pKit->pClass->getError(pKit);
        setError(LOK_ERROR_DOCUMENT_LOAD_FAILED, 
                error ? error : "Unknown error");
        free(error);
        return NULL;
    }
    
    return pDoc;
}
```

## Best Practices

### Memory Management

1. **Always free allocated memory**: Strings returned by LOK
2. **Use reference counting**: For document instances
3. **Clear callbacks**: Before destroying documents
4. **Cache rendered tiles**: Avoid re-rendering

### Thread Safety

1. **LOK is not thread-safe**: Use from single thread
2. **Rendering can be async**: But API calls must be serialized
3. **Use message queues**: For multi-threaded apps
4. **Protect shared data**: When using callbacks

### Performance Tips

1. **Render visible area first**: Prioritize user viewport
2. **Use appropriate tile sizes**: Balance quality/performance
3. **Implement progressive rendering**: Low quality first
4. **Batch commands**: Reduce API call overhead

---

This documentation covers the LibreOfficeKit API for embedding LibreOffice functionality into applications. LOK provides a powerful yet simple interface for document rendering and manipulation across platforms.