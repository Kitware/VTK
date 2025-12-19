## vtk::DataArrayValueRange: Deprecate data() and improve access when TupleSize is 1

The `vtk::DataArrayValueRange` function returns an object to iterate
over the values of a `vtkDataArray`. Recently, the `data()` method was added
to provide access to the raw pointer of the underlying data array. This method
has been deprecated as it was deemed unsafe due to the internal usage of `GetVoidPointer`
which for non `vtkAOSDataArrayTemplate` subclasses it would duplicate the internal data.
Instead, users are encouraged to use the `begin()` and `end()` methods to access the data safely.

Additionally, for dispatched `vtkDataArray` instances with a `TupleSize` of 1, rather than using internally
`GetTypedTuple` and `SetTypedComponent`, the `operator[]` now uses `GetValue` and `SetValue` for better performance.
