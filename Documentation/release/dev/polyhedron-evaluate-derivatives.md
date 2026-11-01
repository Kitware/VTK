## vtkPolyhedron finds points in concave cells and differentiates at its corners

`vtkPolyhedron::EvaluatePosition()` and `CellBoundary()` now report a point
in a dent of a concave polyhedron as outside, so `vtkProbeFilter` and the
other filters that find cells no longer return values there.

`vtkPolyhedron::Derivatives()` is exact for linear fields everywhere in the
cell. It sampled past the cell where a parametric coordinate was near 1, so
`vtkGradientFilter` gave wrong point gradients at most corners of a polyhedron.
