## Add `vtkConvexHull` filter

`vtkConvexHull` is a new `vtkPointSetAlgorithm` that computes the convex hull of a point
set in 1, 2, or 3 dimensions, outputting the result as `vtkPolyData`. The algorithm used
is O(n) for 1-D (min/max scan), O(n log n) for 2-D (Andrew's monotone chain), and
O(n log n) average for 3-D (Quickhull).

The hull is represented internally as a set of half-planes `{Normal, D}` where a point
`P` is inside iff `P · Normal ≤ D` for every plane. `IsPointWithinConvexHull()` tests
membership against the last computed hull.

Static overloads of `ComputeConvexHull()` accept either a `vtkDataArray*` or a raw
`vtkVector3d*` array and write directly into a caller-supplied
`std::vector<vtkConvexHull::Plane>`, preserving its capacity across calls for efficient
per-cell use. The companion `IsPointInside()` tests a point against any such plane set.

Setting `GeneratePolyData` to `false` skips geometry output and computes only the
half-plane representation.
