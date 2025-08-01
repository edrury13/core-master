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

#ifndef INCLUDED_OOX_DRAWINGML_CHART_PLOTAREACONVERTER_HXX
#define INCLUDED_OOX_DRAWINGML_CHART_PLOTAREACONVERTER_HXX

#include <drawingml/chart/converterbase.hxx>
#include <drawingml/chart/seriesmodel.hxx>
#include <oox/drawingml/chart/datasourcemodel.hxx>

namespace com::sun::star {
    namespace chart2 { class XDiagram; }
}

namespace oox::drawingml::chart {


struct View3DModel;
class TypeGroupConverter;

class View3DConverter final : public ConverterBase< View3DModel >
{
public:
    explicit            View3DConverter( const ConverterRoot& rParent, View3DModel& rModel );
    virtual             ~View3DConverter() override;

    /** Converts the OOXML plot area model to a chart2 diagram. */
    void                convertFromModel(
                            const css::uno::Reference< css::chart2::XDiagram >& rxDiagram,
                            TypeGroupConverter const & rTypeGroup );
};


struct WallFloorModel;

class WallFloorConverter final : public ConverterBase< WallFloorModel >
{
public:
    explicit            WallFloorConverter( const ConverterRoot& rParent, WallFloorModel& rModel );
    virtual             ~WallFloorConverter() override;

    /** Converts the OOXML wall/floor model to a chart2 diagram. */
    void                convertFromModel(
                            const css::uno::Reference< css::chart2::XDiagram >& rxDiagram,
                            ObjectType eObjType );
};

struct PlotAreaModel;

class PlotAreaConverter final : public ConverterBase< PlotAreaModel >
{
public:
    explicit            PlotAreaConverter( const ConverterRoot& rParent, PlotAreaModel& rModel );
    virtual             ~PlotAreaConverter() override;

    /** Converts the OOXML plot area model to a chart2 diagram. */
    void                convertFromModel( View3DModel& rView3DModel,
            DataSourceCxModel& raDataMap );
    /** Converts the manual plot area position and size, if set. */
    void                convertPositionFromModel();

    /** Returns the automatic chart title if the chart contains only one series. */
    const OUString&     getAutomaticTitle() const { return maAutoTitle; }
    /** Returns true, if the chart contains only one series and have title textbox (even empty). */
    bool                isSingleSeriesTitle() const { return mbSingleSeriesTitle; }
    /** Returns true, if chart type supports wall and floor format in 3D mode. */
    bool                isWall3dChart() const { return mbWall3dChart; }

private:
    OUString            maAutoTitle;
    bool                mb3dChart;
    bool                mbWall3dChart;
    bool                mbPieChart;
    bool                mbSingleSeriesTitle;;
};


} // namespace oox::drawingml::chart

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
