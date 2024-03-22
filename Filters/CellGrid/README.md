# VTK::FiltersCellGrid
## vtkRenderingCellGrid – Novel discretization support in VTK

### Introduction

The CellGrid filter module includes queries, responders, filters, and calculators
for processing data held in a [vtkCellGrid](https://vtk.org/doc/nightly/html/classvtkCellGrid.html).

This module also introduces concrete subclasses of [vtkCellMetadata](https://vtk.org/doc/nightly/html/classvtkCellMetadata.html)
that support fixed-shape cells (vertex, edge, triangle, quadrilateral, tetrahedral, hexahedral, pyramidal, and wedge)
which admit functions from the following function spaces

+ constant – traditional cell-constant functions (similar to vtkCellData on vtkDataSet objects)
+ HGrad – traditional nodal Lagrangian interpolants (similar to vtkPointData on vtkDataSet objects)
+ HDiv – divergence-based vector fields whose shape functions (defined for each (d-1)-dimensional side)
  ensure vectors are normal to each side of the parent cell.
+ HCurl – curl-based vector fields whose shape functions (defined for each 1-dimensional side)
  ensure continuity of the portion of any vector that is directed along the edge of a cell.

Note that only 2-d and 3-d cell-shapes can have HDiv and HCurl
[cell-attributes](https://vtk.org/doc/nightly/html/classvtkCellAttribute.html).
This is enforced by having those cell metadata classes inherit
[vtkDeRhamCell](https://vtk.org/doc/nightly/html/classvtkDeRhamCell.html).

### Query classes

Queries (and their registered responder objects that answer the query for a given cell type)
are the basic building block for cell-grids.

+ [vtkCellGridRangeQuery](https://vtk.org/doc/nightly/html/classvtkCellGridRangeQuery.html)
  – Obtain the range of values taken on by a cell-attribute defined on cell of a mesh.
+ [vtkCellGridCopyQuery](https://vtk.org/doc/nightly/html/classvtkCellGridCopyQuery.html)
  – Copy the contents of one vtkCellGrid into another (either by reference or value).
+ [vtkCellGridSidesQuery](https://vtk.org/doc/nightly/html/classvtkCellGridSidesQuery.html)
  – Create a new vtkCellGrid holding sides (i.e., boundaries of some lower dimension) of the
  cells in another vtkCellGrid.
+ [vtkCellGridEvaluator](https://vtk.org/doc/nightly/html/classvtkCellGridEvaluator.html)
  – Evaluate the values of a cell-attribute at a set of input points.
  The points may be specified either as world coordinates (in which case the first step is
  to identify their containing cells) or as pairs of a cell ID and parametric coordinates
  within the cell.
+ [vtkCellGridElevationQuery](https://vtk.org/doc/nightly/html/classvtkCellGridElevationQuery.html)
  – Add a cell-attribute whose values correspond to distance in world coordinates (either
  the distance from a point or the distance along a particular direction).

### Calculators

In addition to queries and responders – which operate on cells – the vtkCellGrid
also provides [vtkCellAttributeCalculator](https://vtk.org/doc/nightly/html/classvtkCellAttributeCalculator.html).
Attribute calculators operate on a combination of a particular type of cell and a particular type
of attribute to implement functionality required to evaluate, invert, provide metadata on,
or perform any other task for attributes defined on cell-grids.

+ [vtkCellAttributeInformation](https://vtk.org/doc/nightly/html/classvtkCellAttributeInformation.html)
  – Return metadata about how array data is transformed into field values for a given attribute.
  This is used, for example, by the discontinuous-Galerkin cells in their render responder to
  customize shaders for the attribute being rendered.
+ [vtkInterpolateCalculator](https://vtk.org/doc/nightly/html/classvtkInterpolateCalculator.html)
  – Interpolate a cell-attribute at a given point.

### Filters

For cell-grids, VTK algorithms are thin wrappers around a corresponding query.
In fact, a subclass of `vtkAlgorithm` can simply provide a child class which
inherits `vtkCellGridQuery`. An example of this is the
[vtkUnstructuredGridToCellGrid](https://vtk.org/doc/nightly/html/classvtkUnstructuredGridToCellGrid.html)
algorithm, which declares its query as a child class named
`vtkUnstructuredGridToCellGrid::TranscribeQuery`.

Because algorithms for cell-grids typically use the query-responder pattern,
they tend to do very little work: all of the data processing is performed by
responders that handle specific cell types and/or cell-attribute calculators
that do work for a particular (cell-type, cell-attribute-type) tuple.

### How-to: Adding a new basis function to DG cells

Note that you can query a cell-attribute for a `vtkCellAttribute::CellTypeInfo` object
given any cell type-name. This allows each cell type to use different methods to
interpolate values for the same attribute.
The `CellTypeInfo` object stores the following

+ `DOFSharing` – a string token indicating which array-group (`vtkDataSetAttributes`)
  the values for the degrees-of-freedom (DOF) of the attribute come from. If invalid,
  this indicates that the values are not shared but instead provided directly on a
  per-cell basis. If valid, then a connectivity array allows multiple cells to
  reference values in the array group.
+ `FunctionSpace` – a string-token indicating the type of function used to define
  the attribute.
  For DG cells, this is `constant` for traditional cell-constant data; `HGRAD` for
  traditional point-based Lagrange interpolation; `HCURL` for Nédélec-style edge bases;
  and `HDIV` for Thomas-Raviart face bases.
+ `Basis` – a string token indicating the particular basis inside the function
  space that is used by the attribute on cells of the given type.
  For DG cells, this is `I` for "incomplete" (i.e., serendipity) bases, `C` for
  "complete" polynomial bases, and `F` for "full" bases. These determine whether
  the full tensor product of polynomials is covered by a cell's basis functions or
  some are omitted. (`F` is used when the basis is enriched; e.g., the 15-node
  tetrahedron.) For a given polynomial order, the number of basis function in `I`
  should be less than `C` should be less than `F`.
+ `Order` – the polynomial order of the basis functions.
  This is an integer specifying the "nominal" polynomial order of the basis.
  The nominal order is the polynomial order along each parametric axis of the cell.
  This should not be confused with the actual order of the polynomial interpolant
  which is generally a multiple of this number.

Let's consider how support for the 15-node tetrahedron was added.
The "C"omplete basis was already taken by a 10-DOF tet,
so we used "F"ull as the basis descriptor (e.g., "HGRAD F2").

Assuming you have the basis functions, you'll need to edit the following files:

1. Add the basis function and its derivative to their own header files
   in the `Filters/CellGrid/Basis` directory, following the naming scheme already
   present. For our example, we added `TetF2Basis.h` and `TetF2Gradient.h`
   to the `Filters/CellGrid/Basis/HGrad` directory.
2. Add the header files to `Filters/CellGrid/CMakeLists.txt` so they are
   encoded as strings for use inside glsl shaders.
3. Add the basis functions to the appropriate registrar class in `Filters/CellGrid`:
   `vtkDGConstantOperators.cxx`, `vtkDGHGradOperators.cxx`, `vtkDGHCurlOperators.cxx`,
   or `vtkDGHDivOperators.cxx`. For our example, we added to the second file listed above.
   You must (1) implement a function that includes the given header file and (2) edit
   the `RegisterOperators()` function to add your new functions.
   The `RegisterOperators()` functions are called by the `vtkFiltersCellGrid` class.
   If you want to add operators external to VTK, your application will need to ensure
   these functions are called on application startup.
4. Make the IOSS reader deal with these elements in
   `IO/IOSS/vtkIOSSCellGridUtilities.cxx` (`GetShapeAttributeType` and elsewhere).
   This adds support for reading your data via Exodus `.exdg` files (with the IOSS library).
5. Add a test. Unless you like broken code, you need to exercise the above. Add a small
   (10kB to 100kB) test file containing cells using your new basis.

Where possible, adhere to the existing naming schemes for `FunctionSpace` and `Basis`.
This will make your life easier. But you do need to ensure that no other
bases will conflict with your new cell shape and basis.
