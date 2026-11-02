## Probing every cell-grid attribute

`vtkCellGridPointProbe` now interpolates any number of the cell-attributes of
its source, chosen with `GetAttributeSelection()`. The points are
classified once, and each enabled attribute is interpolated from that
classification. The shape attribute is never interpolated.

`SetAttributeName()` and `GetAttributeName()` are deprecated in favor of the
selection. Setting a name enables only that attribute in the selection.

Supporting changes:

- `vtkCellGridPointProbe` outputs the `InputPointIndex` of each point and fills
  its `CellType` array correctly.
- `vtkCellGridEvaluator::InterpolateCellParameters()` returned no values; it now
  interpolates them.
- `vtkCellGridEvaluator::SetInterpolatedValues()` is now public. The evaluator
  reuses the same array for every evaluation, so setting a new array before
  each one keeps the values of several evaluations.
