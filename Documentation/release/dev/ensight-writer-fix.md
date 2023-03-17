## vtkEnSightWriter improvements

Previously, vtkEnSightWriter produced incorrect files for some inputs
(notably when the input dataset contained cells of `VTK_POLYGON`, `VTK_WEDGE`,
`VTK_QUADRATIC_WEDGE`, `VTK_QUADRATIC_EDGE` or `VTK_CONVEX_POINT_SET` cell type,
or vector/tensor cell arrays). These inputs are now saved correctly.

The writer now has the ability to write `VTK_POLYHEDRON` cells, making it suitable
for use with datasets containing polygons and polyhedra.

You can now disable writing node and element IDs to the EnSight data.
This makes the output geometry file slightly smaller.
