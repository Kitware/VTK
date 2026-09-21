// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise the arbitrary-order ("A") HGrad basis functions registered for the
// DG cell shapes: vertex, edge, quadrilateral, hexahedron, triangle,
// tetrahedron, and wedge. (The pyramid has no arbitrary-order Lagrange basis; a
// polynomial one does not exist, since the tensor-product structure degenerates
// at the apex.)
//
// The properties checked here define a nodal Lagrange basis, so together they
// pin the basis down completely:
//
// + partition of unity - the basis functions sum to 1 everywhere;
// + the Kronecker delta property - the i-th function is 1 at the i-th Lagrange
//   point and 0 at every other one;
// + polynomial reproduction - interpolating a polynomial drawn from the space
//   the basis spans reproduces it exactly, along with its gradient.
//
// The last of these is the property downstream code relies on and is by far the
// most sensitive: an error anywhere in a basis function or its gradient shows
// up as a mismatch at parametric coordinates away from the Lagrange points.
//
// Note that the Cn bases number their degrees of freedom lexicographically (the
// r-axis varying fastest), which is *not* the corner-first ordering used by the
// fixed-order HGrad bases (C1, C2). The Lagrange points below follow the Cn
// convention.

#include "vtkDGCell.h"
#include "vtkDGEdge.h"
#include "vtkDGHex.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDGQuad.h"
#include "vtkDGTet.h"
#include "vtkDGTri.h"
#include "vtkDGWdg.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkStringFormatter.h"

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
  Wedge      //!< A triangle's principal lattice times an edge's points.
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
  switch (lattice)
  {
    case Lattice::Simplex:
      return simplex;
    case Lattice::Wedge:
      return wedge;
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

  auto terms = PolynomialTerms(lattice, order, dimension);
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

    // Polynomial reproduction: interpolating a polynomial from the space the
    // basis spans, given its values at the Lagrange points, must reproduce the
    // polynomial and its gradient.
    double interpolated = 0.;
    std::array<double, 3> interpolatedGradient{ { 0., 0., 0. } };
    for (int ii = 0; ii < numberOfFunctions; ++ii)
    {
      double coefficient = Evaluate(terms, points[ii]);
      interpolated += coefficient * basis[ii];
      for (int axis = 0; axis < gradientOp.OperatorSize; ++axis)
      {
        interpolatedGradient[axis] += coefficient * gradient[ii * gradientOp.OperatorSize + axis];
      }
    }
    std::array<double, 3> expectedGradient;
    ok &= testValue(interpolated, Evaluate(terms, rst, &expectedGradient), "interpolated value");
    for (int axis = 0; axis < gradientOp.OperatorSize; ++axis)
    {
      // The fixed-order HGrad gradients report a 3-tuple per basis function no
      // matter the cell dimension, padding the unused axes with zeros; the
      // arbitrary-order ones must match that convention.
      double expected = axis < dimension ? expectedGradient[axis] : 0.;
      ok &=
        testValue(interpolatedGradient[axis], expected, "interpolated d/dx" + vtk::to_string(axis));
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

  // Orders 1 and 2 have hand-written operators, so the arbitrary-order ones are
  // only reachable from order 3 up. Order 0 has no fixed-order registration and
  // does reach them.
  for (int order = 0; order <= 6; ++order)
  {
    if (order == 1 || order == 2)
    {
      continue;
    }
    ok &= TestBasis(edge, Lattice::Prismatic, { order });
    ok &= TestBasis(quad, Lattice::Prismatic, { order, order });
    ok &= TestBasis(tri, Lattice::Simplex, { order, order });
    // Keep the volumetric shapes' orders low enough that the test stays quick;
    // an order-6 hexahedron has 343 basis functions evaluated at 343 points.
    if (order <= 4)
    {
      ok &= TestBasis(hex, Lattice::Prismatic, { order, order, order });
      ok &= TestBasis(tet, Lattice::Simplex, { order, order, order });
      ok &= TestBasis(wdg, Lattice::Wedge, { order, order, order });
    }
  }

  // Anisotropic order along each parametric axis. A simplex has a single total
  // degree, so only the prismatic shapes and the wedge's t-axis admit this.
  // A nominal order that a hand-written operator implements, but with axes that
  // disagree: the fixed-order operator must not be chosen, since it implements
  // one order along every axis.
  ok &= TestBasis(quad, Lattice::Prismatic, { 2, 3 });
  ok &= TestBasis(quad, Lattice::Prismatic, { 3, 5 });
  ok &= TestBasis(hex, Lattice::Prismatic, { 4, 2, 3 });
  ok &= TestBasis(wdg, Lattice::Wedge, { 3, 3, 5 });

  std::cout << (ok ? "PASS\n" : "FAIL\n");
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
