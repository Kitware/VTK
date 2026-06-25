## Removed signed/unsigned errors with type checks

The `vtkVariantInlineOperators.h` header has some functions that give some type
information (such as signed/unsigned or float/int) that operate on identifiers
`VTK_FLOAT`, `VTK_INT`, and such. These are defined as integer literals, and
they can be stored safely in any integer type.

The functions `IsSigned` and `IsFloatingPoint` somewhat arbitrarily operate on
identifiers stored in `int`. This is fine, except these identifiers are
sometimes stored in `unsigned int` elsewhere. When an unsigned integer is passed
to a function that accepts a signed integer, you can get a signed/unsigned
conversion warning from the compiler. These warnings were prolific during
compiles of VTK.

`vtkVariantInlineOperators.h` now contains overloaded versions of `IsSigned` and
`IsFloatingPoint` that accept `unsigned int`. This avoids the warning without
creating ambiguous overloads.
