// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise vtkCellGridChangeBasis by converting HGrad (Lagrange) attributes to
// the Bernstein-Bezier basis of the same order.
//
// A change of basis rewrites the coefficients but must not change the function
// they describe, so the test evaluates each attribute at a spread of points
// inside every cell before and after the conversion and requires the values to
// agree. That is the property the whole exercise depends on, and it is checked
// through the same interpolation machinery any consumer would use rather than
// against a table of expected numbers.
//
// It also checks the property the Bezier basis is chosen for: the function over
// a cell stays within the range spanned by that cell's control values. Without
// it there would be no point converting.
//
// The input has both a shared-degree-of-freedom attribute and unshared ones, so
// both layouts are covered, and each must keep its own: a shared attribute stays
// shared, which is only correct if every cell meeting at a shared value agrees
// on it.

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridChangeBasis.h"
#include "vtkCellGridReader.h"
#include "vtkDGCell.h"
#include "vtkFiltersCellGrid.h"
#include "vtkInterpolateCalculator.h"
#include "vtkNew.h"
#include "vtkStringToken.h"
#include "vtkTestUtilities.h"
#include "vtkVector.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
using namespace vtk::literals;

constexpr double tolerance = 1e-9;

/// Parametric coordinates spread through a cell, including its boundary.
///
/// The quadrilateral is parameterized over [-1, 1] along both axes.
const std::vector<vtkVector3d>& SamplePoints()
{
  static const std::vector<vtkVector3d> points{ { 0., 0., 0. }, { -1., -1., 0. }, { 1., -1., 0. },
    { 1., 1., 0. }, { -1., 1., 0. }, { 0.3, -0.7, 0. }, { -0.85, 0.2, 0. }, { 0.55, 0.55, 0. },
    { -0.4, -0.15, 0. }, { 0.9, -0.05, 0. } };
  return points;
}

/// Evaluate \a attributeName over every cell of \a grid at SamplePoints().
bool Sample(vtkCellGrid* grid, const std::string& attributeName, std::vector<double>& values)
{
  values.clear();
  auto* attribute = grid->GetCellAttributeByName(attributeName);
  if (!attribute)
  {
    std::cerr << "  ERROR: No attribute named \"" << attributeName << "\".\n";
    return false;
  }
  for (const auto& cellTypeName : grid->CellTypeArray())
  {
    auto* cellType = vtkDGCell::SafeDownCast(grid->GetCellType(cellTypeName));
    if (!cellType)
    {
      continue;
    }
    auto calculator = cellType->GetCaches()->AttributeCalculator<vtkInterpolateCalculator>(
      cellType, attribute, cellType->GetAttributeTags(attribute, true));
    if (!calculator)
    {
      std::cerr << "  ERROR: No interpolation calculator for \"" << attributeName << "\".\n";
      return false;
    }
    vtkIdType numCells = cellType->GetNumberOfCells();
    std::vector<double> value;
    for (vtkIdType cell = 0; cell < numCells; ++cell)
    {
      for (const auto& rst : SamplePoints())
      {
        calculator->Evaluate(cell, rst, value);
        values.insert(values.end(), value.begin(), value.end());
      }
    }
  }
  return !values.empty();
}

/// Check that the function over each cell stays inside the range spanned by
/// that cell's control values, which is what the Bezier basis is chosen for.
bool TestConvexHull(vtkCellGrid* grid, const std::string& attributeName, bool expectShared)
{
  bool ok = true;
  auto* attribute = grid->GetCellAttributeByName(attributeName);
  for (const auto& cellTypeName : grid->CellTypeArray())
  {
    auto* cellType = vtkDGCell::SafeDownCast(grid->GetCellType(cellTypeName));
    if (!cellType)
    {
      continue;
    }
    auto cellTypeInfo = attribute->GetCellTypeInfo(cellTypeName);
    auto* coefficients = cellTypeInfo.GetArrayForRoleAs<vtkDataArray>("values");
    auto* connectivity = cellTypeInfo.GetArrayForRoleAs<vtkDataArray>("connectivity");
    if (!coefficients)
    {
      std::cerr << "  ERROR: The converted attribute has no values.\n";
      return false;
    }
    if (cellTypeInfo.DOFSharing.IsValid() != expectShared)
    {
      std::cerr << "  ERROR: The converted attribute " << (expectShared ? "should" : "should not")
                << " share its degrees of freedom.\n";
      return false;
    }
    if (expectShared && !connectivity)
    {
      std::cerr << "  ERROR: A shared attribute needs a connectivity array.\n";
      return false;
    }
    auto calculator = cellType->GetCaches()->AttributeCalculator<vtkInterpolateCalculator>(
      cellType, attribute, cellType->GetAttributeTags(attribute, true));
    int numControls =
      expectShared ? connectivity->GetNumberOfComponents() : coefficients->GetNumberOfComponents();
    vtkIdType numCells =
      expectShared ? connectivity->GetNumberOfTuples() : coefficients->GetNumberOfTuples();
    std::vector<double> control(numControls);
    std::vector<double> dofIds(numControls);
    std::vector<double> value;
    for (vtkIdType cell = 0; cell < numCells; ++cell)
    {
      if (expectShared)
      {
        connectivity->GetTuple(cell, dofIds.data());
        for (int ii = 0; ii < numControls; ++ii)
        {
          control[ii] = coefficients->GetComponent(static_cast<vtkIdType>(dofIds[ii]), 0);
        }
      }
      else
      {
        coefficients->GetTuple(cell, control.data());
      }
      double lo = control[0];
      double hi = control[0];
      for (int ii = 1; ii < numControls; ++ii)
      {
        lo = std::min(lo, control[ii]);
        hi = std::max(hi, control[ii]);
      }
      for (const auto& rst : SamplePoints())
      {
        calculator->Evaluate(cell, rst, value);
        if (value[0] < lo - tolerance || value[0] > hi + tolerance)
        {
          std::cerr << "  ERROR: cell " << cell << " evaluates to " << value[0]
                    << ", outside the control values' range [" << lo << ", " << hi << "].\n";
          ok = false;
        }
      }
    }
  }
  return ok;
}

/// Convert one attribute to the Bezier basis and verify nothing else changed.
bool TestConversion(
  const char* filename, const std::string& attributeName, bool expectShared, int targetOrder = 2)
{
  std::cout << "## " << attributeName << (expectShared ? " (shared)" : " (per cell)")
            << " to Bezier order " << targetOrder << "\n";

  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  auto* input = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!input)
  {
    std::cerr << "  ERROR: Could not read " << filename << ".\n";
    return false;
  }

  std::vector<double> before;
  if (!Sample(input, attributeName, before))
  {
    return false;
  }

  vtkNew<vtkCellGridChangeBasis> converter;
  converter->SetInputDataObject(input);
  converter->SetCellAttribute(input->GetCellAttributeByName(attributeName));
  converter->SetSourceFunctionSpace("HGRAD");
  converter->SetSourceBasis("C");
  converter->SetSourceOrder(2);
  converter->SetTargetFunctionSpace("Bezier");
  converter->SetTargetBasis("A");
  converter->SetTargetOrder(targetOrder);
  converter->Update();
  auto* output = vtkCellGrid::SafeDownCast(converter->GetOutputDataObject(0));
  if (!output)
  {
    std::cerr << "  ERROR: The filter produced no output.\n";
    return false;
  }

  bool ok = true;

  // The conversion must not have disturbed the input.
  std::vector<double> inputAfter;
  if (!Sample(input, attributeName, inputAfter) || inputAfter != before)
  {
    std::cerr << "  ERROR: The filter modified its own input.\n";
    ok = false;
  }

  auto outputInfo = output->GetCellAttributeByName(attributeName)->GetCellTypeInfo("vtkDGQuad");
  if (outputInfo.FunctionSpace != "Bezier"_token || outputInfo.Order != targetOrder)
  {
    std::cerr << "  ERROR: The output attribute is not in the Bezier basis.\n";
    return false;
  }

  std::vector<double> after;
  if (!Sample(output, attributeName, after))
  {
    return false;
  }
  if (after.size() != before.size())
  {
    std::cerr << "  ERROR: Sampled " << after.size() << " values after conversion but "
              << before.size() << " before.\n";
    return false;
  }
  for (std::size_t ii = 0; ii < before.size(); ++ii)
  {
    if (std::abs(after[ii] - before[ii]) > tolerance)
    {
      std::cerr << "  ERROR: Sample " << ii << " changed from " << before[ii] << " to " << after[ii]
                << ".\n";
      ok = false;
    }
  }

  ok &= TestConvexHull(output, attributeName, expectShared);
  return ok;
}

} // anonymous namespace

int TestCellGridChangeBasis(int argc, char* argv[])
{
  bool ok = true;

  vtkFiltersCellGrid::RegisterCellsAndResponders();

  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgQuadraticQuadrilaterals.dg", 0);

  // "scalar0" holds its degrees of freedom per cell; "scalar3" shares them
  // between cells. Both must convert to the same unshared Bezier form.
  ok &= TestConversion(filename, "scalar0", /* expectShared */ false);
  ok &= TestConversion(filename, "scalar3", /* expectShared */ true);

  // Raising the order is exact, since the target spans everything the source
  // does. The two bases no longer place their degrees of freedom at the same
  // points, so a shared attribute cannot stay shared.
  ok &= TestConversion(filename, "scalar0", /* expectShared */ false, /* targetOrder */ 3);
  ok &= TestConversion(filename, "scalar3", /* expectShared */ false, /* targetOrder */ 4);

  delete[] filename;

  std::cout << (ok ? "PASS\n" : "FAIL\n");
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
