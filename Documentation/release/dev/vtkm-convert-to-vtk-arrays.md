## Convert vtkmDataArrays to ordinary VTK arrays

Arrays a Viskores-accelerated filter computes can come back as
`vtkmDataArray`s, which wrap a Viskores array handle and may hold their data
on a GPU. They support the `vtkDataArray` API, but code that needs a raw host
pointer, or that down-casts to a concrete array type such as
`vtkAOSDataArrayTemplate`, cannot use them directly.

`vtkmDataArray::NewVTKArray()` makes an ordinary VTK array holding the same
values, which the caller owns:

- a constant handle becomes a `vtkConstantArray`, and a counting or index
  handle a `vtkAffineArray`, so nothing is materialized;
- a struct-of-arrays handle becomes a `vtkSOADataArrayTemplate`;
- anything else becomes an interleaved `vtkAOSDataArrayTemplate`.

Data on a device are copied to the host once, and the host copy is kept in
the Viskores handle, so reading the `vtkmDataArray` on the host afterwards
does not transfer it again.

The new `vtkmConvertToVTKArrays` filter applies this to a whole dataset: its
output is a shallow copy of its input in which every `vtkmDataArray` -- in
point, cell and field data, point coordinates, rectilinear coordinates, and
the cells and cell types of polydata and unstructured grids -- is replaced by
such a copy. Active attributes are kept, and the input is not modified. Put
it where a pipeline's data leave Viskores for code that needs host arrays.
