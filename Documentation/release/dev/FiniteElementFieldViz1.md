## Visualize Discontinuous Galerkin Finite Element Fields

You can now visualize DG fields of type H(Grad), H(Curl), and H(Div) with the `vtkFiniteElementFieldDistributor` filter. It is specifically tailored to fit the needs of `vtkIOSSReader`. In order to use this filter, all the cells must be of the same type and the field data on input object must contain a `vtkStringArray` that describes an assortment of various DG fields, finite element basis types and reference cells.

### H(Grad) DG fields

The VTK cell connectivity model shares vertices among neighboring cells. While it is sufficient to represent continuous fields at the nodes, it is incapable of representing fields that are discontinuous (HGrad DG) at the shared vertices. This filter explodes the cell connectivity array such that every cell owns its points, resulting in duplicate points at the cell vertices. The HGrad DG fields are expected on the input dataset as cell-centered (`vtkCellData`) n-component arrays, where `n = no. of points in Lagrange reference element`

Supported H(Grad) discretizationsfor reference elements:
|| VTKCellType | supported order with no. of points per cell |
|-|-------------|--------------------------------------------|
|1.| VTK_LINE       | linear, quadratic (3 point), cubic (4 point)|
|2.| VTK_TRIANGLE   | linear, quadratic (6 point), cubic (10 point)|
|3.| VTK_QUAD       | linear, quadratic (9 point), cubic (16 point)|
|4.| VTK_TETRA      | linear, quadratic (10 point, 11 point, 15 point)|
|5.| VTK_WEDGE      | linear, quadratic (15 point, 18 point, 21 point)|
|6.| VTK_HEXAHEDRON | linear, quadratic (20 points), cubic (27)|


### H(Curl)/H(Div) CG fields on nodes.

While it is not accurate, the filter uses Lagrange reference elements (linear, quadratic, cubic) for HCurl/HDiv edge-nodal and face-nodal field interpolation. The Lagrange points of a cell correspond to the parametric coordinates of `vtkLagrangeXXXX`. A vector basis (HCurl/HDiv) field is evaluated on these new Lagrange points. The process followed is straightforward. After cell explosion, the filter uses pre-computed vector basis lagrange product matrices to interpolate coefficients given on edge (or) face degrees of freedom for each cell onto Lagrange points of a reference cell corresponding to that cell. These coefficients are expected on the input dataset as cell-centered (`vtkCellData`) n-component arrays named `EDGE_COEFF_YOUR_FIELD_NAME` and `FACE_COEFF_YOUR_FIELD_NAME` for edge and face degrees of freedom respectively, where `n = no. of degrees of freedom i.e, no. of edges or no. of faces`. Note the `EDGE_COEFF` and `FACE_COEFF` prefixes.

Supported H(Grad) discretizationsfor reference elements:
|| VTKCellType | supported order with no. of points per cell |
|-|-------------|--------------------------------------------|
|1.| VTK_TRIANGLE   | linear, quadratic (6 point), cubic (10 point)|
|2.| VTK_QUAD       | linear, quadratic (9 point), cubic (16 point)|
|3.| VTK_TETRA      | linear, quadratic (10 point, 11 point, 15 point)|
|4.| VTK_WEDGE      | linear, quadratic (15 point, 18 point, 21 point)|
|5.| VTK_HEXAHEDRON | linear, quadratic (20 points), cubic (27)|

Some undesirable side-effects of using this filter in a vtk pipeline -

1. surface rendering is weird
2. you end up with duplicate points (increased memory usage is one consequence. it gets worse for larger cell size)
