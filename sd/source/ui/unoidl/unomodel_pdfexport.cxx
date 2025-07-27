/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <unomodel.hxx>
#include <formcontrolshape.hxx>
#include <drawdoc.hxx>
#include <sdpage.hxx>
#include <vcl/pdfwriter.hxx>
#include <vcl/pdfextoutdevdata.hxx>
#include <svx/svdpage.hxx>
#include <svx/svditer.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <sal/log.hxx>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>

using namespace ::com::sun::star;

namespace sd {

// Extension to handle form controls during PDF export
void ExportFormControlsToPDF(SdDrawDocument* pDoc, vcl::PDFExtOutDevData& rPDFExtOutDevData, 
                             vcl::PDFWriter& /*rWriter*/, sal_Int32 nPageNum)
{
    if (!pDoc || nPageNum < 0 || nPageNum >= pDoc->GetPageCount())
        return;
        
    SdPage* pPage = static_cast<SdPage*>(pDoc->GetPage(static_cast<sal_uInt16>(nPageNum)));
    if (!pPage)
        return;
        
    // Iterate through all objects on the page
    SdrObjListIter aIter(pPage, SdrIterMode::DeepWithGroups);
    while (aIter.IsMore())
    {
        SdrObject* pObj = aIter.Next();
        
        // Check if it's a form control
        SdFormControlShape* pFormControl = dynamic_cast<SdFormControlShape*>(pObj);
        if (pFormControl)
        {
            try
            {
                // Create PDF widget from form control
                
                // Handle special cases for different control types
                switch (pFormControl->GetFormControlType())
                {
                    case FormControlType::PushButton:
                    {
                        vcl::PDFWriter::PushButtonWidget aPushButton;
                        pFormControl->FillPDFAnyWidget(aPushButton);
                        
                        // Add submit action if needed
                        if (pFormControl->GetFieldName().startsWith("Submit"))
                        {
                            aPushButton.Submit = true;
                            aPushButton.SubmitGet = false; // Use POST
                            aPushButton.URL = ""; // Would be set from properties
                        }
                        
                        rPDFExtOutDevData.CreateControl(aPushButton);
                    }
                    break;
                    
                    case FormControlType::CheckBox:
                    {
                        vcl::PDFWriter::CheckBoxWidget aCheckBox;
                        pFormControl->FillPDFAnyWidget(aCheckBox);
                        aCheckBox.Checked = pFormControl->IsChecked();
                        rPDFExtOutDevData.CreateControl(aCheckBox);
                    }
                    break;
                    
                    case FormControlType::RadioButton:
                    {
                        vcl::PDFWriter::RadioButtonWidget aRadioButton;
                        pFormControl->FillPDFAnyWidget(aRadioButton);
                        aRadioButton.RadioGroup = pFormControl->GetGroupName().isEmpty() ? 
                            sal_Int32(0) : pFormControl->GetGroupName().hashCode();
                        aRadioButton.OnValue = pFormControl->IsChecked() ? "Yes" : "Off";
                        rPDFExtOutDevData.CreateControl(aRadioButton);
                    }
                    break;
                    
                    case FormControlType::TextField:
                    {
                        vcl::PDFWriter::EditWidget aEdit;
                        pFormControl->FillPDFAnyWidget(aEdit);
                        aEdit.MultiLine = pFormControl->IsMultiLine();
                        aEdit.MaxLen = pFormControl->GetMaxLength();
                        aEdit.Format = vcl::PDFWriter::Text;
                        rPDFExtOutDevData.CreateControl(aEdit);
                    }
                    break;
                    
                    case FormControlType::ListBox:
                    {
                        vcl::PDFWriter::ListBoxWidget aListBox;
                        pFormControl->FillPDFAnyWidget(aListBox);
                        aListBox.DropDown = false;
                        aListBox.MultiSelect = false;
                        
                        // Add list items
                        for (const auto& rItem : pFormControl->GetListItems())
                        {
                            aListBox.Entries.push_back(rItem);
                        }
                        
                        if (!aListBox.Entries.empty())
                            aListBox.SelectedEntries.push_back(0); // Select first item
                            
                        rPDFExtOutDevData.CreateControl(aListBox);
                    }
                    break;
                    
                    case FormControlType::ComboBox:
                    {
                        vcl::PDFWriter::ComboBoxWidget aComboBox;
                        pFormControl->FillPDFAnyWidget(aComboBox);
                        
                        // Add list items
                        for (const auto& rItem : pFormControl->GetListItems())
                        {
                            aComboBox.Entries.push_back(rItem);
                        }
                        
                        rPDFExtOutDevData.CreateControl(aComboBox);
                    }
                    break;
                }
                
                SAL_INFO("sd.pdf", "Exported form control: " << pFormControl->GetFieldName());
            }
            catch (const uno::Exception& e)
            {
                SAL_WARN("sd.pdf", "Failed to export form control: " << e.Message);
            }
        }
    }
}

// Note: To integrate this into the PDF export process, the main PDF export code
// in filter/source/pdf/pdfexport.cxx would need to call ExportFormControlsToPDF
// for each page during the export process.

} // namespace sd

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */