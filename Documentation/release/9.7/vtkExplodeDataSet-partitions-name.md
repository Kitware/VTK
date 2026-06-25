## vtkExplodeDataSet partitions name from FieldData

When the new `SetUsePartionNamesFromFieldData` option is set to `true`, the `vtkExplodeDataSet`
will try to retrieve the partition names from the FieldData array passed in `PartitionNamesArray`,
optionally based on the index of the partition scalar in `PartitionValuesArray`.

The naming process for a part with a given `scalarValue` is the following:
- if `UsePartionNamesFromFieldData` is `false`, generate a name as before:
  use the scalar array name suffixed with the partition index
- if no `PartitionValuesArrays`, try to use `scalarValue` as index in `PartitionNamesArray`
- else retrieve the index of `scalarValue` in `PartitionValuesArray` and check `PartitionNamesArray` at same index.
- on any error (name array not found, or value not found in array), fallback to name generation
  as when `UsePartionNamesFromFieldData` is `false`.
