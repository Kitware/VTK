## Visualize Discontinuous Galerkin Finite Element Fields

You can now visualize DG fields of type H(Grad), H(Curl),
and H(Div) with the `vtkFiniteElementFieldDistributor` filter.

This filter is specifically tailored to fit the needs of `vtkIOSSReader`.
It expects to find a `vtkStringArray` that describes an assortment of
various DG fields, finite element basis types and reference cells.
