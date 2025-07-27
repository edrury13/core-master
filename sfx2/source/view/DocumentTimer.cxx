/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/DocumentTimer.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/event.hxx>
#include <unotools/mediadescriptor.hxx>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertyContainer.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <comphelper/string.hxx>
#include <tools/datetime.hxx>
#include <sal/log.hxx>

using namespace css;

namespace sfx2
{

DocumentTimer::DocumentTimer(SfxViewShell* pViewShell)
    : m_pViewShell(pViewShell)
    , m_pDocShell(pViewShell ? pViewShell->GetObjectShell() : nullptr)
    , m_aSecondTimer("sfx2::DocumentTimer m_aSecondTimer")
    , m_aAutoSaveTimer("sfx2::DocumentTimer m_aAutoSaveTimer")
    , m_nTotalSeconds(0)
    , m_nSessionSeconds(0)
    , m_aSessionStart( tools::Time::EMPTY )
    , m_bActive(false)
    , m_bModified(false)
{
    // Setup timers
    m_aSecondTimer.SetTimeout(1000); // 1 second
    m_aSecondTimer.SetInvokeHandler(LINK(this, DocumentTimer, SecondTimerHdl));
    
    m_aAutoSaveTimer.SetTimeout(AUTOSAVE_INTERVAL_MS);
    m_aAutoSaveTimer.SetInvokeHandler(LINK(this, DocumentTimer, AutoSaveTimerHdl));
    
    // Start listening to document events
    if (m_pDocShell)
    {
        StartListening(*m_pDocShell);
    }
    
    // Load existing time data
    LoadFromDocument();
}

DocumentTimer::~DocumentTimer()
{
    Stop();
    if (m_bModified)
        SaveToDocument();
    
    // Stop listening to document events
    if (m_pDocShell)
    {
        EndListening(*m_pDocShell);
    }
}

void DocumentTimer::Start()
{
    if (!m_bActive)
    {
        SAL_WARN("sfx2", "DocumentTimer::Start() called");
        m_bActive = true;
        m_aSessionStart = tools::Time( tools::Time::SYSTEM );
        m_nSessionSeconds = 0;
        
        SAL_WARN("sfx2", "Starting timers - SecondTimer active before: " << m_aSecondTimer.IsActive());
        m_aSecondTimer.Start();
        SAL_WARN("sfx2", "SecondTimer active after: " << m_aSecondTimer.IsActive());
        
        m_aAutoSaveTimer.Start();
        
        UpdateDisplay();
        SAL_WARN("sfx2", "DocumentTimer::Start() completed, timer should be running");
    }
    else
    {
        SAL_WARN("sfx2", "DocumentTimer::Start() called but already active!");
    }
}

void DocumentTimer::Stop()
{
    SAL_WARN("sfx2", "DocumentTimer::Stop() called, active=" << m_bActive);
    if (m_bActive)
    {
        // Add current session time to total BEFORE setting inactive
        m_nTotalSeconds += GetCurrentSessionSeconds();
        
        m_bActive = false;
        m_nSessionSeconds = 0;
        
        m_aSecondTimer.Stop();
        m_aAutoSaveTimer.Stop();
        
        if (m_bModified)
            SaveToDocument();
        
        UpdateDisplay();
        SAL_WARN("sfx2", "DocumentTimer::Stop() completed");
    }
}

sal_Int64 DocumentTimer::GetCurrentSessionSeconds() const
{
    if (!m_bActive)
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

IMPL_LINK_NOARG(DocumentTimer, SecondTimerHdl, Timer*, void)
{
    SAL_WARN("sfx2", "DocumentTimer::SecondTimerHdl called, active=" << m_bActive);
    if (m_bActive)
    {
        UpdateDisplay();
        m_bModified = true;
        m_aSecondTimer.Start(); // Restart the timer
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
    if (m_pViewShell)
    {
        SAL_WARN("sfx2", "DocumentTimer::UpdateDisplay called, time=" << GetTimeString());
        // Invalidate status bar to update timer display
        SfxBindings& rBindings = m_pViewShell->GetViewFrame().GetBindings();
        // Need to use the correct slot ID for Writer's timer
        rBindings.Invalidate(21191); // FN_STAT_DOCTIMER
    }
    else
    {
        SAL_WARN("sfx2", "DocumentTimer::UpdateDisplay - no ViewShell!");
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

void DocumentTimer::Notify(SfxBroadcaster& /*rBC*/, const SfxHint& rHint)
{
    if (rHint.GetId() == SfxHintId::ThisIsAnSfxEventHint)
    {
        const SfxEventHint* pEventHint = dynamic_cast<const SfxEventHint*>(&rHint);
        if (pEventHint)
        {
            SfxEventHintId nEventId = pEventHint->GetEventId();
            
            // Save timer data when document is saved
            if (nEventId == SfxEventHintId::SaveDocDone || 
                nEventId == SfxEventHintId::SaveAsDocDone ||
                nEventId == SfxEventHintId::SaveToDocDone)
            {
                OnDocumentSave();
            }
        }
    }
}

} // namespace sfx2

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */