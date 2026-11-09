# Fix vtkTriangleFilter on meshes that are already triangulated

`vtkTriangleFilter` now honours `PassLines` when the polygons of its input are all triangles, and keeps the cell
data with the cells it belongs to when the input holds vertices, lines and polygons together. Previously, such an
input kept its lines with `PassLinesOff()`, a polyline was not split into lines, and the cell data of the output was
shifted and partly uninitialized.

`vtkImplicitPolyDataDistance`, and with it `vtkDistancePolyDataFilter`, relies on this filter to evaluate triangles
only. On a surface with triangles that have two coincident vertices, which merging the points turns into lines, they
no longer read past the end of a line and crash.
