## vtkGhostCellsGenerator: Improvements

### Add Tolerance Parameter

`vtkGhostCellsGenerator` provides three distinct paths for identifying ghost cells:

1. Utilize the global point IDs present in your dataset
2. If global point IDs were not present:
    1. Generate them on the fly using `vtkGenerateGlobalIds`.
    2. Use a `vtkAbstractPointLocator` to match coincident points.

Previously, these two paths when global point IDs were not present applied inconsistent tolerances, leading to divergent
ghost results:

* The `vtkGenerateGlobalIds` path enforced a strict **zero tolerance**.
* The point locator path utilized a dynamic, value-dependent precision tolerance based on point coordinates (scaling
  with `epsilon`).

This update resolves the discrepancy by introducing a configurable `Tolerance` parameter to `vtkGhostCellsGenerator`,
ensuring identical and predictable results across both paths.

* **Default Value:** The default tolerance is set to `std::numeric_limits<double>::epsilon()`. A strict zero-tolerance
  default was avoided to prevent potential matching issues when datasets contain varying point precisions.
* **Special Note:** If your dataset contains topologically distinct points with bit-identical coordinates that need to
  be strictly separated, you can explicitly configure a zero tolerance.
* **Point precision:** The point locator path matches the points with `vtkKdTreePointLocator`, which compares them in
  double precision. The previous `vtkKdTree` stored the points as floats, so with the default tolerance, the
  coincident points of a dataset with double coordinates were not matched and no ghost cells were generated.
* **Bounding boxes:** The bounding boxes of the partitions are inflated by the `Tolerance`, so that points that match
  within the tolerance are no longer missed when it is larger than the default inflation.
* **Shared points:** When global point IDs are not present, a point owned by several blocks and sent to the same block
  by more than one of them is no longer duplicated in the output.

### Faster generation for unstructured data

`vtkGhostCellsGenerator` generates the ghost cells of `vtkUnstructuredGrid` and `vtkPolyData`
partitions much faster:

* The interface of a partition, the points it may share with its neighbors, is extracted with
  `vtkUnstructuredGridSides` instead of `vtkDataSetSurfaceFilter` and `vtkFeatureEdges`, so
  `VTK::ParallelDIY` no longer depends directly on `VTK::FiltersGeometry`.
* The partitions exchange which bins of their bounding boxes their interface occupies, and only
  send the points of their interface that lie in the occupied bins of a neighbor.
* The interface points are matched, the cells and points to send are found, and the buffers sent
  and the output arrays are filled in parallel with `vtkSMPTools`.

Generating one layer of ghost cells on about 2.5 million cells in 27 partitions, with 12 threads
in total:

| Cells                   | MPI ranks | Global ids generated | Before (s) | After (s) | Speedup |
|-------------------------|-----------|----------------------|------------|-----------|---------|
| Hexahedra               | 1         | No                   | 0.447      | 0.165     | 2.7x    |
| Hexahedra               | 4         | No                   | 0.132      | 0.069     | 1.9x    |
| Polyhedra               | 1         | No                   | 3.115      | 0.258     | 12.1x   |
| Polyhedra               | 4         | No                   | 0.879      | 0.121     | 7.3x    |
| Quads (`vtkPolyData`)   | 1         | No                   | 0.431      | 0.066     | 6.5x    |
| Quads (`vtkPolyData`)   | 4         | No                   | 0.119      | 0.028     | 4.3x    |
| Hexahedra               | 1         | Yes                  | 1.685      | 1.422     | 1.2x    |
| Hexahedra               | 4         | Yes                  | 0.513      | 0.449     | 1.1x    |
| Polyhedra               | 1         | Yes                  | 18.78      | 16.26     | 1.2x    |
| Polyhedra               | 4         | Yes                  | 6.030      | 5.011     | 1.2x    |
| Quads (`vtkPolyData`)   | 1         | Yes                  | 1.720      | 1.361     | 1.3x    |
| Quads (`vtkPolyData`)   | 4         | Yes                  | 0.525      | 0.443     | 1.2x    |

When `GenerateGlobalIds` is on, most of the time goes to generating the global ids.

### Global ids of flat data

`vtkGenerateGlobalIds`, which `vtkGhostCellsGenerator` uses when `GenerateGlobalIds` is on, no
longer hangs with several MPI ranks on data that is flat, such as points in a plane.

### Polyhedra

Inputs with polyhedra and ghost cells to remove no longer crash, and the point ids of the faces
of their polyhedra are now correct. Polyhedra that share faces in the input no longer lose some of
them in the output, where the faces of each polyhedron are copied apart.
