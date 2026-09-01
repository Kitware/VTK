## vtkStaticEdgeLocatorTemplate no longer overruns its edge array

IsInsertedEdge() now bounds its scans to the bin of the queried edge.
Previously a query for an edge that is absent from the locator could
scan past the end of the edge array (crashing in optimized builds) or
report an unrelated edge as inserted. This fixes a release-mode crash
in vtkDelaunay3D when both tetrahedra and lines are produced for an
alpha shape.
