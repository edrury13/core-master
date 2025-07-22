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

#include <osl/mutex.hxx>

#include <com/sun/star/uno/Reference.hxx>
#include <rtl/ref.hxx>
#include <map>
#include <memory>
#include <vector>

namespace sd::framework { class Configuration; }

namespace sd::framework {
class ResourceId;
class ConfigurationControllerBroadcaster;
class ResourceFactoryManager;
class ResourceFactory;
class AbstractResource;

/** Manage the set of active resources.  Activate and deactivate resources.
*/
class ConfigurationControllerResourceManager
{
public:
    /** For every active resource both the resource itself as well as its
        creating factory are remembered, so that on deactivation, the
        resource can be deactivated by this factory.
    */
    struct ResourceDescriptor
    {
        rtl::Reference<AbstractResource> mxResource;
        rtl::Reference<framework::ResourceFactory> mxResourceFactory;
    };

    /** A new ResourceManager object is created with the resource factory
        container for creating resources and the event broadcaster for
        notifying ConfigurationChangeListeners of activated or deactivated
        resources.
    */
    ConfigurationControllerResourceManager (
        std::shared_ptr<ResourceFactoryManager> pResourceFactoryContainer,
        std::shared_ptr<ConfigurationControllerBroadcaster> pBroadcaster);

    ~ConfigurationControllerResourceManager();

    /// Forbid copy construction and copy assignment
    ConfigurationControllerResourceManager(const ConfigurationControllerResourceManager&) = delete;
    ConfigurationControllerResourceManager& operator=(const ConfigurationControllerResourceManager&) = delete;

    /** Activate all the resources that are specified by resource ids in
        rResources.  The resource ids of activated resources are added to
        the given configuration.  Activated resources are notified to all
        interested ConfigurationChangeListeners.
    */
    void ActivateResources (
        const ::std::vector<
            rtl::Reference<sd::framework::ResourceId> >& rResources,
        const rtl::Reference<sd::framework::Configuration>& rxConfiguration);

    /** Deactivate all the resources that are specified by resource ids in
        rResources.  The resource ids of deactivated resources are removed
        from the given configuration.  Activated resources are notified to all
        interested ConfigurationChangeListeners.
    */
    void DeactivateResources (
        const ::std::vector<
            rtl::Reference<sd::framework::ResourceId> >& rResources,
        const rtl::Reference<sd::framework::Configuration>& rxConfiguration);

    /** Return the descriptor for the specified resource.
        @return
            When there is no active resource for the given resource id then
            an empty descriptor is returned.
    */
    ResourceDescriptor GetResource (
        const rtl::Reference<sd::framework::ResourceId>& rxResourceId);

private:
    osl::Mutex maMutex;

    class ResourceComparator
    {
    public:
        bool operator() (
            const rtl::Reference<sd::framework::ResourceId>& rxId1,
            const rtl::Reference<sd::framework::ResourceId>& rxId2) const;
    };

    typedef ::std::map<
        rtl::Reference<sd::framework::ResourceId>,
        ResourceDescriptor,
        ResourceComparator> ResourceMap;
    ResourceMap maResourceMap;

    std::shared_ptr<ResourceFactoryManager> mpResourceFactoryContainer;

    /** This broadcaster is used to notify the activation and deactivation
        of resources.
    */
    std::shared_ptr<ConfigurationControllerBroadcaster> mpBroadcaster;

    void ActivateResource (
        const rtl::Reference<sd::framework::ResourceId>& rxResourceId,
        const rtl::Reference<sd::framework::Configuration>& rxConfiguration);

    void DeactivateResource (
        const rtl::Reference<sd::framework::ResourceId>& rxResourceId,
        const rtl::Reference<sd::framework::Configuration>& rxConfiguration);

    void AddResource (
        const rtl::Reference<sd::framework::AbstractResource>& rxResource,
        const rtl::Reference<framework::ResourceFactory>& rxFactory);

    ResourceDescriptor RemoveResource (
        const rtl::Reference<sd::framework::ResourceId>& rxResourceId);
};

} // end of namespace sd::framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
