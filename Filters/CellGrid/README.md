# VTK::FiltersCellGrid
## vtkRenderingCellGrid – Novel discretization support in VTK

### Introduction

The CellGrid filter module includes queries, responders, filters, and calculators
for processing data held in a [vtkCellGrid](https://vtk.org/doc/nightly/html/classvtkCellGrid.html).

This module also introduces concrete subclasses of [vtkCellMetadata](https://vtk.org/doc/nightly/html/classvtkCellMetadata.html)
that support fixed-shape cells (vertex, edge, triangle, quadrilateral, tetrahedral, hexahedral, pyramidal, and wedge)
which admit functions from the following spaces

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
