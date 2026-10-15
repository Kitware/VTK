# vtkKdTreePointLocator is now backed by nanoflann

vtkKdTreePointLocator builds and queries its kd-tree using the nanoflann
library instead of vtkKdTree. The public vtkAbstractPointLocator interface is
unchanged.

The locator supports both 2D and 3D points. When all the points are in a
plane of constant z (the z bounds are equal), it builds a 2D kd-tree over the
x and y coordinates. Its distances are then in the x-y plane, as those of
vtkStaticPointLocator2D: the z coordinate of the query points is ignored,
whereas it used to add the distance of a query point to the plane of the
points.

The new `GetDimension()` method returns the dimension of the kd-tree, 2 or 3.

The points are indexed with `int` when there are fewer than `VTK_INT_MAX` of
them, and with `vtkIdType` otherwise, as in vtkStaticPointLocator, which halves
the memory of the index of the tree.

You can now control the kd-tree's leaf size with the new
`NumberOfPointsPerLeaf` property (default 10).

You can find all the points in a spherical shell around a query point,
farther than a minimum squared distance and at most a maximum one, with the
new `FindPointsInShell()` method. It can optionally restrict the points to
those inside of at least one spherical region (a petal), and optionally
sorts them from closest to farthest. Consecutive calls on adjacent shells
(the maximum distance of a call being the minimum distance of the next)
return disjoint sets of points, and miss none. `vtkAbstractPointLocator`
provides a default implementation that filters the result of
`FindPointsWithinRadius()`, so it works on any point locator.
`vtkStaticPointLocator` and `vtkStaticPointLocator2D` override it to visit
only the bins overlapping the shell (in the x-y plane for the 2D locator),
and `vtkKdTreePointLocator` to skip the nodes of the kd-tree outside of the
shell and of the petals.

You can also find the N nearest points beyond a minimum distance, optionally
restricted to petals, with the `FindNPointsInShell()` method, which
`vtkAbstractPointLocator` now declares virtually. It returns the radius of the
shell holding them, the distance of the N-th nearest one, and exactly the
points of `FindPointsInShell()` up to that radius, so that it can start a
sequence of adjacent shells. The default implementation gathers shells of
`FindPointsInShell()` of growing radius until they hold N points, so it works
on any point locator. `vtkKdTreePointLocator` overrides it with a single
traversal of its kd-tree, and `vtkStaticPointLocator` and
`vtkStaticPointLocator2D` with a single visit of their bins: the levels of
bins around the query point until they hold N points, and then only up to the
distance of the N-th nearest of these. The static locators now test the petals
per point instead of per bin. `vtkStaticPointLocator2D::FindNPointsInAnnulus()`
is deprecated in favor of it, which does the same.

The petals have as many components as the dimension of the locator plus one: the
spheres (x,y,z,r2) of `vtkStaticPointLocator` and of a 3D
`vtkKdTreePointLocator`, and the circles (x,y,r2), in the x-y plane, of
`vtkStaticPointLocator2D` and of a 2D `vtkKdTreePointLocator`. The default
implementation of `vtkAbstractPointLocator` accepts both.

## Performance

Benchmarked on 10M build points with 100K queries, at 1 and 16 threads (Apple
M4 Max). The build points are random, not on a grid, in the cube [0,1000]^3:

- uniform: uniformly distributed, one point per 100 units of volume on average;
- clustered: in 200 clusters whose centers are uniform in the cube, each a
  gaussian of standard deviation 2 along each axis, clipped to the cube. The
  sizes of the clusters are Zipf-weighted (the k-th one holds a share of the
  points proportional to 1/k): the largest holds 17% of the points (about 1.7M),
  and the smallest 0.085% (about 8,500).

The 100K query points are drawn in the same way, with another random seed: for
the clustered data, they are in 200 other clusters, so most of them are far
from the build points.

`vtkKdTree` ("before", what `vtkKdTreePointLocator` used to wrap directly) has
no `FindNPointsInShell` and no build-time threading. Its query methods also
aren't safe to call concurrently (`vtkBSPIntersections::SelfRegister`,
reached from `FindClosestPoint`, mutates shared state with no locking,
which is a real bug found while building this benchmark), so it only gets
a 1-thread row. `FindPointsWithinRadius` was swept over R=3,6,9,12,15, and
`FindClosestNPoints` and `FindNPointsInShell` were swept over N=5,10,20,50,100.

### Build

| dataset   | engine                | 1 thread (ms) | 16 threads (ms) |
|-----------|-----------------------|---------------|-----------------|
| uniform   | vtkKdTree (old)       | 1,037.5       | n/a             |
| uniform   | vtkKdTreePointLocator | 1,806.7       | 298.5           |
| uniform   | vtkStaticPointLocator | 152.5         | 95.4            |
| clustered | vtkKdTree (old)       | 1,002.6       | n/a             |
| clustered | vtkKdTreePointLocator | 1,806.8       | 469.4           |
| clustered | vtkStaticPointLocator | 110.1         | 89.7            |

#### FindClosestPoint

| dataset   | engine                | 1 thread (ms) | 16 threads (ms) |
|-----------|-----------------------|---------------|-----------------|
| uniform   | vtkKdTree (old)       | 240.0         | n/a             |
| uniform   | vtkKdTreePointLocator | 110.8         | 9.9             |
| uniform   | vtkStaticPointLocator | 96.1          | 7.8             |
| clustered | vtkKdTree (old)       | 1,260.6       | n/a             |
| clustered | vtkKdTreePointLocator | 62.3          | 5.8             |
| clustered | vtkStaticPointLocator | **96,997.7**  | **8,459.8**     |

#### FindPointsWithinRadius

avgCount is identical across all three engines (an exact radius search has
one correct answer), so it's broken out here instead of repeated per engine:

| dataset   | R=3 | R=6 | R=9  | R=12 | R=15  |
|-----------|-----|-----|------|------|-------|
| uniform   | 1.1 | 9.0 | 30.3 | 71.4 | 139.0 |
| clustered | 0.0 | 0.6 | 6.2  | 19.4 | 37.7  |

**1 thread (ms)**

| dataset   | engine                | R=3   | R=6   | R=9   | R=12  | R=15    |
|-----------|-----------------------|-------|-------|-------|-------|---------|
| uniform   | vtkKdTree (old)       | 106.5 | 154.0 | 224.0 | 312.6 | 441.8   |
| uniform   | vtkKdTreePointLocator | 104.1 | 188.9 | 317.4 | 504.8 | 767.2   |
| uniform   | vtkStaticPointLocator | 54.7  | 162.5 | 359.8 | 663.4 | 1,059.1 |
| clustered | vtkKdTree (old)       | 46.2  | 56.6  | 75.4  | 104.8 | 133.3   |
| clustered | vtkKdTreePointLocator | 9.0   | 9.9   | 14.4  | 22.6  | 33.0    |
| clustered | vtkStaticPointLocator | 3.4   | 9.4   | 24.6  | 41.8  | 71.4    |

**16 threads (ms)**

| dataset   | engine                | R=3  | R=6  | R=9  | R=12 | R=15 |
|-----------|-----------------------|------|------|------|------|------|
| uniform   | vtkKdTreePointLocator | 10.9 | 17.1 | 27.9 | 43.8 | 64.4 |
| uniform   | vtkStaticPointLocator | 4.2  | 12.7 | 27.8 | 51.4 | 85.1 |
| clustered | vtkKdTreePointLocator | 1.0  | 1.0  | 1.4  | 2.0  | 3.0  |
| clustered | vtkStaticPointLocator | 0.5  | 0.9  | 2.3  | 4.0  | 6.1  |

Smooth on both engines, both datasets, no jumps anywhere.

#### FindClosestNPoints

Result count is always exactly N for every engine here (a guaranteed-N
query has one correct answer), so there's no separate avgCount table.

**1 thread (ms)**

| dataset   | engine                | N=5          | N=10         | N=20          | N=50          | N=100         |
|-----------|-----------------------|--------------|--------------|---------------|---------------|---------------|
| uniform   | vtkKdTree (old)       | 413.2        | 648.0        | 1,005.0       | 1,871.0       | 3,239.1       |
| uniform   | vtkKdTreePointLocator | 175.2        | 246.0        | 383.1         | 932.3         | 2,080.4       |
| uniform   | vtkStaticPointLocator | 185.1        | 266.1        | 431.7         | 1,076.8       | 1,823.6       |
| clustered | vtkKdTree (old)       | 2,668.2      | 3,547.5      | 5,168.2       | 9,364.5       | 15,516.8      |
| clustered | vtkKdTreePointLocator | 126.6        | 213.1        | 376.0         | 965.4         | 2,111.9       |
| clustered | vtkStaticPointLocator | **98,871.3** | **98,787.9** | **100,694.0** | **107,882.2** | **117,010.3** |

**16 threads (ms)**

| dataset   | engine                | N=5         | N=10        | N=20        | N=50        | N=100       |
|-----------|-----------------------|-------------|-------------|-------------|-------------|-------------|
| uniform   | vtkKdTreePointLocator | 17.0        | 22.0        | 33.4        | 76.2        | 161.2       |
| uniform   | vtkStaticPointLocator | 17.3        | 22.1        | 34.5        | 81.9        | 140.5       |
| clustered | vtkKdTreePointLocator | 12.9        | 20.5        | 35.2        | 82.8        | 172.1       |
| clustered | vtkStaticPointLocator | **8,351.4** | **8,630.0** | **8,610.8** | **9,381.1** | **9,839.4** |

#### FindNPointsInShell

Both engines return the N nearest points, and any other point at the
distance of the N-th one, so the average number of points returned, broken
out below, is the same for both:

| dataset   | N=5 | N=10 | N=20 | N=50 | N=100 |
|-----------|-----|------|------|------|-------|
| uniform   | 5.0 | 10.0 | 20.0 | 50.0 | 100.0 |
| clustered | 5.0 | 10.0 | 20.0 | 50.0 | 100.0 |

**1 thread (ms)**

| dataset   | engine                | N=5          | N=10         | N=20         | N=50         | N=100        |
|-----------|-----------------------|--------------|--------------|--------------|--------------|--------------|
| uniform   | vtkKdTreePointLocator | 212.6        | 299.3        | 455.6        | 916.8        | 1,710.6      |
| uniform   | vtkStaticPointLocator | 161.4        | 259.5        | 414.5        | 744.8        | 1,358.4      |
| clustered | vtkKdTreePointLocator | 154.6        | 264.2        | 492.7        | 1,067.5      | 1,979.8      |
| clustered | vtkStaticPointLocator | **45,594.5** | **46,161.8** | **47,844.7** | **51,116.2** | **53,833.0** |

**16 threads (ms)**

| dataset   | engine                | N=5         | N=10        | N=20        | N=50        | N=100       |
|-----------|-----------------------|-------------|-------------|-------------|-------------|-------------|
| uniform   | vtkKdTreePointLocator | 17.8        | 25.1        | 36.1        | 75.2        | 139.0       |
| uniform   | vtkStaticPointLocator | 14.0        | 20.9        | 32.6        | 57.6        | 105.8       |
| clustered | vtkKdTreePointLocator | 13.3        | 21.1        | 39.1        | 83.7        | 160.7       |
| clustered | vtkStaticPointLocator | **3,929.9** | **3,945.7** | **4,121.4** | **4,335.4** | **4,613.7** |

**FindPointsWithinRadius stays cheap at every radius tested (3 through 15),
on both engines, both datasets, and both build sizes, with smooth scaling
and no discontinuity.** The original "bucket-boundary-crossing" theory for
why it stays cheap turned out to be wrong. What the sweep actually shows is
that FindClosestPoint, FindClosestNPoints, and FindNPointsInShell on
clustered vtkStaticPointLocator all pay a large, *near-fixed* baseline
cost: at 10M build points, already about 97.0s for FindClosestPoint (a
single guaranteed result), and about 98.9s and 45.6s for FindClosestNPoints
and FindNPointsInShell at the smallest N of 5, barely moving through N=20
and only increasing noticeably past N=50. That's not "more neighbors
requires visiting more buckets." It's that landing anywhere near the
overloaded cluster costs roughly the same huge amount regardless of how
few results you actually asked for, which means these three query types
have to fully process everything in an overloaded bucket before they can
satisfy even a small N (or even N=1, for FindClosestPoint).
FindPointsWithinRadius never pays that tax at any radius tested, because
its algorithm is a direct "which buckets intersect this specific sphere"
test, with no iterative "keep expanding until I have enough candidates"
logic to force it through an overloaded bucket's full contents. So the
earlier fix, using R=3 instead of R=15 for clustered data, was still the
right call for representing a comparable *local-neighborhood* query across
datasets. It just isn't why FindPointsWithinRadius avoids the catastrophic
cost; that comes down to a structural difference between the query
algorithms themselves, not the radius size.

The FindNPointsInShell of the static locators now returns the N nearest
points, as vtkKdTreePointLocator does, instead of every point within an upper
bound of their distance (about 5,400 points on average for N=100 on the
clustered data). It also visits the overloaded bucket once instead of twice,
which almost halves its near-fixed cost on the clustered data (from about
76s to about 46s for N=5 on one thread), but that cost remains orders of
magnitude above the one of vtkKdTreePointLocator.
