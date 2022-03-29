## Disable xdmf reader caching

`vtkXdmfReader` now releases the memory of the `XdfmGrid`, after creating the output `vtkDataObject` out of it.
This way caching is disabled, which is useful for large datasets, because otherwise you can run out of memory.

This MR resolves issue https://gitlab.kitware.com/paraview/paraview/-/issues/19633.
