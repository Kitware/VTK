# vtkPolygon::Clip avoids unnecessary triangulation

`vtkPolygon::Clip()` now checks whether a polygon actually straddles the
clip value before doing any work. If every point is on the kept side, the
polygon is passed through unchanged as a single cell; if every point is on
the discarded side, nothing is output. Triangulating the polygon and
clipping each triangle is only done when the polygon is actually cut by the
clip value, as before.
