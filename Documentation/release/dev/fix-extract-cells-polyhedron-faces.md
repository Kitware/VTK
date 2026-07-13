## Preserve polyhedron faces in vtkExtractCellsAlongPolyLine

`vtkExtractCellsAlongPolyLine` now preserves the face topology of
`VTK_POLYHEDRON` cells.

A polyhedron's faces are not described by its point list. They live in the
grid's separate `PolyhedronFaces` and `PolyhedronFaceLocations` arrays. The
filter copied only the point list and passed it to the two argument
`vtkUnstructuredGrid::SetCells()`, which, when any polyhedron is present,
interprets the incoming cell array as a legacy face stream and decomposes it.
The extracted point list was therefore read as face descriptions, yielding
invalid topology and, on larger meshes, very long runtimes inside
`DecomposeAPolyhedronCell`.

The filter now copies each extracted polyhedron's faces, remaps their point ids
through the same input to output point map already used for the connectivity,
and builds the output through `SetPolyhedralCells()`. Grids without polyhedra
take the previous code path unchanged.

This affects any pipeline that runs Plot Over Line on a polyhedral mesh.
