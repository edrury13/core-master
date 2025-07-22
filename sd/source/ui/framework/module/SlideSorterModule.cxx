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

#include "SlideSorterModule.hxx"

#include <comphelper/lok.hxx>
#include <framework/FrameworkHelper.hxx>
#include <framework/ConfigurationController.hxx>
#include <framework/ConfigurationChangeEvent.hxx>
#include <o3tl/test_info.hxx>
#include <officecfg/Office/Impress.hxx>
#include <DrawController.hxx>
#include <ViewTabBar.hxx>
#include <com/sun/star/frame/XController.hpp>

#include <strings.hrc>
#include <sdresid.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing::framework;

using ::sd::framework::FrameworkHelper;

namespace sd::framework {

//===== SlideSorterModule ==================================================

SlideSorterModule::SlideSorterModule (
    const rtl::Reference<::sd::DrawController>& rxController,
    const OUString& rsLeftPaneURL)
    : mxResourceId(new ::sd::framework::ResourceId(FrameworkHelper::msSlideSorterURL, rsLeftPaneURL)),
      mxMainViewAnchorId(new ::sd::framework::ResourceId(FrameworkHelper::msCenterPaneURL)),
      mxViewTabBarId(new ::sd::framework::ResourceId(
          FrameworkHelper::msViewTabBarURL,
          FrameworkHelper::msCenterPaneURL)),
      mxControllerManager(rxController)
{
    if (mxControllerManager.is())
    {
        mxConfigurationController = mxControllerManager->getConfigurationController();

        if (mxConfigurationController.is())
        {
            mxConfigurationController->addEventListener(this);
            mxConfigurationController->addConfigurationChangeListener(
                this,
                ConfigurationChangeEventType::ResourceActivationRequest);
            mxConfigurationController->addConfigurationChangeListener(
                this,
                ConfigurationChangeEventType::ResourceDeactivationRequest);
        }
    }
    if (!mxConfigurationController.is())
        return;

    UpdateViewTabBar(nullptr);

    if (officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::ImpressView::get().value_or(true)
        && (!o3tl::IsRunningUnitTest() || !comphelper::LibreOfficeKit::isActive()))
        AddActiveMainView(FrameworkHelper::msImpressViewURL);
    if (officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::OutlineView::get().value_or(true))
        AddActiveMainView(FrameworkHelper::msOutlineViewURL);
    if (officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::NotesView::get().value_or(true))
        AddActiveMainView(FrameworkHelper::msNotesViewURL);
    if (officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::HandoutView::get().value_or(false))
        AddActiveMainView(FrameworkHelper::msHandoutViewURL);
    if (officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::SlideSorterView::get().value_or(false)
        && !comphelper::LibreOfficeKit::isActive())
        AddActiveMainView(FrameworkHelper::msSlideSorterURL);
    if (officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::DrawView::get().value_or(true))
        AddActiveMainView(FrameworkHelper::msDrawViewURL);

    mxConfigurationController->addConfigurationChangeListener(
        this,
        ConfigurationChangeEventType::ResourceActivation);
}

SlideSorterModule::~SlideSorterModule()
{
}

void SlideSorterModule::SaveResourceState()
{
    auto xChanges = comphelper::ConfigurationChanges::create();
    officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::ImpressView::set(IsResourceActive(FrameworkHelper::msImpressViewURL),xChanges);
    officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::OutlineView::set(IsResourceActive(FrameworkHelper::msOutlineViewURL),xChanges);
    officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::NotesView::set(IsResourceActive(FrameworkHelper::msNotesViewURL),xChanges);
    officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::HandoutView::set(IsResourceActive(FrameworkHelper::msHandoutViewURL),xChanges);
    officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::SlideSorterView::set(IsResourceActive(FrameworkHelper::msSlideSorterURL),xChanges);
    officecfg::Office::Impress::MultiPaneGUI::SlideSorterBar::Visible::DrawView::set(IsResourceActive(FrameworkHelper::msDrawViewURL),xChanges);
    xChanges->commit();
}

void SlideSorterModule::notifyConfigurationChange (
    const ConfigurationChangeEvent& rEvent)
{
    if (rEvent.Type == ConfigurationChangeEventType::ResourceActivation)
    {
        if (rEvent.ResourceId->compareTo(mxViewTabBarId) == 0)
        {
            // Update the view tab bar because the view tab bar has just
            // become active.
            UpdateViewTabBar(dynamic_cast<sd::ViewTabBar*>(rEvent.ResourceObject.get()));
        }
        else if (rEvent.ResourceId->getResourceTypePrefix() ==
                     FrameworkHelper::msViewURLPrefix
                 && rEvent.ResourceId->isBoundTo(
                        new ::sd::framework::ResourceId(FrameworkHelper::msCenterPaneURL),
                        AnchorBindingMode_DIRECT))
        {
            // Update the view tab bar because the view in the center pane
            // has changed.
            UpdateViewTabBar(nullptr);
        }
        return;
    }

    OSL_ASSERT(rEvent.ResourceId.is());
    switch (rEvent.Type)
    {
        case ConfigurationChangeEventType::ResourceActivationRequest:
            if (rEvent.ResourceId->isBoundToURL(
                FrameworkHelper::msCenterPaneURL,
                AnchorBindingMode_DIRECT))
            {
                // A resource directly bound to the center pane has been
                // requested.
                if (rEvent.ResourceId->getResourceTypePrefix() ==
                    FrameworkHelper::msViewURLPrefix)
                {
                    // The requested resource is a view.  Show or hide the
                    // resource managed by this ResourceManager accordingly.
                    HandleMainViewSwitch(
                        rEvent.ResourceId->getResourceURL(),
                        true);
                }
            }
            else if (rEvent.ResourceId->compareTo(mxResourceId) == 0)
            {
                // The resource managed by this ResourceManager has been
                // explicitly been requested (maybe by us).  Remember this
                // setting.
                HandleResourceRequest(true, rEvent.Configuration);
            }
            break;

        case ConfigurationChangeEventType::ResourceDeactivationRequest:
            if (rEvent.ResourceId->compareTo(mxMainViewAnchorId) == 0)
            {
                HandleMainViewSwitch(
                    OUString(),
                    false);
            }
            else if (rEvent.ResourceId->compareTo(mxResourceId) == 0)
            {
                // The resource managed by this ResourceManager has been
                // explicitly been requested to be hidden (maybe by us).
                // Remember this setting.
                HandleResourceRequest(false, rEvent.Configuration);
            }
            break;

        default: break;
    }
}

void SlideSorterModule::UpdateViewTabBar (const rtl::Reference<ViewTabBar>& rxTabBar)
{
    if ( ! mxControllerManager.is())
        return;

    rtl::Reference<ViewTabBar> xBar (rxTabBar);
    if ( ! xBar.is())
    {
        rtl::Reference<ConfigurationController> xCC (
            mxControllerManager->getConfigurationController());
        if (xCC.is())
            xBar = dynamic_cast<ViewTabBar*>(xCC->getResource(mxViewTabBarId).get());
    }

    if (!xBar.is())
        return;

    TabBarButton aButtonA;
    aButtonA.ResourceId = new ::sd::framework::ResourceId(
        FrameworkHelper::msSlideSorterURL,
        FrameworkHelper::msCenterPaneURL);
    aButtonA.ButtonLabel = SdResId(STR_SLIDE_SORTER_MODE);

    TabBarButton aButtonB;
    aButtonB.ResourceId = new ::sd::framework::ResourceId(
        FrameworkHelper::msHandoutViewURL,
        FrameworkHelper::msCenterPaneURL);

    if ( ! xBar->hasTabBarButton(aButtonA))
        xBar->addTabBarButtonAfter(aButtonA, aButtonB);
}

void SlideSorterModule::AddActiveMainView (
    const OUString& rsMainViewURL)
{
    maActiveMainViewContainer.insert(rsMainViewURL);
}

bool SlideSorterModule::IsResourceActive (
    const OUString& rsMainViewURL)
{
    return (maActiveMainViewContainer.find(rsMainViewURL) != maActiveMainViewContainer.end());
}

void SlideSorterModule::disposing(std::unique_lock<std::mutex>&)
{
    if (mxConfigurationController.is())
    {
        mxConfigurationController->removeEventListener(this);
        mxConfigurationController->removeConfigurationChangeListener(this);
        mxConfigurationController = nullptr;
    }
}

void SlideSorterModule::HandleMainViewSwitch (
    const OUString& rsViewURL,
    const bool bIsActivated)
{
    if (bIsActivated)
        msCurrentMainViewURL = rsViewURL;
    else
        msCurrentMainViewURL.clear();

    if (!mxConfigurationController.is())
        return;

    ConfigurationController::Lock aLock (mxConfigurationController);

    if (maActiveMainViewContainer.find(msCurrentMainViewURL)
           != maActiveMainViewContainer.end())
    {
        // Activate resource.
        mxConfigurationController->requestResourceActivation(
            mxResourceId->getAnchor(),
            ResourceActivationMode::ADD);
        mxConfigurationController->requestResourceActivation(
            mxResourceId,
            ResourceActivationMode::REPLACE);
    }
    else
    {
        mxConfigurationController->requestResourceDeactivation(mxResourceId);
    }
}

void SlideSorterModule::HandleResourceRequest(
    bool bActivation,
    const rtl::Reference<Configuration>& rxConfiguration)
{
    std::vector<rtl::Reference<ResourceId> > aCenterViews = rxConfiguration->getResources(
        new ::sd::framework::ResourceId(FrameworkHelper::msCenterPaneURL),
        FrameworkHelper::msViewURLPrefix,
        AnchorBindingMode_DIRECT);
    if (aCenterViews.size() == 1)
    {
        if (bActivation)
        {
            maActiveMainViewContainer.insert(aCenterViews[0]->getResourceURL());
        }
        else
        {
            maActiveMainViewContainer.erase(aCenterViews[0]->getResourceURL());
        }
    }
}

void SAL_CALL SlideSorterModule::disposing (
    const lang::EventObject& rEvent)
{
    if (mxConfigurationController.is()
        && rEvent.Source == cppu::getXWeak(mxConfigurationController.get()))
    {
        SaveResourceState();
        // Without the configuration controller this class can do nothing.
        mxConfigurationController = nullptr;
        dispose();
    }
}

} // end of namespace sd::framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
