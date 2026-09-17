## New Cell Grid "G" Basis For DG Cells

The `vtkDGCell` type in the `Filters/CellGrid` module now
supports a new set of basis functions whose coefficients are
stored at Gauss points. The basis is named "G" and supports
polynomial orders 1–5 for all cell shapes except vertices.

The basis functions are not ordered in the traditional VTK
order where degrees of freedom (DOFs) at corners appear first,
followed by mid-edge DOFs, then mid-face, then mid-volume.
Instead, the DOFs are organized in a regular grid in
parameter-space.

These new basis functions are expressed in terms of Jacobi
polynomials for simplicial (triangular, tetrahedral) and
mixed simplicial-prismatic (wedge, pyramidal) shapes.
They are simple Lagrange polynomials for prismatic
(edge, quadrilateral, hexahedral) shapes.
