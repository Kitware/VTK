## vtkAbstractArray: Replace most of GetVoidPointer instances

`vtkAbstractArray::GetVoidPointer()` is a function that returns a raw pointer to the underlying data of a VTK array.
This function was added in VTK's early days when it only supported `vtkAOSDataArrayTemplate` arrays. Since then,
VTK has added support for multiple other array types, either explicit (e.g., `vtkSOADataArrayTemplate`,
`vtkScaledSOADataArrayTemplate`, etc.) or implicit (e.g., `vtkAffineArray`, `vtkConstantArray`, etc.). These array types
don't store their data in a contiguous block of memory, so using `GetVoidPointer()` leads to duplication of data in
an internal `vtkAOSDataArrayTemplate` array, which leads to memory usage increase and unnecessary copying overhead.

In attempt to modernize VTK and reduce unnecessary data duplication and copying overhead, ~75% (428/574) of the
`GetVoidPointer()` usages have been removed using one of the following techniques:

1. If array is `vtkAOSDataArrayTemplate`, use `GetPointer()` instead of `GetVoidPointer()`.
2. If array is `vtkSOADataArrayTemplate`, and only a single component is needed, use `GetComponentArrayPointer()`
   instead of `GetVoidPointer()`.
3. If array is `vtkDataArray` and all tuples need to be filled, use `Fill` instead of `memset()` with
   `GetVoidPointer()`.
4. If array is `vtkDataArray` and all tuples need to be copied, use `DeepCopy()/ShallowCopy()` instead of
   `memcpy()` with `GetVoidPointer()`.
5. If array is `vtkAbstractArray` and some tuples need to be copied, use `InsertTuples*()` methods instead of
   `GetVoidPointer()` and `memcpy()`.
6. If two `vtkAbstractArray`s need to be compared, use `vtkTestUtilities::CompareAbstractArray()` instead of
   `GetVoidPointer()` and `std::equal()`.
7. If array is `vtkAbstractArray` and values are needed as `vtkVariant`, use `GetVariantValue()` instead of
   `vtkExtraExtendedTemplateMacro` with `GetVoidPointer()`.
8. If array is `vtkDataArray` and access to the tuples/values is needed, use `vtkArrayDispatch` and
   `vtk::DataArray(Value/Tuple)Range()`, instead of `vtkTemplateMacro` with `GetVoidPointer()`.
9. If array is `vtkDataArray`, the data type is known, and raw pointer to data is absolutely needed, use
   `auto aos = array->ToAOSDataArray()`, and then use `vtkAOSDataArrayTemplate<Type>::FastDownCast(aos)->GetPointer()`,
   instead of `GetVoidPointer()`, being aware that this may lead to data duplication if the array is not
   `vtkAOSDataArrayTemplate`.

Moreover, 17% (25/146) of the remaining `GetVoidPointer()` usages have been marked as _safe_ using the `clang-tidy` lint
`(bugprone-unsafe-functions)` when one of the following conditions is met:

1. If array is `vtkDataArray`, but using `HasStandardMemoryLayout`, it has been verified that it actually is
   `vtkAOSDataArrayTemplate`, then `GetVoidPointer()` is _safe_ to use. This usually happens when the array was created
   using `vtkDataArray::CreateDataArray()` or `vtkAbstractArray::CreateArray()` by passing a standard VTK data type.
2. If array is `vtkDataArray`, the data type is NOT known, but raw pointer to data is absolutely needed, use
   `auto aos = array->ToAOSDataArray()`, and then use `aos->GetVoidPointer()`, which is _safe_, being aware that this
   may lead to data duplication if the array is not `vtkAOSDataArrayTemplate`.

In the future, `GetVoidPointer()` will be marked as an unsafe function using `clang-tidy`, and its usage will be warned
as unsafe, unless explicitly marked as safe using the `(bugprone-unsafe-functions)` lint, upon guaranteeing that it is
indeed safe to use it in that specific context.
