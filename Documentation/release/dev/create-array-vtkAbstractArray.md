## `vtkAbstractArray::CreateArray` now outputs `IntN` and `FloatN` type arrays (when possible)

There wasn't a way to safely downcast to `IntN` and `FloatN` type arrays (`vtkTypeInt8Array` for
instance) created by `vtkAbtractArray::CreateArray(int dataType)`. Being able to downcast to such
arrays is especially important for readers, which only have a quantitative description of the
underlying type of the arrays to read: it knows if the array is storing a floating point number or
an integer, and its size per element. VTK defines arrays using this nomenclature,
which are set to inherit from a native-typed data array type counterpart
(`vtkLongLongArray` for instance) matching its element size.
`IntN` or `FloatN` arrays inherit from a native-typed data array share the same
data type. For instance, `VTK_TYPE_INT64`, the data type of
`vtkTypeInt64Array`, either equals `VTK_LONG` or `VTK_LONG_LONG`.

From now on, when calling `vtkAbstractArray::CreateArray(int dataType)`, if `dataType` is
used by any `IntN` or `FloatN` array type, then an instance of this array is created, instead of its
native-typed data array counterpart. This means that it is now safe to do
`vtkArrayDownCast<vtkTypeInt64Array>(vtkAbstractArray::CreateArray(VTK_TYPE_INT64))`.

As a reminder, here is a list of new array types that can be created:
* `vtkTypeInt8Array`
* `vtkTypeUInt8Array`
* `vtkTypeInt16Array`
* `vtkTypeUInt16Array`
* `vtkTypeInt32Array`
* `vtkTypeUInt32Array`
* `vtkTypeInt64Array`
* `vtkTypeUInt64Array`
* `vtkTypeFloat32Array`
* `vtkTypeFloat64Array`
