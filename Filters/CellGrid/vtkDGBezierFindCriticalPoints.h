// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDGBezierFindCriticalPoints_h
#define vtkDGBezierFindCriticalPoints_h

#include "vtkDGCell.h" // For Shape
#include "vtkDGOperatorEntry.h"
#include "vtkFiltersCellGridModule.h" // For export macro.

#include <array>
#include <vector>

namespace vtk
{
namespace basis
{
/// A function space for basis functions expressed in the Bernstein-Bezier basis.
namespace bezier
{
VTK_ABI_NAMESPACE_BEGIN

// Thread safety: every function below may be called concurrently from
// different threads, which is how vtkDGFindCriticalPointsResponder solves a
// grid's cells in parallel. Each thread keeps its own working memory, reused
// from one call to the next, so a call allocates little beyond the points it
// returns; the basisOp and gradOp passed in are only evaluated, never
// rebound.

/// One critical point located by FindPrismaticCriticalPoints(),
/// FindBarycentricCriticalPoints(), FindWedgeCriticalPoints(), or
/// FindPyramidCriticalPoints().
struct VTKFILTERSCELLGRID_EXPORT CriticalPoint
{
  std::array<double, 3> Parameter{ { 0., 0., 0. } };
  double Value{ 0. };
  /// The dimension of the cell entity (volume, face, or edge) this point
  /// was found on - equal to the cell's own dimension for a
  /// volume-interior point and smaller for one confined to a boundary.
  /// Corners are never reported (see the search functions below).
  int EntityDimension{ 0 };
};

/// Evaluate a scalar Bezier field and its gradient at a parametric point.
///
/// Inputs:
/// + \a basisOp / \a gradOp must already be bound to the polynomial order
///   \a coefficients uses.
/// + \a dimension is the cell's parametric dimension (1 for an edge, 2 for
///   a quadrilateral, 3 for a hexahedron).
/// + \a coefficients are a cell's control-net values (one per basis
///   function, numbered the same way vtkDGLagrangePoints numbers
///   arbitrary-order degrees of freedom).
/// + \a point is the (r, s, t) parametric location to evaluate at; only
///   its first \a dimension entries are meaningful.
///
/// Output:
/// + \a gradient is overwritten with the field's slope along each
///   parametric axis at \a point; only its first \a dimension entries are
///   written.
///
/// Returns the field's value at \a point.
VTKFILTERSCELLGRID_EXPORT double EvaluateField(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, int dimension, const std::vector<double>& coefficients,
  const std::array<double, 3>& point, std::array<double, 3>& gradient);

/// Locate every critical point (minimum, maximum, or saddle) of a scalar
/// Bezier cell-attribute strictly within the interior of one tensor-product
/// cell - not its faces, edges, or corners, which
/// FindPrismaticBoundaryCriticalPoints() covers.
///
/// Supported cell types: any cell whose parameter space is a tensor
/// product of independent axes - edges, quadrilaterals, and hexahedra.
///
/// Method: locate every root of grad(field) = 0 over the cell's own full
/// dimension with the (Interval) Projected Polyhedron algorithm of
/// Sherbrooke & Patrikalakis (CAGD 10 (1993) 379-405) - PROJECTED-POLYHEDRON,
/// their own Sect. 5, whose unlettered Steps 1-7 this file recasts into the
/// finer-grained A1-A9 step labels Reuter, Mikkelsen, Sherbrooke, Maekawa &
/// Patrikalakis (The Visual Computer 24 (2008) 187-200) introduce for their
/// own simplex generalization (Sect. 4) - which is what
/// FindBarycentricInteriorCriticalPoints() runs. Neither paper letters the
/// box algorithm's own steps that way; sharing one vocabulary across both
/// domains is this file's own convention, not a restatement either paper
/// makes. A box is the easier domain of the two: restricting a tensor net
/// to a sub-box is one independent de Casteljau clip per axis, with none of
/// the simplex version's blossom subdivision or coordinate bookkeeping. See
/// FindPrismaticInteriorPoints() in the implementation.
///
/// Every root is reported outright, so nothing refines afterwards. That
/// is what lets this report saddle points, and critical points where a
/// derivative touches zero without crossing it - neither of which a
/// descent method or a per-axis sign-change test can reach.
///
/// Limitations: the cell is searched inset by a small margin (1e-4, in
/// its own [-1, 1] parameters) rather than out to its closure. That is
/// deliberate twice over: it is the ownership rule between the interior
/// and its boundary, which FindPrismaticBoundaryCriticalPoints() searches
/// separately at each entity's own dimension, and it is what keeps a
/// field whose gradient vanishes along an entire face or edge from
/// handing a root-isolating solver a continuum to enumerate one subdomain
/// at a time - not a corner case, since a control net that is flat except
/// at a single coefficient does exactly that.
///
/// The solver's hull-versus-axis test is widened from the line y = 0 to a
/// narrow band around it, standing in for the rounded interval arithmetic
/// Sect. 5.1.1 of the 2008 paper calls for; widening can only ever admit
/// more candidates, never discard a real root. A box needs that band
/// scaled by the subdomain being tested rather than by the whole entity,
/// which a simplex does not: every coordinate can be near a face at once,
/// and in that corner these fields fall to a fixed tiny fraction of their
/// own scale.
///
/// Two critical points closer together than 1e-6 in the cell's own
/// parameters are reported as one: a single root routinely leaves several
/// root subdomains behind, and they are merged at that distance. It is also
/// the distance at which the search stops subdividing, since whatever it
/// found below it would be merged away on arrival.
///
/// A continuum lying genuinely inside the interior is still possible; both
/// the reported-point count and the iteration count are capped for that
/// case, so the search stays bounded, but what it returns then only
/// samples the set rather than describing it, and says so with a warning.
/// A field that does not vary along one axis at all is the common instance
/// and is answered outright, with nothing: its critical points are whole
/// lines, which the boundary entities those lines end on report at their
/// own dimension.
///
/// Inputs:
/// + \a basisOp / \a gradOp must already be bound to \a order.
/// + \a order is the polynomial order along each of the cell's
///   parametric axes; its size is the cell's dimension (1 for an edge, 2
///   for a quadrilateral, 3 for a hexahedron).
/// + \a coefficients are the cell's control-net values, one per basis
///   function, numbered the same way vtkDGLagrangePoints numbers
///   arbitrary-order degrees of freedom: a rectangular lattice, indexed
///   with the first parametric axis fastest.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindPrismaticInteriorCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point (minimum, maximum, or saddle) of a scalar
/// Bezier cell-attribute on the parameter-space boundary of one
/// tensor-product cell: recursively, on each face, edge, and (implicitly,
/// unreported) corner - never the cell's own interior, which
/// FindPrismaticInteriorCriticalPoints() covers.
///
/// Supported cell types: the same as FindPrismaticInteriorCriticalPoints().
///
/// Method: each boundary entity is solved separately, at its own
/// dimension and over its own slice of the control net, with the same
/// algorithm FindPrismaticInteriorCriticalPoints() runs on the cell's own
/// interior - every face and edge is itself a lower-dimensional
/// tensor-product cell, searched inset from its own boundary the same way,
/// down to (but not including) its corners.
///
/// Corners are not reported: trivially critical by construction.
///
/// Limitations: the same margin, zero band, merge radius, and continuum
/// cap FindPrismaticInteriorCriticalPoints() describes apply to each
/// boundary entity independently, in its own parameters.
///
/// Inputs: the same as FindPrismaticInteriorCriticalPoints().
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindPrismaticBoundaryCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point of a scalar Bezier cell-attribute on one
/// tensor-product cell, interior and boundary alike: the concatenation of
/// FindPrismaticInteriorCriticalPoints() and
/// FindPrismaticBoundaryCriticalPoints(), which see for method,
/// limitations, and input requirements.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindPrismaticCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point (minimum, maximum, or saddle) of a scalar
/// Bezier cell-attribute strictly within the interior of one barycentric
/// cell - not its edges or (for a tetrahedron) faces, which
/// FindBarycentricBoundaryCriticalPoints() covers.
///
/// Supported cell types: triangles and tetrahedra.
///
/// Method: a barycentric cell's control net sits on a simplex lattice
/// (indices (i, j, ...) with i + j + ... <= order), needing its own
/// indexing (SimplexIndex()/SimplexSize()). The interior is solved
/// directly, by locating every root of grad(field) = 0 with the
/// barycentric Bernstein Interval Projected Polyhedron algorithm of
/// Reuter, Mikkelsen, Sherbrooke, Maekawa & Patrikalakis, "Solving
/// nonlinear polynomial systems in the barycentric Bernstein basis" (The
/// Visual Computer 24 (2008) 187-200) - the simplex-domain generalization
/// of the box algorithm FindPrismaticInteriorCriticalPoints() runs, with
/// the same A1-A9 skeleton and the same hull test, differing only in how
/// a simplex is subdivided (one blossom sweep per vertex, and a
/// coordinate update after each). The accuracy it reports to costs little
/// to ask for: the paper declines to claim a convergence rate in general,
/// but over the randomized validation the implementation describes,
/// tightening the result by six orders of magnitude cost a factor of two
/// in iterations. See FindBarycentricInteriorPoints() in the
/// implementation.
///
/// Every root is reported outright, so nothing refines afterwards - which
/// is what lets this report saddle points, since a descent method walks
/// away from a saddle and this does not.
///
/// Limitations: the interior is searched inset by the same 1e-4 margin
/// FindPrismaticInteriorCriticalPoints() describes, in the cell's own
/// barycentric coordinates, and for the same two reasons - so a critical
/// point nearer a face than that is reported by the face's own search, at
/// the face's dimension, by FindBarycentricBoundaryCriticalPoints()
/// rather than by this one. The continuum this rules out is if anything
/// more common on a simplex: a field of the form
/// 1 - c * prod(lambda_i ^ p_i), which is what a control net that is flat
/// except at one coefficient gives, has a gradient vanishing identically
/// on every face and edge where the exponents of the vanishing
/// coordinates sum to two or more. The zero band that stands in for
/// rounded interval arithmetic is shared too, scaled here by each
/// equation's magnitude over the whole entity, which on a simplex bounds
/// it everywhere below - and unlike the box solver's, not rescaled to the
/// subdomain being tested, which was measured to lose real roots.
///
/// A face the gradient vanishes identically on is detected up front and
/// given a wider margin of its own, because 1e-4 is not enough to keep
/// clear of it: the gradient decays into such a face as lambda^(order-1)
/// rather than dropping to zero at it, which at order 5 already puts the
/// margin inside the zero band. The wider margin is set where that decay
/// still clears the band comfortably, so it grows slowly with order; see
/// DegenerateFaceInset() in the implementation. A critical point nearer
/// such a face than that is reported by the face's own search rather than
/// this one, which is the same ownership rule the ordinary margin applies,
/// drawn further out.
///
/// A continuum lying genuinely inside the interior is still possible; both
/// the reported-point count and the iteration count are capped for that
/// case, so the search stays bounded, but what it returns then only
/// samples the set rather than describing it, and says so with a warning.
///
/// Inputs:
/// + \a basisOp / \a gradOp must already be bound to \a order.
/// + \a order is the polynomial order, shared by every barycentric axis
///   (only order[0] is read, matching
///   vtkDGOperatorEntry::SimplexFunctionCount()); its size selects the
///   cell type (2 for a triangle, 3 for a tetrahedron).
/// + \a coefficients are the cell's control-net values, one per basis
///   function, numbered the same way vtkDGLagrangePoints numbers a
///   triangle's or tetrahedron's arbitrary-order degrees of freedom: the
///   principal simplex lattice, indexed with the last axis outermost and
///   the first axis innermost.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindBarycentricInteriorCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point (minimum, maximum, or saddle) of a scalar
/// Bezier cell-attribute on the boundary of one barycentric cell: on each
/// straight edge, and (for a tetrahedron) on each triangular face - never
/// the cell's own interior, which
/// FindBarycentricInteriorCriticalPoints() covers.
///
/// Supported cell types: the same as
/// FindBarycentricInteriorCriticalPoints().
///
/// Method:
/// + A tetrahedron's 4 triangular faces are searched the same way as a
///   standalone triangle's interior, over the 2-D sub-array of its
///   control net whose pinned index is 0 - which is exactly the Bezier
///   triangle the field restricts to there, not an approximation of it.
/// + Each straight edge is, on its own, an ordinary 1-D Bezier curve, so
///   it is handed to the box solver at K = 1, via a shape-specific affine
///   map back to (r, s, t).
///
/// Every entity here solves its whole system at once and reports the
/// roots, so none of them needs an optimizer afterwards - which is what
/// lets both report saddle points.
///
/// Limitations: the same margin, zero band (including the wider margin a
/// degenerate face gets), and continuum cap
/// FindBarycentricInteriorCriticalPoints() describes apply to each
/// boundary entity independently, in its own parameters.
///
/// Inputs: the same as FindBarycentricInteriorCriticalPoints().
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindBarycentricBoundaryCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point of a scalar Bezier cell-attribute on one
/// barycentric cell, interior and boundary alike: the concatenation of
/// FindBarycentricInteriorCriticalPoints() and
/// FindBarycentricBoundaryCriticalPoints(), which see for method,
/// limitations, and input requirements.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindBarycentricCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point (minimum, maximum, or saddle) of a scalar
/// Bezier cell-attribute strictly within the interior of one wedge cell -
/// not its caps, side faces, or edges, which
/// FindWedgeBoundaryCriticalPoints() covers.
///
/// Supported cell types: wedges - a triangular cross-section
/// (barycentric) extruded along an independent prismatic axis
/// (tensor-product).
///
/// Method: the interior needs machinery neither pure shape has, and it is
/// the two run together rather than anything new in kind: a node is a
/// sub-triangle of (r, s) crossed with a sub-interval of t, and one test
/// covers both. The system is the triangle's 3 pairwise barycentric
/// derivatives plus the prismatic one - 4 equations in 3 unknowns, the
/// redundancy being the kind Sect. 6.4 of the barycentric paper
/// recommends. Neither paper covers this mixed domain, but both halves of
/// the test are domain-agnostic (each only needs a control net's
/// projection onto one coordinate and that projection's convex hull), and
/// the two subdivisions act on disjoint groups of the net's indices, so
/// they compose without interfering. Every root is reported outright, so
/// saddle points are reported here exactly as they are on the two pure
/// shapes.
///
/// Limitations: the interior is searched inset by the 1e-4 margin
/// FindPrismaticInteriorCriticalPoints() describes, which decides which
/// entity a critical point belongs to - one nearer a cap or side face is
/// reported by FindWedgeBoundaryCriticalPoints() instead, at that
/// entity's own dimension. The zero band standing in for rounded interval
/// arithmetic has to be wider here than either pure shape's, and scaled
/// by the node being tested rather than by the starting one;
/// FindWedgeInteriorPoints() records what was measured.
///
/// Inputs:
/// + \a basisOp / \a gradOp must already be bound to \a order.
/// + \a order has 3 entries: order[0] and order[1] (equal) are the
///   triangular cross-section's shared order, order[2] is the
///   independent prismatic-axis order, matching
///   vtkDGOperatorEntry::WedgeFunctionCount()'s own convention.
/// + \a coefficients are the cell's control-net values, one per basis
///   function, numbered the same way vtkDGLagrangePoints numbers a
///   wedge's arbitrary-order degrees of freedom: a triangle's own
///   lattice, repeated once per point along the prismatic axis, that
///   axis outermost.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindWedgeInteriorCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point (minimum, maximum, or saddle) of a scalar
/// Bezier cell-attribute on the boundary of one wedge cell: on each of its
/// 2 triangular caps and their 6 edges, on each of its 3 quadrilateral
/// side faces, and on each of its 3 vertical edges - never the cell's own
/// interior, which FindWedgeInteriorCriticalPoints() covers.
///
/// Supported cell types: the same as FindWedgeInteriorCriticalPoints().
///
/// Method: rather than reinventing either half, a wedge's boundary
/// entities reuse whichever of the two machineries they reduce to:
///
/// + Its 2 triangular caps (prismatic axis pinned to an end) and their
///   6 edges are searched exactly like a standalone triangle's interior
///   and edges (FindBarycentricInteriorCriticalPoints() /
///   FindBarycentricBoundaryCriticalPoints()'s own primitives), the cap's
///   own triangle slice of the control net being exactly what the field
///   restricts to at that end of the axis.
/// + Its 3 quadrilateral side faces (one barycentric coordinate pinned
///   to 0, prismatic axis free) and 3 vertical edges (a triangle corner
///   pinned, prismatic axis free) are ordinary tensor-product entities
///   once extracted, solved through a caller-supplied embedding via
///   FindPrismaticInteriorCriticalPoints()'s own machinery.
///
/// Limitations: each entity inherits the margins of whichever solver owns
/// it - the 1e-4 inset FindPrismaticInteriorCriticalPoints() describes,
/// and the zero band standing in for rounded interval arithmetic. A cap
/// inherits the barycentric solver's wider margin on a face the gradient
/// vanishes identically on as well, which is what a wedge whose net is
/// flat except at one coefficient hands it.
///
/// Inputs: the same as FindWedgeInteriorCriticalPoints().
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindWedgeBoundaryCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point of a scalar Bezier cell-attribute on one
/// wedge cell, interior and boundary alike: the concatenation of
/// FindWedgeInteriorCriticalPoints() and
/// FindWedgeBoundaryCriticalPoints(), which see for method, limitations,
/// and input requirements.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindWedgeCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point (minimum, maximum, or saddle) of a scalar
/// Bezier cell-attribute strictly within the interior of one pyramid cell -
/// not its base, triangular faces, or edges, which
/// FindPyramidBoundaryCriticalPoints() covers.
///
/// Supported cell types: pyramids, in the Bernstein-Bezier basis of Chan &
/// Warburton, "A short note on a Bernstein-Bezier basis for the pyramid"
/// (arXiv:1508.05609).
///
/// Method: that basis is a product of 1-D Bernstein polynomials in the
/// collapsed (Duffy) coordinates a = (r + w) / (2w), b = (s + w) / (2w),
/// c = t, with w = 1 - t, which range over the unit cube and map onto the
/// pyramid one to one everywhere but the apex. Raising every layer of the
/// net to the same degree in a and b turns the field into an ordinary Bezier
/// hexahedron there, so the interior is solved by the same box algorithm
/// FindPrismaticInteriorCriticalPoints() runs. The system is not the plain
/// partial derivatives, though: df/da and df/db vanish on the whole c = 1
/// face the apex is blown up into, so both are divided by 1 - c first -
/// exactly, since they carry that factor - which is also what the pyramid's
/// own gradient in (r, s, t) does. Every root is reported outright, so
/// saddle points are reported here exactly as they are on every other
/// shape.
///
/// Limitations: the interior is searched inset by the 1e-4 margin
/// FindPrismaticInteriorCriticalPoints() describes, in the collapsed
/// coordinates rather than in (r, s, t) - so the margin narrows in (r, s) in
/// step with the cross-section towards the apex. A critical point outside
/// it is reported by FindPyramidBoundaryCriticalPoints() instead, at that
/// entity's own dimension - or, if it is that close to the apex itself, not
/// at all, since the apex is a corner.
///
/// Inputs:
/// + \a basisOp / \a gradOp must already be bound to \a order.
/// + \a order has 3 equal entries, the pyramid's single order n, matching
///   vtkDGOperatorEntry::PyramidFunctionCount()'s own convention.
/// + \a coefficients are the cell's control-net values, one per basis
///   function, numbered the same way vtkDGLagrangePoints numbers a
///   pyramid's arbitrary-order degrees of freedom: one square layer per
///   index k along t, of (n - k + 1)^2 points each, first axis fastest and
///   k outermost.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindPyramidInteriorCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point (minimum, maximum, or saddle) of a scalar
/// Bezier cell-attribute on the boundary of one pyramid cell: on its
/// quadrilateral base, its 4 triangular faces, its 4 base edges, and its 4
/// edges rising to the apex - never the cell's own interior, which
/// FindPyramidInteriorCriticalPoints() covers.
///
/// Supported cell types: the same as FindPyramidInteriorCriticalPoints().
///
/// Method: every boundary entity reduces to one of the other shapes'
/// machinery, the same way a wedge's do, since the field's trace on each of
/// the pyramid's faces is a Bezier polynomial on that face's own shape
/// (Chan & Warburton's Lemma 3.2):
///
/// + The base is the net's own lowest layer, an ordinary tensor-product
///   patch, solved by FindPrismaticInteriorCriticalPoints()'s machinery.
/// + Each triangular face is exactly a Bezier triangle, affinely embedded,
///   whose net is the face's own slice of the pyramid's, and is solved like
///   a standalone triangle's interior (FindBarycentricInteriorCriticalPoints()).
/// + Each edge is a 1-D Bezier curve.
///
/// Limitations: each entity inherits the margins of whichever solver owns
/// it, as FindWedgeBoundaryCriticalPoints() describes.
///
/// Inputs: the same as FindPyramidInteriorCriticalPoints().
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindPyramidBoundaryCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Locate every critical point of a scalar Bezier cell-attribute on one
/// pyramid cell, interior and boundary alike: the concatenation of
/// FindPyramidInteriorCriticalPoints() and
/// FindPyramidBoundaryCriticalPoints(), which see for method, limitations,
/// and input requirements.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindPyramidCriticalPoints(
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

/// Find every critical point of a cell of the given \a shape, dispatching to
/// the Find*CriticalPoints() entry point that matches its parameter space.
VTKFILTERSCELLGRID_EXPORT std::vector<CriticalPoint> FindCriticalPoints(vtkDGCell::Shape shape,
  vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients);

VTK_ABI_NAMESPACE_END
} // namespace bezier
} // namespace basis
} // namespace vtk

#endif // vtkDGBezierFindCriticalPoints_h
