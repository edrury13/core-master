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

#include <memory>
#include <sal/config.h>

#include <framework/factories/BasicPaneFactory.hxx>
#include <framework/ConfigurationChangeEvent.hxx>

#include "ChildWindowPane.hxx"
#include "FrameWindowPane.hxx"
#include "FullScreenPane.hxx"

#include <comphelper/processfactory.hxx>
#include <comphelper/servicehelper.hxx>
#include <framework/FrameworkHelper.hxx>
#include <framework/ConfigurationController.hxx>
#include <PaneShells.hxx>
#include <ViewShellBase.hxx>
#include <PaneChildWindows.hxx>
#include <DrawController.hxx>
#include <ResourceId.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::drawing::framework;

using ::sd::framework::FrameworkHelper;

namespace {
    enum PaneId {
        CenterPaneId,
        FullScreenPaneId,
        LeftImpressPaneId,
        BottomImpressPaneId,
        LeftDrawPaneId
    };
}

namespace sd::framework {

/** Store URL, AbstractPane reference and (local) PaneId for every pane factory
    that is registered at the PaneController.
*/
class BasicPaneFactory::PaneDescriptor
{
public:
    OUString msPaneURL;
    rtl::Reference<AbstractResource> mxPane;
    PaneId mePaneId;
    /** The mbReleased flag is set when the pane has been released.  Some
        panes are just hidden and destroyed.  When the pane is reused this
        flag is reset.
    */
    bool mbIsReleased;

    bool CompareURL(std::u16string_view rsPaneURL) const { return msPaneURL == rsPaneURL; }
    bool ComparePane(const rtl::Reference<AbstractResource>& rxPane) const { return mxPane == rxPane; }
};

//===== PaneFactory ===========================================================

BasicPaneFactory::BasicPaneFactory(
    const rtl::Reference<::sd::DrawController>& rxController)
    : mxListener(new Listener(*this)), mpViewShellBase(nullptr)
{
    try
    {
        // Tunnel through the controller to obtain access to the ViewShellBase.
        mpViewShellBase = rxController->GetViewShellBase();

        rtl::Reference<sd::framework::ConfigurationController> xCC (rxController->getConfigurationController());
        mxConfigurationControllerWeak = xCC.get();

        // Add pane factories for the two left panes (one for Impress and one for
        // Draw) and the center pane.
        if (rxController.is() && xCC.is())
        {
            PaneDescriptor aDescriptor;
            aDescriptor.msPaneURL = FrameworkHelper::msCenterPaneURL;
            aDescriptor.mePaneId = CenterPaneId;
            aDescriptor.mbIsReleased = false;
            maPaneContainer.push_back(aDescriptor);
            xCC->addResourceFactory(aDescriptor.msPaneURL, this);

            aDescriptor.msPaneURL = FrameworkHelper::msFullScreenPaneURL;
            aDescriptor.mePaneId = FullScreenPaneId;
            maPaneContainer.push_back(aDescriptor);
            xCC->addResourceFactory(aDescriptor.msPaneURL, this);

            aDescriptor.msPaneURL = FrameworkHelper::msLeftImpressPaneURL;
            aDescriptor.mePaneId = LeftImpressPaneId;
            maPaneContainer.push_back(aDescriptor);
            xCC->addResourceFactory(aDescriptor.msPaneURL, this);

            aDescriptor.msPaneURL = FrameworkHelper::msBottomImpressPaneURL;
            aDescriptor.mePaneId = BottomImpressPaneId;
            maPaneContainer.push_back(aDescriptor);
            xCC->addResourceFactory(aDescriptor.msPaneURL, this);

            aDescriptor.msPaneURL = FrameworkHelper::msLeftDrawPaneURL;
            aDescriptor.mePaneId = LeftDrawPaneId;
            maPaneContainer.push_back(aDescriptor);
            xCC->addResourceFactory(aDescriptor.msPaneURL, this);
        }

        // Register as configuration change listener.
        if (xCC.is())
        {
            xCC->addConfigurationChangeListener(
                mxListener,
                ConfigurationChangeEventType::ConfigurationUpdateStart);
            xCC->addConfigurationChangeListener(
                mxListener,
                ConfigurationChangeEventType::ConfigurationUpdateEnd);
        }
    }
    catch (RuntimeException&)
    {
        rtl::Reference<ConfigurationController> xCC (mxConfigurationControllerWeak);
        if (xCC.is())
            xCC->removeResourceFactoryForReference(this);
    }
}

BasicPaneFactory::~BasicPaneFactory()
{
}

void BasicPaneFactory::disposing(std::unique_lock<std::mutex>&)
{
    rtl::Reference<ConfigurationController> xCC (mxConfigurationControllerWeak);
    if (xCC.is())
    {
        xCC->removeResourceFactoryForReference(this);
        xCC->removeConfigurationChangeListener(mxListener);
        mxConfigurationControllerWeak.clear();
    }

    for (const auto& rDescriptor : maPaneContainer)
    {
        if (rDescriptor.mbIsReleased)
        {
            if (rDescriptor.mxPane.is())
            {
                rDescriptor.mxPane->removeEventListener(mxListener);
                rDescriptor.mxPane->dispose();
            }
        }
    }
}

//===== AbstractPaneFactory ==========================================================

rtl::Reference<AbstractResource> BasicPaneFactory::createResource (
    const rtl::Reference<ResourceId>& rxPaneId)
{
    ThrowIfDisposed();

    rtl::Reference<AbstractResource> xPane;

    // Based on the ResourceURL of the given ResourceId look up the
    // corresponding factory descriptor.
    PaneContainer::iterator iDescriptor (
        ::std::find_if (
            maPaneContainer.begin(),
            maPaneContainer.end(),
            [&] (PaneDescriptor const& rPane) {
                return rPane.CompareURL(rxPaneId->getResourceURL());
            } ));

    if (iDescriptor == maPaneContainer.end())
    {
        // The requested pane can not be created by any of the factories
        // managed by the called BasicPaneFactory object.
        throw lang::IllegalArgumentException(u"BasicPaneFactory::createPane() called for unknown resource id"_ustr,
            nullptr,
            0);
    }

    if (iDescriptor->mxPane.is())
    {
        // The pane has already been created and is still active (has
        // not yet been released).  This should not happen.
        xPane = iDescriptor->mxPane;
    }
    else
    {
        // Create a new pane.
        switch (iDescriptor->mePaneId)
        {
            case CenterPaneId:
                xPane = CreateFrameWindowPane(rxPaneId);
                break;

            case FullScreenPaneId:
                xPane = CreateFullScreenPane(rxPaneId);
                break;

            case LeftImpressPaneId:
            case BottomImpressPaneId:
            case LeftDrawPaneId:
                xPane = CreateChildWindowPane(
                    rxPaneId,
                    *iDescriptor);
                break;
        }
        iDescriptor->mxPane = xPane;

        // Listen for the pane being disposed.
        if (xPane.is())
            xPane->addEventListener(mxListener);
    }
    iDescriptor->mbIsReleased = false;


    return xPane;
}

void BasicPaneFactory::releaseResource (
    const rtl::Reference<AbstractResource>& rxPane)
{
    ThrowIfDisposed();

    // Based on the given AbstractPane reference look up the corresponding factory
    // descriptor.
    PaneContainer::iterator iDescriptor (
        ::std::find_if(
            maPaneContainer.begin(),
            maPaneContainer.end(),
            [&] (PaneDescriptor const& rPane) { return rPane.ComparePane(rxPane); } ));

    if (iDescriptor == maPaneContainer.end())
    {
        // The given AbstractPane reference is either empty or the pane was not
        // created by any of the factories managed by the called
        // BasicPaneFactory object.
        throw lang::IllegalArgumentException(u"BasicPaneFactory::releasePane() called for pane that was not created by same factory."_ustr,
            nullptr,
            0);
    }

    // The given pane was created by one of the factories.  Child
    // windows are just hidden and will be reused when requested later.
    // Other windows are disposed and their reference is reset so that
    // on the next createPane() call for the same pane type the pane is
    // created anew.
    ChildWindowPane* pChildWindowPane = dynamic_cast<ChildWindowPane*>(rxPane.get());
    if (pChildWindowPane != nullptr)
    {
        iDescriptor->mbIsReleased = true;
        pChildWindowPane->Hide();
    }
    else
    {
        iDescriptor->mxPane = nullptr;
        if (rxPane.is())
        {
            // We are disposing the pane and do not have to be informed of
            // that.
            rxPane->removeEventListener(mxListener);
            rxPane->dispose();
        }
    }

}

//===== ConfigurationChangeListener ==========================================

void BasicPaneFactory::Listener::notifyConfigurationChange (
    const ConfigurationChangeEvent& /* rEvent */ )
{
    // FIXME: nothing to do
}

//===== lang::XEventListener ==================================================

void SAL_CALL BasicPaneFactory::Listener::disposing (
    const lang::EventObject& rEventObject)
{
    if (uno::Reference<XInterface>(cppu::getXWeak(mrParent.mxConfigurationControllerWeak.get().get())) == rEventObject.Source)
    {
        mrParent.mxConfigurationControllerWeak.clear();
    }
    else
    {
        // Has one of the panes been disposed?  If so, then release the
        // reference to that pane, but not the pane descriptor.
        rtl::Reference<AbstractResource> xPane = dynamic_cast<AbstractResource*>(rEventObject.Source.get());
        PaneContainer::iterator iDescriptor (
            ::std::find_if(
                mrParent.maPaneContainer.begin(),
                mrParent.maPaneContainer.end(),
                [&] (PaneDescriptor const& rPane) { return rPane.ComparePane(xPane); } ));
        if (iDescriptor != mrParent.maPaneContainer.end())
        {
            iDescriptor->mxPane = nullptr;
        }
    }
}

rtl::Reference<AbstractResource> BasicPaneFactory::CreateFrameWindowPane (
    const rtl::Reference<ResourceId>& rxPaneId)
{
    if (!mpViewShellBase)
        return nullptr;

    return new FrameWindowPane(rxPaneId, mpViewShellBase->GetViewWindow());
}

rtl::Reference<AbstractResource> BasicPaneFactory::CreateFullScreenPane(
    const rtl::Reference<ResourceId>& rxPaneId)
{
    const Reference<uno::XComponentContext>& xContext = comphelper::getProcessComponentContext();

    rtl::Reference<AbstractResource> xPane (
        new FullScreenPane(
            xContext,
            rxPaneId,
            mpViewShellBase->GetViewWindow(),
            mpViewShellBase->GetDocShell()));

    return xPane;
}

rtl::Reference<AbstractResource> BasicPaneFactory::CreateChildWindowPane (
    const rtl::Reference<ResourceId>& rxPaneId,
    const PaneDescriptor& rDescriptor)
{
    if (!mpViewShellBase)
        return nullptr;

    // Create the corresponding shell and determine the id of the child window.
    sal_uInt16 nChildWindowId = 0;
    ::std::unique_ptr<SfxShell> pShell;
    switch (rDescriptor.mePaneId)
    {
        case LeftImpressPaneId:
            pShell.reset(new LeftImpressPaneShell());
            nChildWindowId = ::sd::LeftPaneImpressChildWindow::GetChildWindowId();
            break;

        case BottomImpressPaneId:
            pShell.reset(new BottomImpressPaneShell());
            nChildWindowId = ::sd::BottomPaneImpressChildWindow::GetChildWindowId();
            break;

        case LeftDrawPaneId:
            pShell.reset(new LeftDrawPaneShell());
            nChildWindowId = ::sd::LeftPaneDrawChildWindow::GetChildWindowId();
            break;

        default:
            break;
    }

    // With shell and child window id create the ChildWindowPane
    // wrapper.
    if (!pShell)
        return nullptr;

    return new ChildWindowPane(
            rxPaneId,
            nChildWindowId,
            *mpViewShellBase,
            std::move(pShell));
}

void BasicPaneFactory::ThrowIfDisposed() const
{
    if (m_bDisposed)
    {
        throw lang::DisposedException (u"BasicPaneFactory object has already been disposed"_ustr,
            const_cast<uno::XWeak*>(static_cast<const uno::XWeak*>(this)));
    }
}

} // end of namespace sd::framework


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
