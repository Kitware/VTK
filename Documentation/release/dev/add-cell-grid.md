## New Data Object Class: vtkCellGrid

We've added a new subclass of vtkDataObject to VTK named vtkCellGrid.
It exists to support finite element techniques using novel function
spaces, which violate vtkDataSet's assumptions – especially
discontinuous Galerkin (DG) elements.

See [this discussion](https://discourse.vtk.org/t/discontinuous-galerkin-elements-and-other-novel-cell-types-function-spaces/9209)
on VTK's discourse site for details on the approach.
Currently, we support rendering of O(1) DG fields on hexahedra and
tetrahedra with an O(1) CG shape function (i.e., the geometric shape
is C0 but the fields defined over the shape may have discontinuities
at element boundaries). More shapes (triangles, quadrilaterals, wedges,
and pyramids) and higher order basis functions will be added in the
near future (CY23 Q1-Q2).

We expect the API to change over the time before the next VTK
release as we add more functionality, but the basic query-responder
pattern that allows new cell types will preserved.
If you develop against this API, be ready for some changes in the
near future.
