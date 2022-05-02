## Improve Cell Locators

vtkCellLocator, vtkStaticCellLocator, vtkCellTreeLocator, vtkModifiedBSPTree, Cell locators have several improvements as
listed below:

1. Caching cell bounds has been multithreaded
2. InsideCellBounds has been modified to check if bounds have been cached for better performance
3. vtkCellTreeLocator's FindCell has been improved by inserting an InsideCellBounds check before EvaluatingPosition
4. vtkCellTreeLocator has been moved to Common/DataModel
5. vtkCellTreeLocator now supports 64-bit ids
7. vtModifiedBSPTree's BuildLocatorInternal has been partially multithreaded
8. vtkCellLocator is now fully thread safe
9. vtkStaticCellLocator now does not utilize Tolerance (like all the other cell locators). The tolerance was added in
   the past to ameliorate issues inside IntersectWithLine/FindCellsAlongLine by padding the bin bounds. These issues
   have now been fixed by using a double tolerance approach, hence the Tolerance is no longer needed.
10. Because vtkStaticCellLocator does not utilize Tolerance, UseDiagonalLengthTolerance has been deprecated.
11. vtkCellTreeLocator's and vtkModifiedBSPTree's IntersectWithLine are now thread-safe.
12. All cell locators now have a new IntersectWithLine function which returns the intersected cells and their
    intersection points sorted by the parametric t. This new function is a more general case of FindCellsAlongLine,
    therefore, all cell locators now also support FindCellsAlongLine.
13. Both IntersectWithLine and FindCellsAlongLine now follow the same approach across all locators. Additionally, the
    tolerance parameter is now also used to check intersection with the cell bounds, to avoid false negatives.
14. All Cell/Points Locators now have UseExistingSearchStructure. This parameter allows the locator to NOT
    be built again. This is useful when you have a dataset that either changes because the FieldData (
    PointData/CellData) changed or the actual dataset object changed, but it's actually the same geometry (useful when a
    dataset has timesteps). When this flag is on you need to use ForceBuildLocator() to rebuild the locator, if your
    dataset changes.
15. LazyEvaluation flag has been deprecated for all cell locators, because it could lead to thread-safety issues. Due
    to this change, the function BuildLocatorIfNeeded has been deprecated across cell locators that they were defining
    it.
16. All Cell Locators now cache their bounds by default (it's still an option) because it's highly beneficial
    performance-wise. vtkModifiedBSPTree has been modified to cache the cell bounds optionally.
17. All Cell Locators now have clear documentation about which vtkLocator/vtkAbstractCellLocator parameters they are
    using, and vtkAbstractCellLocator now clearly states which functions are thread-safe or not. The non-thread-safe
    ones, use internal GenericCell or weights.
