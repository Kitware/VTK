# Multi line and column support in MathText

The MatplotlibMathTextUtilities have been updated to support rendering of multiline and multicolumn text.
You can now render equations with multiple lines defined by '\n' and multiple columns defined by a pipe '|'.
The vtkTextProperty has been updated to control the space between columns via the new property "CellOffset",
expressed in pixels unit.
