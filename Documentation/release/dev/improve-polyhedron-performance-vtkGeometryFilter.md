## vtkGeometryFilter: Improve performance for polyhedron

`vtkGeometryFilter` used to extract the whole cell when a cell was a polyhedron.
Now, it accesses the `GetPolyhedronFaces` and `GetPolyhedronFaceLocations` methods
to efficiently extract the polyhedron faces related information.
