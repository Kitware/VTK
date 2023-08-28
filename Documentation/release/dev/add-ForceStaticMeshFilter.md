## Introduce vtkForceStaticMesh filter

The notion of static mesh refers to temporal dataset having varying attributes
on a static geometry. In VTK data model terms, this is supported by both the
**vtkPolyData** and **vtkUnstructuredGrid**, through their `GetMeshMTime()`.

This filter takes an input polydata or unstructured grid and caches its geometry.
Subsequent request will only lead to attributes update, thus leading to a valid
static mesh output (as long as the number of points / cells stay the same).

It is up to following filters to use the `GetMeshMTime()` method for performance
improvements.
