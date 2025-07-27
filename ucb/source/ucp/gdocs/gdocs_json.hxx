/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_UCB_SOURCE_UCP_GDOCS_JSON_HXX
#define INCLUDED_UCB_SOURCE_UCP_GDOCS_JSON_HXX

#include <rtl/ustring.hxx>
#include <rtl/string.hxx>
#include <map>
#include <vector>

namespace gdocs {

// Simple JSON parser for Google API responses
namespace json {

// Extract a string value from JSON by key
inline OString extractString(const OString& json, const OString& key)
{
    OString searchKey = "\"" + key + "\"";
    sal_Int32 keyPos = json.indexOf(searchKey);
    if (keyPos == -1)
        return OString();
    
    // Find the : after the key
    sal_Int32 colonPos = json.indexOf(':', keyPos);
    if (colonPos == -1)
        return OString();
    
    // Find the opening quote
    sal_Int32 startQuote = json.indexOf('"', colonPos);
    if (startQuote == -1)
        return OString();
    
    // Find the closing quote
    sal_Int32 endQuote = json.indexOf('"', startQuote + 1);
    if (endQuote == -1)
        return OString();
    
    return json.copy(startQuote + 1, endQuote - startQuote - 1);
}

// Extract an integer value from JSON by key
inline sal_Int32 extractInt(const OString& json, const OString& key)
{
    OString searchKey = "\"" + key + "\"";
    sal_Int32 keyPos = json.indexOf(searchKey);
    if (keyPos == -1)
        return 0;
    
    // Find the : after the key
    sal_Int32 colonPos = json.indexOf(':', keyPos);
    if (colonPos == -1)
        return 0;
    
    // Skip whitespace
    sal_Int32 pos = colonPos + 1;
    while (pos < json.getLength() && (json[pos] == ' ' || json[pos] == '\t'))
        pos++;
    
    // Extract number
    sal_Int32 start = pos;
    while (pos < json.getLength() && json[pos] >= '0' && json[pos] <= '9')
        pos++;
    
    if (pos > start)
        return json.copy(start, pos - start).toInt32();
    
    return 0;
}

// Check if JSON contains an error
inline bool hasError(const OString& json)
{
    return json.indexOf("\"error\"") != -1;
}

// Extract error message
inline OString getErrorMessage(const OString& json)
{
    sal_Int32 errorPos = json.indexOf("\"error\"");
    if (errorPos == -1)
        return OString();
    
    // Look for message within error object
    sal_Int32 msgPos = json.indexOf("\"message\"", errorPos);
    if (msgPos != -1)
        return extractString(json, "message");
    
    return OString("Unknown error");
}

} // namespace json
} // namespace gdocs

#endif // INCLUDED_UCB_SOURCE_UCP_GDOCS_JSON_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */