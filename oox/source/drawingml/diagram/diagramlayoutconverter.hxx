/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_OOX_DRAWINGML_DIAGRAM_DIAGRAMLAYOUTCONVERTER_HXX
#define INCLUDED_OOX_DRAWINGML_DIAGRAM_DIAGRAMLAYOUTCONVERTER_HXX

#include <oox/drawingml/diagram/datamodel.hxx>
#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <memory>
#include <vector>

namespace com::sun::star::drawing { class XShapes; }

namespace oox::drawingml {

class Shape;
typedef std::shared_ptr<Shape> ShapePtr;

/** Converts SmartArt diagram layouts into drawing shapes
 *
 * This class handles the conversion of various SmartArt diagram types
 * (process, hierarchy, cycle, etc.) into actual drawing shapes that
 * can be rendered in Impress.
 */
class DiagramLayoutConverter
{
public:
    DiagramLayoutConverter();
    ~DiagramLayoutConverter();

    /** Convert diagram data to shapes within the given bounds */
    void convertToShapes(
        const DiagramDataPtr& pDiagramData,
        const css::uno::Reference<css::drawing::XShapes>& rxShapes,
        const css::awt::Rectangle& rBounds);

private:
    enum class DiagramType
    {
        Unknown,
        Process,
        Hierarchy,
        Cycle,
        Relationship,
        Matrix,
        Pyramid
    };

    /** Detect the type of diagram based on its structure */
    DiagramType detectDiagramType(const DiagramDataPtr& pDiagramData);

    /** Convert a process/flow diagram */
    void convertProcessDiagram(
        const DiagramDataPtr& pDiagramData,
        const css::uno::Reference<css::drawing::XShapes>& rxShapes,
        const css::awt::Rectangle& rBounds);

    /** Convert a hierarchy/org chart diagram */
    void convertHierarchyDiagram(
        const DiagramDataPtr& pDiagramData,
        const css::uno::Reference<css::drawing::XShapes>& rxShapes,
        const css::awt::Rectangle& rBounds);

    /** Convert a cycle diagram */
    void convertCycleDiagram(
        const DiagramDataPtr& pDiagramData,
        const css::uno::Reference<css::drawing::XShapes>& rxShapes,
        const css::awt::Rectangle& rBounds);

    /** Convert a relationship diagram */
    void convertRelationshipDiagram(
        const DiagramDataPtr& pDiagramData,
        const css::uno::Reference<css::drawing::XShapes>& rxShapes,
        const css::awt::Rectangle& rBounds);

    /** Default conversion for unknown diagram types */
    void convertDefaultDiagram(
        const DiagramDataPtr& pDiagramData,
        const css::uno::Reference<css::drawing::XShapes>& rxShapes,
        const css::awt::Rectangle& rBounds);

    /** Helper to create arrow shapes */
    ShapePtr createArrowShape(
        const css::awt::Point& rStart,
        const css::awt::Point& rEnd,
        sal_Int32 nHeight);

    /** Helper to create connector lines */
    ShapePtr createConnectorLine(
        const css::awt::Point& rStart,
        const css::awt::Point& rEnd);

    /** Helper to create curved arrows for cycle diagrams */
    ShapePtr createCurvedArrow(
        double fStartAngle, double fEndAngle,
        sal_Int32 nRadius, sal_Int32 nCenterX, sal_Int32 nCenterY,
        sal_Int32 nShapeSize);

    /** Check if diagram has cyclic connections */
    bool checkForCycles(const DiagramDataPtr& pDiagramData);

    /** Hierarchy tree node structure */
    struct HierarchyNode
    {
        const DiagramNode* mpData = nullptr;
        std::vector<HierarchyNode> maChildren;
    };

    /** Build hierarchy tree from diagram data */
    HierarchyNode buildHierarchyTree(const DiagramDataPtr& pDiagramData);
    
    /** Recursively build hierarchy subtree */
    void buildHierarchySubtree(HierarchyNode& rParent, const DiagramDataPtr& pDiagramData);
    
    /** Layout hierarchy level */
    void layoutHierarchyLevel(
        const HierarchyNode& rNode,
        const css::uno::Reference<css::drawing::XShapes>& rxShapes,
        sal_Int32 nX, sal_Int32 nY,
        sal_Int32 nWidth, sal_Int32 nLevelHeight,
        sal_Int32 nLevel);
    
    /** Get maximum depth of hierarchy tree */
    sal_Int32 getMaxDepth(const HierarchyNode& rNode);
};

} // namespace oox::drawingml

#endif // INCLUDED_OOX_DRAWINGML_DIAGRAM_DIAGRAMLAYOUTCONVERTER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */