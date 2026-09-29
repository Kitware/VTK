## vtkStaticCleanUnstructuredGrid no longer modifies the polyhedron faces of its input

`vtkStaticCleanUnstructuredGrid` used to remap the polyhedron face connectivity of its input in
place and share that buffer with its output. Upstream consumers then read corrupted polyhedra,
and re-executing the filter produced invalid (`-1`) point ids. The face connectivity is now copied
before being remapped.
