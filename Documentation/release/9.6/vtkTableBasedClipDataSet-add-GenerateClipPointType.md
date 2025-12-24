## vtkTableBasedClipDataSet: Add GenerateClipPointType

`vtkTableBasedClipDataSet` now includes a new option called `GenerateClipPointType` that allows users to specify whether
to generate a point array specifying the type of each point in the clipped dataset. A clip point type can be either
0 (input point), 1 (edge point), or 2 (centroid point).
