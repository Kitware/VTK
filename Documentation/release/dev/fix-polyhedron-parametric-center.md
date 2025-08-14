## vtkPolyhedron: Implement GetCentroid and use it in GetParametricCenter

In VTK 9.4, the `GetParametricCenter` of `vtkPolyhedron` used to return the parametric
location of its bounding box center which was always 0.5, 0.5, 0.5.

In VTK 9.5, the `GetParametricCenter` of `vtkPolyhedron` was changed to return the
parametric location of the centroid of its points. This was a good change, as it
allowed for a more meaningful representation of the polyhedron's center. This was
improvement over the previous implementation, but it still was not ideal especially
for non-convex polyhedra.

In VTK 9.5.1, the `vtkPolyhedron` class introduces the `GetCentroid` function, which computes the signed-volume weighted
centroid of a polyhedron using its pyramid decomposition. In this approach, the polyhedron is decomposed into pyramids,
each formed by one of the polyhedron’s faces and a common apex point located at the centroid of all the polyhedron’s
points. The `GetParametricCenter` method has been updated to return the parametric location
of the centroid returned from `vtkPolyhedron`'s `GetCentroid` function.
