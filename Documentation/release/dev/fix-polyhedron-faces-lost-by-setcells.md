## Polyhedron faces lost when cells are handed to SetCells()

`vtkUnstructuredGrid::SetCells(cellTypes, cells)` interprets its cell array as a
legacy face stream (`[nCell0Faces, nFace0Pts, i, j, k, ...]`) as soon as any
cell is a `VTK_POLYHEDRON`, and decomposes it. `GetCells()`, however, returns
point lists. A caller that copies cells out of one grid and hands them to
another therefore has the point list read back as face descriptions: the first
point id is taken as a face count. The result is invalid topology, and on larger
meshes very long runtimes inside `DecomposeAPolyhedronCell`.

Three callers did exactly this, and each now copies the polyhedron faces
alongside the connectivity and builds its output through `SetPolyhedralCells()`:

* `vtkExtractCellsAlongPolyLine`, which backs Plot Over Line. Faces are remapped
  through the same input to output point map already used for the connectivity.
* `vtkOverlappingCellsDetector`, which gathers candidate cells with
  `vtkCell::GetPointIds()` before exchanging them between ranks. The faces now
  travel with the candidates and are remapped with them.
* The `vtkUnstructuredGrid` (de)serialization helper, which wrote only `Cells`
  and `CellTypes`. It now writes `PolyhedronFaces` and `PolyhedronFaceLocations`
  as well. States written without them still load.

Grids containing no polyhedra take the previous code path in every case.

Each fix comes with a test that fails without it. The existing tests could not
have caught any of these: they assert on point counts, cell counts and bounds,
all of which survive the loss of a polyhedron's faces.
