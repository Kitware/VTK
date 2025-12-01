## Polyhedrons support in VTKHDF

Polyhedrons cells are specified as part of the VTKHDF specification,
and `vtkHDFReader` now supports Polyhedron cells as part of the Unstructured Grid type, through new VTKHDF fields.

The `vtkHDFUtilities::TemporalGeometryOffsets` struct is not templated anymore,
and offsets are not retrieved directly by the constructor anymore.
First create the struct, then call `GetOffsets<T>` on it to retrieve offsets.
