# Add source: Lissajous point cloud

The `vtkLissajousPointCloud` algorithm is a source that creates a poly-data object
with vertex cells on or near to a periodic, parameterized, three-dimensional curve.
The curve is defined by a phase, frequency, and amplitude along each coordinate axis.
The vertex cells are placed regularly along the curve according to the number of
points specified and may then be perturbed by a random vector whose components each
come from a uniform distribution whose magnitude is specified along each coordinate
axis.

This source is intended for use in tests, examples, and tutorials.
