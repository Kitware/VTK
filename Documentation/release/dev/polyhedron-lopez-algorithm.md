## Polyhedron clipping and contouring now uses López polygon-tracing algorithm

`vtkPolyhedron::Contour()` and `vtkPolyhedron::Clip()` have been replaced with an
implementation based on the López polygon-tracing algorithm (López et al., "A new
isosurface extraction method on arbitrary grids", JCP 2021). The new algorithm works
directly on the original polyhedron faces without triangulation, eliminating the
complex triangle-merging post-processing step of the previous implementation.

`vtkPolyhedron::IsInside()` has been replaced with a solid-angle (winding number)
summation, replacing the previous random ray-casting approach. The new method is
deterministic and single-pass.

### Breaking change: outward-facing normals required

These new algorithms require that polyhedron face vertices are wound
counter-clockwise when viewed from outside the cell, so that face normals (by the
right-hand rule) point outward. The previous ray-casting and triangulation-based
approaches were tolerant of inconsistent or inward-facing normals; the new
algorithms are not.

Code that constructs `VTK_POLYHEDRON` cells must ensure correct face winding.
`vtkCellTypeSource` has been fixed accordingly.

### New API

- `vtkLine::DistanceToSegment()` — squared distance from a point to a finite
  line segment, with clean degenerate handling
- `vtkTriangle::DistanceToTriangle()` — squared distance from a point to a
  triangle (perpendicular when inside, edge distance otherwise)
- `vtkTriangle::SolidAngle()` — signed solid angle subtended by a triangle at a
  point, using the Van Oosterom-Strackee formula
- `vtkPolyhedronContour::CountClip()` / `EmitClip()` — bulk count/emit API for
  threaded clip filters, used by `vtkTableBasedClipDataSet` and integrated
  with the shared edge locator
- `vtkPolyhedronContour::ContourCell()` — cell-level contour helper used by
  `vtkContour3DLinearGrid`. Runs the López trace for one polyhedron and
  reports intersected edges and per-polygon vertex counts; iso-vertex
  deduplication is performed by the calling filter via its shared edge
  locator

### What changed

- `vtkPolyhedronContour` — new class implementing the López algorithm for both
  contour and clip operations
- `vtkPolyhedron::IsInside()` — replaced random ray-casting with solid-angle
  summation (winding number)
- `vtkPolyhedron::GetCentroid()` — replaced pyramid decomposition with divergence
  theorem computation
- `vtkPolyhedron::ComputeVolume()` — new method using divergence theorem
- `vtkCellTypeSource::GeneratePolyhedron()` — fixed face winding to produce
  outward-pointing normals
- `vtkContour3DLinearGrid` — polyhedra handled via
  `vtkPolyhedronContour::ContourCell` alongside the existing linear cell
  marching tables. Cell normals for non-triangle polygons use
  `vtkPolygon::ComputeNormal` over all vertices rather than a first-3
  approximation. `GenerateTriangles=OFF` outputs native iso-polygons
  (quads, pentagons, etc.) using the polygon case tables for linear cells
  and the López trace for polyhedra. `MergePoints` has been deprecated;
  points are always merged.
- `vtk3dLinearGridPlaneCutter` now uses `vtkContour3DLinearGrid` and therefore
  supports polyhedron cutting, and exposes `GenerateTriangles` and `GenerateCutScalars`.
  `MergePoints` has been deprecated.
- `vtkContourFilter` now dispatches to the fast `vtkContour3DLinearGrid` path for
  linear unstructured grids regardless of `GenerateTriangles`; previously the fast
  path was gated on `GenerateTriangles=ON`, so polygon-mode contouring fell through
  to the generic per-cell `vtkContourGrid` path and was substantially slower.
- `vtkTableBasedClipDataSet` — polyhedra flow through the same batch /
  prefix-sum / extract pipeline as other cell types, using the bulk
  `CountClip` / `EmitClip` cell-array API together with the shared edge
  locator for iso-vertex deduplication. Fast-kept and fast-discarded cells
  use sentinel case values; intersected cells are handled by Count/Emit.
- Tests that load polyhedron data with inconsistent winding now normalize face
  orientation at runtime before contour/clip operations
- `vtkCPExodusIIElementBlock` used by `vtkCPExodusIIInSituReader` has been deprecated in favor of `vtkUnstructuredGrid`
- `vtkCPExodusIIElementBlockCellIterator` has been deprecated, because it's no longer needed.
