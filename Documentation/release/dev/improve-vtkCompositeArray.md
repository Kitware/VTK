## Improve performance of `vtkCompositeArray`

Flattening out the internal tree structure of the `vtkCompositeImplicitBackend` has increased access performance considerably when dealing with multi-level binary trees by avoiding intermediate calls through the `vtkImplicitArray`structure.

This has led to a significant API change in the `vtkCompositeImplicitBackend` constructor which now takes a `std::vector<vtkDataArray*>` instead of two `vtkDataArray*`s directly leading to better quality of life for users as well.
