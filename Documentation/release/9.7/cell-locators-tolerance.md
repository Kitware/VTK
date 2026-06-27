## CellLocators: Make sure that when a tolerance parameter is available it can be used

`vtkCellLocator`, `vtkStaticCellLocator`, `vtkCellTreeLocator`, `vtkModifiedBSPTree`, `vtkOBBTree`,
`vtkLinearTransformCellLocator` have the following
functions that have a tolerance parameter, but it was not used in the implementation. Now it is used.

1. `InsideCellBounds`
2. `IntersectWithLine`
3. `FindCell`
4. `FindCellsAlongLine`
5. `FindCellsAlongPlane`

The following `vtkAbstractCellLocator` functions are no longer virtual, since they are calling other functions
internally, which the subclasses may implement.

1. `vtkIdType FindCell(double x[3]);`
2. `vtkIdType FindCell(double x[3], double tol2, vtkGenericCell* GenCell, double pcoords[3], double* weights);`
3. `void FindCellsAlongLine(const double p1[3], const double p2[3], double tol, vtkIdList* cells);`

Some other classes and tests that previously pretended to use the tolerance (without actually using it) has been updated
accordingly:

- `vtkBinCellDataFilter`
- `TestResampleWithDataSet3`
- Test `CellTreeLocator`

And new tests have been added in TestCellLocatorsEdgeCases.

because of these changes, since `vtkAbstractCellLocator` don't use `vtkLocator::Tolerance`, `vtkLocator::Tolerance` has
been deprecated, and only `vtkAbstractPointLocator` has it now.
