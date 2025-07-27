# Spell Check Feature Testing Guide

## Prerequisites
1. Build completed successfully in WSL
2. X server running on Windows (VcXsrv or similar)

## Running LibreOffice
```bash
# In WSL terminal:
cd ~/libreoffice/core-master
export DISPLAY=:0
./instdir/program/soffice --writer
```

## Test 1: Increased Suggestion Limit (7 → 10)

1. Open Writer
2. Type one of these misspelled words:
   - "recieve" 
   - "occured"
   - "pronounciation"
3. Right-click on the red-underlined word
4. **Expected**: Up to 10 suggestions in the context menu (previously limited to 7)

## Test 2: Learn from Document Feature

1. Open the test document: File → Open → `/mnt/c/Users/drury/Documents/GitHub/core-master/spellcheck-test.txt`
2. Open spell check dialog: Tools → Spelling (F7)
3. **New Button**: Look for "Learn from Document" button in the dialog
4. Click "Learn from Document"
5. **Expected**: A dialog appears showing:
   - blockchain (6 occurrences)
   - technologie (5 occurrences)
   - transactons (4 occurrences)
   - companys (3 occurrences)
   - industrie (3 occurrences)
   - unpresidented (2 occurrences)
   - implimentation (2 occurrences)
   - transparant (2 occurrences)
   - securitie (2 occurrences)
6. Select which words to add (all checked by default)
7. Click OK
8. **Expected**: Message showing "X words added to dictionary"
9. These words should no longer be marked as misspelled

## Test 3: Verify Spell Check Dialog

1. Press F7 to open spell check dialog
2. Navigate through errors
3. **Verify**:
   - Up to 10 suggestions shown in the suggestion list
   - "Learn from Document" button is visible and functional
   - Button is placed after "Add to Dictionary"

## Test 4: Edge Cases

1. Empty document → "Learn from Document" should show "No repeated unknown words found"
2. Document with no repeated misspellings → Same message
3. Document with 50+ repeated words → Should handle gracefully

## Debugging

If LibreOffice doesn't start:
```bash
# Check for errors
./instdir/program/soffice --writer --norestore 2>&1 | tee debug.log

# Try with SAL_USE_VCLPLUGIN
SAL_USE_VCLPLUGIN=gen ./instdir/program/soffice --writer
```

## Screenshots
Take screenshots of:
1. Context menu showing 10 suggestions
2. Spell dialog with "Learn from Document" button
3. Learn from Document word selection dialog
4. Success message after adding words