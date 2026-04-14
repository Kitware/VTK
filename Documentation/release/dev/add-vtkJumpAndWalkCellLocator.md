## vtkJumpAndWalkCellLocator: A new cell locator based on the Jump and Walk algorithm

VTK used to have the concept of `vtkFindCellStrategy`, which could be used to find a cell containing a point.
`vtkFindCellStrategy` has 3 subclasses, `vtkCellLocatorStrategy`, `vtkClosestPointStrategy`, and
`vtkClosestNPointStrategy`. The first one was using a cell locator adaptor to find the cell, while the other two were
using a point locator and implementing the jump-and-walk algorithm to find the cell.

Having both `vtkAbstractCellLocator` and `vtkFindCellStrategy` for finding a cell was confusing, and for that reason
`vtkJumpAndWalkCellLocator` has been implemented as a new `vtkAbstractCellLocator` subclass that implements the
jump-and-walk algorithm, similarly to `vtkClosest(N)PointStrategy`.

Thanks to this addition, `vtkCellLocatorStrategy`, `vtkClosestPointStrategy`, and
`vtkClosestNPointStrategy` have been deprecated, and the desired cell locator can just be passed. It should be noted
that the following functions have been deprecated as well:

1. `void vtkProbeFilter::SetFindCellStrategy()`: Use `SetCellLocator()` instead.
2. `void vtkProbeFilter::SetCellLocatorProtorype()`: Use `SetCellLocator()` instead.
3. `vtkResampleWithDataSet::SetCellLocatorPrototype`: Use `SetCellLocator()` instead.
4. `void vtkCompositeDataProbeFilter::SetFindCellStrategyMap()`: Use `SetCellLocatorMap()` instead.
5. `void vtkAbstractInterpolatedVelocityField::SetFindCellStrategy()`: Use `SetCellLocator()` instead.
6. `void vtkTemporalInterpolatedVelocityField::SetFindCellStrategy()`: Use `SetCellLocator()` instead.

Additionally, given that now there is only one FindCell abstract via `vtkAbstractCellLocator`, the following functions
have also been deprecated:

1. `vtkStreamTracer::SetInterpolatorType()`: Use `SetCellLocator()` instead.
2. `vtkStreamTracer::SetInterpolatorTypeToDataSetPointLocator()`: Use `SetCellLocator()` instead.
3. `vtkStreamTracer::SetInterpolatorTypeToCellLocator()`: Use `SetCellLocator()` instead.
4. `vtkStreamTracer::SetInterpolatorPrototype()`: Use `SetCellLocator()` instead.
5. `vtkEvenlySpacedStreamlines2D::SetInterpolatorType()`: Use `SetCellLocator()` instead.
6. `vtkEvenlySpacedStreamlines2D::SetInterpolatorTypeToDataSetPointLocator()`: Use `SetCellLocator()` instead.
7. `vtkEvenlySpacedStreamlines2D::SetInterpolatorTypeToCellLocator()`: Use `SetCellLocator()` instead.
8. `vtkEvenlySpacedStreamlines2D::SetInterpolatorPrototype()`: Use `SetCellLocator()` instead.
9. `vtkVectorFieldTopology::SetInterpolatorType()`: Use `SetCellLocator()` instead.
10. `vtkVectorFieldTopology::SetInterpolatorTypeToDataSetPointLocator()`: Use `SetCellLocator()` instead.
11. `vtkVectorFieldTopology::SetInterpolatorTypeToCellLocator()`: Use `SetCellLocator()` instead.
12. `vtkParticleTracerBase::SetInterpolatorType()`: Use `SetCellLocator()` instead.
13. `vtkParticleTracerBase::SetInterpolatorTypeToDataSetPointLocator()`: Use `SetCellLocator()` instead.
14. `vtkParticleTracerBase::SetInterpolatorTypeToCellLocator()`: Use `SetCellLocator()` instead.

Moreover, `vtkStaticPointLocator(2D)` `StaticOn()/Off()` have been deprecated in favor of
`UseExistingSearchStructureOn()/Off()`.

Finally, `vtkPointSet::BuildLocator()` has been deprecated in favor of `BuildPointLocator()`.
