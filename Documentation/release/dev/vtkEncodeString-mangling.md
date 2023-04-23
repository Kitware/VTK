## vtkEncodeString mangling

The `vtk_encode_string` CMake API now supports the `ABI_MANGLE_SYMBOL_BEGIN`,
`ABI_MANGLE_SYMBOL_END`, and `ABI_MANGLE_HEADER` arguments to specify a
mangling mechanism. Previously (where mangling was supported), it was
hard-coded to VTK's own mangling decisions.
