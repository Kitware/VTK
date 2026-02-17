## vtkAbstractArray: Replace most of GetVoidPointer instances (Part 2)

`vtkAbstractArray::GetVoidPointer()` is a function that returns a raw pointer to the underlying data of a VTK array.
This function was added in VTK's early days when it only supported `vtkAOSDataArrayTemplate` arrays. Since then,
VTK has added support for multiple other array types, either explicit (`vtkSOADataArrayTemplate`), or implicit (e.g.,
`vtkAffineArray`, `vtkConstantArray`, etc.). These array types don't store their data in a contiguous block of memory,
so using `GetVoidPointer()` leads to duplication of data in an internal `vtkAOSDataArrayTemplate` array, which leads to
memory usage increase and unnecessary copying overhead.

In VTK 9.6, ~75% (428/574) of `GetVoidPointer()` usages were removed.

In this work, ~48% (70/146) of the remaining `GetVoidPointer()` usages have been removed using one of the following
techniques:

1. If array is `vtkAOSDataArrayTemplate/vtkBitArray/vtkmDataArray`, use `GetPointer()` instead of `GetVoidPointer()`.
2. If array is `vtkDataArray` and access to the tuples/values is needed, use `vtkArrayDispatch` and
   `vtk::DataArray(Value/Tuple)Range()`, instead of `vtkTemplateMacro` with `GetVoidPointer()`.
3. If array is `vtkDataArray`, the data type is known, and raw pointer to data is absolutely needed, use
   `auto aos = array->ToAOSDataArray()`, and then use `vtkAOSDataArrayTemplate<Type>::FastDownCast(aos)->GetPointer()`,
   instead of `GetVoidPointer()`, being aware that this may lead to data duplication if the array is not
   `vtkAOSDataArrayTemplate`.

Moreover, in VTK 9.6, 17% (25/146) of the remaining `GetVoidPointer()` usages were explicitly marked as _safe_ using the
`clang-tidy` lint `(bugprone-unsafe-functions)`.

In this work, we complete that effort by marking the remaining unannotated ~67% (51/76) `GetVoidPointer()` usages as
_safe_, when one of the following conditions is met:

1. If array is `vtkDataArray`, but using `HasStandardMemoryLayout`, it has been verified that it actually is
   `vtkAOSDataArrayTemplate`, then `GetVoidPointer()` is _safe_ to use. This usually happens when the array was created
   using `vtkDataArray::CreateDataArray()` or `vtkAbstractArray::CreateArray()` by passing a standard VTK data type.
2. If array is `vtkDataArray`, the data type is NOT known, but raw pointer to data is absolutely needed, use
   `auto aos = array->ToAOSDataArray()`, and then use `aos->GetVoidPointer()`, which is _safe_, being aware that this
   may lead to data duplication if the array is not `vtkAOSDataArrayTemplate`.
3. If a function or class which uses `GetVoidPointer()` has been deprecated, and its usage is guaranteed to be removed
   in the near future, then it has been marked as _safe_ to avoid warnings from `clang-tidy` about its usage.

Finally, `vtkAbstractArray::GetVoidPointer()` and `vtkDataArray::GetVoidPointer()` have been marked as an _unsafe_
function using `clang-tidy`, and its usage will be warned as such, unless explicitly marked as _safe_ using the
`(bugprone-unsafe-functions)` lint, upon guaranteeing that it is indeed safe to use, if and only if, it satisfies one of
the conditions mentioned above.
