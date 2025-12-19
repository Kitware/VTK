## Improve the clipping performances for polyhedrons

Clipping operations on unstructured grids composed mostly of polyhedron cells are now significantly faster.
On a 1M6 cells unstructured grid composed exclusively of polyhedrons, clipping performance has improved by up to 300x, with a typical speedup of around 15x, depending on the clipping shape and position.

Also, there is a slight change in vtkPointLocator behavior. If you initialize a locator with a non empty vtkPoints, it won't start counting at 0 anymore but at the number of points already added.
