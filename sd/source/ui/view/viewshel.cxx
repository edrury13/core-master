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

#include <framework/FrameworkHelper.hxx>
#include <framework/ViewShellWrapper.hxx>
#include <framework/ConfigurationController.hxx>
#include <memory>
#include <ViewShell.hxx>
#include <ViewShellImplementation.hxx>
#include <createtableobjectbar.hxx>

#include <ViewShellBase.hxx>
#include <ShellFactory.hxx>
#include <DrawController.hxx>
#include <LayerTabBar.hxx>
#include <Outliner.hxx>
#include <ResourceId.hxx>

#include <sal/log.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/DocumentTimer.hxx>
#include <vcl/commandevent.hxx>
#include <svl/eitem.hxx>
#include <svx/ruler.hxx>
#include <svx/svxids.hrc>
#include <svx/fmshell.hxx>
#include <WindowUpdater.hxx>
#include <sdxfer.hxx>

#include <app.hrc>

#include <OutlineView.hxx>
#include <DrawViewShell.hxx>
#include <DrawDocShell.hxx>
#include <slideshow.hxx>
#include <drawdoc.hxx>
#include <sdpage.hxx>
#include <zoomlist.hxx>
#include <FrameView.hxx>
#include <BezierObjectBar.hxx>
#include <TextObjectBar.hxx>
#include <GraphicObjectBar.hxx>
#include <MediaObjectBar.hxx>
#include <SlideSorter.hxx>
#include <SlideSorterViewShell.hxx>
#include <ViewShellManager.hxx>
#include <FormShellManager.hxx>
#include <EventMultiplexer.hxx>
#include <svx/extrusionbar.hxx>
#include <svx/fontworkbar.hxx>
#include <svx/svdoutl.hxx>
#include <tools/svborder.hxx>
#include <comphelper/lok.hxx>

#include <svl/slstitm.hxx>
#include <sfx2/request.hxx>
#include <SpellDialogChildWindow.hxx>
#include <controller/SlideSorterController.hxx>
#include <controller/SlsPageSelector.hxx>
#include <controller/SlsSelectionObserver.hxx>
#include <view/SlideSorterView.hxx>

#include <basegfx/utils/zoomtools.hxx>

#include <Window.hxx>
#include <fupoor.hxx>
#include <futext.hxx>

#include <editeng/numitem.hxx>
#include <editeng/eeitem.hxx>
#include <editeng/editview.hxx>
#include <editeng/editeng.hxx>
#include <editeng/editund2.hxx>
#include <svl/itempool.hxx>
#include <svl/intitem.hxx>
#include <svl/poolitem.hxx>
#include <strings.hxx>
#include <sdmod.hxx>
#include <AccessibleDocumentViewBase.hxx>

#include <framework/Configuration.hxx>
#include <framework/AbstractView.hxx>
#include <com/sun/star/frame/XFrame.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::presentation;

namespace {

class ViewShellObjectBarFactory
    : public ::sd::ShellFactory<SfxShell>
{
public:
    explicit ViewShellObjectBarFactory (::sd::ViewShell& rViewShell);
    virtual SfxShell* CreateShell( ::sd::ShellId nId ) override;
    virtual void ReleaseShell (SfxShell* pShell) override;
private:
    ::sd::ViewShell& mrViewShell;
};

} // end of anonymous namespace

namespace sd {

/// When true, scrolling to bottom of a page switches to the next page.
bool ViewShell::CanPanAcrossPages() const
{
    return dynamic_cast<const DrawViewShell*>(this) && mpContentWindow &&
        mpContentWindow->GetVisibleHeight() < 1.0;
}

bool ViewShell::IsPageFlipMode() const
{
    return dynamic_cast< const DrawViewShell *>( this ) !=  nullptr && mpContentWindow &&
        mpContentWindow->GetVisibleHeight() >= 1.0;
}

SfxViewFrame* ViewShell::GetViewFrame() const
{
    const SfxViewShell* pViewShell = GetViewShell();
    if (pViewShell != nullptr)
    {
        return &pViewShell->GetViewFrame();
    }
    else
    {
        OSL_ASSERT (GetViewShell()!=nullptr);
        return nullptr;
    }
}

/// declare SFX-Slotmap and standard interface

ViewShell::ViewShell( vcl::Window* pParentWindow, ViewShellBase& rViewShellBase)
    :   SfxShell(&rViewShellBase)
    ,   mbHasRulers(false)
    ,   mpActiveWindow(nullptr)
    ,   mpView(nullptr)
    ,   mpFrameView(nullptr)
    ,   mpZoomList(new ZoomList( *this ))
    ,   mfLastZoomScale(0)
    ,   mbStartShowWithDialog(false)
    ,   mnPrintedHandoutPageNum(1)
    ,   mnPrintedHandoutPageCount(0)
    ,   meShellType(ST_NONE)
    ,   mpImpl(new Implementation(*this))
    ,   mpParentWindow(pParentWindow)
    ,   mpWindowUpdater(new ::sd::WindowUpdater())
    ,   m_pDocumentTimer(new sfx2::DocumentTimer(&rViewShellBase))
{
    OSL_ASSERT (GetViewShell()!=nullptr);

    if (IsMainViewShell())
        GetDocSh()->Connect (this);

    mpContentWindow.reset(VclPtr< ::sd::Window >::Create(GetParentWindow()));
    SetActiveWindow (mpContentWindow.get());

    GetParentWindow()->SetBackground(Application::GetSettings().GetStyleSettings().GetFaceColor());
    mpContentWindow->SetBackground (Wallpaper());
    mpContentWindow->SetCenterAllowed(true);
    mpContentWindow->SetViewShell(this);
    mpContentWindow->SetPosSizePixel(
        GetParentWindow()->GetPosPixel(),GetParentWindow()->GetSizePixel());

    if ( ! GetDocSh()->IsPreview())
    {
        // Create scroll bars and the filler between the scroll bars.
        mpHorizontalScrollBar.reset (VclPtr<ScrollAdaptor>::Create(GetParentWindow(), true));
        mpHorizontalScrollBar->EnableRTL (false);
        mpHorizontalScrollBar->SetRange(Range(0, 32000));
        mpHorizontalScrollBar->SetScrollHdl(LINK(this, ViewShell, HScrollHdl));

        mpVerticalScrollBar.reset (VclPtr<ScrollAdaptor>::Create(GetParentWindow(), false));
        mpVerticalScrollBar->SetRange(Range(0, 32000));
        mpVerticalScrollBar->SetScrollHdl(LINK(this, ViewShell, VScrollHdl));
    }

    SetName (u"ViewShell"_ustr);

    GetDoc()->StartOnlineSpelling(false);

    mpWindowUpdater->SetDocument (GetDoc());

    // Re-initialize the spell dialog.
    ::sd::SpellDialogChildWindow* pSpellDialog =
          static_cast< ::sd::SpellDialogChildWindow*> (
              GetViewFrame()->GetChildWindow (
                  ::sd::SpellDialogChildWindow::GetChildWindowId()));
    if (pSpellDialog != nullptr)
        pSpellDialog->InvalidateSpellDialog();

    // Register the sub shell factory.
    mpImpl->mpSubShellFactory = std::make_shared<ViewShellObjectBarFactory>(*this);
    GetViewShellBase().GetViewShellManager()->AddSubShellFactory(this,mpImpl->mpSubShellFactory);
    
    // Force initial timer display update
    if (m_pDocumentTimer)
    {
        GetViewShellBase().GetViewFrame().GetBindings().Invalidate(SID_DOC_TIMER);
        GetViewShellBase().GetViewFrame().GetBindings().Update(SID_DOC_TIMER);
    }
}

ViewShell::~ViewShell()
{
    // Keep the content window from accessing in its destructor the
    // WindowUpdater.
    if (mpContentWindow)
        suppress_fun_call_w_exception(mpContentWindow->SetViewShell(nullptr));

    mpZoomList.reset();

    mpLayerTabBar.disposeAndClear();

    if (mpImpl->mpSubShellFactory)
        GetViewShellBase().GetViewShellManager()->RemoveSubShellFactory(
            this,mpImpl->mpSubShellFactory);

    if (mpContentWindow)
    {
        SAL_INFO(
            "sd.view",
            "destroying mpContentWindow at " << mpContentWindow.get()
                << " with parent " << mpContentWindow->GetParent());
        mpContentWindow.disposeAndClear();
    }

    mpVerticalRuler.disposeAndClear();
    mpHorizontalRuler.disposeAndClear();
    mpVerticalScrollBar.disposeAndClear();
    mpHorizontalScrollBar.disposeAndClear();
}

void ViewShell::doShow()
{
    mpContentWindow->Show();
    static_cast< vcl::Window*>(mpContentWindow.get())->Resize();
    SAL_INFO(
        "sd.view",
        "content window has size " << mpContentWindow->GetSizePixel().Width()
            << " " << mpContentWindow->GetSizePixel().Height());

    if ( ! GetDocSh()->IsPreview())
    {
        // Show scroll bars
        mpHorizontalScrollBar->Show();

        mpVerticalScrollBar->Show();
        maScrBarWH = Size(
            mpVerticalScrollBar->GetSizePixel().Width(),
            mpHorizontalScrollBar->GetSizePixel().Height());
    }

    GetParentWindow()->Show();
}

void ViewShell::Init (bool bIsMainViewShell)
{
    mpImpl->mbIsInitialized = true;
    SetIsMainViewShell(bIsMainViewShell);
    if (bIsMainViewShell)
        SetActiveWindow (mpContentWindow.get());
}

void ViewShell::Exit()
{
    sd::View* pView = GetView();
    if (pView!=nullptr && pView->IsTextEdit())
    {
        pView->SdrEndTextEdit();
        pView->UnmarkAll();
    }

    Deactivate (true);

    if (IsMainViewShell())
        GetDocSh()->Disconnect(this);

    SetIsMainViewShell(false);
}

/**
 * set focus to working window
 */
void ViewShell::Activate(bool bIsMDIActivate)
{
    // Do not forward to SfxShell::Activate()

    /* According to MI, nobody is allowed to call GrabFocus, who does not
       exactly know from which window the focus is grabbed. Since Activate()
       is sent sometimes asynchronous,  it can happen, that the wrong window
       gets the focus. */

    if (mpHorizontalRuler)
        mpHorizontalRuler->SetActive();
    if (mpVerticalRuler)
        mpVerticalRuler->SetActive();

    if (bIsMDIActivate)
    {
        // thus, the Navigator will also get a current status
        SfxBoolItem aItem( SID_NAVIGATOR_INIT, true );
        if (GetDispatcher() != nullptr)
        {
            SfxCallMode nCall = SfxCallMode::RECORD;
            if (comphelper::LibreOfficeKit::isActive())
            {
                // Make sure the LOK case doesn't dispatch async events while switching views, that
                // would lead to a loop, see SfxHintPoster::DoEvent_Impl().
                nCall |= SfxCallMode::SYNCHRON;
            }
            else
            {
                nCall |= SfxCallMode::ASYNCHRON;
            }
            GetDispatcher()->ExecuteList(
                SID_NAVIGATOR_INIT,
                nCall,
                { &aItem });
        }

        SfxViewShell* pViewShell = GetViewShell();
        assert(pViewShell!=nullptr);
        SfxBindings& rBindings = pViewShell->GetViewFrame().GetBindings();
        rBindings.Invalidate( SID_3D_STATE, true );

        rtl::Reference< SlideShow > xSlideShow( SlideShow::GetSlideShow( GetViewShellBase() ) );
        if (xSlideShow.is() && xSlideShow->isRunning()) //IASS
        {
            bool bSuccess = xSlideShow->activate(GetViewShellBase());
            assert(bSuccess && "can only return false with a PresentationViewShell"); (void)bSuccess;
        }

        if(HasCurrentFunction())
            GetCurrentFunction()->Activate();

        if(!GetDocSh()->IsUIActive())
            UpdatePreview( GetActualPage() );
    }

    ReadFrameViewData( mpFrameView );

    if (IsMainViewShell())
        GetDocSh()->Connect(this);
}

void ViewShell::UIActivating( SfxInPlaceClient*  )
{
    OSL_ASSERT (GetViewShell()!=nullptr);
    GetViewShellBase().GetToolBarManager()->ToolBarsDestroyed();
}

void ViewShell::UIDeactivated( SfxInPlaceClient*  )
{
    OSL_ASSERT (GetViewShell()!=nullptr);
    GetViewShellBase().GetToolBarManager()->ToolBarsDestroyed();
    if ( GetDrawView() )
        GetViewShellBase().GetToolBarManager()->SelectionHasChanged(*this, *GetDrawView());
}

void ViewShell::Deactivate(bool bIsMDIActivate)
{
    // remove view from a still active drag'n'drop session
    SdTransferable* pDragTransferable = SdModule::get()->pTransferDrag;

    if (IsMainViewShell())
        GetDocSh()->Disconnect(this);

    if( pDragTransferable )
        pDragTransferable->SetView( nullptr );

    OSL_ASSERT (GetViewShell()!=nullptr);

    // remember view attributes of FrameView
    WriteFrameViewData();

    if (bIsMDIActivate)
    {
        rtl::Reference< SlideShow > xSlideShow( SlideShow::GetSlideShow( GetViewShellBase() ) );
        if(xSlideShow.is() && xSlideShow->isRunning() ) //IASS
            xSlideShow->deactivate();

        if(HasCurrentFunction())
            GetCurrentFunction()->Deactivate();
    }

    if (mpHorizontalRuler)
        mpHorizontalRuler->SetActive(false);
    if (mpVerticalRuler)
        mpVerticalRuler->SetActive(false);

    SfxShell::Deactivate(bIsMDIActivate);
}

void ViewShell::BroadcastContextForActivation(const bool bIsActivated)
{
    auto getFrameworkResourceIdForShell
        = [&]() -> rtl::Reference<framework::ResourceId> const
    {
        DrawController* pDrawController = GetViewShellBase().GetDrawController();
        if (!pDrawController)
            return {};

        rtl::Reference<sd::framework::ConfigurationController> xConfigurationController
            = pDrawController->getConfigurationController();
        if (!xConfigurationController.is())
            return {};

        rtl::Reference<framework::Configuration> xConfiguration
            = xConfigurationController->getCurrentConfiguration();
        if (!xConfiguration.is())
            return {};

        auto aResIdsIndirect
            = xConfiguration->getResources({}, u"", drawing::framework::AnchorBindingMode_INDIRECT);

        for (const rtl::Reference<framework::ResourceId>& rResId : aResIdsIndirect)
        {
            auto pFrameworkHelper = framework::FrameworkHelper::Instance(GetViewShellBase());

            rtl::Reference<sd::framework::AbstractView> xView;
            if (rResId->getResourceURL().match(framework::FrameworkHelper::msViewURLPrefix))
            {
                xView = dynamic_cast<sd::framework::AbstractView*>(xConfigurationController->getResource(rResId).get());

                if (xView.is())
                {
                    if (auto pViewShellWrapper = dynamic_cast<framework::ViewShellWrapper*>(xView.get()))
                    {
                        if (pViewShellWrapper->GetViewShell().get() == this)
                        {
                            return rResId;
                        }
                    }
                }
            }
        }
        return {};
    };

    if (bIsActivated)
    {
        GetViewShellBase().GetEventMultiplexer()->MultiplexEvent(
            EventMultiplexerEventId::FocusShifted, nullptr, getFrameworkResourceIdForShell());
    }

    if (GetDispatcher())
        SfxShell::BroadcastContextForActivation(bIsActivated);
}

void ViewShell::Shutdown()
{
    Exit ();
}

// IASS: Check if commands should be used for SlideShow
// This is the case when IASS is on, SlideShow is active
// and the SlideShow Window has the focus
bool ViewShell::useInputForSlideShow() const
{
    rtl::Reference< SlideShow > xSlideShow(SlideShow::GetSlideShow(GetViewShellBase()));

    if (!xSlideShow.is())
        // no SlideShow, do not use
        return false;

    if (!xSlideShow->isRunning())
        // SlideShow not running, do not use
        return false;

    if(!xSlideShow->IsInteractiveSlideshow())
        // if IASS is deactivated, do what was done before when
        // SlideSHow is running: use for SlideShow
        return true;

    // else, check if SlideShow Window has the focus
    OutputDevice* pShOut(xSlideShow->getShowWindow());
    vcl::Window* pShWin(pShOut ? pShOut->GetOwnerWindow() : nullptr);

    // return true if we got the SlideShow Window and it has the focus
    return nullptr != pShWin && pShWin->HasFocus();
}

bool ViewShell::KeyInput(const KeyEvent& rKEvt, ::sd::Window* pWin)
{
    bool bReturn(false);

    if(pWin)
        SetActiveWindow(pWin);

    // give key input first to SfxViewShell to give CTRL+Key
    // (e.g. CTRL+SHIFT+'+', to front) priority.
    OSL_ASSERT(GetViewShell() != nullptr);
    bReturn = GetViewShell()->KeyInput(rKEvt);

    if (sd::View* pView = GetView())
    {
        const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
        const size_t OriCount = rMarkList.GetMarkCount();
        if(!bReturn)
        {
            if(useInputForSlideShow()) //IASS
            {
                // use for SlideShow
                rtl::Reference< SlideShow > xSlideShow( SlideShow::GetSlideShow( GetViewShellBase() ) );
                bReturn = xSlideShow->keyInput(rKEvt);
            }
            else
            {
                bool bConsumed = false;
                bConsumed = pView->getSmartTags().KeyInput(rKEvt);

                if( !bConsumed )
                {
                    rtl::Reference< sdr::SelectionController > xSelectionController( pView->getSelectionController() );
                    if( !xSelectionController.is() || !xSelectionController->onKeyInput( rKEvt, pWin ) )
                    {
                        if(HasCurrentFunction())
                            bReturn = GetCurrentFunction()->KeyInput(rKEvt);
                    }
                    else
                    {
                        bReturn = true;
                        if (HasCurrentFunction())
                        {
                            FuText* pTextFunction = dynamic_cast<FuText*>(GetCurrentFunction().get());
                            if(pTextFunction != nullptr)
                                pTextFunction->InvalidateBindings();
                        }
                    }
                }
            }
        }
        const size_t EndCount = rMarkList.GetMarkCount();
        // Here, oriCount or endCount must have one value=0, another value > 0, then to switch focus between Document and shape objects
        if(bReturn &&  (OriCount + EndCount > 0) && (OriCount * EndCount == 0))
            SwitchActiveViewFireFocus();
    }

    if(!bReturn && GetActiveWindow())
    {
        vcl::KeyCode aKeyCode = rKEvt.GetKeyCode();

        if (aKeyCode.IsMod1() && aKeyCode.IsShift()
            && aKeyCode.GetCode() == KEY_R)
        {
            InvalidateWindows();
            bReturn = true;
        }
    }

    return bReturn;
}

void ViewShell::MouseButtonDown(const MouseEvent& rMEvt, ::sd::Window* pWin)
{
    // We have to lock tool bar updates while the mouse button is pressed in
    // order to prevent the shape under the mouse to be moved (this happens
    // when the number of docked tool bars changes as result of a changed
    // selection;  this changes the window size and thus the mouse position
    // in model coordinates: with respect to model coordinates the mouse
    // moves.)
    OSL_ASSERT(mpImpl->mpUpdateLockForMouse.expired());
    mpImpl->mpUpdateLockForMouse = ViewShell::Implementation::ToolBarManagerLock::Create(
        GetViewShellBase().GetToolBarManager());

    if ( pWin && !pWin->HasFocus() )
    {
        pWin->GrabFocus();
        SetActiveWindow(pWin);
    }

    ::sd::View* pView = GetView();
    if (!pView)
        return;

    // insert MouseEvent into E3dView
    pView->SetMouseEvent(rMEvt);

    bool bConsumed = false;
    bConsumed = pView->getSmartTags().MouseButtonDown( rMEvt );

    if( bConsumed )
        return;

    rtl::Reference< sdr::SelectionController > xSelectionController( pView->getSelectionController() );
    if( !xSelectionController.is() || !xSelectionController->onMouseButtonDown( rMEvt, pWin ) )
    {
        if(HasCurrentFunction())
            GetCurrentFunction()->MouseButtonDown(rMEvt);
    }
    else
    {
        if (HasCurrentFunction())
        {
            FuText* pTextFunction = dynamic_cast<FuText*>(GetCurrentFunction().get());
            if (pTextFunction != nullptr)
                pTextFunction->InvalidateBindings();
        }
    }
}

void ViewShell::SetCursorMm100Position(const Point& rPosition, bool bPoint, bool bClearMark)
{
    if (SdrView* pSdrView = GetView())
    {
        rtl::Reference<sdr::SelectionController> xSelectionController(GetView()->getSelectionController());
        if (!xSelectionController.is() || !xSelectionController->setCursorLogicPosition(rPosition, bPoint))
        {
            if (pSdrView->GetTextEditObject())
            {
                EditView& rEditView = pSdrView->GetTextEditOutlinerView()->GetEditView();
                rEditView.SetCursorLogicPosition(rPosition, bPoint, bClearMark);
            }
        }
    }
}

uno::Reference<datatransfer::XTransferable> ViewShell::GetSelectionTransferable() const
{
    SdrView* pSdrView = GetView();
    if (!pSdrView)
        return uno::Reference<datatransfer::XTransferable>();

    if (!pSdrView->GetTextEditObject())
        return uno::Reference<datatransfer::XTransferable>();

    EditView& rEditView = pSdrView->GetTextEditOutlinerView()->GetEditView();
    return rEditView.getEditEngine().CreateTransferable(rEditView.GetSelection());
}

void ViewShell::SetGraphicMm100Position(bool bStart, const Point& rPosition)
{
    if (bStart)
    {
        MouseEvent aClickEvent(rPosition, 1, MouseEventModifiers::SIMPLECLICK, MOUSE_LEFT);
        MouseButtonDown(aClickEvent, mpActiveWindow);
        MouseEvent aMoveEvent(Point(rPosition.getX(), rPosition.getY()), 0, MouseEventModifiers::SIMPLEMOVE, MOUSE_LEFT);
        MouseMove(aMoveEvent, mpActiveWindow);
    }
    else
    {
        MouseEvent aMoveEvent(Point(rPosition.getX(), rPosition.getY()), 0, MouseEventModifiers::SIMPLEMOVE, MOUSE_LEFT);
        MouseMove(aMoveEvent, mpActiveWindow);
        MouseEvent aClickEvent(rPosition, 1, MouseEventModifiers::SIMPLECLICK, MOUSE_LEFT);
        MouseButtonUp(aClickEvent, mpActiveWindow);
    }
}

void ViewShell::MouseMove(const MouseEvent& rMEvt, ::sd::Window* pWin)
{
    if (rMEvt.IsLeaveWindow())
    {
        if ( ! mpImpl->mpUpdateLockForMouse.expired())
        {
            std::shared_ptr<ViewShell::Implementation::ToolBarManagerLock> pLock(
                mpImpl->mpUpdateLockForMouse);
            if (pLock != nullptr)
                pLock->Release();
        }
    }

    if ( pWin )
    {
        SetActiveWindow(pWin);
    }

    // insert MouseEvent into E3dView
    if (GetView() != nullptr)
        GetView()->SetMouseEvent(rMEvt);

    if(HasCurrentFunction())
    {
        rtl::Reference< sdr::SelectionController > xSelectionController( GetView()->getSelectionController() );
        if( !xSelectionController.is() || !xSelectionController->onMouseMove( rMEvt, pWin ) )
        {
            if(HasCurrentFunction())
                GetCurrentFunction()->MouseMove(rMEvt);
        }
    }
}

void ViewShell::MouseButtonUp(const MouseEvent& rMEvt, ::sd::Window* pWin)
{
    if ( pWin )
        SetActiveWindow(pWin);

    // insert MouseEvent into E3dView
    if (GetView() != nullptr)
        GetView()->SetMouseEvent(rMEvt);

    if( HasCurrentFunction())
    {
        rtl::Reference< sdr::SelectionController > xSelectionController( GetView()->getSelectionController() );
        if( !xSelectionController.is() || !xSelectionController->onMouseButtonUp( rMEvt, pWin ) )
        {
            if(HasCurrentFunction())
                GetCurrentFunction()->MouseButtonUp(rMEvt);
        }
        else
        {
            if (HasCurrentFunction())
            {
                FuText* pTextFunction = dynamic_cast<FuText*>(GetCurrentFunction().get());
                if (pTextFunction != nullptr)
                    pTextFunction->InvalidateBindings();
            }
        }
    }

    if ( ! mpImpl->mpUpdateLockForMouse.expired())
    {
        std::shared_ptr<ViewShell::Implementation::ToolBarManagerLock> pLock(
            mpImpl->mpUpdateLockForMouse);
        if (pLock != nullptr)
            pLock->Release();
    }
}

void ViewShell::Command(const CommandEvent& rCEvt, ::sd::Window* pWin)
{
    bool bDone = HandleScrollCommand (rCEvt, pWin);

    if( bDone )
        return;

    if( rCEvt.GetCommand() == CommandEventId::InputLanguageChange )
    {
        //#i42732# update state of fontname if input language changes
        GetViewFrame()->GetBindings().Invalidate( SID_ATTR_CHAR_FONT );
        GetViewFrame()->GetBindings().Invalidate( SID_ATTR_CHAR_FONTHEIGHT );
    }
    else
    {
        bool bConsumed = false;
        if( GetView() )
            bConsumed = GetView()->getSmartTags().Command(rCEvt);

        if( !bConsumed && HasCurrentFunction())
            GetCurrentFunction()->Command(rCEvt);
    }
}

bool ViewShell::Notify(NotifyEvent const & rNEvt, ::sd::Window* pWin)
{
    // handle scroll commands when they arrived at child windows
    bool bRet = false;
    if( rNEvt.GetType() == NotifyEventType::COMMAND )
    {
        // note: dynamic_cast is not possible as GetData() returns a void*
        CommandEvent* pCmdEvent = static_cast< CommandEvent* >(rNEvt.GetData());
        bRet = HandleScrollCommand(*pCmdEvent, pWin);
    }
    return bRet;
}

bool ViewShell::HandleScrollCommand(const CommandEvent& rCEvt, ::sd::Window* pWin)
{
    bool bDone = false;

    switch( rCEvt.GetCommand() )
    {
        case CommandEventId::GestureSwipe:
            {
                if(useInputForSlideShow()) //IASS
                {
                    // use for SlideShow
                    rtl::Reference< SlideShow > xSlideShow( SlideShow::GetSlideShow( GetViewShellBase() ) );
                    const CommandGestureSwipeData* pSwipeData = rCEvt.GetGestureSwipeData();
                    bDone = xSlideShow->swipe(*pSwipeData);
                }
            }
            break;
        case CommandEventId::GestureLongPress:
            {
                if(useInputForSlideShow()) //IASS
                {
                    // use for SlideShow
                    rtl::Reference< SlideShow > xSlideShow( SlideShow::GetSlideShow( GetViewShellBase() ) );
                    const CommandGestureLongPressData* pLongPressData = rCEvt.GetLongPressData();
                    bDone = xSlideShow->longpress(*pLongPressData);
                }
            }
            break;

        case CommandEventId::Wheel:
            {
                Reference< XSlideShowController > xSlideShowController( SlideShow::GetSlideShowController(GetViewShellBase() ) );
                if( xSlideShowController.is() )
                {
                    if(useInputForSlideShow()) //IASS
                    {
                        // use for SlideShow
                        // We ignore zooming with control+mouse wheel.
                        const CommandWheelData* pData = rCEvt.GetWheelData();
                        if( pData && !pData->GetModifier() && ( pData->GetMode() == CommandWheelMode::SCROLL ) && !pData->IsHorz() )
                        {
                            ::tools::Long nDelta = pData->GetDelta();
                            if( nDelta > 0 )
                                xSlideShowController->gotoPreviousSlide();
                            else if( nDelta < 0 )
                                xSlideShowController->gotoNextEffect();
                        }
                        break;
                    }
                }
            }
            [[fallthrough]];
        case CommandEventId::StartAutoScroll:
        case CommandEventId::AutoScroll:
        {
            const CommandWheelData* pData = rCEvt.GetWheelData();

            if (pData != nullptr)
            {
                if (pData->IsMod1())
                {
                    if( !GetDocSh()->IsUIActive() )
                    {
                        const sal_uInt16  nOldZoom = GetActiveWindow()->GetZoom();
                        sal_uInt16        nNewZoom;
                        Point aOldMousePos = GetActiveWindow()->PixelToLogic(rCEvt.GetMousePosPixel());

                        if( pData->GetDelta() < 0 )
                            nNewZoom = std::max<sal_uInt16>( pWin->GetMinZoom(), basegfx::zoomtools::zoomOut( nOldZoom ));
                        else
                            nNewZoom = std::min<sal_uInt16>( pWin->GetMaxZoom(), basegfx::zoomtools::zoomIn( nOldZoom ));

                        SetZoom( nNewZoom );
                        // Keep mouse at same doc point before zoom
                        Point aNewMousePos = GetActiveWindow()->PixelToLogic(rCEvt.GetMousePosPixel());
                        SetWinViewPos(GetWinViewPos() - (aNewMousePos - aOldMousePos));

                        Invalidate( SID_ATTR_ZOOM );
                        Invalidate( SID_ATTR_ZOOMSLIDER );

                        bDone = true;
                    }
                }
                else
                {
                    if( mpContentWindow.get() == pWin )
                    {
                        double nScrollLines = pData->GetScrollLines();
                        if(IsPageFlipMode())
                            nScrollLines = COMMAND_WHEEL_PAGESCROLL;
                        CommandWheelData aWheelData( pData->GetDelta(),pData->GetNotchDelta(),
                            nScrollLines,pData->GetMode(),pData->GetModifier(),pData->IsHorz() );
                        CommandEvent aReWrite( rCEvt.GetMousePosPixel(),rCEvt.GetCommand(),
                            rCEvt.IsMouseEvent(),static_cast<const void *>(&aWheelData) );
                        bDone = pWin->HandleScrollCommand( aReWrite,
                            mpHorizontalScrollBar.get(),
                            mpVerticalScrollBar.get());
                    }
                }
            }
        }
        break;

        case CommandEventId::GesturePan:
        {
            bDone = pWin->HandleScrollCommand(rCEvt, mpHorizontalScrollBar.get(),
                                              mpVerticalScrollBar.get());
        }
        break;

        case CommandEventId::GestureZoom:
        {
            const CommandGestureZoomData* pData = rCEvt.GetGestureZoomData();

            if (pData->meEventType == GestureEventZoomType::Begin)
            {
                mfLastZoomScale = pData->mfScaleDelta;
                bDone = true;
                break;
            }

            if (pData->meEventType == GestureEventZoomType::Update)
            {
                double deltaBetweenEvents = (pData->mfScaleDelta - mfLastZoomScale) / mfLastZoomScale;
                mfLastZoomScale = pData->mfScaleDelta;

                if (!GetDocSh()->IsUIActive() && !useInputForSlideShow()) //IASS
                {
                    const ::tools::Long nOldZoom = GetActiveWindow()->GetZoom();
                    ::tools::Long nNewZoom;
                    Point aOldMousePos = GetActiveWindow()->PixelToLogic(rCEvt.GetMousePosPixel());

                    // Accumulate fractional zoom to avoid small zoom changes from being ignored
                    mfAccumulatedZoom += deltaBetweenEvents;
                    int nZoomChangePercent = mfAccumulatedZoom * 100;
                    mfAccumulatedZoom -= nZoomChangePercent / 100.0;

                    nNewZoom = nOldZoom + nZoomChangePercent;
                    nNewZoom = std::max<::tools::Long>(pWin->GetMinZoom(), nNewZoom);
                    nNewZoom = std::min<::tools::Long>(pWin->GetMaxZoom(), nNewZoom);

                    SetZoom(nNewZoom);

                    // Keep mouse at same doc point before zoom
                    Point aNewMousePos = GetActiveWindow()->PixelToLogic(rCEvt.GetMousePosPixel());
                    SetWinViewPos(GetWinViewPos() - (aNewMousePos - aOldMousePos));

                    Invalidate(SID_ATTR_ZOOM);
                    Invalidate(SID_ATTR_ZOOMSLIDER);
                }
            }

            bDone = true;
        }
        break;

        default:
        break;
    }

    return bDone;
}

void ViewShell::SetupRulers()
{
    if(!mbHasRulers || !mpContentWindow )
        return;

    if( SlideShow::IsRunning(GetViewShellBase()) && !SlideShow::IsInteractiveSlideshow(&GetViewShellBase())) // IASS
        return;

    ::tools::Long nHRulerOfs = 0;

    if ( !mpVerticalRuler )
    {
        mpVerticalRuler.reset(CreateVRuler(GetActiveWindow()));
        if ( mpVerticalRuler )
        {
            nHRulerOfs = mpVerticalRuler->GetSizePixel().Width();
            mpVerticalRuler->SetActive();
            mpVerticalRuler->Show();
        }
    }
    if ( !mpHorizontalRuler )
    {
        mpHorizontalRuler.reset(CreateHRuler(GetActiveWindow()));
        if ( mpHorizontalRuler )
        {
            mpHorizontalRuler->SetWinPos(nHRulerOfs);
            mpHorizontalRuler->SetActive();
            mpHorizontalRuler->Show();
        }
    }
}

const SvxNumBulletItem* ViewShell::GetNumBulletItem(SfxItemSet& aNewAttr, TypedWhichId<SvxNumBulletItem>& nNumItemId)
{
    const SvxNumBulletItem* pTmpItem = aNewAttr.GetItemIfSet(nNumItemId, false);
    if(pTmpItem)
        return pTmpItem;

    nNumItemId = aNewAttr.GetPool()->GetWhichIDFromSlotID(SID_ATTR_NUMBERING_RULE);
    pTmpItem = aNewAttr.GetItemIfSet(nNumItemId, false);
    if(pTmpItem)
        return pTmpItem;

    bool bOutliner = false;
    bool bTitle = false;

    if( mpView )
    {
        const SdrMarkList& rMarkList = mpView->GetMarkedObjectList();
        const size_t nCount = rMarkList.GetMarkCount();

        for(size_t nNum = 0; nNum < nCount; ++nNum)
        {
            SdrObject* pObj = rMarkList.GetMark(nNum)->GetMarkedSdrObj();
            if( pObj->GetObjInventor() == SdrInventor::Default )
            {
                switch(pObj->GetObjIdentifier())
                {
                case SdrObjKind::TitleText:
                    bTitle = true;
                    break;
                case SdrObjKind::OutlineText:
                    bOutliner = true;
                    break;
                default:
                    break;
                }
            }
        }
    }

    const SvxNumBulletItem *pItem = nullptr;
    if(bOutliner)
    {
        SfxStyleSheetBasePool* pSSPool = mpView->GetDocSh()->GetStyleSheetPool();
        SfxStyleSheetBase* pFirstStyleSheet = pSSPool->Find( STR_LAYOUT_OUTLINE + " 1", SfxStyleFamily::Pseudo);
        if( pFirstStyleSheet )
            pItem = pFirstStyleSheet->GetItemSet().GetItemIfSet(EE_PARA_NUMBULLET, false);
    }

    if( pItem == nullptr )
        pItem = aNewAttr.GetPool()->GetSecondaryPool()->GetUserDefaultItem(EE_PARA_NUMBULLET);

    aNewAttr.Put(pItem->CloneSetWhich(EE_PARA_NUMBULLET));

    const SvxNumBulletItem* pBulletItem = nullptr;
    if(bTitle && aNewAttr.GetItemState(EE_PARA_NUMBULLET, true, &pBulletItem) == SfxItemState::SET )
    {
        const SvxNumRule& rRule = pBulletItem->GetNumRule();
        SvxNumRule aNewRule( rRule );
        aNewRule.SetFeatureFlag( SvxNumRuleFlags::NO_NUMBERS );

        SvxNumBulletItem aNewItem( std::move(aNewRule), EE_PARA_NUMBULLET );
        aNewAttr.Put(aNewItem);
    }

    pTmpItem = aNewAttr.GetItemIfSet(nNumItemId, false);

    return pTmpItem;
}

void ViewShell::Resize()
{
    SetupRulers ();

    if (mpParentWindow == nullptr)
        return;

    // Make sure that the new size is not degenerate.
    const Size aSize (mpParentWindow->GetSizePixel());
    if (aSize.IsEmpty())
        return;

    // Remember the new position and size.
    maViewPos = Point(0,0);
    maViewSize = aSize;

    // Rearrange the UI elements to take care of the new position and size.
    ArrangeGUIElements ();
    // end of included AdjustPosSizePixel.

    ::sd::View* pView = GetView();

    if (pView)
        pView->VisAreaChanged(GetActiveWindow()->GetOutDev());
}

SvBorder ViewShell::GetBorder()
{
    SvBorder aBorder;

    // Horizontal scrollbar.
    if (mpHorizontalScrollBar
        && mpHorizontalScrollBar->IsVisible())
    {
        aBorder.Bottom() = maScrBarWH.Height();
    }

    // Vertical scrollbar.
    if (mpVerticalScrollBar
        && mpVerticalScrollBar->IsVisible())
    {
        aBorder.Right() = maScrBarWH.Width();
    }

    // Place horizontal ruler below tab bar.
    if (mbHasRulers && mpContentWindow)
    {
        SetupRulers();
        if (mpHorizontalRuler)
            aBorder.Top() = mpHorizontalRuler->GetSizePixel().Height();
        if (mpVerticalRuler)
            aBorder.Left() = mpVerticalRuler->GetSizePixel().Width();
    }

    return aBorder;
}

void ViewShell::ArrangeGUIElements()
{
    if (mpImpl->mbArrangeActive)
        return;
    if (maViewSize.IsEmpty())
        return;
    mpImpl->mbArrangeActive = true;

    // Calculate border for in-place editing.
    ::tools::Long nLeft = maViewPos.X();
    ::tools::Long nTop  = maViewPos.Y();
    ::tools::Long nRight = maViewPos.X() + maViewSize.Width();
    ::tools::Long nBottom = maViewPos.Y() + maViewSize.Height();

    // Horizontal scrollbar.
    if (mpHorizontalScrollBar
        && mpHorizontalScrollBar->IsVisible())
    {
        nBottom -= maScrBarWH.Height();
        if (mpLayerTabBar && mpLayerTabBar->IsVisible())
            nBottom -= mpLayerTabBar->GetSizePixel().Height();
        mpHorizontalScrollBar->SetPosSizePixel (
            Point(nLeft, nBottom),
            Size(nRight - nLeft - maScrBarWH.Width(), maScrBarWH.Height()));
    }

    // Vertical scrollbar.
    if (mpVerticalScrollBar
        && mpVerticalScrollBar->IsVisible())
    {
        nRight -= maScrBarWH.Width();
        mpVerticalScrollBar->SetPosSizePixel (
            Point(nRight,nTop),
            Size (maScrBarWH.Width(), nBottom-nTop));
    }

    // Place horizontal ruler below tab bar.
    if (mbHasRulers && mpContentWindow)
    {
        if (mpHorizontalRuler)
        {
            Size aRulerSize = mpHorizontalRuler->GetSizePixel();
            aRulerSize.setWidth( nRight - nLeft );
            mpHorizontalRuler->SetPosSizePixel (
                Point(nLeft,nTop), aRulerSize);
            if (mpVerticalRuler)
                mpHorizontalRuler->SetBorderPos(
                    mpVerticalRuler->GetSizePixel().Width()-1);
            nTop += aRulerSize.Height();
        }
        if (mpVerticalRuler)
        {
            Size aRulerSize = mpVerticalRuler->GetSizePixel();
            aRulerSize.setHeight( nBottom  - nTop );
            mpVerticalRuler->SetPosSizePixel (
                Point (nLeft,nTop), aRulerSize);
            nLeft += aRulerSize.Width();
        }
    }

    rtl::Reference< SlideShow > xSlideShow( SlideShow::GetSlideShow( GetViewShellBase() ) );

    // The size of the window of the center pane is set differently from
    // that of the windows in the docking windows.
    bool bSlideShowActive = (xSlideShow.is() && xSlideShow->isRunning()) //IASS
        && !xSlideShow->isFullScreen() && xSlideShow->getAnimationMode() == ANIMATIONMODE_SHOW;
    if ( !bSlideShowActive)
    {
        OSL_ASSERT (GetViewShell()!=nullptr);

        if (mpContentWindow)
            mpContentWindow->SetPosSizePixel(
                Point(nLeft,nTop),
                Size(nRight-nLeft,nBottom-nTop));
    }

    // Windows in the center and rulers at the left and top side.
    maAllWindowRectangle = ::tools::Rectangle(
        maViewPos,
        Size(maViewSize.Width()-maScrBarWH.Width(),
            maViewSize.Height()-maScrBarWH.Height()));

    if (mpContentWindow)
        mpContentWindow->UpdateMapOrigin();

    UpdateScrollBars();

    mpImpl->mbArrangeActive = false;
}

void ViewShell::SetUIUnit(FieldUnit eUnit)
{
    // Set unit at horizontal and vertical rulers.
    if (mpHorizontalRuler)
        mpHorizontalRuler->SetUnit(eUnit);

    if (mpVerticalRuler)
        mpVerticalRuler->SetUnit(eUnit);
}

/**
 * set DefTab at horizontal rulers
 */
void ViewShell::SetDefTabHRuler( sal_uInt16 nDefTab )
{
    if (mpHorizontalRuler)
        mpHorizontalRuler->SetDefTabDist( nDefTab );
}

/** Tell the FmFormShell that the view shell is closing.  Give it the
    opportunity to prevent that.
*/
bool ViewShell::PrepareClose (bool bUI)
{
    bool bResult = true;

    FmFormShell* pFormShell = GetViewShellBase().GetFormShellManager()->GetFormShell();
    if (pFormShell != nullptr)
        bResult = pFormShell->PrepareClose (bUI);

    return bResult;
}

void ViewShell::UpdatePreview (SdPage*)
{
    // Do nothing.  After the actual preview has been removed,
    // OutlineViewShell::UpdatePreview() is the place where something
    // useful is still done.
}

SfxUndoManager* ViewShell::ImpGetUndoManager() const
{
    const ViewShell* pMainViewShell = GetViewShellBase().GetMainViewShell().get();

    if( pMainViewShell == nullptr )
        pMainViewShell = this;

    ::sd::View* pView = pMainViewShell->GetView();

    // check for text edit our outline view
    if( pView )
    {
        if( pMainViewShell->GetShellType() == ViewShell::ST_OUTLINE )
        {
            OutlineView* pOlView = dynamic_cast< OutlineView* >( pView );
            if( pOlView )
            {
                ::Outliner& rOutl = pOlView->GetOutliner();
                return &rOutl.GetUndoManager();
            }
        }
        else if( pView->IsTextEdit() )
        {
            SdrOutliner* pOL = pView->GetTextEditOutliner();
            if( pOL )
                return &pOL->GetUndoManager();
        }
    }

    if( GetDocSh() )
        return GetDocSh()->GetUndoManager();

    return nullptr;
}

void ViewShell::ImpGetUndoStrings(SfxItemSet &rSet) const
{
    SfxUndoManager* pUndoManager = ImpGetUndoManager();
    if(!pUndoManager)
        return;

    sal_uInt16 nCount(pUndoManager->GetUndoActionCount());
    if(nCount)
    {
        // prepare list
        std::vector<OUString> aStringList;
        aStringList.reserve(nCount);
        for (sal_uInt16 a = 0; a < nCount; ++a)
        {
            // generate one String in list per undo step
            aStringList.push_back( pUndoManager->GetUndoActionComment(a) );
        }

        // set item
        rSet.Put(SfxStringListItem(SID_GETUNDOSTRINGS, &aStringList));
    }
    else
    {
        rSet.DisableItem(SID_GETUNDOSTRINGS);
    }
}

void ViewShell::ImpGetRedoStrings(SfxItemSet &rSet) const
{
    SfxUndoManager* pUndoManager = ImpGetUndoManager();
    if(!pUndoManager)
        return;

    sal_uInt16 nCount(pUndoManager->GetRedoActionCount());
    if(nCount)
    {
        // prepare list
        ::std::vector< OUString > aStringList;
        aStringList.reserve(nCount);
        for(sal_uInt16 a = 0; a < nCount; a++)
            // generate one String in list per undo step
            aStringList.push_back( pUndoManager->GetRedoActionComment(a) );

        // set item
        rSet.Put(SfxStringListItem(SID_GETREDOSTRINGS, &aStringList));
    }
    else
    {
        rSet.DisableItem(SID_GETREDOSTRINGS);
    }
}

namespace {

class KeepSlideSorterInSyncWithPageChanges
{
    sd::slidesorter::view::SlideSorterView::DrawLock m_aDrawLock;
    sd::slidesorter::controller::SlideSorterController::ModelChangeLock m_aModelLock;
    sd::slidesorter::controller::PageSelector::UpdateLock m_aUpdateLock;
    sd::slidesorter::controller::SelectionObserver::Context m_aContext;

public:
    explicit KeepSlideSorterInSyncWithPageChanges(sd::slidesorter::SlideSorter const & rSlideSorter)
        : m_aDrawLock(rSlideSorter)
        , m_aModelLock(rSlideSorter.GetController())
        , m_aUpdateLock(rSlideSorter)
        , m_aContext(rSlideSorter)
    {
    }
};

}

void ViewShell::ImpSidUndo(SfxRequest& rReq)
{
    //The xWatcher keeps the SlideSorter selection in sync
    //with the page insertions/deletions that Undo may introduce
    std::unique_ptr<KeepSlideSorterInSyncWithPageChanges, o3tl::default_delete<KeepSlideSorterInSyncWithPageChanges>> xWatcher;
    slidesorter::SlideSorterViewShell* pSlideSorterViewShell
        = slidesorter::SlideSorterViewShell::GetSlideSorter(GetViewShellBase());
    if (pSlideSorterViewShell)
        xWatcher.reset(new KeepSlideSorterInSyncWithPageChanges(pSlideSorterViewShell->GetSlideSorter()));

    SfxUndoManager* pUndoManager = ImpGetUndoManager();
    sal_uInt16 nNumber(1);
    const SfxItemSet* pReqArgs = rReq.GetArgs();
    bool bRepair = false;

    if(pReqArgs)
    {
        const SfxUInt16Item* pUIntItem = static_cast<const SfxUInt16Item*>(&pReqArgs->Get(SID_UNDO));
        nNumber = pUIntItem->GetValue();

        // Repair mode: allow undo/redo of all undo actions, even if access would
        // be limited based on the view shell ID.
        if (const SfxBoolItem* pRepairItem = pReqArgs->GetItemIfSet(SID_REPAIRPACKAGE, false))
            bRepair = pRepairItem->GetValue();
    }

    if(nNumber && pUndoManager)
    {
        sal_uInt16 nCount(pUndoManager->GetUndoActionCount());
        if(nCount >= nNumber)
        {
            if (comphelper::LibreOfficeKit::isActive() && !bRepair)
            {
                // If another view created the first undo action, prevent redoing it from this view.
                const SfxUndoAction* pAction = pUndoManager->GetUndoAction();
                if (pAction->GetViewShellId() != GetViewShellBase().GetViewShellId())
                {
                    rReq.SetReturnValue(SfxUInt32Item(SID_UNDO, static_cast<sal_uInt32>(SID_REPAIRPACKAGE)));
                    return;
                }
            }

            try
            {
                // when UndoStack is cleared by ModifyPageUndoAction
                // the nCount may have changed, so test GetUndoActionCount()
                while (nNumber && pUndoManager->GetUndoActionCount())
                {
                    pUndoManager->Undo();
                    --nNumber;
                }
            }
            catch( const Exception& )
            {
                // no need to handle. By definition, the UndoManager handled this by clearing the
                // Undo/Redo stacks
            }
        }

        // refresh rulers, maybe UNDO was move of TAB marker in ruler
        if (mbHasRulers)
            Invalidate(SID_ATTR_TABSTOP);
    }

    // This one is corresponding to the default handling
    // of SID_UNDO in sfx2
    GetViewFrame()->GetBindings().InvalidateAll(false);

    rReq.Done();
}

void ViewShell::ImpSidRedo(SfxRequest& rReq)
{
    //The xWatcher keeps the SlideSorter selection in sync
    //with the page insertions/deletions that Undo may introduce
    std::unique_ptr<KeepSlideSorterInSyncWithPageChanges, o3tl::default_delete<KeepSlideSorterInSyncWithPageChanges>> xWatcher;
    slidesorter::SlideSorterViewShell* pSlideSorterViewShell
        = slidesorter::SlideSorterViewShell::GetSlideSorter(GetViewShellBase());
    if (pSlideSorterViewShell)
        xWatcher.reset(new KeepSlideSorterInSyncWithPageChanges(pSlideSorterViewShell->GetSlideSorter()));

    SfxUndoManager* pUndoManager = ImpGetUndoManager();
    sal_uInt16 nNumber(1);
    const SfxItemSet* pReqArgs = rReq.GetArgs();
    bool bRepair = false;

    if(pReqArgs)
    {
        const SfxUInt16Item* pUIntItem = static_cast<const SfxUInt16Item*>(&pReqArgs->Get(SID_REDO));
        nNumber = pUIntItem->GetValue();
        // Repair mode: allow undo/redo of all undo actions, even if access would
        // be limited based on the view shell ID.
        if (const SfxBoolItem* pRepairItem = pReqArgs->GetItemIfSet(SID_REPAIRPACKAGE, false))
            bRepair = pRepairItem->GetValue();
    }

    if(nNumber && pUndoManager)
    {
        sal_uInt16 nCount(pUndoManager->GetRedoActionCount());
        if(nCount >= nNumber)
        {
            if (comphelper::LibreOfficeKit::isActive() && !bRepair)
            {
                // If another view created the first undo action, prevent redoing it from this view.
                const SfxUndoAction* pAction = pUndoManager->GetRedoAction();
                if (pAction->GetViewShellId() != GetViewShellBase().GetViewShellId())
                {
                    rReq.SetReturnValue(SfxUInt32Item(SID_REDO, static_cast<sal_uInt32>(SID_REPAIRPACKAGE)));
                    return;
                }
            }

            try
            {
                // when UndoStack is cleared by ModifyPageRedoAction
                // the nCount may have changed, so test GetRedoActionCount()
                while (nNumber && pUndoManager->GetRedoActionCount())
                {
                    pUndoManager->Redo();
                    --nNumber;
                }
            }
            catch( const Exception& )
            {
                // no need to handle. By definition, the UndoManager handled this by clearing the
                // Undo/Redo stacks
            }
        }

        // refresh rulers, maybe REDO was move of TAB marker in ruler
        if (mbHasRulers)
        {
            Invalidate(SID_ATTR_TABSTOP);
        }
    }

    // This one is corresponding to the default handling
    // of SID_UNDO in sfx2
    GetViewFrame()->GetBindings().InvalidateAll(false);

    rReq.Done();
}

void ViewShell::ExecReq( SfxRequest& rReq )
{
    sal_uInt16 nSlot = rReq.GetSlot();
    switch( nSlot )
    {
        case SID_MAIL_SCROLLBODY_PAGEDOWN:
        {
            rtl::Reference<FuPoor> xFunc( GetCurrentFunction() );
            if( xFunc.is() )
                ScrollLines( 0, -1 );

            rReq.Done();
        }
        break;

        case SID_OUTPUT_QUALITY_COLOR:
        case SID_OUTPUT_QUALITY_GRAYSCALE:
        case SID_OUTPUT_QUALITY_BLACKWHITE:
        case SID_OUTPUT_QUALITY_CONTRAST:
        {
            DrawModeFlags nMode = OUTPUT_DRAWMODE_COLOR;

            switch( nSlot )
            {
                case SID_OUTPUT_QUALITY_COLOR: nMode = OUTPUT_DRAWMODE_COLOR; break;
                case SID_OUTPUT_QUALITY_GRAYSCALE: nMode = OUTPUT_DRAWMODE_GRAYSCALE; break;
                case SID_OUTPUT_QUALITY_BLACKWHITE: nMode = OUTPUT_DRAWMODE_BLACKWHITE; break;
                case SID_OUTPUT_QUALITY_CONTRAST: nMode = OUTPUT_DRAWMODE_CONTRAST; break;
            }

            GetActiveWindow()->GetOutDev()->SetDrawMode( nMode );
            mpFrameView->SetDrawMode( nMode );

            GetActiveWindow()->Invalidate();

            Invalidate();
            rReq.Done();
            break;
        }
    }
}

/** This default implementation returns only an empty reference.  See derived
    classes for more interesting examples.
*/
rtl::Reference<comphelper::OAccessible> ViewShell::CreateAccessibleDocumentView(::sd::Window*)
{
    OSL_FAIL("ViewShell::CreateAccessibleDocumentView should not be called!, perhaps Meyers, 3rd edition, Item 9:");

    return {};
}

::sd::WindowUpdater* ViewShell::GetWindowUpdater() const
{
    return mpWindowUpdater.get();
}

ViewShellBase& ViewShell::GetViewShellBase() const
{
    return *static_cast<ViewShellBase*>(GetViewShell());
}

ViewShell::ShellType ViewShell::GetShellType() const
{
    return meShellType;
}

DrawDocShell* ViewShell::GetDocSh() const
{
    return GetViewShellBase().GetDocShell();
}

SdDrawDocument* ViewShell::GetDoc() const
{
    return GetViewShellBase().GetDocument();
}

ErrCode ViewShell::DoVerb(sal_Int32 /*nVerb*/)
{
    return ERRCODE_NONE;
}

void ViewShell::SetCurrentFunction( const rtl::Reference<FuPoor>& xFunction)
{
    if( mxCurrentFunction.is() && (mxOldFunction != mxCurrentFunction) )
        mxCurrentFunction->Dispose();
    rtl::Reference<FuPoor> xDisposeAfterNewOne( mxCurrentFunction );
    mxCurrentFunction = xFunction;
}

void ViewShell::SetOldFunction(const rtl::Reference<FuPoor>& xFunction)
{
    if( mxOldFunction.is() && (xFunction != mxOldFunction) && (mxCurrentFunction != mxOldFunction) )
        mxOldFunction->Dispose();

    rtl::Reference<FuPoor> xDisposeAfterNewOne( mxOldFunction );
    mxOldFunction = xFunction;
}

/** this method deactivates the current function. If an old function is
    saved, this will become activated and current function.
*/
void ViewShell::Cancel()
{
    if(mxCurrentFunction.is() && (mxCurrentFunction != mxOldFunction ))
    {
        rtl::Reference<FuPoor> xTemp( mxCurrentFunction );
        mxCurrentFunction.clear();
        xTemp->Deactivate();
        xTemp->Dispose();
    }

    if(mxOldFunction.is())
    {
        mxCurrentFunction = mxOldFunction;
        mxCurrentFunction->Activate();
    }
}

void ViewShell::DeactivateCurrentFunction( bool bPermanent /* == false */ )
{
    if( mxCurrentFunction.is() )
    {
        if(bPermanent && (mxOldFunction == mxCurrentFunction))
            mxOldFunction.clear();

        mxCurrentFunction->Deactivate();
        if( mxCurrentFunction != mxOldFunction )
            mxCurrentFunction->Dispose();

        rtl::Reference<FuPoor> xDisposeAfterNewOne( mxCurrentFunction );
        mxCurrentFunction.clear();
    }
}

void ViewShell::DisposeFunctions()
{
    if(mxCurrentFunction.is())
    {
        rtl::Reference<FuPoor> xTemp( mxCurrentFunction );
        mxCurrentFunction.clear();
        xTemp->Deactivate();
        xTemp->Dispose();
    }

    if(mxOldFunction.is())
    {
        rtl::Reference<FuPoor> xDisposeAfterNewOne( mxOldFunction );
        mxOldFunction->Dispose();
        mxOldFunction.clear();
    }
}

bool ViewShell::IsMainViewShell() const
{
    return mpImpl->mbIsMainViewShell;
}

void ViewShell::SetIsMainViewShell (bool bIsMainViewShell)
{
    if (bIsMainViewShell != mpImpl->mbIsMainViewShell)
    {
        mpImpl->mbIsMainViewShell = bIsMainViewShell;
        if (bIsMainViewShell)
            GetDocSh()->Connect (this);
        else
            GetDocSh()->Disconnect (this);
    }
}

void ViewShell::PrePaint()
{
}

void ViewShell::Paint (const ::tools::Rectangle&, ::sd::Window* )
{
}

void ViewShell::ShowUIControls (bool bVisible)
{
    if (mbHasRulers)
    {
        if (mpHorizontalRuler)
            mpHorizontalRuler->Show( bVisible );

        if (mpVerticalRuler)
            mpVerticalRuler->Show( bVisible );
    }

    if (mpVerticalScrollBar)
        mpVerticalScrollBar->Show( bVisible );

    if (mpHorizontalScrollBar)
        mpHorizontalScrollBar->Show( bVisible );

    if (mpContentWindow)
        mpContentWindow->Show( bVisible );
}

bool ViewShell::RelocateToParentWindow (vcl::Window* pParentWindow)
{
    mpParentWindow = pParentWindow;

    mpParentWindow->SetBackground (Wallpaper());

    if (mpContentWindow)
        mpContentWindow->SetParent(pParentWindow);

    if (mpHorizontalScrollBar)
        mpHorizontalScrollBar->SetParent(mpParentWindow);
    if (mpVerticalScrollBar)
        mpVerticalScrollBar->SetParent(mpParentWindow);

    return true;
}

void ViewShell::SwitchViewFireFocus(const css::uno::Reference< css::accessibility::XAccessible >& xAcc )
{
    if (xAcc)
    {
        ::accessibility::AccessibleDocumentViewBase* pBase = static_cast< ::accessibility::AccessibleDocumentViewBase* >(xAcc.get());
        if (pBase)
            pBase->SwitchViewActivated();
    }
}
void ViewShell::SwitchActiveViewFireFocus()
{
    if (mpContentWindow)
    {
        SwitchViewFireFocus(mpContentWindow->GetAccessible(false));
    }
}
// move these two methods from DrawViewShell.
void ViewShell::fireSwitchCurrentPage(sal_Int32 pageIndex)
{
    GetViewShellBase().GetDrawController()->fireSwitchCurrentPage(pageIndex);
}
void ViewShell::NotifyAccUpdate( )
{
    GetViewShellBase().GetDrawController()->NotifyAccUpdate();
}

weld::Window* ViewShell::GetFrameWeld() const
{
    return mpActiveWindow ? mpActiveWindow->GetFrameWeld() : nullptr;
}

sd::Window* ViewShell::GetContentWindow() const
{
    return mpContentWindow.get();
}

} // end of namespace sd

//===== ViewShellObjectBarFactory =============================================

namespace {

ViewShellObjectBarFactory::ViewShellObjectBarFactory (
    ::sd::ViewShell& rViewShell)
    : mrViewShell (rViewShell)
{
}

SfxShell* ViewShellObjectBarFactory::CreateShell( ::sd::ShellId nId )
{
    SfxShell* pShell = nullptr;

    ::sd::View* pView = mrViewShell.GetView();
    switch (nId)
    {
        case ToolbarId::Bezier_Toolbox_Sd:
            pShell = new ::sd::BezierObjectBar(mrViewShell, pView);
            break;

        case ToolbarId::Draw_Text_Toolbox_Sd:
            pShell = new ::sd::TextObjectBar(
                mrViewShell, mrViewShell.GetDoc()->GetPool(), pView);
            break;

        case ToolbarId::Draw_Graf_Toolbox:
            pShell = new ::sd::GraphicObjectBar(mrViewShell, pView);
            break;

        case ToolbarId::Draw_Media_Toolbox:
            pShell = new ::sd::MediaObjectBar(mrViewShell, pView);
            break;

        case ToolbarId::Draw_Table_Toolbox:
            pShell = ::sd::ui::table::CreateTableObjectBar( mrViewShell, pView );
            break;

        case ToolbarId::Svx_Extrusion_Bar:
            pShell = new svx::ExtrusionBar(
                &mrViewShell.GetViewShellBase());
            break;

         case ToolbarId::Svx_Fontwork_Bar:
            pShell = new svx::FontworkBar(
                &mrViewShell.GetViewShellBase());
            break;

        default:
            pShell = nullptr;
            break;
    }

    return pShell;
}

void ViewShellObjectBarFactory::ReleaseShell (SfxShell* pShell)
{
    delete pShell;
}

} // end of anonymous namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
