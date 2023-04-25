## Add a `PointDataWeighingStrategy` option to `vtkCleanUnstructuredGrid`

When using `vtkCleanUnstructuredGrid`, you can now choose which strategy to use to collapse your point data.

Previously, when merging duplicate points, the point with the lowest index had its data transported to the merged output point. With this new option, you can now choose between:
* `vtkCleanUnstructuredGrid::FIRST_POINT` (for backwards compatibility): where the point with the lowest index in the input gets the ownership of the merged point
* `vtkCleanUnstructuredGrid::AVERAGING`: where the data on the merged output point is the number average of the input points
* `vtkCleanUnstructuredGrid::SPATIAL_DENSITY`: where the merged point data is averaged using a partition of the volumes in the cells attached to each point being merged
