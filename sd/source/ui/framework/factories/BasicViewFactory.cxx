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

#include <framework/factories/BasicViewFactory.hxx>

#include <framework/ConfigurationController.hxx>
#include <framework/ViewShellWrapper.hxx>
#include <framework/FrameworkHelper.hxx>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <framework/Pane.hxx>
#include <DrawController.hxx>
#include <ViewShellBase.hxx>
#include <ViewShellManager.hxx>
#include <DrawDocShell.hxx>
#include <DrawViewShell.hxx>
#include <GraphicViewShell.hxx>
#include <OutlineViewShell.hxx>
#include <NotesPanelViewShell.hxx>
#include <PresentationViewShell.hxx>
#include <SlideSorterViewShell.hxx>
#include <FrameView.hxx>
#include <Window.hxx>
#include <ResourceId.hxx>

#include <comphelper/servicehelper.hxx>
#include <sfx2/viewfrm.hxx>
#include <vcl/wrkwin.hxx>
#include <toolkit/helper/vclunohelper.hxx>


using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::drawing::framework;

using ::sd::framework::FrameworkHelper;

namespace sd::framework {

//===== ViewDescriptor ========================================================

class BasicViewFactory::ViewDescriptor
{
public:
    rtl::Reference<ViewShellWrapper> mxView;
    std::shared_ptr<sd::ViewShell> mpViewShell;
    rtl::Reference<ResourceId> mxViewId;
    static bool CompareView (const std::shared_ptr<ViewDescriptor>& rpDescriptor,
        const rtl::Reference<AbstractResource>& rxView)
    { return rpDescriptor->mxView.get() == rxView.get(); }
};

//===== ViewFactory ===========================================================

BasicViewFactory::BasicViewFactory (const rtl::Reference<::sd::DrawController>& rxController)
    : mpBase(nullptr),
      mpFrameView(nullptr),
      mpWindow(VclPtr<WorkWindow>::Create(nullptr,WB_STDWORK)),
      mxLocalPane(new Pane(rtl::Reference<ResourceId>(), mpWindow.get()))
{
    try
    {
        // Tunnel through the controller to obtain a ViewShellBase.
        mpBase = rxController->GetViewShellBase();

        // Register the factory for its supported views.
        mxConfigurationController = rxController->getConfigurationController();
        if ( ! mxConfigurationController.is())
            throw RuntimeException();
        mxConfigurationController->addResourceFactory(FrameworkHelper::msImpressViewURL, this);
        mxConfigurationController->addResourceFactory(FrameworkHelper::msDrawViewURL, this);
        mxConfigurationController->addResourceFactory(FrameworkHelper::msOutlineViewURL, this);
        mxConfigurationController->addResourceFactory(FrameworkHelper::msNotesViewURL, this);
        mxConfigurationController->addResourceFactory(FrameworkHelper::msHandoutViewURL, this);
        mxConfigurationController->addResourceFactory(FrameworkHelper::msPresentationViewURL, this);
        mxConfigurationController->addResourceFactory(FrameworkHelper::msSlideSorterURL, this);
        mxConfigurationController->addResourceFactory(FrameworkHelper::msNotesPanelViewURL, this);
    }
    catch (RuntimeException&)
    {
        mpBase = nullptr;
        if (mxConfigurationController.is())
            mxConfigurationController->removeResourceFactoryForReference(this);
        throw;
    }
}

BasicViewFactory::~BasicViewFactory()
{
}

void BasicViewFactory::disposing(std::unique_lock<std::mutex>&)
{
    // Disconnect from the frame view.
    if (mpFrameView != nullptr)
    {
        mpFrameView->Disconnect();
        mpFrameView = nullptr;
    }

    // Release the view cache.
    for (const auto& rxView : maViewCache)
    {
        ReleaseView(rxView, true);
    }

    // Release the view shell container.  At this point no one other than us
    // should hold references to the view shells (at the moment this is a
    // trivial requirement, because no one other than us holds a shared
    // pointer).
    //    ViewShellContainer::const_iterator iView;
    for (const auto& rxView : maViewShellContainer)
    {
        OSL_ASSERT(rxView->mpViewShell.use_count() == 1);
    }
    maViewShellContainer.clear();
}

rtl::Reference<AbstractResource> BasicViewFactory::createResource (
    const rtl::Reference<ResourceId>& rxViewId)
{
    const bool bIsCenterPane (
        rxViewId->isBoundToURL(FrameworkHelper::msCenterPaneURL, AnchorBindingMode_DIRECT));

    // Get the pane for the anchor URL.
    rtl::Reference<AbstractPane> xPane;
    if (mxConfigurationController.is())
        xPane = dynamic_cast<AbstractPane*>(mxConfigurationController->getResource(rxViewId->getAnchor()).get());

    // For main views use the frame view of the last main view.
    ::sd::FrameView* pFrameView = nullptr;
    if (xPane.is() && bIsCenterPane)
    {
        pFrameView = mpFrameView;
    }

    // Get Window pointer for XWindow of the pane.
    vcl::Window* pWindow = nullptr;
    if (xPane.is())
        pWindow = VCLUnoHelper::GetWindow(xPane->getWindow());

    if (!mpBase || !pWindow)
        return nullptr;

    // Try to get the view from the cache.
    std::shared_ptr<ViewDescriptor> pDescriptor (GetViewFromCache(rxViewId, xPane));

    // When the requested view is not in the cache then create a new view.
    if (pDescriptor == nullptr)
    {
        pDescriptor = CreateView(rxViewId, *pWindow, xPane, pFrameView, bIsCenterPane);
    }

    rtl::Reference<ViewShellWrapper> xView = pDescriptor->mxView;

    maViewShellContainer.push_back(pDescriptor);

    if (bIsCenterPane)
        ActivateCenterView(pDescriptor);
    else
        pWindow->Resize();

    return xView;
}

void BasicViewFactory::releaseResource (const rtl::Reference<AbstractResource>& rxView)
{
    if ( ! rxView.is())
        throw lang::IllegalArgumentException();

    if (!rxView.is() || !mpBase)
        return;

    ViewShellContainer::iterator iViewShell (
        ::std::find_if(
            maViewShellContainer.begin(),
            maViewShellContainer.end(),
            [&] (std::shared_ptr<ViewDescriptor> const& pVD) {
                return ViewDescriptor::CompareView(pVD, rxView);
            } ));
    if (iViewShell == maViewShellContainer.end())
    {
        throw lang::IllegalArgumentException();
    }

    std::shared_ptr<ViewShell> pViewShell ((*iViewShell)->mpViewShell);

    if ((*iViewShell)->mxViewId->isBoundToURL(
        FrameworkHelper::msCenterPaneURL, AnchorBindingMode_DIRECT))
    {
        // Obtain a pointer to and connect to the frame view of the
        // view.  The next view, that is created, will be
        // initialized with this frame view.
        if (mpFrameView == nullptr)
        {
            mpFrameView = pViewShell->GetFrameView();
            if (mpFrameView)
                mpFrameView->Connect();
        }

        // With the view in the center pane the sub controller is
        // released, too.
        mpBase->GetDrawController()->SetSubController(
            Reference<drawing::XDrawSubController>());

        SfxViewShell* pSfxViewShell = pViewShell->GetViewShell();
        if (pSfxViewShell != nullptr)
            pSfxViewShell->DisconnectAllClients();
    }

    ReleaseView(*iViewShell, false);

    maViewShellContainer.erase(iViewShell);
}

std::shared_ptr<BasicViewFactory::ViewDescriptor> BasicViewFactory::CreateView (
    const rtl::Reference<ResourceId>& rxViewId,
    vcl::Window& rWindow,
    const rtl::Reference<AbstractPane>& rxPane,
    FrameView* pFrameView,
    const bool bIsCenterPane)
{
    auto pDescriptor = std::make_shared<ViewDescriptor>();

    pDescriptor->mpViewShell = CreateViewShell(
        rxViewId,
        rWindow,
        pFrameView);
    pDescriptor->mxViewId = rxViewId;

    if (pDescriptor->mpViewShell != nullptr)
    {
        pDescriptor->mpViewShell->Init(bIsCenterPane);
        mpBase->GetViewShellManager()->ActivateViewShell(pDescriptor->mpViewShell.get());

        Reference<awt::XWindow> xWindow(rxPane->getWindow());
        rtl::Reference<ViewShellWrapper> wrapper(new ViewShellWrapper(
            pDescriptor->mpViewShell,
            rxViewId,
            xWindow));

        // register ViewShellWrapper on pane window
        if (xWindow.is())
        {
            xWindow->addWindowListener(wrapper);
            if (pDescriptor->mpViewShell != nullptr)
            {
                pDescriptor->mpViewShell->Resize();
            }
        }

        pDescriptor->mxView = wrapper.get();
    }

    return pDescriptor;
}

std::shared_ptr<ViewShell> BasicViewFactory::CreateViewShell (
    const rtl::Reference<ResourceId>& rxViewId,
    vcl::Window& rWindow,
    FrameView* pFrameView)
{
    std::shared_ptr<ViewShell> pViewShell;
    const OUString sViewURL (rxViewId->getResourceURL());
    if (sViewURL == FrameworkHelper::msImpressViewURL)
    {
        pViewShell =
            std::make_shared<DrawViewShell>(
                *mpBase,
                &rWindow,
                PageKind::Standard,
                pFrameView);
        pViewShell->GetContentWindow()->set_id(u"impress_win"_ustr);
    }
    else if (sViewURL == FrameworkHelper::msDrawViewURL)
    {
        pViewShell = std::shared_ptr<GraphicViewShell>(
                new GraphicViewShell(*mpBase, &rWindow, pFrameView),
                o3tl::default_delete<GraphicViewShell>());
        pViewShell->GetContentWindow()->set_id(u"draw_win"_ustr);
    }
    else if (sViewURL == FrameworkHelper::msOutlineViewURL)
    {
        pViewShell =
            std::make_shared<OutlineViewShell>(
                *mpBase,
                &rWindow,
                pFrameView);
        pViewShell->GetContentWindow()->set_id(u"outline_win"_ustr);
    }
    else if (sViewURL == FrameworkHelper::msNotesViewURL)
    {
        pViewShell =
            std::make_shared<DrawViewShell>(
                *mpBase,
                &rWindow,
                PageKind::Notes,
                pFrameView);
        pViewShell->GetContentWindow()->set_id(u"notes_win"_ustr);
    }
    else if (sViewURL == FrameworkHelper::msHandoutViewURL)
    {
        pViewShell =
            std::make_shared<DrawViewShell>(
                *mpBase,
                &rWindow,
                PageKind::Handout,
                pFrameView);
        pViewShell->GetContentWindow()->set_id(u"handout_win"_ustr);
    }
    else if (sViewURL == FrameworkHelper::msPresentationViewURL)
    {
        pViewShell =
            std::make_shared<PresentationViewShell>(
                *mpBase,
                &rWindow,
                pFrameView);
        pViewShell->GetContentWindow()->set_id(u"presentation_win"_ustr);
    }
    else if (sViewURL == FrameworkHelper::msSlideSorterURL)
    {
        pViewShell = ::sd::slidesorter::SlideSorterViewShell::Create (
            *mpBase,
            &rWindow,
            pFrameView);
        pViewShell->GetContentWindow()->set_id(u"slidesorter"_ustr);
    }
    else if (sViewURL == FrameworkHelper::msNotesPanelViewURL)
    {
        pViewShell = std::make_shared<NotesPanelViewShell>(*mpBase, &rWindow, pFrameView);
        pViewShell->GetContentWindow()->set_id(u"notes_panel_win"_ustr);
    }

    return pViewShell;
}

void BasicViewFactory::ReleaseView (
    const std::shared_ptr<ViewDescriptor>& rpDescriptor,
    bool bDoNotCache)
{
    bool bIsCacheable (!bDoNotCache && IsCacheable(rpDescriptor));

    if (bIsCacheable)
    {
        if (rpDescriptor->mxView)
        {
            if (mxLocalPane.is())
                if (rpDescriptor->mxView->relocateToAnchor(mxLocalPane))
                    maViewCache.push_back(rpDescriptor);
                else
                    bIsCacheable = false;
            else
                bIsCacheable = false;
        }
        else
        {
            bIsCacheable = false;
        }
    }

    if ( ! bIsCacheable)
    {
        // Shut down the current view shell.
        rpDescriptor->mpViewShell->Shutdown ();
        mpBase->GetDocShell()->Disconnect(rpDescriptor->mpViewShell.get());
        mpBase->GetViewShellManager()->DeactivateViewShell(rpDescriptor->mpViewShell.get());

        if (rpDescriptor->mxView)
            rpDescriptor->mxView->dispose();
    }
}

bool BasicViewFactory::IsCacheable (const std::shared_ptr<ViewDescriptor>& rpDescriptor)
{
    bool bIsCacheable (false);

    if (rpDescriptor->mxView)
    {
        static ::std::vector<rtl::Reference<ResourceId> > s_aCacheableResources = [&]()
        {
            ::std::vector<rtl::Reference<ResourceId> > tmp;
            FrameworkHelper::Instance(*mpBase);

            // The slide sorter and the task panel are cacheable and relocatable.
            tmp.push_back(new ::sd::framework::ResourceId(
                FrameworkHelper::msSlideSorterURL, FrameworkHelper::msLeftDrawPaneURL));
            tmp.push_back(new ::sd::framework::ResourceId(
                FrameworkHelper::msSlideSorterURL, FrameworkHelper::msLeftImpressPaneURL));
            return tmp;
        }();

        bIsCacheable = std::any_of(s_aCacheableResources.begin(), s_aCacheableResources.end(),
            [&rpDescriptor](const rtl::Reference<ResourceId>& rxId) { return rxId->compareTo(rpDescriptor->mxViewId) == 0; });
    }

    return bIsCacheable;
}

std::shared_ptr<BasicViewFactory::ViewDescriptor> BasicViewFactory::GetViewFromCache (
    const rtl::Reference<ResourceId>& rxViewId,
    const rtl::Reference<AbstractPane>& rxPane)
{
    std::shared_ptr<ViewDescriptor> pDescriptor;

    // Search for the requested view in the cache.
    ViewCache::iterator iEntry = std::find_if(maViewCache.begin(), maViewCache.end(),
        [&rxViewId](const ViewCache::value_type& rxEntry) { return rxEntry->mxViewId->compareTo(rxViewId) == 0; });
    if (iEntry != maViewCache.end())
    {
        pDescriptor = *iEntry;
        maViewCache.erase(iEntry);
    }

    // When the view has been found then relocate it to the given pane and
    // remove it from the cache.
    if (pDescriptor != nullptr)
    {
        bool bRelocationSuccessful (false);
        if (pDescriptor->mxView && rxPane.is())
        {
            if (pDescriptor->mxView->relocateToAnchor(rxPane))
                bRelocationSuccessful = true;
        }

        if ( ! bRelocationSuccessful)
        {
            ReleaseView(pDescriptor, true);
            pDescriptor.reset();
        }
    }

    return pDescriptor;
}

void BasicViewFactory::ActivateCenterView (
    const std::shared_ptr<ViewDescriptor>& rpDescriptor)
{
    mpBase->GetDocShell()->Connect(rpDescriptor->mpViewShell.get());

    // During the creation of the new sub-shell, resize requests were not
    // forwarded to it because it was not yet registered.  Therefore, we
    // have to request a resize now.
    rpDescriptor->mpViewShell->UIFeatureChanged();
    if (mpBase->GetDocShell()->IsInPlaceActive())
        mpBase->GetViewFrame().Resize(true);

    mpBase->GetDrawController()->SetSubController(
        rpDescriptor->mpViewShell->CreateSubController());
}

} // end of namespace sd::framework


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
