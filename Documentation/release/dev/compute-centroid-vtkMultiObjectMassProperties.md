## Compute centroid in vtkMultiObjectMassProperties

`vtkMultiObjectMassProperties` has been modified to also compute the centroid of each object.
The centroid is calculated as a weighted average of the centroids of the tetrahedrons
which are used to compute the volume of the polygonal object, and the weight is the
tetrahedron's volume contribution.

Additionally, the name of the `Objectids` array can now be modified, but its default is `Objectids`.

This contribution addresses the VTK side of the issue https://gitlab.kitware.com/paraview/paraview/-/issues/19982.
