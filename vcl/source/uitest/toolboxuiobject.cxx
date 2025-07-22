/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <uitest/toolboxitemuiobject.hxx>
#include <uitest/toolboxuiobject.hxx>

#include <o3tl/string_view.hxx>
#include <vcl/toolbox.hxx>

constexpr OUString TOOLBOX_ITEM_ID_PREFIX = u"toolboxitem-"_ustr;

ToolBoxUIObject::ToolBoxUIObject(const VclPtr<ToolBox>& xToolBox)
    : WindowUIObject(xToolBox)
    , mxToolBox(xToolBox)
{
}

ToolBoxUIObject::~ToolBoxUIObject() {}

void ToolBoxUIObject::execute(const OUString& rAction, const StringMap& rParameters)
{
    if (rAction == "CLICK")
    {
        if (rParameters.find(u"POS"_ustr) != rParameters.end())
        {
            auto itr = rParameters.find(u"POS"_ustr);
            sal_uInt16 nPos = itr->second.toUInt32();
            mxToolBox->SetCurItemId(mxToolBox->GetItemId(nPos));
            mxToolBox->Click();
            mxToolBox->Select();
        }
    }
    else
        WindowUIObject::execute(rAction, rParameters);
}

OUString ToolBoxUIObject::get_action(VclEventId nEvent) const
{
    if (nEvent == VclEventId::ToolboxClick)
    {
        return "Click on item number " + OUString::number(sal_uInt16(mxToolBox->GetCurItemId()))
               + " in " + mxToolBox->get_id();
    }
    else
        return WindowUIObject::get_action(nEvent);
}

StringMap ToolBoxUIObject::get_state()
{
    StringMap aMap = WindowUIObject::get_state();
    ToolBoxItemId nCurItemId = mxToolBox->GetCurItemId();
    aMap[u"CurrSelectedItemID"_ustr] = OUString::number(sal_uInt16(nCurItemId));
    aMap[u"CurrSelectedItemText"_ustr] = nCurItemId ? mxToolBox->GetItemText(nCurItemId) : u""_ustr;
    aMap[u"CurrSelectedItemCommand"_ustr]
        = nCurItemId ? mxToolBox->GetItemCommand(nCurItemId) : u""_ustr;
    aMap[u"ItemCount"_ustr] = OUString::number(mxToolBox->GetItemCount());
    return aMap;
}

std::set<OUString> ToolBoxUIObject::get_children() const
{
    std::set<OUString> aChildren = WindowUIObject::get_children();

    const size_t nItemCount = mxToolBox->GetItemCount();
    for (size_t i = 0; i < nItemCount; ++i)
    {
        const OUString sId
            = TOOLBOX_ITEM_ID_PREFIX + OUString::number(sal_uInt16(mxToolBox->GetItemId(i)));
        aChildren.insert(sId);
    }

    return aChildren;
}

std::unique_ptr<UIObject> ToolBoxUIObject::get_child(const OUString& rID)
{
    if (rID.startsWith(TOOLBOX_ITEM_ID_PREFIX))
    {
        const sal_Int32 nPrefixLength = TOOLBOX_ITEM_ID_PREFIX.getLength();
        const ToolBoxItemId nItemId(o3tl::toInt32(rID.subView(nPrefixLength)));
        return std::make_unique<ToolBoxItemUIObject>(mxToolBox, nItemId);
    }

    return WindowUIObject::get_child(rID);
}

OUString ToolBoxUIObject::get_name() const { return u"ToolBoxUIObject"_ustr; }

std::unique_ptr<UIObject> ToolBoxUIObject::create(vcl::Window* pWindow)
{
    ToolBox* pToolBox = dynamic_cast<ToolBox*>(pWindow);
    assert(pToolBox);
    return std::unique_ptr<UIObject>(new ToolBoxUIObject(pToolBox));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
