## Fix calculation of vtkPyramid centroid

The centroid calculation for vtkPyramid was corrected.  Previously,
the calculation returned a point on one of the triangular faces.
