/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#pragma once

#include <memory>
#include "ViewShell.hxx"
#include "tools/AsynchronousCall.hxx"
#include "TabControl.hxx"
#include <glob.hxx>
#include <pres.hxx>
#include <unotools/caserotate.hxx>
#include <unotools/options.hxx>
#include <sddllapi.h>
#include <viewopt.hxx>

namespace svx::sidebar { class SelectionChangeHandler; }
namespace com::sun::star::lang { class XEventListener; }
namespace com::sun::star::scanner { class XScannerManager2; }
namespace com::sun::star::presentation { class XSlideShow; }

class Outliner;
class SdPage;
class SdStyleSheet;
class SdrExternalToolEdit;
class TabBar;
class SdrObject;
class SdrPageView;
class TransferableDataHelper;
class TransferableClipboardListener;
class AbstractSvxNameDialog;
class SdrLayer;
class SvxClipboardFormatItem;
struct ESelection;
class AbstractSvxObjectNameDialog;

namespace sd {

class DrawView;
class LayerTabBar;
class Ruler;
class AnnotationManager;
class ViewOverlayManager;

template <typename MIN_T, typename T, typename MAX_T>
constexpr bool CHECK_RANGE(MIN_T nMin, T nValue, MAX_T nMax)
{
    return nValue >= nMin && nValue <= nMax;
}

/** Base class of the stacked shells that provide graphical views to
    Draw and Impress documents and editing functionality.  In contrast
    to this other stacked shells are responsible for showing an
    overview over several slides or a textual
    overview over the text in an Impress document (OutlineViewShell).
*/
class SAL_DLLPUBLIC_RTTI DrawViewShell
    : public ViewShell,
      public SfxListener,
      public utl::ConfigurationListener
{
public:
    SFX_DECL_INTERFACE(SD_IF_SDDRAWVIEWSHELL)

private:
    /// SfxInterface initializer.
    static void InitInterface_Impl();

public:
    /** Create a new stackable shell that may take some information
        (e.g. the frame view) from the given previous shell.
        @param ePageKind
            This parameter gives the initial page kind that the new shell
            will show.
        @param pFrameView
            The frame view that makes it possible to pass information from
            one view shell to the next.
    */
    DrawViewShell (
        ViewShellBase& rViewShellBase,
        vcl::Window* pParentWindow,
        PageKind ePageKind,
        FrameView* pFrameView);

    virtual ~DrawViewShell() override;

    virtual void Init (bool bIsMainViewShell) override;

    virtual void Shutdown() override;

    void PrePaint() override;
    virtual void Paint(const ::tools::Rectangle& rRect, ::sd::Window* pWin) override;

    /** Arrange and resize the GUI elements like rulers, sliders, and
        buttons as well as the actual document view according to the size of
        the enclosing window and current sizes of buttons, rulers, and
        sliders.
    */
    virtual void    ArrangeGUIElements() override;

    void            HidePage();

    virtual bool    KeyInput(const KeyEvent& rKEvt, ::sd::Window* pWin) override;
    virtual void    MouseMove(const MouseEvent& rMEvt, ::sd::Window* pWin) override;
    virtual void    MouseButtonUp(const MouseEvent& rMEvt, ::sd::Window* pWin) override;
    virtual void    MouseButtonDown(const MouseEvent& rMEvt, ::sd::Window* pWin) override;
    virtual void    Command(const CommandEvent& rCEvt, ::sd::Window* pWin) override;
    bool            IsMouseButtonDown() const { return mbMouseButtonDown; }
    bool            IsMouseSelecting() const { return mbMouseSelecting; }

    virtual void    Resize() override;

    void            ShowMousePosInfo(const ::tools::Rectangle& rRect, ::sd::Window const * pWin);

    virtual void    ChangeEditMode (EditMode eMode, bool bIsLayerModeActive);

    virtual void    SetZoom( ::tools::Long nZoom ) override;
    virtual void    SetZoomRect( const ::tools::Rectangle& rZoomRect ) override;

    void            InsertURLField(const OUString& rURL, const OUString& rText, const OUString& rTarget, OUString const& rAltText);
    void            InsertURLButton(const OUString& rURL, const OUString& rText, const OUString& rTarget,
                                    const Point* pPos);

    void            SelectionHasChanged();
    void            ModelHasChanged();
    virtual void    Activate(bool bIsMDIActivate) override;
    virtual void    Deactivate(bool IsMDIActivate) override;
    virtual void    UIActivating( SfxInPlaceClient* ) override;
    virtual void    UIDeactivated( SfxInPlaceClient* ) override;
    OUString        GetSelectionText( bool bCompleteWords );
    bool            HasSelection( bool bText ) const;

    //If we are editing a PresObjKind::Outline return the Outliner and fill rSel
    //with the current selection
    ::Outliner*     GetOutlinerForMasterPageOutlineTextObj(ESelection &rSel);

    void            ExecCtrl(SfxRequest& rReq);
    void            GetCtrlState(SfxItemSet& rSet);
    void            GetDrawAttrState(SfxItemSet& rSet);
    void            GetMenuState(SfxItemSet& rSet);
    void            GetTableMenuState(SfxItemSet& rSet);
    /** Set the items of the given item set that are related to
        switching the editing mode to the correct values.
        <p>This function also sets the states of the mode buttons
        (those at the upper right corner) accordingly.</p>
    */
    void            GetModeSwitchingMenuState (SfxItemSet &rSet);
    void            GetAttrState(SfxItemSet& rSet);
    void            GetSnapItemState(SfxItemSet& rSet);

    void            SetPageProperties (SfxRequest& rReq);
    void            GetPageProperties(SfxItemSet& rSet);
    void            GetMarginProperties(SfxItemSet& rSet);

    void            GetState (SfxItemSet& rSet);
    void            Execute (SfxRequest& rReq);

    void            ExecStatusBar(SfxRequest& rReq);
    void            GetStatusBarState(SfxItemSet& rSet);

    void            ExecOptionsBar(SfxRequest& rReq);
    void            GetOptionsBarState(SfxItemSet& rSet);

    void            ExecRuler(SfxRequest& rReq);
    void            GetRulerState(SfxItemSet& rSet);

    void            ExecFormText(SfxRequest& rReq);
    void            GetFormTextState(SfxItemSet& rSet);

    void            ExecAnimationWin(SfxRequest& rReq);
    void            GetAnimationWinState(SfxItemSet& rSet);

    void            ExecNavigatorWin(SfxRequest& rReq);
    void            GetNavigatorWinState(SfxItemSet& rSet);

    void            ExecutePropPanelAttr (SfxRequest const & rReq);
    void            GetStatePropPanelAttr(SfxItemSet& rSet);

    void            ExecEffectWin(SfxRequest& rReq);

    void            Update3DWindow();
    void            AssignFrom3DWindow();

    void            ExecGallery(SfxRequest const & rReq);

    void            ExecBmpMask( SfxRequest const & rReq );
    void            GetBmpMaskState( SfxItemSet& rSet );

    void            ExecIMap( SfxRequest const & rReq );
    void            GetIMapState( SfxItemSet& rSet );

    void            FuTemporary(SfxRequest& rReq);
    void            FuPermanent(SfxRequest& rReq);
    void            FuSupport(SfxRequest& rReq);
    void            FuDeleteSelectedObjects();
    void            FuSupportRotate(SfxRequest const & rReq);
    void            FuTable(SfxRequest& rReq);
    
    void            InsertFormControl(SfxRequest& rReq);

    void            AttrExec (SfxRequest& rReq);
    void            AttrState (SfxItemSet& rSet);

    void            ExecGoToNextPage (SfxRequest& rReq);
    void            GetStateGoToNextPage (SfxItemSet& rSet);

    void            ExecGoToPreviousPage (SfxRequest& rReq);
    void            GetStateGoToPreviousPage (SfxItemSet& rSet);

    void            ExecGoToFirstPage (SfxRequest& rReq);
    void            GetStateGoToFirstPage (SfxItemSet& rSet);

    void            ExecGoToLastPage (SfxRequest& rReq);
    void            GetStateGoToLastPage (SfxItemSet& rSet);

    void            ExecGoToPage (SfxRequest& rReq);
    void            GetStateGoToPage (SfxItemSet& rSet);

    SD_DLLPUBLIC void ExecChar(SfxRequest& rReq);

    void            ExecuteAnnotation (SfxRequest const & rRequest);
    void            GetAnnotationState (SfxItemSet& rItemSet);

    AnnotationManager* getAnnotationManagerPtr() { return mpAnnotationManager.get(); }

    void            StartRulerDrag (const Ruler& rRuler, const MouseEvent& rMEvt);

    virtual bool    PrepareClose( bool bUI = true ) override;

    PageKind        GetPageKind() const { return mePageKind; }
    void            SetPageKind( PageKind ePageKind ) { mePageKind = ePageKind; }
    const Point&    GetMousePos() const { return maMousePos; }

    EditMode        GetEditMode() const { return meEditMode; }
    virtual SdPage* GetActualPage() override { return mpActualPage; }

    /// inherited from sd::ViewShell
    virtual SdPage* getCurrentPage() const override;

    void            ResetActualPage();
    void            ResetActualLayer();
    SD_DLLPUBLIC bool SwitchPage(sal_uInt16 nPage, bool bAllowChangeFocus = true,
                                 bool bUpdateScrollbars = true);
    bool            IsSwitchPageAllowed() const;

    /**
     * Mark the desired page as selected (1), deselected (0), toggle (2).
     * nPage refers to the page in question.
     */
    bool            SelectPage(sal_uInt16 nPage, sal_uInt16 nSelect);
    bool            IsSelected(sal_uInt16 nPage);

    void            GotoBookmark(std::u16string_view rBookmark);
    //Realize multi-selection of objects, If object is marked, the
    //corresponding entry is set true, else the corresponding entry is set
    //false.
    void            FreshNavigatrTree();
    void            MakeVisible(const ::tools::Rectangle& rRect, vcl::Window& rWin);

    virtual void    ReadFrameViewData(FrameView* pView) override;
    virtual void    WriteFrameViewData() override;

    virtual ErrCode DoVerb(sal_Int32 nVerb) override;
    virtual bool    ActivateObject(SdrOle2Obj* pObj, sal_Int32 nVerb) override;

    void            SetZoomOnPage( bool bZoom ) { mbZoomOnPage = bZoom; }
    bool            IsZoomOnPage() const { return mbZoomOnPage; }
    static void     CheckLineTo (SfxRequest& rReq);
    void            SetChildWindowState( SfxItemSet& rSet );

    void            UpdateIMapDlg( SdrObject* pObj );

    void            LockInput();
    void            UnlockInput();
    bool            IsInputLocked() const { return mnLockCount > 0; }

    sal_uInt16      GetCurPagePos() const { return maTabControl->GetCurPagePos(); }

    /** Show controls of the UI or hide them, depending on the given flag.
        Do not call this method directly.  Call the method at ViewShellBase
        instead.
    */
    virtual void    ShowUIControls (bool bVisible) override;

    void            ScannerEvent();

    bool            IsLayerModeActive() const { return mbIsLayerModeActive;}

    virtual sal_Int8    AcceptDrop( const AcceptDropEvent& rEvt, DropTargetHelper& rTargetHelper,
                                    ::sd::Window* pTargetWindow, sal_uInt16 nPage, SdrLayerID nLayer ) override;
    virtual sal_Int8    ExecuteDrop( const ExecuteDropEvent& rEvt, DropTargetHelper& rTargetHelper,
                                    ::sd::Window* pTargetWindow, sal_uInt16 nPage, SdrLayerID nLayer ) override;

    virtual void    WriteUserDataSequence ( css::uno::Sequence < css::beans::PropertyValue >& ) override;
    virtual void    ReadUserDataSequence ( const css::uno::Sequence < css::beans::PropertyValue >& ) override;

    virtual void    VisAreaChanged(const ::tools::Rectangle& rRect) override;

    /** Create an accessible object representing the specified window.
        @param pWindow
            The returned object makes the document displayed in this window
            accessible.
        @return
            Returns an <type>AccessibleDrawDocumentView</type> object.
   */
    virtual rtl::Reference<comphelper::OAccessible>
    CreateAccessibleDocumentView(::sd::Window* pWindow) override;

    /** Return the number of layers managed by the layer tab control.  This
        will usually differ from the number of layers managed by the layer
        administrator.
        @return
            The number of layers managed by the layer tab control.  The
            returned value is independent of whether the layer mode is
            currently active and the tab control is visible.
    */
    int GetTabLayerCount() const;

    /** Return the numerical id of the currently active layer as seen by the
        layer tab control.
        @return
            The returned id is a number between zero (inclusive) and the
            number of layers as returned by the
            <member>GetTabLayerCount</member> method (exclusive).
    */
    int GetActiveTabLayerIndex() const;

    /** Set the active layer at the layer tab control and update the control
        accordingly to reflect the change on screen.
        @param nId
            The id is expected to be a number between zero (inclusive) and
            the number of layers as returned by the
            <member>GetTabLayerCount</member> method (exclusive). Note that
            Invalid values are ignored. No exception is thrown in that case.
    */
    void SetActiveTabLayerIndex (int nId);

    /** Return a pointer to the tab control for pages.
    */
    TabControl& GetPageTabControl() { return *maTabControl; }

    /** Return a pointer to the tab control for layers.
    */
    SD_DLLPUBLIC LayerTabBar* GetLayerTabControl(); // export for unit test

    /** Renames the given slide using an SvxNameDialog

        @param nPageId the index of the page in the SdTabControl.
        @param rName the new name of the slide.

        @return false, if the new name is invalid for some reason.

        <p>Implemented in <code>drviews8.cxx</code>.</p>
     */
    bool RenameSlide( sal_uInt16 nPageId, const OUString & rName );

    /** modifies the given layer with the given values */
    void ModifyLayer( SdrLayer* pLayer, const OUString& rLayerName, const OUString& rLayerTitle, const OUString& rLayerDesc, bool bIsVisible, bool bIsLocked, bool bIsPrintable );

    virtual css::uno::Reference<css::drawing::XDrawSubController> CreateSubController() override;

    DrawView*   GetDrawView() const { return mpDrawView.get(); }

    /** Relocation to a new parent window is not supported for DrawViewShell
        objects so this method always returns <FALSE/>.
    */
    virtual bool RelocateToParentWindow (vcl::Window* pParentWindow) override;

    OUString const & GetSidebarContextName() const;

    bool IsInSwitchPage() const { return mbIsInSwitchPage; }

    const SdViewOptions& GetViewOptions() const;
    void SetViewOptions(const SdViewOptions& rOptions);
    //move this method to ViewShell.
    //void  NotifyAccUpdate();

    void destroyXSlideShowInstance();

protected:
                    DECL_DLLPRIVATE_LINK( ClipboardChanged, TransferableDataHelper*, void );
                    DECL_DLLPRIVATE_LINK( TabSplitHdl, TabBar *, void );
                    DECL_DLLPRIVATE_LINK( NameObjectHdl, AbstractSvxObjectNameDialog&, bool );
                    DECL_DLLPRIVATE_LINK( RenameSlideHdl, AbstractSvxNameDialog&, bool );

    void            DeleteActualPage();
    void            DeleteActualLayer();

    virtual VclPtr<SvxRuler> CreateHRuler(::sd::Window* pWin) override;
    virtual VclPtr<SvxRuler> CreateVRuler(::sd::Window* pWin) override;
    virtual void    UpdateHRuler() override;
    virtual void    UpdateVRuler() override;
    virtual void    SetZoomFactor(const Fraction& rZoomX, const Fraction& rZoomY) override;

    void            SetupPage( Size const &rSize, ::tools::Long nLeft, ::tools::Long nRight, ::tools::Long nUpper, ::tools::Long nLower,
                               bool bSize, bool bMargin, bool bScaleAll );

    void            GetMenuStateSel(SfxItemSet& rSet);

private:
    DrawViewShell(const DrawViewShell&) = delete;
    DrawViewShell& operator=(const DrawViewShell&) = delete;

    void Construct (DrawDocShell* pDocSh, PageKind ePageKind);

    void ImplDestroy();

    /** Depending on the given request create a new page or duplicate an
        existing one.  See ViewShell::CreateOrDuplicatePage() for more
        information.
    */
    virtual SdPage* CreateOrDuplicatePage (
        SfxRequest& rRequest,
        PageKind ePageKind,
        SdPage* pPage,
        const sal_Int32 nInsertPosition = -1) override;

    void DuplicateSelectedSlides (SfxRequest& rRequest);

    virtual void Notify (SfxBroadcaster& rBC, const SfxHint& rHint) override;

    /** Stop a running slide show.
    */
    void StopSlideShow();

    /** Show the context menu for snap lines and points.  Because snap lines
        can not be selected the index of the snap line/point for which the
        popup menu is opened has to be passed to the processing slot
        handlers.  This can be done only by manually showing the popup menu.
        @param pParent
            The parent for the context menu.
        @param rRect
            The location at which to display the context menu.
        @param rPageView
            The page view is used to access the help lines.
        @param nSnapLineIndex
            Index of the snap line or snap point for which to show the
            context menu.
    */
    void ShowSnapLineContextMenu(weld::Window* pParent, const ::tools::Rectangle& rRect,
        SdrPageView& rPageView, const sal_uInt16 nSnapLineIndex);

    using ViewShell::Notify;

    virtual void ConfigurationChanged( utl::ConfigurationBroadcaster* pCb, ConfigurationHints ) override;

    void ConfigureAppBackgroundColor( svtools::ColorConfig* pColorConfig = nullptr );

    /// return true if "Edit Hyperlink" in context menu should be disabled
    bool ShouldDisableEditHyperlink() const;

private:
    std::unique_ptr<DrawView> mpDrawView;
    SdPage*             mpActualPage;
    ::tools::Rectangle           maMarkRect;
    Point               maMousePos;
    VclPtr<TabControl>  maTabControl;
    EditMode            meEditMode;
    PageKind            mePageKind;
    bool                mbZoomOnPage;
    bool                mbIsRulerDrag;
    sal_uLong           mnLockCount;
    bool                mbReadOnly;
    static bool         mbPipette;
    /** Prevents grabbing focus while loading - see tdf#83773 that introduced
        the grabbing, and tdf#150773 that needs grabbing disabled on loading
    */
    bool mbFirstTimeActivation = true;
    /** This flag controls whether the layer mode is active, i.e. the layer
        dialog is visible.
    */
    bool mbIsLayerModeActive;
    /** This item contains the clipboard formats of the current clipboard
        content that are supported both by that content and by the
        DrawViewShell.
    */
    ::std::unique_ptr<SvxClipboardFormatItem> mpCurrentClipboardFormats;
    /** On some occasions it is necessary to make SwitchPage calls
        asynchronously.
    */
    tools::AsynchronousCall maAsynchronousSwitchPageCall;
    /** This flag is used to prevent nested calls to SwitchPage().
    */
    bool mbIsInSwitchPage;
    RotateTransliteration m_aRotateCase;
    /** Listen for selection changes and broadcast context changes for the sidebar.
    */
    ::rtl::Reference<svx::sidebar::SelectionChangeHandler> mpSelectionChangeHandler;
    css::uno::Reference< css::scanner::XScannerManager2 > mxScannerManager;
    css::uno::Reference< css::lang::XEventListener >      mxScannerListener;
    rtl::Reference<TransferableClipboardListener>         mxClipEvtLstnr;
    bool                                                  mbPastePossible;
    bool                                                  mbMouseButtonDown;
    bool                                                  mbMouseSelecting;
    std::unique_ptr<AnnotationManager> mpAnnotationManager;
    std::unique_ptr<ViewOverlayManager> mpViewOverlayManager;
    std::vector<std::unique_ptr<SdrExternalToolEdit>> m_ExternalEdits;

    css::uno::Reference<css::presentation::XSlideShow> mxSlideShow;
};

/// Merge the background properties together and deposit the result in rMergeAttr
SD_DLLPUBLIC void MergePageBackgroundFilling(SdPage *pPage, SdStyleSheet *pStyleSheet, bool bMasterPage, SfxItemSet& rMergedAttr);

} // end of namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
