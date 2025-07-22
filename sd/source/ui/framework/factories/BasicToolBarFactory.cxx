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

#include <framework/factories/BasicToolBarFactory.hxx>
#include <framework/ConfigurationController.hxx>

#include <ViewTabBar.hxx>
#include <framework/FrameworkHelper.hxx>
#include <ResourceId.hxx>
#include <DrawController.hxx>
#include <unotools/mediadescriptor.hxx>

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/frame/XController.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::drawing::framework;

namespace sd::framework {

//===== BasicToolBarFactory ===================================================

BasicToolBarFactory::BasicToolBarFactory(const rtl::Reference<::sd::DrawController>& rxController)
{
    try
    {
        mxController = rxController;

        utl::MediaDescriptor aDescriptor (mxController->getModel()->getArgs());
        if ( ! aDescriptor.getUnpackedValueOrDefault(
            utl::MediaDescriptor::PROP_PREVIEW,
            false))
        {
            // Register the factory for its supported tool bars.
            mxConfigurationController = mxController->getConfigurationController();
            if (mxConfigurationController.is())
            {
                mxConfigurationController->addResourceFactory(
                    FrameworkHelper::msViewTabBarURL, this);
                mxConfigurationController->addEventListener(static_cast<lang::XEventListener*>(this));
            }
        }
        else
        {
            // The view shell is in preview mode and thus does not need
            // the view tab bar.
            mxConfigurationController = nullptr;
        }
    }
    catch (RuntimeException&)
    {
        Shutdown();
        throw;
    }
}


BasicToolBarFactory::~BasicToolBarFactory()
{
}

void BasicToolBarFactory::disposing(std::unique_lock<std::mutex>&)
{
    Shutdown();
}

void BasicToolBarFactory::Shutdown()
{
    if (mxConfigurationController.is())
    {
        mxConfigurationController->removeEventListener(static_cast<lang::XEventListener*>(this));
        mxConfigurationController->removeResourceFactoryForReference(this);
        mxConfigurationController = nullptr;
    }
}

//----- lang::XEventListener --------------------------------------------------

void SAL_CALL BasicToolBarFactory::disposing (
    const lang::EventObject& rEventObject)
{
    if (rEventObject.Source == cppu::getXWeak(mxConfigurationController.get()))
        mxConfigurationController = nullptr;
}

//===== AbstractPaneFactory ==========================================================

rtl::Reference<AbstractResource> BasicToolBarFactory::createResource (
    const rtl::Reference<ResourceId>& rxToolBarId)
{
    ThrowIfDisposed();

    if (rxToolBarId->getResourceURL() != FrameworkHelper::msViewTabBarURL)
        throw lang::IllegalArgumentException();

    rtl::Reference<AbstractResource> xToolBar = new ViewTabBar(rxToolBarId, mxController);
    return xToolBar;
}

void BasicToolBarFactory::releaseResource (
    const rtl::Reference<AbstractResource>& rxToolBar)
{
    ThrowIfDisposed();

    if (rxToolBar.is())
        rxToolBar->dispose();
}

void BasicToolBarFactory::ThrowIfDisposed() const
{
    if (m_bDisposed)
    {
        throw lang::DisposedException (u"BasicToolBarFactory object has already been disposed"_ustr,
            const_cast<uno::XWeak*>(static_cast<const uno::XWeak*>(this)));
    }
}

} // end of namespace sd::framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
