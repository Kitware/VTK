## vtkDelaunay2D performance optimization

vtkDelaunay2D now uses a lightweight internal mesh with explicit
O(1) edge-neighbour pointers and reuses the vtkTriangle geometric
predicates, yielding approximately 2.7x speed-up on a typical
50k-point random input (2.27s -> 0.84s).

In addition, the new `UseHilbertSorter` option (default off) inserts
the points along a Hilbert curve computed with vtkHilbertCurveSorter.
Successive insertions are then spatially local, which shortens the
triangle walks and yields a further ~15x speed-up at 50k points
(~40x combined), growing with input size.

The public API is otherwise unchanged.
