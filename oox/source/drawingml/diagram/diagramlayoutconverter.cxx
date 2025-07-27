/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "diagramlayoutconverter.hxx"
#include <oox/drawingml/shape.hxx>
#include <oox/drawingml/shapepropertymap.hxx>
#include <oox/helper/propertymap.hxx>
#include <com/sun/star/awt/Point.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/drawing/XShapes.hpp>
#include <basegfx/point/b2dpoint.hxx>
#include <sal/log.hxx>
#include <cmath>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::drawing;

namespace oox::drawingml {

DiagramLayoutConverter::DiagramLayoutConverter()
{
}

DiagramLayoutConverter::~DiagramLayoutConverter()
{
}

void DiagramLayoutConverter::convertToShapes(
    const DiagramDataPtr& pDiagramData,
    const Reference<XShapes>& rxShapes,
    const awt::Rectangle& rBounds)
{
    if (!pDiagramData || !rxShapes.is())
        return;

    // Determine diagram type and layout
    DiagramType eType = detectDiagramType(pDiagramData);
    
    switch (eType)
    {
        case DiagramType::Process:
            convertProcessDiagram(pDiagramData, rxShapes, rBounds);
            break;
        case DiagramType::Hierarchy:
            convertHierarchyDiagram(pDiagramData, rxShapes, rBounds);
            break;
        case DiagramType::Cycle:
            convertCycleDiagram(pDiagramData, rxShapes, rBounds);
            break;
        case DiagramType::Relationship:
            convertRelationshipDiagram(pDiagramData, rxShapes, rBounds);
            break;
        default:
            SAL_WARN("oox.drawingml", "Unknown diagram type, using default layout");
            convertDefaultDiagram(pDiagramData, rxShapes, rBounds);
            break;
    }
}

DiagramLayoutConverter::DiagramType DiagramLayoutConverter::detectDiagramType(
    const DiagramDataPtr& pDiagramData)
{
    if (!pDiagramData)
        return DiagramType::Unknown;

    // Analyze the diagram structure to determine type
    // This is a simplified detection - in practice would check layout atoms
    const auto& rConnections = pDiagramData->getConnections();
    const auto& rNodes = pDiagramData->getNodes();
    
    if (rNodes.size() > 0)
    {
        // Check for hierarchical structure (tree-like)
        int nRootNodes = 0;
        for (const auto& rNode : rNodes)
        {
            bool bHasIncoming = false;
            for (const auto& rConn : rConnections)
            {
                if (rConn.msDestId == rNode.msNodeId)
                {
                    bHasIncoming = true;
                    break;
                }
            }
            if (!bHasIncoming)
                nRootNodes++;
        }
        
        if (nRootNodes == 1)
            return DiagramType::Hierarchy;
        
        // Check for cyclic structure
        bool bCyclic = checkForCycles(pDiagramData);
        if (bCyclic)
            return DiagramType::Cycle;
        
        // Default to process for linear flows
        return DiagramType::Process;
    }
    
    return DiagramType::Unknown;
}

void DiagramLayoutConverter::convertProcessDiagram(
    const DiagramDataPtr& pDiagramData,
    const Reference<XShapes>& rxShapes,
    const awt::Rectangle& rBounds)
{
    const auto& rNodes = pDiagramData->getNodes();
    if (rNodes.empty())
        return;

    // Calculate shape dimensions
    sal_Int32 nNodeCount = rNodes.size();
    sal_Int32 nSpacing = rBounds.Width / 20; // 5% spacing
    sal_Int32 nShapeWidth = (rBounds.Width - (nNodeCount + 1) * nSpacing) / nNodeCount;
    sal_Int32 nShapeHeight = rBounds.Height * 0.6;
    sal_Int32 nArrowHeight = rBounds.Height * 0.1;
    
    sal_Int32 nCurrentX = rBounds.X + nSpacing;
    sal_Int32 nShapeY = rBounds.Y + (rBounds.Height - nShapeHeight) / 2;
    
    // Create shapes for each node
    for (size_t i = 0; i < rNodes.size(); ++i)
    {
        const auto& rNode = rNodes[i];
        
        // Create main shape (rectangle with rounded corners)
        ShapePtr pShape = std::make_shared<Shape>("com.sun.star.drawing.CustomShape");
        pShape->setSize(awt::Size(nShapeWidth, nShapeHeight));
        pShape->setPosition(awt::Point(nCurrentX, nShapeY));
        
        // Set shape properties
        PropertyMap aProps;
        aProps.setProperty(PROP_FillColor, sal_Int32(0x4472C4)); // Blue fill
        aProps.setProperty(PROP_LineColor, sal_Int32(0x2E5395)); // Darker blue border
        aProps.setProperty(PROP_LineWidth, sal_Int32(2540)); // 2pt line
        
        // Add text
        if (rNode.mpShape && rNode.mpShape->getTextBody())
        {
            pShape->setTextBody(rNode.mpShape->getTextBody());
        }
        
        pShape->getShapeProperties().assignUsed(aProps);
        pShape->addShape(rxShapes);
        
        // Add arrow between shapes (except after last shape)
        if (i < rNodes.size() - 1)
        {
            ShapePtr pArrow = createArrowShape(
                awt::Point(nCurrentX + nShapeWidth + nSpacing/3, nShapeY + nShapeHeight/2),
                awt::Point(nCurrentX + nShapeWidth + nSpacing*2/3, nShapeY + nShapeHeight/2),
                nArrowHeight);
            pArrow->addShape(rxShapes);
        }
        
        nCurrentX += nShapeWidth + nSpacing;
    }
}

void DiagramLayoutConverter::convertHierarchyDiagram(
    const DiagramDataPtr& pDiagramData,
    const Reference<XShapes>& rxShapes,
    const awt::Rectangle& rBounds)
{
    const auto& rNodes = pDiagramData->getNodes();
    const auto& rConnections = pDiagramData->getConnections();
    
    if (rNodes.empty())
        return;
    
    // Build hierarchy tree structure
    HierarchyNode rootNode = buildHierarchyTree(pDiagramData);
    
    // Calculate layout parameters
    sal_Int32 nLevelHeight = rBounds.Height / (getMaxDepth(rootNode) + 1);
    
    // Layout and create shapes recursively
    layoutHierarchyLevel(rootNode, rxShapes, rBounds.X, rBounds.Y, 
                        rBounds.Width, nLevelHeight, 0);
}

void DiagramLayoutConverter::layoutHierarchyLevel(
    const HierarchyNode& rNode,
    const Reference<XShapes>& rxShapes,
    sal_Int32 nX, sal_Int32 nY,
    sal_Int32 nWidth, sal_Int32 nLevelHeight,
    sal_Int32 nLevel)
{
    if (!rNode.mpData)
        return;
    
    // Calculate shape size and position
    sal_Int32 nShapeWidth = std::min(nWidth * 0.8, sal_Int32(200000)); // Max 200pt width
    sal_Int32 nShapeHeight = nLevelHeight * 0.6;
    sal_Int32 nShapeX = nX + (nWidth - nShapeWidth) / 2;
    sal_Int32 nShapeY = nY + (nLevelHeight - nShapeHeight) / 2;
    
    // Create shape
    ShapePtr pShape = std::make_shared<Shape>("com.sun.star.drawing.CustomShape");
    pShape->setSize(awt::Size(nShapeWidth, nShapeHeight));
    pShape->setPosition(awt::Point(nShapeX, nShapeY));
    
    // Set shape properties based on level
    PropertyMap aProps;
    sal_Int32 nFillColor = (nLevel == 0) ? 0x4472C4 : // Root: Blue
                          (nLevel == 1) ? 0x70AD47 : // Level 1: Green
                          0xFFC000; // Level 2+: Orange
    aProps.setProperty(PROP_FillColor, nFillColor);
    aProps.setProperty(PROP_LineColor, sal_Int32(0x5B5B5B));
    aProps.setProperty(PROP_LineWidth, sal_Int32(2540));
    
    // Add text
    if (rNode.mpData->mpShape && rNode.mpData->mpShape->getTextBody())
    {
        pShape->setTextBody(rNode.mpData->mpShape->getTextBody());
    }
    
    pShape->getShapeProperties().assignUsed(aProps);
    pShape->addShape(rxShapes);
    
    // Store shape position for connector lines
    awt::Point aBottomCenter(nShapeX + nShapeWidth/2, nShapeY + nShapeHeight);
    
    // Layout children
    if (!rNode.maChildren.empty())
    {
        sal_Int32 nChildWidth = nWidth / rNode.maChildren.size();
        sal_Int32 nChildX = nX;
        sal_Int32 nChildY = nY + nLevelHeight;
        
        for (const auto& rChild : rNode.maChildren)
        {
            // Draw connector line
            awt::Point aChildTop(nChildX + nChildWidth/2, nChildY);
            ShapePtr pConnector = createConnectorLine(aBottomCenter, aChildTop);
            pConnector->addShape(rxShapes);
            
            // Layout child node
            layoutHierarchyLevel(rChild, rxShapes, nChildX, nChildY,
                               nChildWidth, nLevelHeight, nLevel + 1);
            nChildX += nChildWidth;
        }
    }
}

void DiagramLayoutConverter::convertCycleDiagram(
    const DiagramDataPtr& pDiagramData,
    const Reference<XShapes>& rxShapes,
    const awt::Rectangle& rBounds)
{
    const auto& rNodes = pDiagramData->getNodes();
    if (rNodes.empty())
        return;
    
    // Calculate circle parameters
    sal_Int32 nCenterX = rBounds.X + rBounds.Width / 2;
    sal_Int32 nCenterY = rBounds.Y + rBounds.Height / 2;
    sal_Int32 nRadius = std::min(rBounds.Width, rBounds.Height) * 0.35;
    sal_Int32 nShapeSize = nRadius * 0.4;
    
    double fAngleStep = 2.0 * M_PI / rNodes.size();
    double fStartAngle = -M_PI / 2; // Start at top
    
    // Create shapes in circular arrangement
    for (size_t i = 0; i < rNodes.size(); ++i)
    {
        const auto& rNode = rNodes[i];
        double fAngle = fStartAngle + i * fAngleStep;
        
        // Calculate position
        sal_Int32 nShapeX = nCenterX + static_cast<sal_Int32>(nRadius * cos(fAngle)) - nShapeSize/2;
        sal_Int32 nShapeY = nCenterY + static_cast<sal_Int32>(nRadius * sin(fAngle)) - nShapeSize/2;
        
        // Create circular shape
        ShapePtr pShape = std::make_shared<Shape>("com.sun.star.drawing.EllipseShape");
        pShape->setSize(awt::Size(nShapeSize, nShapeSize));
        pShape->setPosition(awt::Point(nShapeX, nShapeY));
        
        // Set properties
        PropertyMap aProps;
        aProps.setProperty(PROP_FillColor, sal_Int32(0x4472C4));
        aProps.setProperty(PROP_LineColor, sal_Int32(0x2E5395));
        aProps.setProperty(PROP_LineWidth, sal_Int32(2540));
        
        // Add text
        if (rNode.mpShape && rNode.mpShape->getTextBody())
        {
            pShape->setTextBody(rNode.mpShape->getTextBody());
        }
        
        pShape->getShapeProperties().assignUsed(aProps);
        pShape->addShape(rxShapes);
        
        // Add curved arrow to next shape
        size_t nNextIdx = (i + 1) % rNodes.size();
        double fNextAngle = fStartAngle + nNextIdx * fAngleStep;
        
        ShapePtr pArrow = createCurvedArrow(
            fAngle, fNextAngle, nRadius, nCenterX, nCenterY, nShapeSize);
        pArrow->addShape(rxShapes);
    }
}

void DiagramLayoutConverter::convertRelationshipDiagram(
    const DiagramDataPtr& pDiagramData,
    const Reference<XShapes>& rxShapes,
    const awt::Rectangle& rBounds)
{
    // Implementation for relationship diagrams
    // This would handle diagrams showing relationships between entities
    convertDefaultDiagram(pDiagramData, rxShapes, rBounds);
}

void DiagramLayoutConverter::convertDefaultDiagram(
    const DiagramDataPtr& pDiagramData,
    const Reference<XShapes>& rxShapes,
    const awt::Rectangle& rBounds)
{
    // Simple grid layout for unknown diagram types
    const auto& rNodes = pDiagramData->getNodes();
    if (rNodes.empty())
        return;
    
    sal_Int32 nCols = static_cast<sal_Int32>(ceil(sqrt(static_cast<double>(rNodes.size()))));
    sal_Int32 nRows = (rNodes.size() + nCols - 1) / nCols;
    
    sal_Int32 nCellWidth = rBounds.Width / nCols;
    sal_Int32 nCellHeight = rBounds.Height / nRows;
    sal_Int32 nShapeWidth = nCellWidth * 0.8;
    sal_Int32 nShapeHeight = nCellHeight * 0.8;
    
    size_t nIdx = 0;
    for (sal_Int32 nRow = 0; nRow < nRows && nIdx < rNodes.size(); ++nRow)
    {
        for (sal_Int32 nCol = 0; nCol < nCols && nIdx < rNodes.size(); ++nCol, ++nIdx)
        {
            const auto& rNode = rNodes[nIdx];
            
            sal_Int32 nX = rBounds.X + nCol * nCellWidth + (nCellWidth - nShapeWidth) / 2;
            sal_Int32 nY = rBounds.Y + nRow * nCellHeight + (nCellHeight - nShapeHeight) / 2;
            
            ShapePtr pShape = std::make_shared<Shape>("com.sun.star.drawing.CustomShape");
            pShape->setSize(awt::Size(nShapeWidth, nShapeHeight));
            pShape->setPosition(awt::Point(nX, nY));
            
            PropertyMap aProps;
            aProps.setProperty(PROP_FillColor, sal_Int32(0x4472C4));
            aProps.setProperty(PROP_LineColor, sal_Int32(0x2E5395));
            
            if (rNode.mpShape && rNode.mpShape->getTextBody())
            {
                pShape->setTextBody(rNode.mpShape->getTextBody());
            }
            
            pShape->getShapeProperties().assignUsed(aProps);
            pShape->addShape(rxShapes);
        }
    }
}

ShapePtr DiagramLayoutConverter::createArrowShape(
    const awt::Point& rStart,
    const awt::Point& rEnd,
    sal_Int32 nHeight)
{
    ShapePtr pArrow = std::make_shared<Shape>("com.sun.star.drawing.PolyLineShape");
    
    // Create arrow path
    std::vector<awt::Point> aPoints;
    aPoints.push_back(rStart);
    aPoints.push_back(rEnd);
    
    // Add arrowhead
    sal_Int32 nArrowSize = nHeight / 2;
    aPoints.push_back(awt::Point(rEnd.X - nArrowSize, rEnd.Y - nArrowSize/2));
    aPoints.push_back(awt::Point(rEnd.X, rEnd.Y));
    aPoints.push_back(awt::Point(rEnd.X - nArrowSize, rEnd.Y + nArrowSize/2));
    
    PropertyMap aProps;
    aProps.setProperty(PROP_LineColor, sal_Int32(0x5B5B5B));
    aProps.setProperty(PROP_LineWidth, sal_Int32(2540));
    
    pArrow->getShapeProperties().assignUsed(aProps);
    return pArrow;
}

ShapePtr DiagramLayoutConverter::createConnectorLine(
    const awt::Point& rStart,
    const awt::Point& rEnd)
{
    ShapePtr pLine = std::make_shared<Shape>("com.sun.star.drawing.LineShape");
    pLine->setPosition(rStart);
    pLine->setSize(awt::Size(rEnd.X - rStart.X, rEnd.Y - rStart.Y));
    
    PropertyMap aProps;
    aProps.setProperty(PROP_LineColor, sal_Int32(0x5B5B5B));
    aProps.setProperty(PROP_LineWidth, sal_Int32(1270)); // 1pt
    
    pLine->getShapeProperties().assignUsed(aProps);
    return pLine;
}

ShapePtr DiagramLayoutConverter::createCurvedArrow(
    double fStartAngle, double fEndAngle,
    sal_Int32 nRadius, sal_Int32 nCenterX, sal_Int32 nCenterY,
    sal_Int32 nShapeSize)
{
    // Create a curved connector with arrow
    ShapePtr pCurve = std::make_shared<Shape>("com.sun.star.drawing.OpenBezierShape");
    
    // Calculate control points for bezier curve
    double fMidAngle = (fStartAngle + fEndAngle) / 2;
    double fCurveRadius = nRadius * 0.8;
    
    awt::Point aStart(
        nCenterX + static_cast<sal_Int32>((nRadius - nShapeSize/2) * cos(fStartAngle)),
        nCenterY + static_cast<sal_Int32>((nRadius - nShapeSize/2) * sin(fStartAngle)));
    
    awt::Point aEnd(
        nCenterX + static_cast<sal_Int32>((nRadius - nShapeSize/2) * cos(fEndAngle)),
        nCenterY + static_cast<sal_Int32>((nRadius - nShapeSize/2) * sin(fEndAngle)));
    
    awt::Point aControl(
        nCenterX + static_cast<sal_Int32>(fCurveRadius * cos(fMidAngle)),
        nCenterY + static_cast<sal_Int32>(fCurveRadius * sin(fMidAngle)));
    
    PropertyMap aProps;
    aProps.setProperty(PROP_LineColor, sal_Int32(0x5B5B5B));
    aProps.setProperty(PROP_LineWidth, sal_Int32(2540));
    
    pCurve->getShapeProperties().assignUsed(aProps);
    return pCurve;
}

bool DiagramLayoutConverter::checkForCycles(const DiagramDataPtr& pDiagramData)
{
    // Simple cycle detection - in practice would use DFS/BFS
    const auto& rConnections = pDiagramData->getConnections();
    
    // Check if last node connects back to first
    if (!rConnections.empty())
    {
        const auto& rNodes = pDiagramData->getNodes();
        if (rNodes.size() > 2)
        {
            OUString sFirstId = rNodes.front().msNodeId;
            OUString sLastId = rNodes.back().msNodeId;
            
            for (const auto& rConn : rConnections)
            {
                if (rConn.msSourceId == sLastId && rConn.msDestId == sFirstId)
                    return true;
            }
        }
    }
    
    return false;
}

DiagramLayoutConverter::HierarchyNode DiagramLayoutConverter::buildHierarchyTree(
    const DiagramDataPtr& pDiagramData)
{
    HierarchyNode rootNode;
    const auto& rNodes = pDiagramData->getNodes();
    const auto& rConnections = pDiagramData->getConnections();
    
    // Find root node (no incoming connections)
    for (const auto& rNode : rNodes)
    {
        bool bIsRoot = true;
        for (const auto& rConn : rConnections)
        {
            if (rConn.msDestId == rNode.msNodeId)
            {
                bIsRoot = false;
                break;
            }
        }
        
        if (bIsRoot)
        {
            rootNode.mpData = &rNode;
            buildHierarchySubtree(rootNode, pDiagramData);
            break;
        }
    }
    
    return rootNode;
}

void DiagramLayoutConverter::buildHierarchySubtree(
    HierarchyNode& rParent,
    const DiagramDataPtr& pDiagramData)
{
    if (!rParent.mpData)
        return;
    
    const auto& rNodes = pDiagramData->getNodes();
    const auto& rConnections = pDiagramData->getConnections();
    
    // Find children
    for (const auto& rConn : rConnections)
    {
        if (rConn.msSourceId == rParent.mpData->msNodeId)
        {
            // Find child node
            for (const auto& rNode : rNodes)
            {
                if (rNode.msNodeId == rConn.msDestId)
                {
                    HierarchyNode childNode;
                    childNode.mpData = &rNode;
                    buildHierarchySubtree(childNode, pDiagramData);
                    rParent.maChildren.push_back(childNode);
                    break;
                }
            }
        }
    }
}

sal_Int32 DiagramLayoutConverter::getMaxDepth(const HierarchyNode& rNode)
{
    if (rNode.maChildren.empty())
        return 0;
    
    sal_Int32 nMaxChildDepth = 0;
    for (const auto& rChild : rNode.maChildren)
    {
        nMaxChildDepth = std::max(nMaxChildDepth, getMaxDepth(rChild));
    }
    
    return nMaxChildDepth + 1;
}

} // namespace oox::drawingml

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */