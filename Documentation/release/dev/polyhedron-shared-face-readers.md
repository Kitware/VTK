## Readers store a face shared by two polyhedra once

The CGNS, OpenFOAM, Fluent, Fluent CFF and CONVERGE CFD readers store a face
shared by two polyhedra once, as their file formats do. The cell that traverses
it in reverse lists it as `-(n+1)` (see `vtkPolyhedronCellArray`). On the test
meshes of these readers, this stores 22% to 48% fewer faces. To give every
polyhedron its own copy of its faces, as before, for example to write files
that older readers can open, call `DuplicateSharedFaces()` on the
`vtkPolyhedronCellArray` of the output.

The Fluent and Fluent CFF readers now orient the faces of polyhedra. A face was
wound as stored for both of its cells, so it pointed into one of them, which
gave wrong contours, cuts and volumes for that cell.

The CGNS reader orients the faces of polyhedra outward also when a file's
NFACE_n or ParentElements does not, as in `EngineSector.cgns`.
`FlipFaceNormals` orients them inward.
