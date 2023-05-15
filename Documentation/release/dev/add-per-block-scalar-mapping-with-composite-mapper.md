## Map different scalar arrays to colors with vtkCompositePolyDataMapper

`vtkCompositePolyDataMapper` can now color separate blocks with different scalar arrays. In order to use this functionality, turn on `ScalarVisibility` and select a `ScalarMode` and/or a `ColorMode`.

You can choose which scalar array or component of an array is used to map colors by overriding these parameters per block. Refer to `vtkMapper` documentation. Here's a summary:
1. **ArrayAccessMode**: `VTK_GET_ARRAY_BY_ID` or `VTK_GET_ARRAY_BY_NAME`
2. **ArrayComponent**: which component should be used to map an array?
3. **ArrayId**: Applicable when arrays are accessed by indices. this integer is an index of the vtkDataArray on the point/cell data.
4. **ArrayName**: Applicable when arrays are accessed by names.
5. **FieldDataTupleId**:  this integer is an index of the vtkDataArray on the field data.
