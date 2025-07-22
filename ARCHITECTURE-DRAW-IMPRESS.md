# Draw and Impress (SD Module) Architecture

## Overview

The SD module (from German "StarDraw") implements both Draw (vector graphics editor) and Impress (presentation software). These applications share the same codebase, with Impress adding presentation-specific features on top of Draw's drawing capabilities. The module leverages the SVX drawing layer extensively.

## Core Architecture

### Module Structure

```
SD Module
    ├── Draw Application
    │   ├── Vector Drawing
    │   ├── Page Management
    │   └── Export Formats
    └── Impress Application
        ├── Draw Features +
        ├── Slide Shows
        ├── Transitions/Animations
        └── Presenter Console
```

### Document Model

```cpp
class SdDrawDocument : public FmFormModel  // From SVX
{
    // Document type
    DocumentType meDocType;  // DOCUMENT_TYPE_DRAW or DOCUMENT_TYPE_IMPRESS
    
    // Page management
    std::vector<SdPage*> maPages;         // Regular pages
    std::vector<SdPage*> maMasterPages;   // Master slides
    
    // Presentation data (Impress only)
    std::unique_ptr<SdCustomShow> mpCustomShow;
    SdAnimationInfo* mpAnimationInfo;
    
    // Styles
    SdStyleSheetPool* mpStyleSheetPool;
};
```

### Page Architecture

```cpp
class SdPage : public FmFormPage  // From SVX
{
    // Page types
    PageKind mePageKind;  // PK_STANDARD, PK_NOTES, PK_HANDOUT
    
    // Layout
    AutoLayout meAutoLayout;
    OUString maLayoutName;
    
    // Presentation settings
    double mfTime;              // Display duration
    PresChange mePresChange;    // Manual/Auto/Semi-auto
    
    // Transition effects
    css::presentation::FadeEffect meFadeEffect;
    double mfTransitionDuration;
    
    // Master page link
    SdPage* mpMasterPage;
};
```

## Drawing Layer Integration

### SVX Drawing Objects

Draw/Impress uses the SVX drawing layer:

```cpp
// Object hierarchy from SVX
SdrObject
    ├── SdrTextObj
    │   ├── SdrRectObj
    │   ├── SdrCircObj
    │   └── SdrObjCustomShape
    ├── SdrGrafObj      // Images
    ├── SdrOle2Obj      // OLE objects
    ├── SdrTableObj     // Tables
    └── SdrMediaObj     // Audio/Video
```

### Custom Shape Engine

```cpp
class EnhancedCustomShapeEngine
{
    // Shape definitions
    Reference<XCustomShapeEngine> mxEngine;
    
    // Generate geometry
    Reference<XShape> CreateCustomShape(
        const OUString& rShapeType,
        const awt::Rectangle& rBounds);
    
    // Shape gallery
    static bool GetShapeTypeFromStr(const OUString& rStr, 
                                   MSO_SPT& rShapeType);
};
```

### Connector System

```cpp
class SdrEdgeObj : public SdrTextObj
{
    // Connection points
    SdrObject* mpConnectedNode1;
    SdrObject* mpConnectedNode2;
    
    // Routing algorithm
    XPolygon CalculateEdgeTrack(const XPolygon& rTrack,
                                SdrObject* pNode1,
                                SdrObject* pNode2);
};
```

## Impress-Specific Features

### Slide Show Engine

```cpp
class SlideShow
{
    // Presentation control
    Reference<presentation::XSlideShow> mxSlideShow;
    
    // Start presentation
    void StartPresentation();
    
    // Navigation
    void GotoNextSlide();
    void GotoPreviousSlide();
    void GotoSlide(sal_Int32 nSlide);
    
    // Effects
    void StartAnimations();
    void StopAnimations();
};
```

### Animation Framework

```cpp
namespace sd { namespace animations {

class CustomAnimationEffect
{
    // Effect properties
    Reference<XAnimationNode> mxNode;
    OUString maPresetId;
    double mfDuration;
    double mfDelay;
    
    // Target
    Any maTarget;  // Shape or paragraph
    
    // Timing
    sal_Int16 mnNodeType;  // ON_CLICK, WITH_PREVIOUS, AFTER_PREVIOUS
};

class CustomAnimationPane
{
    // Effect list
    std::vector<CustomAnimationEffectPtr> maEffects;
    
    // UI interaction
    void AddEffect(const CustomAnimationPresetPtr& pPreset);
    void RemoveEffect(const CustomAnimationEffectPtr& pEffect);
    void ModifyEffect(const CustomAnimationEffectPtr& pEffect);
};

}} // namespace sd::animations
```

### Transition Effects

```cpp
class TransitionPane
{
    // Available transitions
    std::vector<TransitionPresetPtr> maTransitions;
    
    // Apply transition
    void ApplyTransition(SdPage* pPage,
                        const TransitionPresetPtr& pPreset);
    
    // Preview
    void PlayTransitionEffect(const TransitionPreset& rPreset);
};

struct TransitionPreset
{
    sal_Int16 mnTransition;      // Type
    sal_Int16 mnSubtype;         // Variation
    bool mbDirection;            // Direction/Orientation
    sal_Int32 mnFadeColor;       // For fade transitions
};
```

### Presenter Console

```cpp
class PresenterConsole
{
    // Multiple displays
    Reference<XPresentation2> mxPresentation;
    
    // Console components
    std::unique_ptr<PresenterSlidePreview> mpCurrentSlide;
    std::unique_ptr<PresenterSlidePreview> mpNextSlide;
    std::unique_ptr<PresenterNotesView> mpNotesView;
    std::unique_ptr<PresenterTimer> mpTimer;
    
    // Display management
    void SetupMultiDisplay(sal_Int32 nDisplay);
};
```

## View System

### View Types

```cpp
class DrawViewShell : public ViewShell
{
    // Drawing operations
    virtual void SetZoom(long nZoom) override;
    virtual void SetZoomRect(const Rectangle& rZoomRect) override;
    
    // Tool handling
    void SetCurrentFunction(FuPoor* pFunction);
    
    // Selection
    void MarkListHasChanged();
};

class SlideViewShell : public ViewShell
{
    // Slide sorter specific
    void UpdateAllSlides();
    void SelectSlide(sal_uInt16 nSlide);
    void MoveSelectedSlides(sal_uInt16 nTargetPos);
};

class OutlineViewShell : public ViewShell
{
    // Outline editing
    void UpdateOutlineObject(SdPage* pPage);
    void PrepareClose();
};
```

### Drawing Functions

```cpp
class FuPoor  // Base class for all functions
{
    DrawViewShell* mpViewShell;
    SdrView* mpView;
    
    virtual bool MouseButtonDown(const MouseEvent& rMEvt);
    virtual bool MouseMove(const MouseEvent& rMEvt);
    virtual bool MouseButtonUp(const MouseEvent& rMEvt);
};

// Specific functions
class FuDraw : public FuPoor { };           // Basic drawing
class FuText : public FuDraw { };           // Text editing
class FuConstRectangle : public FuDraw { }; // Rectangle tool
class FuConstructBezierPolygon : public FuDraw { }; // Bezier tool
```

## Layout System

### Auto Layouts

```cpp
enum class AutoLayout
{
    TITLE,                    // Title slide
    TITLE_CONTENT,           // Title + content
    TITLE_2CONTENT,          // Title + 2 contents
    TITLE_CONTENT_CONTENT,   // Title + content + content
    TITLE_4CONTENT,          // Title + 4 contents
    // ... many more
};

class LayoutMenu
{
    // Apply layout
    void AssignLayout(AutoLayout eLayout);
    
    // Layout templates
    void CreateLayoutShapes(SdPage* pPage, AutoLayout eLayout);
};
```

### Master Pages

```cpp
class MasterPagesSelector
{
    // Master page management
    void ApplyMasterPage(SdPage* pMasterPage,
                        const PageList& rPageList);
    
    // Style inheritance
    void UpdateStyleSheets(SdPage* pPage);
};
```

## Import/Export

### Presentation Formats

```cpp
// PowerPoint import/export
class SdPPTImport : public SdrPowerPointImport
{
    bool Import();
    bool ImportSlide(sal_uInt16 nSlide);
    bool ImportMasterSlide(sal_uInt16 nMaster);
};

class SdPPTXExport
{
    bool ExportDocument();
    bool ExportSlide(sal_uInt16 nSlide);
    bool ExportMasterSlide(sal_uInt16 nMaster);
};
```

### Graphics Export

```cpp
class GraphicExporter
{
    // Export formats
    bool ExportGraphic(const Graphic& rGraphic,
                      const OUString& rPath,
                      const OUString& rFilterName);
    
    // Supported formats
    // PNG, JPEG, SVG, PDF, WMF, EMF, etc.
};
```

## 3D Capabilities

### 3D Scene Objects

```cpp
class E3dScene : public E3dObject
{
    // 3D objects container
    std::vector<E3dObject*> ma3DObjects;
    
    // Camera settings
    Camera3D aCamera;
    
    // Lighting
    std::vector<B3dLight> maLights;
};

class E3dObject : public SdrObject
{
    // 3D transformations
    basegfx::B3DHomMatrix maTransform;
    
    // Material properties
    Material3D maMaterial;
};
```

### 3D Effects

```cpp
class E3dView : public SdrView
{
    // 3D operations
    void ConvertMarkedObjTo3D();
    void Rotate3D(double fHAngle, double fVAngle);
    void SetPerspective(bool bPerspective);
};
```

## Slide Sorter

### Architecture

```cpp
namespace sd { namespace slidesorter {

class SlideSorter
{
    // Model
    std::unique_ptr<model::SlideSorterModel> mpModel;
    
    // View  
    std::unique_ptr<view::SlideSorterView> mpView;
    
    // Controller
    std::unique_ptr<controller::SlideSorterController> mpController;
};

namespace model {
    class PageDescriptor
    {
        SdPage* mpPage;
        sal_Int32 mnIndex;
        bool mbIsSelected;
        bool mbIsVisible;
    };
}

}} // namespace sd::slidesorter
```

## Performance Optimizations

### Rendering Cache

```cpp
class SlideRenderer
{
    // Thumbnail cache
    std::map<SdPage*, BitmapEx> maThumbnailCache;
    
    // Async rendering
    void QueueSlideRendering(SdPage* pPage);
    
    // Preview generation
    BitmapEx CreatePreview(SdPage* pPage, const Size& rSize);
};
```

### Smart Object Updates

```cpp
class SdrHint : public SfxHint
{
    SdrHintKind meHint;
    const SdrObject* mpObj;
    const SdrPage* mpPage;
};

// Selective invalidation
void SdDrawDocument::BroadcastChanged(const SdrHint& rHint)
{
    // Only update affected views/objects
}
```

## Collaborative Features

### Change Tracking

```cpp
class SdChangeAction
{
    ChangeActionType meType;
    DateTime maDateTime;
    OUString maAuthor;
    
    // Undo/Redo integration
    std::unique_ptr<SdrUndoAction> mpUndoAction;
};
```

### Comments/Annotations

```cpp
class AnnotationManager
{
    // Annotation data
    Reference<XAnnotation> CreateAnnotation(SdPage* pPage,
                                           const Point& rPos);
    
    // UI
    void ShowAnnotation(const Reference<XAnnotation>& xAnnotation);
};
```

## Special Features

### Smart Art Import

```cpp
namespace oox { namespace drawingml {

class DiagramImporter
{
    // Import SmartArt from OOXML
    bool ImportDiagram(const OUString& rDataModelPath,
                      const OUString& rLayoutPath,
                      const OUString& rStylePath);
    
    // Convert to shapes
    void LayoutDiagram(SdrObjGroup* pGroup);
};

}} // namespace oox::drawingml
```

### Media Playback

```cpp
class MediaObjectBar : public SfxShell
{
    // Media controls
    void Execute(SfxRequest& rReq);
    void GetState(SfxItemSet& rSet);
    
    // Playback
    void Play();
    void Pause();
    void Stop();
};
```

## Accessibility

### Accessible Objects

```cpp
class AccessibleDrawDocumentView : public AccessibleDocumentViewBase
{
    // Shape access
    Reference<XAccessible> GetAccessibleShape(const SdrObject* pObj);
    
    // Navigation
    sal_Int32 GetAccessibleChildCount() override;
    Reference<XAccessible> GetAccessibleChild(sal_Int32 i) override;
};
```

## Future Enhancements

### Planned Features

1. **Modern Rendering**: GPU-accelerated drawing
2. **Collaboration**: Real-time co-editing
3. **AI Features**: Smart layout suggestions
4. **Touch Support**: Better tablet/touch interaction
5. **Cloud Integration**: Direct cloud storage access

### Technical Improvements

- Modernize animation engine
- Better SVG support
- Enhanced 3D capabilities
- Improved performance for large presentations
- Better multimedia handling

---

This documentation covers the Draw and Impress architecture. The SD module demonstrates how LibreOffice reuses code effectively, with Impress building on Draw's foundation to add presentation capabilities.