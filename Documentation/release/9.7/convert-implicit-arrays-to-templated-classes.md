## Convert implicit arrays to templated classes

In the past, the following arrays were not concrete templated classes but typedefs:

1. `vtkAffineArray`
2. `vtkCompositeArray`
3. `vtkConstantArray`
4. `vtkIndexedArray`
5. `vtkStdFunctionArray`
6. `vtkStridedArray`
7. `vtkStructuredPointArray`

This work converts these arrays to concrete templated classes, similar to `vtk(AOS/SOA)DataArrayTemplate`. This allows
you to instantiate them in Python.
