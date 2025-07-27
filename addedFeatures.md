# Added Features

This document tracks new features and improvements added to LibreOffice.

---

## 1. DOCX noProof Attribute Support

**Date Added:** July 23, 2025  
**Files Modified:**
- `sw/source/writerfilter/dmapper/DomainMapper.cxx`

**What It Does:**
- Adds support for the OOXML `w:noProof` attribute which disables spell and grammar checking
- When a DOCX file contains text marked with `noProof="1"` or `noProof="true"`, LibreOffice will now correctly import this and not show spelling/grammar errors for that text

**Technical Details:**
- The noProof token (`LN_EG_RPrBase_noProof`) was already defined but not implemented
- Implementation sets the character locale properties to `LANGUAGE_NONE` which is LibreOffice's way of disabling proofing
- Sets all three locale types (Western, Asian, Complex) to ensure complete coverage

**Code Changes:**
```cpp
// In DomainMapper.cxx, case NS_ooxml::LN_EG_RPrBase_noProof:
if (nIntValue) 
{
    // LibreOffice disables proofing by setting language to LANGUAGE_NONE
    lang::Locale aLocale(LanguageTag::convertToLocale(LANGUAGE_NONE));
    rContext->Insert(PROP_CHAR_LOCALE, uno::Any(aLocale));
    rContext->Insert(PROP_CHAR_LOCALE_ASIAN, uno::Any(aLocale)); 
    rContext->Insert(PROP_CHAR_LOCALE_COMPLEX, uno::Any(aLocale));
}
```

**Testing:**
Create a DOCX with:
```xml
<w:r>
    <w:rPr><w:noProof w:val="1"/></w:rPr>
    <w:t>Thiss textt shouldd nott bee checkedd</w:t>
</w:r>
```
The misspelled words should not show red underlines after import.

---

## 2. DOCX PRINTDATE and SAVEDATE Field Support

**Date Added:** July 23, 2025  
**Files Modified:**
- `sw/source/writerfilter/dmapper/DomainMapper_Impl.cxx`

**What It Does:**
- Adds proper field service mapping for PRINTDATE and SAVEDATE fields in DOCX import
- PRINTDATE shows when the document was last printed
- SAVEDATE shows when the document was last saved
- Both fields support date/time formatting and can be set as fixed or updating

**Technical Details:**
- Added entries to the field conversion map (lcl_GetFieldConversion)
- PRINTDATE maps to "DocInfo.PrintDateTime" service
- SAVEDATE maps to "DocInfo.ChangeDateTime" service
- The field handling code already existed, only the service mapping was missing

**Code Changes:**
```cpp
// In lcl_GetFieldConversion():
{u"PRINTDATE"_ustr,   {"DocInfo.PrintDateTime",    FIELD_PRINTDATE     }},
{u"SAVEDATE"_ustr,    {"DocInfo.ChangeDateTime",   FIELD_SAVEDATE      }},
```

**Note:**
- FILENAME field was already properly implemented
- AUTHOR field was already properly mapped to "DocInfo.CreateAuthor"

**Testing:**
Create a DOCX with these fields:
```
Last printed: { PRINTDATE \@ "dd/MM/yyyy HH:mm" }
Last saved: { SAVEDATE \@ "dd/MM/yyyy HH:mm" }
Author: { AUTHOR }
Filename: { FILENAME \p }
```

---

<!-- Add more features below as they are implemented -->