## Critical points of cell-grid attributes

The new `vtkCellGridFindCriticalPoints` filter locates every critical point
(minimum, maximum, or saddle) of a scalar `Bezier` cell-attribute, on every cell
shape, in each cell's interior and on its faces and edges. Each is located
directly by a Bernstein subdivision solver, so saddles are found as reliably as
extrema. Convert an attribute in another basis with `vtkCellGridChangeBasis`
first.

The output is a `vtkPolyData` with one point per critical point, reporting the
cell it came from, its parametric coordinates, the field's value there, and
whether it is a minimum, maximum, or saddle.
