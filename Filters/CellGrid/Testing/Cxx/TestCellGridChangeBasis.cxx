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
#include "vtkDGOperatorEntry.h"
#include "vtkFiltersCellGrid.h"
#include "vtkInterpolateCalculator.h"
#include "vtkNew.h"
#include "vtkStringFormatter.h"
#include "vtkStringToken.h"
#include "vtkTestUtilities.h"
#include "vtkVector.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
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

/// Parametric coordinates spread through a pyramid, including its boundary,
/// but always strictly inside |r|, |s| <= w = 1 - t so that no rational term
/// this shape's bases carry (see PyrCnBasis.h) is ever asked to divide by
/// zero.
const std::vector<vtkVector3d>& PyramidSamplePoints()
{
  static const std::vector<vtkVector3d> points{ { 0., 0., 0. }, { -1., -1., 0. }, { 1., -1., 0. },
    { 1., 1., 0. }, { -1., 1., 0. }, { 0., 0., 1. }, { 0.2, -0.1, 0.3 }, { -0.5, 0.4, 0.1 },
    { 0.05, 0.05, 0.9 }, { -0.3, -0.3, 0.6 } };
  return points;
}

/// Parametric coordinates spread through one reference cell of \a shape,
/// including its boundary, for the sweep below - which meets every shape and
/// so cannot use one set. A sample has to lie inside the cell for the value
/// there to mean anything, and the pyramid's must also keep |r|, |s| <= 1 - t.
const std::vector<vtkVector3d>& ShapeSamplePoints(vtkDGCell::Shape shape)
{
  static const std::vector<vtkVector3d> vertex{ { 0., 0., 0. } };
  static const std::vector<vtkVector3d> edge{ { -1., 0., 0. }, { -0.6, 0., 0. }, { 0., 0., 0. },
    { 0.35, 0., 0. }, { 1., 0., 0. } };
  static const std::vector<vtkVector3d> hexahedron{ { 0., 0., 0. }, { -1., -1., -1. },
    { 1., 1., 1. }, { 0.3, -0.7, 0.55 }, { -0.9, 0.15, -0.25 }, { 0.62, 0.62, 0.62 },
    { -0.43, -0.11, 0.87 } };
  // Non-negative coordinates summing to at most one, so inside the simplex.
  static const std::vector<vtkVector3d> triangle{ { 0., 0., 0. }, { 1., 0., 0. }, { 0., 1., 0. },
    { 1. / 3., 1. / 3., 0. }, { 0.1, 0.2, 0. }, { 0.7, 0.15, 0. }, { 0.05, 0.6, 0. } };
  static const std::vector<vtkVector3d> tetrahedron{ { 0., 0., 0. }, { 1., 0., 0. }, { 0., 1., 0. },
    { 0., 0., 1. }, { 0.25, 0.25, 0.25 }, { 0.1, 0.2, 0.3 }, { 0.7, 0.15, 0.05 } };
  // The triangle's samples paired with coordinates along t in [-1, 1].
  static const std::vector<vtkVector3d> wedge{ { 0., 0., -1. }, { 1., 0., 1. }, { 0., 1., 0. },
    { 1. / 3., 1. / 3., -0.65 }, { 0.1, 0.2, 0.4 }, { 0.7, 0.15, 0.93 }, { 0.05, 0.6, -0.87 } };
  switch (shape)
  {
    case vtkDGCell::Shape::Vertex:
      return vertex;
    case vtkDGCell::Shape::Edge:
      return edge;
    case vtkDGCell::Shape::Triangle:
      return triangle;
    case vtkDGCell::Shape::Tetrahedron:
      return tetrahedron;
    case vtkDGCell::Shape::Hexahedron:
      return hexahedron;
    case vtkDGCell::Shape::Wedge:
      return wedge;
    case vtkDGCell::Shape::Pyramid:
      return PyramidSamplePoints();
    case vtkDGCell::Shape::Quadrilateral:
    default:
      break;
  }
  return SamplePoints();
}

/// Evaluate \a attributeName over every cell of \a grid at \a samples, or - if
/// \a samples is empty - at each cell type's own ShapeSamplePoints().
bool Sample(vtkCellGrid* grid, const std::string& attributeName, std::vector<double>& values,
  const std::vector<vtkVector3d>& samples = SamplePoints())
{
  values.clear();
  auto* attribute = grid->GetCellAttributeByName(attributeName);
  if (!attribute)
  {
    std::cerr << "  ERROR: No attribute named \"" << attributeName << "\".\n";
    return false;
  }
  // By name, not in the grid's own order: a filter may hand its output's cell
  // types back in a different one (vtkCellGridChangeBasis does, on dgMixed.dg),
  // and these values are compared between two grids position by position.
  // CellTypeArray() returns by value, so it is bound once here: calling it for
  // each iterator would leave them pointing into two different temporaries.
  std::vector<vtkStringToken> cellTypeNames = grid->CellTypeArray();
  std::sort(cellTypeNames.begin(), cellTypeNames.end(),
    [](vtkStringToken aa, vtkStringToken bb) { return aa.Data() < bb.Data(); });
  for (const auto& cellTypeName : cellTypeNames)
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
    const auto& points = samples.empty() ? ShapeSamplePoints(cellType->GetShape()) : samples;
    vtkIdType numCells = cellType->GetNumberOfCells();
    std::vector<double> value;
    for (vtkIdType cell = 0; cell < numCells; ++cell)
    {
      for (const auto& rst : points)
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
bool TestConvexHull(vtkCellGrid* grid, const std::string& attributeName, bool expectShared,
  const std::vector<vtkVector3d>& samples = SamplePoints())
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
    if (attribute->GetNumberOfComponents() != 1)
    {
      // The range below is a scalar one; a vector-valued attribute's control
      // values bound each component separately, which is not what this checks.
      continue;
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
      for (const auto& rst : samples)
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

/// What converting one attribute of a file to the Bezier basis must produce.
struct Conversion
{
  /// The order the file stores the attribute at, or notConvertible.
  int SourceOrder;
  /// The Bezier order to convert to; equal to SourceOrder unless the target
  /// has to be raised to span the source.
  int TargetOrder;
  /// Whether the result still shares its degrees of freedom between cells,
  /// which it can only do when both bases place them at the same points.
  bool Shared;
};

/// A Conversion::SourceOrder saying no conversion of this attribute is
/// possible, so the test requires it to be skipped rather than converted.
constexpr int notConvertible = -1;

/// Convert every attribute of \a fileName to the Bezier basis and check the
/// result against \a expected, keyed by attribute name.
///
/// Every attribute the file carries must appear in \a expected, so a file that
/// gains one cannot go unnoticed, and the conversion must not change the
/// function any of them describes: each is sampled over every cell before and
/// after and the values must agree.
///
/// The files are the ones TestCellGridFindCriticalPoints reads, which converts
/// each of them to Bezier before searching - so a conversion that quietly
/// changed a field would surface there as a baseline that moved for no reason.
bool TestConversion(
  int argc, char* argv[], const char* fileName, const std::map<std::string, Conversion>& expected)
{
  char* path = vtkTestUtilities::ExpandDataFileName(argc, argv, fileName, 0);
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(path);
  reader->Update();
  delete[] path;

  auto* grid = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!grid)
  {
    std::cerr << "  ERROR: Could not read " << fileName << ".\n";
    return false;
  }

  std::cout << "## " << fileName << "\n";
  if (grid->CellTypeArray().empty() || grid->GetCellAttributeList().empty())
  {
    // The reader hands back an empty grid rather than nothing when it cannot
    // read a file, and an empty grid would otherwise pass every check below.
    std::cerr << "  ERROR: " << fileName << " yielded no cells or no attributes.\n";
    return false;
  }

  bool ok = true;
  for (const auto& attribute : grid->GetCellAttributeList())
  {
    const std::string attributeName = attribute->GetName().Data();

    auto it = expected.find(attributeName);
    if (it == expected.end())
    {
      std::cerr << "  ERROR: no expected conversion recorded for \"" << attributeName << "\".\n";
      ok = false;
      continue;
    }
    const Conversion& want = it->second;

    // An attribute need not carry the same function space, basis or order on
    // every cell type it applies to; one conversion can only name a single
    // source, so a mixed one cannot be converted at all.
    vtkStringToken space;
    vtkStringToken basis;
    int order = -1;
    bool any = false;
    bool uniform = true;
    for (const auto& cellTypeName : grid->CellTypeArray())
    {
      auto info = attribute->GetCellTypeInfo(cellTypeName);
      if (!info.FunctionSpace.IsValid())
      {
        continue;
      }
      if (!any)
      {
        space = info.FunctionSpace;
        basis = info.Basis;
        order = info.Order;
        any = true;
      }
      else
      {
        uniform &= info.FunctionSpace == space && info.Basis == basis && info.Order == order;
      }
    }

    // A shared attribute's connectivity must name one degree of freedom per
    // basis function, or the file disagrees with itself.
    std::string mismatch;
    for (const auto& cellTypeName : grid->CellTypeArray())
    {
      auto info = attribute->GetCellTypeInfo(cellTypeName);
      if (!info.FunctionSpace.IsValid() || !info.DOFSharing.IsValid())
      {
        continue;
      }
      auto* cellType = vtkDGCell::SafeDownCast(grid->GetCellType(cellTypeName));
      auto sourceOp = cellType ? cellType->GetOperatorEntry("Basis", info) : vtkDGOperatorEntry();
      auto* connectivity = info.GetArrayForRoleAs<vtkDataArray>("connectivity"_token);
      if (sourceOp && connectivity &&
        connectivity->GetNumberOfComponents() != sourceOp.NumberOfFunctions)
      {
        mismatch = vtk::to_string(connectivity->GetNumberOfComponents()) +
          " degrees of freedom per cell but a basis of " +
          vtk::to_string(sourceOp.NumberOfFunctions) + " functions";
      }
    }

    std::string skip;
    if (!any)
    {
      skip = "applies to no cell type here";
    }
    else if (!uniform)
    {
      skip = "differs from one cell type to the next";
    }
    else if (space == "Bezier"_token)
    {
      skip = "already Bezier";
    }
    else if (space != "HGRAD"_token)
    {
      // HCURL and HDIV attributes are vector-valued in a way no scalar Bezier
      // basis reproduces, and a constant one has no basis to change.
      skip = "no Bezier counterpart for this function space";
    }
    else if (!mismatch.empty())
    {
      skip = mismatch;
    }

    if (!skip.empty())
    {
      std::cout << "  " << attributeName << ": skipped (" << skip << ")\n";
      if (want.SourceOrder != notConvertible)
      {
        std::cerr << "    ERROR: expected this to convert from order " << want.SourceOrder << ".\n";
        ok = false;
      }
      continue;
    }
    if (want.SourceOrder == notConvertible)
    {
      std::cerr << "  ERROR: \"" << attributeName << "\" was expected not to convert, but "
                << space.Data() << " \"" << basis.Data() << "\" order " << order << " can.\n";
      ok = false;
      continue;
    }
    if (order != want.SourceOrder)
    {
      std::cerr << "  ERROR: \"" << attributeName << "\" is stored at order " << order
                << ", not the expected " << want.SourceOrder << ".\n";
      ok = false;
      continue;
    }

    std::cout << "  " << attributeName << ": " << space.Data() << " \"" << basis.Data()
              << "\" order " << order << " to Bezier order " << want.TargetOrder
              << (want.Shared ? " (shared)" : " (per cell)") << "\n";

    std::vector<double> before;
    if (!Sample(grid, attributeName, before, {}))
    {
      ok = false;
      continue;
    }

    vtkNew<vtkCellGridChangeBasis> converter;
    converter->SetInputDataObject(grid);
    converter->SetCellAttribute(attribute);
    converter->SetSourceFunctionSpace(space.Data().c_str());
    converter->SetSourceBasis(basis.Data().c_str());
    converter->SetSourceOrder(order);
    converter->SetTargetFunctionSpace("Bezier");
    converter->SetTargetBasis("A");
    converter->SetTargetOrder(want.TargetOrder);
    converter->Update();

    auto* output = vtkCellGrid::SafeDownCast(converter->GetOutputDataObject(0));
    if (!output)
    {
      std::cerr << "    ERROR: The filter produced no output.\n";
      ok = false;
      continue;
    }

    // The conversion must not have disturbed the input.
    std::vector<double> inputAfter;
    if (!Sample(grid, attributeName, inputAfter, {}) || inputAfter != before)
    {
      std::cerr << "    ERROR: The filter modified its own input.\n";
      ok = false;
    }

    auto* result = output->GetCellAttributeByName(attributeName);
    if (!result)
    {
      std::cerr << "    ERROR: The output has no \"" << attributeName << "\" attribute.\n";
      ok = false;
      continue;
    }
    for (const auto& cellTypeName : output->CellTypeArray())
    {
      auto info = result->GetCellTypeInfo(cellTypeName);
      if (info.FunctionSpace.IsValid() &&
        (info.FunctionSpace != "Bezier"_token || info.Order != want.TargetOrder))
      {
        std::cerr << "    ERROR: " << cellTypeName.Data() << " is not Bezier at order "
                  << want.TargetOrder << " after conversion.\n";
        ok = false;
      }
    }

    std::vector<double> after;
    if (!Sample(output, attributeName, after, {}))
    {
      ok = false;
      continue;
    }
    if (after.size() != before.size())
    {
      std::cerr << "    ERROR: Sampled " << after.size() << " values after conversion but "
                << before.size() << " before.\n";
      ok = false;
      continue;
    }
    for (std::size_t ii = 0; ii < before.size(); ++ii)
    {
      if (std::abs(after[ii] - before[ii]) > tolerance)
      {
        std::cerr << "    ERROR: Sample " << ii << " changed from " << before[ii] << " to "
                  << after[ii] << ".\n";
        ok = false;
        break;
      }
    }

    ok &= TestConvexHull(output, attributeName, want.Shared, {});
  }
  return ok;
}

} // anonymous namespace

int TestCellGridChangeBasis(int argc, char* argv[])
{
  bool ok = true;

  vtkFiltersCellGrid::RegisterCellsAndResponders();

  // Every file TestCellGridFindCriticalPoints reads, grouped by cell type in
  // vtkDGCell::Shape order, then Mixed - the same grouping that test uses.
  //
  // Each attribute's entry is {source order, target order, shared afterwards}.
  // Converting at the source's own order is exact and, when both bases place
  // their degrees of freedom at the same points, keeps sharing. "curl1" and
  // "div1" are HCURL and HDIV, which no scalar Bezier basis reproduces, so
  // those are recorded as notConvertible.
  // clang-format off

  // Edge
  ok &= TestConversion(argc, argv, "Data/dgEdges.dg", {
    { "shape",   { 1, 1, true } },
    { "scalar0", { 1, 1, false } },
    { "scalar1", { 1, 1, false } },
    { "scalar2", { 1, 1, false } },
    { "scalar3", { 1, 1, true } },
  });
  ok &= TestConversion(argc, argv, "Data/dgEdge-GBasis.dg", {
    { "shape", { 1, 1, true } },
    { "G1",    { 1, 1, false } },
    { "G2",    { 2, 2, false } },
    { "G3",    { 3, 3, false } },
    { "G4",    { 4, 4, false } },
    { "G5",    { 5, 5, false } },
  });

  // Triangle
  ok &= TestConversion(argc, argv, "Data/dgTriangle.dg", {
    { "shape",   { 1, 1, true } },
    { "scalar0", { 1, 1, false } },
    { "scalar1", { 1, 1, false } },
    { "scalar2", { 1, 1, false } },
    { "scalar3", { 1, 1, true } },
  });
  ok &= TestConversion(argc, argv, "Data/dgTriangle-GBasis.dg", {
    { "shape", { 1, 1, true } },
    { "G1",    { 1, 1, false } },
    { "G2",    { 2, 2, false } },
    { "G3",    { 3, 3, false } },
    { "G4",    { 4, 4, false } },
    { "G5",    { 5, 5, false } },
  });

  // Quadrilateral
  ok &= TestConversion(argc, argv, "Data/dgQuadrilateral.dg", {
    { "shape",   { 1, 1, true } },
    { "scalar0", { 1, 1, false } },
    { "scalar1", { 1, 1, false } },
    { "scalar2", { 1, 1, false } },
    { "scalar3", { 1, 1, true } },
  });
  ok &= TestConversion(argc, argv, "Data/dgQuad-GBasis.dg", {
    { "shape", { 1, 1, true } },
    { "G1",    { 1, 1, false } },
    { "G2",    { 2, 2, false } },
    { "G3",    { 3, 3, false } },
    { "G4",    { 4, 4, false } },
    { "G5",    { 5, 5, false } },
  });
  ok &= TestConversion(argc, argv, "Data/dgQuadraticQuadrilaterals.dg", {
    { "shape",   { 2, 2, true } },
    { "linear",  { 1, 1, false } },
    { "scalar0", { 2, 2, false } },
    { "scalar1", { 2, 2, false } },
    { "scalar2", { 2, 2, false } },
    { "scalar3", { 2, 2, true } },
  });
  // Raising the order is exact too, since the target spans everything the
  // source does, but the two bases no longer place their degrees of freedom at
  // the same points, so a shared attribute cannot stay shared.
  ok &= TestConversion(argc, argv, "Data/dgQuadraticQuadrilaterals.dg", {
    { "shape",   { 2, 3, false } },
    { "linear",  { 1, 2, false } },
    { "scalar0", { 2, 3, false } },
    { "scalar1", { 2, 4, false } },
    { "scalar2", { 2, 5, false } },
    { "scalar3", { 2, 4, false } },
  });

  // Tetrahedron
  ok &= TestConversion(argc, argv, "Data/dgTetrahedra.dg", {
    { "shape",   { 1, 1, true } },
    { "scalar0", { 1, 1, false } },
    { "scalar1", { 1, 1, false } },
    { "scalar2", { 1, 1, false } },
    { "scalar3", { 1, 1, true } },
    { "curl1",   { notConvertible, 0, false } },
    { "div1",    { notConvertible, 0, false } },
    { "G1",      { 1, 1, false } },
    { "G2",      { 2, 2, false } },
    { "G3",      { 3, 3, false } },
    { "G4",      { 4, 4, false } },
    { "G5",      { 5, 5, false } },
  });

  // Hexahedron
  ok &= TestConversion(argc, argv, "Data/dgHexahedra.dg", {
    { "shape",       { 1, 1, true } },
    { "scalar0",     { 1, 1, false } },
    { "scalar1",     { 1, 1, false } },
    { "scalar2",     { 1, 1, false } },
    { "scalar3",     { 1, 1, true } },
    { "quadratic",   { 2, 2, false } },
    { "gauss_cubic", { 3, 3, false } },
    { "curl1",       { notConvertible, 0, false } },
    { "div1",        { notConvertible, 0, false } },
  });
  ok &= TestConversion(argc, argv, "Data/dgGhostHexahedra.dg", {
    { "shape",     { 1, 1, true } },
    { "scalar0",   { 1, 1, false } },
    { "scalar1",   { 1, 1, false } },
    { "scalar2",   { 1, 1, false } },
    { "scalar3",   { 1, 1, true } },
    { "quadratic", { 2, 2, false } },
    { "curl1",     { notConvertible, 0, false } },
  });
  ok &= TestConversion(argc, argv, "Data/dgHexahedra-GBasis.dg", {
    { "shape",       { 1, 1, true } },
    { "scalar0",     { 1, 1, false } },
    { "scalar1",     { 1, 1, false } },
    { "scalar2",     { 1, 1, false } },
    { "scalar3",     { 1, 1, true } },
    { "quadratic",   { 2, 2, false } },
    { "gauss_cubic", { 3, 3, false } },
    { "G1",          { 1, 1, false } },
    { "G2",          { 2, 2, false } },
    { "G3",          { 3, 3, false } },
    { "G4",          { 4, 4, false } },
    { "G5",          { 5, 5, false } },
    { "curl1",       { notConvertible, 0, false } },
    { "div1",        { notConvertible, 0, false } },
  });
  ok &= TestConversion(argc, argv, "Data/fandisk.dg", {
    { "shape",  { 1, 1, true } },
    { "curlyq", { 1, 1, true } },
  });

  // Wedge
  ok &= TestConversion(argc, argv, "Data/dgWedges.dg", {
    { "shape",   { 1, 1, true } },
    { "scalar0", { 1, 1, false } },
    { "scalar1", { 1, 1, false } },
    { "scalar2", { 1, 1, false } },
    { "scalar3", { 1, 1, true } },
  });
  ok &= TestConversion(argc, argv, "Data/dgWedges-GBasis.dg", {
    { "shape",   { 1, 1, true } },
    { "scalar0", { 1, 1, false } },
    { "scalar1", { 1, 1, false } },
    { "scalar2", { 1, 1, false } },
    { "scalar3", { 1, 1, true } },
    { "G1",      { 1, 1, false } },
    { "G2",      { 2, 2, false } },
    { "G3",      { 3, 3, false } },
    { "G4",      { 4, 4, false } },
    { "G5",      { 5, 5, false } },
  });

  // Pyramid. Its bases are rational rather than polynomial once composed with
  // vtkDGPyr's reference coordinates (see PyrCnBasis.h), which these exercise
  // in a way the other shapes cannot: a wrong change of basis - say, missing
  // domain points near the apex - shows up as values that no longer agree
  // rather than as a hard failure.
  ok &= TestConversion(argc, argv, "Data/dgPyramids.dg", {
    { "shape",   { 1, 1, true } },
    { "scalar0", { 1, 1, false } },
    { "scalar1", { 1, 1, false } },
    { "scalar2", { 1, 1, false } },
    { "scalar3", { 1, 1, true } },
  });
  ok &= TestConversion(argc, argv, "Data/dgPyramids.dg", {
    { "shape",   { 1, 2, false } },
    { "scalar0", { 1, 2, false } },
    { "scalar1", { 1, 2, false } },
    { "scalar2", { 1, 2, false } },
    { "scalar3", { 1, 2, false } },
  });
  // The modal "G" basis carries hierarchical Jacobi-polynomial coefficients
  // rather than nodal values, so it has no degrees of freedom a conversion
  // could reuse - no domain points, no sharing - yet it spans the same space
  // as the Bezier basis of the same order (Chan & Warburton, Lemma 3.3), which
  // makes each of these a square, exact change of basis.
  ok &= TestConversion(argc, argv, "Data/dgPyramids-GBasis.dg", {
    { "shape", { 1, 1, true } },
    { "G1",    { 1, 1, false } },
    { "G2",    { 2, 2, false } },
    { "G3",    { 3, 3, false } },
    { "G4",    { 4, 4, false } },
    { "G5",    { 5, 5, false } },
  });

  // Mixed: one attribute spanning two cell types at once.
  ok &= TestConversion(argc, argv, "Data/dgMixed.dg", {
    { "shape",   { 1, 1, true } },
    { "scalar0", { 1, 1, false } },
    { "scalar1", { 1, 1, false } },
    { "scalar2", { 1, 1, false } },
    { "scalar3", { 1, 1, true } },
  });
  // clang-format on

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
