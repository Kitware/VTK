// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise the arbitrary-order ("A") HGrad basis functions registered for the
// DG cell shapes: vertex, edge, quadrilateral, hexahedron, triangle,
// tetrahedron, wedge, and pyramid.
//
// The properties checked here define a nodal Lagrange basis, so together they
// pin the basis down completely:
//
// + partition of unity - the basis functions sum to 1 everywhere;
// + the Kronecker delta property - the i-th function is 1 at the i-th Lagrange
//   point and 0 at every other one;
// + reproduction - interpolating a function drawn from the space the basis
//   spans reproduces it exactly, along with its gradient.
//
// The last of these is the property downstream code relies on and is by far the
// most sensitive: an error anywhere in a basis function or its gradient shows
// up as a mismatch at parametric coordinates away from the Lagrange points.
//
// Every shape runs through TestBasis(), the Lattice enum saying how its
// Lagrange points are laid out and which space it spans. The pyramid differs
// only in what it reproduces: its basis (PyrCnBasis in vtkDGHGradOperators.cxx)
// is rational rather than polynomial, so most monomials are not in its span at
// all. That is not particular to this basis - no conforming pyramid element can
// be polynomial, and every pyramid basis in this module divides by 1 - t,
// PyrC1Basis.h and PyrC2Basis.h included - so there is no polynomial
// arbitrary-order pyramid basis to compare against instead. The modal "G"
// basis, which spans the same space, stands in for the polynomial there, every
// one of its functions tried in turn for the same reason PolynomialTerms()
// below carries every monomial rather than one.
//
// Gradients are checked here only through reproduction, which compares them
// against another basis; TestCellGridBasisGradients checks each one against a
// central difference of its own basis, which no other basis can taint.
//
// Note that the Cn bases number their degrees of freedom lexicographically (the
// r-axis varying fastest), which is *not* the corner-first ordering used by the
// fixed-order HGrad bases (C1, C2). The Lagrange points below follow the Cn
// convention.

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
#include "vtkNew.h"
#include "vtkStringFormatter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{

constexpr double tolerance = 1e-9;

/// How a cell shape's parameter space is subdivided into Lagrange points, which
/// also determines the space of polynomials the resulting basis spans.
enum class Lattice
{
  Prismatic, //!< A tensor product with an independent order along each axis.
  Simplex,   //!< A principal lattice with one total degree shared by all axes.
  Wedge,     //!< A triangle's principal lattice times an edge's points.
  Pyramid    //!< Square layers shrinking to a point at the apex.
};

bool testValue(double actual, double expected, const std::string& what)
{
  if (std::abs(actual - expected) > tolerance)
  {
    std::cerr << "  ERROR: " << what << " = " << actual << " != " << expected << " (off by "
              << (actual - expected) << ").\n";
    return false;
  }
  return true;
}

/// The parametric coordinate of the \a ii-th of \a order + 1 points spanning
/// the [-1, 1] range that a prismatic axis is parameterized over.
double PrismaticCoordinate(int ii, int order)
{
  return order > 0 ? -1. + 2. * ii / order : 0.;
}

/// Return the parametric coordinates of the Lagrange point for each basis
/// function, in the order the basis functions are evaluated.
///
/// Axes the cell does not parameterize are held at 0.
std::vector<std::array<double, 3>> LagrangePoints(
  Lattice lattice, const std::vector<int>& order, int dimension)
{
  std::vector<std::array<double, 3>> points;
  switch (lattice)
  {
    case Lattice::Prismatic:
      // A tensor product, expanded one axis at a time so that the first axis
      // ends up varying fastest.
      points.push_back({ { 0., 0., 0. } });
      for (int axis = 0; axis < dimension; ++axis)
      {
        std::vector<std::array<double, 3>> expanded;
        for (int ii = 0; ii <= order[axis]; ++ii)
        {
          for (const auto& point : points)
          {
            auto next = point;
            next[axis] = PrismaticCoordinate(ii, order[axis]);
            expanded.push_back(next);
          }
        }
        points = expanded;
      }
      break;

    case Lattice::Simplex:
    {
      // Every multi-index of non-negative integers summing to the order; the
      // barycentric coordinate paired with the r-axis varies fastest.
      double scale = order[0] > 0 ? 1. / order[0] : 0.;
      for (int i3 = 0; i3 <= (dimension > 2 ? order[0] : 0); ++i3)
      {
        for (int i2 = 0; i2 <= order[0] - i3; ++i2)
        {
          for (int i1 = 0; i1 <= order[0] - i2 - i3; ++i1)
          {
            points.push_back({ { i1 * scale, i2 * scale, dimension > 2 ? i3 * scale : 0. } });
          }
        }
      }
    }
    break;

    case Lattice::Wedge:
    {
      // The triangle's lattice repeated once per point along t.
      double scale = order[0] > 0 ? 1. / order[0] : 0.;
      for (int it = 0; it <= order[2]; ++it)
      {
        for (int i2 = 0; i2 <= order[0]; ++i2)
        {
          for (int i1 = 0; i1 <= order[0] - i2; ++i1)
          {
            points.push_back({ { i1 * scale, i2 * scale, PrismaticCoordinate(it, order[2]) } });
          }
        }
      }
    }
    break;

    case Lattice::Pyramid:
    {
      // Layer kk lies at t = kk/n and carries a uniform (n-kk+1)^2 grid whose
      // own half-width is 1 - t, so the cross-section shrinks to one node at
      // the apex. This is the lattice PyrCnBasis.h and vtkDGLagrangePoints
      // use; re-derived here so a change to either has to disagree to pass.
      int nn = order[0];
      for (int kk = 0; kk <= nn; ++kk)
      {
        int mm = nn - kk;
        double tt = nn > 0 ? double(kk) / nn : 0.;
        for (int jj = 0; jj <= mm; ++jj)
        {
          for (int ii = 0; ii <= mm; ++ii)
          {
            points.push_back({ { nn > 0 ? double(2 * ii - mm) / nn : 0.,
              nn > 0 ? double(2 * jj - mm) / nn : 0., tt } });
          }
        }
      }
    }
    break;
  }
  return points;
}

/// One monomial of the polynomial a basis must reproduce.
struct Term
{
  double Coefficient;
  std::array<int, 3> Exponent;
};

/// Return a polynomial spanning exactly the space the basis spans.
///
/// Every monomial of that space appears, so no error in a basis function can
/// hide behind a missing term. The coefficients are arbitrary but deterministic
/// so that failures are reproducible.
std::vector<Term> PolynomialTerms(Lattice lattice, const std::vector<int>& order, int dimension)
{
  std::vector<Term> terms;
  auto add = [&terms](int rr, int ss, int tt)
  {
    // Vary the coefficients with the exponents so that a basis which mixes up
    // its parametric axes cannot pass by accident.
    double coefficient = 1. + 0.375 * rr - 0.25 * ss + 0.5 * tt + 0.125 * (rr * ss + tt);
    terms.push_back({ coefficient, { { rr, ss, tt } } });
  };

  int maxR = dimension > 0 ? order[0] : 0;
  int maxS = dimension > 1 ? order[1] : 0;
  int maxT = dimension > 2 ? order[2] : 0;
  for (int tt = 0; tt <= maxT; ++tt)
  {
    for (int ss = 0; ss <= maxS; ++ss)
    {
      for (int rr = 0; rr <= maxR; ++rr)
      {
        switch (lattice)
        {
          case Lattice::Prismatic:
            // Independent degrees along each axis.
            add(rr, ss, tt);
            break;
          case Lattice::Simplex:
            // A single total degree shared by all axes.
            if (rr + ss + tt <= order[0])
            {
              add(rr, ss, tt);
            }
            break;
          case Lattice::Wedge:
            // A total degree shared by r and s, independent of the degree in t.
            if (rr + ss <= order[0])
            {
              add(rr, ss, tt);
            }
            break;
          case Lattice::Pyramid:
            // Rational rather than polynomial, so no monomial basis to list;
            // TestBasis() reproduces the modal "G" basis in its place.
            break;
        }
      }
    }
  }
  return terms;
}

/// Evaluate the polynomial defined by \a terms and, optionally, its gradient.
double Evaluate(const std::vector<Term>& terms, const std::array<double, 3>& rst,
  std::array<double, 3>* gradient = nullptr)
{
  double value = 0.;
  if (gradient)
  {
    *gradient = { { 0., 0., 0. } };
  }
  for (const auto& term : terms)
  {
    double monomial = term.Coefficient;
    for (int axis = 0; axis < 3; ++axis)
    {
      monomial *= std::pow(rst[axis], term.Exponent[axis]);
    }
    value += monomial;

    if (!gradient)
    {
      continue;
    }
    // Differentiate with respect to each axis in turn, holding the factors
    // contributed by the other axes fixed.
    for (int axis = 0; axis < 3; ++axis)
    {
      int exponent = term.Exponent[axis];
      if (exponent == 0)
      {
        continue;
      }
      double derivative = term.Coefficient * exponent * std::pow(rst[axis], exponent - 1);
      for (int other = 0; other < 3; ++other)
      {
        if (other != axis)
        {
          derivative *= std::pow(rst[other], term.Exponent[other]);
        }
      }
      (*gradient)[axis] += derivative;
    }
  }
  return value;
}

/// Points at which to check the properties that must hold throughout the cell.
///
/// These deliberately avoid the Lagrange points, where a basis is constrained by
/// construction and where errors are least likely to show. Simplicial shapes get
/// their own set, since a sample must lie inside the reference cell for the
/// values there to be meaningful.
const std::vector<std::array<double, 3>>& SamplePoints(Lattice lattice)
{
  static const std::vector<std::array<double, 3>> prismatic{ { 0., 0., 0. }, { 0.3, -0.7, 0.55 },
    { -0.9, 0.15, -0.25 }, { 0.62, 0.62, 0.62 }, { -0.43, -0.11, 0.87 }, { 0.98, -0.98, 0.02 } };
  // Points with non-negative coordinates summing to less than one, so that they
  // lie inside the reference triangle and tetrahedron.
  static const std::vector<std::array<double, 3>> simplex{ { 0.25, 0.25, 0.25 }, { 0.1, 0.2, 0.3 },
    { 0.7, 0.15, 0.05 }, { 0.05, 0.6, 0.2 }, { 0.33, 0.33, 0.01 }, { 0.02, 0.03, 0.9 } };
  // The triangle's samples paired with coordinates along t in [-1, 1].
  static const std::vector<std::array<double, 3>> wedge{ { 0.25, 0.25, 0. }, { 0.1, 0.2, -0.65 },
    { 0.7, 0.15, 0.4 }, { 0.05, 0.6, 0.93 }, { 0.33, 0.33, -0.87 }, { 0.02, 0.03, 0.11 } };
  // Strictly inside |r|, |s| < 1 - t, so that no rational term divides by zero.
  static const std::vector<std::array<double, 3>> pyramid{ { 0., 0., 0.5 }, { 0.2, -0.1, 0.3 },
    { -0.5, 0.4, 0.1 }, { 0.05, 0.05, 0.9 }, { -0.3, -0.3, 0.6 }, { 0.15, -0.15, 0.75 } };
  switch (lattice)
  {
    case Lattice::Simplex:
      return simplex;
    case Lattice::Wedge:
      return wedge;
    case Lattice::Pyramid:
      return pyramid;
    default:
      break;
  }
  return prismatic;
}

/// Check every property of the Cn basis for one cell shape at one order.
bool TestBasis(vtkDGCell* cellType, Lattice lattice, const std::vector<int>& order)
{
  bool ok = true;
  int dimension = cellType->GetDimension();

  std::cout << "  " << cellType->GetClassName() << " order";
  for (int axisOrder : order)
  {
    std::cout << " " << axisOrder;
  }
  std::cout << "\n";

  vtkCellAttribute::CellTypeInfo cellTypeInfo;
  cellTypeInfo.FunctionSpace = "HGRAD";
  cellTypeInfo.Basis = "A";
  // The nominal order selects which operator is registered for the attribute.
  // Orders 1 and 2 have hand-written operators; requesting them here would
  // exercise those rather than the arbitrary-order ones under test.
  cellTypeInfo.Order = order.empty() ? 0 : order[0];

  bool anisotropic = false;
  for (int axisOrder : order)
  {
    anisotropic |= axisOrder != order[0];
  }
  if (anisotropic)
  {
    // An array in the "order" role overrides the nominal order with one order
    // per parametric axis.
    vtkNew<vtkIntArray> orderArray;
    orderArray->SetName("order");
    orderArray->SetNumberOfComponents(dimension);
    orderArray->SetNumberOfTuples(1);
    for (int axis = 0; axis < dimension; ++axis)
    {
      orderArray->SetTypedComponent(0, axis, order[axis]);
    }
    cellTypeInfo.ArraysByRole["order"] = orderArray;
  }

  auto basisOp = cellType->GetOperatorEntry("Basis", cellTypeInfo);
  auto gradientOp = cellType->GetOperatorEntry("BasisGradient", cellTypeInfo);
  if (!basisOp || !gradientOp)
  {
    std::cerr << "  ERROR: No arbitrary-order basis registered.\n";
    return false;
  }

  auto points = LagrangePoints(lattice, order, dimension);
  int numberOfFunctions = static_cast<int>(points.size());
  if (basisOp.NumberOfFunctions != numberOfFunctions ||
    gradientOp.NumberOfFunctions != numberOfFunctions)
  {
    std::cerr << "  ERROR: Expected " << numberOfFunctions << " basis functions but the operators "
              << "report " << basisOp.NumberOfFunctions << " and " << gradientOp.NumberOfFunctions
              << ".\n";
    return false;
  }

  std::vector<double> basis(numberOfFunctions * basisOp.OperatorSize);
  std::vector<double> gradient(numberOfFunctions * gradientOp.OperatorSize);

  // The Kronecker delta property: the ii-th function is 1 at the ii-th Lagrange
  // point and 0 at the others.
  for (int ii = 0; ii < numberOfFunctions; ++ii)
  {
    basisOp.Evaluate(points[ii], basis);
    for (int jj = 0; jj < numberOfFunctions; ++jj)
    {
      ok &= testValue(basis[jj], ii == jj ? 1. : 0.,
        "basis " + vtk::to_string(jj) + " at Lagrange point " + vtk::to_string(ii));
    }
  }

  // What the basis must reproduce exactly. Every lattice but the pyramid's
  // spans a space of polynomials, so a single polynomial carrying every
  // monomial of that space covers it. The pyramid's span is rational (see the
  // file comment), so the modal "G" basis - which spans the same space, and is
  // fetched through the same public API rather than from anything internal to
  // the basis under test - stands in for it, every one of its functions tried
  // in turn.
  auto terms = PolynomialTerms(lattice, order, dimension);
  vtkDGOperatorEntry modalOp;
  vtkDGOperatorEntry modalGradientOp;
  std::vector<double> modeAtPoints;
  if (lattice == Lattice::Pyramid)
  {
    vtkCellAttribute::CellTypeInfo modalInfo;
    modalInfo.FunctionSpace = "HGRAD";
    modalInfo.Basis = "G";
    modalInfo.Order = order[0];
    modalOp = cellType->GetOperatorEntry("Basis", modalInfo);
    modalGradientOp = cellType->GetOperatorEntry("BasisGradient", modalInfo);
    if (modalOp && modalGradientOp && modalOp.NumberOfFunctions != numberOfFunctions)
    {
      std::cerr << "  ERROR: the modal \"G\" basis has " << modalOp.NumberOfFunctions
                << " functions where the nodal one has " << numberOfFunctions << ".\n";
      return false;
    }
    // modeAtPoints[ii * numberOfFunctions + mm] is the mm-th modal function's
    // value at the ii-th Lagrange point.
    if (modalOp && modalGradientOp)
    {
      modeAtPoints.resize(numberOfFunctions * numberOfFunctions);
      for (int ii = 0; ii < numberOfFunctions; ++ii)
      {
        std::vector<double> modeAtPoint(numberOfFunctions);
        modalOp.Evaluate(points[ii], modeAtPoint);
        std::copy(
          modeAtPoint.begin(), modeAtPoint.end(), modeAtPoints.begin() + ii * numberOfFunctions);
      }
    }
    else
    {
      // The modal basis is registered at fixed orders 1-5 only; past that the
      // nodal basis is still checked, just without a function to reproduce.
      std::cout << "    (no modal \"G\" basis at this order; reproduction not checked)\n";
    }
  }
  bool reproduce = lattice != Lattice::Pyramid || !modeAtPoints.empty();
  std::vector<double> modalBasis(lattice == Lattice::Pyramid ? numberOfFunctions : 0);
  std::vector<double> modalGradient(lattice == Lattice::Pyramid ? numberOfFunctions * 3 : 0);

  for (const auto& rst : SamplePoints(lattice))
  {
    basisOp.Evaluate(rst, basis);
    gradientOp.Evaluate(rst, gradient);

    // Partition of unity.
    double sum = 0.;
    for (int ii = 0; ii < numberOfFunctions; ++ii)
    {
      sum += basis[ii];
    }
    ok &= testValue(sum, 1., "sum of basis functions");

    // The gradients of a partition of unity must cancel.
    for (int axis = 0; axis < gradientOp.OperatorSize; ++axis)
    {
      double gradientSum = 0.;
      for (int ii = 0; ii < numberOfFunctions; ++ii)
      {
        gradientSum += gradient[ii * gradientOp.OperatorSize + axis];
      }
      ok &= testValue(gradientSum, 0., "sum of basis gradients along axis " + vtk::to_string(axis));
    }

    // Reproduction: interpolating a function of the basis' own span from its
    // values at the Lagrange points must give back that function and its
    // gradient here.
    int numberOfTargets = reproduce ? 1 : 0;
    if (lattice == Lattice::Pyramid && reproduce)
    {
      modalOp.Evaluate(rst, modalBasis);
      modalGradientOp.Evaluate(rst, modalGradient);
      numberOfTargets = numberOfFunctions;
    }
    for (int target = 0; target < numberOfTargets; ++target)
    {
      double interpolated = 0.;
      std::array<double, 3> interpolatedGradient{ { 0., 0., 0. } };
      for (int ii = 0; ii < numberOfFunctions; ++ii)
      {
        double coefficient = lattice == Lattice::Pyramid
          ? modeAtPoints[ii * numberOfFunctions + target]
          : Evaluate(terms, points[ii]);
        interpolated += coefficient * basis[ii];
        for (int axis = 0; axis < gradientOp.OperatorSize; ++axis)
        {
          interpolatedGradient[axis] += coefficient * gradient[ii * gradientOp.OperatorSize + axis];
        }
      }

      std::array<double, 3> expectedGradient;
      double expectedValue;
      std::string what;
      if (lattice == Lattice::Pyramid)
      {
        expectedValue = modalBasis[target];
        for (int axis = 0; axis < 3; ++axis)
        {
          expectedGradient[axis] = modalGradient[target * 3 + axis];
        }
        what = "mode " + vtk::to_string(target);
      }
      else
      {
        expectedValue = Evaluate(terms, rst, &expectedGradient);
        what = "polynomial";
      }

      ok &= testValue(interpolated, expectedValue, "interpolated " + what + " value");
      for (int axis = 0; axis < gradientOp.OperatorSize; ++axis)
      {
        // The fixed-order HGrad gradients report a 3-tuple per basis function
        // no matter the cell dimension, padding the unused axes with zeros;
        // the arbitrary-order ones must match that convention.
        double expected = axis < dimension ? expectedGradient[axis] : 0.;
        ok &= testValue(interpolatedGradient[axis], expected,
          "interpolated " + what + " d/dx" + vtk::to_string(axis));
      }
    }
  }

  return ok;
}

} // anonymous namespace

int TestCellGridArbitraryOrder(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool ok = true;

  vtkFiltersCellGrid::RegisterCellsAndResponders();

  vtkNew<vtkDGEdge> edge;
  vtkNew<vtkDGQuad> quad;
  vtkNew<vtkDGHex> hex;
  vtkNew<vtkDGTri> tri;
  vtkNew<vtkDGTet> tet;
  vtkNew<vtkDGWdg> wdg;
  vtkNew<vtkDGPyr> pyr;

  // Every order reaches the arbitrary-order operators, orders 1 and 2 included:
  // vtkDGCell::GetOperatorEntry() looks the basis token up before the order, and
  // the "A" token is registered at order -1 alone, so the hand-written "C"
  // operators at those orders are never in reach from here.
  //
  // No shape carries an order bound. Above order 5 the pyramid has no modal "G"
  // basis for TestBasis() to reproduce, so those orders check the Kronecker
  // delta property, partition of unity and gradient cancellation only - the
  // first of which is what a badly conditioned solve breaks soonest.
  for (int order = 0; order <= 8; ++order)
  {
    ok &= TestBasis(edge, Lattice::Prismatic, { order });
    ok &= TestBasis(quad, Lattice::Prismatic, { order, order });
    ok &= TestBasis(tri, Lattice::Simplex, { order, order });
    ok &= TestBasis(hex, Lattice::Prismatic, { order, order, order });
    ok &= TestBasis(tet, Lattice::Simplex, { order, order, order });
    ok &= TestBasis(wdg, Lattice::Wedge, { order, order, order });
    ok &= TestBasis(pyr, Lattice::Pyramid, { order, order, order });
  }

  // Anisotropic order along each parametric axis. A simplex has a single total
  // degree shared by every axis and a pyramid one degree for the whole cell, so
  // only the prismatic shapes and the wedge's t-axis admit this. The nominal
  // orders here are ones a hand-written operator implements, which the
  // arbitrary-order lattice and function count must not be confused by.
  ok &= TestBasis(quad, Lattice::Prismatic, { 2, 3 });
  ok &= TestBasis(quad, Lattice::Prismatic, { 3, 5 });
  ok &= TestBasis(hex, Lattice::Prismatic, { 4, 2, 3 });
  ok &= TestBasis(wdg, Lattice::Wedge, { 3, 3, 5 });

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
