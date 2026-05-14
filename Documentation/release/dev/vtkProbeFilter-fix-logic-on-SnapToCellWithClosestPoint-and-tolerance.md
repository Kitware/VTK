## vtkProbeFilter: Fix logic on SnapToCellWithClosestPoint and use of tolerance

The previous implementation of `vtkProbeFilter` assumed in the case of `SnapToCellWithClosestPoint` that the previously
computed cell internal coordinates were valid. However, this is not true since this branch is reached when no cell has
been found by any FindCell. This is now corrected, finding the coordinates directly by using
`vtkCell::EvaluatePosition`.

In addition, distance to the found cell is used coherently to evaluate if the point is within the tolerance, even if
evaluated as outside by `vtkCell::EvaluatePosition`. This is especially relevant for 1D and 2D cells, such as edges and
triangles.

The `SnappingRadius` is now also a user-defined variable, used if `SnapToCellWithClosestPoint` is on. Previously it was
always infinite.

A test for `vtkProbeFilter` with a PolyData source and using `SnapToCellWithClosestPoint` has been added as part of
`TestProbeFilter`.

As a side-effect, `vtkClosestPointStrategy::FindClosestPointWithinRadius` has been fixed, ensuring that the returned
genCell is always the correct one corresponding to the returned closestCellId.

Finally, `vtkAbstractInterpolatedVelocityField`, similarly to `vtkProbeFilter`, it now uses
`vtkDataSet::GetSampledMaxCellLength2(numSamples)` to compute a tolerance for `FindCell`.
