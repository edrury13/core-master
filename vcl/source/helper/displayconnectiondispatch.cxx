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

#include <vcl/svapp.hxx>
#include <tools/debug.hxx>

#include <displayconnectiondispatch.hxx>
#include <svdata.hxx>
#include <salinst.hxx>

using namespace vcl;
using namespace com::sun::star::uno;
using namespace com::sun::star::awt;

DisplayConnectionDispatch::DisplayConnectionDispatch()
{
}

DisplayConnectionDispatch::~DisplayConnectionDispatch()
{}

void DisplayConnectionDispatch::start()
{
    DBG_TESTSOLARMUTEX();
    ImplSVData* pSVData = ImplGetSVData();
    pSVData->mpDefInst->SetEventCallback( this );
}

void DisplayConnectionDispatch::terminate()
{
    DBG_TESTSOLARMUTEX();
    ImplSVData* pSVData = ImplGetSVData();

    if( pSVData )
    {
        pSVData->mpDefInst->SetEventCallback( nullptr );
    }

    SolarMutexReleaser aRel;

    std::scoped_lock aGuard( m_aMutex );
    std::vector<rtl::Reference<DisplayEventHandler>> aLocalList(m_aHandlers);
    for (auto const& elem : aLocalList)
        elem->shutdown();
}

void DisplayConnectionDispatch::addEventHandler(const rtl::Reference<DisplayEventHandler>& handler)
{
    std::scoped_lock aGuard( m_aMutex );

    m_aHandlers.push_back( handler );
}

void DisplayConnectionDispatch::removeEventHandler(
    const rtl::Reference<DisplayEventHandler>& handler)
{
    std::scoped_lock aGuard( m_aMutex );

    std::erase(m_aHandlers, handler);
}

bool DisplayConnectionDispatch::dispatchEvent(const void* pEvent)
{
    SolarMutexReleaser aRel;

    std::vector<rtl::Reference<DisplayEventHandler>> handlers;
    {
        std::scoped_lock aGuard( m_aMutex );
        handlers = m_aHandlers;
    }
    for (auto const& handle : handlers)
        if (handle->handleEvent(pEvent))
            return true;
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
