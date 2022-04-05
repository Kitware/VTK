# Deprecation in openfoam reader

Polyhedral decomposition is now deprecated and slated for the earliest
possible removal. It is holdover from before `vtkPolyhedron` was added
and `vtkConvexPointSet` was a poor substitute.

Now that `vtkPolyhedron` mostly _works_ everywhere (clipping planes
may are still produce non-manifold cuts, but crinkle cuts are fine) it
is time to remove this old support.

Also note that the decomposition overhead can be absolutely enormous
on larger meshes. In some cases that is can make the difference between
loading and viewing the results and exhausting the available memory.
