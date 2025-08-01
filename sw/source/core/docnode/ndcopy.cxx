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
#include <doc.hxx>
#include <IDocumentFieldsAccess.hxx>
#include <IDocumentUndoRedo.hxx>
#include <node.hxx>
#include <frmfmt.hxx>
#include <swtable.hxx>
#include <ndtxt.hxx>
#include <swtblfmt.hxx>
#include <cellatr.hxx>
#include <ddefld.hxx>
#include <swddetbl.hxx>
#include <ndindex.hxx>
#include <frameformats.hxx>
#include <vector>
#include <osl/diagnose.h>
#include <svl/numformat.hxx>


#ifdef DBG_UTIL
#define CHECK_TABLE(t) (t).CheckConsistency();
#else
#define CHECK_TABLE(t)
#endif

namespace {

// Structure for the mapping from old and new frame formats to the
// boxes and lines of a table
struct MapTableFrameFormat
{
    const SwFrameFormat *pOld;
    SwFrameFormat *pNew;
    MapTableFrameFormat( const SwFrameFormat *pOldFormat, SwFrameFormat*pNewFormat )
        : pOld( pOldFormat ), pNew( pNewFormat )
    {}
};

}

typedef std::vector<MapTableFrameFormat> MapTableFrameFormats;

SwContentNode* SwTextNode::MakeCopy(SwDoc& rDoc, SwNode& rIdx, bool const bNewFrames) const
{
    // the Copy-Textnode is the Node with the Text, the Copy-Attrnode is the
    // node with the collection and hard attributes. Normally is the same
    // node, but if insert a glossary without formatting, then the Attrnode
    // is the prev node of the destination position in dest. document.
    SwTextNode* pCpyTextNd = const_cast<SwTextNode*>(this);
    SwTextNode* pCpyAttrNd = pCpyTextNd;

    // Copy the formats to the other document
    SwTextFormatColl* pColl = nullptr;
    if( rDoc.IsInsOnlyTextGlossary() )
    {
        SwNodeIndex aIdx( rIdx, -1 );
        if( aIdx.GetNode().IsTextNode() )
        {
            pCpyAttrNd = aIdx.GetNode().GetTextNode();
            pColl = &pCpyAttrNd->GetTextColl()->GetNextTextFormatColl();
        }
    }
    if( !pColl )
        pColl = rDoc.CopyTextColl( *GetTextColl() );

    SwTextNode* pTextNd = rDoc.GetNodes().MakeTextNode(rIdx, pColl, bNewFrames);

    // METADATA: register copy
    pTextNd->RegisterAsCopyOf(*pCpyTextNd);

    // Copy Attribute/Text
    if( !pCpyAttrNd->HasSwAttrSet() )
        // An AttrSet was added for numbering, so delete it
        pTextNd->ResetAllAttr();

    // if Copy-Textnode unequal to Copy-Attrnode, then copy first
    // the attributes into the new Node.
    if( pCpyAttrNd != pCpyTextNd )
    {
        pCpyAttrNd->CopyAttr( pTextNd, 0, 0 );
        if( pCpyAttrNd->HasSwAttrSet() )
        {
            SwAttrSet aSet( *pCpyAttrNd->GetpSwAttrSet() );
            aSet.ClearItem( RES_PAGEDESC );
            aSet.ClearItem( RES_BREAK );
            aSet.CopyToModify( *pTextNd );
        }
    }

    // Is that enough? What about PostIts/Fields/FieldTypes?
    // #i96213# - force copy of all attributes
    pCpyTextNd->CopyText( pTextNd, SwContentIndex( pCpyTextNd ),
        pCpyTextNd->GetText().getLength(), true );

    if( RES_CONDTXTFMTCOLL == pColl->Which() )
        pTextNd->ChkCondColl();

    return pTextNd;
}

static bool lcl_SrchNew( const MapTableFrameFormat& rMap, SwFrameFormat** pPara )
{
    if( rMap.pOld != *pPara )
        return true;
    *pPara = rMap.pNew;
    return false;
}

namespace {

struct CopyTable
{
    SwDoc& m_rDoc;
    SwNodeOffset m_nOldTableSttIdx;
    MapTableFrameFormats& m_rMapArr;
    SwTableLine* m_pInsLine;
    SwTableBox* m_pInsBox;
    SwTableNode *m_pTableNd;
    const SwTable *m_pOldTable;

    CopyTable(SwDoc& rDc, MapTableFrameFormats& rArr, SwNodeOffset nOldStt,
               SwTableNode& rTableNd, const SwTable* pOldTable)
        : m_rDoc(rDc), m_nOldTableSttIdx(nOldStt), m_rMapArr(rArr),
          m_pInsLine(nullptr), m_pInsBox(nullptr), m_pTableNd(&rTableNd), m_pOldTable(pOldTable)
    {}
};

}

static void lcl_CopyTableLine( const SwTableLine* pLine, CopyTable* pCT );

static void lcl_CopyTableBox( SwTableBox* pBox, CopyTable* pCT )
{
    SwTableBoxFormat * pBoxFormat = pBox->GetFrameFormat();
    for (const auto& rMap : pCT->m_rMapArr)
        if ( !lcl_SrchNew( rMap, reinterpret_cast<SwFrameFormat**>(&pBoxFormat) ) )
            break;

    if (pBoxFormat == pBox->GetFrameFormat()) // Create a new one?
    {
        const SwTableBoxFormula* pFormulaItem = pBoxFormat->GetItemIfSet( RES_BOXATR_FORMULA, false );
        if( pFormulaItem && pFormulaItem->IsIntrnlName() )
        {
            const_cast<SwTableBoxFormula*>(pFormulaItem)->PtrToBoxNm(pCT->m_pOldTable);
        }

        pBoxFormat = pCT->m_rDoc.MakeTableBoxFormat();
        pBoxFormat->CopyAttrs( *pBox->GetFrameFormat() );

        if( pBox->GetSttIdx() )
        {
            SvNumberFormatter* pN = pCT->m_rDoc.GetNumberFormatter(false);
            const SwTableBoxNumFormat* pFormatItem;
            if( pN && pN->HasMergeFormatTable() &&
                (pFormatItem = pBoxFormat->GetItemIfSet( RES_BOXATR_FORMAT, false )) )
            {
                sal_uLong nOldIdx = pFormatItem->GetValue();
                sal_uLong nNewIdx = pN->GetMergeFormatIndex( nOldIdx );
                if( nNewIdx != nOldIdx )
                    pBoxFormat->SetFormatAttr( SwTableBoxNumFormat( nNewIdx ));

            }
        }

        pCT->m_rMapArr.emplace_back(pBox->GetFrameFormat(), pBoxFormat);
    }

    sal_uInt16 nLines = pBox->GetTabLines().size();
    SwTableBox* pNewBox;
    if( nLines )
        pNewBox = new SwTableBox(pBoxFormat, nLines, pCT->m_pInsLine);
    else
    {
        SwNodeIndex aNewIdx(*pCT->m_pTableNd, pBox->GetSttIdx() - pCT->m_nOldTableSttIdx);
        assert(aNewIdx.GetNode().IsStartNode() && "Index is not on the start node");

        pNewBox = new SwTableBox(pBoxFormat, aNewIdx, pCT->m_pInsLine);
        pNewBox->setRowSpan( pBox->getRowSpan() );
    }

    pCT->m_pInsLine->GetTabBoxes().push_back( pNewBox );

    if (nLines)
    {
        CopyTable aPara(*pCT);
        aPara.m_pInsBox = pNewBox;
        for( const SwTableLine* pLine : pBox->GetTabLines() )
            lcl_CopyTableLine( pLine, &aPara );
    }
    else if (pNewBox->IsInHeadline(&pCT->m_pTableNd->GetTable()))
    {
        // In the headline, the paragraphs must match conditional styles
        pNewBox->GetSttNd()->CheckSectionCondColl();
    }
}

static void lcl_CopyTableLine( const SwTableLine* pLine, CopyTable* pCT )
{
    SwTableLineFormat * pLineFormat = pLine->GetFrameFormat();
    for (const auto& rMap : pCT->m_rMapArr)
        if ( !lcl_SrchNew( rMap, reinterpret_cast<SwFrameFormat**>(&pLineFormat) ) )
            break;

    if( pLineFormat == pLine->GetFrameFormat() ) // Create a new one?
    {
        pLineFormat = pCT->m_rDoc.MakeTableLineFormat();
        pLineFormat->CopyAttrs( *pLine->GetFrameFormat() );
        pCT->m_rMapArr.emplace_back(pLine->GetFrameFormat(), pLineFormat);
    }

    SwTableLine* pNewLine = new SwTableLine(pLineFormat, pLine->GetTabBoxes().size(), pCT->m_pInsBox);
    // Insert the new row into the table
    if (pCT->m_pInsBox)
    {
        pCT->m_pInsBox->GetTabLines().push_back(pNewLine);
    }
    else
    {
        pCT->m_pTableNd->GetTable().GetTabLines().push_back(pNewLine);
    }

    pCT->m_pInsLine = pNewLine;
    for( auto& rpBox : const_cast<SwTableLine*>(pLine)->GetTabBoxes() )
        lcl_CopyTableBox(rpBox, pCT);
}

SwTableNode* SwTableNode::MakeCopy( SwDoc& rDoc, const SwNodeIndex& rIdx ) const
{
    // In which array are we? Nodes? UndoNodes?
    SwNodes& rNds = const_cast<SwNodes&>(GetNodes());

    if( rIdx < rDoc.GetNodes().GetEndOfInserts().GetIndex() &&
        rIdx >= rDoc.GetNodes().GetEndOfInserts().StartOfSectionIndex() )
        return nullptr;

    // Copy the TableFrameFormat
    UIName sTableName( GetTable().GetFrameFormat()->GetName() );
    if( !rDoc.IsCopyIsMove() )
    {
        const sw::TableFrameFormats& rTableFormats = *rDoc.GetTableFrameFormats();
        for( size_t n = rTableFormats.size(); n; )
        {
            const SwTableFormat* pFormat = rTableFormats[--n];
            if (pFormat->GetName() == sTableName && rDoc.IsUsed(*pFormat))
            {
                sTableName = rDoc.GetUniqueTableName();
                break;
            }
        }
    }

    SwFrameFormat* pTableFormat = rDoc.MakeTableFrameFormat( sTableName, rDoc.GetDfltFrameFormat() );
    pTableFormat->CopyAttrs( *GetTable().GetFrameFormat() );
    SwTableNode* pTableNd = new SwTableNode( rIdx.GetNode() );
    SwEndNode* pEndNd = new SwEndNode( rIdx.GetNode(), *pTableNd );
    SwNodeIndex aInsPos( *pEndNd );

    SwTable& rTable = pTableNd->GetTable();
    rTable.SetTableStyleName(GetTable().GetTableStyleName());
    rTable.RegisterToFormat( *pTableFormat );

    rTable.SetRowsToRepeat( GetTable().GetRowsToRepeat() );
    rTable.SetTableChgMode( GetTable().GetTableChgMode() );
    rTable.SetTableModel( GetTable().IsNewModel() );

    SwDDEFieldType* pDDEType = nullptr;
    if( auto pSwDDETable = dynamic_cast<const SwDDETable*>( &GetTable() ) )
    {
        // We're copying a DDE table
        // Is the field type available in the new document?
        pDDEType = const_cast<SwDDETable*>(pSwDDETable)->GetDDEFieldType();
        if( pDDEType->IsDeleted() )
            rDoc.getIDocumentFieldsAccess().InsDeletedFieldType( *pDDEType );
        else
            pDDEType = static_cast<SwDDEFieldType*>(rDoc.getIDocumentFieldsAccess().InsertFieldType( *pDDEType ));
        OSL_ENSURE( pDDEType, "unknown FieldType" );

        // Swap the table pointers in the node
        std::unique_ptr<SwDDETable> pNewTable(new SwDDETable( pTableNd->GetTable(), pDDEType ));
        pTableNd->SetNewTable( std::move(pNewTable), false );
    }
    // First copy the content of the tables, we will later assign the
    // boxes/lines and create the frames
    SwNodeRange aRg( *this, SwNodeOffset(+1), *EndOfSectionNode() );

    // If there is a table in this table, the table format for the outer table
    // does not seem to be used, because the table does not have any contents yet
    // (see IsUsed). Therefore the inner table gets the same name as the outer table.
    // We have to make sure that the table node of the SwTable is accessible, even
    // without any content in m_TabSortContentBoxes. #i26629#
    pTableNd->GetTable().SetTableNode( pTableNd );
    rNds.Copy_( aRg, aInsPos.GetNode(), false );
    pTableNd->GetTable().SetTableNode( nullptr );

    // Special case for a single box
    if( 1 == GetTable().GetTabSortBoxes().size() )
    {
        aRg.aStart.Assign( *pTableNd, 1 );
        aRg.aEnd.Assign( *pTableNd->EndOfSectionNode() );
        rDoc.GetNodes().SectionDown( &aRg, SwTableBoxStartNode );
    }

    // Delete all frames from the copied area, they will be created
    // during the generation of the table frame
    pTableNd->DelFrames();

    MapTableFrameFormats aMapArr;
    CopyTable aPara( rDoc, aMapArr, GetIndex(), *pTableNd, &GetTable() );

    for( const SwTableLine* pLine : GetTable().GetTabLines() )
        lcl_CopyTableLine( pLine, &aPara );

    if( pDDEType )
        pDDEType->IncRefCnt();

    CHECK_TABLE( GetTable() );
    return pTableNd;
}

void SwTextNode::CopyCollFormat(SwTextNode& rDestNd, bool const bUndoForChgFormatColl)
{
    // Copy the formats into the other document:
    // Special case for PageBreak/PageDesc/ColBrk
    SwDoc& rDestDoc = rDestNd.GetDoc();
    SwAttrSet aPgBrkSet( rDestDoc.GetAttrPool(), aBreakSetRange );
    const SwAttrSet* pSet;

    pSet = rDestNd.GetpSwAttrSet();
    if( nullptr != pSet )
    {
        // Special cases for Break-Attributes
        const SfxPoolItem* pAttr;
        if( SfxItemState::SET == pSet->GetItemState( RES_BREAK, false, &pAttr ) )
            aPgBrkSet.Put( *pAttr );

        if( SfxItemState::SET == pSet->GetItemState( RES_PAGEDESC, false, &pAttr ) )
            aPgBrkSet.Put( *pAttr );
    }

    // this may create undo action SwUndoFormatCreate
    auto const pCopy( rDestDoc.CopyTextColl( *GetTextColl() ) );
    if (bUndoForChgFormatColl)
    {
        rDestNd.ChgFormatColl(pCopy);
    }
    else // tdf#138897
    {
        ::sw::UndoGuard const ug(rDestDoc.GetIDocumentUndoRedo());
        rDestNd.ChgFormatColl(pCopy);
    }
    pSet = GetpSwAttrSet();
    if( nullptr != pSet )
    {
        // note: this may create undo actions but not for setting the items
        pSet->CopyToModify( rDestNd );
    }

    if( aPgBrkSet.Count() )
        rDestNd.SetAttr( aPgBrkSet );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
