/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <svx/svdorect.hxx>
#include <vcl/pdfwriter.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>

namespace sd {

enum class FormControlType
{
    PushButton,
    CheckBox,
    RadioButton,
    TextField,
    ListBox,
    ComboBox
};

class SdFormControlShape : public SdrRectObj
{
private:
    FormControlType meType;
    OUString maName;
    OUString maText;
    OUString maDescription;
    bool mbReadOnly;
    bool mbRequired;
    bool mbMultiLine;
    
    // Form-specific properties
    std::vector<OUString> maListItems;  // For ListBox/ComboBox
    OUString maGroupName;               // For RadioButton groups
    bool mbChecked;                     // For CheckBox/RadioButton
    sal_Int32 mnMaxLength;              // For TextField
    OUString maDefaultValue;
    
    // PDF export properties
    vcl::Font maFont;
    Color maTextColor;
    Color maBorderColor;
    Color maBackgroundColor;
    bool mbBorder;
    bool mbBackground;

public:
    SdFormControlShape(SdrModel& rSdrModel, FormControlType eType);
    SdFormControlShape(SdrModel& rSdrModel, const SdFormControlShape& rSource);
    virtual ~SdFormControlShape() override;

    // SdrObject overrides
    virtual SdrObjKind GetObjIdentifier() const override;
    virtual OUString TakeObjNameSingul() const override;
    virtual OUString TakeObjNamePlural() const override;
    virtual rtl::Reference<SdrObject> CloneSdrObject(SdrModel& rTargetModel) const override;
    
    // Property access
    FormControlType GetFormControlType() const { return meType; }
    void SetFormControlType(FormControlType eType) { meType = eType; }
    
    const OUString& GetFieldName() const { return maName; }
    void SetFieldName(const OUString& rName) { maName = rName; }
    
    const OUString& GetText() const { return maText; }
    void SetText(const OUString& rText) { maText = rText; }
    
    const OUString& GetFieldDescription() const { return maDescription; }
    void SetFieldDescription(const OUString& rDesc) { maDescription = rDesc; }
    
    bool IsReadOnly() const { return mbReadOnly; }
    void SetReadOnly(bool bReadOnly) { mbReadOnly = bReadOnly; }
    
    bool IsRequired() const { return mbRequired; }
    void SetRequired(bool bRequired) { mbRequired = bRequired; }
    
    bool IsMultiLine() const { return mbMultiLine; }
    void SetMultiLine(bool bMultiLine) { mbMultiLine = bMultiLine; }
    
    bool IsChecked() const { return mbChecked; }
    void SetChecked(bool bChecked) { mbChecked = bChecked; }
    
    const std::vector<OUString>& GetListItems() const { return maListItems; }
    void SetListItems(const std::vector<OUString>& rItems) { maListItems = rItems; }
    void AddListItem(const OUString& rItem) { maListItems.push_back(rItem); }
    
    const OUString& GetGroupName() const { return maGroupName; }
    void SetGroupName(const OUString& rName) { maGroupName = rName; }
    
    sal_Int32 GetMaxLength() const { return mnMaxLength; }
    void SetMaxLength(sal_Int32 nMaxLength) { mnMaxLength = nMaxLength; }
    
    const OUString& GetDefaultValue() const { return maDefaultValue; }
    void SetDefaultValue(const OUString& rValue) { maDefaultValue = rValue; }
    
    // Visual properties
    const vcl::Font& GetFont() const { return maFont; }
    void SetFont(const vcl::Font& rFont) { maFont = rFont; }
    
    Color GetTextColor() const { return maTextColor; }
    void SetTextColor(Color nColor) { maTextColor = nColor; }
    
    Color GetBorderColor() const { return maBorderColor; }
    void SetBorderColor(Color nColor) { maBorderColor = nColor; }
    
    Color GetBackgroundColor() const { return maBackgroundColor; }
    void SetBackgroundColor(Color nColor) { maBackgroundColor = nColor; }
    
    bool HasBorder() const { return mbBorder; }
    void SetBorder(bool bBorder) { mbBorder = bBorder; }
    
    bool HasBackground() const { return mbBackground; }
    void SetBackground(bool bBackground) { mbBackground = bBackground; }
    
    // Convert to PDF widget for export
    void FillPDFAnyWidget(vcl::PDFWriter::AnyWidget& rWidget) const;
    
    // UNO property support
    css::uno::Sequence<css::beans::PropertyValue> GetFormProperties() const;
    void SetFormProperties(const css::uno::Sequence<css::beans::PropertyValue>& rProps);
};

} // namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */