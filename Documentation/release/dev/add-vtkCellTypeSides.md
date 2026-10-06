## Add vtkCellTypeSides: the sides of the cells, without building them when possible

`vtkCellTypeSides` (new) gives the vertices, edges and faces of the cells of a dataset, e.g., a
`vtkUnstructuredGrid` or a `vtkPolyData`, by cell type, without building the cells (when possible):
their number, cell type, size and points. `DispatchCellSides()` calls a functor with the sides of a
cell type, whose sizes are known at compile time. The changes below make it possible.

### The metadata of the cell types at compile time

`vtkCellTypeTraits` (new, included by `vtkCell.h`) gives the metadata of every cell type at compile
time, e.g., `vtkCellTypeTraits<VTK_HEXAHEDRON>::NumberOfEdges`, also exposed by every cell class,
e.g., `vtkHexahedron::NumberOfEdges`. `vtkCellTypeUtilities` is built on it:
`GetDimension(VTK_HIGHER_ORDER_CURVE)` now returns 1 instead of 3.

### The faces of the polyhedra

`vtkPolyhedronCellArray` (new) stores the faces of polyhedral cells, with an API inspired by
`vtkCellArray`, and `vtkUnstructuredGrid` now stores its polyhedra in one, given by
`GetPolyhedronCellArray()`. `DecomposeAPolyhedronCell()` is deprecated in favor of it.
`vtkUnstructuredGrid`'s protected members Faces and FaceLocations are now also deprecated in favor
of Polyhedra. `vtkCellArray::Dispatch()` now returns the same type deduced from its functor.

### Inlining control

`VTK_NEVER_INLINE` and `VTK_ALWAYS_INLINE_LAMBDA` (new, `vtkSetGet.h`) keep a function from being
inlined, e.g., a rare path out of a hot loop, and force a lambda to be inlined, e.g., the functor of
`DispatchCellSides()`.

### The sides of the cells of a vtkDataSet

`vtkDataSet::GetCellEdgePoints()`, `GetCellFacePoints()`, `GetCellNumberOfEdges()` and
`GetCellNumberOfFaces(cellId, cellType)` (new) read the sides of a cell with `vtkCellTypeSides`,
without building it when possible. The quadratic and biquadratic triangles and quads have a
`GetEdgeArray()` (new), and the edges 2 and 3 of `vtkQuad::GetEdgeArray()` are now `{2, 3}` and
`{3, 0}`, oriented as `vtkQuad::GetEdge()`.

### Bug fixes

`vtkMergeCells` and `vtkXMLPUnstructuredGridReader` are fixed to properly support polyhedron when
they are mixed with other cells.
