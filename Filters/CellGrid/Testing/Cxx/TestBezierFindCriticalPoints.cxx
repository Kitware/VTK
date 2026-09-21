// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise vtk::basis::bezier's critical-point search directly - a single
// cell of every supported shape (edge, quadrilateral, hexahedron, triangle,
// tetrahedron, wedge, pyramid), at every Bernstein-Bezier order 1 through 5,
// and on every entity of that shape (its interior, and every face and edge
// FindPrismaticCriticalPoints()/FindBarycentricCriticalPoints()/
// FindWedgeCriticalPoints()/FindPyramidCriticalPoints() recurse into).
//
// The search takes two operators, an order and a control net, so that is all
// a test of it needs to build: no grid, no geometry, no connectivity. Which
// of the three entry points a shape goes to is the one piece of dispatch
// living outside the search - in vtkDGFindCriticalPointsResponder, which is
// what carries the answers back to a vtkCellGrid - and Search() below
// repeats it (and extends it to pyramids, which the responder does not yet
// dispatch).
//
// Every case uses the same construction: every control-point coefficient
// is 1 except one target index, set to -5. By the Bernstein/Bezier
// partition of unity, the field then reduces exactly to
//   field(x) = 1 + B_target(x) * (-5 - 1) = 1 - 6 * B_target(x),
// where B_target is that one control point's own basis function - so
// minimizing the field is exactly maximizing B_target(x). A single
// Bernstein basis function (tensor-product, axis by axis, or the
// barycentric multinomial form) has its unique global maximum exactly at
// its own Greville/peak point (x_j = i_j / n_j per tensor axis, or
// lambda = (i/n, j/n, k/n, ...) barycentric) - true for *any* interior
// lattice index, not only a symmetric/central one (the barycentric case is
// the AM-GM argument TestTriangleInteriorMinimum used to derive its own
// answer by hand, before this file generalized it; the tensor case is the
// same argument applied per axis, independently, since a tensor Bernstein
// basis factors into one 1-D factor per axis). This is an *exact* identity
// for any coefficient below 1, not merely a good approximation for a very
// negative one, so a single fixed dip value works at every order.
// PeakWeight()/TensorPeakWeight() below compute that basis function's own
// value at its own peak, giving the resulting critical point's exact value
// and location in closed form for every (shape, order, entity) case. A
// pyramid's basis function is a product of 1-D Bernstein polynomials in its
// collapsed coordinates (see RunPyramidDipTest()), so the tensor argument
// covers it too.

#include "vtkCellAttribute.h"
#include "vtkDGBezierFindCriticalPoints.h"
#include "vtkDGCell.h"
#include "vtkDGEdge.h"
#include "vtkDGHex.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDGPyr.h"
#include "vtkDGQuad.h"
#include "vtkDGTet.h"
#include "vtkDGTri.h"
#include "vtkDGWdg.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkStringToken.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
using namespace vtk::literals;

constexpr double tolerance = 1e-4;
constexpr double dipValue = -5.;

/// How large a directional derivative a reported critical point may still
/// have, relative to the net's own largest coefficient.
///
/// A reported point sits within about 1e-10 of a root, so what is measured
/// here is that distance times the field's own curvature - which grows with
/// order and dimension. The worst seen over the sweeps below is 6.1e-8, on
/// the order-5 quadrilaterals; this leaves over two orders of magnitude on
/// that, since the quantity is a difference of large basis-function sums and
/// need not round identically on every platform.
constexpr double criticalityTolerance = 1e-5;

// -----------------------------------------------------------------------
// Combinatorics / peak-weight / Greville-point helpers
// -----------------------------------------------------------------------

/// The value of the order-`n`, index-`i` 1-D Bernstein basis function at
/// its own unique peak u = i/n (0^0 = 1 for i in {0, n}, matching a
/// pinned/boundary control point's own basis function being 1 there).
double TensorPeakWeight(int n, int i)
{
  const double u = static_cast<double>(i) / n;
  const double a = (i == 0) ? 1. : std::pow(u, i);
  const double b = (i == n) ? 1. : std::pow(1. - u, n - i);
  return vtkMath::DoubleBinomial(n, i) * a * b;
}

/// This file's tensor-axis convention: index i of n maps to [-1, 1].
double TensorGreville(int n, int i)
{
  return -1. + 2. * static_cast<double>(i) / n;
}

/// The value of the order-`n` barycentric Bernstein basis function with
/// multi-index `idx` (summing to n) at its own unique peak
/// lambda = idx / n (0^0 = 1 for any zero entry, matching a pinned
/// edge/face/corner's own basis function value there).
double SimplexPeakWeight(const std::vector<int>& idx)
{
  int n = 0;
  for (int v : idx)
  {
    n += v;
  }
  double weight = 1.;
  int remaining = n;
  for (int v : idx)
  {
    weight *= vtkMath::DoubleBinomial(remaining, v);
    remaining -= v;
  }
  for (int v : idx)
  {
    if (v > 0)
    {
      weight *= std::pow(static_cast<double>(v) / n, v);
    }
  }
  return weight;
}

// -----------------------------------------------------------------------
// Flat-index helpers, matching vtkDGLagrangePoints' own DOF numbering
// (already documented in this file's existing Build* functions and in
// vtkDGBezierFindCriticalPoints.cxx): tensor axes r-fastest; a simplex's
// "j outer, i inner" (triangle) / "k outer, j middle, i inner"
// (tetrahedron) lattice; a wedge's triangle lattice repeated once per
// prismatic-axis point, that axis outermost.
// -----------------------------------------------------------------------

/// The number of points in a d-dimensional simplex lattice of order n -
/// one per multi-index summing to at most n, which is C(n + d, d).
int SimplexSize(int d, int n)
{
  return static_cast<int>(vtkMath::Binomial(n + d, d));
}

int TriSize(int n)
{
  return SimplexSize(2, n);
}

int TriFlat2D(int n, int i, int j)
{
  int flat = 0;
  for (int jj = 0; jj < j; ++jj)
  {
    flat += (n - jj + 1);
  }
  return flat + i;
}

int TetFlat3D(int n, int i, int j, int k)
{
  int flat = 0;
  for (int kk = 0; kk < k; ++kk)
  {
    flat += TriSize(n - kk);
  }
  return flat + TriFlat2D(n - k, i, j);
}

/// The flat index of a `d`-dimensional (triangle: d = 2, tetrahedron:
/// d = 3) barycentric multi-index `idx` (length d + 1, its *last* entry
/// always the implicit coordinate n - sum(others), per this file's own
/// convention - so only idx[0..d-1] are ever passed to TriFlat2D/TetFlat3D).
int SimplexFlatIndex(int n, const std::vector<int>& idx)
{
  const int d = static_cast<int>(idx.size()) - 1;
  return d == 2 ? TriFlat2D(n, idx[0], idx[1]) : TetFlat3D(n, idx[0], idx[1], idx[2]);
}

/// Build the target barycentric multi-index (length d + 1) for a
/// sub-simplex where the coordinates listed in `pinned` are forced to 0
/// and every other coordinate splits `order` as evenly as possible (each
/// getting at least 1) - any strictly-positive integer split works, not
/// only a perfectly symmetric one, per this file's own top-of-file
/// argument. Returns false (no interior lattice point at this order) when
/// `order` is too small for every free coordinate to get at least 1.
bool BuildSimplexTarget(int order, int d, const std::vector<int>& pinned, std::vector<int>& idx)
{
  idx.assign(d + 1, 0);
  std::vector<int> freeCoords;
  for (int c = 0; c <= d; ++c)
  {
    if (std::find(pinned.begin(), pinned.end(), c) == pinned.end())
    {
      freeCoords.push_back(c);
    }
  }
  const int numFree = static_cast<int>(freeCoords.size());
  if (numFree == 0 || order < numFree)
  {
    return false;
  }
  const int base = order / numFree;
  const int remainder = order % numFree;
  for (int fi = 0; fi < numFree; ++fi)
  {
    idx[freeCoords[fi]] = base + (fi < remainder ? 1 : 0);
  }
  return true;
}

// -----------------------------------------------------------------------
// The cell under test
// -----------------------------------------------------------------------

enum class Family
{
  Prismatic,   ///< Edge, quadrilateral, hexahedron: axes in [-1, 1].
  Barycentric, ///< Triangle, tetrahedron.
  Wedge,       ///< A barycentric cross-section crossed with a prismatic axis.
  Pyramid      ///< A square cross-section collapsing to the apex at t = 1.
};

/// Everything the search needs to know about one cell.
///
/// The search takes a basis operator, a gradient operator, an order and a
/// control net - no grid, no geometry, no connectivity - so a test for it
/// needs no more than this either. Which of the three entry points a shape
/// goes to is the one piece of dispatch that lives outside the search, in
/// vtkDGFindCriticalPointsResponder; Search() below is that dispatch.
struct Cell
{
  vtkSmartPointer<vtkDGCell> Instance;
  Family Kind = Family::Prismatic;
  int Dimension = 0;
  std::vector<int> Order;
  vtkDGOperatorEntry BasisOp;
  vtkDGOperatorEntry GradOp;
  /// The order array a wedge needs, kept alive for as long as the operators
  /// bound to it are.
  vtkSmartPointer<vtkIntArray> OrderArray;

  std::size_t NetSize() const { return static_cast<std::size_t>(this->BasisOp.NumberOfFunctions); }
};

vtkSmartPointer<vtkDGCell> NewCell(vtkStringToken shape)
{
  if (shape == "vtkDGEdge"_token)
  {
    return vtkSmartPointer<vtkDGEdge>::New();
  }
  if (shape == "vtkDGQuad"_token)
  {
    return vtkSmartPointer<vtkDGQuad>::New();
  }
  if (shape == "vtkDGHex"_token)
  {
    return vtkSmartPointer<vtkDGHex>::New();
  }
  if (shape == "vtkDGTri"_token)
  {
    return vtkSmartPointer<vtkDGTri>::New();
  }
  if (shape == "vtkDGTet"_token)
  {
    return vtkSmartPointer<vtkDGTet>::New();
  }
  if (shape == "vtkDGPyr"_token)
  {
    return vtkSmartPointer<vtkDGPyr>::New();
  }
  return vtkSmartPointer<vtkDGWdg>::New();
}

/// Bind a cell of the given \a shape to an arbitrary-order Bezier basis.
/// \a prismaticOrder is a wedge's independent axis; every other shape
/// takes a single \a order and ignores it.
bool MakeCell(vtkStringToken shape, int order, int prismaticOrder, Cell& cell)
{
  cell.Instance = NewCell(shape);
  cell.Dimension = cell.Instance->GetDimension();
  switch (cell.Instance->GetShape())
  {
    case vtkDGCell::Shape::Triangle:
    case vtkDGCell::Shape::Tetrahedron:
      cell.Kind = Family::Barycentric;
      break;
    case vtkDGCell::Shape::Wedge:
      cell.Kind = Family::Wedge;
      break;
    case vtkDGCell::Shape::Pyramid:
      cell.Kind = Family::Pyramid;
      break;
    default:
      cell.Kind = Family::Prismatic;
      break;
  }

  vtkCellAttribute::CellTypeInfo info;
  info.FunctionSpace = "Bezier"_token;
  info.Basis = "A"_token;
  if (cell.Kind == Family::Wedge)
  {
    cell.OrderArray = vtkSmartPointer<vtkIntArray>::New();
    cell.OrderArray->SetName("order");
    cell.OrderArray->SetNumberOfComponents(3);
    cell.OrderArray->SetNumberOfTuples(1);
    const std::array<int, 3> triple{ { order, order, prismaticOrder } };
    cell.OrderArray->SetTypedTuple(0, triple.data());
    info.ArraysByRole["order"_token] = cell.OrderArray;
  }
  else
  {
    info.Order = order;
  }

  cell.BasisOp = cell.Instance->GetOperatorEntry("Basis"_token, info);
  cell.GradOp = cell.Instance->GetOperatorEntry("BasisGradient"_token, info);
  cell.Order = cell.Instance->GetBasisOrder(info);
  if (!cell.BasisOp || !cell.GradOp || cell.Order.empty())
  {
    std::cerr << "  ERROR: no arbitrary-order Bezier basis for " << shape.Data() << " at order "
              << order << ".\n";
    return false;
  }
  return true;
}

/// Locate every critical point of \a coefficients on \a cell, through
/// whichever entry point its parameter space calls for.
std::vector<vtk::basis::bezier::CriticalPoint> Search(
  Cell& cell, const std::vector<double>& coefficients)
{
  switch (cell.Kind)
  {
    case Family::Wedge:
      return vtk::basis::bezier::FindWedgeCriticalPoints(
        cell.BasisOp, cell.GradOp, cell.Order, coefficients);
    case Family::Pyramid:
      return vtk::basis::bezier::FindPyramidCriticalPoints(
        cell.BasisOp, cell.GradOp, cell.Order, coefficients);
    case Family::Barycentric:
      return vtk::basis::bezier::FindBarycentricCriticalPoints(
        cell.BasisOp, cell.GradOp, cell.Order, coefficients);
    default:
      return vtk::basis::bezier::FindPrismaticCriticalPoints(
        cell.BasisOp, cell.GradOp, cell.Order, coefficients);
  }
}

// -----------------------------------------------------------------------
// An independent root solve, used as a (fallible) ground truth
// -----------------------------------------------------------------------

/// How close to a boundary a coordinate must be to count as pinned there.
///
/// Not a fuzzy threshold in practice: an entity's own search reports its
/// points through an embedding that sets the pinned coordinates exactly, so
/// these comparisons are against exact 0 and +/-1.
constexpr double kPinned = 1e-9;

/// A basis of the directions that stay inside the entity \a point was found
/// on, in the cell's own parametric coordinates.
///
/// Which entity that is need not be passed in: it is what the pinned
/// coordinates of \a point say it is. A prismatic axis is pinned where it
/// reaches +/-1, a barycentric coordinate where it reaches 0, and the free
/// ones span the entity. The returned count is therefore the entity's own
/// dimension, which is worth checking against what the search reported.
std::vector<std::array<double, 3>> EntityTangents(
  Family family, int dimension, const std::array<double, 3>& point)
{
  std::vector<std::array<double, 3>> tangents;

  // A pyramid's faces are not coordinate planes: the base is t = 0 and the
  // triangular faces are r = -/+(1 - t) and s = -/+(1 - t). The directions
  // left free are whatever the normals of those it lies on leave, found by
  // Gram-Schmidt: the normals first, then whichever axes survive them.
  if (family == Family::Pyramid)
  {
    const double w = 1. - point[2];
    // Never more than 3 independent directions in 3-D, so a fixed array.
    std::array<std::array<double, 3>, 3> basis;
    int count = 0;
    auto absorb = [&basis, &count](std::array<double, 3> direction)
    {
      for (int b = 0; b < count; ++b)
      {
        const auto& prior = basis[b];
        const double along = vtkMath::Dot(direction.data(), prior.data());
        for (int axis = 0; axis < 3; ++axis)
        {
          direction[axis] -= along * prior[axis];
        }
      }
      const double length = vtkMath::Norm(direction.data());
      if (length < 1e-6)
      {
        return false;
      }
      for (double& component : direction)
      {
        component /= length;
      }
      basis[count++] = direction;
      return true;
    };
    if (std::abs(point[2]) < kPinned)
    {
      absorb({ { 0., 0., 1. } });
    }
    if (std::abs(point[0] + w) < kPinned)
    {
      absorb({ { 1., 0., -1. } });
    }
    if (std::abs(point[0] - w) < kPinned)
    {
      absorb({ { 1., 0., 1. } });
    }
    if (std::abs(point[1] + w) < kPinned)
    {
      absorb({ { 0., 1., -1. } });
    }
    if (std::abs(point[1] - w) < kPinned)
    {
      absorb({ { 0., 1., 1. } });
    }
    for (int axis = 0; axis < 3; ++axis)
    {
      std::array<double, 3> direction{ { 0., 0., 0. } };
      direction[axis] = 1.;
      if (absorb(direction))
      {
        tangents.push_back(basis[count - 1]);
      }
    }
    return tangents;
  }

  // The prismatic axes, if any: each free one contributes itself.
  int prismaticAxes = 0;
  if (family == Family::Prismatic)
  {
    prismaticAxes = dimension;
  }
  else if (family == Family::Wedge)
  {
    prismaticAxes = 1;
  }
  const int prismaticFirst = (family == Family::Wedge) ? 2 : 0;
  for (int axis = 0; axis < prismaticAxes; ++axis)
  {
    if (std::abs(point[prismaticFirst + axis]) < 1. - kPinned)
    {
      std::array<double, 3> direction{ { 0., 0., 0. } };
      direction[prismaticFirst + axis] = 1.;
      tangents.push_back(direction);
    }
  }

  // The barycentric coordinates, if any. Coordinate 0 is the implicit one,
  // 1 - sum of the rest; a direction within the entity moves weight from one
  // free coordinate to another, so the free ones minus one span it.
  int barycentricCount = 0;
  if (family == Family::Barycentric)
  {
    barycentricCount = dimension + 1;
  }
  else if (family == Family::Wedge)
  {
    barycentricCount = 3;
  }
  if (barycentricCount > 0)
  {
    double implicitCoordinate = 1.;
    for (int k = 1; k < barycentricCount; ++k)
    {
      implicitCoordinate -= point[k - 1];
    }
    std::vector<int> free;
    for (int k = 0; k < barycentricCount; ++k)
    {
      const double coordinate = (k == 0) ? implicitCoordinate : point[k - 1];
      if (coordinate > kPinned)
      {
        free.push_back(k);
      }
    }
    // Weight moved from free[0] into each of the others: raising lambda_k
    // for k > 0 raises that parameter, and raising the implicit lambda_0
    // means lowering the others, which is why it appears with a sign.
    for (std::size_t i = 1; i < free.size(); ++i)
    {
      std::array<double, 3> direction{ { 0., 0., 0. } };
      if (free[i] > 0)
      {
        direction[free[i] - 1] += 1.;
      }
      if (free[0] > 0)
      {
        direction[free[0] - 1] -= 1.;
      }
      tangents.push_back(direction);
    }
  }

  return tangents;
}

/// Whether \a point is at least \a margin inside the cell's own parameter
/// domain.
bool InsideCell(Family family, int dimension, const std::array<double, 3>& point, double margin)
{
  switch (family)
  {
    case Family::Prismatic:
      for (int axis = 0; axis < dimension; ++axis)
      {
        if (std::abs(point[axis]) > 1. - margin)
        {
          return false;
        }
      }
      return true;
    case Family::Barycentric:
    {
      double sum = 0.;
      for (int axis = 0; axis < dimension; ++axis)
      {
        if (point[axis] < margin)
        {
          return false;
        }
        sum += point[axis];
      }
      return 1. - sum >= margin;
    }
    case Family::Wedge:
      if (point[0] < margin || point[1] < margin || 1. - point[0] - point[1] < margin)
      {
        return false;
      }
      return std::abs(point[2]) <= 1. - margin;
    case Family::Pyramid:
    {
      const double w = 1. - point[2];
      return point[2] >= margin && w >= margin && std::abs(point[0]) <= w - margin &&
        std::abs(point[1]) <= w - margin;
    }
  }
  return false;
}

/// The field's gradient, evaluated with one reusable buffer and without the
/// basis values it does not need.
///
/// Newton wants the gradient and nothing else, and wants it a great many
/// times - once per Hessian column per iteration per starting point - so
/// the public EvaluateField() is the wrong tool for it: that evaluates the
/// basis as well as its gradient, and allocates two vectors on every call.
/// This still goes through VTK's own gradient operator, which is what makes
/// it an independent check of the search.
class FieldGradient
{
public:
  FieldGradient(
    const vtkDGOperatorEntry& gradOp, int dimension, const std::vector<double>& coefficients)
    : GradOp(gradOp)
    , Dimension(dimension)
    , Coefficients(&coefficients)
    , Values(static_cast<std::size_t>(gradOp.NumberOfFunctions) * 3, 0.)
  {
  }

  /// Write the gradient at \a point into \a gradient, returning its largest
  /// component.
  double At(const std::array<double, 3>& point, std::array<double, 3>& gradient)
  {
    this->GradOp.Evaluate(point, this->Values);
    gradient = { { 0., 0., 0. } };
    const int functions = this->GradOp.NumberOfFunctions;
    double worst = 0.;
    for (int axis = 0; axis < this->Dimension; ++axis)
    {
      double slope = 0.;
      for (int function = 0; function < functions; ++function)
      {
        slope += (*this->Coefficients)[function] * this->Values[function * 3 + axis];
      }
      gradient[axis] = slope;
      worst = std::max(worst, std::abs(slope));
    }
    return worst;
  }

private:
  vtkDGOperatorEntry GradOp;
  int Dimension;
  const std::vector<double>* Coefficients;
  std::vector<double> Values;
};

/// Solve A x = b (b in \a x) by Gaussian elimination with partial pivoting,
/// returning false if A is singular.
///
/// Deliberately not vtkMath::SolveLinearSystem(), which is a perfectly good
/// solver but not the same one. On the degenerate fields the closed-form
/// cases above are built from, the gradient decays into a whole face rather
/// than dropping to zero at it, and Newton wanders in the sliver beside one;
/// where it comes to rest there depends on the exact arithmetic of the step.
/// Keeping the step's solver here keeps that behaviour a property of this
/// file rather than of a shared one - see the note on ReferenceRoots() for
/// why the reference is allowed to be imprecise there, and IsolatedRoot()
/// for what discards the results when it is.
bool SolveStep(double** matrix, double* rhs, int size)
{
  for (int column = 0; column < size; ++column)
  {
    int pivot = column;
    for (int row = column + 1; row < size; ++row)
    {
      if (std::abs(matrix[row][column]) > std::abs(matrix[pivot][column]))
      {
        pivot = row;
      }
    }
    if (std::abs(matrix[pivot][column]) < 1e-14)
    {
      return false;
    }
    if (pivot != column)
    {
      for (int entry = 0; entry < size; ++entry)
      {
        std::swap(matrix[column][entry], matrix[pivot][entry]);
      }
      std::swap(rhs[column], rhs[pivot]);
    }
    for (int row = column + 1; row < size; ++row)
    {
      const double factor = matrix[row][column] / matrix[column][column];
      for (int entry = column; entry < size; ++entry)
      {
        matrix[row][entry] -= factor * matrix[column][entry];
      }
      rhs[row] -= factor * rhs[column];
    }
  }
  for (int row = size - 1; row >= 0; --row)
  {
    for (int entry = row + 1; entry < size; ++entry)
    {
      rhs[row] -= matrix[row][entry] * rhs[entry];
    }
    rhs[row] /= matrix[row][row];
  }
  return true;
}

/// Damped Newton on grad(f) = 0 from \a point, which it moves. The Hessian
/// comes from central differences of the gradient; the step is halved until
/// it neither leaves the cell nor increases the residual.
void NewtonToRoot(FieldGradient& field, Family family, int dimension, std::array<double, 3>& point)
{
  constexpr double step = 1e-6;
  std::array<double, 3> gradient;
  std::array<double, 3> ahead;
  std::array<double, 3> behind;
  for (int iteration = 0; iteration < 120; ++iteration)
  {
    const double residual = field.At(point, gradient);

    // The Newton step solves H dx = -grad, with the Hessian by central
    // differences of the gradient.
    double hessian[3][3] = { { 0. } };
    double* rows[3] = { hessian[0], hessian[1], hessian[2] };
    std::array<double, 3> direction{ { 0., 0., 0. } };
    for (int column = 0; column < dimension; ++column)
    {
      std::array<double, 3> shifted = point;
      shifted[column] += step;
      field.At(shifted, ahead);
      shifted[column] -= 2. * step;
      field.At(shifted, behind);
      for (int row = 0; row < dimension; ++row)
      {
        hessian[row][column] = (ahead[row] - behind[row]) / (2. * step);
      }
    }
    for (int row = 0; row < dimension; ++row)
    {
      direction[row] = -gradient[row];
    }
    if (!SolveStep(rows, direction.data(), dimension))
    {
      return; // Singular: no Newton direction to follow from here.
    }

    double scale = 1.;
    std::array<double, 3> next = point;
    bool moved = false;
    for (int halving = 0; halving < 40; ++halving)
    {
      next = point;
      for (int axis = 0; axis < dimension; ++axis)
      {
        next[axis] = point[axis] + scale * direction[axis];
      }
      if (InsideCell(family, dimension, next, 0.) &&
        field.At(next, ahead) <= residual * (1. + 1e-12))
      {
        moved = true;
        break;
      }
      scale *= 0.5;
    }
    if (!moved)
    {
      return;
    }
    double shift = 0.;
    for (int axis = 0; axis < dimension; ++axis)
    {
      shift = std::max(shift, std::abs(next[axis] - point[axis]));
    }
    point = next;
    if (shift < 1e-15)
    {
      return;
    }
  }
}

/// Whether the critical point at \a point is isolated, judged by its
/// Hessian being non-singular there.
///
/// This is what keeps the reference honest on the degenerate fields the
/// closed-form cases above are built from. A net that is flat except at one
/// coefficient routinely has a gradient vanishing along a whole edge or
/// face, and every lattice point that starts on one is already a solution of
/// grad(f) = 0, so Newton stops where it started and the reference would
/// report a line of "roots" - none of them isolated, and none of them the
/// search's job to enumerate one at a time. Along such a set the gradient
/// does not change either, so the Hessian is singular in that direction,
/// which is exactly what this rejects.
///
/// It also rejects a genuinely degenerate isolated critical point - r^3 on
/// an edge, whose second derivative vanishes too. The search does report
/// those, so all that costs is coverage, which is the direction this
/// reference is allowed to be wrong in.
bool IsolatedRoot(
  FieldGradient& field, int dimension, const std::array<double, 3>& point, double scale)
{
  constexpr double step = 1e-6;
  double hessian[3][3] = { { 0. } };
  double largest = 0.;
  std::array<double, 3> ahead;
  std::array<double, 3> behind;
  for (int column = 0; column < dimension; ++column)
  {
    std::array<double, 3> shifted = point;
    shifted[column] += step;
    field.At(shifted, ahead);
    shifted[column] -= 2. * step;
    field.At(shifted, behind);
    for (int row = 0; row < dimension; ++row)
    {
      hessian[row][column] = (ahead[row] - behind[row]) / (2. * step);
      largest = std::max(largest, std::abs(hessian[row][column]));
    }
  }
  if (largest <= 1e-9 * scale)
  {
    return false; // Flat in every direction: not one point but a region.
  }

  double determinant = 0.;
  switch (dimension)
  {
    case 1:
      determinant = hessian[0][0];
      break;
    case 2:
      determinant =
        vtkMath::Determinant2x2(hessian[0][0], hessian[0][1], hessian[1][0], hessian[1][1]);
      break;
    default:
      determinant = vtkMath::Determinant3x3(hessian);
      break;
  }
  double reference = 1.;
  for (int axis = 0; axis < dimension; ++axis)
  {
    reference *= largest;
  }
  return std::abs(determinant) > 1e-8 * reference;
}

/// Critical points found by running NewtonToRoot() from a lattice of
/// starting points, keeping only those whose gradient really does vanish
/// and that sit \a margin inside the cell.
///
/// This is a cross-check, not an oracle, and a pass proves nothing on its
/// own: Newton only reaches a root whose basin of attraction some starting
/// point happens to land in, so this can and does miss roots the search
/// reports - measured at about 0.6% of them on order-5 hexahedra at this
/// lattice density. It is useful because it fails differently from the
/// thing it is checking: a subdivision solver that prunes a region it
/// should have kept and a descent method seeded away from a basin have
/// nothing in common, so a root that both agree on is good evidence, and a
/// root only one of them has is always worth reading.
///
/// It is therefore asserted in one direction only - everything found here
/// must be reported by the search - and never the reverse.
///
/// The lattice is deliberately no denser than it is. Raising it to 9 and 22
/// steps costs 2.7 times as much and finds 5 more roots out of 798 - all of
/// them on the two order-4 cases - and doubling it again from there was
/// measured to close one further gap on order-5 hexahedra and nothing else,
/// at 7.6 times the cost, with a third doubling changing nothing at all.
/// Since only one direction is asserted, a sparser lattice costs coverage
/// and never correctness, which is what makes this a free trade to make.
std::vector<std::array<double, 3>> ReferenceRoots(
  FieldGradient& field, Family family, int dimension, double scale, double margin)
{
  std::vector<std::array<double, 3>> roots;
  const int steps = (dimension == 3) ? 6 : 13;
  int total = 1;
  for (int axis = 0; axis < dimension; ++axis)
  {
    total *= steps + 1;
  }
  std::array<double, 3> gradient;
  for (int seed = 0; seed < total; ++seed)
  {
    int remaining = seed;
    std::array<double, 3> point{ { 0., 0., 0. } };
    for (int axis = 0; axis < dimension; ++axis)
    {
      const double unit = static_cast<double>(remaining % (steps + 1)) / steps;
      remaining /= steps + 1;
      const bool prismaticAxis = (family == Family::Prismatic) ||
        (family == Family::Wedge && axis == 2) || (family == Family::Pyramid && axis < 2);
      point[axis] = prismaticAxis ? (2. * unit - 1.) : unit;
    }
    if (family == Family::Pyramid)
    {
      // Seeded over the collapsed coordinates rather than the bounding box,
      // so the lattice shrinks with the cross-section instead of mostly
      // falling outside it.
      point[0] *= 1. - point[2];
      point[1] *= 1. - point[2];
    }
    if (!InsideCell(family, dimension, point, 1e-3))
    {
      continue;
    }
    NewtonToRoot(field, family, dimension, point);
    if (field.At(point, gradient) > 1e-9 * scale || !InsideCell(family, dimension, point, margin) ||
      !IsolatedRoot(field, dimension, point, scale))
    {
      continue;
    }
    bool seen = false;
    for (const auto& prior : roots)
    {
      double apart = 0.;
      for (int axis = 0; axis < dimension; ++axis)
      {
        apart = std::max(apart, std::abs(prior[axis] - point[axis]));
      }
      if (apart < 1e-5)
      {
        seen = true;
        break;
      }
    }
    if (!seen)
    {
      roots.push_back(point);
    }
  }
  return roots;
}

/// Assert that every critical point an independent Newton solve can find in
/// this cell's interior is among \a points.
///
/// Everything the solve needs is already on the grid the test built - the
/// shape, the order, the control net and the basis operators - so every
/// check in this file can afford to ask for it without threading extra
/// arguments through.
///
/// Interior points only: a critical point confined to a face or an edge
/// does not have a vanishing gradient in the directions leaving it, so it
/// is not this reference's to find. Those are covered instead by the
/// closed-form expectations above and, for random nets, by the tangential
/// gradient check in RunRandomCase().
bool CrossCheckAgainstReference(Cell& cell, const std::vector<double>& coefficients,
  const std::vector<vtk::basis::bezier::CriticalPoint>& points, int* rootsChecked = nullptr)
{
  const int dimension = cell.Dimension;
  double scale = 1.;
  for (double coefficient : coefficients)
  {
    scale = std::max(scale, std::abs(coefficient));
  }

  constexpr double referenceMargin = 2e-2;
  FieldGradient field(cell.GradOp, dimension, coefficients);
  const auto reference = ReferenceRoots(field, cell.Kind, dimension, scale, referenceMargin);
  if (rootsChecked)
  {
    *rootsChecked += static_cast<int>(reference.size());
  }

  bool ok = true;
  for (const auto& root : reference)
  {
    bool reported = false;
    for (const auto& point : points)
    {
      double apart = 0.;
      for (int axis = 0; axis < dimension; ++axis)
      {
        apart = std::max(apart, std::abs(point.Parameter[axis] - root[axis]));
      }
      if (apart < 5e-3)
      {
        reported = true;
        break;
      }
    }
    if (!reported)
    {
      std::cerr << "  ERROR: an independent Newton solve finds a critical point at (" << root[0]
                << ", " << root[1] << ", " << root[2] << ") that the search does not report.\n";
      ok = false;
    }
  }
  return ok;
}

// -----------------------------------------------------------------------
// Generic checkers and drivers
// -----------------------------------------------------------------------

enum class Outcome
{
  NotApplicable, // this entity has no interior lattice point at this order
  Passed,
  Failed
};

bool CheckCriticalPoint(Cell& cell, const std::vector<double>& coefficients,
  const std::array<double, 3>& expectedParameter, double expectedValue, int expectedDimension,
  const std::string& label)
{
  std::cout << label << ":\n";
  const auto points = Search(cell, coefficients);

  std::cout << "  found " << points.size() << " critical point(s)\n";
  if (points.size() != 1)
  {
    std::cerr << "  ERROR: expected exactly 1 critical point, found " << points.size() << ".\n";
    return false;
  }

  const auto& criticalPoint = points[0];
  std::cout << "  point = (" << criticalPoint.Parameter[0] << ", " << criticalPoint.Parameter[1]
            << ", " << criticalPoint.Parameter[2] << "), value = " << criticalPoint.Value
            << ", dimension = " << criticalPoint.EntityDimension << "\n";

  bool ok = true;
  if (criticalPoint.EntityDimension != expectedDimension)
  {
    std::cerr << "  ERROR: expected dimension " << expectedDimension << ", got "
              << criticalPoint.EntityDimension << ".\n";
    ok = false;
  }
  for (int axis = 0; axis < 3; ++axis)
  {
    if (std::abs(criticalPoint.Parameter[axis] - expectedParameter[axis]) > tolerance)
    {
      std::cerr << "  ERROR: expected parameter axis " << axis << " to be "
                << expectedParameter[axis] << ", got " << criticalPoint.Parameter[axis] << ".\n";
      ok = false;
    }
  }
  if (std::abs(criticalPoint.Value - expectedValue) > tolerance)
  {
    std::cerr << "  ERROR: expected value " << expectedValue << ", got " << criticalPoint.Value
              << ".\n";
    ok = false;
  }
  int crossChecked = 0;
  ok &= CrossCheckAgainstReference(cell, coefficients, points, &crossChecked);
  std::cout << "  cross-checked against " << crossChecked << " independently found root(s)\n";
  return ok;
}

bool CheckNoCriticalPoints(
  Cell& cell, const std::vector<double>& coefficients, const std::string& label)
{
  std::cout << label << ":\n";
  const auto points = Search(cell, coefficients);
  std::cout << "  found " << points.size() << " critical point(s)\n";
  if (!points.empty())
  {
    std::cerr << "  ERROR: expected no critical points, found " << points.size() << ".\n";
    return false;
  }
  // "None reported" is exactly the claim an independent solve is best placed
  // to contradict, so this case wants the cross-check most of all.
  int crossChecked = 0;
  const bool ok = CrossCheckAgainstReference(cell, coefficients, points, &crossChecked);
  std::cout << "  cross-checked against " << crossChecked << " independently found root(s)\n";
  return ok;
}

/// One tensor-domain entity (an edge's interior, or a quad/hex's interior,
/// face, or edge): `pinnedAxis[a]` is -1 if parent axis `a` is free, else
/// 0 or `order` (the boundary index it is pinned to). Every free axis
/// needs `order >= 2` to have an interior lattice index at all.
Outcome RunTensorDipTest(
  Cell& cell, int order, const std::vector<int>& pinnedAxis, const std::string& label)
{
  const int fullDim = static_cast<int>(pinnedAxis.size());
  int numFree = 0;
  for (int a = 0; a < fullDim; ++a)
  {
    if (pinnedAxis[a] < 0)
    {
      ++numFree;
    }
  }
  if (numFree == 0 || order < 2)
  {
    return Outcome::NotApplicable;
  }

  std::vector<int> idx(fullDim);
  for (int a = 0; a < fullDim; ++a)
  {
    idx[a] = pinnedAxis[a] >= 0 ? pinnedAxis[a] : order / 2;
  }

  std::vector<int> stride(fullDim);
  stride[0] = 1;
  for (int a = 1; a < fullDim; ++a)
  {
    stride[a] = stride[a - 1] * (order + 1);
  }
  const int total = stride[fullDim - 1] * (order + 1);
  int flat = 0;
  double weight = 1.;
  std::array<double, 3> location{ { 0., 0., 0. } };
  for (int a = 0; a < fullDim; ++a)
  {
    flat += idx[a] * stride[a];
    weight *= TensorPeakWeight(order, idx[a]);
    location[a] = TensorGreville(order, idx[a]);
  }

  std::vector<double> coefficients(total, 1.);
  coefficients[flat] = dipValue;
  const double value = 1. + weight * (dipValue - 1.);

  return CheckCriticalPoint(cell, coefficients, location, value, numFree, label) ? Outcome::Passed
                                                                                 : Outcome::Failed;
}

/// One barycentric-domain entity (a triangle/tetrahedron's interior, or
/// one of its faces or edges): `pinned` lists which of the shape's
/// `d + 1` barycentric coordinates are forced to 0 for this entity
/// (empty = interior, one entry = a face, two entries = an edge for a
/// tetrahedron / one entry = an edge for a triangle).
Outcome RunSimplexDipTest(
  Cell& cell, int order, int d, const std::vector<int>& pinned, const std::string& label)
{
  std::vector<int> idx;
  if (!BuildSimplexTarget(order, d, pinned, idx))
  {
    return Outcome::NotApplicable;
  }

  const int numFree = d + 1 - static_cast<int>(pinned.size());
  const int flat = SimplexFlatIndex(order, idx);
  const double weight = SimplexPeakWeight(idx);
  const double value = 1. + weight * (dipValue - 1.);
  std::array<double, 3> location{ { 0., 0., 0. } };
  for (int a = 0; a < d; ++a)
  {
    location[a] = static_cast<double>(idx[a]) / order;
  }

  std::vector<double> coefficients(SimplexSize(d, order), 1.);
  coefficients[flat] = dipValue;

  return CheckCriticalPoint(cell, coefficients, location, value, numFree - 1, label)
    ? Outcome::Passed
    : Outcome::Failed;
}

/// One wedge entity: `triPinned` lists which of the triangle's 3
/// barycentric coordinates are forced to 0 (empty = triangle interior, one
/// entry = a triangle edge, two entries = a triangle corner); `prismatic`
/// is -1 if the prismatic axis is free, else 0 or `order` (pinned to a
/// cap). Both `n` (triangle order) and `m` (prismatic order) are tied to
/// the same swept `order`.
Outcome RunWedgeDipTest(
  Cell& cell, int order, const std::vector<int>& triPinned, int prismatic, const std::string& label)
{
  std::vector<int> triIdx;
  if (!BuildSimplexTarget(order, 2, triPinned, triIdx))
  {
    return Outcome::NotApplicable;
  }
  const bool prismaticFree = prismatic < 0;
  if (prismaticFree && order < 2)
  {
    return Outcome::NotApplicable;
  }
  const int kPrismatic = prismaticFree ? order / 2 : prismatic;

  const int triNumFree = 3 - static_cast<int>(triPinned.size());
  const int dimension = (triNumFree - 1) + (prismaticFree ? 1 : 0);
  const int triSize = TriSize(order);
  const int flat = kPrismatic * triSize + SimplexFlatIndex(order, triIdx);
  const double weight = SimplexPeakWeight(triIdx) * TensorPeakWeight(order, kPrismatic);
  const double value = 1. + weight * (dipValue - 1.);
  const std::array<double, 3> location{ { static_cast<double>(triIdx[0]) / order,
    static_cast<double>(triIdx[1]) / order, TensorGreville(order, kPrismatic) } };

  std::vector<double> coefficients(triSize * (order + 1), 1.);
  coefficients[flat] = dipValue;

  return CheckCriticalPoint(cell, coefficients, location, value, dimension, label)
    ? Outcome::Passed
    : Outcome::Failed;
}

/// One pyramid entity, named by the lattice index (i, j, k) whose
/// coefficient dips: k the layer along t, (i, j) within that layer's own
/// (order - k + 1)^2 grid. Which entity it lands on is up to the caller -
/// i or j at 0 or order - k puts it on a triangular face, k = 0 on the base.
///
/// The basis function B_i^(n-k)(a) B_j^(n-k)(b) B_k^n(c) of Chan & Warburton
/// is a product of 1-D Bernstein polynomials in the collapsed coordinates
/// a = (r + w) / (2w), b = (s + w) / (2w), c = t (w = 1 - t), so it peaks at
/// a = i / (n - k), b = j / (n - k), c = k / n, by the same per-axis
/// argument as a tensor cell's, and that is its only critical point on the
/// pyramid since the collapsed map is one to one away from the apex. Needs
/// k < order, or the dip is the apex, a corner.
Outcome RunPyramidDipTest(
  Cell& cell, int order, int i, int j, int k, int dimension, const std::string& label)
{
  const int m = order - k;
  std::vector<double> coefficients(cell.NetSize(), 1.);
  int flat = 0;
  for (int layer = 0; layer < k; ++layer)
  {
    flat += (order - layer + 1) * (order - layer + 1);
  }
  flat += i + j * (m + 1);
  coefficients[flat] = dipValue;

  const double weight =
    TensorPeakWeight(m, i) * TensorPeakWeight(m, j) * TensorPeakWeight(order, k);
  const double value = 1. + weight * (dipValue - 1.);
  const double t = static_cast<double>(k) / order;
  const double w = 1. - t;
  const std::array<double, 3> location{ { TensorGreville(m, i) * w, TensorGreville(m, j) * w, t } };

  return CheckCriticalPoint(cell, coefficients, location, value, dimension, label)
    ? Outcome::Passed
    : Outcome::Failed;
}

/// Fold one entity's Outcome into the running (ok, anyApplicable) state.
void FoldOutcome(Outcome outcome, bool& ok, bool& anyApplicable)
{
  if (outcome == Outcome::Failed)
  {
    ok = false;
  }
  if (outcome != Outcome::NotApplicable)
  {
    anyApplicable = true;
  }
}

// -----------------------------------------------------------------------
// Per-shape order sweeps
// -----------------------------------------------------------------------

bool TestEdgeOrders()
{
  bool ok = true;
  for (int order = 1; order <= 5; ++order)
  {
    Cell cell;
    if (!MakeCell("vtkDGEdge"_token, order, 0, cell))
    {
      ok = false;
      continue;
    }
    const std::string tag = "TestEdgeOrders order=" + vtk::to_string(order);
    bool anyApplicable = false;
    FoldOutcome(RunTensorDipTest(cell, order, { -1 }, tag + " interior"), ok, anyApplicable);

    if (!anyApplicable)
    {
      const std::vector<double> flat(cell.NetSize(), 1.);
      if (!CheckNoCriticalPoints(cell, flat, tag + " (no interior)"))
      {
        ok = false;
      }
    }
  }
  return ok;
}

bool TestQuadOrders()
{
  bool ok = true;
  for (int order = 1; order <= 5; ++order)
  {
    Cell cell;
    if (!MakeCell("vtkDGQuad"_token, order, 0, cell))
    {
      ok = false;
      continue;
    }
    const std::string tag = "TestQuadOrders order=" + vtk::to_string(order);
    bool anyApplicable = false;
    FoldOutcome(RunTensorDipTest(cell, order, { -1, -1 }, tag + " interior"), ok, anyApplicable);
    FoldOutcome(RunTensorDipTest(cell, order, { 0, -1 }, tag + " edge r=-1"), ok, anyApplicable);
    FoldOutcome(
      RunTensorDipTest(cell, order, { order, -1 }, tag + " edge r=+1"), ok, anyApplicable);
    FoldOutcome(RunTensorDipTest(cell, order, { -1, 0 }, tag + " edge s=-1"), ok, anyApplicable);
    FoldOutcome(
      RunTensorDipTest(cell, order, { -1, order }, tag + " edge s=+1"), ok, anyApplicable);

    if (!anyApplicable)
    {
      const std::vector<double> flat(cell.NetSize(), 1.);
      if (!CheckNoCriticalPoints(cell, flat, tag + " (no interior)"))
      {
        ok = false;
      }
    }
  }
  return ok;
}

bool TestHexOrders()
{
  bool ok = true;
  for (int order = 1; order <= 5; ++order)
  {
    Cell cell;
    if (!MakeCell("vtkDGHex"_token, order, 0, cell))
    {
      ok = false;
      continue;
    }
    const std::string tag = "TestHexOrders order=" + vtk::to_string(order);
    bool anyApplicable = false;

    FoldOutcome(
      RunTensorDipTest(cell, order, { -1, -1, -1 }, tag + " interior"), ok, anyApplicable);

    const std::array<const char*, 3> axisName{ { "r", "s", "t" } };
    for (int pinnedAxis = 0; pinnedAxis < 3; ++pinnedAxis)
    {
      for (int end : { 0, order })
      {
        std::vector<int> pins(3, -1);
        pins[pinnedAxis] = end;
        const std::string label =
          tag + " face " + axisName[pinnedAxis] + (end == 0 ? "=-1" : "=+1");
        FoldOutcome(RunTensorDipTest(cell, order, pins, label), ok, anyApplicable);
      }
    }

    for (int freeAxis = 0; freeAxis < 3; ++freeAxis)
    {
      const int otherA = (freeAxis + 1) % 3;
      const int otherB = (freeAxis + 2) % 3;
      for (int endA : { 0, order })
      {
        for (int endB : { 0, order })
        {
          std::vector<int> pins(3, -1);
          pins[otherA] = endA;
          pins[otherB] = endB;
          const std::string label = tag + " edge free=" + axisName[freeAxis] + " " +
            axisName[otherA] + (endA == 0 ? "=-1" : "=+1") + " " + axisName[otherB] +
            (endB == 0 ? "=-1" : "=+1");
          FoldOutcome(RunTensorDipTest(cell, order, pins, label), ok, anyApplicable);
        }
      }
    }

    if (!anyApplicable)
    {
      const std::vector<double> flat(cell.NetSize(), 1.);
      if (!CheckNoCriticalPoints(cell, flat, tag + " (no interior)"))
      {
        ok = false;
      }
    }
  }
  return ok;
}

bool TestTriangleOrders()
{
  bool ok = true;
  for (int order = 1; order <= 5; ++order)
  {
    Cell cell;
    if (!MakeCell("vtkDGTri"_token, order, 0, cell))
    {
      ok = false;
      continue;
    }
    const std::string tag = "TestTriangleOrders order=" + vtk::to_string(order);
    bool anyApplicable = false;

    FoldOutcome(RunSimplexDipTest(cell, order, 2, {}, tag + " interior"), ok, anyApplicable);
    for (int pin = 0; pin < 3; ++pin)
    {
      FoldOutcome(RunSimplexDipTest(
                    cell, order, 2, { pin }, tag + " edge lambda" + vtk::to_string(pin) + "=0"),
        ok, anyApplicable);
    }

    if (!anyApplicable)
    {
      const std::vector<double> flat(cell.NetSize(), 1.);
      if (!CheckNoCriticalPoints(cell, flat, tag + " (no interior)"))
      {
        ok = false;
      }
    }
  }
  return ok;
}

bool TestTetrahedronOrders()
{
  bool ok = true;
  for (int order = 1; order <= 5; ++order)
  {
    Cell cell;
    if (!MakeCell("vtkDGTet"_token, order, 0, cell))
    {
      ok = false;
      continue;
    }
    const std::string tag = "TestTetrahedronOrders order=" + vtk::to_string(order);
    bool anyApplicable = false;

    FoldOutcome(RunSimplexDipTest(cell, order, 3, {}, tag + " interior"), ok, anyApplicable);
    for (int pin = 0; pin < 4; ++pin)
    {
      FoldOutcome(RunSimplexDipTest(
                    cell, order, 3, { pin }, tag + " face lambda" + vtk::to_string(pin) + "=0"),
        ok, anyApplicable);
    }
    for (int pinA = 0; pinA < 4; ++pinA)
    {
      for (int pinB = pinA + 1; pinB < 4; ++pinB)
      {
        FoldOutcome(
          RunSimplexDipTest(cell, order, 3, { pinA, pinB },
            tag + " edge lambda" + vtk::to_string(pinA) + "=lambda" + vtk::to_string(pinB) + "=0"),
          ok, anyApplicable);
      }
    }

    if (!anyApplicable)
    {
      const std::vector<double> flat(cell.NetSize(), 1.);
      if (!CheckNoCriticalPoints(cell, flat, tag + " (no interior)"))
      {
        ok = false;
      }
    }
  }
  return ok;
}

/// A critical point that is neither a minimum nor a maximum, on a
/// tensor-product entity: the case no seed-and-descend search can reach,
/// since an optimizer walks away from a saddle and a per-axis sign-change
/// test cannot see a derivative that touches zero without crossing it.
///
/// Each field below is written directly in the Bernstein basis, which for
/// these needs no computation: an order-1 net's coefficients are just the
/// field's corner values, and the blossom of a product of coordinates is
/// the product of those coordinates' own blossoms, so a coefficient is the
/// polynomial evaluated at its own index's corner. Each was chosen to have
/// exactly one critical point anywhere on the cell, so the same
/// single-point checker every other case here uses applies unchanged.
bool TestPrismaticSaddles()
{
  bool ok = true;
  struct Case
  {
    const char* Label;
    vtkStringToken CellType;
    int Order;
    std::vector<double> Coefficients;
    std::array<double, 3> Location;
    int Dimension;
  };

  const std::vector<Case> cases{
    // field = r * s: a saddle at the cell's center, and nothing on the 4
    // edges, where the field is linear.
    { "TestPrismaticSaddles quad order=1 (r*s)", "vtkDGQuad"_token, 1, { 1., -1., -1., 1. },
      { { 0., 0., 0. } }, 2 },
    // The same field at order 2 (r is (-1, 0, 1) there), so the search has
    // to subdivide rather than read the answer off a 2x2 net.
    { "TestPrismaticSaddles quad order=2 (r*s)", "vtkDGQuad"_token, 2,
      { 1., 0., -1., 0., 0., 0., -1., 0., 1. }, { { 0., 0., 0. } }, 2 },
    // field = r*s + r*t + s*t: grad = (s+t, r+t, r+s) vanishes only at the
    // center. Each face's own restriction is critical only at one of that
    // face's corners, and each edge's is either linear or constant, so the
    // center is again the only point reported.
    { "TestPrismaticSaddles hex order=1 (r*s + r*t + s*t)", "vtkDGHex"_token, 1,
      { 3., -1., -1., -1., -1., -1., -1., 3. }, { { 0., 0., 0. } }, 3 },
    // field = r^3: its derivative touches zero at r = 0 without changing
    // sign there, so this is critical but is neither a minimum nor a
    // maximum. The net is (-1, 1, -1, 1) because the blossom of r^3 at an
    // index repeating the two ends is (-1)^(3-i) * 1^i.
    { "TestPrismaticSaddles edge order=3 (r^3)", "vtkDGEdge"_token, 3, { -1., 1., -1., 1. },
      { { 0., 0., 0. } }, 1 },
  };

  for (const auto& testCase : cases)
  {
    Cell cell;
    if (!MakeCell(testCase.CellType, testCase.Order, 0, cell))
    {
      ok = false;
      continue;
    }
    // Every field above is a sum of products of coordinates, so it is 0 at
    // the cell's own center.
    if (!CheckCriticalPoint(
          cell, testCase.Coefficients, testCase.Location, 0., testCase.Dimension, testCase.Label))
    {
      ok = false;
    }
  }
  return ok;
}

/// A wedge whose interior critical point is a saddle - the case the
/// interior's old seed-and-descend search could not report, since an
/// optimizer walks away from one.
///
/// The field is f = t * (r - s) + (r - s)^2 + (r + s - 1/2)^2, which is
/// quadratic on the triangular cross-section and linear along the prismatic
/// axis, so it is an order (2, 2, 1) wedge. Writing it in d = r - s and
/// u = r + s, grad(f) = (d, t + 2d, 2u - 1) against (t, d, u), which
/// vanishes only at d = 0, t = 0, u = 1/2 - the point r = s = 1/4, t = 0,
/// where f = 0. Its Hessian in (r, s, t) is
/// {{4, 0, 1}, {0, 4, -1}, {1, -1, 0}}, whose determinant is -8: indefinite,
/// so this is a saddle and not an extremum of any kind.
///
/// The rest of the cell is checked along with it, since every entity's
/// restriction here is simple enough to solve by hand. On a cap the field
/// is critical where 2d = -/+1 and u = 1/2, which is a point of that cap's
/// boundary rather than its interior, so the caps themselves report nothing
/// and four of their edges report one point each; both quadrilateral faces
/// meeting r = 0 or s = 0 need r or s to vanish to be critical, which is
/// again their own boundary, while the third (r + s = 1) is critical at its
/// center; and the three vertical edges are linear in t or constant. Six
/// points in all.
bool TestWedgeSaddle()
{
  // Bernstein coefficients, by the blossom of each term: the quadratic
  // forms' polar forms at the triangle vertex pairs an index names, and the
  // prismatic axis' own endpoint for t. Laid out (i, j) with j outermost,
  // once per t endpoint, the way vtkDGLagrangePoints numbers a wedge.
  const std::vector<double> coefficients{
    0.25, -0.75, 0.25, 0.25, -0.75, 2.25, // t = -1
    0.25, 0.25, 2.25, -0.75, -0.75, 0.25  // t = +1
  };
  Cell cell;
  if (!MakeCell("vtkDGWdg"_token, 2, 1, cell))
  {
    return false;
  }

  struct Expected
  {
    std::array<double, 3> Parameter;
    double Value;
    int Dimension;
  };
  const std::vector<Expected> expected{
    { { { 0.25, 0.25, 0. } }, 0., 3 },  // the interior saddle
    { { { 0.5, 0.5, 0. } }, 0.25, 2 },  // the r + s = 1 side face's center
    { { { 0.5, 0., -1. } }, -0.25, 1 }, // s = 0 on the t = -1 cap
    { { { 0.75, 0.25, -1. } }, 0., 1 }, // r + s = 1 on the t = -1 cap
    { { { 0., 0.5, 1. } }, -0.25, 1 },  // r = 0 on the t = +1 cap
    { { { 0.25, 0.75, 1. } }, 0., 1 },  // r + s = 1 on the t = +1 cap
  };

  std::cout << "TestWedgeSaddle:\n";
  const auto points = Search(cell, coefficients);
  std::cout << "  found " << points.size() << " critical point(s)\n";
  for (const auto& point : points)
  {
    std::cout << "  point = (" << point.Parameter[0] << ", " << point.Parameter[1] << ", "
              << point.Parameter[2] << "), value = " << point.Value
              << ", dimension = " << point.EntityDimension << "\n";
  }
  const bool crossChecked = CrossCheckAgainstReference(cell, coefficients, points);

  bool ok = true;
  if (points.size() != expected.size())
  {
    std::cerr << "  ERROR: expected " << expected.size() << " critical points, found "
              << points.size() << ".\n";
    ok = false;
  }
  for (const auto& want : expected)
  {
    bool found = false;
    for (const auto& point : points)
    {
      double distance = 0.;
      for (int axis = 0; axis < 3; ++axis)
      {
        const double delta = point.Parameter[axis] - want.Parameter[axis];
        distance += delta * delta;
      }
      if (std::sqrt(distance) > tolerance)
      {
        continue;
      }
      found = true;
      if (point.EntityDimension != want.Dimension)
      {
        std::cerr << "  ERROR: expected dimension " << want.Dimension << " at ("
                  << want.Parameter[0] << ", " << want.Parameter[1] << ", " << want.Parameter[2]
                  << "), got " << point.EntityDimension << ".\n";
        ok = false;
      }
      if (std::abs(point.Value - want.Value) > tolerance)
      {
        std::cerr << "  ERROR: expected value " << want.Value << " at (" << want.Parameter[0]
                  << ", " << want.Parameter[1] << ", " << want.Parameter[2] << "), got "
                  << point.Value << ".\n";
        ok = false;
      }
      break;
    }
    if (!found)
    {
      std::cerr << "  ERROR: no critical point at (" << want.Parameter[0] << ", "
                << want.Parameter[1] << ", " << want.Parameter[2] << ").\n";
      ok = false;
    }
  }
  return ok && crossChecked;
}

bool TestWedgeOrders()
{
  bool ok = true;
  for (int order = 1; order <= 5; ++order)
  {
    const std::string tag = "TestWedgeOrders order=" + vtk::to_string(order);
    Cell cell;
    if (!MakeCell("vtkDGWdg"_token, order, order, cell))
    {
      ok = false;
      continue;
    }
    bool anyApplicable = false;

    FoldOutcome(RunWedgeDipTest(cell, order, {}, -1, tag + " interior"), ok, anyApplicable);
    for (int cap : { 0, order })
    {
      FoldOutcome(RunWedgeDipTest(cell, order, {}, cap, tag + " cap t=" + vtk::to_string(cap)), ok,
        anyApplicable);
      for (int pin = 0; pin < 3; ++pin)
      {
        FoldOutcome(
          RunWedgeDipTest(cell, order, { pin }, cap,
            tag + " cap edge lambda" + vtk::to_string(pin) + "=0 t=" + vtk::to_string(cap)),
          ok, anyApplicable);
      }
    }
    for (int pin = 0; pin < 3; ++pin)
    {
      FoldOutcome(RunWedgeDipTest(cell, order, { pin }, -1,
                    tag + " side face lambda" + vtk::to_string(pin) + "=0"),
        ok, anyApplicable);
    }
    for (int pinA = 0; pinA < 3; ++pinA)
    {
      for (int pinB = pinA + 1; pinB < 3; ++pinB)
      {
        FoldOutcome(RunWedgeDipTest(cell, order, { pinA, pinB }, -1,
                      tag + " vertical edge lambda" + vtk::to_string(pinA) + "=lambda" +
                        vtk::to_string(pinB) + "=0"),
          ok, anyApplicable);
      }
    }

    if (!anyApplicable)
    {
      const std::vector<double> flat(cell.NetSize(), 1.);
      if (!CheckNoCriticalPoints(cell, flat, tag + " (no interior)"))
      {
        ok = false;
      }
    }
  }
  return ok;
}

bool TestPyramidOrders()
{
  bool ok = true;
  for (int order = 1; order <= 5; ++order)
  {
    const std::string tag = "TestPyramidOrders order=" + vtk::to_string(order);
    Cell cell;
    if (!MakeCell("vtkDGPyr"_token, order, 0, cell))
    {
      ok = false;
      continue;
    }
    bool anyApplicable = false;

    // The interior and the triangular faces need a layer strictly between
    // the base and the apex that is still at least 2 wide, so that its grid
    // has an interior index: order 3 and up.
    const int middle = (order - 1) / 2;
    const int width = order - middle;
    if (order >= 3)
    {
      FoldOutcome(
        RunPyramidDipTest(cell, order, width / 2, width / 2, middle, 3, tag + " interior"), ok,
        anyApplicable);
      FoldOutcome(RunPyramidDipTest(cell, order, 0, width / 2, middle, 2, tag + " face r=-w"), ok,
        anyApplicable);
      FoldOutcome(RunPyramidDipTest(cell, order, width, width / 2, middle, 2, tag + " face r=+w"),
        ok, anyApplicable);
      FoldOutcome(RunPyramidDipTest(cell, order, width / 2, 0, middle, 2, tag + " face s=-w"), ok,
        anyApplicable);
      FoldOutcome(RunPyramidDipTest(cell, order, width / 2, width, middle, 2, tag + " face s=+w"),
        ok, anyApplicable);
    }

    if (order >= 2)
    {
      const int half = order / 2;
      FoldOutcome(
        RunPyramidDipTest(cell, order, half, half, 0, 2, tag + " base"), ok, anyApplicable);
      FoldOutcome(
        RunPyramidDipTest(cell, order, 0, half, 0, 1, tag + " base edge r=-1"), ok, anyApplicable);
      FoldOutcome(RunPyramidDipTest(cell, order, order, half, 0, 1, tag + " base edge r=+1"), ok,
        anyApplicable);
      FoldOutcome(
        RunPyramidDipTest(cell, order, half, 0, 0, 1, tag + " base edge s=-1"), ok, anyApplicable);
      FoldOutcome(RunPyramidDipTest(cell, order, half, order, 0, 1, tag + " base edge s=+1"), ok,
        anyApplicable);

      // The edges rising to the apex: each layer's own corner.
      const int rest = order - half;
      for (int endR : { 0, rest })
      {
        for (int endS : { 0, rest })
        {
          FoldOutcome(RunPyramidDipTest(cell, order, endR, endS, half, 1,
                        tag + " slant edge r=" + (endR == 0 ? "-w" : "+w") +
                          " s=" + (endS == 0 ? "-w" : "+w")),
            ok, anyApplicable);
        }
      }
    }

    if (!anyApplicable)
    {
      const std::vector<double> flat(cell.NetSize(), 1.);
      if (!CheckNoCriticalPoints(cell, flat, tag + " (no interior)"))
      {
        ok = false;
      }
    }
  }
  return ok;
}

/// A pyramid whose interior critical point is a saddle.
///
/// The field is f = r * s + (t - 1/3)^2, a quadratic, which the order-2
/// pyramid space contains. grad(f) = (s, r, 2t - 2/3) vanishes only at
/// (0, 0, 1/3), where f = 0, and its Hessian {{0, 1, 0}, {1, 0, 0},
/// {0, 0, 2}} has determinant -2: a saddle.
///
/// The rest of the cell is solved by hand as well. On the base (t = 0) f is
/// r * s + 1/9, a saddle at its center, and linear along each base edge. On
/// a triangular face, say r = -(1 - t), f's slope along s is -(1 - t), which
/// never vanishes below the apex, and likewise on the other three. On the
/// edges rising to the apex, r * s = +/-(1 - t)^2, and only the two where it
/// is + are critical, at t = 2/3. Four points in all.
bool TestPyramidSaddle()
{
  // In the collapsed coordinates f = (2a - 1)(2b - 1)(1 - c)^2 + (c - 1/3)^2,
  // so the first term lives on the k = 0 layer only, with (2a - 1)'s own
  // degree-2 Bernstein coefficients (-1, 0, 1) in a and in b; the second is
  // constant across each layer, at the blossom of (c - 1/3)^2 at that layer's
  // index: 1/9, -2/9, 4/9.
  const double ninth = 1. / 9.;
  const std::vector<double> coefficients{
    1. + ninth, ninth, -1. + ninth, ninth, ninth, ninth, -1. + ninth, ninth, 1. + ninth, // k = 0
    -2. * ninth, -2. * ninth, -2. * ninth, -2. * ninth,                                  // k = 1
    4. * ninth                                                                           // k = 2
  };
  Cell cell;
  if (!MakeCell("vtkDGPyr"_token, 2, 0, cell))
  {
    return false;
  }

  struct Expected
  {
    std::array<double, 3> Parameter;
    double Value;
    int Dimension;
  };
  const std::vector<Expected> expected{
    { { { 0., 0., 1. / 3. } }, 0., 3 },                     // the interior saddle
    { { { 0., 0., 0. } }, ninth, 2 },                       // the base's center
    { { { 1. / 3., 1. / 3., 2. / 3. } }, 2. * ninth, 1 },   // the r = s = +w edge
    { { { -1. / 3., -1. / 3., 2. / 3. } }, 2. * ninth, 1 }, // the r = s = -w edge
  };

  std::cout << "TestPyramidSaddle:\n";
  const auto points = Search(cell, coefficients);
  std::cout << "  found " << points.size() << " critical point(s)\n";
  for (const auto& point : points)
  {
    std::cout << "  point = (" << point.Parameter[0] << ", " << point.Parameter[1] << ", "
              << point.Parameter[2] << "), value = " << point.Value
              << ", dimension = " << point.EntityDimension << "\n";
  }
  const bool crossChecked = CrossCheckAgainstReference(cell, coefficients, points);

  bool ok = true;
  if (points.size() != expected.size())
  {
    std::cerr << "  ERROR: expected " << expected.size() << " critical points, found "
              << points.size() << ".\n";
    ok = false;
  }
  for (const auto& want : expected)
  {
    bool found = false;
    for (const auto& point : points)
    {
      double distance = 0.;
      for (int axis = 0; axis < 3; ++axis)
      {
        const double delta = point.Parameter[axis] - want.Parameter[axis];
        distance += delta * delta;
      }
      if (std::sqrt(distance) > tolerance)
      {
        continue;
      }
      found = true;
      if (point.EntityDimension != want.Dimension)
      {
        std::cerr << "  ERROR: expected dimension " << want.Dimension << " at ("
                  << want.Parameter[0] << ", " << want.Parameter[1] << ", " << want.Parameter[2]
                  << "), got " << point.EntityDimension << ".\n";
        ok = false;
      }
      if (std::abs(point.Value - want.Value) > tolerance)
      {
        std::cerr << "  ERROR: expected value " << want.Value << " at (" << want.Parameter[0]
                  << ", " << want.Parameter[1] << ", " << want.Parameter[2] << "), got "
                  << point.Value << ".\n";
        ok = false;
      }
      break;
    }
    if (!found)
    {
      std::cerr << "  ERROR: no critical point at (" << want.Parameter[0] << ", "
                << want.Parameter[1] << ", " << want.Parameter[2] << ").\n";
      ok = false;
    }
  }
  return ok && crossChecked;
}

// -----------------------------------------------------------------------
// Random control nets
// -----------------------------------------------------------------------
//
// Everything above builds a field whose critical points are known in closed
// form, which is the right way to test that the search is correct but only
// reaches the fields that construction can express - all of them with a
// single dip, and none of them with a saddle. Random nets reach the rest:
// several critical points at once, of every type, in no particular
// arrangement, which is what general Bezier data looks like.
//
// There is no closed form to compare against there, so two things are
// asserted instead, and neither needs one:
//
// + Every point reported really is critical - the field's gradient along
//   every direction that stays inside the entity the point was found on
//   vanishes there. That is an exact property of a correct answer, checked
//   against VTK's own basis and gradient operators rather than against
//   anything the search computed.
// + The number of points found is exactly what it is today. This one is a
//   record of behaviour, not a derivation - but it is the half that can
//   catch a lost root, which the gradient check above cannot: a search that
//   quietly stops finding something still reports nothing wrong about what
//   it did find.
//
// + On a prefix of each case's nets, every critical point an independent
//   multi-start Newton solve of grad(f) = 0 can find is also reported by
//   the search. That is the only one of the three that can catch a *lost*
//   root, which is worth having: a search that quietly stops finding
//   something still reports nothing wrong about what it did find. See
//   ReferenceRoots() for what it is and, just as importantly, what it is
//   not - it is a cross-check that misses roots of its own, so it is
//   asserted in one direction only and a pass proves nothing by itself.
//
// So a count that moves is not by itself a failure. It means either that
// roots are being lost or that roots previously missed are now found, and
// the Newton check is what tells those apart - if it starts reporting
// unfound roots, they were lost.

/// A reproducible stream of control-point coefficients in [-1, 1].
///
/// std::uniform_real_distribution is not specified to produce the same
/// sequence across standard-library implementations, and the expected
/// counts below are exact, so the mapping from the engine's own output -
/// which *is* specified - is written out here rather than delegated.
class CoefficientStream
{
public:
  explicit CoefficientStream(std::uint_fast32_t seed)
    : Engine(seed)
  {
  }

  double Next()
  {
    const double unit = static_cast<double>(this->Engine() - std::mt19937::min()) /
      static_cast<double>(std::mt19937::max() - std::mt19937::min());
    return 2. * unit - 1.;
  }

  std::vector<double> Net(std::size_t size)
  {
    std::vector<double> net(size);
    for (auto& coefficient : net)
    {
      coefficient = this->Next();
    }
    return net;
  }

private:
  std::mt19937 Engine;
};

/// One (shape, order) sweep of random nets.
struct RandomCase
{
  std::string Label;
  vtkStringToken CellType;
  Family Kind;
  int Dimension;
  int Order;
  int PrismaticOrder; ///< Wedges only; 0 elsewhere.
  int Nets;
  /// This case's own seed. Written out rather than derived from the case's
  /// name or position so that adding a case, renaming one, or reordering
  /// the table leaves every other case's nets - and so its recorded count -
  /// exactly where they were.
  std::uint_fast32_t Seed;
  /// How many of this case's nets also get the Newton cross-check of
  /// ReferenceRoots(). It costs far more than the search it checks, so it
  /// runs over a prefix of the nets rather than all of them.
  int NewtonNets;
  int ExpectedPoints; ///< Total over all nets: see this section's note.
};

bool RunRandomCase(const RandomCase& testCase)
{
  std::cout << testCase.Label << ":\n";

  Cell cell;
  if (!MakeCell(testCase.CellType, testCase.Order, testCase.PrismaticOrder, cell))
  {
    return false;
  }

  CoefficientStream stream(testCase.Seed);

  bool ok = true;
  int total = 0;
  int checkedRoots = 0;
  double worstResidual = 0.;
  for (int net = 0; net < testCase.Nets; ++net)
  {
    const std::vector<double> coefficients = stream.Net(cell.NetSize());
    double scale = 1.;
    for (double coefficient : coefficients)
    {
      scale = std::max(scale, std::abs(coefficient));
    }

    const auto points = Search(cell, coefficients);
    total += static_cast<int>(points.size());

    for (const auto& point : points)
    {
      const auto tangents = EntityTangents(cell.Kind, cell.Dimension, point.Parameter);
      if (static_cast<int>(tangents.size()) != point.EntityDimension)
      {
        std::cerr << "  ERROR: net " << net << ": point (" << point.Parameter[0] << ", "
                  << point.Parameter[1] << ", " << point.Parameter[2] << ") is reported on a "
                  << point.EntityDimension << "-dimensional entity, but its coordinates leave "
                  << tangents.size() << " direction(s) free.\n";
        ok = false;
        continue;
      }

      std::array<double, 3> gradient{ { 0., 0., 0. } };
      vtk::basis::bezier::EvaluateField(
        cell.BasisOp, cell.GradOp, cell.Dimension, coefficients, point.Parameter, gradient);
      for (const auto& tangent : tangents)
      {
        double slope = 0.;
        for (int axis = 0; axis < 3; ++axis)
        {
          slope += gradient[axis] * tangent[axis];
        }
        const double residual = std::abs(slope) / scale;
        worstResidual = std::max(worstResidual, residual);
        if (residual > criticalityTolerance)
        {
          std::cerr << "  ERROR: net " << net << ": point (" << point.Parameter[0] << ", "
                    << point.Parameter[1] << ", " << point.Parameter[2]
                    << ") is not critical - directional derivative " << slope << " (relative "
                    << residual << ").\n";
          ok = false;
        }
      }
    }

    if (net < testCase.NewtonNets &&
      !CrossCheckAgainstReference(cell, coefficients, points, &checkedRoots))
    {
      std::cerr << "  (the unreported point(s) above are from net " << net << ")\n";
      ok = false;
    }
  }

  std::cout << "  " << testCase.Nets << " nets, " << total
            << " critical point(s), worst relative directional derivative " << worstResidual << "; "
            << checkedRoots << " of them cross-checked against an independent solve on "
            << testCase.NewtonNets << " net(s)\n";
  if (total != testCase.ExpectedPoints)
  {
    std::cerr << "  ERROR: expected " << testCase.ExpectedPoints << " critical point(s) in total, "
              << "found " << total << ". See this section's note: a moved count means roots were "
              << "lost or roots previously missed are now found, and either needs explaining.\n";
    ok = false;
  }
  return ok;
}

bool TestRandomNets()
{
  // Orders and net counts are chosen to keep this sweep a small fraction of
  // the test's runtime; the search cost grows steeply with a cell's
  // dimension and order, so hexahedra, wedges and pyramids carry fewer.
  const std::vector<RandomCase> cases{
    { "Random edge nets, order 2", "vtkDGEdge"_token, Family::Prismatic, 1, 2, 0, 100, 1001u, 50,
      62 },
    { "Random edge nets, order 3", "vtkDGEdge"_token, Family::Prismatic, 1, 3, 0, 100, 1002u, 50,
      104 },
    { "Random edge nets, order 4", "vtkDGEdge"_token, Family::Prismatic, 1, 4, 0, 100, 1003u, 50,
      149 },
    { "Random edge nets, order 5", "vtkDGEdge"_token, Family::Prismatic, 1, 5, 0, 100, 1004u, 50,
      150 },
    { "Random quadrilateral nets, order 2", "vtkDGQuad"_token, Family::Prismatic, 2, 2, 0, 100,
      1005u, 20, 375 },
    { "Random quadrilateral nets, order 3", "vtkDGQuad"_token, Family::Prismatic, 2, 3, 0, 100,
      1006u, 20, 592 },
    { "Random quadrilateral nets, order 4", "vtkDGQuad"_token, Family::Prismatic, 2, 4, 0, 100,
      1007u, 20, 794 },
    { "Random quadrilateral nets, order 5", "vtkDGQuad"_token, Family::Prismatic, 2, 5, 0, 100,
      1008u, 20, 1084 },
    { "Random triangle nets, order 2", "vtkDGTri"_token, Family::Barycentric, 2, 2, 0, 100, 1009u,
      20, 273 },
    { "Random triangle nets, order 3", "vtkDGTri"_token, Family::Barycentric, 2, 3, 0, 100, 1010u,
      20, 397 },
    { "Random triangle nets, order 4", "vtkDGTri"_token, Family::Barycentric, 2, 4, 0, 100, 1011u,
      20, 553 },
    { "Random triangle nets, order 5", "vtkDGTri"_token, Family::Barycentric, 2, 5, 0, 100, 1012u,
      20, 767 },
    { "Random tetrahedron nets, order 2", "vtkDGTet"_token, Family::Barycentric, 3, 2, 0, 100,
      1013u, 20, 578 },
    { "Random tetrahedron nets, order 3", "vtkDGTet"_token, Family::Barycentric, 3, 3, 0, 100,
      1014u, 20, 1053 },
    { "Random tetrahedron nets, order 4", "vtkDGTet"_token, Family::Barycentric, 3, 4, 0, 50, 1015u,
      20, 734 },
    { "Random hexahedron nets, order 2", "vtkDGHex"_token, Family::Prismatic, 3, 2, 0, 100, 1016u,
      20, 1516 },
    { "Random hexahedron nets, order 3", "vtkDGHex"_token, Family::Prismatic, 3, 3, 0, 50, 1017u,
      20, 1297 },
    { "Random hexahedron nets, order 4", "vtkDGHex"_token, Family::Prismatic, 3, 4, 0, 20, 1018u,
      20, 814 },
    { "Random wedge nets, order 2 x 1", "vtkDGWdg"_token, Family::Wedge, 3, 2, 1, 100, 1019u, 20,
      696 },
    { "Random wedge nets, order 3 x 2", "vtkDGWdg"_token, Family::Wedge, 3, 3, 2, 50, 1020u, 20,
      765 },
    { "Random wedge nets, order 4 x 2", "vtkDGWdg"_token, Family::Wedge, 3, 4, 2, 20, 1021u, 20,
      420 },
    { "Random pyramid nets, order 2", "vtkDGPyr"_token, Family::Pyramid, 3, 2, 0, 100, 1022u, 20,
      843 },
    { "Random pyramid nets, order 3", "vtkDGPyr"_token, Family::Pyramid, 3, 3, 0, 50, 1023u, 20,
      794 },
    { "Random pyramid nets, order 4", "vtkDGPyr"_token, Family::Pyramid, 3, 4, 0, 20, 1024u, 20,
      457 },
  };

  bool ok = true;
  for (const auto& testCase : cases)
  {
    ok &= RunRandomCase(testCase);
  }
  return ok;
}

} // anonymous namespace

int TestBezierFindCriticalPoints(int, char*[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  bool ok = true;
  ok &= TestEdgeOrders();
  ok &= TestQuadOrders();
  ok &= TestHexOrders();
  ok &= TestTriangleOrders();
  ok &= TestTetrahedronOrders();
  ok &= TestWedgeOrders();
  ok &= TestWedgeSaddle();
  ok &= TestPyramidOrders();
  ok &= TestPyramidSaddle();
  ok &= TestPrismaticSaddles();
  ok &= TestRandomNets();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
