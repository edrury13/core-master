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

#include "CenterViewFocusModule.hxx"

#include <framework/ConfigurationController.hxx>
#include <framework/ConfigurationChangeEvent.hxx>
#include <framework/FrameworkHelper.hxx>
#include <framework/ViewShellWrapper.hxx>

#include <DrawController.hxx>
#include <ViewShellBase.hxx>
#include <ViewShellManager.hxx>
#include <comphelper/servicehelper.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing::framework;

using ::sd::framework::FrameworkHelper;

namespace sd::framework {

//===== CenterViewFocusModule ====================================================

CenterViewFocusModule::CenterViewFocusModule (rtl::Reference<sd::DrawController> const & rxController)
    : mbValid(false),
      mpBase(nullptr),
      mbNewViewCreated(false)
{
    if (rxController.is())
    {
        mxConfigurationController = rxController->getConfigurationController();

        // Tunnel through the controller to obtain a ViewShellBase.
        if (rxController != nullptr)
            mpBase = rxController->GetViewShellBase();

        // Check, if all required objects do exist.
        if (mxConfigurationController.is() && mpBase!=nullptr)
        {
            mbValid = true;
        }
    }

    if (mbValid)
    {
        mxConfigurationController->addConfigurationChangeListener(
            this,
            ConfigurationChangeEventType::ConfigurationUpdateEnd);
        mxConfigurationController->addConfigurationChangeListener(
            this,
            ConfigurationChangeEventType::ResourceActivation);
    }
}

CenterViewFocusModule::~CenterViewFocusModule()
{
}

void CenterViewFocusModule::disposing(std::unique_lock<std::mutex>&)
{
    if (mxConfigurationController.is())
        mxConfigurationController->removeConfigurationChangeListener(this);

    mbValid = false;
    mxConfigurationController = nullptr;
    mpBase = nullptr;
}

void CenterViewFocusModule::notifyConfigurationChange (
    const ConfigurationChangeEvent& rEvent)
{
    if (mbValid)
    {
        if (rEvent.Type == ConfigurationChangeEventType::ConfigurationUpdateEnd)
        {
            HandleNewView(rEvent.Configuration);
        }
        else if (rEvent.Type == ConfigurationChangeEventType::ResourceActivation)
        {
            if (rEvent.ResourceId->getResourceURL().match(FrameworkHelper::msViewURLPrefix))
                mbNewViewCreated = true;
        }
    }
}

void CenterViewFocusModule::HandleNewView (
    const rtl::Reference<Configuration>& rxConfiguration)
{
    if (!mbNewViewCreated)
        return;

    mbNewViewCreated = false;
    // Make the center pane the active one.  Tunnel through the
    // controller to obtain a ViewShell pointer.

    std::vector<rtl::Reference<ResourceId> > xViewIds (rxConfiguration->getResources(
        new ::sd::framework::ResourceId(FrameworkHelper::msCenterPaneURL),
        FrameworkHelper::msViewURLPrefix,
        AnchorBindingMode_DIRECT));
    rtl::Reference<AbstractView> xView;
    if (!xViewIds.empty())
        xView = dynamic_cast<AbstractView*>( mxConfigurationController->getResource(xViewIds[0]).get() );
    if (mpBase!=nullptr)
    {
        auto pViewShellWrapper = dynamic_cast<ViewShellWrapper*>(xView.get());
        if (pViewShellWrapper != nullptr)
        {
            std::shared_ptr<ViewShell> pViewShell = pViewShellWrapper->GetViewShell();
            if (pViewShell != nullptr)
                mpBase->GetViewShellManager()->MoveToTop(*pViewShell);
        }
    }
}

void SAL_CALL CenterViewFocusModule::disposing (
    const lang::EventObject& rEvent)
{
    if (mxConfigurationController.is())
        if (rEvent.Source == cppu::getXWeak(mxConfigurationController.get()))
        {
            mbValid = false;
            mxConfigurationController = nullptr;
            mpBase = nullptr;
        }
}

} // end of namespace sd::framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
