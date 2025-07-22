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

#include "ConfigurationControllerBroadcaster.hxx"
#include <framework/ConfigurationChangeListener.hxx>
#include <framework/ConfigurationChangeEvent.hxx>
#include <framework/AbstractResource.hxx>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <framework/ConfigurationController.hxx>
#include <comphelper/diagnose_ex.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing::framework;

namespace sd::framework {

ConfigurationControllerBroadcaster::ConfigurationControllerBroadcaster (
    const rtl::Reference<ConfigurationController>& rxController)
    : mxConfigurationController(rxController)
{
}

void ConfigurationControllerBroadcaster::AddListener(
    const rtl::Reference<ConfigurationChangeListener>& rxListener,
    ConfigurationChangeEventType rsEventType)
{
    if ( ! rxListener.is())
        throw lang::IllegalArgumentException(u"invalid listener"_ustr,
            cppu::getXWeak(mxConfigurationController.get()),
            0);

    maListenerMap.try_emplace(rsEventType);

    maListenerMap[rsEventType].push_back(rxListener);
}

void ConfigurationControllerBroadcaster::RemoveListener(
    const rtl::Reference<ConfigurationChangeListener>& rxListener)
{
    if ( ! rxListener.is())
        throw lang::IllegalArgumentException(u"invalid listener"_ustr,
            cppu::getXWeak(mxConfigurationController.get()),
            0);

    ListenerList::iterator iList;
    for (auto& rMap : maListenerMap)
    {
        iList = std::find(rMap.second.begin(), rMap.second.end(), rxListener);
        if (iList != rMap.second.end())
            rMap.second.erase(iList);
    }
}

void ConfigurationControllerBroadcaster::NotifyListeners (
    const ListenerList& rList,
    const ConfigurationChangeEvent& rEvent)
{
    // Create a local copy of the event in which the user data is modified
    // for every listener.
    ConfigurationChangeEvent aEvent (rEvent);

    for (const auto& rListener : rList)
    {
        try
        {
            rListener->notifyConfigurationChange(aEvent);
        }
        catch (const lang::DisposedException& rException)
        {
            // When the exception comes from the listener itself then
            // unregister it.
            if (rException.Context == cppu::getXWeak(rListener.get()))
                RemoveListener(rListener);
        }
        catch (const RuntimeException&)
        {
            DBG_UNHANDLED_EXCEPTION("sd");
        }
    }
}

void ConfigurationControllerBroadcaster::NotifyListeners (const ConfigurationChangeEvent& rEvent)
{
    // Notify the specialized listeners.
    ListenerMap::const_iterator iMap (maListenerMap.find(rEvent.Type));
    if (iMap != maListenerMap.end())
    {
        // Create a local list of the listeners to avoid problems with
        // concurrent changes and to be able to remove disposed listeners.
        ListenerList aList (iMap->second.begin(), iMap->second.end());
        NotifyListeners(aList,rEvent);
    }
}

void ConfigurationControllerBroadcaster::NotifyListeners (
    ConfigurationChangeEventType rsEventType,
    const rtl::Reference<ResourceId>& rxResourceId,
    const rtl::Reference<AbstractResource>& rxResourceObject)
{
    ConfigurationChangeEvent aEvent;
    aEvent.Type = rsEventType;
    aEvent.ResourceId = rxResourceId;
    aEvent.ResourceObject = rxResourceObject;
    try
    {
        NotifyListeners(aEvent);
    }
    catch (const lang::DisposedException&)
    {
    }
}

void ConfigurationControllerBroadcaster::DisposeAndClear()
{
    lang::EventObject aEvent;
    aEvent.Source = cppu::getXWeak(mxConfigurationController.get());
    while (!maListenerMap.empty())
    {
        ListenerMap::iterator iMap (maListenerMap.begin());
        if (iMap == maListenerMap.end())
            break;

        // When the first vector is empty then remove it from the map.
        if (iMap->second.empty())
        {
            maListenerMap.erase(iMap);
            continue;
        }
        else
        {
            rtl::Reference<ConfigurationChangeListener> xListener ( iMap->second.front() );
            if (xListener.is())
            {
                // Tell the listener that the configuration controller is
                // being disposed and remove the listener (for all event
                // types).
                try
                {
                    RemoveListener(xListener);
                    xListener->disposing(aEvent);
                }
                catch (const RuntimeException&)
                {
                    DBG_UNHANDLED_EXCEPTION("sd");
                }
            }
            else
            {
                // Remove just this reference to the listener.
                iMap->second.erase(iMap->second.begin());
            }
        }
    }
}

} // end of namespace sd::framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
