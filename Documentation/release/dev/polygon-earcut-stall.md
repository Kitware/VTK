## Ear cutting no longer leaves holes in folded polygons

`vtkPolygon::EarCutTriangulation()` stops when no vertex left is a valid ear,
which happens on polygons that fold over themselves, e.g., the iso-polygons
traced through the polyhedra of hanging-node meshes. It returned the triangles
cut so far, so callers that used them left a hole. It now closes the rest of the
polygon with a fan, and still returns 0 to report the failure. Contours of
polyhedra with triangulation on no longer crack there.
