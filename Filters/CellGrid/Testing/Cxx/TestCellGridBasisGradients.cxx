// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Check every registered HGrad basis' gradient against a central difference of
// that same basis' values, for every cell shape, basis and order.
//
// This leans on nothing but the basis under test, which is what makes it worth
// having on its own: the other basis tests check a gradient either against
// another basis' gradient (circular if that one is wrong) or through a property
// - partition of unity, polynomial reproduction - that a wrong constant factor
// on one parametric axis can survive. A factor dropped or doubled by a change
// of variables shows up here and almost nowhere else.
//
// Samples are chosen strictly inside each reference cell, since a basis' values
// outside it are not meaningful and, for the shapes whose parameter space
// collapses, not even finite.

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
#include "vtkNew.h"
#include "vtkStringFormatter.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{

/// Points strictly inside each reference cell.
///
/// The tetrahedron's and triangle's coordinates are non-negative and sum to
/// less than one; the pyramid's keep |r|, |s| well inside 1 - t; the wedge
/// pairs the triangle's with t in [-1, 1].
const std::vector<std::array<double, 3>>& SamplePoints(vtkDGCell::Shape shape)
{
  static const std::vector<std::array<double, 3>> prismatic{ { 0.13, -0.27, 0.41 },
    { -0.62, 0.35, -0.18 }, { 0.44, 0.71, -0.55 }, { -0.31, -0.09, 0.86 } };
  static const std::vector<std::array<double, 3>> simplex{ { 0.21, 0.17, 0.31 },
    { 0.54, 0.11, 0.09 }, { 0.08, 0.63, 0.12 }, { 0.3, 0.25, 0.2 } };
  static const std::vector<std::array<double, 3>> wedge{ { 0.21, 0.17, 0.35 },
    { 0.54, 0.11, -0.62 }, { 0.08, 0.63, 0.81 }, { 0.3, 0.25, -0.14 } };
  // The last of these sits near the apex, where the pyramid bases' 1/(1 - t)
  // term makes them worst conditioned - but not nearer than the differencing
  // below can follow. The clamp those bases apply at t == 1 itself is not
  // reachable this way: a central difference across that singularity is
  // meaningless, whatever the clamp does.
  static const std::vector<std::array<double, 3>> pyramid{ { 0.17, -0.22, 0.34 },
    { -0.41, 0.28, 0.12 }, { 0.05, 0.06, 0.82 }, { -0.19, -0.23, 0.55 }, { 0.02, -0.03, 0.92 } };
  switch (shape)
  {
    case vtkDGCell::Shape::Triangle:
    case vtkDGCell::Shape::Tetrahedron:
      return simplex;
    case vtkDGCell::Shape::Wedge:
      return wedge;
    case vtkDGCell::Shape::Pyramid:
      return pyramid;
    default:
      break;
  }
  return prismatic;
}

/// Central-difference every basis function of \a cellType in the basis that
/// \a basisName and \a order name, or report that there is none to check.
bool TestGradient(vtkDGCell* cellType, const std::string& basisName, int order, bool& registered)
{
  registered = false;

  vtkCellAttribute::CellTypeInfo cellTypeInfo;
  cellTypeInfo.FunctionSpace = "HGRAD";
  cellTypeInfo.Basis = basisName;
  cellTypeInfo.Order = order;

  auto basisOp = cellType->GetOperatorEntry("Basis", cellTypeInfo);
  auto gradientOp = cellType->GetOperatorEntry("BasisGradient", cellTypeInfo);
  if (!basisOp && !gradientOp)
  {
    // Neither is registered at this order, so there is nothing to check.
    // vtkDGCell::GetOperatorEntry()'s fallback to the arbitrary-order -1
    // registration is confined to the basis token asked for, so this can only
    // mean this shape does not offer this basis at this order.
    return true;
  }
  if (!basisOp || !gradientOp || basisOp.NumberOfFunctions != gradientOp.NumberOfFunctions)
  {
    // One without the other, or disagreeing about how many functions there
    // are, is itself a defect: nothing could evaluate this basis' derivatives.
    std::cerr << "  ERROR: " << cellType->GetClassName() << " \"" << basisName << "\" order "
              << order << ": basis reports " << (basisOp ? basisOp.NumberOfFunctions : -1)
              << " functions but its gradient reports "
              << (gradientOp ? gradientOp.NumberOfFunctions : -1) << ".\n";
    registered = true;
    return false;
  }
  registered = true;

  const int numberOfFunctions = basisOp.NumberOfFunctions;
  const int dimension = cellType->GetDimension();
  std::vector<double> gradient(numberOfFunctions * gradientOp.OperatorSize);
  std::vector<double> lo(numberOfFunctions * basisOp.OperatorSize);
  std::vector<double> hi(numberOfFunctions * basisOp.OperatorSize);

  bool ok = true;
  constexpr double hh = 1e-6;
  constexpr double tolerance = 1e-5;
  for (const auto& rst : SamplePoints(cellType->GetShape()))
  {
    gradientOp.Evaluate(rst, gradient);
    for (int axis = 0; axis < dimension; ++axis)
    {
      auto before = rst;
      auto after = rst;
      before[axis] -= hh;
      after[axis] += hh;
      basisOp.Evaluate(before, lo);
      basisOp.Evaluate(after, hi);
      for (int ii = 0; ii < numberOfFunctions; ++ii)
      {
        double difference = (hi[ii] - lo[ii]) / (2. * hh);
        double analytic = gradient[ii * gradientOp.OperatorSize + axis];
        // Checked before the comparison below, which a NaN would quietly pass.
        if (!std::isfinite(analytic))
        {
          std::cerr << "  ERROR: " << cellType->GetClassName() << " \"" << basisName << "\" order "
                    << order << ": d(basis " << ii << ")/dx" << axis << " is not finite ("
                    << analytic << ").\n";
          ok = false;
        }
        else if (std::abs(analytic - difference) > tolerance)
        {
          std::cerr << "  ERROR: " << cellType->GetClassName() << " \"" << basisName << "\" order "
                    << order << ": d(basis " << ii << ")/dx" << axis << " = " << analytic
                    << " but a central difference gives " << difference << ".\n";
          ok = false;
        }
      }
    }
  }
  return ok;
}

} // anonymous namespace

int TestCellGridBasisGradients(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
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
  std::vector<vtkDGCell*> cellTypes{ edge, quad, hex, tri, tet, wdg, pyr };

  // "C", "I" and "F" are the fixed-order bases, "G" the Gauss/modal ones and
  // "A" the arbitrary-order ones; whichever of them a shape does not register
  // at a given order is skipped.
  int checked = 0;
  for (auto* cellType : cellTypes)
  {
    for (const std::string basisName : { "C", "I", "F", "G", "A" })
    {
      for (int order = 0; order <= 5; ++order)
      {
        bool registered = false;
        bool passed = TestGradient(cellType, basisName, order, registered);
        ok &= passed;
        if (registered)
        {
          ++checked;
          std::cout << "  " << cellType->GetClassName() << " \"" << basisName << "\" order "
                    << order << (passed ? "\n" : " FAILED\n");
        }
      }
    }
  }

  // A registration that disappears would otherwise shrink this test's reach
  // without failing it. This is a floor, not a count to keep in step with
  // every basis added: raise it only when it stops being one.
  constexpr int leastCombinations = 90;
  std::cout << "  checked " << checked << " shape/basis/order combinations\n";
  if (checked < leastCombinations)
  {
    std::cerr << "  ERROR: only " << checked << " combinations were registered; expected at least "
              << leastCombinations << ".\n";
    ok = false;
  }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
