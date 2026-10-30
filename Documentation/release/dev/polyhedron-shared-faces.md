## Polyhedra can share faces

A face shared by two polyhedra can be stored once in
`vtkPolyhedronCellArray`. It is wound outward for one cell, and the other
lists it as `-(n+1)` in its face locations to traverse face `n` in reverse.
The queries of `vtkPolyhedronCellArray` read through the sign, so
`vtkPolyhedron` and the filters built on it see every face wound outward.
Code that reads the arrays through `vtkPolyhedronCellArray::Dispatch()` passes
the signed ids to `DispatchUtilities::GetFaceSize()` and
`GetOrientedFaceRange()`. `vtkAppendFilter`, `vtkExtractCells`,
`vtkCellTypeSides` and the ghost cell generation of `vtkDIYGhostUtilities`
handle shared faces too.

Without the sign, a face shared between two cells is inward for one of them,
which gives wrong contours, cuts and volumes for that cell.

`vtkUnstructuredGrid::GetOrientedPolyhedronFaces()` and
`GetOrientedPolyhedronFaceLocations()` return the stored arrays.
`GetPolyhedronFaces()` and `GetPolyhedronFaceLocations()` are deprecated; they
return a copy in which every cell owns its faces with positive ids, so
existing code keeps working. `vtkPolyhedronCellArray::DuplicateSharedFaces()`
makes such a copy for other code that needs one.

VTKHDF 2.9 and VTK XML 2.5 store `-(n+1)` in `PolyhedronToFaces` and
`polyhedron_to_faces`. Readers of earlier versions open these files without a
warning and misread the polyhedra that share faces; call `DuplicateSharedFaces()`
before writing to produce a file they read correctly.

`vtkAppendFilter` no longer overflows on polyhedra that share faces, and
`vtkPolyhedronCellArray::ExportLegacyFormat()` sizes its output for them.
