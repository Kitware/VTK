## Isocontours of cell-grid attributes

The new `vtkCellGridIsocontour` filter extracts the isosurface of a scalar
cell-attribute of curved cells, in any basis, as a cell-grid of linear triangles
(lines from surfaces, vertices from curves). Only the cells whose
Bernstein-Bezier control values bracket the isovalue are contoured, as linear
cells each subdivided `SubdivisionLevel` times (by default once less than the
basis's order). Each point carries the cell it lies in (`CellType`, `CellId`,
`RST`) and the attribute, its gradient and, with `InterpolateAttributes`, the
other attributes evaluated there. As in `vtkContourFilter`, triangles are
oriented toward the lower values.

Each cell is contoured with its own values: the surface of a continuous
attribute is closed, while that of a discontinuous one, such as a Discontinuous
Galerkin field, has gaps where the field jumps between cells.

The new `vtkCellGridIsocontourPoints` filter locates points on the isocontour of
a scalar `Bezier` cell-attribute: one for each segment between the corners and
critical points of a cell whose ends lie on opposite sides of the isovalue. Its
`vtkPolyData` output has one poly-vertex per cell, with each point's cell,
parametric coordinates, value, gradient (`ComputeGradients`) and the other
attributes (`InterpolateAttributes`). Points found by several cells are not
merged; use `vtkStaticCleanPolyData` without averaging. `Include1DCells`,
`Include2DCells` and `Include3DCells` restrict the cells searched. With
`OutputIsocontourPoints` on, `vtkCellGridIsocontour` outputs these points as its
second output; `ParticleRelaxationIterations` applies to them.

Supporting changes:

- `vtkCellGridToUnstructuredGrid` can transcribe only some cells, with
  `SetCellsToTranscribe()`.
- `vtkCellGridToUnstructuredGrid::MergePointsOff()` keeps the linear cells of
  different input cells apart, with the attributes of their own cell.
  `GenerateInputCellArrays` adds the `CellType`, `CellId` and `RST` point-data
  arrays saying where each point was sampled.
- `vtkCellGridEvaluator` computes the world-space gradient of a scalar
  cell-attribute (`ComputeGradients`, `GetInterpolatedGradients()`).
- `vtkCellGridEvaluator::InterpolateCellParameters()` returned no values; it now
  interpolates them.
- `vtkCellGridPointProbe` outputs the `InputPointIndex` of each point and fills
  its `CellType` array correctly.
- `vtkMath::LinearSolveTransposed3x3()` solves A^T y = x for a 3x3 matrix and,
  unlike `vtkMath::LinearSolve3x3()`, reports a singular matrix.
