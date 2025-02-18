## Add `vtkBitArray` support to `numpy_support`

Array conversion between vtk and numpy (using `vtk_to_numpy`) now supports `vtkBitArray`.
Converting a `vtkBitArray` to numpy results in an array of uint8.
