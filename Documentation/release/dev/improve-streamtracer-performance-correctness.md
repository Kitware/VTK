## vtkStreamTracer: Improve performance and correctness

`vtkStreamTracer` has be enabled to run using SMP when 1 MPI is core is used. Moreover, `FindCell` operation using both
`vtkCellLocatorStrategy` and `vtkClosestPointStrategy` has been improved performance-wise. Also, `vtkFindCellStrategy`
has a `FindClosestPointWithinRadius` function which both `vtkCellLocatorStrategy` and `vtkClosestPointStrategy`
implement. `vtkCellLocatorStrategy` does it using a CellLocator, and `vtkClosestPointStrategy` does it using the
approach previously existing in `vtkAbstractInterpolatedVelocityField`'s `FindCellAndUpdate`. Similarly, since now both
`vtkInterpolatedVelocityField` and `vtkCellLocatorInterpolatedVelocityField` should be able to perform `SnapPointOnCell`
, and they were almost identical (except the used strategy), their implementation has been moved to their parent class
`vtkCompositeInterpolatedVelocityField`. Furthermore, `vtkAbstractInterpolatedVelocityField`'s `FindCellAndUpdate` has
been carefully optimized to perform the minimum number of `EvaluatePosition`/`EvaluateLocation`/`GetCell`. Finally, the
quality when the SurfaceDataSet is enabled has been improved significantly.
