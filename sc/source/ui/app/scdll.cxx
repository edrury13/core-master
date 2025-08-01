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

#include <config_features.h>

#include <svx/fmobjfac.hxx>
#include <svx/objfac3d.hxx>

#include <comphelper/lok.hxx>
#include <sfx2/sidebar/SidebarChildWindow.hxx>
#include <sfx2/app.hxx>
#include <sfx2/devtools/DevelopmentToolChildWindow.hxx>
#include <avmedia/mediatoolbox.hxx>
#include <NumberFormatControl.hxx>

#include <unotools/resmgr.hxx>

#include <scmod.hxx>
#include <scresid.hxx>
#include <sc.hrc>

#include <docsh.hxx>
#include <tabvwsh.hxx>
#include <prevwsh.hxx>
#include <drawsh.hxx>
#include <drformsh.hxx>
#include <drtxtob.hxx>
#include <editsh.hxx>
#include <pivotsh.hxx>
#include <auditsh.hxx>
#include <cellsh.hxx>
#include <oleobjsh.hxx>
#include <chartsh.hxx>
#include <graphsh.hxx>
#include <mediash.hxx>
#include <pgbrksh.hxx>
#include <scdll.hxx>
#include <SparklineShell.hxx>

#include <appoptio.hxx>
#include <searchresults.hxx>

// Controls

#include <svx/tbxctl.hxx>
#include <svx/fillctrl.hxx>
#include <svx/linectrl.hxx>
//#include <svx/tbcontrl.hxx>
#include <svx/selctrl.hxx>
#include <svx/insctrl.hxx>
#include <svx/zoomctrl.hxx>
#include <svx/modctrl.hxx>
#include <svx/pszctrl.hxx>
#include <svx/grafctrl.hxx>
#include <timerctrl.hxx>
#include <svx/clipboardctl.hxx>
#include <svx/formatpaintbrushctrl.hxx>
#include <tbzoomsliderctrl.hxx>
#include <svx/zoomsliderctrl.hxx>

#include <svx/xmlsecctrl.hxx>
// Child windows
#include <reffact.hxx>
#include <navipi.hxx>
#include <inputwin.hxx>
#include <spelldialog.hxx>
#include <svx/fontwork.hxx>
#include <svx/srchdlg.hxx>
#include <svx/hyperdlg.hxx>
#include <svx/imapdlg.hxx>

#include <filter.hxx>
#include <scabstdlg.hxx>
#include <acctrl.hxx>

OUString ScResId(TranslateId aId)
{
    return Translate::get(aId, ScModule::get()->GetResLocale());
}

OUString ScResId(TranslateNId aContextSingularPlural, int nCardinality)
{
    return Translate::nget(aContextSingularPlural, nCardinality, ScModule::get()->GetResLocale());
}

void ScDLL::Init()
{
    if ( SfxApplication::GetModule(SfxToolsModule::Calc) )    // Module already active
        return;

    auto pUniqueModule = std::make_unique<ScModule>(&ScDocShell::Factory());
    ScModule* pMod = pUniqueModule.get();
    SfxApplication::SetModule(SfxToolsModule::Calc, std::move(pUniqueModule));

    ScDocShell::Factory().SetDocumentServiceName( u"com.sun.star.sheet.SpreadsheetDocument"_ustr );

    // Not until the ResManager is initialized
    // The AppOptions must be initialized not until after ScGlobal::Init
    ScGlobal::Init();

    // register your view-factories here
    ScTabViewShell      ::RegisterFactory(SFX_INTERFACE_SFXAPP);
    ScPreviewShell      ::RegisterFactory(SFX_INTERFACE_SFXDOCSH);

    // register your shell-interfaces here
    ScModule            ::RegisterInterface(pMod);
    ScDocShell          ::RegisterInterface(pMod);
    ScTabViewShell      ::RegisterInterface(pMod);
    ScPreviewShell      ::RegisterInterface(pMod);
    ScDrawShell         ::RegisterInterface(pMod);
    ScDrawFormShell     ::RegisterInterface(pMod);
    ScDrawTextObjectBar ::RegisterInterface(pMod);
    ScEditShell         ::RegisterInterface(pMod);
    ScPivotShell        ::RegisterInterface(pMod);
    sc::SparklineShell  ::RegisterInterface(pMod);
    ScAuditingShell     ::RegisterInterface(pMod);
    ScFormatShell       ::RegisterInterface(pMod);
    ScCellShell         ::RegisterInterface(pMod);
    ScOleObjectShell    ::RegisterInterface(pMod);
    ScChartShell        ::RegisterInterface(pMod);
    ScGraphicShell      ::RegisterInterface(pMod);
    ScMediaShell        ::RegisterInterface(pMod);
    ScPageBreakShell    ::RegisterInterface(pMod);

    // Own Controller
    ScZoomSliderControl             ::RegisterControl(SID_PREVIEW_SCALINGFACTOR, pMod);

    // SvxToolboxController
    SvxTbxCtlDraw                   ::RegisterControl(SID_INSERT_DRAW,          pMod);
    SvxFillToolBoxControl           ::RegisterControl(0, pMod);
    SvxLineWidthToolBoxControl      ::RegisterControl(0, pMod);
    SvxClipBoardControl             ::RegisterControl(SID_PASTE,                pMod );
    SvxClipBoardControl             ::RegisterControl(SID_PASTE_UNFORMATTED,    pMod );
    svx::FormatPaintBrushToolBoxControl::RegisterControl(SID_FORMATPAINTBRUSH,  pMod );
    sc::ScNumberFormatControl       ::RegisterControl(SID_NUMBER_TYPE_FORMAT,   pMod );

    SvxGrafModeToolBoxControl       ::RegisterControl(SID_ATTR_GRAF_MODE,       pMod);
    SvxGrafRedToolBoxControl        ::RegisterControl(SID_ATTR_GRAF_RED,        pMod);
    SvxGrafGreenToolBoxControl      ::RegisterControl(SID_ATTR_GRAF_GREEN,      pMod);
    SvxGrafBlueToolBoxControl       ::RegisterControl(SID_ATTR_GRAF_BLUE,       pMod);
    SvxGrafLuminanceToolBoxControl  ::RegisterControl(SID_ATTR_GRAF_LUMINANCE,  pMod);
    SvxGrafContrastToolBoxControl   ::RegisterControl(SID_ATTR_GRAF_CONTRAST,   pMod);
    SvxGrafGammaToolBoxControl      ::RegisterControl(SID_ATTR_GRAF_GAMMA,      pMod);
    SvxGrafTransparenceToolBoxControl::RegisterControl(SID_ATTR_GRAF_TRANSPARENCE, pMod);

    // Media Controller
#if HAVE_FEATURE_AVMEDIA
    ::avmedia::MediaToolBoxControl::RegisterControl( SID_AVMEDIA_TOOLBOX, pMod );
#endif

    // Common SFX Controller
    sfx2::sidebar::SidebarChildWindow::RegisterChildWindow(false, pMod);
    DevelopmentToolChildWindow::RegisterChildWindow(false, pMod);

    // SvxStatusBar Controller
    SvxInsertStatusBarControl       ::RegisterControl(SID_ATTR_INSERT,      pMod);
    SvxSelectionModeControl         ::RegisterControl(SID_STATUS_SELMODE,   pMod);
    SvxZoomStatusBarControl         ::RegisterControl(SID_ATTR_ZOOM,        pMod);
    SvxZoomSliderControl            ::RegisterControl(SID_ATTR_ZOOMSLIDER,  pMod);
    SvxModifyControl                ::RegisterControl(SID_DOC_MODIFIED,     pMod);
    XmlSecStatusBarControl          ::RegisterControl( SID_SIGNATURE,       pMod );
    ScTimerStatusBarControl         ::RegisterControl( SID_DOC_TIMER,       pMod );
    ScAutoCalculateControl          ::RegisterControl(FID_AUTO_CALC,        pMod);
    // ScTimerStatusBarControl         ::RegisterControl(SID_DOC_TIMER,        pMod);

    SvxPosSizeStatusBarControl      ::RegisterControl(SID_ATTR_SIZE,        pMod);

    // Child Windows

    ScInputWindowWrapper        ::RegisterChildWindow(true, pMod, SfxChildWindowFlags::TASK|SfxChildWindowFlags::FORCEDOCK);
    ScSolverDlgWrapper          ::RegisterChildWindow(false, pMod);
    ScOptSolverDlgWrapper       ::RegisterChildWindow(false, pMod);
    ScXMLSourceDlgWrapper       ::RegisterChildWindow(false, pMod);
    ScNameDlgWrapper            ::RegisterChildWindow(false, pMod);
    ScNameDefDlgWrapper         ::RegisterChildWindow(false, pMod);
    ScPivotLayoutWrapper        ::RegisterChildWindow(false, pMod);
    ScTabOpDlgWrapper           ::RegisterChildWindow(false, pMod);
    ScFilterDlgWrapper          ::RegisterChildWindow(false, pMod);
    ScSpecialFilterDlgWrapper   ::RegisterChildWindow(false, pMod);
    ScDbNameDlgWrapper          ::RegisterChildWindow(false, pMod);
    ScConsolidateDlgWrapper     ::RegisterChildWindow(false, pMod);
    ScPrintAreasDlgWrapper      ::RegisterChildWindow(false, pMod);
    ScColRowNameRangesDlgWrapper::RegisterChildWindow(false, pMod);
    ScFormulaDlgWrapper         ::RegisterChildWindow(false, pMod);

    ScRandomNumberGeneratorDialogWrapper::RegisterChildWindow(false, pMod);
    ScSamplingDialogWrapper             ::RegisterChildWindow(false, pMod);
    ScDescriptiveStatisticsDialogWrapper::RegisterChildWindow(false, pMod);
    ScAnalysisOfVarianceDialogWrapper   ::RegisterChildWindow(false, pMod);
    ScCorrelationDialogWrapper          ::RegisterChildWindow(false, pMod);
    ScCovarianceDialogWrapper           ::RegisterChildWindow(false, pMod);
    ScExponentialSmoothingDialogWrapper ::RegisterChildWindow(false, pMod);
    ScMovingAverageDialogWrapper        ::RegisterChildWindow(false, pMod);
    ScRegressionDialogWrapper           ::RegisterChildWindow(false, pMod);
    ScTTestDialogWrapper                ::RegisterChildWindow(false, pMod);
    ScFTestDialogWrapper                ::RegisterChildWindow(false, pMod);
    ScZTestDialogWrapper                ::RegisterChildWindow(false, pMod);
    ScChiSquareTestDialogWrapper        ::RegisterChildWindow(false, pMod);
    ScFourierAnalysisDialogWrapper      ::RegisterChildWindow(false, pMod);
    sc::SparklineDialogWrapper          ::RegisterChildWindow(false, pMod);
    sc::SparklineDataRangeDialogWrapper ::RegisterChildWindow(false, pMod);
    sc::ConditionalFormatEasyDialogWrapper          ::RegisterChildWindow(false, pMod);

    // Redlining Window
    ScAcceptChgDlgWrapper       ::RegisterChildWindow(false, pMod);
    ScSimpleRefDlgWrapper       ::RegisterChildWindow(false, pMod, SfxChildWindowFlags::ALWAYSAVAILABLE|SfxChildWindowFlags::NEVERHIDE );
    ScHighlightChgDlgWrapper    ::RegisterChildWindow(false, pMod);

    SvxSearchDialogWrapper      ::RegisterChildWindow(false, pMod);
    SvxHlinkDlgWrapper          ::RegisterChildWindow(false, pMod);
    SvxFontWorkChildWindow      ::RegisterChildWindow(false, pMod);
    SvxIMapDlgChildWindow       ::RegisterChildWindow(false, pMod);
    ScSpellDialogChildWindow    ::RegisterChildWindow(false, pMod);

    ScValidityRefChildWin::RegisterChildWindow(false, pMod);
    sc::SearchResultsDlgWrapper::RegisterChildWindow(false, pMod);
    ScCondFormatDlgWrapper::RegisterChildWindow(false, pMod);

    ScNavigatorWrapper::RegisterChildWindow(false, pMod, SfxChildWindowFlags::NEVERHIDE);

    // Add 3DObject Factory
    E3dObjFactory();

    // Add css::form::component::FormObject Factory
    FmFormObjFactory();

    pMod->PutItem( SfxUInt16Item( SID_ATTR_METRIC, sal::static_int_cast<sal_uInt16>(pMod->GetAppOptions().GetAppMetric()) ) );

    //  StarOne Services are now handled in the registry
}

#ifndef DISABLE_DYNLOADING

extern "C" SAL_DLLPUBLIC_EXPORT
void lok_preload_hook()
{
    // scfilt
    ScFormatFilter::Get();
    // scui
    ScAbstractDialogFactory::Create();
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
