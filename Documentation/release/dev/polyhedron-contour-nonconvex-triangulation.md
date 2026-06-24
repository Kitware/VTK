## Correct triangulation of non-convex polyhedron contour polygons

vtkPolyhedron contouring now ear-clips the polygons it produces along an
iso-surface, rather than fanning them from a single vertex. Fanning produced
self-overlapping triangles for non-convex contour polygons, which can occur
when an iso-surface cuts a general polyhedron. Those polygons are now
triangulated correctly, using the same measure-ordered ear clip that the rest
of VTK's triangulation uses.
