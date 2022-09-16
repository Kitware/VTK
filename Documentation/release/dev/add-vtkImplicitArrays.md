# New vtkImplicitArrays!

VTK now offers new flexible `vtkImplicitArray` template class that implements the `vtkGenericDataArray` interface. It essentially transforms an implicit function mapping integers to values into a pratically zero cost `vtkDataArray`. This is helpful in cases where one needs to attach data to data sets and memory efficiency is paramount.

Building this new development is optional when compiling VTK and can be activated using the `VTK_BUILD_IMPLICIT_ARRAYS` option.
