## vtkTransformPolyDataFilter deprecation

vtkTransformPolyDataFilter is deprecated in favor of vtkTransformFilter.
The main API difference is `GetOutput` not returning same type.
Use `vtkTransformFilter::GetPolyDataOutput()` as replacement of
`vtkTransformPolyDataFilter::GetOutput()`
