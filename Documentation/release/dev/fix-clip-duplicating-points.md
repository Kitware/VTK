## vtkTableBasedClipDataSet: Fix Clipping Duplicating Points

`vtkTableBasedClipDataSet` no longer duplicates points when processing cells that are not supported by it and need
to be delegated to `vtkClipDataSet`.
