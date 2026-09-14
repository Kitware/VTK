## vtkCellGridToUnstructuredGrid: Add SubdivisionLevel and Enable Multithreading

`vtkCellGridToUnstructuredGrid` can now approximate a cell-grid with more than one linear cell per input cell. Call
`SetSubdivisionLevel(n)` to subdivide each input cell into 2^n pieces along every parametric axis; the default of 0
preserves the previous behavior. Output cells stay linear — the level refines the output mesh (h-refinement) rather than
raising the degree of its cells — but sampling the grid more densely follows curved geometry and higher-order
cell-attributes much more closely. Every shape subdivides into cells of its own shape, except the pyramid, which yields
a mix of pyramids and tetrahedra. No upper limit is imposed on the level, but the cell count grows quickly: a hexahedron
at level 3 becomes 512 hexahedra.

The filter is also multithreaded with `vtkSMPTools`.

Three changes affect existing pipelines:

* Output points are merged using a distance relative to the input's size (`SetPointMergeTolerance`, a fraction of the
  bounding-box diagonal) rather than an absolute 1e-3, which used to collapse the points of small models.
* Cells whose shape attribute is higher-order now carry the number of points their cell type requires. Previously each
  one kept every shape degree of freedom as a point, so a quadratic quadrilateral became a `VTK_QUAD` holding 9 point
  IDs, and its attributes were sampled at the cell center.
* Output points are merged with `vtkStaticPointLocator` instead of `vtkIncrementalOctreePointLocator`. The tolerance
  means the same thing, but output points come out in a different order, so anything relying on particular output point
  IDs will see them move.

`vtkCellGridToUnstructuredGrid::Query::GetConnectivityTransform()` and the
`ConnectivityTransformType` alias have been removed: output points are samples of the input cells, so they no longer
correspond to input point IDs.

Responders of this query need updating. A `GenerateSamples` pass now runs between `CountOutputs` and
`GenerateConnectivity`, and `Query::GetLocator()` and `Query::GetConnectivityCount()` are gone: the query merges the
samples and counts each output point's contributions itself. Responders report their counts in `CountOutputs` and then
write their output at the offsets the query gives them, rather than appending to it.
