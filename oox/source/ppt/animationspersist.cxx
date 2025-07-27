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

#include <oox/ppt/animationspersist.hxx>

#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/drawing/XShape.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/presentation/ParagraphTarget.hpp>
#include <com/sun/star/presentation/ShapeAnimationSubType.hpp>
#include <com/sun/star/animations/Event.hpp>
#include <com/sun/star/animations/XAnimationNode.hpp>
#include <com/sun/star/animations/XAnimateMotion.hpp>
#include <com/sun/star/animations/AnimationFill.hpp>
#include <com/sun/star/animations/AnimationRestart.hpp>
#include <com/sun/star/animations/Timing.hpp>
#include <com/sun/star/drawing/PointSequence.hpp>
#include <com/sun/star/animations/AnimationNodeType.hpp>
#include <com/sun/star/animations/AnimationTransformType.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/container/XEnumeration.hpp>

#include <oox/drawingml/shape.hxx>
#include <oox/helper/addtosequence.hxx>
#include <oox/token/namespaces.hxx>
#include <oox/token/tokens.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::presentation;
using namespace ::com::sun::star::animations;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::text;

namespace oox
{

Any addToSequence( const Any& rOldValue, const Any& rNewValue )
{
    if( !rNewValue.hasValue() )
    {
        return rOldValue;
    }
    else if( !rOldValue.hasValue() )
    {
        return rNewValue;
    }
    else
    {
        Sequence< Any > aNewSeq;
        if( rOldValue >>= aNewSeq )
        {
            sal_Int32 nSize = aNewSeq.getLength();
            aNewSeq.realloc(nSize+1);
            aNewSeq.getArray()[nSize] = rNewValue;
        }
        else
        {
            aNewSeq = { rOldValue, rNewValue };
        }
        return Any( aNewSeq );
    }
}

} // namespace oox

namespace oox::ppt {

    void ShapeTargetElement::convert( css::uno::Any & rTarget, sal_Int16 & rSubType ) const
    {
        switch(mnType)
        {
        case XML_subSp:
            rSubType = ShapeAnimationSubType::AS_WHOLE;
            break;
        case XML_bg:
            rSubType = ShapeAnimationSubType::ONLY_BACKGROUND;
            break;
        case XML_txEl:
        {
            ParagraphTarget aParaTarget;
            Reference< XShape > xShape;
            rTarget >>= xShape;
            aParaTarget.Shape = xShape;
            rSubType = ShapeAnimationSubType::ONLY_TEXT;

            Reference< XText > xText( xShape, UNO_QUERY );
            if( xText.is() )
            {
                switch(mnRangeType)
                {
                case XML_charRg:
                    // TODO calculate the corresponding paragraph for the text range...
                    SAL_INFO("oox.ppt", "OOX: TODO calculate the corresponding paragraph for the text range..." );
                    break;
                case XML_pRg:
                    aParaTarget.Paragraph = static_cast< sal_Int16 >( maRange.start );
                    // TODO what to do with more than one.
                    SAL_INFO("oox.ppt", "OOX: TODO what to do with more than one" );
                    break;
                }
                rTarget <<= aParaTarget;
            }
            break;
        }
        default:
            break;
        }
    }

    Any AnimTargetElement::convert(const SlidePersistPtr & pSlide, sal_Int16 & nSubType) const
    {
        Any aTarget;
        // see sd/source/files/ppt/pptinanimations.cxx:3191 (in importTargetElementContainer())
        switch(mnType)
        {
        case XML_inkTgt:
            // TODO
            SAL_INFO("oox.ppt", "OOX: TODO inkTgt" );
            break;
        case XML_sldTgt:
            // TODO
            SAL_INFO("oox.ppt", "OOX: TODO sldTgt" );
            break;
        case XML_sndTgt:
            aTarget <<= msValue;
            break;
        case XML_spTgt:
        {
            OUString sShapeName = msValue;

            // bnc#705982 - catch referenced diagram fallback shapes
            if( maShapeTarget.mnType == XML_dgm )
                sShapeName = maShapeTarget.msSubShapeId;

            ::oox::drawingml::ShapePtr pShape = pSlide->getShape( sShapeName );
            SAL_WARN_IF( !pShape, "oox.ppt", "failed to locate Shape" );

            if( !pShape && maShapeTarget.mnType == XML_dgm )
            {
                pShape = pSlide->getShape( msValue );
            }

            if( pShape )
            {
                Reference< XShape > xShape( pShape->getXShape() );
                SAL_WARN_IF( !xShape.is(), "oox.ppt", "fail to get XShape from shape" );
                if( xShape.is() )
                {
                    Any aTmpTarget;
                    aTmpTarget <<= xShape;
                    maShapeTarget.convert(aTmpTarget, nSubType);
                    aTarget = std::move(aTmpTarget);
                }
            }
            break;
        }
        default:
            break;
        }
        return aTarget;
    }

    // Convert a time node condition to XAnimation.Begin or XAnimation.End
    Any AnimationCondition::convert(const SlidePersistPtr & pSlide) const
    {
        Any aAny;
        Event aEvent;
        if(mpTarget && (maValue >>= aEvent))
        {
            sal_Int16 nSubType;
            aAny = mpTarget->convert( pSlide, nSubType );
            aEvent.Source = aAny;
            aAny <<= aEvent;
        }
        else if (mnType == PPT_TOKEN(tn) && (maValue >>= aEvent))
        {
            OUString sId;
            aEvent.Source >>= sId;
            css::uno::Reference<XAnimationNode> xNode = pSlide->getAnimationNode(sId);
            if (xNode.is())
            {
                aEvent.Source <<= xNode;
            }
            else
                aEvent.Source.clear();
            aAny <<= aEvent;
        }
        else
        {
            aAny = maValue;
        }
        return aAny;
    }

    Any AnimationCondition::convertList(const SlidePersistPtr & pSlide, const AnimationConditionList & l)
    {
        Any aAny;

        if (l.size() == 1)
            return l[0].convert(pSlide);

        for (auto const& elem : l)
        {
            aAny = addToSequence( aAny, elem.convert(pSlide) );
        }
        return aAny;
    }

    void MotionPathProperties::convertToSvgPath(OUString& rSvgPath) const
    {
        if (maPath.isEmpty())
            return;

        // Convert PowerPoint motion path to SVG path
        // PowerPoint uses a coordinate system where paths are relative to shape position
        OUStringBuffer aSvgPath;
        sal_Int32 nIndex = 0;
        
        while (nIndex < maPath.getLength())
        {
            sal_Unicode cCommand = maPath[nIndex];
            nIndex++;
            
            switch (cCommand)
            {
                case 'M': // Move to
                case 'L': // Line to
                case 'C': // Cubic bezier
                case 'Z': // Close path
                    aSvgPath.append(cCommand);
                    break;
                case 'E': // End (PowerPoint specific)
                    // Convert to SVG close path
                    aSvgPath.append('Z');
                    break;
                default:
                    // Copy coordinates
                    while (nIndex < maPath.getLength() && 
                           (maPath[nIndex] == ' ' || maPath[nIndex] == ',' ||
                            (maPath[nIndex] >= '0' && maPath[nIndex] <= '9') ||
                            maPath[nIndex] == '.' || maPath[nIndex] == '-'))
                    {
                        aSvgPath.append(maPath[nIndex]);
                        nIndex++;
                    }
                    break;
            }
        }
        
        rSvgPath = aSvgPath.makeStringAndClear();
    }

    void MotionPathProperties::applyToAnimationNode(
        const Reference<XAnimationNode>& xNode) const
    {
        Reference<XAnimateMotion> xAnimateMotion(xNode, UNO_QUERY);
        if (!xAnimateMotion.is())
            return;

        // Convert and set the motion path
        OUString sSvgPath;
        convertToSvgPath(sSvgPath);
        if (!sSvgPath.isEmpty())
        {
            xAnimateMotion->setPath(Any(sSvgPath));
        }

        // Set origin if specified
        if (mnOrigin != 0)
        {
            xAnimateMotion->setOrigin(Any(mnOrigin));
        }
    }

    double AnimationTiming::convertDuration(const OUString& rDuration)
    {
        // Convert PowerPoint duration format to seconds
        // Format can be: "1000" (milliseconds), "1s", "1.5s", "indefinite"
        
        if (rDuration == "indefinite")
            return -1.0; // Indefinite duration
        
        double fDuration = 0.0;
        OUString sDuration = rDuration.trim();
        
        if (sDuration.endsWith("s"))
        {
            // Duration in seconds
            sDuration = sDuration.copy(0, sDuration.getLength() - 1);
            fDuration = sDuration.toDouble();
        }
        else if (sDuration.endsWith("ms"))
        {
            // Duration in milliseconds
            sDuration = sDuration.copy(0, sDuration.getLength() - 2);
            fDuration = sDuration.toDouble() / 1000.0;
        }
        else
        {
            // Assume milliseconds if no unit
            fDuration = sDuration.toDouble() / 1000.0;
        }
        
        return fDuration;
    }

    void AnimationTiming::fixNodeTiming(const Reference<XAnimationNode>& xNode)
    {
        if (!xNode.is())
            return;

        // Fix common timing issues
        
        // 1. Ensure fill behavior is set correctly
        if (!xNode->getFill())
        {
            sal_Int16 nNodeType = xNode->getType();
            if (nNodeType == AnimationNodeType::ANIMATE ||
                nNodeType == AnimationNodeType::ANIMATEMOTION ||
                nNodeType == AnimationNodeType::ANIMATETRANSFORM ||
                nNodeType == AnimationNodeType::ANIMATECOLOR ||
                nNodeType == AnimationNodeType::SET)
            {
                // Default to "hold" for animation nodes
                xNode->setFill(AnimationFill::HOLD);
            }
        }

        // 2. Fix restart behavior
        if (xNode->getRestart() == AnimationRestart::DEFAULT)
        {
            // Set to "never" to prevent unexpected restarts
            xNode->setRestart(AnimationRestart::NEVER);
        }

        // 3. Ensure begin time is properly set
        Any aBegin = xNode->getBegin();
        if (!aBegin.hasValue())
        {
            // Set to 0 for immediate start
            xNode->setBegin(Any(0.0));
        }

        // 4. Fix duration for container nodes
        sal_Int16 nNodeType = xNode->getType();
        if (nNodeType == AnimationNodeType::PAR ||
            nNodeType == AnimationNodeType::SEQ)
        {
            Any aDuration = xNode->getDuration();
            if (!aDuration.hasValue())
            {
                // Calculate duration from children
                double fMaxEndTime = 0.0;
                Reference<XEnumerationAccess> xEnumAccess(xNode, UNO_QUERY);
                if (xEnumAccess.is())
                {
                    Reference<XEnumeration> xEnum = xEnumAccess->createEnumeration();
                    while (xEnum->hasMoreElements())
                    {
                        Reference<XAnimationNode> xChild;
                        xEnum->nextElement() >>= xChild;
                        if (xChild.is())
                        {
                            double fChildEnd = calculateNodeEndTime(xChild);
                            fMaxEndTime = std::max(fMaxEndTime, fChildEnd);
                        }
                    }
                }
                
                if (fMaxEndTime > 0.0)
                {
                    xNode->setDuration(Any(fMaxEndTime));
                }
            }
        }
    }

    double AnimationTiming::calculateNodeEndTime(const Reference<XAnimationNode>& xNode)
    {
        if (!xNode.is())
            return 0.0;

        double fBegin = 0.0;
        double fDuration = 0.0;
        
        // Get begin time
        Any aBegin = xNode->getBegin();
        if (aBegin.hasValue())
        {
            if (aBegin >>= fBegin)
            {
                // Already a double
            }
            else
            {
                Event aEvent;
                if (aBegin >>= aEvent)
                {
                    // Event-based, assume 0 for calculation
                    fBegin = 0.0;
                }
            }
        }
        
        // Get duration
        Any aDuration = xNode->getDuration();
        if (aDuration.hasValue())
        {
            if (aDuration >>= fDuration)
            {
                // Already a double
            }
            else
            {
                OUString sDuration;
                if (aDuration >>= sDuration)
                {
                    fDuration = convertDuration(sDuration);
                }
            }
        }
        
        // For indefinite or invalid duration, use a default
        if (fDuration < 0.0 || fDuration > 3600.0) // Max 1 hour
        {
            fDuration = 1.0; // Default 1 second
        }
        
        return fBegin + fDuration;
    }

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
