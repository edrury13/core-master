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

#include <sal/config.h>

#include <cstddef>

#include <dlg_ObjectProperties.hxx>
#include "tp_AxisLabel.hxx"
#include "tp_DataLabel.hxx"
#include "tp_LegendPosition.hxx"
#include "tp_PointGeometry.hxx"
#include "tp_Scale.hxx"
#include "tp_AxisPositions.hxx"
#include "tp_ErrorBars.hxx"
#include "tp_Trendline.hxx"
#include "tp_SeriesToAxis.hxx"
#include "tp_TitleRotation.hxx"
#include "tp_PolarOptions.hxx"
#include "tp_DataPointOption.hxx"
#include "tp_DataTable.hxx"
#include "tp_ChartColorPalette.hxx"
#include <ViewElementListProvider.hxx>
#include <ChartType.hxx>
#include <ChartTypeHelper.hxx>
#include <ObjectNameProvider.hxx>
#include <DataSeries.hxx>
#include <DiagramHelper.hxx>
#include <Diagram.hxx>
#include <NumberFormatterWrapper.hxx>
#include <Axis.hxx>
#include <AxisHelper.hxx>
#include <ExplicitCategoriesProvider.hxx>
#include <ChartModel.hxx>
#include <CommonConverters.hxx>
#include <RegressionCalculationHelper.hxx>
#include <BaseCoordinateSystem.hxx>

#include <com/sun/star/chart2/AxisType.hpp>
#include <com/sun/star/chart2/XAxis.hpp>
#include <svl/intitem.hxx>
#include <svl/ctloptions.hxx>

#include <svx/svxids.hrc>

#include <svx/drawitem.hxx>
#include <svx/ofaitem.hxx>
#include <svx/svxgraphicitem.hxx>

#include <svx/dialogs.hrc>
#include <editeng/flstitem.hxx>

#include <svx/flagsdef.hxx>
#include <svx/numinf.hxx>

#include <svl/cjkoptions.hxx>
#include <utility>
#include <comphelper/diagnose_ex.hxx>

#include <vcl/tabs.hrc>

namespace chart
{

using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Exception;
using ::com::sun::star::beans::XPropertySet;

ObjectPropertiesDialogParameter::ObjectPropertiesDialogParameter( OUString aObjectCID )
        : m_aObjectCID(std::move( aObjectCID ))
        , m_eObjectType( ObjectIdentifier::getObjectType( m_aObjectCID ) )
        , m_bAffectsMultipleObjects(false)
        , m_bHasGeometryProperties(false)
        , m_bHasStatisticProperties(false)
        , m_bProvidesSecondaryYAxis(false)
        , m_bProvidesOverlapAndGapWidth(false)
        , m_bProvidesBarConnectors(false)
        , m_bHasAreaProperties(false)
        , m_bHasSymbolProperties(false)
        , m_bHasNumberProperties(false)
        , m_bProvidesStartingAngle(false)
        , m_bProvidesMissingValueTreatments(false)
        , m_bIsPieChartDataPoint(false)
        , m_bHasScaleProperties(false)
        , m_bCanAxisLabelsBeStaggered(false)
        , m_bSupportingAxisPositioning(false)
        , m_bShowAxisOrigin(false)
        , m_bIsCrossingAxisIsCategoryAxis(false)
        , m_bSupportingCategoryPositioning(false)
        , m_bComplexCategoriesAxis( false )
        , m_nNbPoints( 0 )
{
    std::u16string_view aParticleID = ObjectIdentifier::getParticleID( m_aObjectCID );
    m_bAffectsMultipleObjects = (aParticleID == u"ALLELEMENTS");
}
ObjectPropertiesDialogParameter::~ObjectPropertiesDialogParameter()
{
}

void ObjectPropertiesDialogParameter::init( const rtl::Reference<::chart::ChartModel>& xChartModel )
{
    m_xChartDocument = xChartModel;
    rtl::Reference< Diagram > xDiagram = xChartModel->getFirstChartDiagram();
    rtl::Reference< DataSeries > xSeries = ObjectIdentifier::getDataSeriesForCID( m_aObjectCID, xChartModel );
    rtl::Reference< ChartType > xChartType = xChartModel->getChartTypeOfSeries( xSeries );
    sal_Int32 nDimensionCount = 0;
    if (xDiagram)
        nDimensionCount = xDiagram->getDimension();

    bool bHasSeriesProperties = (m_eObjectType==OBJECTTYPE_DATA_SERIES);
    bool bHasDataPointproperties = (m_eObjectType==OBJECTTYPE_DATA_POINT);

    if( bHasSeriesProperties || bHasDataPointproperties )
    {
        m_bHasGeometryProperties = xChartType->isSupportingGeometryProperties(nDimensionCount );
        m_bHasAreaProperties     = xChartType->isSupportingAreaProperties(nDimensionCount);
        m_bHasSymbolProperties   = xChartType->isSupportingSymbolProperties(nDimensionCount);
        m_bIsPieChartDataPoint   = bHasDataPointproperties && xChartType->isSupportingStartingAngle();

        if( bHasSeriesProperties )
        {
            m_bHasStatisticProperties =  xChartType->isSupportingStatisticProperties(nDimensionCount);
            m_bProvidesSecondaryYAxis =  xChartType->isSupportingSecondaryAxis(nDimensionCount);
            m_bProvidesOverlapAndGapWidth =  xChartType->isSupportingOverlapAndGapWidthProperties(nDimensionCount);
            m_bProvidesBarConnectors =  xChartType->isSupportingBarConnectors(nDimensionCount);
            m_bProvidesStartingAngle = xChartType->isSupportingStartingAngle();

            m_bProvidesMissingValueTreatments = ChartTypeHelper::getSupportedMissingValueTreatments( xChartType )
                                            .hasElements();
        }
    }

    if( m_eObjectType == OBJECTTYPE_DATA_ERRORS_X ||
        m_eObjectType == OBJECTTYPE_DATA_ERRORS_Y ||
        m_eObjectType == OBJECTTYPE_DATA_ERRORS_Z)
        m_bHasStatisticProperties = true;

    if( m_eObjectType == OBJECTTYPE_AXIS )
    {
        //show scale properties only for a single axis not for multiselection
        m_bHasScaleProperties = !m_bAffectsMultipleObjects;

        if( m_bHasScaleProperties )
        {
            rtl::Reference< Axis > xAxis = ObjectIdentifier::getAxisForCID( m_aObjectCID, xChartModel );
            if( xAxis.is() )
            {
                //no scale page for series axis
                ScaleData aData( xAxis->getScaleData() );
                if( aData.AxisType == chart2::AxisType::SERIES )
                    m_bHasScaleProperties = false;
                if( aData.AxisType != chart2::AxisType::SERIES )
                    m_bHasNumberProperties = true;

                //is the crossing main axis a category axes?:
                rtl::Reference< BaseCoordinateSystem > xCooSys( AxisHelper::getCoordinateSystemOfAxis( xAxis, xDiagram ) );
                rtl::Reference< Axis > xCrossingMainAxis( AxisHelper::getCrossingMainAxis( xAxis, xCooSys ) );
                if( xCrossingMainAxis.is() )
                {
                    ScaleData aScale( xCrossingMainAxis->getScaleData() );
                    m_bIsCrossingAxisIsCategoryAxis = ( aScale.AxisType == chart2::AxisType::CATEGORY  );
                    if( m_bIsCrossingAxisIsCategoryAxis )
                    {
                        if (xChartModel)
                            m_aCategories = DiagramHelper::getExplicitSimpleCategories( *xChartModel );
                    }
                }

                sal_Int32 nCooSysIndex=0;
                sal_Int32 nDimensionIndex=0;
                sal_Int32 nAxisIndex=0;
                if( AxisHelper::getIndicesForAxis( xAxis, xDiagram, nCooSysIndex, nDimensionIndex, nAxisIndex ) )
                {
                    xChartType = AxisHelper::getFirstChartTypeWithSeriesAttachedToAxisIndex( xDiagram, nAxisIndex );
                    //show positioning controls only if they make sense
                    m_bSupportingAxisPositioning = xChartType->isSupportingAxisPositioning(nDimensionCount, nDimensionIndex);

                    //show axis origin only for secondary y axis
                    if( nDimensionIndex==1 && nAxisIndex==1 && xChartType->isSupportingBaseValue())
                        m_bShowAxisOrigin = true;

                    if ( nDimensionIndex == 0 && ( aData.AxisType == chart2::AxisType::CATEGORY || aData.AxisType == chart2::AxisType::DATE ) )
                    {
                        if (xChartModel)
                        {
                            ExplicitCategoriesProvider aExplicitCategoriesProvider( xCooSys, *xChartModel );
                            m_bComplexCategoriesAxis = aExplicitCategoriesProvider.hasComplexCategories();
                        }

                        if (!m_bComplexCategoriesAxis)
                            m_bSupportingCategoryPositioning = xChartType->isSupportingCategoryPositioning(nDimensionCount);
                    }
                }
            }
        }

        //no staggering of labels for 3D axis
        m_bCanAxisLabelsBeStaggered = nDimensionCount==2;
    }

    if( m_eObjectType == OBJECTTYPE_DATA_CURVE )
    {
        const std::vector< uno::Reference< chart2::data::XLabeledDataSequence > > & aDataSeqs( xSeries->getDataSequences2());
        Sequence< double > aXValues, aYValues;
        bool bXValuesFound = false, bYValuesFound = false;
        m_nNbPoints = 0;
        for( std::size_t i=0;
             ! (bXValuesFound && bYValuesFound) && i<aDataSeqs.size();
             ++i )
        {
            try
            {
                Reference< data::XDataSequence > xSeq( aDataSeqs[i]->getValues());
                Reference< XPropertySet > xProp( xSeq, uno::UNO_QUERY_THROW );
                OUString aRole;
                if( xProp->getPropertyValue( u"Role"_ustr ) >>= aRole )
                {
                    if( !bXValuesFound && aRole == "values-x" )
                    {
                        aXValues = DataSequenceToDoubleSequence( xSeq );
                        bXValuesFound = true;
                    }
                    else if( !bYValuesFound && aRole == "values-y" )
                    {
                        aYValues = DataSequenceToDoubleSequence( xSeq );
                        bYValuesFound = true;
                    }
                }
            }
            catch( const Exception & )
            {
                DBG_UNHANDLED_EXCEPTION("chart2");
            }
        }
        if( !bXValuesFound && bYValuesFound )
        {
            // initialize with 1, 2, ...
            //first category (index 0) matches with real number 1.0
            aXValues.realloc( aYValues.getLength() );
            auto pXValues = aXValues.getArray();
            for( sal_Int32 i=0; i<aXValues.getLength(); ++i )
                pXValues[i] = i+1;
            bXValuesFound = true;
        }

        if( bXValuesFound && bYValuesFound &&
            aXValues.hasElements() &&
            aYValues.hasElements() )
        {
            RegressionCalculationHelper::tDoubleVectorPair aValues(
                RegressionCalculationHelper::cleanup( aXValues, aYValues, RegressionCalculationHelper::isValid()));
            m_nNbPoints = aValues.second.size();
        }
    }

     //create gui name for this object
    if( !m_bAffectsMultipleObjects && m_eObjectType == OBJECTTYPE_AXIS )
    {
        m_aLocalizedName = ObjectNameProvider::getAxisName( m_aObjectCID, xChartModel );
    }
    else if( !m_bAffectsMultipleObjects && ( m_eObjectType == OBJECTTYPE_GRID || m_eObjectType == OBJECTTYPE_SUBGRID ) )
    {
        m_aLocalizedName = ObjectNameProvider::getGridName( m_aObjectCID, xChartModel );
    }
    else if( !m_bAffectsMultipleObjects && m_eObjectType == OBJECTTYPE_TITLE )
    {
        m_aLocalizedName = ObjectNameProvider::getTitleName( m_aObjectCID, xChartModel );
    }
    else
    {
        switch( m_eObjectType )
        {
            case OBJECTTYPE_DATA_POINT:
            case OBJECTTYPE_DATA_LABEL:
            case OBJECTTYPE_DATA_LABELS:
            case OBJECTTYPE_DATA_ERRORS_X:
            case OBJECTTYPE_DATA_ERRORS_Y:
            case OBJECTTYPE_DATA_ERRORS_Z:
            case OBJECTTYPE_DATA_AVERAGE_LINE:
            case OBJECTTYPE_DATA_CURVE:
            case OBJECTTYPE_DATA_CURVE_EQUATION:
                if( m_bAffectsMultipleObjects )
                    m_aLocalizedName = ObjectNameProvider::getName_ObjectForAllSeries( m_eObjectType );
                else
                    m_aLocalizedName = ObjectNameProvider::getName_ObjectForSeries( m_eObjectType, m_aObjectCID, m_xChartDocument );
                break;
            default:
                m_aLocalizedName = ObjectNameProvider::getName(m_eObjectType,m_bAffectsMultipleObjects);
                break;
        }
    }
}

const sal_uInt16 nNoArrowNoShadowDlg    = 1101;

void SchAttribTabDlg::setSymbolInformation( SfxItemSet&& rSymbolShapeProperties,
                std::optional<Graphic> oAutoSymbolGraphic )
{
    m_oSymbolShapeProperties.emplace(std::move(rSymbolShapeProperties));
    m_oAutoSymbolGraphic = std::move(oAutoSymbolGraphic);
}

void SchAttribTabDlg::SetAxisMinorStepWidthForErrorBarDecimals( double fMinorStepWidth )
{
    m_fAxisMinorStepWidthForErrorBarDecimals = fMinorStepWidth;
}

SchAttribTabDlg::SchAttribTabDlg(weld::Window* pParent,
                                 const SfxItemSet* pAttr,
                                 const ObjectPropertiesDialogParameter& rDialogParameter,
                                 const ViewElementListProvider* pViewElementListProvider,
                                 const uno::Reference< util::XNumberFormatsSupplier >& xNumberFormatsSupplier)
    : SfxTabDialogController(pParent, u"modules/schart/ui/attributedialog.ui"_ustr, u"AttributeDialog"_ustr, pAttr)
    , m_rParameter(rDialogParameter)
    , m_pViewElementListProvider( pViewElementListProvider )
    , m_pNumberFormatter(nullptr)
    , m_fAxisMinorStepWidthForErrorBarDecimals(0.1)
    , m_bOKPressed(false)
{
    NumberFormatterWrapper aNumberFormatterWrapper( xNumberFormatsSupplier );
    m_pNumberFormatter = aNumberFormatterWrapper.getSvNumberFormatter();

    m_xDialog->set_title(rDialogParameter.getLocalizedName());

    ObjectType eType = rDialogParameter.getObjectType();
    switch (eType)
    {
        case OBJECTTYPE_TITLE:
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_BORDER.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_BORDER.sIconName);
            AddTabPage(u"area"_ustr, TabResId(RID_TAB_AREA.aLabel), RID_SVXPAGE_AREA,
                       RID_M + RID_TAB_AREA.sIconName);
            AddTabPage(u"transparent"_ustr, TabResId(RID_TAB_TRANSPARENCE.aLabel), RID_SVXPAGE_TRANSPARENCE,
                       RID_M + RID_TAB_TRANSPARENCE.sIconName);
            AddTabPage(u"fontname"_ustr, TabResId(RID_TAB_FONT.aLabel), RID_SVXPAGE_CHAR_NAME,
                       RID_M + RID_TAB_FONT.sIconName);
            AddTabPage(u"effects"_ustr, TabResId(RID_TAB_FONTEFFECTS.aLabel), RID_SVXPAGE_CHAR_EFFECTS,
                       RID_M + RID_TAB_FONTEFFECTS.sIconName);
            AddTabPage(u"alignment"_ustr, TabResId(RID_TAB_ALIGNMENT.aLabel), SchAlignmentTabPage::Create,
                       RID_M + RID_TAB_ALIGNMENT.sIconName);
            if( SvtCJKOptions::IsAsianTypographyEnabled() )
                AddTabPage(u"asian"_ustr, TabResId(RID_TAB_ASIANTYPO.aLabel), RID_SVXPAGE_PARA_ASIAN,
                           RID_M + RID_TAB_ASIANTYPO.sIconName);
            break;

        case OBJECTTYPE_LEGEND:
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_BORDER.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_BORDER.sIconName);
            AddTabPage(u"area"_ustr, TabResId(RID_TAB_AREA.aLabel), RID_SVXPAGE_AREA,
                       RID_M + RID_TAB_AREA.sIconName);
            AddTabPage(u"transparent"_ustr, TabResId(RID_TAB_TRANSPARENCE.aLabel), RID_SVXPAGE_TRANSPARENCE,
                       RID_M + RID_TAB_TRANSPARENCE.sIconName);
            AddTabPage(u"colorpalette"_ustr, TabResId(RID_TAB_COLORPALETTE.aLabel), ChartColorPaletteTabPage::Create,
                       RID_M + RID_TAB_COLORPALETTE.sIconName);
            AddTabPage(u"fontname"_ustr, TabResId(RID_TAB_FONT.aLabel), RID_SVXPAGE_CHAR_NAME,
                       RID_M + RID_TAB_FONT.sIconName);
            AddTabPage(u"effects"_ustr, TabResId(RID_TAB_FONTEFFECTS.aLabel), RID_SVXPAGE_CHAR_EFFECTS,
                       RID_M + RID_TAB_FONTEFFECTS.sIconName);
            AddTabPage(u"legendpos"_ustr, TabResId(RID_TAB_CHART_LEGENDPOS.aLabel), SchLegendPosTabPage::Create,
                       RID_M + RID_TAB_CHART_LEGENDPOS.sIconName);
            if (SvtCJKOptions::IsAsianTypographyEnabled())
                AddTabPage(u"asian"_ustr, TabResId(RID_TAB_ASIANTYPO.aLabel), RID_SVXPAGE_PARA_ASIAN,
                           RID_M + RID_TAB_ASIANTYPO.sIconName);
            break;

        case OBJECTTYPE_DATA_SERIES:
        case OBJECTTYPE_DATA_POINT:
            if (m_rParameter.ProvidesSecondaryYAxis() || m_rParameter.ProvidesOverlapAndGapWidth() || m_rParameter.ProvidesMissingValueTreatments())
                AddTabPage(u"options"_ustr, TabResId(RID_TAB_CHART_OPTIONS.aLabel), SchOptionTabPage::Create,
                           RID_M + RID_TAB_CHART_OPTIONS.sIconName);
            if (m_rParameter.ProvidesStartingAngle())
                AddTabPage(u"polaroptions"_ustr, TabResId(RID_TAB_CHART_OPTIONS.aLabel), PolarOptionsTabPage::Create,
                           RID_M + RID_TAB_CHART_OPTIONS.sIconName);
            if (m_rParameter.IsPieChartDataPoint())
                AddTabPage(u"datapointoption"_ustr, TabResId(RID_TAB_CHART_OPTIONS.aLabel), DataPointOptionTabPage::Create,
                           RID_M + RID_TAB_CHART_OPTIONS.sIconName);

            if (m_rParameter.HasGeometryProperties())
                AddTabPage(u"layout"_ustr, TabResId(RID_TAB_CHART_LAYOUT.aLabel), SchLayoutTabPage::Create,
                           RID_M + RID_TAB_CHART_LAYOUT.sIconName);

            if (m_rParameter.HasAreaProperties())
            {
                AddTabPage(u"area"_ustr, TabResId(RID_TAB_AREA.aLabel), RID_SVXPAGE_AREA,
                           RID_M + RID_TAB_AREA.sIconName);
                AddTabPage(u"transparent"_ustr, TabResId(RID_TAB_TRANSPARENCE.aLabel), RID_SVXPAGE_TRANSPARENCE,
                           RID_M + RID_TAB_TRANSPARENCE.sIconName);
                AddTabPage(u"border"_ustr, TabResId(RID_TAB_BORDER.aLabel), RID_SVXPAGE_LINE,
                        RID_M + RID_TAB_BORDER.sIconName);
            }
            else
                AddTabPage(u"border"_ustr, TabResId(RID_TAB_LINE.aLabel), RID_SVXPAGE_LINE,
                           RID_M + RID_TAB_LINE.sIconName);

            AddTabPage(u"colorpalette"_ustr, TabResId(RID_TAB_COLORPALETTE.aLabel), ChartColorPaletteTabPage::Create,
                       RID_M + RID_TAB_COLORPALETTE.sIconName);
            break;

        case OBJECTTYPE_DATA_LABEL:
        case OBJECTTYPE_DATA_LABELS:
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_BORDER.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_BORDER.sIconName);
            AddTabPage(u"datalabels"_ustr, TabResId(RID_TAB_CHART_DATALABEL.aLabel), DataLabelsTabPage::Create,
                       RID_M + RID_TAB_CHART_DATALABEL.sIconName);
            AddTabPage(u"fontname"_ustr, TabResId(RID_TAB_FONT.aLabel), RID_SVXPAGE_CHAR_NAME,
                       RID_M + RID_TAB_FONT.sIconName);
            AddTabPage(u"effects"_ustr, TabResId(RID_TAB_FONTEFFECTS.aLabel), RID_SVXPAGE_CHAR_EFFECTS,
                       RID_M + RID_TAB_FONTEFFECTS.sIconName);
            if( SvtCJKOptions::IsAsianTypographyEnabled() )
                AddTabPage(u"asian"_ustr, TabResId(RID_TAB_ASIANTYPO.aLabel), RID_SVXPAGE_PARA_ASIAN,
                           RID_M + RID_TAB_ASIANTYPO.sIconName);

            break;

        case OBJECTTYPE_AXIS:
        {
            if (m_rParameter.HasScaleProperties())
            {
                AddTabPage(u"scale"_ustr, TabResId(RID_TAB_CHART_SCALE.aLabel), ScaleTabPage::Create,
                           RID_M + RID_TAB_CHART_SCALE.sIconName);
                //no positioning page for z axes so far as the tickmarks are not shown so far
                AddTabPage(u"axispos"_ustr, TabResId(RID_TAB_CHART_POSITIONING.aLabel), AxisPositionsTabPage::Create,
                           RID_M + RID_TAB_CHART_POSITIONING.sIconName);
            }
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_BORDER.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_BORDER.sIconName);
            AddTabPage(u"axislabel"_ustr, TabResId(RID_TAB_CHART_AXISLABEL.aLabel), SchAxisLabelTabPage::Create,
                       RID_M + RID_TAB_CHART_AXISLABEL.sIconName);
            if (m_rParameter.HasNumberProperties())
                AddTabPage(u"numberformat"_ustr, TabResId(RID_TAB_NUMBERS.aLabel), RID_SVXPAGE_NUMBERFORMAT,
                           RID_M + RID_TAB_NUMBERS.sIconName);
            AddTabPage(u"fontname"_ustr, TabResId(RID_TAB_FONT.aLabel), RID_SVXPAGE_CHAR_NAME,
                       RID_M + RID_TAB_FONT.sIconName);
            AddTabPage(u"effects"_ustr, TabResId(RID_TAB_FONTEFFECTS.aLabel), RID_SVXPAGE_CHAR_EFFECTS,
                       RID_M + RID_TAB_FONTEFFECTS.sIconName);
            if( SvtCJKOptions::IsAsianTypographyEnabled() )
                AddTabPage(u"asian"_ustr, TabResId(RID_TAB_ASIANTYPO.aLabel), RID_SVXPAGE_PARA_ASIAN,
                           RID_M + RID_TAB_ASIANTYPO.sIconName);
            break;
        }

        case OBJECTTYPE_DATA_ERRORS_X:
            AddTabPage(u"xerrorbar"_ustr, TabResId(RID_TAB_CHART_ERROR_X.aLabel), ErrorBarsTabPage::Create,
                       RID_M  + RID_TAB_CHART_ERROR_X.sIconName);
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_LINE.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_LINE.sIconName);
            break;

        case OBJECTTYPE_DATA_ERRORS_Y:
            AddTabPage(u"yerrorbar"_ustr, TabResId(RID_TAB_CHART_ERROR_Y.aLabel), ErrorBarsTabPage::Create,
                       RID_M  + RID_TAB_CHART_ERROR_Y.sIconName);
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_LINE.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_LINE.sIconName);
            break;

        case OBJECTTYPE_DATA_ERRORS_Z:
            break;

        case OBJECTTYPE_GRID:
        case OBJECTTYPE_SUBGRID:
        case OBJECTTYPE_DATA_AVERAGE_LINE:
        case OBJECTTYPE_DATA_STOCK_RANGE:
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_LINE.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_LINE.sIconName);
            break;

        case OBJECTTYPE_DATA_CURVE:
            AddTabPage(u"trendline"_ustr, TabResId(RID_TAB_CHART_TREND.aLabel), TrendlineTabPage::Create,
                       RID_M + RID_TAB_CHART_TREND.sIconName);
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_LINE.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_LINE.sIconName);
            break;

        case OBJECTTYPE_DATA_STOCK_LOSS:
        case OBJECTTYPE_DATA_STOCK_GAIN:
        case OBJECTTYPE_PAGE:
        case OBJECTTYPE_DIAGRAM_FLOOR:
        case OBJECTTYPE_DIAGRAM_WALL:
        case OBJECTTYPE_DIAGRAM:
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_BORDER.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_BORDER.sIconName);
            AddTabPage(u"area"_ustr, TabResId(RID_TAB_AREA.aLabel), RID_SVXPAGE_AREA,
                       RID_M + RID_TAB_AREA.sIconName);
            AddTabPage(u"transparent"_ustr, TabResId(RID_TAB_TRANSPARENCE.aLabel), RID_SVXPAGE_TRANSPARENCE,
                       RID_M + RID_TAB_TRANSPARENCE.sIconName);
            if (eType != OBJECTTYPE_DATA_STOCK_LOSS && eType != OBJECTTYPE_DATA_STOCK_GAIN)
                AddTabPage(u"colorpalette"_ustr, TabResId(RID_TAB_COLORPALETTE.aLabel), ChartColorPaletteTabPage::Create,
                           RID_M + RID_TAB_COLORPALETTE.sIconName);

            break;

        case OBJECTTYPE_LEGEND_ENTRY:
        case OBJECTTYPE_AXIS_UNITLABEL:
        case OBJECTTYPE_UNKNOWN:
            // nothing
            break;
        case OBJECTTYPE_DATA_TABLE:
            AddTabPage(u"datatable"_ustr, TabResId(RID_TAB_CHART_TABLE.aLabel), DataTableTabPage::Create,
                       RID_M + RID_TAB_CHART_TABLE.sIconName);
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_BORDER.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_BORDER.sIconName);
            AddTabPage(u"area"_ustr, TabResId(RID_TAB_AREA.aLabel), RID_SVXPAGE_AREA,
                       RID_M + RID_TAB_AREA.sIconName);
            AddTabPage(u"fontname"_ustr, TabResId(RID_TAB_FONT.aLabel), RID_SVXPAGE_CHAR_NAME,
                       RID_M + RID_TAB_FONT.sIconName);
            AddTabPage(u"effects"_ustr, TabResId(RID_TAB_FONTEFFECTS.aLabel), RID_SVXPAGE_CHAR_EFFECTS,
                       RID_M + RID_TAB_FONTEFFECTS.sIconName);
            break;
        case OBJECTTYPE_DATA_CURVE_EQUATION:
            AddTabPage(u"border"_ustr, TabResId(RID_TAB_BORDER.aLabel), RID_SVXPAGE_LINE,
                       RID_M + RID_TAB_BORDER.sIconName);
            AddTabPage(u"area"_ustr, TabResId(RID_TAB_AREA.aLabel), RID_SVXPAGE_AREA,
                       RID_M + RID_TAB_AREA.sIconName);
            AddTabPage(u"transparent"_ustr, TabResId(RID_TAB_TRANSPARENCE.aLabel), RID_SVXPAGE_TRANSPARENCE,
                       RID_M + RID_TAB_TRANSPARENCE.sIconName);
            AddTabPage(u"fontname"_ustr, TabResId(RID_TAB_FONT.aLabel), RID_SVXPAGE_CHAR_NAME,
                       RID_M + RID_TAB_FONT.sIconName);
            AddTabPage(u"effects"_ustr, TabResId(RID_TAB_FONTEFFECTS.aLabel), RID_SVXPAGE_CHAR_EFFECTS,
                       RID_M + RID_TAB_FONTEFFECTS.sIconName);
            AddTabPage(u"numberformat"_ustr, TabResId(RID_TAB_NUMBERS.aLabel), RID_SVXPAGE_NUMBERFORMAT,
                           RID_M + RID_TAB_NUMBERS.sIconName);
            if (SvtCTLOptions::IsCTLFontEnabled())
            {
                /*  When rotation is supported for equation text boxes, use
                    SchAlignmentTabPage::Create here. The special
                    SchAlignmentTabPage::CreateWithoutRotation can be deleted. */
                AddTabPage(u"alignment"_ustr, TabResId(RID_TAB_ALIGNMENT.aLabel), SchAlignmentTabPage::CreateWithoutRotation,
                           RID_M + RID_TAB_ALIGNMENT.sIconName);
            }
            break;
        default:
            break;
    }

    // used to find out if user left the dialog with OK. When OK is pressed but
    // no changes were done, Cancel is returned by the SfxTabDialog. See method
    // DialogWasClosedWithOK.
    GetOKButton().connect_clicked(LINK(this, SchAttribTabDlg, OKPressed));
}

SchAttribTabDlg::~SchAttribTabDlg()
{
}

void SchAttribTabDlg::PageCreated(const OUString& rId, SfxTabPage &rPage)
{
    SfxAllItemSet aSet(*(GetInputSetImpl()->GetPool()));
    if (rId == "border")
    {
        aSet.Put (SvxColorListItem(m_pViewElementListProvider->GetColorTable(),SID_COLOR_TABLE));
        aSet.Put (SvxDashListItem(m_pViewElementListProvider->GetDashList(),SID_DASH_LIST));
        aSet.Put (SvxLineEndListItem(m_pViewElementListProvider->GetLineEndList(),SID_LINEEND_LIST));
        aSet.Put (SfxUInt16Item(SID_PAGE_TYPE,0));
        aSet.Put (SfxUInt16Item(SID_DLG_TYPE,nNoArrowNoShadowDlg));

        if (m_rParameter.HasSymbolProperties())
        {
            aSet.Put(OfaPtrItem(SID_OBJECT_LIST,m_pViewElementListProvider->GetSymbolList()));
            if( m_oSymbolShapeProperties )
                aSet.Put(SfxTabDialogItem(SID_ATTR_SET, *m_oSymbolShapeProperties));
            if( m_oAutoSymbolGraphic )
                aSet.Put(SvxGraphicItem(*m_oAutoSymbolGraphic));
        }
        rPage.PageCreated(aSet);
    }
    else if (rId == "area")
    {
        aSet.Put(SvxColorListItem(m_pViewElementListProvider->GetColorTable(),SID_COLOR_TABLE));
        aSet.Put(SvxGradientListItem(m_pViewElementListProvider->GetGradientList(),SID_GRADIENT_LIST));
        aSet.Put(SvxHatchListItem(m_pViewElementListProvider->GetHatchList(),SID_HATCH_LIST));
        aSet.Put(SvxBitmapListItem(m_pViewElementListProvider->GetBitmapList(),SID_BITMAP_LIST));
        aSet.Put(SvxPatternListItem(m_pViewElementListProvider->GetPatternList(),SID_PATTERN_LIST));
        aSet.Put(SfxUInt16Item(SID_PAGE_TYPE,0));
        aSet.Put(SfxUInt16Item(SID_DLG_TYPE,nNoArrowNoShadowDlg));
        rPage.PageCreated(aSet);
    }
    else if (rId == "transparent")
    {
        aSet.Put (SfxUInt16Item(SID_PAGE_TYPE,0));
        aSet.Put (SfxUInt16Item(SID_DLG_TYPE,nNoArrowNoShadowDlg));
        rPage.PageCreated(aSet);
    }
    else if (rId == "fontname")
    {
        aSet.Put (SvxFontListItem(m_pViewElementListProvider->getFontList(), SID_ATTR_CHAR_FONTLIST));
        rPage.PageCreated(aSet);
    }
    else if (rId == "effects")
    {
        aSet.Put (SfxUInt16Item(SID_DISABLE_CTL,DISABLE_CASEMAP));
        rPage.PageCreated(aSet);
    }
    else if (rId == "axislabel")
    {
        bool bShowStaggeringControls = m_rParameter.CanAxisLabelsBeStaggered();
        auto & rLabelPage = static_cast<SchAxisLabelTabPage&>(rPage);
        rLabelPage.ShowStaggeringControls( bShowStaggeringControls );
        rLabelPage.SetComplexCategories(m_rParameter.IsComplexCategoriesAxis());
    }
    else if (rId == "axispos")
    {
        AxisPositionsTabPage* pPage = dynamic_cast< AxisPositionsTabPage* >( &rPage );
        if(pPage)
        {
            pPage->SetNumFormatter( m_pNumberFormatter );
            if (m_rParameter.IsCrossingAxisIsCategoryAxis())
            {
                pPage->SetCrossingAxisIsCategoryAxis(m_rParameter.IsCrossingAxisIsCategoryAxis());
                pPage->SetCategories(m_rParameter.GetCategories());
            }
            pPage->SupportAxisPositioning(m_rParameter.IsSupportingAxisPositioning());
            pPage->SupportCategoryPositioning(m_rParameter.IsSupportingCategoryPositioning());
        }
    }
    else if (rId == "scale")
    {
        ScaleTabPage* pScaleTabPage = dynamic_cast< ScaleTabPage* >( &rPage );
        if(pScaleTabPage)
        {
            pScaleTabPage->SetNumFormatter( m_pNumberFormatter );
            pScaleTabPage->ShowAxisOrigin(m_rParameter.ShowAxisOrigin());
        }
    }
    else if (rId == "datalabels")
    {
        DataLabelsTabPage* pLabelPage = dynamic_cast< DataLabelsTabPage* >( &rPage );
        if( pLabelPage )
            pLabelPage->SetNumberFormatter( m_pNumberFormatter );
    }
    else if (rId == "numberformat")
    {
        aSet.Put (SvxNumberInfoItem( m_pNumberFormatter, SID_ATTR_NUMBERFORMAT_INFO));
        rPage.PageCreated(aSet);
    }
    else if (rId == "xerrorbar")
    {
        ErrorBarsTabPage * pTabPage = dynamic_cast< ErrorBarsTabPage * >( &rPage );
        OSL_ASSERT( pTabPage );
        if( pTabPage )
        {
            pTabPage->SetAxisMinorStepWidthForErrorBarDecimals( m_fAxisMinorStepWidthForErrorBarDecimals );
            pTabPage->SetErrorBarType( ErrorBarResources::ERROR_BAR_X );
            pTabPage->SetChartDocumentForRangeChoosing(m_rParameter.getDocument());
        }
    }
    else if (rId == "yerrorbar")
    {
        ErrorBarsTabPage * pTabPage = dynamic_cast< ErrorBarsTabPage * >( &rPage );
        OSL_ASSERT( pTabPage );
        if( pTabPage )
        {
            pTabPage->SetAxisMinorStepWidthForErrorBarDecimals( m_fAxisMinorStepWidthForErrorBarDecimals );
            pTabPage->SetErrorBarType( ErrorBarResources::ERROR_BAR_Y );
            pTabPage->SetChartDocumentForRangeChoosing(m_rParameter.getDocument());
        }
    }
    else if (rId == "options")
    {
        SchOptionTabPage* pTabPage = dynamic_cast< SchOptionTabPage* >( &rPage );
        if (pTabPage)
            pTabPage->Init(m_rParameter.ProvidesSecondaryYAxis(),
                           m_rParameter.ProvidesOverlapAndGapWidth(),
                           m_rParameter.ProvidesBarConnectors());
    }
    else if (rId == "trendline")
    {
        TrendlineTabPage* pTrendlineTabPage = dynamic_cast< TrendlineTabPage* >( &rPage );
        if(pTrendlineTabPage)
        {
            pTrendlineTabPage->SetNumFormatter( m_pNumberFormatter );
            pTrendlineTabPage->SetNbPoints(m_rParameter.getNbPoints());
        }
    }
    else if (rId == "colorpalette")
    {
        auto* pColorPaletteTabPage = dynamic_cast<ChartColorPaletteTabPage*>( &rPage );
        if (pColorPaletteTabPage)
        {
            pColorPaletteTabPage->init(m_rParameter.getDocument());
        }
    }
}

IMPL_LINK(SchAttribTabDlg, OKPressed, weld::Button&, rButton, void)
{
    m_bOKPressed = true;
    OkHdl(rButton);
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
