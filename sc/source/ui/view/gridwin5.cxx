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

#include <editeng/flditem.hxx>

#include <svx/fmpage.hxx>
#include <svx/svdobj.hxx>
#include <svx/svdpagv.hxx>
#include <svx/ImageMapInfo.hxx>
#include <vcl/imapobj.hxx>
#include <vcl/help.hxx>
#include <tools/urlobj.hxx>
#include <sfx2/sfxhelp.hxx>

#include <AccessibleDocument.hxx>
#include <com/sun/star/accessibility/XAccessible.hpp>

#include <gridwin.hxx>
#include <viewdata.hxx>
#include <drawview.hxx>
#include <drwlayer.hxx>
#include <document.hxx>
#include <notemark.hxx>
#include <chgtrack.hxx>
#include <chgviset.hxx>
#include <dbfunc.hxx>
#include <postit.hxx>
#include <global.hxx>

bool ScGridWindow::ShowNoteMarker( SCCOL nPosX, SCROW nPosY, bool bKeyboard )
{
    bool bDone = false;

    ScDocument& rDoc = mrViewData.GetDocument();
    SCTAB       nTab = mrViewData.GetTabNo();
    ScAddress   aCellPos( nPosX, nPosY, nTab );

    OUString aTrackText;
    bool bLeftEdge = false;

    // change tracking

    ScChangeTrack* pTrack = rDoc.GetChangeTrack();
    ScChangeViewSettings* pSettings = rDoc.GetChangeViewSettings();
    if ( pTrack && pTrack->GetFirst() && pSettings && pSettings->ShowChanges())
    {
        const ScChangeAction* pFound = nullptr;
        const ScChangeAction* pFoundContent = nullptr;
        const ScChangeAction* pFoundMove = nullptr;
        const ScChangeAction* pAction = pTrack->GetFirst();
        while (pAction)
        {
            if ( pAction->IsVisible() &&
                 ScViewUtil::IsActionShown( *pAction, *pSettings, rDoc ) )
            {
                ScChangeActionType eType = pAction->GetType();
                const ScBigRange& rBig = pAction->GetBigRange();
                if ( rBig.aStart.Tab() == nTab )
                {
                    ScRange aRange = rBig.MakeRange( rDoc );

                    if ( eType == SC_CAT_DELETE_ROWS )
                        aRange.aEnd.SetRow( aRange.aStart.Row() );
                    else if ( eType == SC_CAT_DELETE_COLS )
                        aRange.aEnd.SetCol( aRange.aStart.Col() );

                    if ( aRange.Contains( aCellPos ) )
                    {
                        pFound = pAction;       // the last one wins
                        switch ( eType )
                        {
                            case SC_CAT_CONTENT :
                                pFoundContent = pAction;
                            break;
                            case SC_CAT_MOVE :
                                pFoundMove = pAction;
                            break;
                            default:
                            {
                                // added to avoid warnings
                            }
                        }
                    }
                }
                if ( eType == SC_CAT_MOVE )
                {
                    ScRange aRange =
                        static_cast<const ScChangeActionMove*>(pAction)->
                        GetFromRange().MakeRange( rDoc );
                    if ( aRange.Contains( aCellPos ) )
                    {
                        pFound = pAction;
                    }
                }
            }
            pAction = pAction->GetNext();
        }

        if ( pFound )
        {
            if ( pFoundContent && pFound->GetType() != SC_CAT_CONTENT )
                pFound = pFoundContent;     // content wins
            if ( pFoundMove && pFound->GetType() != SC_CAT_MOVE &&
                    pFoundMove->GetActionNumber() >
                    pFound->GetActionNumber() )
                pFound = pFoundMove;        // move wins

            // for deleted columns: Arrow on the left side of the cell
            if ( pFound->GetType() == SC_CAT_DELETE_COLS )
                bLeftEdge = true;

            DateTime aDT = pFound->GetDateTime();
            aTrackText  = pFound->GetUser()
                        + ", "
                        + ScGlobal::getLocaleData().getDate(aDT)
                        + " "
                        + ScGlobal::getLocaleData().getTime(aDT)
                        + ":\n";
            OUString aComStr=pFound->GetComment();
            if(!aComStr.isEmpty())
            {
                aTrackText += aComStr + "\n( ";
            }
            OUString aTmp = pFound->GetDescription(rDoc);
            aTrackText += aTmp;
            if(!aComStr.isEmpty())
            {
                aTrackText += ")";
            }
        }
    }

    // Note, only if it is not already displayed on the Drawing Layer:
    const ScPostIt* pNote = rDoc.GetNote( aCellPos );
    if ( (!aTrackText.isEmpty()) || (pNote && !pNote->IsCaptionShown()) )
    {
        bool bNew = true;
        bool bFast = false;
        if (mpNoteOverlay) // A note already shown
        {
            if (mpNoteOverlay->GetDocPos() == aCellPos)
                bNew = false; // then stop
            else
                bFast = true; // otherwise, at once

            //  marker which was shown for ctrl-F1 isn't removed by mouse events
            if (mpNoteOverlay->IsByKeyboard() && !bKeyboard)
                bNew = false;
        }
        if (bNew)
        {
            if (bKeyboard)
                bFast = true; // keyboard also shows the marker immediately

            mpNoteOverlay.reset(new ScNoteOverlay(*this, aCellPos, aTrackText, bLeftEdge, bFast, bKeyboard));
        }

        bDone = true;       // something is shown (old or new)
    }

    return bDone;
}

void ScGridWindow::RequestHelp(const HelpEvent& rHEvt)
{
    bool bDone = false;
    OUString aFormulaText;
    tools::Rectangle aFormulaPixRect;
    bool bHelpEnabled = bool(rHEvt.GetMode() & ( HelpEventMode::BALLOON | HelpEventMode::QUICK ));
    SdrView* pDrView = mrViewData.GetScDrawView();
    bool bDrawTextEdit = false;
    if (pDrView)
        bDrawTextEdit = pDrView->IsTextEdit();
    //  notes or change tracking
    if ( bHelpEnabled && !bDrawTextEdit )
    {
        Point       aPosPixel = ScreenToOutputPixel( rHEvt.GetMousePosPixel() );
        SCCOL nPosX;
        SCROW nPosY;
        ScDocument& rDoc = mrViewData.GetDocument();
        SCTAB       nTab = mrViewData.GetTabNo();
        const ScViewOptions& rOpts = mrViewData.GetOptions();
        mrViewData.GetPosFromPixel( aPosPixel.X(), aPosPixel.Y(), eWhich, nPosX, nPosY );

        if ( ShowNoteMarker( nPosX, nPosY, false ) )
        {
            Window::RequestHelp( rHEvt );   // turn off old Tip/Balloon
            bDone = true;
        }

        if ( rOpts.GetOption(sc::ViewOption::FORMULAS_MARKS) )
        {
            aFormulaText = rDoc.GetFormula( nPosX, nPosY, nTab );
            if ( !aFormulaText.isEmpty() ) {
                const ScPatternAttr* pPattern = rDoc.GetPattern( nPosX, nPosY, nTab );
                aFormulaPixRect = mrViewData.GetEditArea( eWhich, nPosX, nPosY, this, pPattern, true );
            }
        }
    }

    if (!bDone && mpNoteOverlay)
    {
        if (mpNoteOverlay->IsByKeyboard())
        {
            //  marker which was shown for ctrl-F1 isn't removed by mouse events
        }
        else
        {
            mpNoteOverlay.reset();
        }
    }

    if ( !aFormulaText.isEmpty() )
    {
        tools::Rectangle aScreenRect(OutputToScreenPixel(aFormulaPixRect.TopLeft()),
                                     OutputToScreenPixel(aFormulaPixRect.BottomRight()));
        if ( rHEvt.GetMode() & HelpEventMode::BALLOON )
            Help::ShowBalloon(this, rHEvt.GetMousePosPixel(), aScreenRect, aFormulaText);
        else if ( rHEvt.GetMode() & HelpEventMode::QUICK )
            Help::ShowQuickHelp(this, aScreenRect, aFormulaText);
        bDone = true;
    }

    //  Image-Map / Text-URL

    if ( bHelpEnabled && !bDone && !nButtonDown )       // only without pressed button
    {
        OUString aHelpText;
        tools::Rectangle aPixRect;
        Point aPosPixel = ScreenToOutputPixel( rHEvt.GetMousePosPixel() );

        if ( pDrView )                                      // URL / Image-Map
        {
            SdrViewEvent aVEvt;
            MouseEvent aMEvt( aPosPixel, 1, MouseEventModifiers::NONE, MOUSE_LEFT );
            SdrHitKind eHit = pDrView->PickAnything( aMEvt, SdrMouseEventKind::BUTTONDOWN, aVEvt );

            if ( eHit != SdrHitKind::NONE && aVEvt.mpObj != nullptr )
            {
                // URL for IMapObject below Pointer is help text
                if (SvxIMapInfo::GetIMapInfo(aVEvt.mpObj))
                {
                    Point aLogicPos = PixelToLogic( aPosPixel );
                    IMapObject* pIMapObj = SvxIMapInfo::GetHitIMapObject(
                                                    aVEvt.mpObj, aLogicPos, GetOutDev() );

                    if ( pIMapObj )
                    {
                        // For image maps show the description, if available
                        aHelpText = pIMapObj->GetAltText();
                        if (aHelpText.isEmpty())
                            aHelpText = SfxHelp::GetURLHelpText(pIMapObj->GetURL());
                        aPixRect = LogicToPixel(aVEvt.mpObj->GetLogicRect());
                    }
                }
                // URL in shape text or at shape itself (URL in text overrides object URL)
                if ( aHelpText.isEmpty() )
                {
                    if( aVEvt.meEvent == SdrEventKind::ExecuteUrl )
                    {
                        if (aVEvt.mpURLField && !aVEvt.mpURLField->GetURL().startsWith("#"))
                        {
                            aHelpText = SfxHelp::GetURLHelpText(aVEvt.mpURLField->GetURL());
                            aPixRect = LogicToPixel(aVEvt.mpObj->GetLogicRect());
                        }
                    }
                    else
                    {
                        SdrPageView* pPV = nullptr;
                        Point aMDPos = PixelToLogic( aPosPixel );
                        SdrObject* pObj = pDrView->PickObj(aMDPos, pDrView->getHitTolLog(), pPV, SdrSearchOptions::ALSOONMASTER);
                        if (pObj)
                        {
                            if ( pObj->IsGroupObject() )
                            {
                                    SdrObject* pHit = pDrView->PickObj(aMDPos, pDrView->getHitTolLog(), pPV, SdrSearchOptions::DEEP);
                                    if (pHit)
                                        pObj = pHit;
                            }
                            // Fragments pointing into the current document need no tooltip
                            // describing the ctrl-click functionality.
                            if ( !pObj->getHyperlink().isEmpty() && !pObj->getHyperlink().startsWith("#") )
                            {
                                aPixRect = LogicToPixel(aVEvt.mpObj->GetLogicRect());
                                aHelpText = SfxHelp::GetURLHelpText(pObj->getHyperlink());
                            }
                        }
                    }
                }
            }
        }

        if ( aHelpText.isEmpty() )                                 // Text-URL
        {
            OUString aUrl;
            if ( GetEditUrl( aPosPixel, nullptr, &aUrl ) )
            {
                aHelpText = SfxHelp::GetURLHelpText(
                    INetURLObject::decode(aUrl, INetURLObject::DecodeMechanism::Unambiguous));

                ScDocument& rDoc = mrViewData.GetDocument();
                SCCOL nPosX;
                SCROW nPosY;
                SCTAB       nTab = mrViewData.GetTabNo();
                mrViewData.GetPosFromPixel( aPosPixel.X(), aPosPixel.Y(), eWhich, nPosX, nPosY );
                const ScPatternAttr* pPattern = rDoc.GetPattern( nPosX, nPosY, nTab );

                // bForceToTop = sal_False, use the cell's real position
                aPixRect = mrViewData.GetEditArea( eWhich, nPosX, nPosY, this, pPattern, false );
            }
        }

        if ( !aHelpText.isEmpty() )
        {
            tools::Rectangle aScreenRect(OutputToScreenPixel(aPixRect.TopLeft()),
                                         OutputToScreenPixel(aPixRect.BottomRight()));

            if ( rHEvt.GetMode() & HelpEventMode::BALLOON )
                Help::ShowBalloon(this,rHEvt.GetMousePosPixel(), aScreenRect, aHelpText);
            else if ( rHEvt.GetMode() & HelpEventMode::QUICK )
                Help::ShowQuickHelp(this,aScreenRect, aHelpText);

            bDone = true;
        }
    }

    // basic controls

    if ( pDrView && bHelpEnabled && !bDone )
    {
        SdrPageView* pPV = pDrView->GetSdrPageView();
        OSL_ENSURE( pPV, "SdrPageView* is NULL" );
        if (pPV)
            bDone = FmFormPage::RequestHelp( this, pDrView, rHEvt );
    }

    // If QuickHelp for AutoFill is shown, do not allow it to be removed

    if ( nMouseStatus == SC_GM_TABDOWN && mrViewData.GetRefType() == SC_REFTYPE_FILL &&
            Help::IsQuickHelpEnabled() )
        bDone = true;

    if (!bDone)
        Window::RequestHelp( rHEvt );
}

bool ScGridWindow::IsMyModel(const SdrEditView* pSdrView)
{
    return pSdrView &&
            &pSdrView->GetModel() == mrViewData.GetDocument().GetDrawLayer();
}

void ScGridWindow::HideNoteOverlay()
{
    mpNoteOverlay.reset();
}

css::uno::Reference< css::accessibility::XAccessible >
    ScGridWindow::CreateAccessible()
{
    rtl::Reference<ScAccessibleDocument> pAccessibleDocument =
        new ScAccessibleDocument(GetAccessibleParent(),
            mrViewData.GetViewShell(), eWhich);
    pAccessibleDocument->PreInit();

    SetAccessible(pAccessibleDocument);

    pAccessibleDocument->Init();

    return pAccessibleDocument;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
