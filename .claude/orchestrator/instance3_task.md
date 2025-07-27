# Task for Instance 3: PPTX Improvements

## Your Assignment
You are responsible for improving PPTX import/export fidelity in LibreOffice Impress.

## Specific Tasks

1. **Analysis Phase** (First Priority)
   - Examine `oox/source/ppt/` thoroughly
   - Document current limitations in:
     - SmartArt support
     - Animation/transition handling
     - Custom shape geometry
   - Create test presentations that demonstrate issues

2. **SmartArt Implementation**
   - Study SmartArt XML structure in `oox/source/drawingml/`
   - Implement basic SmartArt layouts:
     - Process diagrams
     - Hierarchy charts
     - Lists and matrices
   - Convert to native LibreOffice shapes while preserving appearance

3. **Animation/Transition Enhancements**
   - Extend `oox/source/ppt/animationspersist.cxx`
   - Add missing animation effects:
     - Motion paths
     - Advanced entrance/exit effects
     - Animation triggers
   - Improve slide transition support

4. **Custom Shape Improvements**
   - Fix geometry calculation in `oox/source/drawingml/customshapeproperties.cxx`
   - Improve gradient and pattern fills
   - Better text box positioning within shapes
   - Handle shape effects (shadows, reflections, glows)

## Important Guidelines
- Focus on `oox/source/ppt/` and `oox/source/drawingml/`
- Create unit tests in `sd/qa/unit/`
- Test with real-world PPTX files
- Ensure backward compatibility
- Report progress every 30 minutes

## Output Expected
1. Enhanced oox module with better PPTX support
2. Unit tests for all improvements
3. Sample presentations demonstrating fixes
4. Documentation of implementation approach

## Files You Own
- `oox/source/ppt/*`
- `oox/source/drawingml/diagram/*`
- `oox/source/drawingml/shape*`
- `sd/qa/unit/import-tests*.cxx` (new tests)
- `sd/qa/unit/export-tests*.cxx` (new tests)

Start by analyzing the current SmartArt and animation implementation gaps.