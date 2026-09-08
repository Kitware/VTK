# VTK::FiltersCellGrid

## Novel discretization support in VTK

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

## Query classes

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

## Calculators

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

## Filters

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

### vtkCellGridAlgorithm and array processing

The [vtkCellGridAlgorithm](https://vtk.org/doc/nightly/html/classCellGridAlgorithm.html)
class is intended for use as a base class for algorithms that process cell grids.
Beyond the usual methods that algorithm subclasses provide, it provides a facility
for marking [cell-attributes](https://vtk.org/doc/nightly/html/classCellAttribute.html)
on input data for use by algorithms:
instead of calling `SetInputArrayToProcess()` and providing an array name or type,
you may call`SetInputAttributeToProcess()` and provide an array name (there is
no way to select cell-attributes by type at this point).
The `vtkCellGridWarp` filter is a good example of how to use these methods.

Note that the implementation of `SetInputAttributeToProcess()` simply calls
`SetInputArrayToProcess()` with a forced association to cells (since `vtkCellGrid`
does not have an concept of other association types). This means that if you wish
to expose a filter in ParaView that requires users to select an attribute,
you may use a string-vector property with the `SetInputArrayToProcess`.

## How-to: Adding a new basis function to DG cells

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

### Basis functions of arbitrary order

A basis function may instead be written for an arbitrary polynomial order, in which
case it is registered under the order -1 rather than a specific order. When a
cell-attribute asks for an order that no operator implements exactly, `vtkDGCell`
falls back to the operator registered under -1. Operators for specific orders
therefore take precedence: registering an arbitrary-order basis does not disturb
the hand-written low-order ones.

Such an operator does not know how many basis functions it provides until it is
told the order, so it is registered with a function that computes the count from
an order rather than with a literal count:

```c++
  basisMap["A"_token][-1]["vtkDGQuad"_token] =
    { TensorProductFunctionCount, 1, QuadCnBasis, Basis_HGrad_QuadCnBasis };
```

`vtkDGCell::GetOperatorEntry()` binds the entry it returns to the order the
cell-attribute requests, so callers see a concrete `NumberOfFunctions` and may
call `vtkDGOperatorEntry::Evaluate()` without handling the two cases differently.

The order is a vector holding one polynomial order per parametric axis of the
cell, which the basis function's implementation reads as `order[0]`, `order[1]`,
and `order[2]`. By default every axis takes the nominal
`vtkCellAttribute::CellTypeInfo::Order`; supplying an array in the `order` role
with a single tuple and one component per axis instead gives an anisotropic
basis. The order is fixed for all cells sharing a `CellTypeInfo` – it may not
vary from cell to cell.

Arbitrary-order implementations usually need scratch space whose size is not
known at compile time. Declare it with the `WORKSPACE(type, name, size)` macro
rather than a bare array, since the two compilation targets provide it
differently: on the CPU it expands to a reusable `thread_local` vector, and in
GLSL – where array sizes must be constant expressions and the order is baked
into the generated shader – it expands to a fixed-size array.

Note that the arbitrary-order bases number their degrees of freedom
lexicographically, with the r-axis varying fastest, rather than in the
corner-first order used by the fixed-order bases. For the simplicial shapes,
"lexicographic" means the multi-index of barycentric exponents is enumerated
with the component paired to the r-axis varying fastest.

Which orders a shape admits depends on how its parameter space is built:

+ Prismatic shapes (edge, quadrilateral, hexahedron) take an independent order
  along each axis, so the `order` role may hold a different value per axis.
+ Simplicial shapes (triangle, tetrahedron) have a single total degree shared
  by every axis, so an anisotropic order is rejected: the entry returned by
  `vtkDGCell::GetOperatorEntry()` converts to false.
+ The wedge is a triangle crossed with an edge. `order[0]` is the order of the
  triangular cross-section and `order[2]` the order along t; `order[1]` must
  equal `order[0]`.
+ A vertex has an empty parameter space and one degree of freedom at every
  order.

The pyramid has no arbitrary-order Lagrange basis. Its tensor-product structure
degenerates at the apex, so higher-order pyramid elements use *rational* shape
functions rather than polynomials; only the hand-written orders 0 through 2 are
registered.
