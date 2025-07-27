/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <DrawViewShell.hxx>
#include <DrawDocShell.hxx>
#include <ViewShellBase.hxx>
#include <formcontrolshape.hxx>
#include <drawdoc.hxx>
#include <sdpage.hxx>
#include <View.hxx>
#include <svx/svxids.hrc>
#include <svx/svdpagv.hxx>
#include <svx/svdundo.hxx>
#include <sfx2/request.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/bindings.hxx>
#include <tools/gen.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>

namespace sd {

void DrawViewShell::InsertFormControl(SfxRequest& rReq)
{
    SdPage* pPage = GetActualPage();
    if (!pPage)
        return;

    FormControlType eType;
    OUString sDefaultText;
    
    switch (rReq.GetSlot())
    {
        case SID_INSERT_PUSHBUTTON:
            eType = FormControlType::PushButton;
            sDefaultText = "Button";
            break;
        case SID_INSERT_CHECKBOX:
            eType = FormControlType::CheckBox;
            sDefaultText = "Check Box";
            break;
        case SID_INSERT_RADIOBUTTON:
            eType = FormControlType::RadioButton;
            sDefaultText = "Option";
            break;
        case SID_INSERT_EDIT:  // Text field
            eType = FormControlType::TextField;
            sDefaultText = "";
            break;
        case SID_INSERT_LISTBOX:
            eType = FormControlType::ListBox;
            sDefaultText = "";
            break;
        case SID_INSERT_COMBOBOX:
            eType = FormControlType::ComboBox;
            sDefaultText = "";
            break;
        default:
            return;
    }

    // Create the form control shape
    rtl::Reference<SdFormControlShape> pFormControl = 
        new SdFormControlShape(*GetDoc(), eType);
    
    // Set default properties
    pFormControl->SetText(sDefaultText);
    
    // Generate unique name
    static sal_Int32 nControlCounter = 0;
    OUString sName;
    switch (eType)
    {
        case FormControlType::PushButton:
            sName = "Button" + OUString::number(++nControlCounter);
            break;
        case FormControlType::CheckBox:
            sName = "CheckBox" + OUString::number(++nControlCounter);
            break;
        case FormControlType::RadioButton:
            sName = "RadioButton" + OUString::number(++nControlCounter);
            break;
        case FormControlType::TextField:
            sName = "TextField" + OUString::number(++nControlCounter);
            break;
        case FormControlType::ListBox:
            sName = "ListBox" + OUString::number(++nControlCounter);
            break;
        case FormControlType::ComboBox:
            sName = "ComboBox" + OUString::number(++nControlCounter);
            break;
    }
    pFormControl->SetFieldName(sName);
    
    // Set default size and position
    Point aPos(5000, 5000); // Center of page roughly
    Size aSize;
    
    switch (eType)
    {
        case FormControlType::PushButton:
            aSize = Size(3000, 1000);
            break;
        case FormControlType::CheckBox:
        case FormControlType::RadioButton:
            aSize = Size(3000, 800);
            break;
        case FormControlType::TextField:
            aSize = Size(5000, 1000);
            break;
        case FormControlType::ListBox:
        case FormControlType::ComboBox:
            aSize = Size(4000, 3000);
            break;
    }
    
    ::tools::Rectangle aRect(aPos, aSize);
    pFormControl->SetLogicRect(aRect);
    
    // Add sample items for list/combo boxes
    if (eType == FormControlType::ListBox || eType == FormControlType::ComboBox)
    {
        pFormControl->AddListItem("Item 1");
        pFormControl->AddListItem("Item 2");
        pFormControl->AddListItem("Item 3");
    }
    
    // Insert into page with undo
    View* pView = GetView();
    if (!pView)
        return;
        
    pView->BegUndo(u"Insert Form Control"_ustr);
    
    pPage->InsertObject(pFormControl.get());
    pView->AddUndo(
        std::make_unique<SdrUndoInsertObj>(*pFormControl));
    
    pView->EndUndo();
    
    // Select the new object
    pView->UnmarkAll();
    pView->MarkObj(pFormControl.get(), pView->GetSdrPageView());
    
    // Update the view
    GetViewFrame()->GetBindings().Invalidate(SID_ATTR_ZOOM);
}

} // namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */