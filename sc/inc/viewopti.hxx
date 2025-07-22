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

#pragma once

#include <svx/optgrid.hxx>

#include "scdllapi.h"
#include "optutil.hxx"
#include "global.hxx"

// View options

namespace sc
{
enum class ViewOption : sal_Int32
{
    FORMULAS = 0,
    NULLVALS,
    SYNTAX,
    NOTES,
    NOTEAUTHOR,
    FORMULAS_MARKS,
    VSCROLL,
    HSCROLL,
    TABCONTROLS,
    OUTLINER,
    HEADER,
    GRID,
    GRID_ONTOP,
    HELPLINES,
    ANCHOR,
    PAGEBREAKS,
    SUMMARY,
    // tdf#96854 - move/copy sheet dialog: last used option for action (true: copy, false: move)
    COPY_SHEET,
    THEMEDCURSOR,
};

enum class ViewObjectType : sal_Int32
{
    OLE = 0,
    CHART,
    DRAW,
};

} // end sc

constexpr sal_uInt16 MAX_OPT = sal_uInt16(sc::ViewOption::THEMEDCURSOR) + 1;
constexpr sal_uInt16 MAX_TYPE = sal_uInt16(sc::ViewObjectType::DRAW) + 1;

// SvxGrid options with standard operators

class ScGridOptions : public SvxOptionsGrid
{
public:
                ScGridOptions()  {}
                ScGridOptions( const SvxOptionsGrid& rOpt ) : SvxOptionsGrid( rOpt ) {}

    void                    SetDefaults();
    bool                    operator== ( const ScGridOptions& rOpt ) const;
};

class SC_DLLPUBLIC ScViewRenderingOptions
{
public:
    ScViewRenderingOptions();

    const OUString& GetColorSchemeName() const { return msColorSchemeName; }
    void SetColorSchemeName( const OUString& rName ) { msColorSchemeName = rName; }

    const Color& GetDocColor() const { return maDocumentColor; }
    void SetDocColor(const Color& rDocColor) { maDocumentColor = rDocColor; }

    bool operator==(const ScViewRenderingOptions& rOther) const;

private:
    // The name of the color scheme
    OUString msColorSchemeName;
    // The background color of the document
    Color maDocumentColor;
};

// Options - View

class SC_DLLPUBLIC ScViewOptions
{
public:
                ScViewOptions();
                ScViewOptions( const ScViewOptions& rCpy );
                ~ScViewOptions();

    void                    SetDefaults();

    void SetOption(sc::ViewOption eOption, bool bNew)
    {
        aOptArr[sal_Int32(eOption)] = bNew;
    }
    bool GetOption(sc::ViewOption eOption) const
    {
        return aOptArr[sal_Int32(eOption)];
    }

    void SetObjMode(sc::ViewObjectType eObject, ScVObjMode eMode)
    {
        aModeArr[sal_Int32(eObject)] = eMode;
    }
    ScVObjMode GetObjMode(sc::ViewObjectType eObject) const
    {
        return aModeArr[sal_Int32(eObject)];
    }

    void                    SetGridColor( const Color& rCol, const OUString& rName ) { aGridCol = rCol; aGridColName = rName;}
    Color const &           GetGridColor( OUString* pStrName = nullptr ) const;

    const ScGridOptions&    GetGridOptions() const                      { return aGridOpt; }
    void                    SetGridOptions( const ScGridOptions& rNew ) { aGridOpt = rNew; }
    std::unique_ptr<SvxGridItem> CreateGridItem() const;

    ScViewOptions&          operator=  ( const ScViewOptions& rCpy );
    bool                    operator== ( const ScViewOptions& rOpt ) const;

private:
    bool            aOptArr     [MAX_OPT];
    ScVObjMode      aModeArr    [MAX_TYPE];
    Color           aGridCol;
    OUString        aGridColName;
    ScGridOptions   aGridOpt;
};

// Item for the options dialog - View

class SC_DLLPUBLIC ScTpViewItem final : public SfxPoolItem
{
public:
                ScTpViewItem( const ScViewOptions& rOpt );
                virtual ~ScTpViewItem() override;

    DECLARE_ITEM_TYPE_FUNCTION(ScTpViewItem)
    ScTpViewItem(ScTpViewItem const &) = default;
    ScTpViewItem(ScTpViewItem &&) = default;
    ScTpViewItem & operator =(ScTpViewItem const &) = delete; // due to SfxPoolItem
    ScTpViewItem & operator =(ScTpViewItem &&) = delete; // due to SfxPoolItem

    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual ScTpViewItem*   Clone( SfxItemPool *pPool = nullptr ) const override;

    const ScViewOptions&    GetViewOptions() const { return theOptions; }

private:
    ScViewOptions   theOptions;
};

// CfgItem for View options

class ScViewCfg : public ScViewOptions
{
    ScLinkConfigItem    aLayoutItem;
    ScLinkConfigItem    aDisplayItem;
    ScLinkConfigItem    aGridItem;

    DECL_LINK( LayoutCommitHdl, ScLinkConfigItem&, void );
    DECL_LINK( DisplayCommitHdl, ScLinkConfigItem&, void );
    DECL_LINK( DisplayNotifyHdl, ScLinkConfigItem&, void );
    DECL_LINK( GridCommitHdl, ScLinkConfigItem&, void );
    DECL_LINK( GridNotifyHdl, ScLinkConfigItem&, void );

    void ReadDisplayCfg();
    void ReadGridCfg();

    static css::uno::Sequence<OUString> GetLayoutPropertyNames();
    static css::uno::Sequence<OUString> GetDisplayPropertyNames();
    static css::uno::Sequence<OUString> GetGridPropertyNames();

public:
            ScViewCfg();

    void            SetOptions( const ScViewOptions& rNew );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
