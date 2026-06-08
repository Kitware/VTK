## Improved `vtkCellGrid` evaluation performance and API

`vtkDGEvaluator::ClassifyPoints` has been rewritten for substantially better performance.

Classification now runs in parallel over cell batches via `vtkSMPTools`.

Before invoking the Newton-Raphson solver, each candidate probe point is tested against
the convex hull of the cell's corner points using the new `vtkConvexHull` static API;
points outside the hull are rejected immediately, avoiding the expensive Newton step for the
vast majority of probe points in practice.

The separate `EvaluatePositions` pass has been merged into `ClassifyPoints`, so parametric coordinates
computed during classification are stored directly, eliminating a redundant second pass. Divergence
detection now uses the new `GetSignedParametricDistance` method instead of a second virtual dispatch,
and the linear solve uses `partialPivLu` instead of `HouseholderQR`, which is sufficient and cheaper for
the guaranteed 3×3 Jacobian.

A bug where `ClassifyPoints` crashed when a connectivity array's component count exceeded the number of
cell corners has also been fixed.

`vtkDGInterpolateCalculator::Evaluate` and `EvaluateDerivative` now use thread-local
scratch arrays for intermediate computations, enabling safe concurrent evaluation without
per-call heap allocation.

All `vtkDGCell` subclasses now implement `GetSignedParametricDistance(rst)`, which returns
the signed distance of a parametric point from the reference-element boundary (negative
inside, zero on the boundary, positive outside). `IsInside(rst, tol)` is now a non-virtual
inline wrapper around it.

`vtkStaticPointLocator::FindPointsWithinRadius` (and its 2D counterpart) is now faster.
Buckets lying entirely outside the query sphere are rejected early via a distance-to-AABB
test, and buckets lying entirely inside it skip the per-point distance check and bulk-copy
their point IDs directly. Both optimizations also apply to `vtkStaticPointLocator2D`.
