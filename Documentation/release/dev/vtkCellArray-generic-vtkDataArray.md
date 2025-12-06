## vtkCellArray: Support offsets/connectivity as vtkDataArray including vtkAffineArray for Offsets

The `vtkCellArray` used to be able to support storing connectivity and offsets that are either `vtkTypedInt32Array` or
`vtkTypedInt64Array`. This was limiting because it did not allow using other `vtkDataArray` subclasses, therefore,
many times a copy to the supported types was needed.

A user can now store the connectivity and offsets as any `vtkDataArray` subclass using
`vtkCellArray::SetData(vtkDataArray* offsets, vtkDataArray* conn)`. This is useful because it allows
storing any user given array without worrying whether VTK will deep copy the data arrays or not.

`vtkCellArray` used to have 2 storage types, but now has the following 5:

1. `Int64` (Old): Offsets `vtkAOSDataArrayTemplate<vtkTypeInt64>`, Connectivity `vtkAOSDataArrayTemplate<vtkTypeInt64>`
2. `Int32` (Old): Offsets `vtkAOSDataArrayTemplate<vtkTypeInt32>`, Connectivity `vtkAOSDataArrayTemplate<vtkTypeInt32>`
3. `FixedSizeInt64` (New): Offsets `vtkAffineArray<vtkTypeInt64>`, Connectivity `vtkAOSDataArrayTemplate<vtkTypeInt64>`
4. `FixedSizeInt32` (New): Offsets `vtkAffineArray<vtkTypeInt32>`, Connectivity `vtkAOSDataArrayTemplate<vtkTypeInt32>`
5. `Generic` (New): Offsets `vtkDataArray`, Connectivity `vtkDataArray`

`vtkAffineArray` has become a first class citizen in `vtkCellArray` to support creating a cell array with cells
of constant size without needing to store the offsets explicitly, and therefore saving memory.

The following functions can be used to interact with the new storage types:

1. `bool IsStorage64Bit()`
2. `bool IsStorage32Bit()`
3. `bool IsStorageFixedSize64Bit()`
4. `bool IsStorageFixedSize32Bit()`
5. `bool IsStorageFixedSize()`
6. `bool IsStorageGeneric()`
7. `void Use64BitStorage()`
8. `void Use32BitStorage()`
9. `void UseFixedSize32BitStorage(int size)`
10. `void UseFixedSize64BitStorage(int size)`
11. `bool CanConvertTo64BitStorage()`
12. `bool CanConvertTo32BitStorage()`
13. `bool CanConvertToDefaultStorage()`
14. `bool CanConvertToFixedSize32BitStorage()`
15. `bool CanConvertToFixedSize64BitStorage()`
16. `bool CanConvertToFixedSizeStorage()`
17. `void ConvertTo64BitStorage()`
18. `void ConvertTo32BitStorage()`
19. `void ConvertToDefaultStorage()`
20. `void ConvertToFixedSize64BitStorage()`
21. `void ConvertToFixedSize32BitStorage()`
22. `void ConvertToFixedSizeStorage()`
23. `bool ConvertToStorageType(int type)`
24. `vtkAOSDataArrayTemplate<vtkTypeInt64>* GetOffsetsAOSArray64()`
25. `vtkAOSDataArrayTemplate<vtkTypeInt32>* GetConnectivityAOSArray32()`
26. `vtkAffineArray<vtkTypeInt32>* GetOffsetsAffineArray32()`
27. `vtkAffineArray<vtkTypeInt64>* GetOffsetsAffineArray64()`
28. `vtkDataArray* GetOffsetsArray()`
29. `vtkAOSDataArrayTemplate<vtkTypeInt64>* GetConnectivityAOSArray64()`
30. `vtkAOSDataArrayTemplate<vtkTypeInt32>* GetConnectivityAOSArray32()`
31. `vtkDataArray* GetConnectivityArray()`
32. `int GetStorageType()`

The following methods have been deprecated in favor of the new API:

1. (old) `vtkTypeInt32Array* GetOffsetsArray32()`, (new) `vtkAOSDataArrayTemplate<vtkTypeInt32>* GetOffsetsAOSArray32()`
2. (old) `vtkTypeInt64Array* GetOffsetsArray64()`, (new) `vtkAOSDataArrayTemplate<vtkTypeInt64>* GetOffsetsAOSArray64()`
3. (old) `vtkTypeInt32Array* GetConnectivityArray32()`, (new)
   `vtkAOSDataArrayTemplate<vtkTypeInt32>* GetConnectivityAOSArray32()`
4. (old) `vtkTypeInt64Array* GetConnectivityArray64()`, (new)
   `vtkAOSDataArrayTemplate<vtkTypeInt64>* GetConnectivityAOSArray64()`

This was done to allow, when possible, to store the given array as is without needing to shallow copy it to either
`vtkTypeInt32Array` or `vtkTypeInt64Array`.

When the storage type is `Int64`, `Int32`, `FixedSizeInt64` or `FixedSizeInt32` then direct access to the array is
available through the `Visit` functor. When the storage type is `Generic`, then the `Visit` functor will provide the
arrays as `vtkDataArray`, therefore the user can no longer assume the arrays will be a subclass of
`vtkAOSDataArrayTemplate`. To solve this problem, the array access calls shown in the following `Visit` functor:

```cpp
struct MyVisitor
{
  template <typename CellStateT>
  void operator()(CellStateT& state)
  {
    using ArrayType = typename CellStateT::ArrayType;
    using ValueType = typename CellStateT::ValueType;
    auto* conn = state.GetConnectivity();
    auto connRange = vtk::DataArrayValueRange<1>(conn);
    ValueType connValue = conn->GetValue(0);
    conn->SetValue(0, connValue);
    ValueType* connPtr = conn->GetPointer(0);
    conn->InsertNextValue(connValue);
    vtkIdType numCells = state.GetNumberOfCells();
    vtkIdType beginOffset = state.GetBeginOffset(0);
    vtkIdType endOffset = state.GetEndOffset(0);
    vtkIdType cellSize = state.GetCellSize(0);
    auto cellRange = state.GetCellRange(0);
  }
};
cells->Visit(MyVisitor{});
```

should be replaced with:

```cpp
struct MyVisitor : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn)
  {
    using AccessorType = vtkDataArrayAccessor<ConnectivityT>;
    using ValueType = GetAPIType<ConnectivityT>; // The result of GetAPIType<OffsetsT> is the same
    auto connRange = GetRange(conn);             // or vtk::DataArrayValueRange<1, vtkIdType>(conn)
    ValueType connValue = connRange[0];
    connRange[0] = connValue;
    auto connPtr = connRange.begin();
    AccessorType accessor(conn);
    accessor.InsertNext(connValue);
    vtkIdType numCells = GetNumberOfCells(offsets);
    vtkIdType beginOffset = GetBeginOffset(offsets, 0);
    vtkIdType endOffset = GetEndOffset(offsets, 0);
    vtkIdType cellSize = GetCellSize(offsets, 0);
    auto cellRange = GetCellRange(offsets, conn, 0);
  }
};
cells->Dispatch(MyVisitor{});
// or also
if (!vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<vtkCellArray::OffsetsArrays,
      vtkCellArray::ConnectivityArrays>::Execute(cells->GetOffsetsArray(),
      cells->GetConnectivityArray(), MyVisitor{}))
{
  MyVisitor{}(cells->GetOffsetsArray(), cells->GetConnectivityArray());
}
```

Due to this new requirement, all `Visit` functors in VTK have been updated to use the new `Dispatch` API.

It should be noted that due to this change, offsets and connectivity arrays generated by a VTKm filter can now also
be stored in `vtkCellArray` as `vtkmDataArray` without needing to copy the data to
`vtkAOSDataArrayTemplate<vtkTypeInt32>` or `vtkAOSDataArrayTemplate<vtkTypeInt64>`.

Additionally, the majority of VTK filters that were generating constant size cells have been updated to use
`FixedSizeInt32/64` storage type to save memory.

Finally, the following methods that are associated with the legacy format of `vtkCellArray` have been deprecated:

1. `vtkTypeBool Allocate(vtkIdType sz, vtkIdType vtkNotUsed(ext) = 1000)`
2. `virtual void SetNumberOfCells(vtkIdType)`
3. `vtkIdType EstimateSize(vtkIdType numCells, int maxPtsPerCell)`
4. `vtkIdType GetSize()`
5. `vtkIdType GetNumberOfConnectivityEntries()`
6. `void GetCell(vtkIdType loc, vtkIdType& npts, const vtkIdType*& pts)`
7. `void GetCell(vtkIdType loc, vtkIdList* pts)`
8. `vtkIdType GetInsertLocation(int npts)`
9. `vtkIdType GetTraversalLocation()`
10. `vtkIdType GetTraversalLocation(vtkIdType npts)`
11. `void SetTraversalLocation(vtkIdType loc)`
12. `void ReverseCell(vtkIdType loc)`
13. `void ReplaceCell(vtkIdType loc, int npts, const vtkIdType pts[])`
14. `void SetCells(vtkIdType ncells, vtkIdTypeArray* cells)`
15. `vtkIdTypeArray* GetData()`
