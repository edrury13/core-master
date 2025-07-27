/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <formcontrolshape.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdpage.hxx>
#include <svx/xfillit0.hxx>
#include <svx/xlineit0.hxx>
#include <svx/xlnwtit.hxx>
#include <svx/xlnclit.hxx>
#include <svx/xflclit.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <comphelper/sequence.hxx>

namespace sd {

// Custom object identifier for form controls
constexpr SdrObjKind SdrObjKind_FormControl = static_cast<SdrObjKind>(0xFF00);

SdFormControlShape::SdFormControlShape(SdrModel& rSdrModel, FormControlType eType)
    : SdrRectObj(rSdrModel)
    , meType(eType)
    , mbReadOnly(false)
    , mbRequired(false)
    , mbMultiLine(false)
    , mbChecked(false)
    , mnMaxLength(0)
    , maTextColor(COL_BLACK)
    , maBorderColor(COL_BLACK)
    , maBackgroundColor(COL_WHITE)
    , mbBorder(true)
    , mbBackground(true)
{
    // Set default appearance based on type
    SetMergedItem(XFillStyleItem(css::drawing::FillStyle_SOLID));
    SetMergedItem(XFillColorItem(OUString(), COL_LIGHTGRAY));
    SetMergedItem(XLineStyleItem(css::drawing::LineStyle_SOLID));
    SetMergedItem(XLineWidthItem(100));
    SetMergedItem(XLineColorItem(OUString(), COL_BLACK));
}

SdFormControlShape::SdFormControlShape(SdrModel& rSdrModel, const SdFormControlShape& rSource)
    : SdrRectObj(rSdrModel, rSource)
    , meType(rSource.meType)
    , maName(rSource.maName)
    , maText(rSource.maText)
    , maDescription(rSource.maDescription)
    , mbReadOnly(rSource.mbReadOnly)
    , mbRequired(rSource.mbRequired)
    , mbMultiLine(rSource.mbMultiLine)
    , maListItems(rSource.maListItems)
    , maGroupName(rSource.maGroupName)
    , mbChecked(rSource.mbChecked)
    , mnMaxLength(rSource.mnMaxLength)
    , maDefaultValue(rSource.maDefaultValue)
    , maFont(rSource.maFont)
    , maTextColor(rSource.maTextColor)
    , maBorderColor(rSource.maBorderColor)
    , maBackgroundColor(rSource.maBackgroundColor)
    , mbBorder(rSource.mbBorder)
    , mbBackground(rSource.mbBackground)
{
}

SdFormControlShape::~SdFormControlShape()
{
}

SdrObjKind SdFormControlShape::GetObjIdentifier() const
{
    return SdrObjKind_FormControl;
}

OUString SdFormControlShape::TakeObjNameSingul() const
{
    OUString sName;
    switch(meType)
    {
        case FormControlType::PushButton:
            sName = "Push Button";
            break;
        case FormControlType::CheckBox:
            sName = "Check Box";
            break;
        case FormControlType::RadioButton:
            sName = "Radio Button";
            break;
        case FormControlType::TextField:
            sName = "Text Field";
            break;
        case FormControlType::ListBox:
            sName = "List Box";
            break;
        case FormControlType::ComboBox:
            sName = "Combo Box";
            break;
    }
    
    if (!maName.isEmpty())
        sName += " (" + maName + ")";
        
    return sName;
}

OUString SdFormControlShape::TakeObjNamePlural() const
{
    switch(meType)
    {
        case FormControlType::PushButton:
            return "Push Buttons";
        case FormControlType::CheckBox:
            return "Check Boxes";
        case FormControlType::RadioButton:
            return "Radio Buttons";
        case FormControlType::TextField:
            return "Text Fields";
        case FormControlType::ListBox:
            return "List Boxes";
        case FormControlType::ComboBox:
            return "Combo Boxes";
    }
    return "Form Controls";
}

rtl::Reference<SdrObject> SdFormControlShape::CloneSdrObject(SdrModel& rTargetModel) const
{
    return new SdFormControlShape(rTargetModel, *this);
}

void SdFormControlShape::FillPDFAnyWidget(vcl::PDFWriter::AnyWidget& rWidget) const
{
    // Common properties
    rWidget.Name = maName;
    rWidget.Description = maDescription;
    rWidget.Text = maText;
    rWidget.ReadOnly = mbReadOnly;
    rWidget.Location = GetCurrentBoundRect();
    rWidget.Border = mbBorder;
    rWidget.BorderColor = maBorderColor;
    rWidget.Background = mbBackground;
    rWidget.BackgroundColor = maBackgroundColor;
    rWidget.TextFont = maFont;
    rWidget.TextColor = maTextColor;
    
    // Text style based on type
    if (meType == FormControlType::TextField && mbMultiLine)
    {
        rWidget.TextStyle = DrawTextFlags::MultiLine | DrawTextFlags::WordBreak | 
                           DrawTextFlags::Left | DrawTextFlags::Top;
    }
    else if (meType == FormControlType::PushButton)
    {
        rWidget.TextStyle = DrawTextFlags::Center | DrawTextFlags::VCenter;
    }
    else
    {
        rWidget.TextStyle = DrawTextFlags::Left | DrawTextFlags::VCenter;
    }
}

css::uno::Sequence<css::beans::PropertyValue> SdFormControlShape::GetFormProperties() const
{
    std::vector<css::beans::PropertyValue> aProps;
    
    css::beans::PropertyValue aProp;
    
    aProp.Name = "Type";
    aProp.Value <<= static_cast<sal_Int32>(meType);
    aProps.push_back(aProp);
    
    aProp.Name = "Name";
    aProp.Value <<= maName;
    aProps.push_back(aProp);
    
    aProp.Name = "Text";
    aProp.Value <<= maText;
    aProps.push_back(aProp);
    
    aProp.Name = "Description";
    aProp.Value <<= maDescription;
    aProps.push_back(aProp);
    
    aProp.Name = "ReadOnly";
    aProp.Value <<= mbReadOnly;
    aProps.push_back(aProp);
    
    aProp.Name = "Required";
    aProp.Value <<= mbRequired;
    aProps.push_back(aProp);
    
    if (meType == FormControlType::TextField)
    {
        aProp.Name = "MultiLine";
        aProp.Value <<= mbMultiLine;
        aProps.push_back(aProp);
        
        aProp.Name = "MaxLength";
        aProp.Value <<= mnMaxLength;
        aProps.push_back(aProp);
    }
    
    if (meType == FormControlType::CheckBox || meType == FormControlType::RadioButton)
    {
        aProp.Name = "Checked";
        aProp.Value <<= mbChecked;
        aProps.push_back(aProp);
    }
    
    if (meType == FormControlType::RadioButton)
    {
        aProp.Name = "GroupName";
        aProp.Value <<= maGroupName;
        aProps.push_back(aProp);
    }
    
    if (meType == FormControlType::ListBox || meType == FormControlType::ComboBox)
    {
        aProp.Name = "Items";
        aProp.Value <<= comphelper::containerToSequence(maListItems);
        aProps.push_back(aProp);
    }
    
    return comphelper::containerToSequence(aProps);
}

void SdFormControlShape::SetFormProperties(const css::uno::Sequence<css::beans::PropertyValue>& rProps)
{
    for (const auto& rProp : rProps)
    {
        if (rProp.Name == "Type")
        {
            sal_Int32 nType = 0;
            if (rProp.Value >>= nType)
                meType = static_cast<FormControlType>(nType);
        }
        else if (rProp.Name == "Name")
            rProp.Value >>= maName;
        else if (rProp.Name == "Text")
            rProp.Value >>= maText;
        else if (rProp.Name == "Description")
            rProp.Value >>= maDescription;
        else if (rProp.Name == "ReadOnly")
            rProp.Value >>= mbReadOnly;
        else if (rProp.Name == "Required")
            rProp.Value >>= mbRequired;
        else if (rProp.Name == "MultiLine")
            rProp.Value >>= mbMultiLine;
        else if (rProp.Name == "MaxLength")
            rProp.Value >>= mnMaxLength;
        else if (rProp.Name == "Checked")
            rProp.Value >>= mbChecked;
        else if (rProp.Name == "GroupName")
            rProp.Value >>= maGroupName;
        else if (rProp.Name == "Items")
        {
            css::uno::Sequence<OUString> aItems;
            if (rProp.Value >>= aItems)
            {
                maListItems.clear();
                maListItems.insert(maListItems.end(), aItems.begin(), aItems.end());
            }
        }
    }
}

} // namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */