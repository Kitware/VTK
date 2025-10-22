## vtkArrayDispatch: Dispatch known Arrays with O(1) complexity

The `vtkArrayDispatch` mechanism provides a way to dispatch 1/2/3 array(s) such that
a `vtkDataArray` is converted to a known array type (e.g. `vtkAOSDataArrayTemplate<double>`) for faster access.
This is usually done by using a compile-time list of known arrays stored in `vtkArrayDispatch::Arrays` or a list that is
given by the user.

In order for that to happen, the `vtkArrayDispatch` mechanism needs to attempt to use `vtkArrayDownCast<ArrayType>` on
as many of the given array as needed until a match is found. This means that the time complexity of the dispatching is
O(N) where N is the number of given arrays to check, O(N^2) if 2 arrays are dispatched, and O(N^3) if 3 arrays are
dispatched.

That is perfectly fine if the dispatching cost is negligible compared to the cost of the operation that is performed on
the array(s). However, if the operation is very small, e.g. only on 1 or a small number of points/cells, the dispatching
cost can become significant.

To solve this problem, `vtkArrayDispatch` mechanism has been optimized to O(1) complexity, regardless of the number of
given arrays. This was accomplished by adding 2 compile time known tags to each VTK array: `ArrayTypeTag` and
`DataTypeTag`, and ensuring that all concrete `vtkAbstractArray` subclasses have a `ValueType` typedef too.

1. `vtkAbstractArray`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_ABSTRACT_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_OPAQUE>`
2. `vtkStringArray`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_STRING_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_STRING>`
   3. `ValueType = vtkStdString`
3. `vtkVariantArray`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_VARIANT_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_VARIANT>`
   3. `ValueType = vtkVariant`
4. `vtkDataArray`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_DATA_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_OPAQUE>`
5. `vtkBitArray`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_BIT_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_BIT>`\
   3. `ValueType = unsigned char`
6. `vtkAOSDataArrayTemplate`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_AOS_DATA_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
   3. `ValueType` is the template parameter of `vtkAOSDataArrayTemplate`
7. `vtkSOADataArrayTemplate`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_SOA_DATA_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
   3. `ValueType` is the template parameter of `vtkSOADataArrayTemplate`
8. `vtkScaledSoADataArrayTemplate`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_SCALED_SOA_DATA_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
   3. `ValueType` is the template parameter of `vtkScaledSoADataArrayTemplate`
9. `vtkmDataArray`:
   1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTKM_DATA_ARRAY>`,
   2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
   3. `ValueType` is the template parameter of `vtkmDataArray`
10. `vtkPeriodicDataArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_PERIODIC_DATA_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkPeriodicDataArray`
11. `vtkImplicitArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_IMPLICIT_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkImplicitArray`
12. `vtkAffineArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_AFFINE_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkAffineArray`
13. `vtkCompositeArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_COMPOSITE_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkCompositeArray`
14. `vtkConstantArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_CONSTANT_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkConstantArray`
15. `vtkIndexedArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_INDEXED_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkIndexedArray`
16. `vtkStdFunctionArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_STD_FUNCTION_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkStdFunctionArray`
17. `vtkStridedArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_STRIDED_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkStridedArray`
18. `vtkStructuredPointArray`:
    1. `ArrayTypeTag = std::integral_constant<int, vtkArrayTypes::VTK_STRUCTURED_POINT_ARRAY>`,
    2. `DataTypeTag = std::integral_constant<int, vtkTypeTraits<ValueType>::VTK_TYPE_ID>>`
    3. `ValueType` is the template parameter of `vtkStructuredPointArray`

With these tags, the dispatching can be done in O(1) by creating at compile-time a map from
(`GetArrayType()`, `GetDataType()`) to (`ArrayTypeTag`, `DataTypeTag`), such that we can directly access the correct
array subclass without having to try each one sequentially.

It should be noted that because of this optimization, and the fact that all concrete `vtkAbstractArray` subclasses now
have a `ValueType`, all of the `vtkArrayDispatch::Dispatch*` functions now accept any `vtkAbstractArray` arrays,
e.g. `vtkStringArray` and `vtkVariantArray`, and not just `vtkDataArray` subclasses as before.

Finally, the following list of dispatch function have deprecated:

1. `DispatchByValueTypeUsingArrays`: Use `DispatchByArrayAndValueType` instead.
2. `Dispatch2ByValueTypeUsingArrays`: Use `Dispatch2ByArrayAndValueType` instead.
3. `Dispatch2SameValueTypeUsingArrays`: Use `Dispatch2ByArrayWithSameValueType` instead.
4. `Dispatch2BySameValueTypeUsingArrays`: Use `Dispatch2ByArrayAndSameValueType` instead.
5. `Dispatch3ByValueTypeUsingArrays`: Use `Dispatch3ByArrayAndValueType` instead.
6. `Dispatch3SameValueTypeUsingArrays`: Use `Dispatch3ByArrayWithSameValueType` instead.
7. `Dispatch3BySameValueTypeUsingArrays`: Use `Dispatch3ByArraySameValue` instead.
