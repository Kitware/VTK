## vtkPolyhedron finds, interpolates and differentiates in concave cells

`vtkPolyhedron::EvaluatePosition()` and `CellBoundary()` now report a point
in a dent of a concave polyhedron as outside, so `vtkProbeFilter` and the
other filters that find cells no longer return values there.

`vtkMeanValueCoordinatesInterpolator` now reproduces linear fields on
polygonal meshes that are not convex, and at points outside a mesh. Each face
contributed as if it were seen from the inside; a face seen from its back now
counts with the opposite sign, as in the triangle-mesh formula. This corrects
the interpolation in concave `vtkPolyhedron` cells, and the results of
`vtkProbePolyhedron` and `vtkDeformPointSet` there. The faces have to be
oriented consistently, as the triangle-mesh formula and
`vtkPolyhedron::IsInside()` already require.

`vtkPolyhedron::Derivatives()` is exact for linear fields everywhere in the
cell. It sampled past the cell where a parametric coordinate was near 1, so
`vtkGradientFilter` gave wrong point gradients at most corners of a polyhedron.
