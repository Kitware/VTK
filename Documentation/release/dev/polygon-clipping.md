# Polygons clipping results are now polygons.

`vtkPolygon::Clip()` now produces a polygon output in all cases. Previously, it triangulated every input polygon and clipped the resulting triangles, which led the an output mesh topology far different from the input, even in cells that were not clipped in any way. Now, the result of clipping a polygon is itself a polygon.
