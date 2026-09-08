// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Verify that vtkDGLagrangePoints reports, for every registered HGrad basis, the
// parametric coordinate of each of that basis's degrees of freedom.
//
// The check is the defining property of a nodal basis and needs no reference
// data: evaluating the basis at the points it reports must give the identity
// matrix. That is far stronger than it looks, because it fails if the points
// are wrong, if there are too few or too many of them, or - the case most
// likely to slip through review - if they are listed in a different order than
// the basis evaluates its functions. Downstream code such as
// vtkCellGridChangeBasis pairs the i-th point with the i-th coefficient, so a
// permutation would silently scramble every converted cell.
//
// Every (basis, order, cell type) combination registered for the HGrad function
// space is discovered from vtkDGCell's operator map rather than listed here, so
// a newly registered basis is covered the moment it is added.

#include "vtkCellAttribute.h"
#include "vtkDGCell.h"
#include "vtkDGEdge.h"
#include "vtkDGHex.h"
#include "vtkDGLagrangePoints.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDGPyr.h"
#include "vtkDGQuad.h"
#include "vtkDGTet.h"
#include "vtkDGTri.h"
#include "vtkDGVert.h"
#include "vtkDGWdg.h"
#include "vtkFiltersCellGrid.h"
#include "vtkNew.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace
{

constexpr double tolerance = 1e-9;

/// Bases that are not nodal, and so have no points for this calculator to
/// report. The calculator must refuse these rather than invent points, and the
/// test fails if the set ever changes in either direction.
///
/// The 18-node quadratic pyramid is one: its face functions reach just 8/9 at
/// the corresponding face centroid, with the surrounding corner and edge
/// functions taking up the remainder, so its degrees of freedom are not values
/// the function takes on anywhere.
///
/// The tetrahedron's and pyramid's "G" bases are the others. The G bases are
/// nodal on Gauss points for the edge, triangle, quadrilateral, hexahedron and
/// wedge, but on these two shapes they are built from Jacobi polynomials and
/// are modal: no point of the cell exists where one function is 1 and the rest
/// are 0. Sampling the reference tetrahedron uniformly shows functions whose
/// greatest value over the whole cell is below 1 (0.98 at G1, falling to 0.54
/// by G5), so there is nothing for the calculator to report.
bool HasNoNodalPoints(vtkDGCell* cellType, vtkStringToken basis, int order)
{
  using namespace vtk::literals;
  auto shape = cellType->GetShape();
  if (shape == vtkDGCell::Shape::Pyramid && basis == "C"_token && order == 2)
  {
    return true;
  }
  return basis == "G"_token &&
    (shape == vtkDGCell::Shape::Tetrahedron || shape == vtkDGCell::Shape::Pyramid);
}

/// Check one basis: the points reported for it must be its Lagrange points.
bool TestOneBasis(vtkDGCell* cellType, vtkStringToken basis, int order)
{
  bool ok = true;
  bool expectNoPoints = HasNoNodalPoints(cellType, basis, order);
  std::cout << "  " << cellType->GetClassName() << " " << basis.Data() << order
            << (expectNoPoints ? " (expected to have no nodal points)" : "") << "\n";

  vtkNew<vtkCellAttribute> attribute;
  attribute->Initialize("test", "ℝ¹", 1);
  vtkCellAttribute::CellTypeInfo cellTypeInfo;
  cellTypeInfo.FunctionSpace = "HGRAD";
  cellTypeInfo.Basis = basis;
  cellTypeInfo.Order = order;
  attribute->SetCellTypeInfo(cellType->GetClassName(), cellTypeInfo);

  auto basisOp = cellType->GetOperatorEntry("Basis", cellTypeInfo);
  if (!basisOp)
  {
    std::cerr << "  ERROR: No basis operator.\n";
    return false;
  }

  vtkNew<vtkDGLagrangePoints> prototype;
  auto calculator = prototype->Prepare<vtkLagrangePoints>(cellType, attribute);
  if (!calculator)
  {
    std::cerr << "  ERROR: Could not prepare a vtkLagrangePoints calculator.\n";
    return false;
  }

  std::vector<std::vector<double>> points;
  bool gotPoints = calculator->GetParameters(points);
  if (expectNoPoints)
  {
    if (gotPoints)
    {
      std::cerr << "  ERROR: Reported points for a basis that has none. If this basis has "
                   "since been given a nodal point set, remove it from HasNoNodalPoints().\n";
      return false;
    }
    return true;
  }
  if (!gotPoints)
  {
    std::cerr << "  ERROR: Could not obtain Lagrange points.\n";
    return false;
  }

  if (static_cast<int>(points.size()) != basisOp.NumberOfFunctions)
  {
    std::cerr << "  ERROR: Reported " << points.size() << " points for a basis of "
              << basisOp.NumberOfFunctions << " functions.\n";
    return false;
  }

  int dimension = cellType->GetDimension();
  std::vector<double> values(basisOp.NumberOfFunctions * basisOp.OperatorSize);
  for (int ii = 0; ii < basisOp.NumberOfFunctions; ++ii)
  {
    if (static_cast<int>(points[ii].size()) != dimension)
    {
      std::cerr << "  ERROR: Point " << ii << " has " << points[ii].size()
                << " coordinates; expected " << dimension << ".\n";
      ok = false;
      continue;
    }
    std::array<double, 3> rst{ { 0., 0., 0. } };
    for (int axis = 0; axis < dimension; ++axis)
    {
      rst[axis] = points[ii][axis];
    }

    basisOp.Evaluate(rst, values);
    for (int jj = 0; jj < basisOp.NumberOfFunctions; ++jj)
    {
      double expected = ii == jj ? 1. : 0.;
      if (std::abs(values[jj] - expected) > tolerance)
      {
        std::cerr << "  ERROR: basis " << jj << " is " << values[jj] << " at point " << ii
                  << "; expected " << expected << ".\n";
        ok = false;
      }
    }
  }
  return ok;
}

} // anonymous namespace

int TestCellGridLagrangePoints(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  using namespace vtk::literals;
  bool ok = true;

  vtkFiltersCellGrid::RegisterCellsAndResponders();

  vtkNew<vtkDGVert> vert;
  vtkNew<vtkDGEdge> edge;
  vtkNew<vtkDGTri> tri;
  vtkNew<vtkDGQuad> quad;
  vtkNew<vtkDGTet> tet;
  vtkNew<vtkDGHex> hex;
  vtkNew<vtkDGWdg> wdg;
  vtkNew<vtkDGPyr> pyr;
  std::map<vtkStringToken, vtkDGCell*> cellTypes{ { "vtkDGVert"_token, vert },
    { "vtkDGEdge"_token, edge }, { "vtkDGTri"_token, tri }, { "vtkDGQuad"_token, quad },
    { "vtkDGTet"_token, tet }, { "vtkDGHex"_token, hex }, { "vtkDGWdg"_token, wdg },
    { "vtkDGPyr"_token, pyr } };

  // Walk every basis registered for the HGrad function space rather than
  // listing them, so that a basis added later is covered automatically.
  auto& hgradBases = vtkDGCell::GetOperators()["Basis"_token]["HGRAD"_token];
  int tested = 0;
  for (const auto& basisEntry : hgradBases)
  {
    for (const auto& orderEntry : basisEntry.second)
    {
      for (const auto& cellEntry : orderEntry.second)
      {
        auto it = cellTypes.find(cellEntry.first);
        if (it == cellTypes.end())
        {
          std::cerr << "  ERROR: Unknown cell type \"" << cellEntry.first.Data() << "\".\n";
          ok = false;
          continue;
        }
        if (orderEntry.first >= 0)
        {
          // A basis registered for one specific order.
          ok &= TestOneBasis(it->second, basisEntry.first, orderEntry.first);
          ++tested;
          continue;
        }
        // A basis accepting any order. Orders 1 and 2 are included even though
        // most shapes have a hand-written operator at those orders: a shape that
        // does not - the vertex - must still resolve to this one rather than
        // finding the order registered but its own shape missing from it.
        for (int order : { 0, 1, 2, 3, 4, 5 })
        {
          ok &= TestOneBasis(it->second, basisEntry.first, order);
          ++tested;
        }
      }
    }
  }

  if (tested == 0)
  {
    std::cerr << "ERROR: No bases were tested.\n";
    ok = false;
  }
  std::cout << tested << " bases tested.\n";

  std::cout << (ok ? "PASS\n" : "FAIL\n");
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
