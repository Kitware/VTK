## vtkMeshQuality: Add non-linear cells support

The `vtkMeshQuality` filter has been updated to expose `Verdict` metrics for non-linear cells.
The metrics are the following

1. `vtkQuadraticTriangle`: AREA, DISTORTION, NORMALIZED_INRADIUS, SCALED_JACOBIAN
2. `vtkBiQuadraticTriangle`: AREA, DISTORTION
3. `vtkQuadraticQuad`: AREA, DISTORTION
4. `vtkBiQuadraticQuad`: AREA, DISTORTION
5. `vtkQuadraticTetra`:  DISTORTION, EQUIVOLUME_SKEW, INRADIUS, JACOBIAN, MEAN_RATIO, NORMALIZED_INRADIUS,
   SCALED_JACOBIAN, VOLUME
6. `vtkQuadraticHexahedron`: DISTORTION, VOLUME
7. `vtkTriQuadraticHexahedron`: DISTORTION, JACOBIAN, VOLUME

Additionally, the INRADIUS metric was added also for `vtkTetra`.

Moreover, a simplified documentation for each metric has been added to the `vtkMeshQuality` class documentation.

Also, it has been documented that the distortion for `vtkTriangle` and `vtkTetra` always returns 1.0 because they are
simplices.

Additionally, because the point order of `vtkWedge` is different between VTK and Verdict, the `vtkMeshQuality` filter
was updated to reorder the points of the `vtkWedge` before computing any metric.

Moreover, the `vtkCellSizeFilter` has been updated to use all the area/volume functions defined in `vtkMeshQuality`
for the supported cell types, including the non-linear cells mentioned above.

Finally, a bug was fixed in `vtkCellQuality` where filters the metrics RELATIVE_SIZE_SQUARED, SHAPE_AND_SIZE, and
SHEAR_AND_SIZE were generating always 0. This was fixed by exposing the `vtkMeshQuality:ComputeAverageCellSize` function
to compute the average cell size for the input mesh and use it in `vtkCellQuality`to compute the metrics.
