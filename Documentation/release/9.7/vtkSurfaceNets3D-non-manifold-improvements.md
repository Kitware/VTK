## vtkSurfaceNets3D: Identify and correct non-manifold points

`vtkSurfaceNets3D` now detects local 2x2x2 voxel neighborhoods which can yield non-manifold
edges/vertices if incident faces share points naively. These configurations can occur even for
binary segmentations (single extracted label vs background), and additional cases can arise in
multi-label images depending on how labels meet in the neighborhood.

When a known split pattern exists, the filter resolves such neighborhoods by selectively duplicating
points so that incident faces are separated into locally consistent components while still keeping
coincident geometry where regions share a face. Some detected non-manifold neighborhoods may remain
unresolvable; in those cases, the output may still contain non-manifold connectivity locally.

To aid inspection/debugging, the filter now adds a point-data array named `NonManifoldTableIndices`
(signed int8) to the output. For each generated output point:

- `-2` means the neighborhood was treated as manifold (no point duplication).
- `-1` means the neighborhood was detected as non-manifold but no split pattern could be selected.
- `0` or greater means a split pattern was selected; the value is an index into an internal
  non-manifold metadata table used to choose the point duplication pattern.

Additional related improvements:

1. The performance of the triangulation of the quads has been improved
2. A crash was fixed when `DataCaching` is off
3. Pass 2 is run in odd/even slice phases to avoid a potential race when reading
   neighboring-slice edge metadata
4. Output generation has been accelerated by adding an additional pass
   (`BuildPointGeneratingEdgeRowXIndices`) which builds point-generating
   x-indices for each edge row (used to avoid scanning
   many non-point-generating triads inside a trimmed interval). The corresponding
   index array uses a compact unsigned integer type based on the x-dimension.
5. `SetOptimizedSmoothingStencils` has been deprecated since it was not actually used.
