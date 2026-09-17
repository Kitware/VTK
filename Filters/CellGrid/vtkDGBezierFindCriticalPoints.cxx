// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGBezierFindCriticalPoints.h"

#include "vtkLogger.h"
#include "vtkMath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <tuple>
#include <vector>

namespace vtk
{
namespace basis
{
namespace bezier
{
VTK_ABI_NAMESPACE_BEGIN

// -----------------------------------------------------------------------
// Overview
// -----------------------------------------------------------------------
// Every FindXCriticalPoints() entry point at the bottom of this file
// breaks a cell into entities - its interior, then each face, edge and
// (implicitly, unreported) corner of its parameter-space boundary - and
// solves grad(field) = 0 on each one separately, at that entity's own
// dimension. Every entity is solved outright, by a Bernstein
// Projected-Polyhedron subdivision solver: three of them, one per
// parameter-space shape - box, simplex, and the wedge's simplex-x-interval
// product - sharing the same A1-A9 skeleton and the same hull test
// (HullZeroSpan()) but differing in how each subdivides its own domain. The
// pyramid adds none of its own: in collapsed coordinates each of its
// entities is a box, a triangle or an edge.
//
// The A1-A9 labels are Reuter, Mikkelsen, Sherbrooke, Maekawa &
// Patrikalakis's own (Visual Computer 24 (2008) 187-200, Sect. 4), written
// for their simplex algorithm. This file reuses that numbering for the box
// and wedge domains too, as a shared vocabulary across all three, since the
// three skeletons line up step for step - not because either paper letters
// the box algorithm that way itself (Sherbrooke & Patrikalakis, CAGD 10
// (1993) 379-405, numbers its own steps 1-7, unlettered). Nothing is refined
// afterwards: each solver reports roots, not starting guesses.
//
// Pseudocode for each entry point:
//
// FindPrismaticCriticalPoints(order, coefficients)  -- edge, quad, hex
//   SearchEntity(the whole cell, every axis free):
//     entityPoints = FindPrismaticInteriorPoints(this entity's own slice)  // box Bernstein PP
//     AppendPrismaticInterior(entityPoints)          // already located; nothing to refine
//     for each free axis:
//       for boundaryValue in {0, order[axis]}:
//         SearchEntity(one dimension lower, that axis now fixed)  // recurse
//   // a corner (0 free axes) is trivially critical and never searched
//
// FindBarycentricCriticalPoints(order, coefficients)  -- triangle, tetrahedron
//   interiorPoints = FindBarycentricInteriorPoints(whole simplex)  // barycentric Bernstein IPP
//   AppendBarycentricInterior(interiorPoints)         // already located; nothing to refine
//   for each straight edge (2 barycentric coords free, rest pinned to 0):
//     SearchBarycentricEdge(edge)                     // box solver at K = 1, own embedding
//   if tetrahedron:
//     for each triangular face (1 barycentric coord pinned to 0):
//       facePoints = FindBarycentricInteriorPoints(that face's own net)  // same, at D = 2
//       AppendBarycentricInterior(facePoints)         // dLocal = 2, embedded into 3-D
//
// FindWedgeCriticalPoints(order, coefficients)  -- wedge (mixed)
//   interiorPoints = FindWedgeInteriorPoints(whole wedge)  // joint simplex-x-interval Bernstein PP
//   AppendIfNew(interiorPoints)                        // already located; nothing to refine
//   for each cap (prismatic axis pinned to -1 or +1):
//     capPoints = FindBarycentricInteriorPoints(that cap's own triangle slice)  // same, at D = 2
//     AppendBarycentricInterior(capPoints)             // dLocal = 2, prismatic axis held fixed
//     for each of that cap's 3 triangle edges:
//       SearchBarycentricEdge(edge)                    // same 1-D machinery as a plain triangle
//   for each of the 3 quadrilateral side faces (1 barycentric coord pinned to 0):
//     SearchBarycentricFace(face)                      // box solver at K = 2, own embedding
//   for each of the 3 vertical edges (a triangle corner pinned):
//     SearchBarycentricEdge(edge)                      // same 1-D machinery, along t
//
// FindPyramidCriticalPoints(order, coefficients)  -- pyramid (collapsed box)
//   interiorPoints = FindPyramidInteriorPoints(whole pyramid)  // box solver, collapsed coords
//   AppendIfNew(interiorPoints)                        // already located; nothing to refine
//   SearchBarycentricFace(the quadrilateral base)      // box solver at K = 2, own embedding
//   for each of the 4 triangular faces:
//     facePoints = FindBarycentricInteriorPoints(that face's own triangle net)  // same, at D = 2
//     AppendBarycentricInterior(facePoints)            // dLocal = 2, affine embedding
//   for each of the 4 base edges and the 4 edges rising to the apex:
//     SearchBarycentricEdge(edge)                      // same 1-D machinery
//
// Every path ends the same way: a point is deduplicated against what has
// already been found before being appended, so that an entity searched
// from more than one parent, or a point close enough to a boundary that
// the boundary's own search finds it too, is only reported once. See
// AppendIfNew().
//
// Memory: each thread keeps one Scratch for the solvers' working memory, one
// EntityNets for the nets the entry points cut out of a cell, and one
// FieldValues for EvaluateField(), so that a search allocates little beyond
// the points it returns, and cells can be solved concurrently without
// locking. See AcquireScratch().
// -----------------------------------------------------------------------

namespace
{

// -----------------------------------------------------------------------
// Shared by all three solvers
// -----------------------------------------------------------------------

/// A stack of reusable net buffers, so a subdivision node that is pruned,
/// accepted, or bisected leaves its whole System behind for a later node's
/// System to take, rather than the search allocating and freeing its way
/// down the tree. Every solver draws from the one in its Scratch, including
/// for the equations it starts from; only how many nets one node's System
/// holds differs between them, which Recycle()'s array overload takes as a
/// template argument.
class NetPool
{
public:
  /// One buffer from the pool, or an empty one if the pool has none to
  /// give - the caller resizes it either way.
  std::vector<double> Take()
  {
    std::vector<double> buffer;
    if (!this->Pool.empty())
    {
      buffer.swap(this->Pool.back());
      this->Pool.pop_back();
    }
    return buffer;
  }

  /// Return \a buffer to the pool for a later Take() to reuse.
  void Recycle(std::vector<double>& buffer)
  {
    if (buffer.capacity() > 0)
    {
      this->Pool.push_back(std::move(buffer));
    }
  }

  /// Recycle() every buffer of one node's whole System at once.
  template <std::size_t Count>
  void Recycle(std::array<std::vector<double>, Count>& nets)
  {
    for (auto& net : nets)
    {
      this->Recycle(net);
    }
  }

private:
  std::vector<std::vector<double>> Pool;
};

/// The part of the entity a node of a solver's problem stack stands for, in
/// the entity's own parameters - one kind per solver.
///
/// A sub-box of a tensor-product entity, as its extent along each of its K
/// axes in [-1, 1].
template <std::size_t K>
struct BoxRegion
{
  std::array<double, K> Lo;
  std::array<double, K> Hi;
};

/// A sub-simplex of a D-dimensional simplex, as its D + 1 vertices in the
/// entity's own (lambda_1 .. lambda_D) coordinates.
template <std::size_t D>
struct SimplexRegion
{
  std::array<std::array<double, D>, D + 1> Vertices;
};

/// A sub-triangle of a wedge's cross-section, as its vertices in (r, s),
/// crossed with a sub-interval [Lo, Hi] of t.
struct WedgeRegion
{
  std::array<std::array<double, 2>, 3> Vertices;
  double Lo;
  double Hi;
};

/// One node of a solver's problem stack: a region of the entity, and the
/// system of Equations nets re-expressed over it.
template <typename RegionT, std::size_t Equations>
struct SearchNode
{
  RegionT Region;
  std::array<std::vector<double>, Equations> System;
};

/// The nodes each solver's stack holds: a box's K equations are its K
/// partial derivatives; a simplex's, the derivative along every pair of its
/// D + 1 barycentric coordinates; a wedge's, the triangle's 3 such pairs and
/// its prismatic axis.
template <std::size_t K>
using BoxNode = SearchNode<BoxRegion<K>, K>;
template <std::size_t D>
using SimplexNode = SearchNode<SimplexRegion<D>, D*(D + 1) / 2>;
using WedgeNode = SearchNode<WedgeRegion, 4>;

/// A solver's working memory, threaded through the subdivision rather than
/// allocated where it is used.
///
/// Every routine below that these are passed to runs once per equation per
/// coordinate per node of the subdivision tree - tens of times per node,
/// and up to the iteration cap in nodes - and each one wants a handful of
/// small arrays whose size is the polynomial order, the one quantity in
/// this file that genuinely is not known at compile time. Carrying them
/// down, and keeping them from one search to the next (AcquireScratch()),
/// leaves a search allocating little beyond the points it returns.
///
/// Pool and Stacks are live for a whole search, Net for a whole
/// re-expression, and Sources and Blossom are tables kept across searches.
/// Every other buffer is only live within one call, and none of them nests
/// inside another's use.
struct Scratch
{
  /// The two bounding curves of each projection HullZeroSpan() consumes,
  /// one pair per coordinate a single sweep projects onto. The box solver
  /// projects one axis at a time and uses pair 0 alone (PrismaticRootInterval());
  /// SimplexAllLayerBounds() fills one pair per barycentric coordinate, D + 1
  /// of them; WedgeNodeBounds() fills the triangle's 3 and the prismatic
  /// axis as pair 3. So 4 pairs is the most any of them needs.
  std::array<std::vector<double>, 4> Smallest;
  std::array<std::vector<double>, 4> Largest;
  /// The monotone chain LowerHullZeroSpan() builds.
  std::vector<int> Hull;
  /// One 1-D run of control points, and the other piece when it is split in
  /// two - the run itself keeps the right one (see SplitAt()).
  std::vector<double> Line;
  std::vector<double> Left;
  /// The de Casteljau ladder SimplexMoveVertex() sweeps, orders n down to 0,
  /// and the per-level pointers its gather addresses them through - level 0
  /// being the caller's own net rather than a copy of it, so Nets[0] is
  /// never used.
  std::vector<std::vector<double>> Nets;
  std::vector<const double*> Levels;
  /// One whole net held beside the ones being worked on: a re-expressed net
  /// while the net it was built from is still needed (SimplexReexpress(),
  /// WedgeReexpress()), or a pyramid's net raised to the tensor net its
  /// equations are built from (PyramidTensorNet()).
  std::vector<double> Net;
  /// The source indices SimplexDeCasteljau() reads, by simplex dimension
  /// and then by the order it steps down from; see SimplexSources().
  std::array<std::vector<std::vector<int>>, 4> Sources;
  /// Where SimplexMoveVertex() gathers its result from, by simplex
  /// dimension and then by order and moved vertex; see BlossomSources().
  std::array<std::vector<std::vector<int>>, 4> Blossom;
  /// Every net a solver's nodes hold, when no node holds it.
  NetPool Pool;
  /// Each solver's problem stack, one per kind of node in use: boxes of 1, 2
  /// and 3 dimensions (edges, faces and volumes, and the pyramid interior),
  /// triangles, tetrahedra, and the wedge. Taken by Stack<Node>(), which
  /// picks the stack by its node type.
  std::tuple<std::vector<BoxNode<1>>, std::vector<BoxNode<2>>, std::vector<BoxNode<3>>,
    std::vector<SimplexNode<2>>, std::vector<SimplexNode<3>>, std::vector<WedgeNode>>
    Stacks;

  /// The problem stack of solvers whose nodes are \a Node.
  template <typename Node>
  std::vector<Node>& Stack()
  {
    return std::get<std::vector<Node>>(this->Stacks);
  }

  /// Empty every working buffer for a new search, keeping its capacity.
  ///
  /// Sources, Blossom and Pool are left as they are: the first two are
  /// tables that depend only on (D, n), never on the coefficients, so they
  /// hold for every later search at that order too, and Pool holds nothing
  /// but spare buffers.
  void Clear()
  {
    for (auto& curve : this->Smallest)
    {
      curve.clear();
    }
    for (auto& curve : this->Largest)
    {
      curve.clear();
    }
    this->Hull.clear();
    this->Line.clear();
    this->Left.clear();
    for (auto& net : this->Nets)
    {
      net.clear();
    }
    this->Levels.clear();
    this->Net.clear();
    // Every solver hands its stack's nets back to Pool before returning, so
    // what is cleared here holds nothing but empty nodes.
    std::apply([](auto&... stack) { (stack.clear(), ...); }, this->Stacks);
  }
};

/// This thread's Scratch, cleared for a new search.
///
/// thread_local rather than one per search, so that its buffers - and the
/// nets in its Pool, and the simplex tables - are allocated once per thread
/// rather than once per entity of every cell: a hexahedron alone is 27
/// searches. Per thread, so that vtkDGFindCriticalPointsResponder can solve
/// cells concurrently with no locking at all.
///
/// Only the four solvers take it - FindPrismaticInteriorPoints() (or
/// SolvePrismaticSystem() under FindPyramidInteriorPoints()),
/// FindBarycentricInteriorPoints() and FindWedgeInteriorPoints() - and none
/// of them calls another, so no search clears it under one still using it.
/// That is also why the nets the entry points cut out of a cell to hand to
/// those solvers live in EntityNets rather than here: a solver would clear
/// its own input.
Scratch& AcquireScratch()
{
  static thread_local Scratch scratch;
  scratch.Clear();
  return scratch;
}

/// The nets an entry point cuts out of a cell's own to hand to a solver -
/// one entity at a time, each overwritten by the next. Kept apart from
/// Scratch because they are the solvers' input, not their working memory:
/// see AcquireScratch().
struct EntityNets
{
  /// A face's, or a whole entity's, net (ExtractEntityCoefficients()).
  std::vector<double> Face;
  /// An edge's net: one 1-D run.
  std::vector<double> Edge;

  void Clear()
  {
    this->Face.clear();
    this->Edge.clear();
  }
};

/// This thread's EntityNets, cleared for a new cell. Taken once per entry
/// point, by FindXBoundaryCriticalPoints(), for the same reasons
/// AcquireScratch() is thread_local.
EntityNets& AcquireEntityNets()
{
  static thread_local EntityNets nets;
  nets.Clear();
  return nets;
}

/// The basis values and gradients EvaluateField() fills, one set per
/// thread.
///
/// Its own rather than part of Scratch: EvaluateField() is public, and
/// vtkDGFindCriticalPointsResponder calls it outside any search, so it must
/// not take - and clear - the solvers' working memory. Per thread because it
/// runs once per reported point, from concurrent cells.
struct FieldValues
{
  std::vector<double> Basis;
  std::vector<double> Gradient;

  void Clear()
  {
    this->Basis.clear();
    this->Gradient.clear();
  }
};

/// This thread's FieldValues, cleared for a new evaluation.
FieldValues& AcquireFieldValues()
{
  static thread_local FieldValues values;
  values.Clear();
  return values;
}

/// The largest magnitude of any coefficient of \a net - the yardstick each
/// solver scales its zero band by.
double MaxAbs(const std::vector<double>& net)
{
  double magnitude = 0.;
  for (double value : net)
  {
    magnitude = std::max(magnitude, std::abs(value));
  }
  return magnitude;
}

/// The Euclidean distance between two points of an N-dimensional entity.
template <std::size_t N>
double Distance(const std::array<double, N>& a, const std::array<double, N>& b)
{
  double total = 0.;
  for (std::size_t axis = 0; axis < N; ++axis)
  {
    const double d = a[axis] - b[axis];
    total += d * d;
  }
  return std::sqrt(total);
}

/// Step A6's bookkeeping: record \a root among \a located unless one
/// already there sits within \a merge of it, since a single root routinely
/// leaves several root subdomains behind. Returns false once \a cap points
/// have been recorded, which the caller takes as "this root set is not
/// isolated" and stops rather than enumerating it.
template <std::size_t N>
bool AppendRoot(std::vector<std::array<double, N>>& located, const std::array<double, N>& root,
  double merge, int cap)
{
  for (const auto& prior : located)
  {
    if (Distance(prior, root) < merge)
    {
      return true;
    }
  }
  located.push_back(root);
  return static_cast<int>(located.size()) < cap;
}

/// Whether every point of a subdomain already lies within \a merge of a
/// root in \a located, given the subdomain's \a corners - the vertices of
/// its convex hull, which is where the distance to any outside point is
/// greatest.
///
/// A subdomain like that has already been answered. Whatever root it holds,
/// AppendRoot() would find a prior root within \a merge of it and drop it,
/// so subdividing further cannot add a point - which makes discarding it
/// exact rather than a heuristic, and not the same thing as pruning a
/// subdomain that provably holds no root at all.
///
/// It is worth testing for because the alternative is paying for it: two
/// roots closer together than a subdivision can separate at the first
/// attempt leave a cloud of subdomains around them, each of which the
/// search would otherwise refine all the way down to its root tolerance,
/// four orders of magnitude below the distance at which the answers merge.
/// Measured on a hexahedral net that behaves this way, one such pair drew
/// 604 accepted subdomains and 99% of the search's entire subdivision
/// budget, which it exhausted before reaching four other roots elsewhere in
/// the cell; with this test the same net costs 6 accepted subdomains and
/// finds all six roots.
template <std::size_t N, std::size_t Count>
bool AlreadyLocated(const std::vector<std::array<double, N>>& located,
  const std::array<std::array<double, N>, Count>& corners, double merge)
{
  for (const auto& prior : located)
  {
    bool covered = true;
    for (std::size_t corner = 0; corner < Count && covered; ++corner)
    {
      covered = Distance(prior, corners[corner]) < merge;
    }
    if (covered)
    {
      return true;
    }
  }
  return false;
}

/// AlreadyLocated() for an axis-aligned box, whose farthest corner from a
/// point is found per axis rather than enumerated.
template <std::size_t N>
bool AlreadyLocatedInBox(const std::vector<std::array<double, N>>& located,
  const std::array<double, N>& lo, const std::array<double, N>& hi, double merge)
{
  for (const auto& prior : located)
  {
    double farthest = 0.;
    for (std::size_t axis = 0; axis < N; ++axis)
    {
      const double centre = 0.5 * (lo[axis] + hi[axis]);
      const double reach = std::abs(prior[axis] - centre) + 0.5 * (hi[axis] - lo[axis]);
      farthest += reach * reach;
    }
    if (farthest < merge * merge)
    {
      return true;
    }
  }
  return false;
}

/// The length of the longest edge of a Count-vertex simplex, with the pair
/// of vertices spanning it in \a from and \a to - what step A9 bisects.
template <std::size_t N, std::size_t Count>
double LongestEdge(const std::array<std::array<double, N>, Count>& vertices, int& from, int& to)
{
  double longest = 0.;
  from = 0;
  to = 1;
  for (std::size_t i = 0; i < Count; ++i)
  {
    for (std::size_t j = i + 1; j < Count; ++j)
    {
      const double length = Distance(vertices[i], vertices[j]);
      if (length > longest)
      {
        longest = length;
        from = static_cast<int>(i);
        to = static_cast<int>(j);
      }
    }
  }
  return longest;
}

/// True when every edge of \a shrunk is shorter than \a ratio times the
/// corresponding edge of \a parent - step A8's test, for a simplex factor.
template <std::size_t N, std::size_t Count>
bool EdgesShrankBy(const std::array<std::array<double, N>, Count>& shrunk,
  const std::array<std::array<double, N>, Count>& parent, double ratio)
{
  for (std::size_t i = 0; i < Count; ++i)
  {
    for (std::size_t j = i + 1; j < Count; ++j)
    {
      if (Distance(shrunk[i], shrunk[j]) >= ratio * Distance(parent[i], parent[j]))
      {
        return false;
      }
    }
  }
  return true;
}

/// Step A5 for a simplex factor: turn the per-coordinate lower bounds \a
/// lower into the sub-simplex they define (Eqs. 19 and 21), as the
/// barycentric rows \a local of its vertices and as those vertices \a
/// shrunk in the entity's own coordinates, given the current simplex's own
/// \a vertices. Returns false when the bounds sum past one, which leaves no
/// point satisfying all of them at once - another way of having proved
/// there is no root here.
template <std::size_t N, std::size_t Count>
bool ShrinkSimplex(const std::array<double, Count>& lower,
  const std::array<std::array<double, N>, Count>& vertices,
  std::array<std::array<double, Count>, Count>& local,
  std::array<std::array<double, N>, Count>& shrunk)
{
  double total = 0.;
  for (std::size_t j = 0; j < Count; ++j)
  {
    total += lower[j];
  }
  if (total > 1.)
  {
    return false;
  }
  for (std::size_t i = 0; i < Count; ++i)
  {
    for (std::size_t j = 0; j < Count; ++j)
    {
      local[i][j] = lower[j];
    }
    local[i][i] = 1. - (total - lower[i]);
    for (std::size_t axis = 0; axis < N; ++axis)
    {
      double value = 0.;
      for (std::size_t j = 0; j < Count; ++j)
      {
        value += local[i][j] * vertices[j][axis];
      }
      shrunk[i][axis] = value;
    }
  }
  return true;
}

/// Whether moving vertex \a k to the barycentric point \a q would leave the
/// net exactly as it is - that is, whether \a q is already vertex k.
///
/// A5 shrinks by per-coordinate lower bounds, and a bound that came back
/// zero moves nothing; when every other coordinate's did too, the whole
/// blossom sweep for that vertex is an identity map computed the expensive
/// way. Skipping it is not an approximation: a de Casteljau step toward a
/// unit vector selects one coefficient per output with the other terms
/// multiplied by zero, so the sweep reproduces its input to the bit.
template <std::size_t Count>
bool MovesNothing(const std::array<double, Count>& q, int k)
{
  for (std::size_t j = 0; j < Count; ++j)
  {
    const double wanted = (static_cast<int>(j) == k) ? 1. : 0.;
    if (q[j] != wanted)
    {
      return false;
    }
  }
  return true;
}

/// Lemma 7's update of the sub-simplex's own coordinates once its vertex
/// \a k has been placed (Eqs. 25/26: x'[i][k] = x[i][k] / x[k][k],
/// x'[i][j] = x[i][j] - x'[i][k] * x[k][j]), so that the vertices not yet
/// placed are expressed in the simplex the placement just produced. Step A7
/// interleaves this with one blossom subdivision per vertex; only the
/// subdivision differs between a plain simplex and a wedge's triangular
/// factor, so only this half is shared.
template <std::size_t Count>
void ReexpressUpdate(std::array<std::array<double, Count>, Count>& local, int k)
{
  for (std::size_t i = 0; i < Count; ++i)
  {
    if (static_cast<int>(i) == k)
    {
      continue;
    }
    const double ratio = local[i][k] / local[k][k];
    for (std::size_t j = 0; j < Count; ++j)
    {
      if (static_cast<int>(j) != k)
      {
        local[i][j] -= ratio * local[k][j];
      }
    }
    local[i][k] = ratio;
  }
}

// -----------------------------------------------------------------------
// Tensor-product (box) domain
// -----------------------------------------------------------------------

/// The lexicographic stride of each axis in a rectangular control-net
/// lattice of the given per-axis \a order, r-axis fastest - the same
/// numbering vtkDGLagrangePoints uses for arbitrary-order degrees of
/// freedom. Used for a whole cell's own (up to 3-D, so a fixed array with
/// only its first order.size() entries meaningful) stride, computed once per
/// query; see the std::array<int, K> overload below for the per-subdivision-
/// node hot path.
std::array<int, 3> ComputeStride(const std::vector<int>& order)
{
  std::array<int, 3> stride{ { 0, 0, 0 } };
  stride[0] = 1;
  for (std::size_t axis = 1; axis < order.size(); ++axis)
  {
    stride[axis] = stride[axis - 1] * (order[axis - 1] + 1);
  }
  return stride;
}

/// Same as the std::vector overload above, but for a K-dimensional entity
/// (K = 1, 2, or 3 - fixed at compile time by whichever SearchEntity<K>()
/// dispatch is above it), avoiding a heap allocation on every one of the
/// (potentially very many) subdivision nodes that call it.
template <std::size_t K>
std::array<int, K> ComputeStride(const std::array<int, K>& order)
{
  std::array<int, K> stride;
  stride[0] = 1;
  for (std::size_t axis = 1; axis < K; ++axis)
  {
    stride[axis] = stride[axis - 1] * (order[axis - 1] + 1);
  }
  return stride;
}

/// Return true if \a axis varies over the entity described by \a freeAxes
/// (a K-dimensional entity's own free-axis list, K known at compile time
/// by whichever SearchEntity<K>() dispatch is calling this - see its own
/// comment), as opposed to being pinned to one end of its range.
template <std::size_t K>
bool IsFreeAxis(const std::array<int, K>& freeAxes, int axis)
{
  return std::find(freeAxes.begin(), freeAxes.end(), axis) != freeAxes.end();
}

/// The number of points in a K-dimensional rectangular control-net lattice
/// of the given per-axis \a order.
template <std::size_t K>
int PrismaticSize(const std::array<int, K>& order)
{
  int total = 1;
  for (std::size_t axis = 0; axis < K; ++axis)
  {
    total *= order[axis] + 1;
  }
  return total;
}

/// Visit every index of such a lattice, first axis fastest, as
/// visit(index, flatIndex) - the tensor-product counterpart of
/// ForEachSimplexIndex().
template <std::size_t K, typename Functor>
void ForEachPrismaticIndex(const std::array<int, K>& order, Functor&& visit)
{
  std::array<int, K> index{};
  const int total = PrismaticSize(order);
  for (int flat = 0; flat < total; ++flat)
  {
    visit(index, flat);
    for (std::size_t axis = 0; axis < K; ++axis)
    {
      if (++index[axis] <= order[axis])
      {
        break;
      }
      index[axis] = 0;
    }
  }
}

/// Copy out just one entity's own (k-dimensional) slice of the full cell's
/// control net: the sub-array visited when \a freeAxes vary and the
/// remaining axes are pinned to \a fixedIndex. This is a plain slice, not
/// an interpolation - a face or edge's control points already exist as
/// exact entries of the parent cell's coefficient array; only subdividing
/// *within* the extracted slice (SplitAxis()) needs to interpolate.
///
/// Written into \a result, which the caller reuses from entity to entity.
template <std::size_t K>
void ExtractEntityCoefficients(const std::vector<int>& order, const std::array<int, 3>& stride,
  const std::vector<double>& coefficients, const std::array<int, K>& freeAxes,
  const std::array<int, 3>& fixedIndex, std::vector<double>& result)
{
  const int dimension = static_cast<int>(order.size());
  std::array<int, 3> fullIndex{};
  for (int axis = 0; axis < dimension; ++axis)
  {
    if (!IsFreeAxis(freeAxes, axis))
    {
      fullIndex[axis] = fixedIndex[axis];
    }
  }

  int entitySize = 1;
  for (std::size_t fi = 0; fi < K; ++fi)
  {
    entitySize *= order[freeAxes[fi]] + 1;
  }
  result.clear();
  result.reserve(entitySize);
  std::array<int, K> freeIndex{};
  bool done = false;
  while (!done)
  {
    for (std::size_t fi = 0; fi < K; ++fi)
    {
      fullIndex[freeAxes[fi]] = freeIndex[fi];
    }
    int flat = 0;
    for (int axis = 0; axis < dimension; ++axis)
    {
      flat += fullIndex[axis] * stride[axis];
    }
    result.push_back(coefficients[flat]);

    std::size_t fi = 0;
    for (; fi < K; ++fi)
    {
      if (++freeIndex[fi] <= order[freeAxes[fi]])
      {
        break;
      }
      freeIndex[fi] = 0;
    }
    done = (fi == K);
  }
}

/// De Casteljau-split a 1-D run of Bezier control-point values at parameter
/// \a t in [0, 1] (the fraction of the way across the curve's own domain,
/// not a real (r, s, t) coordinate), into the control points of the left
/// ([0, t]) and right ([t, 1]) pieces - each the same size as the run,
/// since subdivision keeps the polynomial degree fixed and only changes the
/// control values. At t = 0, left collapses to a point and right is the run
/// unchanged; at t = 1, the reverse.
///
/// The run to split arrives in \a right and the right piece is left there:
/// that buffer is also the de Casteljau scratch, holding step r's whole row
/// in its first n - r + 1 entries while every entry past them is already
/// one of the right piece's own final control points, so the triangle is
/// computed in place with no per-level allocation. Taking the run that way
/// rather than copying it in is worth asking callers to gather straight
/// into \a right: this is the hot inner loop of every subdivision in this
/// file - the solvers below run it once per axis per equation per node - so
/// one copy of every run of every net is not a small thing, and \a left and
/// \a right are reused buffers rather than fresh ones for the same reason.
void SplitAt(double t, std::vector<double>& right, std::vector<double>& left)
{
  const int n = static_cast<int>(right.size()) - 1;
  left.resize(n + 1);
  left[0] = right[0];
  for (int r = 1; r <= n; ++r)
  {
    for (int i = 0; i <= n - r; ++i)
    {
      right[i] = (1. - t) * right[i] + t * right[i + 1];
    }
    left[r] = right[0];
  }
}

/// De Casteljau-bisect a 1-D run of Bezier control-point values at its
/// midpoint - SplitAt() at t = 0.5.
void Bisect1D(std::vector<double>& right, std::vector<double>& left)
{
  SplitAt(0.5, right, left);
}

/// Apply Bisect1D() along one axis (\a splitAxis) of a k-dimensional
/// control net whose per-axis sizes are \a freeOrder, independently for
/// every combination of the other axes' indices, producing the two
/// control nets of the left and right halves along that axis.
template <std::size_t K>
void SplitAxis(const std::array<int, K>& freeOrder, const std::vector<double>& coeffs,
  int splitAxis, Scratch& scratch, std::vector<double>& left, std::vector<double>& right)
{
  const std::array<int, K> freeStride = ComputeStride(freeOrder);
  const int total = freeStride[K - 1] * (freeOrder[K - 1] + 1);
  // Sized, not cleared: the scatter below writes every entry, so a recycled
  // buffer that is already big enough costs nothing here at all.
  left.resize(total);
  right.resize(total);

  const int n = freeOrder[splitAxis];
  std::array<int, K - 1> otherAxes;
  int m = 0;
  for (int axis = 0; axis < static_cast<int>(K); ++axis)
  {
    if (axis != splitAxis)
    {
      otherAxes[m++] = axis;
    }
  }

  // Each run is gathered straight into the buffer SplitAt() splits in
  // place, so nothing is copied between the two.
  std::vector<double>& leftLine = scratch.Left;
  std::vector<double>& rightLine = scratch.Line;
  rightLine.resize(n + 1);
  std::array<int, K> index{};
  std::array<int, K - 1> otherIndex{};
  bool done = false;
  while (!done)
  {
    for (int oi = 0; oi < m; ++oi)
    {
      index[otherAxes[oi]] = otherIndex[oi];
    }
    for (int ii = 0; ii <= n; ++ii)
    {
      index[splitAxis] = ii;
      int flat = 0;
      for (int axis = 0; axis < static_cast<int>(K); ++axis)
      {
        flat += index[axis] * freeStride[axis];
      }
      rightLine[ii] = coeffs[flat];
    }

    Bisect1D(rightLine, leftLine);

    for (int ii = 0; ii <= n; ++ii)
    {
      index[splitAxis] = ii;
      int flat = 0;
      for (int axis = 0; axis < static_cast<int>(K); ++axis)
      {
        flat += index[axis] * freeStride[axis];
      }
      left[flat] = leftLine[ii];
      right[flat] = rightLine[ii];
    }

    if (m == 0)
    {
      break;
    }
    int oi = 0;
    for (; oi < m; ++oi)
    {
      if (++otherIndex[oi] <= freeOrder[otherAxes[oi]])
      {
        break;
      }
      otherIndex[oi] = 0;
    }
    done = (oi == m);
  }
}

/// Restrict a 1-D Bezier net to [\a a, \a b] of its own [0, 1] domain, in
/// place: one de Casteljau split at \a b keeping the left piece, then one
/// at \a a rescaled by the first, keeping the right. \a discard is
/// caller-owned scratch, reused across the many lines of one net, and holds
/// whichever piece was not kept when this returns.
///
/// Neither split copies: SplitAt() leaves the right piece in the buffer it
/// was handed, so the second split needs no move at all and the first needs
/// only a swap of two vectors.
void ClipRange(std::vector<double>& line, double a, double b, std::vector<double>& discard)
{
  if (b < 1.)
  {
    SplitAt(b, line, discard);
    line.swap(discard); // Keep the left piece.
  }
  if (a > 0.)
  {
    SplitAt(b > 0. ? a / b : 0., line, discard); // Keeping the right piece is free.
  }
}

/// Apply ClipRange() along one axis of a K-dimensional net, independently
/// for every combination of the other axes' indices - the box counterpart
/// of the simplex's SimplexMoveVertex(), and what step A7 below uses to
/// re-express a system over a smaller box.
template <std::size_t K>
void ClipAxis(const std::array<int, K>& order, std::vector<double>& coeffs, int axis, double a,
  double b, Scratch& scratch)
{
  const std::array<int, K> stride = ComputeStride(order);
  const int n = order[axis];
  // One line per combination of the *other* axes' indices: pinning this
  // axis to 0 in the lattice being walked enumerates exactly their starts.
  std::array<int, K> lineOrder = order;
  lineOrder[axis] = 0;
  std::vector<double>& line = scratch.Line;
  std::vector<double>& discard = scratch.Left;
  line.resize(n + 1);
  ForEachPrismaticIndex<K>(lineOrder,
    [&](const std::array<int, K>& index, int)
    {
      int flat = 0;
      for (std::size_t a2 = 0; a2 < K; ++a2)
      {
        flat += index[a2] * stride[a2];
      }
      for (int ii = 0; ii <= n; ++ii)
      {
        line[ii] = coeffs[flat + ii * stride[axis]];
      }
      ClipRange(line, a, b, discard);
      for (int ii = 0; ii <= n; ++ii)
      {
        coeffs[flat + ii * stride[axis]] = line[ii];
      }
    });
}

/// The net of the field's derivative along one parametric axis, one degree
/// lower along that axis and unchanged along the others - the tensor
/// counterpart of SimplexDerivative() - written into \a result, which the
/// caller takes from its Scratch's Pool.
template <std::size_t K>
void PrismaticDerivative(const std::array<int, K>& order, const std::vector<double>& coefficients,
  int axis, std::array<int, K>& derivativeOrder, std::vector<double>& result)
{
  derivativeOrder = order;
  derivativeOrder[axis] = order[axis] - 1;
  const std::array<int, K> stride = ComputeStride(order);
  const int n = order[axis];
  result.resize(PrismaticSize(derivativeOrder)); // Every entry is written below.
  ForEachPrismaticIndex<K>(derivativeOrder,
    [&](const std::array<int, K>& index, int flat)
    {
      int base = 0;
      for (std::size_t a = 0; a < K; ++a)
      {
        base += index[a] * stride[a];
      }
      result[flat] = n * (coefficients[base + stride[axis]] - coefficients[base]);
    });
}

/// The span of x in [0, 1] over which the lower convex hull of the points
/// (t / (size - 1), \a values[t]) stays at or below zero, as [\a lo, \a
/// hi]. Returns false when that hull is strictly positive throughout, i.e.
/// when it never reaches the horizontal axis.
///
/// Callers get the "upper hull stays at or above zero" span by negating
/// their values: the upper hull of a sequence is the negated lower hull of
/// its negation. Shared by both domains' solvers, which differ in how they
/// project a control net onto one coordinate but not in what they then ask
/// of the result.
/// \a lowest must be the smallest of \a values, which every caller has
/// already passed over to subtract its band and so has in hand. It decides
/// the answer outright when it is positive: a convex hull's lowest vertex is
/// the lowest of the points it was built from, so a sequence with nothing at
/// or below the axis has a hull with nothing at or below it either.
bool LowerHullZeroSpan(
  const std::vector<double>& values, double lowest, std::vector<int>& hull, double& lo, double& hi)
{
  const int last = static_cast<int>(values.size()) - 1;
  if (last < 0)
  {
    return false;
  }
  if (!(lowest <= 0.))
  {
    return false; // Convex, so its minimum is at a vertex: positive throughout.
  }
  if (last == 0)
  {
    lo = 0.;
    hi = 1.;
    return true;
  }

  // Andrew's monotone chain over points that are already sorted by x. The
  // 1/last scaling every x difference shares cannot change the sign of the
  // cross product, so the integer abscissa is used directly.
  //
  // The chain is at most one entry per point, so \a hull is grown to that
  // once and its depth tracked separately: this runs twice per coordinate
  // per equation per node, and at that rate a push_back's capacity check
  // costs more than the geometry does.
  if (hull.size() < static_cast<std::size_t>(last) + 1)
  {
    hull.resize(last + 1);
  }
  int count = 0;
  for (int t = 0; t <= last; ++t)
  {
    while (count >= 2)
    {
      const int i = hull[count - 2];
      const int j = hull[count - 1];
      const double cross = static_cast<double>(j - i) * (values[t] - values[j]) -
        (values[j] - values[i]) * static_cast<double>(t - j);
      if (cross > 0.)
      {
        break;
      }
      --count;
    }
    hull[count++] = t;
  }

  // Some hull vertex is at or below the axis: the lowest of \a values is,
  // and every lowest point is a vertex of the lower hull. That is what the
  // early rejection above already established, so the walks below always
  // find their crossing.
  const int top = count - 1;
  const double scale = 1. / last;
  // The hull is convex, so walking in from each end finds that end's
  // crossing: the first vertex at or below zero, and the segment into it.
  lo = 0.;
  if (values[hull[0]] > 0.)
  {
    for (int p = 1; p <= top; ++p)
    {
      const double above = values[hull[p - 1]];
      const double below = values[hull[p]];
      if (below <= 0.)
      {
        lo = (hull[p - 1] + (hull[p] - hull[p - 1]) * (above / (above - below))) * scale;
        break;
      }
    }
  }
  hi = 1.;
  if (values[hull[top]] > 0.)
  {
    for (int p = top - 1; p >= 0; --p)
    {
      const double above = values[hull[p + 1]];
      const double below = values[hull[p]];
      if (below <= 0.)
      {
        hi = (hull[p + 1] + (hull[p] - hull[p + 1]) * (above / (above - below))) * scale;
        break;
      }
    }
  }
  return lo <= hi;
}

/// The span of one coordinate, as [\a lo, \a hi] in [0, 1], over which a
/// polynomial bounded below by the Bezier curve scratch.Smallest[0] and
/// above by scratch.Largest[0] can vanish. Returns false when it cannot vanish
/// anywhere, which proves the domain those two curves came from holds no
/// root.
///
/// The hull of both curves' control points together is bounded below by the
/// lower hull of scratch.Smallest[0]'s and above by the upper hull of
/// scratch.Largest[0]'s: no point of the latter can fall below the former,
/// since largest >= smallest layer by layer. So the axis crossing is exactly
/// where both of those admit zero. Each caller fills those two buffers with
/// its own projection of a control net and this consumes them in place.
///
/// \a band widens the axis the hulls are tested against from the line y = 0
/// to the strip |y| <= band. Sect. 5.1.1 implements the whole solver in
/// rounded interval arithmetic because "the method will frequently fail when
/// implemented with standard floating point operations", and this strip is
/// the cheap one-sided stand-in for that: it matters at a tangential root,
/// where a bounding curve should touch the axis exactly and instead misses
/// it by rounding noise, turning a correct "a root could be anywhere in
/// here" into an incorrect "there is no root here". Widening can only ever
/// admit more candidates, never discard a real root.
///
/// This is step A4, shared by all three solvers. They differ only in how
/// they project a net onto one coordinate to get these two curves - which
/// layers the net's coefficients fall into - not in what they then ask of
/// the result.
bool HullZeroSpan(std::vector<double>& smallest, std::vector<double>& largest,
  std::vector<int>& hull, double band, double& lo, double& hi)
{
  // Each curve is shifted onto the band and its smallest entry noted in the
  // same pass, since LowerHullZeroSpan() needs that minimum to decide
  // whether a hull is worth building at all.
  double lowerLowest = std::numeric_limits<double>::infinity();
  for (double& value : smallest)
  {
    value -= band;
    lowerLowest = std::min(lowerLowest, value);
  }
  double lowerLo;
  double lowerHi;
  if (!LowerHullZeroSpan(smallest, lowerLowest, hull, lowerLo, lowerHi))
  {
    return false;
  }
  double upperLowest = std::numeric_limits<double>::infinity();
  for (double& value : largest)
  {
    value = -(value + band);
    upperLowest = std::min(upperLowest, value);
  }
  double upperLo;
  double upperHi;
  if (!LowerHullZeroSpan(largest, upperLowest, hull, upperLo, upperHi))
  {
    return false;
  }
  lo = std::max(lowerLo, upperLo);
  hi = std::min(lowerHi, upperHi);
  return lo <= hi;
}

/// HullZeroSpan() over the single pair of curves a caller that projects one
/// coordinate at a time leaves in scratch.Smallest[0] / scratch.Largest[0].
bool HullZeroSpan(Scratch& scratch, double band, double& lo, double& hi)
{
  return HullZeroSpan(scratch.Smallest[0], scratch.Largest[0], scratch.Hull, band, lo, hi);
}

/// The range [\a lo, \a hi], in this box's own [0, 1] parametrization of
/// \a axis, that any root of \a coeffs must have its \a axis coordinate
/// in. Returns false when no such range exists, which proves this box
/// holds no root.
///
/// Taking the smallest and largest coefficient over each layer (the
/// control points sharing one index along \a axis) gives two univariate
/// Bezier curves that bound the polynomial from below and above along that
/// coordinate; the answer is where the convex hull of their control points
/// crosses the horizontal axis (HullZeroSpan(), which is also what \a band
/// is for). This is exactly the reduction Sect. 5.3.2 (page 390) describes
/// for the general n-dimensional-to-2-D projection of Sect. 5.2's Theorem
/// 5.4: "at most two of [the points sharing one abscissa] can belong to the
/// boundary of the convex hull (specifically, the points with the minimum
/// and maximum ordinates)", so only those two per layer are worth keeping.
/// This is the box counterpart of SimplexAllLayerBounds().
template <std::size_t K>
bool PrismaticRootInterval(const std::array<int, K>& order, const std::vector<double>& coeffs,
  int axis, double band, Scratch& scratch, double& lo, double& hi)
{
  const int n = order[axis];
  std::vector<double>& smallest = scratch.Smallest[0];
  std::vector<double>& largest = scratch.Largest[0];
  smallest.assign(n + 1, std::numeric_limits<double>::infinity());
  largest.assign(n + 1, -std::numeric_limits<double>::infinity());

  // Layer-major rather than a walk of the lattice: one layer along this
  // axis is the runs of stride[axis] consecutive entries repeating every
  // stride[axis] * (n + 1), so the whole projection is three counted loops
  // over contiguous memory with no per-coefficient index arithmetic at all.
  // This is the box solver's steps A3 and A4, run once per axis per
  // equation per node, and the walk it replaces was among the larger costs
  // in the profile.
  const std::array<int, K> stride = ComputeStride(order);
  const int step = stride[axis];
  const int block = step * (n + 1);
  const int total = PrismaticSize(order);
  const double* net = coeffs.data();
  for (int base = 0; base < total; base += block)
  {
    for (int layer = 0; layer <= n; ++layer)
    {
      const double* run = net + base + layer * step;
      double lowest = smallest[layer];
      double highest = largest[layer];
      for (int i = 0; i < step; ++i)
      {
        lowest = std::min(lowest, run[i]);
        highest = std::max(highest, run[i]);
      }
      smallest[layer] = lowest;
      largest[layer] = highest;
    }
  }
  return HullZeroSpan(scratch, band, lo, hi);
}

/// How small a sub-box's longest side must get before it is accepted as
/// holding a root (the paper's EPS, step A6), in the entity's own [-1, 1]
/// parameters. This is the accuracy of the reported critical point, not a
/// starting guess for something else, so it is set where the method stops
/// paying: over the randomized sweep FindPrismaticInteriorPoints()
/// describes, going from 1e-4 to 1e-10 cost a factor of two in iterations
/// (a mean of 51 to 105) and bought five orders of magnitude of accuracy
/// (a worst error of 1.4e-4 to 5.2e-9, with no root lost at either
/// setting).
constexpr double PrismaticRootTolerance = 1e-10;

/// How far inside each face FindPrismaticInteriorPoints() searches, in the
/// same [-1, 1] parameters.
constexpr double PrismaticInterior = 1e-4;

/// Below this ratio of a sub-box's side to its parent's, step A8 calls the
/// shrink fast enough to recurse on; above it, step A9 bisects instead.
///
/// Sherbrooke & Patrikalakis's own experiments (Sect. 5.3, page 389)
/// suggest CRIT = 0.8, or 0.85-0.9 "for higher-dimensional problems since
/// the boxes tend to decrease in size more slowly anyway".
constexpr double PrismaticShrinkRatio = 0.8;

/// The width of PrismaticRootInterval()'s zero band, relative to each
/// equation's own largest coefficient over the sub-box being tested.
constexpr double PrismaticZeroBand = 2e-16;

/// Backstops on step A2, so that a field whose critical points are not
/// isolated - a whole curve or patch of them, which no finite set of
/// points can represent anyway - costs bounded work rather than unbounded.
constexpr int MaxPrismaticIterations = 20000;
constexpr int MaxPrismaticPoints = 256;

/// How close two root boxes must be to be treated as the same critical
/// point. One root routinely leaves several behind; the widest spread
/// measured across the randomized sweep was 1.1e-8, and this has to sit
/// above that and below the 1e-4 separation AppendIfNew() already refuses
/// to distinguish.
constexpr double PrismaticPointMerge = 1e-6;

/// The box solver behind FindPrismaticInteriorPoints(): every root of the K
/// equations \a system - each a tensor net over the same [-1, 1]^K, at its
/// own per-axis degrees \a orders - strictly inside that box.
///
/// Split out because the equations need not be a field's plain partial
/// derivatives. The pyramid's are not: see FindPyramidInteriorPoints(),
/// which also builds its equations in the same \a scratch it then solves
/// them with.
///
/// \a system's nets must come from scratch.Pool, and go back to it whatever
/// the outcome: the search's first node takes them over rather than
/// copying them.
template <std::size_t K>
std::vector<std::array<double, K>> SolvePrismaticSystem(
  const std::array<std::array<int, K>, K>& orders, std::array<std::vector<double>, K>& system,
  Scratch& scratch)
{
  NetPool& pool = scratch.Pool;
  std::vector<std::array<double, K>> located;
  double steepest = 0.;
  double flattest = std::numeric_limits<double>::infinity();
  for (std::size_t k = 0; k < K; ++k)
  {
    const double magnitude = MaxAbs(system[k]);
    steepest = std::max(steepest, magnitude);
    flattest = std::min(flattest, magnitude);
  }
  // An equation that vanishes identically says the field does not vary
  // along that axis at all, so none of its critical points is isolated:
  // each extends along a whole line, and the boundary entities that line
  // ends on report it at their own dimension. This is not a corner case
  // here - a face of a cell whose only feature is in its interior extracts
  // to a constant net, whose every equation is identically zero - so it is
  // answered up front rather than subdivided at.
  if (flattest <= 1e-12 * std::max(steepest, 1.))
  {
    pool.Recycle(system);
    return located;
  }

  // The problem stack, of sub-boxes and the system re-expressed over each.
  // The nets of nodes already dealt with go back to the pool, for the
  // children of later ones. Every node that is pruned, accepted, or bisected
  // leaves K nets behind, and every bisection wants 2K fresh ones the same
  // size, so without it the search allocates and frees its way down the
  // tree.
  using Node = BoxNode<K>;
  std::vector<Node>& stack = scratch.Stack<Node>();

  // A1, over the inset box rather than the whole of it.
  {
    Node start;
    start.System = std::move(system);
    for (std::size_t axis = 0; axis < K; ++axis)
    {
      start.Region.Lo[axis] = -1. + PrismaticInterior;
      start.Region.Hi[axis] = 1. - PrismaticInterior;
    }
    for (std::size_t k = 0; k < K; ++k)
    {
      for (std::size_t axis = 0; axis < K; ++axis)
      {
        ClipAxis<K>(orders[k], start.System[k], static_cast<int>(axis), 0.5 * PrismaticInterior,
          1. - 0.5 * PrismaticInterior, scratch);
      }
    }
    stack.push_back(std::move(start));
  }

  int iterations = 0;
  while (!stack.empty()) // A2
  {
    if (++iterations > MaxPrismaticIterations)
    {
      vtkLogF(WARNING,
        "Critical-point search gave up on a %zu-dimensional tensor-product entity after %d "
        "subdivisions; its critical points are probably not isolated, and what is reported "
        "here samples them rather than listing them.",
        K, MaxPrismaticIterations);
      break;
    }
    Node node = std::move(stack.back());
    stack.pop_back();

    std::array<double, K> band;
    for (std::size_t k = 0; k < K; ++k)
    {
      band[k] = PrismaticZeroBand * MaxAbs(node.System[k]);
    }

    // A3 and A4: per axis, intersect what every equation allows.
    std::array<double, K> lo;
    std::array<double, K> hi;
    bool feasible = true;
    for (std::size_t axis = 0; axis < K && feasible; ++axis)
    {
      lo[axis] = 0.;
      hi[axis] = 1.;
      for (std::size_t k = 0; k < K; ++k)
      {
        double equationLo;
        double equationHi;
        if (!PrismaticRootInterval<K>(orders[k], node.System[k], static_cast<int>(axis), band[k],
              scratch, equationLo, equationHi))
        {
          feasible = false;
          break;
        }
        lo[axis] = std::max(lo[axis], equationLo);
        hi[axis] = std::min(hi[axis], equationHi);
        if (lo[axis] > hi[axis])
        {
          feasible = false;
          break;
        }
      }
    }
    if (!feasible)
    {
      pool.Recycle(node.System);
      continue;
    }

    // A5: those bounds define the smaller box.
    std::array<double, K> shrunkLo;
    std::array<double, K> shrunkHi;
    double widest = 0.;
    int widestAxis = 0;
    for (std::size_t axis = 0; axis < K; ++axis)
    {
      const double width = node.Region.Hi[axis] - node.Region.Lo[axis];
      shrunkLo[axis] = node.Region.Lo[axis] + lo[axis] * width;
      shrunkHi[axis] = node.Region.Lo[axis] + hi[axis] * width;
      if (shrunkHi[axis] - shrunkLo[axis] > widest)
      {
        widest = shrunkHi[axis] - shrunkLo[axis];
        widestAxis = static_cast<int>(axis);
      }
    }

    // Already answered: see AlreadyLocated(). Tested before A6 rather than
    // after because the point is to stop paying for this box, and the
    // width test below is the cheap half of it.
    if (widest < PrismaticPointMerge &&
      AlreadyLocatedInBox<K>(located, shrunkLo, shrunkHi, PrismaticPointMerge))
    {
      pool.Recycle(node.System);
      continue;
    }

    // A6: small enough to call a root. One root can end up split across
    // several root boxes, so they are merged as they are found.
    if (widest < PrismaticRootTolerance)
    {
      std::array<double, K> root;
      for (std::size_t axis = 0; axis < K; ++axis)
      {
        root[axis] = 0.5 * (shrunkLo[axis] + shrunkHi[axis]);
      }
      const bool room = AppendRoot(located, root, PrismaticPointMerge, MaxPrismaticPoints);
      pool.Recycle(node.System);
      if (!room)
      {
        break; // Not an isolated root set; stop rather than enumerate it.
      }
      continue;
    }

    // A7. The node's own system is not read again past this point, so it
    // is re-expressed in place rather than copied.
    auto reduced = std::move(node.System);
    for (std::size_t k = 0; k < K; ++k)
    {
      for (std::size_t axis = 0; axis < K; ++axis)
      {
        if (lo[axis] > 0. || hi[axis] < 1.)
        {
          ClipAxis<K>(orders[k], reduced[k], static_cast<int>(axis), lo[axis], hi[axis], scratch);
        }
      }
    }

    // A8: shrinking fast enough to simply recurse on the result.
    bool shrinkingFast = true;
    for (std::size_t axis = 0; axis < K; ++axis)
    {
      if (shrunkHi[axis] - shrunkLo[axis] >=
        PrismaticShrinkRatio * (node.Region.Hi[axis] - node.Region.Lo[axis]))
      {
        shrinkingFast = false;
        break;
      }
    }
    if (shrinkingFast)
    {
      stack.push_back(Node{ { shrunkLo, shrunkHi }, std::move(reduced) });
      continue;
    }

    // A9: it is not, so bisect the widest side. Both children come out of
    // the one de Casteljau sweep SplitAxis() does.
    const double middle = 0.5 * (shrunkLo[widestAxis] + shrunkHi[widestAxis]);
    Node lower;
    Node upper;
    lower.Region.Lo = shrunkLo;
    lower.Region.Hi = shrunkHi;
    lower.Region.Hi[widestAxis] = middle;
    upper.Region.Lo = shrunkLo;
    upper.Region.Hi = shrunkHi;
    upper.Region.Lo[widestAxis] = middle;
    for (std::size_t k = 0; k < K; ++k)
    {
      lower.System[k] = pool.Take();
      upper.System[k] = pool.Take();
      SplitAxis(orders[k], reduced[k], widestAxis, scratch, lower.System[k], upper.System[k]);
      pool.Recycle(reduced[k]);
    }
    stack.push_back(std::move(lower));
    stack.push_back(std::move(upper));
  }
  // A search stopped at one of its caps leaves nodes behind; their nets go
  // back to the pool rather than out with the stack.
  for (auto& node : stack)
  {
    pool.Recycle(node.System);
  }
  stack.clear();
  return located;
}

/// A tensor-product entity's strictly-interior critical points, in its own
/// [-1, 1]^K parameters: every point where the field's gradient vanishes,
/// located to PrismaticRootTolerance by the box-domain Bernstein Projected
/// Polyhedron algorithm, run over the entity's own control net \a
/// coefficients at per-axis degrees \a order.
///
/// These are answers, not starting guesses - nothing refines them
/// afterwards. The algorithm is the same A1-A9 skeleton
/// FindBarycentricInteriorPoints() follows, which is where the fuller
/// commentary lives; a box needs neither that one's blossom subdivision
/// nor its Lemma 7 coordinate updates, since restricting a tensor net to a
/// sub-box is just one independent de Casteljau clip per axis (ClipAxis()).
/// The system is the K partial derivatives, each one degree lower along
/// its own axis.
///
/// Two departures from a literal reading, both shared with the barycentric
/// solver and for the same reasons given there: the search runs over the
/// entity inset by PrismaticInterior rather than its closure, and the
/// hull-versus-axis test is widened to a narrow band around zero in place
/// of rounded interval arithmetic.
///
/// The band differs from the barycentric one in a way the box domain
/// forces: it is scaled by each equation's magnitude over the *sub-box
/// being tested*, not over the starting one. A box, unlike a simplex, lets
/// every coordinate be small at once, and near such a corner these fields
/// decay to a fixed tiny fraction of their global size - so a band fixed
/// at the start is never small enough to prune there, and the search
/// subdivides without end. That is measured, not assumed: with a globally
/// scaled band, 23 of the degenerate control nets below ran to the
/// iteration cap; with a locally scaled one, none did.
///
/// Validated before being trusted, the same two ways the barycentric
/// solver was: against the system of Reuter et al.'s own Sect. 6.2 worked
/// example posed over a box (the bounding box of their triangle), which it
/// solves to the root they publish; and against an independent multi-start
/// Newton solver over 1750 random control nets at K = 1, 2 and 3 and
/// orders 2 through 5, which found 3494 roots between them with none
/// missed and none added. That sweep, plus the degenerate nets
/// Testing/Cxx/TestBezierFindCriticalPoints.cxx builds, is also what set
/// the band: with no band at all, three of those degenerate cases lost
/// their only critical point outright - exactly the floating-point failure
/// Sect. 5.1.1 describes.
template <std::size_t K>
std::vector<std::array<double, K>> FindPrismaticInteriorPoints(
  const std::array<int, K>& order, const std::vector<double>& coefficients)
{
  for (std::size_t axis = 0; axis < K; ++axis)
  {
    if (order[axis] < 1)
    {
      // Constant along this axis: see the identically-zero case in
      // SolvePrismaticSystem().
      return {};
    }
  }

  // grad(field) = 0, as one partial derivative per axis. Unlike a simplex,
  // order 1 is not degenerate here: a multilinear field is not affine, and
  // r * s has an honest saddle no single-axis test can find.
  Scratch& scratch = AcquireScratch();
  std::array<std::array<int, K>, K> orders;
  std::array<std::vector<double>, K> system;
  for (std::size_t axis = 0; axis < K; ++axis)
  {
    system[axis] = scratch.Pool.Take();
    PrismaticDerivative<K>(order, coefficients, static_cast<int>(axis), orders[axis], system[axis]);
  }
  return SolvePrismaticSystem<K>(orders, system, scratch);
}

/// If \a rst is not already present (within tolerance) among \a points,
/// evaluate the field there and append a new CriticalPoint with the given
/// \a entityDimension. Shared by every entry point below, whatever the
/// shape, since "is this a new critical point" and "record it" never depend
/// on how \a rst was reached.
void AppendIfNew(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, int dimension,
  const std::vector<double>& coefficients, const std::array<double, 3>& rst, int entityDimension,
  std::vector<CriticalPoint>& points)
{
  constexpr double tol2 = 1e-8;
  for (const auto& prior : points)
  {
    double d2 = 0.;
    for (int axis = 0; axis < dimension; ++axis)
    {
      const double d = prior.Parameter[axis] - rst[axis];
      d2 += d * d;
    }
    if (d2 < tol2)
    {
      return;
    }
  }

  std::array<double, 3> gradient;
  const double value = EvaluateField(basisOp, gradOp, dimension, coefficients, rst, gradient);
  CriticalPoint criticalPoint;
  criticalPoint.Parameter = rst;
  criticalPoint.Value = value;
  criticalPoint.EntityDimension = entityDimension;
  points.push_back(criticalPoint);
}

/// Record each of one tensor-product entity's own critical points, mapping
/// it from that entity's [-1, 1]^K parameters back into the cell's (r, s,
/// t) by filling in the axes \a fixedIndex pins.
///
/// There is nothing to refine: FindPrismaticInteriorPoints() reports roots
/// of grad(field) = 0 rather than starting guesses for an optimizer. That
/// is also what lets this report saddle points, since a descent method
/// walks away from a saddle and this does not.
template <std::size_t K>
void AppendPrismaticInterior(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, int dimension,
  const std::vector<double>& coefficients, const std::array<int, K>& freeAxes,
  const std::array<int, 3>& fixedIndex, const std::vector<std::array<double, K>>& located,
  std::vector<CriticalPoint>& points)
{
  for (const auto& local : located)
  {
    std::array<double, 3> rst{ { 0., 0., 0. } };
    for (int axis = 0; axis < dimension; ++axis)
    {
      if (!IsFreeAxis(freeAxes, axis))
      {
        rst[axis] = fixedIndex[axis] == 0 ? -1. : 1.;
      }
    }
    for (std::size_t i = 0; i < K; ++i)
    {
      rst[freeAxes[i]] = local[i];
    }
    AppendIfNew(basisOp, gradOp, dimension, coefficients, rst, K, points);
  }
}

/// How many entity codes EncodeEntity() can return: 3 states per axis, at
/// most 3 axes.
constexpr int EntityCodeCount = 27;

/// A base-3 code (free / fixed-low / fixed-high per axis) identifying one
/// cell entity, so that an entity shared by more than one parent - every
/// edge of a hexahedron borders 2 of its faces - is only ever searched once.
template <std::size_t K>
int EncodeEntity(
  int dimension, const std::array<int, K>& freeAxes, const std::array<int, 3>& fixedIndex)
{
  int code = 0;
  int mult = 1;
  for (int axis = 0; axis < dimension; ++axis)
  {
    int state = 0;
    if (!IsFreeAxis(freeAxes, axis))
    {
      state = fixedIndex[axis] == 0 ? 1 : 2;
    }
    code += state * mult;
    mult *= 3;
  }
  return code;
}

template <std::size_t K>
void SearchEntity(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp,
  const std::vector<int>& order, const std::array<int, 3>& stride,
  const std::vector<double>& coefficients, int dimension, const std::array<int, K>& freeAxes,
  const std::array<int, 3>& fixedIndex, std::array<bool, EntityCodeCount>& visited,
  std::vector<double>& entityNet, std::vector<CriticalPoint>& points);

/// Recurse onto each entity one dimension lower than \a freeAxes that
/// bounds it - fixing one more (previously free) axis to either end of its
/// range - searching each with SearchEntity(), which in turn owns that
/// entity's own further recursion. \a visited prevents an entity shared by
/// more than one parent - every edge of a hexahedron borders two of its
/// faces - from being searched twice; the caller supplies it so the same
/// array can be shared across every top-level facet.
///
/// This is SearchEntity()'s own boundary step, factored out so
/// FindPrismaticBoundaryCriticalPoints() can start the same recursion one
/// level down without also repeating the parent's own interior search.
///
/// K is a template parameter for the same reason SearchEntity()'s is: the
/// `if constexpr (K >= 2)` below skips generating a K = 0 (cell corner)
/// instantiation, which is never needed since corners are never reported.
template <std::size_t K>
void SearchBoundaryEntities(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp,
  const std::vector<int>& order, const std::array<int, 3>& stride,
  const std::vector<double>& coefficients, int dimension, const std::array<int, K>& freeAxes,
  const std::array<int, 3>& fixedIndex, std::array<bool, EntityCodeCount>& visited,
  std::vector<double>& entityNet, std::vector<CriticalPoint>& points)
{
  if constexpr (K >= 2)
  {
    for (std::size_t fi = 0; fi < K; ++fi)
    {
      const int axisToFix = freeAxes[fi];
      std::array<int, K - 1> childFree;
      std::size_t cj = 0;
      for (std::size_t fj = 0; fj < K; ++fj)
      {
        if (fj != fi)
        {
          childFree[cj++] = freeAxes[fj];
        }
      }
      for (int boundaryValue : { 0, order[axisToFix] })
      {
        auto childFixed = fixedIndex;
        childFixed[axisToFix] = boundaryValue;
        SearchEntity(basisOp, gradOp, order, stride, coefficients, dimension, childFree, childFixed,
          visited, entityNet, points);
      }
    }
  }
}

/// Locate the critical points on one cell entity (the volume, a face, or an
/// edge - identified by which axes in \a freeAxes vary versus are
/// pinned to a boundary in \a fixedIndex), record any found in \a points,
/// then recurse onto each entity one dimension lower that bounds it via
/// SearchBoundaryEntities(). \a visited prevents an entity shared by more
/// than one parent - every edge of a hexahedron borders two of its faces -
/// from being searched twice.
///
/// K (the entity's own free-axis count) is a template parameter, not a
/// runtime size, so every function this calls can use std::array instead
/// of a heap-allocating std::vector for anything sized by it. This is a
/// clean fit for recursion specifically because SearchEntity() only ever
/// *shrinks* K by exactly 1 per level (never grows it, never branches into
/// more than one K at a time): SearchBoundaryEntities()'s
/// `if constexpr (K >= 2)` skips generating the recursive call - and so
/// never needs a K = 0 instantiation to exist - for K = 1, whose children
/// are single points (K = 0, i.e. cell corners), which are never reported.
template <std::size_t K>
void SearchEntity(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp,
  const std::vector<int>& order, const std::array<int, 3>& stride,
  const std::vector<double>& coefficients, int dimension, const std::array<int, K>& freeAxes,
  const std::array<int, 3>& fixedIndex, std::array<bool, EntityCodeCount>& visited,
  std::vector<double>& entityNet, std::vector<CriticalPoint>& points)
{
  bool& seen = visited[EncodeEntity(dimension, freeAxes, fixedIndex)];
  if (seen)
  {
    return; // Already searched via another parent entity.
  }
  seen = true;

  std::array<int, K> freeOrder{};
  for (std::size_t fi = 0; fi < K; ++fi)
  {
    freeOrder[fi] = order[freeAxes[fi]];
  }
  // \a entityNet is free again once this entity's search returns, so each
  // entity below reuses it.
  ExtractEntityCoefficients(order, stride, coefficients, freeAxes, fixedIndex, entityNet);
  const auto located = FindPrismaticInteriorPoints<K>(freeOrder, entityNet);
  AppendPrismaticInterior(
    basisOp, gradOp, dimension, coefficients, freeAxes, fixedIndex, located, points);

  SearchBoundaryEntities(basisOp, gradOp, order, stride, coefficients, dimension, freeAxes,
    fixedIndex, visited, entityNet, points);
}

/// The number of points in a d-dimensional simplex (barycentric) lattice
/// of order n - a triangle's own (d=2) or a tetrahedron's (d=3) control
/// net.
///
/// One point per multi-index summing to at most n, which is C(n + d, d).
/// vtkMath::Binomial() reads that off a table of Pascal's triangle at every
/// order this file will see, and answers 0 for a negative order.
int SimplexSize(int d, int n)
{
  return static_cast<int>(vtkMath::Binomial(n + d, d));
}

/// The flat index of a D-dimensional simplex lattice point \a indices -
/// indices[i] >= 0, sum(indices) <= n - in the same lexicographic order
/// (last index outermost, first index innermost) vtkDGLagrangePoints uses
/// for a triangle's or tetrahedron's arbitrary-order degrees of freedom; for
/// D = 1 it is the plain 1-D index.
///
/// Peels the last index and recurses on the rest at a reduced order: the
/// lattice points the peeled index skips past - sum(m < last) of the
/// (D - 1)-dimensional layer at order n - m each - are counted directly,
/// one term at a time. That is the plain definition rather than a fast one,
/// which is all it needs to be: only SimplexSources() and BlossomSources()
/// call it, while building tables kept once per (D, n) (or (D, n, k)).
template <std::size_t D>
int SimplexIndex(const std::array<int, D>& indices, int n)
{
  if constexpr (D == 1)
  {
    return indices[0];
  }
  else
  {
    const int last = indices[D - 1];
    int offset = 0;
    for (int m = 0; m < last; ++m)
    {
      offset += SimplexSize(static_cast<int>(D) - 1, n - m);
    }
    std::array<int, D - 1> rest;
    std::copy_n(indices.begin(), D - 1, rest.begin());
    return offset + SimplexIndex<D - 1>(rest, n - last);
  }
}

/// Extract the SimplexSize(2, n)-sized 2-D triangular sub-array of a
/// tetrahedron's control net for the face on which barycentric
/// coordinate \a pinned (0 for lambda_0, the implicit i+j+k=n
/// coordinate; 1, 2, or 3 for lambda_1 = r, lambda_2 = s, or lambda_3 = t
/// respectively) is pinned to 0. Local indices (a, b) - b outer, a inner,
/// matching SimplexIndex()'s own convention - are placed into the
/// tetrahedron's (i, j, k) according to which coordinate is pinned; this
/// placement must (and, by construction below, does) agree with
/// whichever embedding FindBarycentricCriticalPoints() pairs it with, so
/// that a point found on the face lands at its own (r, s, t). Written into
/// \a result, which the caller reuses from face to face.
void ExtractTetrahedronFace(
  int n, const std::vector<double>& coefficients, int pinned, std::vector<double>& result)
{
  result.clear();
  result.reserve(SimplexSize(2, n));
  for (int b = 0; b <= n; ++b)
  {
    for (int a = 0; a <= n - b; ++a)
    {
      std::array<int, 3> indices{};
      switch (pinned)
      {
        case 0:
          indices = { { a, b, n - a - b } }; // i + j + k = n
          break;
        case 1:
          indices = { { 0, a, b } }; // i = 0
          break;
        case 2:
          indices = { { a, 0, b } }; // j = 0
          break;
        default:
          indices = { { a, b, 0 } }; // k = 0 (pinned == 3)
          break;
      }
      result.push_back(coefficients[SimplexIndex<3>(indices, n)]);
    }
  }
}

/// One of the three edges of a triangular lattice, as the two things every
/// use of it needs: which control points lie on it, and where its own
/// parameter lands in (r, s).
///
/// Shared because the same three edges are searched from three places - a
/// plain triangle's own edges, a wedge cap's, and (with the prismatic axis
/// left free rather than pinned) the quadrilateral side face each one
/// carries, which is also why ExtractWedgeSideFace() selects a face by the
/// edge it stands on.
struct TriangleEdge
{
  /// 0: j = 0, free i. 1: i = 0, free j. 2: i + j = n, the hypotenuse.
  int Case;

  /// The (i, j) lattice index of the control point at local index \a p
  /// along this edge of an order-\a n triangle.
  std::array<int, 2> Index(int p, int n) const
  {
    switch (this->Case)
    {
      case 0:
        return { { p, 0 } };
      case 1:
        return { { 0, p } };
      default:
        return { { n - p, p } };
    }
  }

  /// The (r, s) that this edge's own parameter \a u in [-1, 1] maps to.
  std::array<double, 2> Parameter(double u) const
  {
    const double t = (u + 1.) / 2.;
    switch (this->Case)
    {
      case 0:
        return { { t, 0. } };
      case 1:
        return { { 0., t } };
      default:
        return { { 1. - t, t } };
    }
  }
};
constexpr std::array<TriangleEdge, 3> TriangleEdges{ { { 0 }, { 1 }, { 2 } } };

/// Maps a barycentric entity's own \a dLocal free coordinates (in the
/// first \a dLocal slots; any trailing ones are unused padding, so this
/// can be shared across dLocal = 2 and 3 without a heap-allocating
/// std::vector) to the cell's real (r, s, t) point.
using BarycentricEmbed = std::function<std::array<double, 3>(const std::array<double, 3>&)>;

// -----------------------------------------------------------------------
// Barycentric interval projected polyhedron
// -----------------------------------------------------------------------
// A simplex control net cannot use the box solver above at all: its
// lattice is not a rectangular product, and subdividing it means moving a
// vertex rather than clipping independent axes. The rest of this section
// is that machinery, following
//
//   Reuter, Mikkelsen, Sherbrooke, Maekawa & Patrikalakis, "Solving
//   nonlinear polynomial systems in the barycentric Bernstein basis",
//   The Visual Computer 24 (2008) 187-200,
//
// the simplex-domain generalization of the Interval Projected Polyhedron
// algorithm of Sherbrooke & Patrikalakis (CAGD 10 (1993) 379-405) that
// FindPrismaticCriticalPoints()'s own comment cites. Step names (A1-A9),
// theorem and lemma numbers below are all that paper's.
//
// The system solved is grad(field) = 0, written barycentrically as the
// directional derivative along every pair of the simplex's D+1 barycentric
// coordinates. Only D of those D(D+1)/2 equations are independent, but
// redundant ones only help: they discard root-free sub-simplices sooner
// (the paper makes the same observation in its Sect. 6.4 example).
//
// Two deliberate departures from the paper are marked at their use below:
// the zero band HullZeroSpan() tests against (standing in for the paper's
// rounded interval arithmetic) and FindBarycentricInteriorPoints()'s inset
// (which keeps this an interior search). Both were arrived at by measurement,
// not by taste; see their comments.

/// Visit every point of an order-\a n D-dimensional simplex lattice in
/// exactly the order SimplexIndex() numbers them, calling
/// visit(index, flatIndex, implicitIndex). The flat index comes along so
/// callers never recompute it, and \a implicitIndex is n - sum(index), the
/// barycentric coordinate SimplexIndex()'s numbering leaves implicit.
template <std::size_t D, typename Functor>
void ForEachSimplexIndex(int n, Functor&& visit)
{
  std::array<int, D> index{};
  int flat = 0;
  int sum = 0;
  while (true)
  {
    visit(index, flat, n - sum);
    ++flat;
    std::size_t axis = 0;
    for (; axis < D; ++axis)
    {
      ++index[axis];
      ++sum;
      if (sum <= n)
      {
        break;
      }
      sum -= index[axis];
      index[axis] = 0;
    }
    if (axis == D)
    {
      break;
    }
  }
}

/// The flat indices one de Casteljau step down from order \a n reads: for
/// each point of the order n-1 lattice, in that lattice's own order, the
/// D+1 places in an order-\a n net holding it and its neighbors one step
/// along each axis, laid out D+1 at a time.
///
/// They depend only on the lattice, never on the coefficients, so they are
/// built once per (D, \a n) and reused by every step of every node. That is
/// worth doing rather than recomputing: the alternative is D+1 calls to
/// SimplexIndex() per output coefficient, which is what the subdivision
/// spent most of its time on.
template <std::size_t D>
const std::vector<int>& SimplexSources(int n, Scratch& scratch)
{
  std::vector<std::vector<int>>& byOrder = scratch.Sources[D];
  if (byOrder.size() <= static_cast<std::size_t>(n))
  {
    byOrder.resize(n + 1);
  }
  std::vector<int>& sources = byOrder[n];
  if (sources.empty())
  {
    sources.resize(SimplexSize(static_cast<int>(D), n - 1) * (D + 1));
    ForEachSimplexIndex<D>(n - 1,
      [&](const std::array<int, D>& index, int flat, int)
      {
        int* entry = sources.data() + flat * (D + 1);
        entry[0] = SimplexIndex<D>(index, n);
        for (std::size_t axis = 0; axis < D; ++axis)
        {
          std::array<int, D> neighbor = index;
          ++neighbor[axis];
          entry[axis + 1] = SimplexIndex<D>(neighbor, n);
        }
      });
  }
  return sources;
}

/// One de Casteljau step of \a coefficients toward the barycentric point
/// \a q: an order-\a n net in, an order n-1 net written into \a result.
///
/// How many coefficients that is - SimplexSize(D, n - 1) - is read off the
/// source table's own length rather than recomputed. The table is built at
/// exactly that size, and this runs once per level of every blossom sweep
/// of every node, which is too often for a call across a library boundary
/// to something a size already on hand answers.
template <std::size_t D>
void SimplexDeCasteljau(int n, const double* coefficients, const std::array<double, D + 1>& q,
  Scratch& scratch, std::vector<double>& result)
{
  const std::vector<int>& sources = SimplexSources<D>(n, scratch);
  const int count = static_cast<int>(sources.size() / (D + 1));
  result.resize(count);
  const int* entry = sources.data();
  for (int flat = 0; flat < count; ++flat, entry += D + 1)
  {
    double value = 0.;
    for (std::size_t axis = 0; axis <= D; ++axis)
    {
      value += q[axis] * coefficients[entry[axis]];
    }
    result[flat] = value;
  }
}

/// Where SimplexMoveVertex() reads each coefficient of its result from,
/// given the order \a n it works at and the vertex \a k it moves: for each
/// point of the order-\a n lattice, in that lattice's own order, which
/// level of the de Casteljau ladder holds it and where in that level's own
/// net, laid out as that pair.
///
/// Cached for the same reason SimplexSources() is, and keyed alongside it:
/// the pair depends only on the lattice and the vertex being moved, never
/// on the coefficients, while the gather itself runs once per equation per
/// vertex per node.
template <std::size_t D>
const std::vector<int>& BlossomSources(int n, int k, Scratch& scratch)
{
  std::vector<std::vector<int>>& byCase = scratch.Blossom[D];
  const std::size_t entry = static_cast<std::size_t>(n) * (D + 1) + k;
  if (byCase.size() <= entry)
  {
    byCase.resize(entry + 1);
  }
  std::vector<int>& sources = byCase[entry];
  if (sources.empty())
  {
    sources.resize(SimplexSize(static_cast<int>(D), n) * 2);
    ForEachSimplexIndex<D>(n,
      [&](const std::array<int, D>& index, int flat, int implicitIndex)
      {
        // Vertex 0's repeat count is the coordinate SimplexIndex()'s
        // numbering leaves implicit; every other vertex's is an index.
        int repeats = implicitIndex;
        std::array<int, D> zeroed = index;
        if (k > 0)
        {
          repeats = index[k - 1];
          zeroed[k - 1] = 0;
        }
        sources[flat * 2] = repeats;
        sources[flat * 2 + 1] = SimplexIndex<D>(zeroed, n - repeats);
      });
  }
  return sources;
}

/// The same polynomial's net over the simplex whose vertex \a k has been
/// moved to the barycentric point \a q, leaving every other vertex alone.
///
/// By the blossom (polar form) of a Bezier simplex, the new coefficient at
/// multi-index I is the blossom with vertex k's argument repeated i_k
/// times at \a q - that is, the i_k-th de Casteljau net read at I with its
/// k-th component zeroed. So one sweep of de Casteljau nets, orders n-1
/// down to 0, answers the whole net at once (Sects. 5.1.3-5.1.4).
///
/// Sect. 5.1.4 attributes this scheme to Goldman, who "represented the de
/// Casteljau algorithm by an n+1 simplex... after the first full de
/// Casteljau computation, he restarts it a second and final time along one
/// of its edges (with adjusted coordinates)": the ladder below is that
/// first computation, run once per vertex rather than once per output
/// coefficient. The paper is explicit that this is not the same as
/// evaluating the blossom directly - "straightforward blossoming, where
/// every coefficient is computed by a full de Casteljau subdivision, is
/// very slow, since many identical values are computed over and over" - so
/// "blossom" above names the identity this sweep computes, not the
/// algorithm it uses to compute it.
///
/// That sweep is the subdivision every node of the search runs, once per
/// equation per vertex, so it is written into \a result and over
/// scratch.Nets rather than allocating either. \a result must not be \a
/// coefficients.
template <std::size_t D>
void SimplexMoveVertex(int n, const double* coefficients, int k, const std::array<double, D + 1>& q,
  Scratch& scratch, double* result)
{
  // Level 0 of the ladder is \a coefficients itself. Reading it in place
  // rather than copying it in saves a whole net's worth of memory traffic
  // per sweep, and the gather below stays branch-free by addressing the
  // ladder through a level-to-net table whose first entry points at the
  // caller's own array.
  std::vector<std::vector<double>>& nets = scratch.Nets;
  if (nets.size() < static_cast<std::size_t>(n) + 1)
  {
    nets.resize(n + 1);
  }
  std::vector<const double*>& levels = scratch.Levels;
  levels.resize(n + 1);
  levels[0] = coefficients;
  for (int level = 1; level <= n; ++level)
  {
    SimplexDeCasteljau<D>(n - level + 1, levels[level - 1], q, scratch, nets[level]);
    levels[level] = nets[level].data();
  }

  // As in SimplexDeCasteljau(), the count comes from the table's own length
  // rather than from a fresh SimplexSize() call: the table holds one (level,
  // offset) pair per coefficient.
  const std::vector<int>& sources = BlossomSources<D>(n, k, scratch);
  const int count = static_cast<int>(sources.size() / 2);
  const int* source = sources.data();
  for (int flat = 0; flat < count; ++flat, source += 2)
  {
    result[flat] = levels[source[0]][source[1]];
  }
}

/// SimplexMoveVertex() over a net held in a std::vector, which is what every
/// caller outside a wedge has: the result is sized here rather than by the
/// caller.
template <std::size_t D>
void SimplexMoveVertex(int n, const std::vector<double>& coefficients, int k,
  const std::array<double, D + 1>& q, Scratch& scratch, std::vector<double>& result)
{
  result.resize(BlossomSources<D>(n, k, scratch).size() / 2);
  SimplexMoveVertex<D>(n, coefficients.data(), k, q, scratch, result.data());
}

/// Re-express every net in \a system over the sub-simplex whose vertices
/// are \a local in the current simplex's own barycentric coordinates: one
/// full de Casteljau subdivision per vertex (SimplexMoveVertex(), which see
/// for the paper's own attribution of this scheme to Goldman), with the
/// vertices not yet placed having their coordinates corrected in between by
/// ReexpressUpdate(). This is step A7. Returns false if the sub-simplex is
/// degenerate, which leaves \a system unusable and the caller must discard
/// it.
template <std::size_t D, std::size_t E>
bool SimplexReexpress(int n, std::array<std::vector<double>, E>& system,
  std::array<std::array<double, D + 1>, D + 1> local, Scratch& scratch)
{
  const int count = static_cast<int>(D) + 1;
  for (int k = 0; k < count; ++k)
  {
    if (!(std::abs(local[k][k]) > 0.))
    {
      return false;
    }
    const std::array<double, D + 1> q = local[k];
    if (!MovesNothing(q, k))
    {
      for (auto& net : system)
      {
        SimplexMoveVertex<D>(n, net, k, q, scratch, scratch.Net);
        net.swap(scratch.Net);
      }
    }
    ReexpressUpdate(local, k);
  }
  return true;
}

/// Project a simplex control net onto every barycentric coordinate at once:
/// for each coordinate j, the smallest and largest coefficient of each layer
/// i_j = t, which Definition 5 and Theorem 1 read as two univariate order-\a n
/// Bezier curves bounding the polynomial from below and above along that
/// coordinate. Written into scratch.Smallest[j] / scratch.Largest[j]
/// for j = 0 .. D, for HullZeroSpan() to consume one pair at a time.
///
/// Which layer a coefficient falls into differs per coordinate, but every
/// one of those layer numbers is already in hand at each point of the
/// lattice walk - coordinate j > 0 is index[j - 1] and coordinate 0 is the
/// implicit index - so all D + 1 projections can be accumulated as the net
/// is read rather than re-reading it once per coordinate. That is the whole
/// point: this runs once per equation per node with D + 1 coordinates to
/// answer, and reading the net D + 1 times was among the largest costs in
/// the profile.
///
/// Each layer's running min and max see their coefficients in the same
/// order a separate sweep per coordinate would, so the curves are identical
/// to the last bit, not merely equivalent.
template <std::size_t D>
void SimplexAllLayerBounds(int n, const std::vector<double>& net, Scratch& scratch)
{
  constexpr std::size_t count = D + 1;
  std::array<double*, count> smallest;
  std::array<double*, count> largest;
  for (std::size_t j = 0; j < count; ++j)
  {
    scratch.Smallest[j].assign(n + 1, std::numeric_limits<double>::infinity());
    scratch.Largest[j].assign(n + 1, -std::numeric_limits<double>::infinity());
    smallest[j] = scratch.Smallest[j].data();
    largest[j] = scratch.Largest[j].data();
  }
  // Through raw pointers taken once rather than the vectors themselves: the
  // walk cannot be shown not to alias them, so addressing each layer through
  // its container would reload the container's own data pointer per
  // coefficient per coordinate.
  const double* values = net.data();
  ForEachSimplexIndex<D>(n,
    [&](const std::array<int, D>& index, int flat, int implicitIndex)
    {
      const double value = values[flat];
      for (std::size_t j = 0; j < count; ++j)
      {
        const int layer = (j == 0) ? implicitIndex : index[j - 1];
        smallest[j][layer] = std::min(smallest[j][layer], value);
        largest[j][layer] = std::max(largest[j][layer], value);
      }
    });
}

/// The order n-1 net of the field's derivative along the direction from
/// barycentric coordinate \a b toward \a a - the barycentric counterpart
/// of the forward difference the tensor-product path takes along an axis.
///
/// Written into \a result, SimplexSize(D, n - 1) entries of it. Taking both
/// nets as pointers lets a wedge run this on each triangle slice of its own
/// net in place, rather than copying every slice in and out.
template <std::size_t D>
void SimplexDerivative(int n, const double* coefficients, int a, int b, double* result)
{
  ForEachSimplexIndex<D>(n - 1,
    [&](const std::array<int, D>& index, int flat, int)
    {
      auto neighbor = [&](int coordinate)
      {
        std::array<int, D> shifted = index;
        if (coordinate > 0)
        {
          ++shifted[coordinate - 1];
        }
        return coefficients[SimplexIndex<D>(shifted, n)];
      };
      result[flat] = n * (neighbor(a) - neighbor(b));
    });
}

/// How small a sub-simplex's longest edge must get before it is accepted
/// as holding a root (the paper's EPS, step A6). This is the accuracy of
/// the reported critical point, not a starting guess for something else,
/// so it is set where the method stops paying: convergence is quadratic,
/// and over the randomized sweep FindBarycentricInteriorPoints() describes,
/// going from 1e-4 to 1e-10 cost a factor of two in iterations (a mean of
/// 38.8 to 76.4) and bought five orders of magnitude of accuracy (a worst
/// error of 5.3e-5 to 9.7e-10, with no root lost at either setting).
constexpr double BarycentricRootTolerance = 1e-10;

/// How far inside each face FindBarycentricInteriorPoints() searches.
constexpr double BarycentricInterior = 1e-4;

/// How many times the zero band the gradient must still be at the margin of
/// a face the gradient vanishes identically on, and how far in that margin
/// is ever allowed to move. See DegenerateFaceInset().
constexpr double BarycentricDegenerateClearance = 1e4;
constexpr double BarycentricDegenerateInsetCap = 0.1;

/// Below this ratio of a sub-simplex's edge to its parent's, step A8 calls
/// the shrink fast enough to recurse on; above it, step A9 bisects
/// instead. "Our experiments suggest that 0.7 is a reasonable value."
constexpr double BarycentricShrinkRatio = 0.7;

/// The width of the zero band the simplex solver's hull test is widened by,
/// relative to each equation's own largest coefficient.
constexpr double BarycentricZeroBand = 2e-16;

/// Backstops on step A2, so that a field whose critical points are not
/// isolated - a whole curve or patch of them, which no finite set of points
/// can represent anyway - costs bounded work rather than unbounded.
constexpr int MaxBarycentricIterations = 20000;
constexpr int MaxBarycentricPoints = 256;

/// How close two of those root simplices must be to be treated as the same
/// critical point. One root can leave more than one behind (the paper's
/// own Sect. 6.2 example ends with several), so they are merged here.
///
/// This is deliberately not tied to BarycentricRootTolerance. It only has
/// to sit above how far apart one root's simplices land - measured at
/// 9.7e-10 across the randomized sweep - and below the 1e-4 separation
/// AppendIfNew() already refuses to distinguish, which is the real limit
/// on how close two reported critical points can be.
constexpr double BarycentricPointMerge = 1e-6;

/// A barycentric entity's strictly-interior critical points: every point
/// where the field's gradient vanishes, located to
/// BarycentricRootTolerance by the barycentric Bernstein IPP solver
/// above, run over the entity's own control net.
///
/// These are answers, not starting guesses - nothing refines them
/// afterwards, for the reasons AppendBarycentricInterior() gives. \a n is
/// the polynomial order and \a coefficients the simplex control net; each
/// result is the (lambda_1 .. lambda_D) of one critical point.
///
/// The search domain is the simplex inset by BarycentricInterior on every
/// face, not the closed simplex. This is a rule about which entity owns a
/// critical point, not a statement about accuracy - the margin is far
/// wider than BarycentricRootTolerance, deliberately - and the caller
/// searches every face, edge and corner separately at its own dimension,
/// so nothing inside the margin goes unreported; it is reported there
/// instead.
///
/// The margin has to be that wide because of what sits exactly on those
/// faces. A field like 1 - c * prod(lambda_i ^ p_i) - which is what a
/// control net that is flat except at one coefficient gives - has a
/// gradient that vanishes identically on every face and edge where the
/// exponents of the vanishing coordinates sum to two or more. The closed
/// simplex therefore hands the solver a continuum of roots rather than
/// isolated ones, and Sect. 6.5 describes what the algorithm correctly
/// does with that: "we end up with many small simplices, covering the
/// solution point set", 1236 of them in the paper's own example. Insetting
/// puts those continua outside the domain, where the boundary searches
/// that own them can find them instead.
///
/// Validated before being trusted: against the paper's own Sect. 6.2
/// worked example (two order-5 polynomials over the triangle
/// [(0,0); (2,0); (0,2)], whose common root is reported there as
/// u1 = u2 = 0.726602621583884 to 0.726602621590015) and against an
/// independent multi-start Newton solver over 1635 random control nets at
/// D = 2 and 3 and orders 2 through 6, which found 1767 roots between them
/// with none missed. That sweep is also what set BarycentricZeroBand: with
/// no band at all, 9 of the first 236 of those roots were lost to exactly
/// the floating-point failure Sect. 5.1.1 describes.
/// Whether \a net vanishes identically on the face where barycentric
/// coordinate \a j is zero.
///
/// Exact rather than conservative: the coefficients whose \a j-th index is
/// zero are the control net of the restriction to that face, and a Bernstein
/// polynomial is identically zero exactly when its own coefficients are.
template <std::size_t D>
bool VanishesOnFace(int m, const std::vector<double>& net, int j, double tolerance)
{
  bool vanishes = true;
  ForEachSimplexIndex<D>(m,
    [&](const std::array<int, D>& index, int flat, int implicitIndex)
    {
      const int coordinate = (j == 0) ? implicitIndex : index[j - 1];
      if (coordinate == 0 && std::abs(net[flat]) > tolerance)
      {
        vanishes = false;
      }
    });
  return vanishes;
}

/// How far inside a face the search has to start when the gradient vanishes
/// identically on that face, which BarycentricInterior alone is not enough
/// for.
///
/// Such a face carries a continuum of critical points rather than isolated
/// ones. It is excluded from this search deliberately - the face is searched
/// separately, at its own dimension - but excluding it is not enough on its
/// own, because the gradient does not drop to zero at the face so much as
/// decay into it. A field whose net is flat except at one corner is
/// 1 - c * lambda_0^n, whose gradient falls off as lambda_0^(n-1); at the
/// ordinary 1e-4 margin that is 1e-12 at order 4 but 1e-16 at order 5, which
/// is the zero band itself. Past that order the solver cannot tell a sliver
/// beside a degenerate face from a sliver around a root, and subdivides it
/// until the iteration cap stops it - reporting, into the bargain, points
/// that sit on the margin rather than anywhere critical.
///
/// So the margin is set where the worst-case decay is still
/// BarycentricDegenerateClearance times the band, which is what inverting
/// lambda^(n-1) gives. n - 1 is the fastest any single coordinate's exponent
/// can decay, so this is the conservative choice among the exponents a
/// degenerate face can have. It is never tighter than BarycentricInterior
/// and never wider than BarycentricDegenerateInsetCap; past the order where
/// it would want to be wider than that, the iteration cap is once again the
/// backstop.
double DegenerateFaceInset(int n)
{
  if (n < 2)
  {
    return BarycentricInterior;
  }
  const double wanted =
    std::pow(BarycentricDegenerateClearance * BarycentricZeroBand, 1. / (n - 1));
  return std::min(std::max(wanted, BarycentricInterior), BarycentricDegenerateInsetCap);
}

template <std::size_t D>
std::vector<std::array<double, D>> FindBarycentricInteriorPoints(
  int n, const std::vector<double>& coefficients)
{
  constexpr int count = static_cast<int>(D) + 1;
  std::vector<std::array<double, D>> located;
  if (n < 2)
  {
    // A linear field's gradient is constant: either it vanishes nowhere,
    // or it vanishes everywhere and no one point is the critical one.
    return located;
  }

  // Reused across every node rather than reallocated at each: see Scratch.
  Scratch& scratch = AcquireScratch();
  NetPool& pool = scratch.Pool;

  // grad(field) = 0, as the derivative along every pair of coordinates.
  const int order = n - 1;
  constexpr std::size_t equationCount = D * (D + 1) / 2;
  std::array<std::vector<double>, equationCount> system;
  std::size_t equation = 0;
  for (int a = 0; a < count; ++a)
  {
    for (int b = a + 1; b < count; ++b)
    {
      std::vector<double>& net = system[equation++];
      net = pool.Take();
      net.resize(SimplexSize(static_cast<int>(D), order));
      SimplexDerivative<D>(n, coefficients.data(), a, b, net.data());
    }
  }

  // Every net the search derives is a convex combination of these
  // coefficients - de Casteljau toward a point inside the simplex weighs
  // by barycentric coordinates, which are non-negative and sum to one - so
  // each equation's magnitude here bounds it everywhere below, and is the
  // right yardstick for the zero band.
  std::array<double, equationCount> band;
  double steepest = 0.;
  for (std::size_t k = 0; k < equationCount; ++k)
  {
    const double magnitude = MaxAbs(system[k]);
    steepest = std::max(steepest, magnitude);
    band[k] = BarycentricZeroBand * (magnitude > 0. ? magnitude : 1.);
  }

  // A field that does not vary has no one critical point: every point of it
  // is equally critical, and the solver would faithfully say so by
  // subdividing until it ran out of iterations. This is not a corner case
  // here - a face of a cell whose only feature is in its interior extracts
  // to exactly such a net - so it is answered up front, with no points.
  if (steepest <= 1e-12 * std::max(MaxAbs(coefficients), 1.))
  {
    pool.Recycle(system);
    return located;
  }

  // The nets of nodes already dealt with go back to the pool, for the
  // children of later ones - the same free list SolvePrismaticSystem() uses,
  // and for the same reason: every node that is pruned, accepted, or
  // bisected leaves equationCount nets behind, and every bisection wants
  // 2 * equationCount fresh ones the same size.

  // How far in from each face to start. Ordinarily the same margin all
  // round, but a face the gradient vanishes identically on needs its own,
  // wider one - see DegenerateFaceInset().
  std::array<double, D + 1> inset;
  inset.fill(BarycentricInterior);
  for (int j = 0; j < count; ++j)
  {
    bool degenerate = true;
    for (std::size_t k = 0; k < equationCount && degenerate; ++k)
    {
      degenerate = VanishesOnFace<D>(order, system[k], j, 1e-12 * MaxAbs(system[k]));
    }
    if (degenerate)
    {
      inset[j] = DegenerateFaceInset(n);
    }
  }

  // A1, over the inset simplex {lambda_j >= inset[j]} rather than the whole
  // of it. The problem stack holds sub-simplices and the system re-expressed
  // over each.
  using Node = SimplexNode<D>;
  std::vector<Node>& stack = scratch.Stack<Node>();
  {
    Node start;
    std::array<std::array<double, D + 1>, D + 1> local;
    for (int i = 0; i < count; ++i)
    {
      double others = 0.;
      for (int j = 0; j < count; ++j)
      {
        local[i][j] = inset[j];
        if (j != i)
        {
          others += inset[j];
        }
      }
      local[i][i] = 1. - others;
      start.Region.Vertices[i].fill(0.);
      // Vertex 0 of the entity is the origin and vertex i > 0 is the unit
      // point along lambda_i, so a point's lambda_1..lambda_D are just its
      // barycentric coordinates 1..D.
      for (std::size_t axis = 0; axis < D; ++axis)
      {
        start.Region.Vertices[i][axis] = local[i][axis + 1];
      }
    }
    start.System = std::move(system);
    if (!SimplexReexpress<D>(order, start.System, local, scratch))
    {
      pool.Recycle(start.System);
      return located;
    }
    stack.push_back(std::move(start));
  }

  int iterations = 0;
  while (!stack.empty()) // A2
  {
    if (++iterations > MaxBarycentricIterations)
    {
      vtkLogF(WARNING,
        "Critical-point search gave up on an order-%d %zu-dimensional barycentric entity after "
        "%d subdivisions; its critical points are probably not isolated, and what is reported "
        "here samples them rather than listing them.",
        n, D, MaxBarycentricIterations);
      break;
    }
    Node node = std::move(stack.back());
    stack.pop_back();

    // A3 and A4: per coordinate, intersect what every equation allows.
    //
    // Equation-major, so that each equation's net is read once for all
    // D + 1 coordinates (SimplexAllLayerBounds()) rather than once per
    // coordinate. The bounds are intersected across equations either way,
    // so this only changes how much of a node's projection is done before
    // an infeasible one is abandoned, never the outcome.
    std::array<double, D + 1> lower;
    std::array<double, D + 1> upper;
    lower.fill(0.);
    upper.fill(1.);
    bool feasible = true;
    for (std::size_t k = 0; k < equationCount && feasible; ++k)
    {
      SimplexAllLayerBounds<D>(order, node.System[k], scratch);
      for (int j = 0; j < count; ++j)
      {
        double equationLo;
        double equationHi;
        if (!HullZeroSpan(scratch.Smallest[j], scratch.Largest[j], scratch.Hull, band[k],
              equationLo, equationHi))
        {
          feasible = false;
          break;
        }
        lower[j] = std::max(lower[j], equationLo);
        upper[j] = std::min(upper[j], equationHi);
        if (lower[j] > upper[j])
        {
          feasible = false;
          break;
        }
      }
    }
    if (!feasible)
    {
      pool.Recycle(node.System);
      continue;
    }

    // A5: those lower bounds define the smaller simplex.
    std::array<std::array<double, D + 1>, D + 1> local;
    std::array<std::array<double, D>, D + 1> shrunk;
    if (!ShrinkSimplex(lower, node.Region.Vertices, local, shrunk))
    {
      pool.Recycle(node.System);
      continue;
    }

    int splitFrom;
    int splitTo;
    const double longest = LongestEdge(shrunk, splitFrom, splitTo);

    // Already answered: see AlreadyLocated(). A sub-simplex is the convex
    // hull of its own vertices, so those are the corners to test.
    if (longest < BarycentricPointMerge &&
      AlreadyLocated<D, D + 1>(located, shrunk, BarycentricPointMerge))
    {
      pool.Recycle(node.System);
      continue;
    }

    // A6: small enough to call a root. One root can end up split across
    // several root simplices, so located are merged as they are found.
    if (longest < BarycentricRootTolerance)
    {
      std::array<double, D> root{};
      for (std::size_t axis = 0; axis < D; ++axis)
      {
        double value = 0.;
        for (int i = 0; i < count; ++i)
        {
          value += shrunk[i][axis];
        }
        root[axis] = value / count;
      }
      if (!AppendRoot(located, root, BarycentricPointMerge, MaxBarycentricPoints))
      {
        break; // Not an isolated root set; stop rather than enumerate it.
      }
      pool.Recycle(node.System);
      continue;
    }

    // A7. The node's own system is not read again past this point, so it
    // is re-expressed in place rather than copied.
    auto reduced = std::move(node.System);
    if (!SimplexReexpress<D>(order, reduced, local, scratch))
    {
      pool.Recycle(reduced);
      continue;
    }

    // A8: shrinking fast enough to simply recurse on the result.
    if (EdgesShrankBy(shrunk, node.Region.Vertices, BarycentricShrinkRatio))
    {
      stack.push_back(Node{ { shrunk }, std::move(reduced) });
      continue;
    }

    // A9: it is not, so bisect the longest edge. Both children share the
    // one de Casteljau sweep toward the midpoint.
    std::array<double, D + 1> midpoint{};
    midpoint[splitFrom] = 0.5;
    midpoint[splitTo] = 0.5;
    std::array<double, D> moved;
    for (std::size_t axis = 0; axis < D; ++axis)
    {
      moved[axis] = 0.5 * (shrunk[splitFrom][axis] + shrunk[splitTo][axis]);
    }
    for (int replaced : { splitTo, splitFrom })
    {
      Node child;
      child.Region.Vertices = shrunk;
      child.Region.Vertices[replaced] = moved;
      for (std::size_t k = 0; k < equationCount; ++k)
      {
        child.System[k] = pool.Take();
        SimplexMoveVertex<D>(order, reduced[k], replaced, midpoint, scratch, child.System[k]);
      }
      stack.push_back(std::move(child));
    }
    pool.Recycle(reduced);
  }
  // A search stopped at one of its caps leaves nodes behind; their nets go
  // back to the pool rather than out with the stack.
  for (auto& node : stack)
  {
    pool.Recycle(node.System);
  }
  stack.clear();
  return located;
}

/// Record a barycentric entity's critical points, mapping each from the
/// entity's own barycentric coordinates (lambda_1..lambda_dLocal, in that
/// order) to the cell's real (r, s, t) point with \a embed - a direct
/// assignment for a plain triangle or tetrahedron's interior, a
/// permuted-and-padded one for a tetrahedron's face or a wedge's cap.
/// \a entityDimension is recorded on each CriticalPoint found.
///
/// There is nothing to refine: FindBarycentricInteriorPoints() reports
/// roots to BarycentricRootTolerance directly. Nor should anything be: a
/// descent method walks away from a saddle, which the subdivision solver
/// finds as readily as an extremum - measured on a triangle whose only
/// interior critical point is a saddle, a descent pass over the solver's
/// output lost it outright once its starting points were coarser than
/// 1e-3.
template <std::size_t D>
void AppendBarycentricInterior(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp,
  int dimension, const std::vector<double>& coefficients, const BarycentricEmbed& embed,
  const std::vector<std::array<double, D>>& located, int entityDimension,
  std::vector<CriticalPoint>& points)
{
  for (const auto& local : located)
  {
    std::array<double, 3> lambda{};
    for (std::size_t i = 0; i < D; ++i)
    {
      lambda[i] = local[i];
    }
    AppendIfNew(basisOp, gradOp, dimension, coefficients, embed(lambda), entityDimension, points);
  }
}

/// Locate every critical point on one barycentric shape's edge, already
/// extracted as an ordinary n + 1-control-point 1-D Bezier curve, and
/// append each to \a points. \a embed maps that curve's own [-1, 1]
/// parameter to the cell's real (r, s, t) point.
///
/// An edge is a 1-D tensor-product entity once extracted - just not one
/// whose embedding fits the free-axis/fixed-axis convention SearchEntity()
/// assumes - so it uses FindPrismaticInteriorPoints() directly, at K = 1.
/// Nothing refines the result: see AppendPrismaticInterior().
void SearchBarycentricEdge(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, int dimension,
  int n, const std::vector<double>& edgeCoefficients, const std::vector<double>& fullCoefficients,
  const std::function<std::array<double, 3>(double)>& embed, std::vector<CriticalPoint>& points)
{
  const auto located = FindPrismaticInteriorPoints<1>({ { n } }, edgeCoefficients);
  for (const auto& local : located)
  {
    AppendIfNew(basisOp, gradOp, dimension, fullCoefficients, embed(local[0]), 1, points);
  }
}

/// The flat index of wedge lattice point (i, j, k) - (i, j) on the
/// triangular cross-section's simplex lattice (i, j >= 0, i + j <= n),
/// k along the independent prismatic axis (0 <= k <= m) - in the same
/// order (k outermost, then j, then i) vtkDGLagrangePoints uses for a
/// wedge's arbitrary-order degrees of freedom: a triangle's own lattice,
/// repeated once per point along the prismatic axis.
int WedgeIndex(int i, int j, int k, int n)
{
  return k * SimplexSize(2, n) + SimplexIndex<2>({ { i, j } }, n);
}

/// The pair of orders a wedge net carries: \a Triangle along the triangular
/// cross-section and \a Prismatic along the independent axis.
///
/// Both pure shapes' solvers can keep one order for the whole system; this
/// one cannot. Its four equations differ in which factor they differentiate,
/// so the three barycentric derivatives drop a degree on the triangle and
/// the prismatic one drops a degree along t, and every net has to carry its
/// own pair.
struct WedgeOrder
{
  int Triangle = 0;
  int Prismatic = 0;
  int Size() const { return SimplexSize(2, this->Triangle) * (this->Prismatic + 1); }
};

/// Move the triangle's vertex \a k to the barycentric point \a q, on every
/// prismatic slice - the wedge's SimplexMoveVertex(). WedgeIndex() groups the
/// lattice by k, so each slice is a plain contiguous sub-range and the
/// triangle's own subdivision applies to it unchanged.
void WedgeMoveVertex(const WedgeOrder& order, const std::vector<double>& net, int k,
  const std::array<double, 3>& q, Scratch& scratch, std::vector<double>& result)
{
  const int triangleSize = SimplexSize(2, order.Triangle);
  result.resize(net.size());
  // Each slice is already contiguous in \a net and wants to land contiguous
  // in \a result, so the triangle's own sweep reads and writes them in place
  // rather than staging each through a buffer and back.
  for (int p = 0; p <= order.Prismatic; ++p)
  {
    const int base = p * triangleSize;
    SimplexMoveVertex<2>(order.Triangle, net.data() + base, k, q, scratch, result.data() + base);
  }
}

/// Restrict every prismatic line of a wedge net to [\a a, \a b] of its own
/// [0, 1] - the wedge's ClipAxis(), along the one axis it has. The lines run
/// with a stride of one triangle, again because WedgeIndex() groups by k.
void WedgeClipPrismatic(
  const WedgeOrder& order, std::vector<double>& net, double a, double b, Scratch& scratch)
{
  const int triangleSize = SimplexSize(2, order.Triangle);
  std::vector<double>& line = scratch.Line;
  std::vector<double>& discard = scratch.Left;
  line.resize(order.Prismatic + 1);
  for (int p = 0; p < triangleSize; ++p)
  {
    for (int k = 0; k <= order.Prismatic; ++k)
    {
      line[k] = net[k * triangleSize + p];
    }
    ClipRange(line, a, b, discard);
    for (int k = 0; k <= order.Prismatic; ++k)
    {
      net[k * triangleSize + p] = line[k];
    }
  }
}

/// De Casteljau-bisect every prismatic line of a wedge net, producing the
/// nets of its lower and upper halves along that axis.
void WedgeSplitPrismatic(const WedgeOrder& order, const std::vector<double>& net, Scratch& scratch,
  std::vector<double>& lower, std::vector<double>& upper)
{
  const int triangleSize = SimplexSize(2, order.Triangle);
  // Sized, not cleared: the scatter below writes every entry of both, so a
  // recycled buffer of the right size costs nothing here.
  lower.resize(net.size());
  upper.resize(net.size());
  std::vector<double>& left = scratch.Left;
  std::vector<double>& right = scratch.Line;
  right.resize(order.Prismatic + 1);
  for (int p = 0; p < triangleSize; ++p)
  {
    for (int k = 0; k <= order.Prismatic; ++k)
    {
      right[k] = net[k * triangleSize + p];
    }
    Bisect1D(right, left);
    for (int k = 0; k <= order.Prismatic; ++k)
    {
      lower[k * triangleSize + p] = left[k];
      upper[k * triangleSize + p] = right[k];
    }
  }
}

/// The derivative along the triangle direction from barycentric coordinate
/// \a b toward \a a, slice by slice: SimplexDerivative() with t along for
/// the ride.
void WedgeTriangleDerivative(const WedgeOrder& order, const std::vector<double>& net, int a, int b,
  WedgeOrder& derivativeOrder, std::vector<double>& result)
{
  derivativeOrder = { order.Triangle - 1, order.Prismatic };
  const int triangleSize = SimplexSize(2, order.Triangle);
  const int reducedSize = SimplexSize(2, order.Triangle - 1);
  result.resize(derivativeOrder.Size()); // Every entry is written below.
  for (int k = 0; k <= order.Prismatic; ++k)
  {
    SimplexDerivative<2>(
      order.Triangle, net.data() + k * triangleSize, a, b, result.data() + k * reducedSize);
  }
}

/// The derivative along the prismatic axis: the forward difference between
/// neighboring triangle slices, which is PrismaticDerivative() written for
/// this layout.
void WedgePrismaticDerivative(const WedgeOrder& order, const std::vector<double>& net,
  WedgeOrder& derivativeOrder, std::vector<double>& result)
{
  derivativeOrder = { order.Triangle, order.Prismatic - 1 };
  const int triangleSize = SimplexSize(2, order.Triangle);
  result.resize(derivativeOrder.Size()); // Every entry is written below.
  for (int k = 0; k < order.Prismatic; ++k)
  {
    for (int p = 0; p < triangleSize; ++p)
    {
      result[k * triangleSize + p] =
        order.Prismatic * (net[(k + 1) * triangleSize + p] - net[k * triangleSize + p]);
    }
  }
}

/// Everything one node's A3/A4 asks of one wedge equation, in a single
/// sweep of its net: the magnitude its zero band is scaled by, its
/// projection onto each of the triangle's 3 barycentric coordinates
/// (scratch.Smallest / scratch.Largest), and its projection onto
/// the prismatic axis (scratch.Smallest[3] / scratch.Largest[3]).
///
/// The five answers come from the same coefficients, and every layer number
/// each of them needs is already in hand at each point of the walk, so
/// reading the net once for all five replaces the five separate reads - one
/// for the magnitude, one per triangle coordinate and one for the prismatic
/// axis - this node would otherwise do per equation. A wedge node is the
/// costliest of the three domains' and this was most of it.
///
/// The walk visits the net in its stored order, which is the order each of
/// those five made its own pass in, so every value here is identical to the
/// last bit rather than merely equivalent.
void WedgeNodeBounds(
  const WedgeOrder& order, const std::vector<double>& net, Scratch& scratch, double& magnitude)
{
  const int n = order.Triangle;
  const int m = order.Prismatic;
  const int triangleSize = SimplexSize(2, n);
  std::array<double*, 3> smallest;
  std::array<double*, 3> largest;
  for (int j = 0; j < 3; ++j)
  {
    scratch.Smallest[j].assign(n + 1, std::numeric_limits<double>::infinity());
    scratch.Largest[j].assign(n + 1, -std::numeric_limits<double>::infinity());
    smallest[j] = scratch.Smallest[j].data();
    largest[j] = scratch.Largest[j].data();
  }
  std::vector<double>& axisSmallest = scratch.Smallest[3];
  std::vector<double>& axisLargest = scratch.Largest[3];
  axisSmallest.assign(m + 1, std::numeric_limits<double>::infinity());
  axisLargest.assign(m + 1, -std::numeric_limits<double>::infinity());
  // Through raw pointers taken once, for the reason SimplexAllLayerBounds()
  // gives.
  const double* values = net.data();
  double largestMagnitude = 0.;
  for (int k = 0; k <= m; ++k)
  {
    const double* slice = values + k * triangleSize;
    double prismaticSmallest = std::numeric_limits<double>::infinity();
    double prismaticLargest = -std::numeric_limits<double>::infinity();
    ForEachSimplexIndex<2>(n,
      [&](const std::array<int, 2>& index, int flat, int implicitIndex)
      {
        const double value = slice[flat];
        largestMagnitude = std::max(largestMagnitude, std::abs(value));
        for (int j = 0; j < 3; ++j)
        {
          const int layer = (j == 0) ? implicitIndex : index[j - 1];
          smallest[j][layer] = std::min(smallest[j][layer], value);
          largest[j][layer] = std::max(largest[j][layer], value);
        }
        prismaticSmallest = std::min(prismaticSmallest, value);
        prismaticLargest = std::max(prismaticLargest, value);
      });
    axisSmallest[k] = prismaticSmallest;
    axisLargest[k] = prismaticLargest;
  }
  magnitude = largestMagnitude;
}

/// Re-express every net in \a system over the sub-triangle whose vertices are
/// \a local in the current triangle's barycentric coordinates, leaving the
/// prismatic axis alone: SimplexReexpress() with WedgeMoveVertex() doing the
/// subdivision, and with each net's own order. Returns false on a degenerate
/// sub-triangle, which leaves \a system unusable.
bool WedgeReexpress(const std::array<WedgeOrder, 4>& orders,
  std::array<std::vector<double>, 4>& system, std::array<std::array<double, 3>, 3> local,
  Scratch& scratch)
{
  for (int k = 0; k < 3; ++k)
  {
    if (!(std::abs(local[k][k]) > 0.))
    {
      return false;
    }
    const std::array<double, 3> q = local[k];
    if (!MovesNothing(q, k))
    {
      for (std::size_t equation = 0; equation < system.size(); ++equation)
      {
        WedgeMoveVertex(orders[equation], system[equation], k, q, scratch, scratch.Net);
        system[equation].swap(scratch.Net);
      }
    }
    ReexpressUpdate(local, k);
  }
  return true;
}

/// The width of the wedge solver's zero band, relative to each equation's
/// magnitude over the node being tested.
///
/// Both wider than either pure shape's band and scaled the way only the box
/// one is, and both for reasons that were measured rather than assumed, over
/// the degenerate dip nets Testing/Cxx/TestBezierFindCriticalPoints.cxx
/// builds. The scaling: with a band fixed at the starting node, 14 of those
/// nets ran to the iteration cap, exactly as on a box and for the same
/// reason - the prismatic axis lets a node sit where the equations
/// have decayed to a tiny fraction of their global size, and a globally
/// scaled band can never prune there. The width: at 2e-16, the band the pure
/// shapes use, one of those nets lost its only critical point outright, and
/// at 1e-15 the same one did; 1e-14 loses none. A locally scaled band is
/// measured against a net that has already been subdivided many times, so
/// the rounding it has to absorb is that of the whole subdivision history,
/// not of one evaluation.
constexpr double WedgeZeroBand = 1e-14;

/// A wedge interior's critical points, in (r, s, t) with (r, s) the
/// triangular cross-section's barycentric pair and t the prismatic axis'
/// [-1, 1] parameter: every point where the field's gradient vanishes,
/// located to BarycentricRootTolerance.
///
/// These are answers, not starting guesses - nothing refines them
/// afterwards, which is what retired this path's LBFGS pass along with the
/// lattice-neighbor seeding that fed it. The gain is the same one the pure
/// shapes' ports made: a descent method cannot converge to a saddle, so the
/// interior saddles of a wedge went unreported before and are reported now.
///
/// The algorithm is the one A1-A9 skeleton both pure solvers follow, run
/// over a domain that is a triangle crossed with an interval. Each node is a
/// sub-triangle (three vertices in (r, s)) crossed with a sub-interval of t;
/// the system is the triangle's three pairwise barycentric derivatives plus
/// the prismatic one, four equations in three unknowns, the redundancy being
/// the kind Sect. 6.4 recommends. The two halves interleave without
/// interfering: A3/A4 projects onto all four coordinates, A5 shrinks the
/// triangle by its lower bounds (Eqs. 19 and 21) and the interval by both of
/// its, A7 re-expresses with one blossom sweep per triangle vertex and one de
/// Casteljau clip along t - they act on disjoint index groups, so they
/// commute - and A9 bisects whichever of the two is longer. A6 accepts only
/// when both have shrunk below tolerance.
///
/// Validated before being trusted, the same way both pure solvers were:
/// against an independent multi-start Newton solver over 695 random control
/// nets at triangle orders 1 through 5 and prismatic orders 1 through 5,
/// which found 797 roots between them with none missed, none spurious (six
/// points reported beyond the reference's own were checked directly and are
/// roots the reference's starting grid never reached, with residuals of
/// 1e-10 against equation magnitudes near 7), and a worst position error of
/// 2.7e-9 at a tolerance of 1e-10.
std::vector<std::array<double, 3>> FindWedgeInteriorPoints(
  int n, int m, const std::vector<double>& coefficients)
{
  std::vector<std::array<double, 3>> located;
  if (n < 1 || m < 1)
  {
    return located;
  }
  // Order 1 is not degenerate here, for the reason it is not on a box: the
  // triangle's derivatives are constant in (r, s) but still vary along t, so
  // the system can have an isolated root even at the lowest order.
  const WedgeOrder full{ n, m };

  // Reused across every node rather than reallocated at each: see Scratch.
  Scratch& scratch = AcquireScratch();
  NetPool& pool = scratch.Pool;

  std::array<WedgeOrder, 4> orders;
  std::array<std::vector<double>, 4> system;
  const std::array<std::array<int, 2>, 3> pairs{ { { { 0, 1 } }, { { 0, 2 } }, { { 1, 2 } } } };
  for (int equation = 0; equation < 3; ++equation)
  {
    system[equation] = pool.Take();
    WedgeTriangleDerivative(full, coefficients, pairs[equation][0], pairs[equation][1],
      orders[equation], system[equation]);
  }
  system[3] = pool.Take();
  WedgePrismaticDerivative(full, coefficients, orders[3], system[3]);

  double steepest = 0.;
  double flattest = std::numeric_limits<double>::infinity();
  for (int equation = 0; equation < 4; ++equation)
  {
    const double magnitude = MaxAbs(system[equation]);
    steepest = std::max(steepest, magnitude);
    flattest = std::min(flattest, magnitude);
  }
  // An equation that vanishes identically says the field does not vary along
  // that direction, so none of its critical points is isolated: see
  // FindPrismaticInteriorPoints(), which answers the same case the same way.
  if (flattest <= 1e-12 * std::max(steepest, 1.))
  {
    pool.Recycle(system);
    return located;
  }

  // The nets of nodes already dealt with go back to the pool, for the
  // children of later ones - the same free list the two pure shapes' solvers
  // use, and for the same reason. The problem stack holds sub-triangles
  // crossed with sub-intervals of t, and the system re-expressed over each.
  using Node = WedgeNode;
  std::vector<Node>& stack = scratch.Stack<Node>();

  // A1, over the wedge inset by BarycentricInterior on every face.
  {
    Node start;
    start.System = std::move(system);
    std::array<std::array<double, 3>, 3> local;
    for (int i = 0; i < 3; ++i)
    {
      local[i].fill(BarycentricInterior);
      local[i][i] = 1. - 2. * BarycentricInterior;
      start.Region.Vertices[i] = { { local[i][1], local[i][2] } };
    }
    if (!WedgeReexpress(orders, start.System, local, scratch))
    {
      pool.Recycle(start.System);
      return located;
    }
    start.Region.Lo = -1. + BarycentricInterior;
    start.Region.Hi = 1. - BarycentricInterior;
    for (int equation = 0; equation < 4; ++equation)
    {
      WedgeClipPrismatic(orders[equation], start.System[equation], 0.5 * BarycentricInterior,
        1. - 0.5 * BarycentricInterior, scratch);
    }
    stack.push_back(std::move(start));
  }

  int iterations = 0;
  while (!stack.empty()) // A2
  {
    if (++iterations > MaxBarycentricIterations)
    {
      vtkLogF(WARNING,
        "Critical-point search gave up on a wedge interior of triangle order %d and prismatic "
        "order %d after %d subdivisions; its critical points are probably not isolated, and "
        "what is reported here samples them rather than listing them.",
        n, m, MaxBarycentricIterations);
      break;
    }
    Node node = std::move(stack.back());
    stack.pop_back();

    // A3 and A4: per coordinate - the triangle's three, then t - intersect
    // what every equation allows.
    //
    // Equation-major, so that each equation's net is read once for its zero
    // band and all four of its projections together (WedgeNodeBounds())
    // rather than once for each. The bounds are intersected across equations
    // either way, so this only changes how much of a node's projection is
    // done before an infeasible one is abandoned, never the outcome.
    std::array<double, 3> lowerBound{ { 0., 0., 0. } };
    std::array<double, 3> upperBound{ { 1., 1., 1. } };
    double prismaticLo = 0.;
    double prismaticHi = 1.;
    bool feasible = true;
    for (int equation = 0; equation < 4 && feasible; ++equation)
    {
      double magnitude;
      WedgeNodeBounds(orders[equation], node.System[equation], scratch, magnitude);
      const double band = WedgeZeroBand * magnitude;

      double equationLo;
      double equationHi;
      for (int j = 0; j < 3; ++j)
      {
        if (!HullZeroSpan(
              scratch.Smallest[j], scratch.Largest[j], scratch.Hull, band, equationLo, equationHi))
        {
          feasible = false;
          break;
        }
        lowerBound[j] = std::max(lowerBound[j], equationLo);
        upperBound[j] = std::min(upperBound[j], equationHi);
        if (lowerBound[j] > upperBound[j])
        {
          feasible = false;
          break;
        }
      }
      if (!feasible)
      {
        break;
      }
      if (!HullZeroSpan(
            scratch.Smallest[3], scratch.Largest[3], scratch.Hull, band, equationLo, equationHi))
      {
        feasible = false;
        break;
      }
      prismaticLo = std::max(prismaticLo, equationLo);
      prismaticHi = std::min(prismaticHi, equationHi);
      feasible = prismaticLo <= prismaticHi;
    }
    if (!feasible)
    {
      pool.Recycle(node.System);
      continue;
    }

    // A5: the triangle's lower bounds define the smaller triangle and the
    // prismatic bounds the smaller interval.
    std::array<std::array<double, 3>, 3> local;
    std::array<std::array<double, 2>, 3> shrunk;
    if (!ShrinkSimplex(lowerBound, node.Region.Vertices, local, shrunk))
    {
      pool.Recycle(node.System);
      continue;
    }
    const double width = node.Region.Hi - node.Region.Lo;
    const double shrunkLo = node.Region.Lo + prismaticLo * width;
    const double shrunkHi = node.Region.Lo + prismaticHi * width;
    const double shrunkWidth = shrunkHi - shrunkLo;

    int splitFrom;
    int splitTo;
    const double longest = LongestEdge(shrunk, splitFrom, splitTo);

    // Already answered: see AlreadyLocated(). A wedge node is a triangle
    // crossed with an interval, so its corners are each triangle vertex at
    // each end of that interval.
    if (longest < BarycentricPointMerge && shrunkWidth < BarycentricPointMerge)
    {
      std::array<std::array<double, 3>, 6> corners;
      for (int vertex = 0; vertex < 3; ++vertex)
      {
        for (int end = 0; end < 2; ++end)
        {
          auto& corner = corners[vertex * 2 + end];
          corner[0] = shrunk[vertex][0];
          corner[1] = shrunk[vertex][1];
          corner[2] = (end == 0) ? shrunkLo : shrunkHi;
        }
      }
      if (AlreadyLocated<3, 6>(located, corners, BarycentricPointMerge))
      {
        pool.Recycle(node.System);
        continue;
      }
    }

    // A6: small enough, in both factors at once, to call a root.
    if (longest < BarycentricRootTolerance && shrunkWidth < BarycentricRootTolerance)
    {
      std::array<double, 3> root{};
      for (int axis = 0; axis < 2; ++axis)
      {
        root[axis] = (shrunk[0][axis] + shrunk[1][axis] + shrunk[2][axis]) / 3.;
      }
      root[2] = 0.5 * (shrunkLo + shrunkHi);
      if (!AppendRoot(located, root, BarycentricPointMerge, MaxBarycentricPoints))
      {
        break; // Not an isolated root set; stop rather than enumerate it.
      }
      pool.Recycle(node.System);
      continue;
    }

    // A7: the triangle's blossom subdivision and the prismatic clip, which
    // act on disjoint groups of indices and so commute. The node's own
    // system is not read again past this point, so it is re-expressed in
    // place rather than copied.
    auto reduced = std::move(node.System);
    if (!WedgeReexpress(orders, reduced, local, scratch))
    {
      pool.Recycle(reduced);
      continue;
    }
    if (prismaticLo > 0. || prismaticHi < 1.)
    {
      for (int equation = 0; equation < 4; ++equation)
      {
        WedgeClipPrismatic(orders[equation], reduced[equation], prismaticLo, prismaticHi, scratch);
      }
    }

    // A8: shrinking fast enough, in both factors, to simply recurse on.
    if (shrunkWidth < BarycentricShrinkRatio * width &&
      EdgesShrankBy(shrunk, node.Region.Vertices, BarycentricShrinkRatio))
    {
      stack.push_back(Node{ { shrunk, shrunkLo, shrunkHi }, std::move(reduced) });
      continue;
    }

    // A9: it is not, so bisect whichever of the two factors is longer.
    if (longest >= shrunkWidth)
    {
      std::array<double, 3> midpoint{};
      midpoint[splitFrom] = 0.5;
      midpoint[splitTo] = 0.5;
      std::array<double, 2> moved;
      for (int axis = 0; axis < 2; ++axis)
      {
        moved[axis] = 0.5 * (shrunk[splitFrom][axis] + shrunk[splitTo][axis]);
      }
      for (int replaced : { splitTo, splitFrom })
      {
        Node child;
        child.Region.Vertices = shrunk;
        child.Region.Vertices[replaced] = moved;
        child.Region.Lo = shrunkLo;
        child.Region.Hi = shrunkHi;
        for (int equation = 0; equation < 4; ++equation)
        {
          child.System[equation] = pool.Take();
          WedgeMoveVertex(orders[equation], reduced[equation], replaced, midpoint, scratch,
            child.System[equation]);
        }
        stack.push_back(std::move(child));
      }
    }
    else
    {
      const double middle = 0.5 * (shrunkLo + shrunkHi);
      Node lowerHalf;
      Node upperHalf;
      lowerHalf.Region.Vertices = shrunk;
      upperHalf.Region.Vertices = shrunk;
      lowerHalf.Region.Lo = shrunkLo;
      lowerHalf.Region.Hi = middle;
      upperHalf.Region.Lo = middle;
      upperHalf.Region.Hi = shrunkHi;
      for (int equation = 0; equation < 4; ++equation)
      {
        lowerHalf.System[equation] = pool.Take();
        upperHalf.System[equation] = pool.Take();
        WedgeSplitPrismatic(orders[equation], reduced[equation], scratch,
          lowerHalf.System[equation], upperHalf.System[equation]);
      }
      stack.push_back(std::move(lowerHalf));
      stack.push_back(std::move(upperHalf));
    }
    pool.Recycle(reduced);
  }
  // A search stopped at one of its caps leaves nodes behind; their nets go
  // back to the pool rather than out with the stack.
  for (auto& node : stack)
  {
    pool.Recycle(node.System);
  }
  stack.clear();
  return located;
}

/// Locate every critical point on one wedge quadrilateral side face,
/// already extracted as an ordinary (n+1)x(m+1)-control-point
/// tensor-product patch, and append each to \a points. \a embed maps that
/// patch's own [-1,1]x[-1,1] parameters to the cell's real (r, s, t)
/// point.
///
/// The 2-D counterpart of SearchBarycentricEdge(), and for the same
/// reason: a side face is an ordinary tensor-product patch whose
/// embedding does not fit SearchEntity()'s convention.
void SearchBarycentricFace(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, int n, int m,
  const std::vector<double>& faceCoefficients, const std::vector<double>& fullCoefficients,
  const std::function<std::array<double, 3>(double, double)>& embed,
  std::vector<CriticalPoint>& points)
{
  const auto located = FindPrismaticInteriorPoints<2>({ { n, m } }, faceCoefficients);
  for (const auto& local : located)
  {
    AppendIfNew(basisOp, gradOp, 3, fullCoefficients, embed(local[0], local[1]), 2, points);
  }
}

/// Extract the (n+1)x(m+1)-sized tensor-product sub-array of a wedge's
/// control net for the quadrilateral side face on the triangular
/// cross-section's edge selected by \a edgeCase (0: j = 0, free i; 1:
/// i = 0, free j; 2: i + j = n, the hypotenuse, free "diagonal" index
/// d with i = n - d, j = d), laid out first-axis-fastest to match the
/// tensor-product convention FindPrismaticInteriorPoints() expects. Written
/// into \a result, which the caller reuses from face to face.
void ExtractWedgeSideFace(int n, int m, const std::vector<double>& coefficients,
  const TriangleEdge& edge, std::vector<double>& result)
{
  result.resize((n + 1) * (m + 1)); // Every entry is written below.
  for (int k = 0; k <= m; ++k)
  {
    for (int p = 0; p <= n; ++p)
    {
      const auto ij = edge.Index(p, n);
      result[p + k * (n + 1)] = coefficients[WedgeIndex(ij[0], ij[1], k, n)];
    }
  }
}

// -----------------------------------------------------------------------
// Pyramid (collapsed box) domain
// -----------------------------------------------------------------------
// The pyramid needs no solver of its own. Chan & Warburton's basis
// (arXiv:1508.05609; see Basis/Bezier/PyrCnBasis.h) is a product of 1-D
// Bernstein polynomials in the collapsed coordinates a = (r + w) / (2w),
// b = (s + w) / (2w), c = t, with w = 1 - t, which range over the unit cube.
// That map is a diffeomorphism everywhere but the apex, which it blows up
// into the whole c = 1 face, and the apex is a corner, which is never
// reported. So each of the pyramid's entities is solved by the machinery
// for whatever shape it is in those coordinates:
//
// + The interior is a box, once every layer of the net is raised to the
//   same degree in a and b (PyramidTensorNet()), and goes to the box
//   solver - with the two cross-section equations divided by the factor
//   1 - c that makes them vanish on the collapsed face
//   (FindPyramidInteriorPoints()).
// + The quadrilateral base is the net's own k = 0 layer, a tensor net.
// + Each triangular face is exactly a Bezier triangle (Chan & Warburton's
//   Lemma 3.2), in affine coordinates, so it goes to the barycentric
//   solver.
// + Each edge is a 1-D Bezier curve.
//
// The net is laid out the way vtkDGLagrangePoints and PyrCnBasis.h number a
// pyramid's arbitrary-order degrees of freedom: one square layer per index k
// along t (0 <= k <= n), of (n - k + 1)^2 points (i, j) each, i fastest, then
// j, then k. Lattice point (i, j, k) is therefore at
// sum(l < k) (n - l + 1)^2 + i + j (n - k + 1); every walk below advances
// that layer offset as it goes rather than recomputing it.

/// The pyramid's control net as an order (n, n, n) tensor net over the
/// collapsed coordinates (a, b, c), first axis fastest, written into \a net.
///
/// Layer k is a Bernstein net of degree m = n - k in a and in b, so raising
/// it to degree n in each - which changes the net and not the polynomial -
/// makes the field an ordinary Bezier hexahedron in (a, b, c). That keeps
/// the convex-hull bound every step of the box solver relies on.
///
/// Degree-n coefficient i of a raised run is the sum over p of
/// C(m, p) C(k, i - p) times degree-m coefficient p, divided by C(n, i).
/// The binomials are read off vtkMath's table as they are needed, so this
/// keeps no table of its own. The division comes last on purpose: the
/// weights are then integers, so a constant run sums to exactly C(n, i)
/// times itself and divides back to exactly that constant. Folding
/// 1 / C(n, i) into each weight instead rounds it, and a flat region of the
/// net comes out flat only to within 1e-16 - which the box solver's zero
/// band, scaled to the subdomain it is testing, does not absorb: measured on
/// the order-5 single-dip nets of TestBezierFindCriticalPoints, that noise
/// alone sent the interior search to its iteration cap.
void PyramidTensorNet(int n, const std::vector<double>& coefficients, std::vector<double>& net)
{
  const int size = n + 1;
  net.resize(size * size * size);
  const double* source = coefficients.data();
  for (int k = 0; k <= n; ++k)
  {
    const int m = n - k;
    double* target = net.data() + k * size * size;
    // Along a, each of the layer's m + 1 rows into the same row of the
    // target layer. The rows are read from the cell's net, so nothing here
    // needs a buffer.
    for (int j = 0; j <= m; ++j)
    {
      const double* row = source + j * (m + 1);
      for (int i = 0; i <= n; ++i)
      {
        double raised = 0.;
        for (int p = std::max(0, i - k); p <= std::min(m, i); ++p)
        {
          raised += vtkMath::DoubleBinomial(m, p) * vtkMath::DoubleBinomial(k, i - p) * row[p];
        }
        target[i + j * size] = raised / vtkMath::DoubleBinomial(n, i);
      }
    }
    // Then along b, in place. Raised row j reads rows q <= min(m, j) only,
    // so filling rows from j = n down never overwrites a row a later (lower)
    // j still reads; row j itself is read, one column at a time, before
    // that column of it is written.
    for (int j = n; j >= 0; --j)
    {
      for (int i = 0; i <= n; ++i)
      {
        double raised = 0.;
        for (int q = std::max(0, j - k); q <= std::min(m, j); ++q)
        {
          raised += vtkMath::DoubleBinomial(m, q) * vtkMath::DoubleBinomial(k, j - q) *
            target[i + q * size];
        }
        target[i + j * size] = raised / vtkMath::DoubleBinomial(n, j);
      }
    }
    source += (m + 1) * (m + 1);
  }
}

/// A pyramid's strictly-interior critical points, in its own (r, s, t)
/// parameters.
///
/// Solved in the collapsed coordinates, over PyramidTensorNet(), by the box
/// solver. Since the map is a diffeomorphism off the apex, grad(field) = 0 in
/// (r, s, t) exactly where it is in (a, b, c), but the plain partial
/// derivatives are the wrong system to hand the solver. Every layer below
/// the apex carries a factor of (1 - c)^(n - k), and the apex layer does not
/// vary in a or b at all, so df/da and df/db vanish on the whole c = 1 face:
/// a continuum of spurious roots, which the box's inset would only keep the
/// search a hair away from. Dividing it out is exact - their nets' own
/// k = n layer is zero - and is also what the pyramid's own gradient does:
/// df/dr = (df/da) / (2w), with w = 1 - c. So the system is
///
///   (df/da) / (1 - c),  (df/db) / (1 - c),  df/dc,
///
/// which is grad(field) in (r, s, t) up to a non-singular change of basis
/// (when the first two vanish, df/dt reduces to df/dc). On the collapsed
/// face it only has a root where the gradient tends to zero as the apex is
/// approached from one particular direction - three conditions on two
/// unknowns, so not a continuum and not generic either. Chan & Warburton's
/// own derivative formulas (Sect. 3.1) absorb the same 1 / (1 - c).
///
/// The inset does do its job without the division: measured over random
/// nets at orders 2 to 5, the undivided system finds exactly the same roots.
/// It costs 4-27% more, though.
std::vector<std::array<double, 3>> FindPyramidInteriorPoints(
  int n, const std::vector<double>& coefficients)
{
  if (n < 1)
  {
    return {};
  }

  // One Scratch for building the equations and for solving them.
  Scratch& scratch = AcquireScratch();
  PyramidTensorNet(n, coefficients, scratch.Net);
  const std::vector<double>& net = scratch.Net;
  const std::array<int, 3> order{ { n, n, n } };
  std::array<std::array<int, 3>, 3> orders;
  std::array<std::vector<double>, 3> system;
  for (int axis = 0; axis < 3; ++axis)
  {
    system[axis] = scratch.Pool.Take();
    PrismaticDerivative<3>(order, net, axis, orders[axis], system[axis]);
  }
  // Divide the two cross-section equations by (1 - c). With c the slowest
  // axis, dropping their (zero) k = n layer is a truncation, and each
  // remaining layer k is rescaled by n / (n - k), since
  // B_k^n(c) = n / (n - k) (1 - c) B_k^(n-1)(c).
  for (int axis = 0; axis < 2; ++axis)
  {
    const int layerSize = (orders[axis][0] + 1) * (orders[axis][1] + 1);
    system[axis].resize(layerSize * n);
    for (int k = 0; k < n; ++k)
    {
      const double scale = static_cast<double>(n) / (n - k);
      for (int flat = 0; flat < layerSize; ++flat)
      {
        system[axis][k * layerSize + flat] *= scale;
      }
    }
    orders[axis][2] = n - 1;
  }

  // Back from the box's [-1, 1]^3 to (r, s, t), in place.
  auto located = SolvePrismaticSystem<3>(orders, system, scratch);
  for (auto& point : located)
  {
    const double t = 0.5 * (point[2] + 1.);
    const double w = 1. - t;
    point = { { point[0] * w, point[1] * w, t } };
  }
  return located;
}

/// One of a pyramid's 4 triangular faces: the collapsed coordinate that is
/// pinned (0 for a, 1 for b) and the end it is pinned to (0 or 1). The face
/// is r = -w, r = w, s = -w, or s = w.
struct PyramidFace
{
  int Axis;
  int End;

  /// The face's own triangle net, in SimplexIndex() order: local index p
  /// along the free cross-section axis, q = k along t.
  ///
  /// On a = 0 the pyramid's basis function B_0jk reduces to
  /// B_j^(n-k)(b) B_k^n(c), which is the order-n triangle Bernstein
  /// polynomial with barycentric coordinates b(1 - c), (1 - b)(1 - c) and c
  /// - so the field there is exactly the Bezier triangle on these
  /// coefficients. The other 3 faces differ only in which index is pinned.
  ///
  /// Written into \a net, which the caller reuses across faces.
  void Net(int n, const std::vector<double>& coefficients, std::vector<double>& net) const
  {
    net.resize(SimplexSize(2, n));
    int local = 0;
    int offset = 0; // Offset of layer q, advanced one layer at a time.
    for (int q = 0; q <= n; ++q)
    {
      const int width = n - q + 1;
      const int pinned = this->End == 0 ? 0 : n - q;
      const int pinnedStride = this->Axis == 0 ? 1 : width;
      const int freeStride = this->Axis == 0 ? width : 1;
      for (int p = 0; p <= n - q; ++p)
      {
        net[local++] = coefficients[offset + pinned * pinnedStride + p * freeStride];
      }
      offset += width * width;
    }
  }

  /// Where the face's own barycentric point (lambda_0 along p, lambda_1 = t)
  /// lands in (r, s, t). The map is affine, being a triangle's.
  std::array<double, 3> Embed(const std::array<double, 3>& lambda) const
  {
    const double w = 1. - lambda[1];
    const double pinned = this->End == 0 ? -w : w;
    const double along = 2. * lambda[0] - w;
    return this->Axis == 0 ? std::array<double, 3>{ { pinned, along, lambda[1] } }
                           : std::array<double, 3>{ { along, pinned, lambda[1] } };
  }
};
constexpr std::array<PyramidFace, 4> PyramidFaces{ { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } } };

} // anonymous namespace

double EvaluateField(vtkDGOperatorEntry& basisOp, vtkDGOperatorEntry& gradOp, int dimension,
  const std::vector<double>& coefficients, const std::array<double, 3>& point,
  std::array<double, 3>& gradient)
{
  const int numFunctions = basisOp.NumberOfFunctions;
  FieldValues& values = AcquireFieldValues();
  std::vector<double>& basisValues = values.Basis;
  std::vector<double>& gradValues = values.Gradient;
  basisValues.resize(numFunctions);
  gradValues.resize(numFunctions * 3);
  basisOp.Evaluate(point, basisValues);
  gradOp.Evaluate(point, gradValues);

  double value = 0.;
  for (int ff = 0; ff < numFunctions; ++ff)
  {
    value += coefficients[ff] * basisValues[ff];
  }

  gradient = { { 0., 0., 0. } };
  for (int axis = 0; axis < dimension; ++axis)
  {
    double g = 0.;
    for (int ff = 0; ff < numFunctions; ++ff)
    {
      g += coefficients[ff] * gradValues[ff * 3 + axis];
    }
    gradient[axis] = g;
  }
  return value;
}

std::vector<CriticalPoint> FindPrismaticInteriorCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  std::vector<CriticalPoint> points;
  const int dimension = static_cast<int>(order.size());
  if (dimension < 1)
  {
    return points; // A vertex has no interior to search.
  }

  const std::array<int, 3> fixedIndex{ { 0, 0, 0 } };
  // dimension is dispatched to a fixed template argument here - the one
  // place a runtime cell dimension becomes a compile-time one for
  // FindPrismaticInteriorPoints<K>() below (see its own comment).
  switch (dimension)
  {
    case 1:
    {
      const auto located = FindPrismaticInteriorPoints<1>({ { order[0] } }, coefficients);
      AppendPrismaticInterior(basisOp, gradOp, dimension, coefficients, std::array<int, 1>{ { 0 } },
        fixedIndex, located, points);
      break;
    }
    case 2:
    {
      const auto located = FindPrismaticInteriorPoints<2>({ { order[0], order[1] } }, coefficients);
      AppendPrismaticInterior(basisOp, gradOp, dimension, coefficients,
        std::array<int, 2>{ { 0, 1 } }, fixedIndex, located, points);
      break;
    }
    case 3:
    {
      const auto located =
        FindPrismaticInteriorPoints<3>({ { order[0], order[1], order[2] } }, coefficients);
      AppendPrismaticInterior(basisOp, gradOp, dimension, coefficients,
        std::array<int, 3>{ { 0, 1, 2 } }, fixedIndex, located, points);
      break;
    }
    default:
      break; // dimension is always 1..3: bounded by the supported shapes.
  }
  return points;
}

std::vector<CriticalPoint> FindPrismaticBoundaryCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  std::vector<CriticalPoint> points;
  const int dimension = static_cast<int>(order.size());
  if (dimension < 2)
  {
    return points; // An edge's boundary is its two corners, never reported.
  }

  const std::array<int, 3> stride = ComputeStride(order);
  const std::array<int, 3> fixedIndex{ { 0, 0, 0 } };
  std::array<bool, EntityCodeCount> visited{};
  std::vector<double>& entityNet = AcquireEntityNets().Face;
  // dimension is dispatched to a fixed template argument here - the one
  // place a runtime cell dimension becomes a compile-time one for the
  // whole SearchBoundaryEntities() recursion below it.
  switch (dimension)
  {
    case 2:
      SearchBoundaryEntities(basisOp, gradOp, order, stride, coefficients, dimension,
        std::array<int, 2>{ { 0, 1 } }, fixedIndex, visited, entityNet, points);
      break;
    case 3:
      SearchBoundaryEntities(basisOp, gradOp, order, stride, coefficients, dimension,
        std::array<int, 3>{ { 0, 1, 2 } }, fixedIndex, visited, entityNet, points);
      break;
    default:
      break; // dimension is always 1..3: bounded by the supported shapes.
  }
  return points;
}

std::vector<CriticalPoint> FindPrismaticCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  auto points = FindPrismaticInteriorCriticalPoints(basisOp, gradOp, order, coefficients);
  auto boundaryPoints = FindPrismaticBoundaryCriticalPoints(basisOp, gradOp, order, coefficients);
  points.insert(points.end(), boundaryPoints.begin(), boundaryPoints.end());
  return points;
}

std::vector<CriticalPoint> FindBarycentricInteriorCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  std::vector<CriticalPoint> points;
  const int dimension = static_cast<int>(order.size());
  if (dimension < 2 || order[0] < 1)
  {
    return points; // A constant or linear cell has no critical points to find.
  }
  const int n = order[0];

  // A genuine dimension-many-variable barycentric search, shared by
  // triangle (dimension 2) and tetrahedron (dimension 3) alike. dimension
  // is dispatched to a fixed template argument here, the same reason
  // SearchEntity() does so for the tensor-product path.
  if (dimension == 2)
  {
    auto interiorPoints = FindBarycentricInteriorPoints<2>(n, coefficients);
    if (!interiorPoints.empty())
    {
      BarycentricEmbed embed = [](const std::array<double, 3>& lambda) -> std::array<double, 3>
      { return { { lambda[0], lambda[1], 0. } }; };
      AppendBarycentricInterior<2>(
        basisOp, gradOp, dimension, coefficients, embed, interiorPoints, dimension, points);
    }
  }
  else
  {
    auto interiorPoints = FindBarycentricInteriorPoints<3>(n, coefficients);
    if (!interiorPoints.empty())
    {
      BarycentricEmbed embed = [](const std::array<double, 3>& lambda) -> std::array<double, 3>
      { return { { lambda[0], lambda[1], lambda[2] } }; };
      AppendBarycentricInterior<3>(
        basisOp, gradOp, dimension, coefficients, embed, interiorPoints, dimension, points);
    }
  }
  return points;
}

std::vector<CriticalPoint> FindBarycentricBoundaryCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  std::vector<CriticalPoint> points;
  const int dimension = static_cast<int>(order.size());
  if (dimension < 2 || order[0] < 1)
  {
    return points; // A constant or linear cell has no critical points to find.
  }
  const int n = order[0];
  EntityNets& nets = AcquireEntityNets();
  std::vector<double>& edgeCoefficients = nets.Edge;
  edgeCoefficients.resize(n + 1);

  // Every straight edge of the simplex: two of its dimension + 1
  // barycentric coordinates free, the rest pinned to 0.
  if (dimension == 2)
  {
    for (const auto& edge : TriangleEdges)
    {
      for (int p = 0; p <= n; ++p)
      {
        edgeCoefficients[p] = coefficients[SimplexIndex<2>(edge.Index(p, n), n)];
      }
      auto embed = [&edge](double u) -> std::array<double, 3>
      {
        const auto rs = edge.Parameter(u);
        return { { rs[0], rs[1], 0. } };
      };
      SearchBarycentricEdge(
        basisOp, gradOp, dimension, n, edgeCoefficients, coefficients, embed, points);
    }
  }
  else if (dimension == 3)
  {
    // The 4 triangular faces, one per barycentric coordinate pinned to 0.
    // Each is searched exactly like a standalone triangle's own interior
    // (FindBarycentricInteriorPoints()/AppendBarycentricInterior() with
    // dLocal = 2), over the 2-D sub-array ExtractTetrahedronFace() takes
    // from the tetrahedron's control net. That sub-array is not merely a
    // signal about the face, the way ExtractEntityCoefficients() is for
    // the tensor-product path: a Bezier simplex restricted to one of its
    // faces is exactly the Bezier triangle on the coefficients whose
    // pinned index is 0, so a critical point of the face is located here
    // outright. Only the field's value at it is read back through the
    // tetrahedron's own full basis at dimension 3, in AppendIfNew().
    struct FaceSpec
    {
      int Pinned;
      BarycentricEmbed Embed;
    };
    const std::array<FaceSpec, 4> faces{ {
      // lambda_0 = 0: r + s + t = 1.
      { 0, [](const std::array<double, 3>& lambda) -> std::array<double, 3>
        { return { { lambda[0], lambda[1], 1. - lambda[0] - lambda[1] } }; } },
      // lambda_1 = 0: r = 0.
      { 1, [](const std::array<double, 3>& lambda) -> std::array<double, 3>
        { return { { 0., lambda[0], lambda[1] } }; } },
      // lambda_2 = 0: s = 0.
      { 2, [](const std::array<double, 3>& lambda) -> std::array<double, 3>
        { return { { lambda[0], 0., lambda[1] } }; } },
      // lambda_3 = 0: t = 0.
      { 3, [](const std::array<double, 3>& lambda) -> std::array<double, 3>
        { return { { lambda[0], lambda[1], 0. } }; } },
    } };
    for (const auto& face : faces)
    {
      ExtractTetrahedronFace(n, coefficients, face.Pinned, nets.Face);
      auto facePoints = FindBarycentricInteriorPoints<2>(n, nets.Face);
      if (!facePoints.empty())
      {
        AppendBarycentricInterior<2>(
          basisOp, gradOp, dimension, coefficients, face.Embed, facePoints, 2, points);
      }
    }

    // The 6 edges, one per pair of the tetrahedron's 4 barycentric
    // coordinates (lambda_0 implicit, lambda_1 = r, lambda_2 = s,
    // lambda_3 = t) that stay free while the other two are pinned to 0.
    struct EdgeSpec
    {
      std::function<int(int)> Index; // maps a local index 0..n to a flat coefficient index
      std::function<std::array<double, 3>(double)> Embed;
    };
    const std::array<EdgeSpec, 6> edges{ {
      // {lambda_0, lambda_1} free: s = 0, t = 0.
      { [n](int i) { return SimplexIndex<3>({ { i, 0, 0 } }, n); },
        [](double u) -> std::array<double, 3> { return { { (u + 1.) / 2., 0., 0. } }; } },
      // {lambda_0, lambda_2} free: r = 0, t = 0.
      { [n](int j) { return SimplexIndex<3>({ { 0, j, 0 } }, n); },
        [](double u) -> std::array<double, 3> { return { { 0., (u + 1.) / 2., 0. } }; } },
      // {lambda_0, lambda_3} free: r = 0, s = 0.
      { [n](int k) { return SimplexIndex<3>({ { 0, 0, k } }, n); },
        [](double u) -> std::array<double, 3> { return { { 0., 0., (u + 1.) / 2. } }; } },
      // {lambda_1, lambda_2} free: t = 0, r + s = 1.
      { [n](int i) { return SimplexIndex<3>({ { i, n - i, 0 } }, n); },
        [](double u) -> std::array<double, 3>
        {
          const double t = (u + 1.) / 2.;
          return { { t, 1. - t, 0. } };
        } },
      // {lambda_1, lambda_3} free: s = 0, r + t = 1.
      { [n](int i) { return SimplexIndex<3>({ { i, 0, n - i } }, n); },
        [](double u) -> std::array<double, 3>
        {
          const double t = (u + 1.) / 2.;
          return { { t, 0., 1. - t } };
        } },
      // {lambda_2, lambda_3} free: r = 0, s + t = 1.
      { [n](int j) { return SimplexIndex<3>({ { 0, j, n - j } }, n); },
        [](double u) -> std::array<double, 3>
        {
          const double t = (u + 1.) / 2.;
          return { { 0., t, 1. - t } };
        } },
    } };
    for (const auto& edge : edges)
    {
      for (int local = 0; local <= n; ++local)
      {
        edgeCoefficients[local] = coefficients[edge.Index(local)];
      }
      SearchBarycentricEdge(
        basisOp, gradOp, dimension, n, edgeCoefficients, coefficients, edge.Embed, points);
    }
  }

  return points;
}

std::vector<CriticalPoint> FindBarycentricCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  auto points = FindBarycentricInteriorCriticalPoints(basisOp, gradOp, order, coefficients);
  auto boundaryPoints = FindBarycentricBoundaryCriticalPoints(basisOp, gradOp, order, coefficients);
  points.insert(points.end(), boundaryPoints.begin(), boundaryPoints.end());
  return points;
}

std::vector<CriticalPoint> FindWedgeInteriorCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  std::vector<CriticalPoint> points;
  if (order.size() != 3 || order[0] < 1)
  {
    return points; // A constant or linear wedge has no critical points to find.
  }
  const int n = order[0]; // Triangular cross-section order (order[1] == order[0]).
  const int m = order[2]; // Independent prismatic-axis order.

  // The mixed shape's own combined search.
  for (const auto& rst : FindWedgeInteriorPoints(n, m, coefficients))
  {
    AppendIfNew(basisOp, gradOp, 3, coefficients, rst, 3, points);
  }
  return points;
}

std::vector<CriticalPoint> FindWedgeBoundaryCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  std::vector<CriticalPoint> points;
  if (order.size() != 3 || order[0] < 1)
  {
    return points; // A constant or linear wedge has no critical points to find.
  }
  const int n = order[0]; // Triangular cross-section order (order[1] == order[0]).
  const int m = order[2]; // Independent prismatic-axis order.
  const int triSize = SimplexSize(2, n);

  // The 2 triangular caps (t = -1, t = +1): each is a standalone
  // triangle's own interior search (dLocal = 2), using the contiguous
  // slice of coefficients for that t already laid out that way.
  // Every entity's net is cut into one of two buffers: each cap's and side
  // face's into the first, and each of the 9 edges' into the second, which
  // each resizes to its own order first.
  EntityNets& nets = AcquireEntityNets();
  std::vector<double>& edgeCoefficients = nets.Edge;
  for (int k : { 0, m })
  {
    const double tval = -1. + 2. * k / m;
    nets.Face.assign(coefficients.begin() + k * triSize, coefficients.begin() + (k + 1) * triSize);
    auto capPoints = FindBarycentricInteriorPoints<2>(n, nets.Face);
    if (!capPoints.empty())
    {
      BarycentricEmbed embed = [tval](const std::array<double, 3>& lambda) -> std::array<double, 3>
      { return { { lambda[0], lambda[1], tval } }; };
      AppendBarycentricInterior<2>(basisOp, gradOp, 3, coefficients, embed, capPoints, 2, points);
    }

    // That cap's own 3 edges.
    edgeCoefficients.resize(n + 1);
    for (const auto& edge : TriangleEdges)
    {
      for (int p = 0; p <= n; ++p)
      {
        const auto ij = edge.Index(p, n);
        edgeCoefficients[p] = coefficients[WedgeIndex(ij[0], ij[1], k, n)];
      }
      auto embed = [&edge, tval](double u) -> std::array<double, 3>
      {
        const auto rs = edge.Parameter(u);
        return { { rs[0], rs[1], tval } };
      };
      SearchBarycentricEdge(basisOp, gradOp, 3, n, edgeCoefficients, coefficients, embed, points);
    }
  }

  // The 3 quadrilateral side faces: an ordinary tensor-product patch
  // (triangle-edge parameter x prismatic axis) once extracted.
  for (const auto& edge : TriangleEdges)
  {
    ExtractWedgeSideFace(n, m, coefficients, edge, nets.Face);
    auto embed = [&edge](double u, double v) -> std::array<double, 3>
    {
      const auto rs = edge.Parameter(u);
      return { { rs[0], rs[1], v } };
    };
    SearchBarycentricFace(basisOp, gradOp, n, m, nets.Face, coefficients, embed, points);
  }

  // The 3 vertical edges, one per triangle corner, along the prismatic axis.
  const std::array<std::array<int, 2>, 3> corners{ { { { 0, 0 } }, { { n, 0 } }, { { 0, n } } } };
  edgeCoefficients.resize(m + 1);
  for (const auto& corner : corners)
  {
    for (int k = 0; k <= m; ++k)
    {
      edgeCoefficients[k] = coefficients[WedgeIndex(corner[0], corner[1], k, n)];
    }
    const double r = corner[0] / static_cast<double>(n);
    const double s = corner[1] / static_cast<double>(n);
    auto embed = [r, s](double u) -> std::array<double, 3> { return { { r, s, u } }; };
    SearchBarycentricEdge(basisOp, gradOp, 3, m, edgeCoefficients, coefficients, embed, points);
  }

  return points;
}

std::vector<CriticalPoint> FindWedgeCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  auto points = FindWedgeInteriorCriticalPoints(basisOp, gradOp, order, coefficients);
  auto boundaryPoints = FindWedgeBoundaryCriticalPoints(basisOp, gradOp, order, coefficients);
  points.insert(points.end(), boundaryPoints.begin(), boundaryPoints.end());
  return points;
}

std::vector<CriticalPoint> FindPyramidInteriorCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  std::vector<CriticalPoint> points;
  if (order.size() != 3 || order[0] < 1)
  {
    return points; // A constant pyramid has no critical points to find.
  }
  for (const auto& rst : FindPyramidInteriorPoints(order[0], coefficients))
  {
    AppendIfNew(basisOp, gradOp, 3, coefficients, rst, 3, points);
  }
  return points;
}

std::vector<CriticalPoint> FindPyramidBoundaryCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  std::vector<CriticalPoint> points;
  if (order.size() != 3 || order[0] < 1)
  {
    return points; // A constant pyramid has no critical points to find.
  }
  const int n = order[0]; // order[1] and order[2] are equal to it.

  // Every entity's net is cut into one of these two buffers: the base's and
  // each triangular face's into the first, each edge's into the second.
  EntityNets& nets = AcquireEntityNets();
  std::vector<double>& faceCoefficients = nets.Face;
  std::vector<double>& edgeCoefficients = nets.Edge;

  // The quadrilateral base (t = 0), whose net is the k = 0 layer as is.
  faceCoefficients.assign(coefficients.begin(), coefficients.begin() + (n + 1) * (n + 1));
  SearchBarycentricFace(
    basisOp, gradOp, n, n, faceCoefficients, coefficients,
    [](double u, double v) -> std::array<double, 3> { return { { u, v, 0. } }; }, points);

  // The 4 triangular faces.
  for (const auto& face : PyramidFaces)
  {
    face.Net(n, coefficients, faceCoefficients);
    const auto facePoints = FindBarycentricInteriorPoints<2>(n, faceCoefficients);
    if (!facePoints.empty())
    {
      BarycentricEmbed embed = [&face](const std::array<double, 3>& lambda)
      { return face.Embed(lambda); };
      AppendBarycentricInterior<2>(basisOp, gradOp, 3, coefficients, embed, facePoints, 2, points);
    }
  }

  // The 4 base edges, each a row or column of the base's own net - the
  // first (n + 1)^2 coefficients, laid out the same way.
  edgeCoefficients.resize(n + 1);
  for (int axis = 0; axis < 2; ++axis)
  {
    for (int end : { 0, n })
    {
      for (int p = 0; p <= n; ++p)
      {
        edgeCoefficients[p] =
          axis == 0 ? coefficients[end + p * (n + 1)] : coefficients[p + end * (n + 1)];
      }
      const double pinned = end == 0 ? -1. : 1.;
      auto embed = [axis, pinned](double u) -> std::array<double, 3>
      {
        return axis == 0 ? std::array<double, 3>{ { pinned, u, 0. } }
                         : std::array<double, 3>{ { u, pinned, 0. } };
      };
      SearchBarycentricEdge(basisOp, gradOp, 3, n, edgeCoefficients, coefficients, embed, points);
    }
  }

  // The 4 edges rising to the apex, one per corner of the base: each
  // layer's own corner, along t.
  for (int endA : { 0, 1 })
  {
    for (int endB : { 0, 1 })
    {
      int offset = 0; // Offset of layer k, advanced one layer at a time.
      for (int k = 0; k <= n; ++k)
      {
        const int width = n - k + 1;
        edgeCoefficients[k] = coefficients[offset + (endA + endB * width) * (width - 1)];
        offset += width * width;
      }
      const double signR = endA == 0 ? -1. : 1.;
      const double signS = endB == 0 ? -1. : 1.;
      auto embed = [signR, signS](double u) -> std::array<double, 3>
      {
        const double t = 0.5 * (u + 1.);
        const double w = 1. - t;
        return { { signR * w, signS * w, t } };
      };
      SearchBarycentricEdge(basisOp, gradOp, 3, n, edgeCoefficients, coefficients, embed, points);
    }
  }

  return points;
}

std::vector<CriticalPoint> FindPyramidCriticalPoints(vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  auto points = FindPyramidInteriorCriticalPoints(basisOp, gradOp, order, coefficients);
  auto boundaryPoints = FindPyramidBoundaryCriticalPoints(basisOp, gradOp, order, coefficients);
  points.insert(points.end(), boundaryPoints.begin(), boundaryPoints.end());
  return points;
}

std::vector<CriticalPoint> FindCriticalPoints(vtkDGCell::Shape shape, vtkDGOperatorEntry& basisOp,
  vtkDGOperatorEntry& gradOp, const std::vector<int>& order,
  const std::vector<double>& coefficients)
{
  switch (shape)
  {
    case vtkDGCell::Shape::Triangle:
    case vtkDGCell::Shape::Tetrahedron:
      return FindBarycentricCriticalPoints(basisOp, gradOp, order, coefficients);
    case vtkDGCell::Shape::Wedge:
      return FindWedgeCriticalPoints(basisOp, gradOp, order, coefficients);
    case vtkDGCell::Shape::Pyramid:
      return FindPyramidCriticalPoints(basisOp, gradOp, order, coefficients);
    case vtkDGCell::Shape::Vertex:
    case vtkDGCell::Shape::Edge:
    case vtkDGCell::Shape::Quadrilateral:
    case vtkDGCell::Shape::Hexahedron:
    default:
      return FindPrismaticCriticalPoints(basisOp, gradOp, order, coefficients);
  }
}

VTK_ABI_NAMESPACE_END
} // namespace bezier
} // namespace basis
} // namespace vtk
