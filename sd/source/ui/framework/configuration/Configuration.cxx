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

#include <framework/Configuration.hxx>
#include <framework/ConfigurationChangeEvent.hxx>
#include <framework/FrameworkHelper.hxx>
#include <framework/ConfigurationController.hxx>
#include <framework/AbstractPane.hxx>
#include <framework/AbstractView.hxx>

#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing::framework;
using ::sd::framework::FrameworkHelper;

namespace {
/** Use the ResourceId::compareTo() method to implement a compare operator
    for STL containers.
*/
class XResourceIdLess
{
public:
    bool operator () (const rtl::Reference<sd::framework::ResourceId>& rId1, const rtl::Reference<sd::framework::ResourceId>& rId2) const
    {
        return rId1->compareTo(rId2) == -1;
    }
};

} // end of anonymous namespace

namespace sd::framework {

class Configuration::ResourceContainer
    : public ::std::set<rtl::Reference<ResourceId>, XResourceIdLess>
{
public:
    ResourceContainer() {}
};

//===== Configuration =========================================================

Configuration::Configuration (
    const rtl::Reference<ConfigurationController>& rxBroadcaster,
    bool bBroadcastRequestEvents)
    : mpResourceContainer(new ResourceContainer()),
      mxBroadcaster(rxBroadcaster),
      mbBroadcastRequestEvents(bBroadcastRequestEvents)
{
}

Configuration::Configuration (
    const rtl::Reference<ConfigurationController>& rxBroadcaster,
    bool bBroadcastRequestEvents,
    const ResourceContainer& rResourceContainer)
    : mpResourceContainer(new ResourceContainer(rResourceContainer)),
      mxBroadcaster(rxBroadcaster),
      mbBroadcastRequestEvents(bBroadcastRequestEvents)
{
}

Configuration::~Configuration()
{
}

void Configuration::disposing(std::unique_lock<std::mutex>&)
{
    mpResourceContainer->clear();
    mxBroadcaster = nullptr;
}

//----- Configuration --------------------------------------------------------

void Configuration::addResource (const rtl::Reference<ResourceId>& rxResourceId)
{
    ThrowIfDisposed();

    if ( ! rxResourceId.is() || rxResourceId->getResourceURL().isEmpty())
        throw css::lang::IllegalArgumentException();

    if (mpResourceContainer->insert(rxResourceId).second)
    {
        SAL_INFO("sd.fwk", __func__ << ": Configuration::addResource() " <<
                FrameworkHelper::ResourceIdToString(rxResourceId));
        PostEvent(rxResourceId, true);
    }
}

void Configuration::removeResource (const rtl::Reference<ResourceId>& rxResourceId)
{
    ThrowIfDisposed();

    if ( ! rxResourceId.is() || rxResourceId->getResourceURL().isEmpty())
        throw css::lang::IllegalArgumentException();

    ResourceContainer::iterator iResource (mpResourceContainer->find(rxResourceId));
    if (iResource != mpResourceContainer->end())
    {
        SAL_INFO("sd.fwk", __func__ << ": Configuration::removeResource() " <<
                FrameworkHelper::ResourceIdToString(rxResourceId));
        PostEvent(rxResourceId,false);
        mpResourceContainer->erase(iResource);
    }
}

std::vector<rtl::Reference<ResourceId> > Configuration::getResources (
    const rtl::Reference<ResourceId>& rxAnchorId,
    std::u16string_view rsResourceURLPrefix,
    AnchorBindingMode eMode)
{
    std::unique_lock aGuard (m_aMutex);
    ThrowIfDisposed();

    const bool bFilterResources (!rsResourceURLPrefix.empty());

    // Collect the matching resources in a vector.
    ::std::vector<rtl::Reference<ResourceId> > aResources;
    for (const auto& rxResource : *mpResourceContainer)
    {
        if ( ! rxResource->isBoundTo(rxAnchorId,eMode))
            continue;

        if (bFilterResources)
        {
            // Apply the given resource prefix as filter.

            // Make sure that the resource is bound directly to the anchor.
            if (eMode != AnchorBindingMode_DIRECT
                && ! rxResource->isBoundTo(rxAnchorId, AnchorBindingMode_DIRECT))
            {
                continue;
            }

            // Make sure that the resource URL matches the given prefix.
            if ( ! rxResource->getResourceURL().match(rsResourceURLPrefix))
            {
                continue;
            }
        }

        aResources.push_back(rxResource);
    }

    return aResources;
}

bool Configuration::hasResource (const rtl::Reference<ResourceId>& rxResourceId)
{
    std::unique_lock aGuard (m_aMutex);
    ThrowIfDisposed();

    return rxResourceId.is()
        && mpResourceContainer->find(rxResourceId) != mpResourceContainer->end();
}

rtl::Reference<Configuration> Configuration::createClone()
{
    std::unique_lock aGuard (m_aMutex);
    ThrowIfDisposed();

    return new Configuration(
        mxBroadcaster,
        mbBroadcastRequestEvents,
        *mpResourceContainer);
}

//----- XNamed ----------------------------------------------------------------

OUString SAL_CALL Configuration::getName()
{
    std::unique_lock aGuard (m_aMutex);
    OUStringBuffer aString;

    if (m_bDisposed)
        aString.append("DISPOSED ");
    aString.append("Configuration[");

    ResourceContainer::const_iterator iResource;
    for (iResource=mpResourceContainer->begin();
         iResource!=mpResourceContainer->end();
         ++iResource)
    {
        if (iResource != mpResourceContainer->begin())
            aString.append(", ");
        aString.append(FrameworkHelper::ResourceIdToString(*iResource));
    }
    aString.append("]");

    return aString.makeStringAndClear();
}

void SAL_CALL Configuration::setName (const OUString&)
{
    // ignored.
}

void Configuration::PostEvent (
    const rtl::Reference<ResourceId>& rxResourceId,
    const bool bActivation)
{
    OSL_ASSERT(rxResourceId.is());

    if (!mxBroadcaster.is())
        return;

    ConfigurationChangeEvent aEvent;
    aEvent.ResourceId = rxResourceId;
    if (bActivation)
        if (mbBroadcastRequestEvents)
            aEvent.Type = ConfigurationChangeEventType::ResourceActivationRequest;
        else
            aEvent.Type = ConfigurationChangeEventType::ResourceActivation;
    else
        if (mbBroadcastRequestEvents)
            aEvent.Type = ConfigurationChangeEventType::ResourceDeactivationRequest;
        else
            aEvent.Type = ConfigurationChangeEventType::ResourceDeactivation;
    aEvent.Configuration = this;

    mxBroadcaster->notifyEvent(aEvent);
}

void Configuration::ThrowIfDisposed() const
{
    if (m_bDisposed)
    {
        throw lang::DisposedException (u"Configuration object has already been disposed"_ustr,
            const_cast<uno::XWeak*>(static_cast<const uno::XWeak*>(this)));
    }
}

bool AreConfigurationsEquivalent (
    const rtl::Reference<Configuration>& rxConfiguration1,
    const rtl::Reference<Configuration>& rxConfiguration2)
{
    if (rxConfiguration1.is() != rxConfiguration2.is())
        return false;
    if ( ! rxConfiguration1.is() && ! rxConfiguration2.is())
        return true;

    // Get the lists of resources from the two given configurations.
    const std::vector<rtl::Reference<ResourceId> > aResources1(
        rxConfiguration1->getResources(
            nullptr, u"", AnchorBindingMode_INDIRECT));
    const std::vector<rtl::Reference<ResourceId> > aResources2(
        rxConfiguration2->getResources(
            nullptr, u"", AnchorBindingMode_INDIRECT));

    // When the number of resources differ then the configurations can not
    // be equivalent.
    // Comparison of the two lists of resource ids relies on their
    // ordering.
    return std::equal(aResources1.begin(), aResources1.end(), aResources2.begin(), aResources2.end(),
        [](const rtl::Reference<ResourceId>& a, const rtl::Reference<ResourceId>& b) {
            if (a.is() && b.is())
                return a->compareTo(b) == 0;
            return a.is() == b.is();
        });
}

ConfigurationChangeListener::~ConfigurationChangeListener() {}

ConfigurationChangeRequest::~ConfigurationChangeRequest() {}

AbstractPane::~AbstractPane() {}

AbstractView::~AbstractView() {}

AbstractResource::~AbstractResource() {}

} // end of namespace sd::framework


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
