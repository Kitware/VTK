## Deprecate classes/functions that use vtkAbstractArray::GetVoidPointer()

Several classes and methods have been deprecated to further reduce the usage of `vtkAbstractArray::GetVoidPointer()`.

1. `vtkSMPContourGrid` has been deprecated. Use `vtkContourFilter` instead.
2. `vtkSMPMergePoints` has been deprecated. Use `vtkStaticPointLocator` instead.
3. `vtkSMPMergePolyDataHelper` has been deprecated. Use `vtkAppendPolyData` instead.
4. `vtkPoints(2D)::GetVoidPointer()` have been deprecated. Use `vtkPoints(2D)::GetData()` along with `vtkArrayDispatch`
   instead.
5. `vtkGenericDataArray::GetPointer()` has been deprecated. Use `vtkArrayDispatch` and
   `vtk::DataArray(Tuple/Value)Range` instead.
6. `vtk(Array/Points)Portal` have been deprecated because they are no longer needed.
7. `vtkAbstractArray::ExportToVoidPointer()` has been deprecated. Use DeepCopy with an `vtkAOSDataArrayTemplate`
   instead.
8. `vtkAbstractArray::NewIterator()` along with `vtkArrayIterator`, `vtkBitArrayIterator`, `vtkArrayIteratorTemplate`
   have been deprecated. Use `vtk::DataArrayValueRange` or the array directly instead.
9. `vtkVoidArray` has been deprecated because it is no longer needed.
10. `vtkScaledSOADataArrayTemplate` has been deprecated because it is no longer needed.
11. `vtk(Angular)PeriodicDataArray` have been deprecated because they are no longer needed since
    `vtkAngularPeriodicFilter` generates an implicit array using an internally defined backend.
12. `vtkStdFunctionArray` has been deprecated because it is no longer needed.

Additionally, to further reduce the usage of `vtkAbstractArray::GetVoidPointer()`, `vtk::DataArray(Value/Tuple)Range`
have been extended to support all concrete `vtkAbstractArray` subclasses, including `vtkBitArray`, `vtkStringArray`, and
`vtkVariantArray`. This was accomplished by adding `(Set/Get)Typed(Component/Tuple)`, `GetVariantArray`, and
`ShallowCopy` methods to these classes, and change their implementation so that they use `vtkBuffer` internally, instead
of raw pointers, so that their implementation is similar to `vtkAOSDataArrayTemplate`.

Finally, all `vtkGenericDataArray` subclasses (except `vtkAOSDataArrayTemplate`) now have a `GetVoidPointer()` method
which warns you when you use it because it performs a deep-copy internally. To that end, `vtkSOADataArrayTemplate` no
longer converts to `vtkAOSDataArrayTemplate` when `GetVoidPointer()` is called.
