// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise the arbitrary-order Bernstein-Bezier basis functions registered for
// the DG cell shapes: vertex, edge, quadrilateral, hexahedron, triangle,
// tetrahedron, and wedge. (The pyramid is absent for the same reason it has no
// arbitrary-order Lagrange basis; see TestCellGridArbitraryOrder.)
//
// A Bernstein-Bezier basis is not nodal, so most of the checks in
// TestCellGridArbitraryOrder do not apply: away from a cell's corners the
// functions do not interpolate their degrees of freedom, so there is no
// Kronecker delta property and no lattice of points to interpolate at. What is
// checked here instead is what the basis is chosen for:
//
// + partition of unity - the basis functions sum to 1 everywhere;
// + non-negativity throughout the reference cell, which together with the
//   above is what puts the function inside the convex hull of its control
//   values, the property downstream isocontouring relies on;
// + corner interpolation - at each corner of the reference cell exactly one
//   function is 1 and the rest are 0, so control values at the corners are
//   attained;
// + linear precision - control values placed at the domain points b/n
//   reproduce the identity map, which pins down the indexing and the
//   multinomial normalization;
// + gradients that agree with central differences of the basis functions
//   themselves, which is what catches a missing or spurious factor from the
//   change of variables between [-1, 1] and the [0, 1] the Bernstein
//   polynomials are defined over.

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

/// How a cell shape's control points are laid out in its parameter space.
enum class Layout
{
  Prismatic, //!< A tensor product with an independent order along each axis.
  Simplex,   //!< A barycentric multi-index with one degree shared by all axes.
  Wedge      //!< A triangle's multi-index times an edge's control points.
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

/// The parametric coordinate of the \a ii-th of \a order + 1 control points
/// spanning the [-1, 1] range a prismatic axis is parameterized over.
double PrismaticCoordinate(int ii, int order)
{
  return order > 0 ? -1. + 2. * ii / order : 0.;
}

/// Return the domain point of each control value, in the order the basis
/// functions are evaluated.
///
/// These are not points the basis interpolates - a Bernstein-Bezier basis does
/// not, except at the corners - but the points whose control values reproduce
/// the identity map. Axes the cell does not parameterize are held at 0.
std::vector<std::array<double, 3>> DomainPoints(
  Layout layout, const std::vector<int>& order, int dimension)
{
  std::vector<std::array<double, 3>> points;
  switch (layout)
  {
    case Layout::Prismatic:
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

    case Layout::Simplex:
    {
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

    case Layout::Wedge:
    {
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

/// The polynomial order governing one parametric \a axis of a shape.
///
/// A simplex shares one order across all of its axes; a wedge shares one across
/// its two triangular axes and has another along t; a prismatic shape has an
/// independent order per axis.
int AxisOrder(Layout layout, const std::vector<int>& order, int axis)
{
  switch (layout)
  {
    case Layout::Simplex:
      return order[0];
    case Layout::Wedge:
      return axis < 2 ? order[0] : order[2];
    default:
      break;
  }
  return order[axis];
}

/// The corners of the reference cell, where a Bernstein-Bezier basis does
/// interpolate its control values.
std::vector<std::array<double, 3>> Corners(Layout layout, int dimension)
{
  std::vector<std::array<double, 3>> corners;
  switch (layout)
  {
    case Layout::Prismatic:
      corners.push_back({ { 0., 0., 0. } });
      for (int axis = 0; axis < dimension; ++axis)
      {
        std::vector<std::array<double, 3>> expanded;
        for (double coord : { -1., 1. })
        {
          for (const auto& corner : corners)
          {
            auto next = corner;
            next[axis] = coord;
            expanded.push_back(next);
          }
        }
        corners = expanded;
      }
      break;
    case Layout::Simplex:
      corners.push_back({ { 0., 0., 0. } });
      corners.push_back({ { 1., 0., 0. } });
      corners.push_back({ { 0., 1., 0. } });
      if (dimension > 2)
      {
        corners.push_back({ { 0., 0., 1. } });
      }
      break;
    case Layout::Wedge:
      for (double zz : { -1., 1. })
      {
        corners.push_back({ { 0., 0., zz } });
        corners.push_back({ { 1., 0., zz } });
        corners.push_back({ { 0., 1., zz } });
      }
      break;
  }
  return corners;
}

/// Points at which to check the properties that hold throughout the cell.
const std::vector<std::array<double, 3>>& SamplePoints(Layout layout)
{
  static const std::vector<std::array<double, 3>> prismatic{ { 0., 0., 0. }, { 0.3, -0.7, 0.55 },
    { -0.9, 0.15, -0.25 }, { 0.62, 0.62, 0.62 }, { -0.43, -0.11, 0.87 }, { 0.98, -0.98, 0.02 } };
  static const std::vector<std::array<double, 3>> simplex{ { 0.25, 0.25, 0.25 }, { 0.1, 0.2, 0.3 },
    { 0.7, 0.15, 0.05 }, { 0.05, 0.6, 0.2 }, { 0.33, 0.33, 0.01 }, { 0.02, 0.03, 0.9 } };
  static const std::vector<std::array<double, 3>> wedge{ { 0.25, 0.25, 0. }, { 0.1, 0.2, -0.65 },
    { 0.7, 0.15, 0.4 }, { 0.05, 0.6, 0.93 }, { 0.33, 0.33, -0.87 }, { 0.02, 0.03, 0.11 } };
  switch (layout)
  {
    case Layout::Simplex:
      return simplex;
    case Layout::Wedge:
      return wedge;
    default:
      break;
  }
  return prismatic;
}

/// Check every property of the Bezier basis for one cell shape at one order.
bool TestBasis(vtkDGCell* cellType, Layout layout, const std::vector<int>& order)
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
  cellTypeInfo.FunctionSpace = "Bezier";
  cellTypeInfo.Basis = "A";
  cellTypeInfo.Order = order.empty() ? 0 : order[0];

  bool anisotropic = false;
  for (int axisOrder : order)
  {
    anisotropic |= axisOrder != order[0];
  }
  if (anisotropic)
  {
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
    std::cerr << "  ERROR: No Bezier basis registered.\n";
    return false;
  }

  auto points = DomainPoints(layout, order, dimension);
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
  std::vector<double> shifted(numberOfFunctions * basisOp.OperatorSize);

  // At each corner of the reference cell exactly one basis function is 1 and
  // the rest are 0, so control values at the corners are attained exactly.
  for (const auto& corner : Corners(layout, dimension))
  {
    basisOp.Evaluate(corner, basis);
    int atOne = 0;
    for (int ii = 0; ii < numberOfFunctions; ++ii)
    {
      if (std::abs(basis[ii] - 1.) <= tolerance)
      {
        ++atOne;
      }
      else if (std::abs(basis[ii]) > tolerance)
      {
        std::cerr << "  ERROR: basis " << ii << " is " << basis[ii]
                  << " at a corner; expected 0 or 1.\n";
        ok = false;
      }
    }
    if (atOne != 1)
    {
      std::cerr << "  ERROR: " << atOne << " basis functions equal 1 at a corner; expected 1.\n";
      ok = false;
    }
  }

  for (const auto& rst : SamplePoints(layout))
  {
    basisOp.Evaluate(rst, basis);
    gradientOp.Evaluate(rst, gradient);

    // Partition of unity, and non-negativity throughout the reference cell.
    double sum = 0.;
    for (int ii = 0; ii < numberOfFunctions; ++ii)
    {
      sum += basis[ii];
      if (basis[ii] < -tolerance)
      {
        std::cerr << "  ERROR: basis " << ii << " is negative (" << basis[ii]
                  << ") inside the reference cell.\n";
        ok = false;
      }
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

    // Linear precision: control values placed at the domain points reproduce
    // the identity map, so the interpolated position is the point itself and
    // the interpolated Jacobian is the identity matrix.
    for (int axis = 0; axis < dimension; ++axis)
    {
      if (AxisOrder(layout, order, axis) < 1)
      {
        // A degree-0 basis along this axis is a single constant, which cannot
        // reproduce a coordinate that varies. There is nothing to check.
        continue;
      }
      double position = 0.;
      std::array<double, 3> jacobianRow{ { 0., 0., 0. } };
      for (int ii = 0; ii < numberOfFunctions; ++ii)
      {
        position += points[ii][axis] * basis[ii];
        for (int other = 0; other < gradientOp.OperatorSize; ++other)
        {
          jacobianRow[other] += points[ii][axis] * gradient[ii * gradientOp.OperatorSize + other];
        }
      }
      ok &= testValue(position, rst[axis], "interpolated coordinate " + vtk::to_string(axis));
      for (int other = 0; other < gradientOp.OperatorSize; ++other)
      {
        ok &= testValue(jacobianRow[other], axis == other ? 1. : 0.,
          "Jacobian entry (" + vtk::to_string(axis) + "," + vtk::to_string(other) + ")");
      }
    }

    // Every basis function's gradient must match a central difference of that
    // function. This is what catches a wrong constant factor from the change of
    // variables, which the checks above are all blind to.
    constexpr double hh = 1e-6;
    for (int axis = 0; axis < dimension; ++axis)
    {
      auto lo = rst;
      auto hi = rst;
      lo[axis] -= hh;
      hi[axis] += hh;
      basisOp.Evaluate(lo, basis);
      basisOp.Evaluate(hi, shifted);
      for (int ii = 0; ii < numberOfFunctions; ++ii)
      {
        double difference = (shifted[ii] - basis[ii]) / (2. * hh);
        double analytic = gradient[ii * gradientOp.OperatorSize + axis];
        if (std::abs(analytic - difference) > 1e-6)
        {
          std::cerr << "  ERROR: d(basis " << ii << ")/dx" << axis << " = " << analytic
                    << " but a central difference gives " << difference << ".\n";
          ok = false;
        }
      }
    }
  }

  return ok;
}

} // anonymous namespace

int TestCellGridBezierBasis(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool ok = true;

  vtkFiltersCellGrid::RegisterCellsAndResponders();

  vtkNew<vtkDGEdge> edge;
  vtkNew<vtkDGQuad> quad;
  vtkNew<vtkDGHex> hex;
  vtkNew<vtkDGTri> tri;
  vtkNew<vtkDGTet> tet;
  vtkNew<vtkDGWdg> wdg;

  // Unlike the Lagrange bases there are no hand-written low-order operators to
  // take precedence, so every order reaches the arbitrary-order kernels.
  for (int order = 0; order <= 5; ++order)
  {
    ok &= TestBasis(edge, Layout::Prismatic, { order });
    ok &= TestBasis(quad, Layout::Prismatic, { order, order });
    ok &= TestBasis(tri, Layout::Simplex, { order, order });
    if (order <= 4)
    {
      ok &= TestBasis(hex, Layout::Prismatic, { order, order, order });
      ok &= TestBasis(tet, Layout::Simplex, { order, order, order });
      ok &= TestBasis(wdg, Layout::Wedge, { order, order, order });
    }
  }

  // Anisotropic order along each parametric axis. A simplex has a single total
  // degree, so only the prismatic shapes and the wedge's t-axis admit this.
  ok &= TestBasis(quad, Layout::Prismatic, { 3, 5 });
  ok &= TestBasis(hex, Layout::Prismatic, { 4, 2, 3 });
  ok &= TestBasis(wdg, Layout::Wedge, { 3, 3, 5 });

  std::cout << (ok ? "PASS\n" : "FAIL\n");
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
