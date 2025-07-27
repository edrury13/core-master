/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <DocumentTimer.hxx>
#include <view.hxx>
#include <docsh.hxx>
#include <doc.hxx>
#include <cmdid.h>
#include <sfx2/objsh.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/viewfrm.hxx>
#include <unotools/mediadescriptor.hxx>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertyContainer.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <comphelper/string.hxx>
#include <tools/datetime.hxx>

using namespace css;

namespace sw
{

DocumentTimer::DocumentTimer(SwView* pView)
    : m_pView(pView)
    , m_pDocShell(pView ? pView->GetDocShell() : nullptr)
    , m_aSecondTimer("sw::DocumentTimer m_aSecondTimer")
    , m_aIdleTimer("sw::DocumentTimer m_aIdleTimer")
    , m_aAutoSaveTimer("sw::DocumentTimer m_aAutoSaveTimer")
    , m_nTotalSeconds(0)
    , m_nSessionSeconds(0)
    , m_aSessionStart( tools::Time::EMPTY )  // Initialize with empty time
    , m_bActive(false)
    , m_bIdle(false)
    , m_bModified(false)
{
    // Setup timers
    m_aSecondTimer.SetTimeout(1000); // 1 second
    m_aSecondTimer.SetInvokeHandler(LINK(this, DocumentTimer, SecondTimerHdl));
    
    m_aIdleTimer.SetTimeout(IDLE_TIMEOUT_MS);
    m_aIdleTimer.SetInvokeHandler(LINK(this, DocumentTimer, IdleTimerHdl));
    
    m_aAutoSaveTimer.SetTimeout(AUTOSAVE_INTERVAL_MS);
    m_aAutoSaveTimer.SetInvokeHandler(LINK(this, DocumentTimer, AutoSaveTimerHdl));
    
    // Load existing time data
    LoadFromDocument();
}

DocumentTimer::~DocumentTimer()
{
    Stop();
    if (m_bModified)
        SaveToDocument();
}

void DocumentTimer::Start()
{
    if (!m_bActive)
    {
        m_bActive = true;
        m_bIdle = false;
        m_aSessionStart = tools::Time( tools::Time::SYSTEM );
        m_nSessionSeconds = 0;
        
        m_aSecondTimer.Start();
        // Don't start idle timer for now
        // m_aIdleTimer.Start();
        m_aAutoSaveTimer.Start();
        
        UpdateDisplay();
    }
}

void DocumentTimer::Stop()
{
    if (m_bActive)
    {
        // Add current session time to total BEFORE setting inactive
        m_nTotalSeconds += GetCurrentSessionSeconds();
        
        m_bActive = false;
        m_nSessionSeconds = 0;
        
        m_aSecondTimer.Stop();
        m_aIdleTimer.Stop();
        m_aAutoSaveTimer.Stop();
        
        if (m_bModified)
            SaveToDocument();
        
        UpdateDisplay();
    }
}

void DocumentTimer::Pause()
{
    if (m_bActive && !m_bIdle)
    {
        m_bIdle = true;
        // Save current session time
        m_nSessionSeconds = GetCurrentSessionSeconds();
        UpdateDisplay();
    }
}

sal_Int64 DocumentTimer::GetCurrentSessionSeconds() const
{
    if (!m_bActive || m_bIdle)
        return m_nSessionSeconds;
    
    tools::Time aNow( tools::Time::SYSTEM );
    tools::Time aDiff = aNow - m_aSessionStart;
    return m_nSessionSeconds + aDiff.GetSec();
}

OUString DocumentTimer::GetTimeString() const
{
    sal_Int64 nTotalSec = GetTotalSeconds();
    sal_Int64 nHours = nTotalSec / 3600;
    sal_Int64 nMinutes = (nTotalSec % 3600) / 60;
    sal_Int64 nSeconds = nTotalSec % 60;
    
    OUString sHours = OUString::number(nHours);
    OUString sMinutes = OUString::number(nMinutes);
    OUString sSeconds = OUString::number(nSeconds);
    
    // Pad with zeros
    if (sHours.getLength() < 2)
        sHours = "0" + sHours;
    if (sMinutes.getLength() < 2)
        sMinutes = "0" + sMinutes;
    if (sSeconds.getLength() < 2)
        sSeconds = "0" + sSeconds;
    
    return sHours + ":" + sMinutes + ":" + sSeconds;
}

void DocumentTimer::OnUserActivity()
{
    if (m_bActive)
    {
        if (m_bIdle)
        {
            // Resume from idle
            m_bIdle = false;
            m_aSessionStart = tools::Time( tools::Time::SYSTEM );
        }
        m_aIdleTimer.Stop();
        m_aIdleTimer.Start();
    }
}

void DocumentTimer::OnFocusChanged(bool bHasFocus)
{
    if (m_bActive)
    {
        if (!bHasFocus && !m_bIdle)
        {
            Pause();
        }
        else if (bHasFocus && m_bIdle)
        {
            OnUserActivity();
        }
    }
}

IMPL_LINK_NOARG(DocumentTimer, SecondTimerHdl, Timer*, void)
{
    if (m_bActive)
    {
        UpdateDisplay();
        m_bModified = true;
        m_aSecondTimer.Start(); // Restart the timer
    }
}

IMPL_LINK_NOARG(DocumentTimer, IdleTimerHdl, Timer*, void)
{
    if (m_bActive && !m_bIdle)
    {
        Pause();
    }
}

IMPL_LINK_NOARG(DocumentTimer, AutoSaveTimerHdl, Timer*, void)
{
    if (m_bModified)
    {
        SaveTimeData();
    }
}

void DocumentTimer::UpdateDisplay()
{
    if (m_pView)
    {
        // Invalidate status bar to update timer display
        SfxBindings& rBindings = m_pView->GetViewFrame().GetBindings();
        rBindings.Invalidate(FN_STAT_DOCTIMER);
    }
}

void DocumentTimer::SaveTimeData()
{
    if (m_pDocShell)
    {
        SaveToDocument();
        m_bModified = false;
    }
}

void DocumentTimer::LoadFromDocument()
{
    if (!m_pDocShell)
        return;
    
    try
    {
        uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
            m_pDocShell->GetModel(), uno::UNO_QUERY);
        if (xDPS.is())
        {
            uno::Reference<document::XDocumentProperties> xDocProps = xDPS->getDocumentProperties();
            if (xDocProps.is())
            {
                uno::Reference<beans::XPropertyContainer> xUserProps = xDocProps->getUserDefinedProperties();
                uno::Reference<beans::XPropertySet> xPropSet(xUserProps, uno::UNO_QUERY);
                
                if (xPropSet.is() && xPropSet->getPropertySetInfo()->hasPropertyByName("TotalTimeWorked"))
                {
                    uno::Any aValue = xPropSet->getPropertyValue("TotalTimeWorked");
                    OUString sTime;
                    if (aValue >>= sTime)
                    {
                        m_nTotalSeconds = sTime.toInt64();
                    }
                }
            }
        }
    }
    catch (...)
    {
        // If property doesn't exist or error, start with 0
        m_nTotalSeconds = 0;
    }
}

void DocumentTimer::SaveToDocument()
{
    if (!m_pDocShell)
        return;
    
    try
    {
        uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
            m_pDocShell->GetModel(), uno::UNO_QUERY);
        if (xDPS.is())
        {
            uno::Reference<document::XDocumentProperties> xDocProps = xDPS->getDocumentProperties();
            if (xDocProps.is())
            {
                uno::Reference<beans::XPropertyContainer> xUserProps = xDocProps->getUserDefinedProperties();
                uno::Reference<beans::XPropertySet> xPropSet(xUserProps, uno::UNO_QUERY);
                
                if (xUserProps.is() && xPropSet.is())
                {
                    OUString sPropName = "TotalTimeWorked";
                    OUString sValue = OUString::number(GetTotalSeconds());
                    
                    // Remove old property if exists
                    if (xPropSet->getPropertySetInfo()->hasPropertyByName(sPropName))
                    {
                        xUserProps->removeProperty(sPropName);
                    }
                    
                    // Add new property
                    xUserProps->addProperty(sPropName, 
                        beans::PropertyAttribute::REMOVABLE, 
                        uno::Any(sValue));
                    
                    // Also store last update time
                    sPropName = "TimerLastUpdated";
                    DateTime aDateTime(DateTime::SYSTEM);
                    // Store as simple date/time string
                    sValue = OUString::number(aDateTime.GetYear()) + "-" +
                             OUString::number(aDateTime.GetMonth()) + "-" +
                             OUString::number(aDateTime.GetDay()) + " " +
                             OUString::number(aDateTime.GetHour()) + ":" +
                             OUString::number(aDateTime.GetMin()) + ":" +
                             OUString::number(aDateTime.GetSec());
                    
                    if (xPropSet->getPropertySetInfo()->hasPropertyByName(sPropName))
                    {
                        xUserProps->removeProperty(sPropName);
                    }
                    
                    xUserProps->addProperty(sPropName,
                        beans::PropertyAttribute::REMOVABLE,
                        uno::Any(sValue));
                }
            }
        }
    }
    catch (...)
    {
        // Ignore errors in saving
    }
}

void DocumentTimer::OnDocumentSave()
{
    if (m_bModified)
    {
        SaveToDocument();
        m_bModified = false;
    }
}

} // namespace sw

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */