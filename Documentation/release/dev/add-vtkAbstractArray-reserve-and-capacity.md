## vtkAbstractArray: Deprecate GetSize(), and Resize(); Add GetCapacity() and ReserveTuples()

The new `GetCapacity()` method in `vtkAbstractArray` clarifies the semantics of VTK array storage. The previous
`GetSize()` method was frequently misunderstood, as many users assumed it behaved like `std::vector<T>::size()`, when in
fact it reported allocated storage (i.e., capacity). The new naming removes this ambiguity and makes it explicit that
the method reflects allocation rather than logical size.

It should be noted that the `vtkGenericDataArray::Capacity()` which was deprecated in 9.6, has been removed in 9.7, so
that the `vtkAbstractArray:Capacity` variable can exist.

The `vtkGenericDataArray` 's `SetNumberOfTuples()` now behaves like `vtkBitArray`/`vtkStringArray`/`vtkVariantArray` 's
`SetNumberOfTuples` which uses `SetNumberOfValues` under the hood, so that data preservation can occur.

Similarly, `ReserveTuples(numTuples)` replaces the old `Resize(numTuples)`, whose name suggested behavior analogous to
`std::vector<T>::resize()`. In practice, however, `Resize(numTuples)` managed allocated storage rather than logical size
and would shrink the allocation when `n` was smaller than the current capacity.

`ReserveTuples(numTuples)` makes the intent explicit: it adjusts the array’s allocated capacity to `numTuples * numComp`
without modifying the logical number of tuples or values. Increasing the allocation is conceptually similar to
`std::vector<T>::reserve()`. To reduce the logical size and capacity to match the current logical size—behavior
analogous to `std::vector<T>::resize(n)` and `std::vector<T>::shrink_to_fit()`—call `SetNumberOfTuples(numTuples)` and
`Squeeze()`.

Finally, the legacy `GetSize()`, `Resize(numTuples)` methods are now deprecated and will be removed in a future release.

The same changes have also been applied to `vtkPoints`, `vtkPoints2D` and `vtkIdList`, i.e:

1. `vtkPoints`/`vtkPoints2D`: `SetNumberOfPoints(numPts)` now preserves data and `Reserve(numPts)` replaces the
   deprecated `Resize(numPts)`.
2. `vtkIdList`: `SetNumberOfIds(numIds)` now preserves data and `Reserve(numIds)` replaces the deprecated
   `Resize(numIds)`.
