## Add vtkResolveHangingNodes and vtkPResolveHangingNodes

Multi-resolution meshes, such as AMR converted to `vtkPolyData` or `vtkUnstructuredGrid`, have
hanging nodes: the corners of finer cells hang on the edges or faces of coarser neighbors, so point
data interpolated from cell data is discontinuous across refinement levels.

`vtkResolveHangingNodes` (`Filters/Core`) inserts the hanging points of 2D (quads) and 3D
(hexahedra) meshes into the coarser cells, so that neighboring cells share their points. No point is
moved and the cells keep their order, so the cell data stays valid. The input is a `vtkPolyData`, a
`vtkUnstructuredGrid`, a `vtkPartitionedDataSet` or a `vtkPartitionedDataSetCollection`. The
partitions of a `vtkPartitionedDataSet` are resolved together, and the ones of a collection
separately: the points that hang on the boundary of a partition, but belong to another partition,
are inserted too. A `vtkCartesianGrid`, e.g., a `vtkImageData`, and the blocks of a
`vtkNonOverlappingAMR` are supported too: a block only receives the points of the blocks of the
other levels, and the output is a `vtkUnstructuredGrid`, or a `vtkPartitionedDataSetCollection` of
them for an AMR. `vtkOverlappingAMR` is not supported.

* `Decompose` (default `true`): the resolved cells are decomposed into conforming triangles and
  quads (2D) or tetrahedra and pyramids (3D), or, when it is off, turned into single polygons or
  polyhedra. A 3D decomposition may add a point at the center of a cell, with interpolated point
  data, and new point global ids, unique over all the processes and the whole collection, when the
  input has some.
* `ComputeInverseMeasure` (default `false`): add an `InverseArea` or `InverseVolume` cell array,
  to weight the interpolation of cell data to points with the new `EnableWeightedAverage` of
  `vtkCellDataToPointData`.
* `PassThroughCellIds` (default `false`): add a cell array, named by `OriginalCellIdsName`
  (default `vtkOriginalResolvedCellIds`), with the input cell of every output cell.
* `vtkPResolveHangingNodes` (`Filters/ParallelDIY2`, object factory override) exchanges the
  points of the partition boundaries with DIY2, so that both sides of a partition interface get the
  same points.

The changes below come with it.

### vtkIdList memory fixes

`vtkIdList` allocated its ids with `malloc`, while `Release()` says to free them with `delete[]`,
and every buffer allocated after a `Release()` was leaked. The ids are now allocated with `new[]`
and freed with `delete[]`.

### Optional side order in vtkUnstructuredGridSides

`vtkUnstructuredGridSides::SetDeterministicSideOrder()` (on by default) keeps the sides of a key in
the order of their cells, whatever the threads. The consumers that do not depend on that order can
turn it off, to skip the cost of sorting them.

### Faster vtkPolyDataToUnstructuredGrid

`vtkPolyDataToUnstructuredGrid` is faster: it shares the cells of the input when possible, and uses
implicit cell types and offsets. The field data is now passed too.

### Weighted average in vtkCellDataToPointData

`vtkCellDataToPointData`: `EnableWeightedAverage` (new) weights the average by a single component
cell array selected with `SetInputArrayToProcess()`, e.g., the `InverseArea` or `InverseVolume` of
`vtkResolveHangingNodes`, so that point values are not biased by the number of fine cells around a
hanging point. All the dataset types and `ContributingCellOption` values are supported. The
protected `InterpolatePointData(input, output)` is deprecated in favor of the overload with a
weight array.
