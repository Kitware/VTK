# The Voronoi filters can use any point locator

vtkVoronoiFlower2D, vtkVoronoiFlower3D and vtkGeneralizedSurfaceNets3D have
new `SetPointLocator()` and `GetPointLocator()` methods, to choose the point
locator that finds the neighbors of each generating point. It is now a
vtkKdTreePointLocator by default, instead of a vtkStaticPointLocator (or a
vtkStaticPointLocator2D for vtkVoronoiFlower2D). The kd-tree adapts to the
density of the points, so clustered points are processed much faster: the
bins of the static locators are either too large in the clusters or too many
elsewhere. vtkVoronoiCore2D and vtkVoronoiCore3D, on which the filters are
built, now accept any vtkAbstractPointLocator.

The neighbors of a generating point are requested from the locator in
shells: the first one with `FindNPointsInShell()`, which holds the nearest
points, and the next ones with `FindPointsInShell()`, which grow until they
reach the circumflower, and skip the points outside of the Voronoi flower. The
static locators can still be set instead, and are requested the same way:
their output is the same tessellation as before, but its cells may be in a
different order.

The neighbors are requested through `vtkVoronoiNeighborIterator<DIM>`, in
vtkVoronoiCore.h, which vtkVoronoiCore2D (DIM = 2) and vtkVoronoiCore3D (DIM =
3) use. It passes the petals of the Voronoi flower to the locator with DIM + 1
components, which the locator expects when it is of dimension DIM. When it is
not (for example a vtkKdTreePointLocator of non-planar points in
vtkVoronoiFlower2D, or a vtkStaticPointLocator2D in vtkVoronoiFlower3D), the
filters warn, and the petals are not used, which is slower.

vtkShellBinIterator and vtkAnnularBinIterator, which visited the bins of
vtkStaticPointLocator and vtkStaticPointLocator2D for the Voronoi filters, are
deprecated. Use `FindNPointsInShell()` and `FindPointsInShell()` instead,
which every vtkAbstractPointLocator provides.

`GetLocator()` is deprecated in the three filters: it returns the locator if
it is a static one, and nullptr otherwise. Use `GetPointLocator()` instead,
for example to set the `NumberOfPointsPerLeaf` of the default kd-tree.

## Performance

Time to generate the Voronoi tessellation of vtkVoronoiFlower2D and
vtkVoronoiFlower3D (`SetOutputTypeToSpeedTest()`), with a
vtkStaticPointLocator2D (2D) or a vtkStaticPointLocator (3D) of 2 points per
bucket (the previous default of the filters), visiting its bins as before
(vtkShellBinIterator or vtkAnnularBinIterator) or requested in shells as now,
and with the default vtkKdTreePointLocator (20 points per leaf, which the filters
set). Best of 3 runs (1 run for 3D clustered 1M points), 16 threads, Apple M4
Max. The speedup is the time of the static locator before divided by the time
of the kd-tree.

The 10K, 100K and 1M points are random, not on a grid, in the square
[0,1000]^2 (with z = 0) or in the cube [0,1000]^3:

- uniform: uniformly distributed, so that 1M points hold one point per unit of
  area on average in 2D, and one per 1,000 units of volume in 3D;
- clustered: in 200 clusters whose centers are uniform in the domain, each a
  gaussian of standard deviation 2 along each axis, clipped to the domain. The
  sizes of the clusters are Zipf-weighted (the k-th one holds a share of the
  points proportional to 1/k): the largest holds 17% of the points (170K of
  1M), and the smallest 0.085% (850 of 1M).

### 2D (vtkVoronoiFlower2D)

| dataset   | points | static, before (s) | static, now (s) | kd-tree (s) | speedup |
|-----------|--------|--------------------|-----------------|-------------|---------|
| uniform   | 10K    | 0.002              | 0.002           | 0.002       | 1.00    |
| uniform   | 100K   | 0.009              | 0.011           | 0.015       | 0.60    |
| uniform   | 1M     | 0.115              | 0.111           | 0.172       | 0.67    |
| clustered | 10K    | 0.016              | 0.006           | 0.003       | 5.33    |
| clustered | 100K   | 0.331              | 0.091           | 0.023       | 14.39   |
| clustered | 1M     | 5.824              | 1.503           | 0.275       | 21.18   |

### 3D (vtkVoronoiFlower3D)

| dataset   | points | static, before (s) | static, now (s) | kd-tree (s) | speedup |
|-----------|--------|--------------------|-----------------|-------------|---------|
| uniform   | 10K    | 0.016              | 0.013           | 0.012       | 1.33    |
| uniform   | 100K   | 0.122              | 0.103           | 0.103       | 1.18    |
| uniform   | 1M     | 1.371              | 1.146           | 1.202       | 1.14    |
| clustered | 10K    | 0.034              | 0.032           | 0.026       | 1.31    |
| clustered | 100K   | 1.558              | 0.642           | 0.249       | 6.26    |
| clustered | 1M     | 115.915            | 33.445          | 3.942       | 29.41   |

On uniform 2D points, where each tile only needs a few neighbors, a
vtkStaticPointLocator2D remains about 1.5 times as fast as the kd-tree, and it
can be set with `SetPointLocator()`.
