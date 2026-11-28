# WebGPU: replaced mesh arrays

The WebGPU backend now draws the new values when you replace the points of a mesh, or one of
its point or cell attribute arrays such as normals, with a different array, for example
through `vtkPolyData::SetPoints()`. It used to upload an array again only when the array had
been modified since the previous upload, so a same-size array created before then was ignored
and the old geometry stayed on screen.

Two cases are not covered yet:

- Cells are still compared by modification time only. A `vtkCellArray` that replaces another,
  for example through `vtkPolyData::SetPolys()`, is uploaded only if it was modified after the
  cells on screen were uploaded. If you replace both the points and the cells of a mesh, the
  new points are drawn with the old cells until you call `Modified()` on the new cell array.
- With `vtkCompositePolyDataMapper`, the WebGPU backend does not notice changes to the arrays
  of a block, replaced or modified in place, until you call `Modified()` on the mapper.
