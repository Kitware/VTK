## More efficient and accurate EvaluatePosition for linear cells

VTK now provides simpler, faster, and more accurate `EvaluatePosition`
implementations for `vtkTriangle`, `vtkTetra`, `vtkQuad`, and `vtkHexahedron`.

### vtkTriangle and vtkTetra

The `EvaluatePosition` implementation for `vtkTriangle` and `vtkTetra` has been
simplified. A new algorithm using cell coordinates has been introduced for both
cell types, which is substantially more computationally efficient (with speeds
around x2 for `vtkTriangle` and x7 for `vtkTetra`). This implements a
long-standing comment (>20 years) on `vtkTetra.cxx` saying "could easily be sped
up using parametric localization - next release".

The implementation is based on the method sketched in
[Ericson, C. (2004). Real-time collision detection. CRC Press] in pages 137--138
and page 145, respectively, using barycentric coordinates.

To better evaluate the functions, more exhaustive tests with irregular obtuse
triangles and tetrahedra have been added to `TestTriangle` and `TestTetra`,
respectively, including all permutations of their vertices.

The new `vtkTetra::EvaluatePosition` is also less tolerant. The previous one had
a hardcoded tolerance of 0.1% around the tetrahedron. The new one is more strict,
with only a numerical tolerance proportional to
`std::numeric_limits<double>::epsilon()`. This has led to also updating
`TestBinCellDataFilter` and `TestResampleToImage`, changing the ground-truth
values. However, in contrast with the previous one, the produced bins are
independent of the order of the input cells. Thus, this is also tested in
`TestBinCellDataFilter` by several cell permutations.

### vtkQuad

`vtkQuad::EvaluatePosition` now correctly computes the closest point for
non-planar (warped) quadrilaterals. A quadrilateral is in general a bilinear
patch rather than a plane; the previous implementation fit a single plane through
three of the four corners and projected the query point onto it, which was only
exact for planar quads. The new implementation finds the closest point on the
bilinear patch directly: it locates the interior critical point of the squared
distance function and compares it against the exact closest point on each of the
four (straight) edges, keeping the global minimum. This is exact for planar quads
(the common case) and correct for warped ones. When the closest point is not
requested, only the minimal interior test needed to determine the parametric
coordinates is performed.

### vtkHexahedron

`vtkHexahedron::EvaluatePosition` is now faster and more accurate. Newton's
method is now seeded with the exact solution of the affine (parallelepiped)
approximation of the cell rather than the cell center, which noticeably reduces
the number of iterations (about half for parallelepiped-shaped cells). When the
query point lies outside the cell, the closest point is now computed by
projecting onto each of the six faces — which are themselves (possibly
non-planar) bilinear patches, handled by `vtkQuad::EvaluatePosition` — and
keeping the nearest result. The previous implementation clamped the parametric
coordinates component-wise, which was only approximate for warped hexahedra.
