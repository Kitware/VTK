# Speed up rendering of large number of points in Context2D module.

The Rendering/Context2D module can now render large number of points(~1 million) much faster than before. You can use
the cached overloads of `vtkContext2D::DrawPoints`, `vtkContext2D::DrawPointSprites`, `vtkContext2D::DrawMarkers`
to speed 2D plot rendering. Similarly, you may be interested in the cached overloads of `vtkContext3D::DrawPoints`
and `vtkContext3D::DrawTriangleMesh` for 3D point, line and surface plots.

## Rendering/Context2D: Cached draw functions
The cached overloads require `vtkDataArray` inputs instead of raw-pointers and an integer called the `cacheIdentifier`. Generally, you can rely upon the in-memory address
of a `vtkAbstractContextItem` for the cache identifier.

## Limitations
- The other Draw functions (DrawLine, DrawPolygon, etc) cannot use caching because new primitives are generated in temporary C++ vectors just before the draw call.
