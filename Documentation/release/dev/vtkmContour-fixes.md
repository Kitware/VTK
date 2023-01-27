# Fix `ComputeScalars` in vtkmContour filter

The `ComputScalars` parameter was toggling the interpolation and generation of
all of the fields in the output. Whereas, in `vtkContourFilter` it is only used
to enable the generation of the scalar field. This has now been fixed and setting
the option in `vtkmContour` has the same effect as `vtkContourFilter`.
