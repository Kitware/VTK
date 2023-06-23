## Improve vtkVariant conversion functions performance

Thanks to `vtkValueFromString`, `vtkVariant::ToX` functions now have better performances when converting a variant holding a string to an integer or a floating-point value.
The performances gains are higher for the conversion to floating-point values (x4 times faster average), but there is a gain for integers too.

This indirectly improves some code that uses vtkVariant for convering values, such as `vtkDelimitedTextReader` with detect numeric columns option.

Note: `[-]infinity` is no longer supported when converting a string to a floating-point value. Only `[-]inf` is supported.
