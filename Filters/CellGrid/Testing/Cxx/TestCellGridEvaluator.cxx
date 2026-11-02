// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise vtkCellGridEvaluator against every example .dg dataset, over every
// cell-attribute each one carries (but its shape). Test input points are
// classified and each attribute interpolated there; which points lie inside
// a cell and the values interpolated are checked against values recorded
// from the evaluator itself (a regression baseline) except for scalar1 of
// dgHexahedra.dg and dgTetrahedra.dg, which were derived by hand. The
// classification is then passed back in (skipping it) and must interpolate
// the same values.

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridEvaluator.h"
#include "vtkCellGridReader.h"
#include "vtkDoubleArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkNew.h"
#include "vtkStringToken.h"
#include "vtkTestUtilities.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

namespace
{

using Tuple = std::vector<double>;

double tupleDiffMag(const Tuple& aa, const Tuple& bb, bool& ok)
{
  double diffMag = 0.;
  std::size_t nn = aa.size();
  if (bb.size() != nn)
  {
    nn = std::min(nn, bb.size());
    std::cerr << "ERROR: Tuples of different size! (" << aa.size() << " vs " << bb.size() << ")\n";
    ok = false;
  }
  for (std::size_t ii = 0; ii < nn; ++ii)
  {
    double compDiff = aa[ii] - bb[ii];
    diffMag += compDiff * compDiff;
  }
  diffMag = std::sqrt(diffMag);
  return diffMag;
}

std::ostream& operator<<(std::ostream& os, const std::vector<Tuple>& tuples)
{
  os << std::setprecision(9) << "{";
  for (std::size_t ii = 0; ii < tuples.size(); ++ii)
  {
    os << (ii ? ", " : " ") << "{";
    for (std::size_t cc = 0; cc < tuples[ii].size(); ++cc)
    {
      os << (cc ? ", " : " ") << tuples[ii][cc];
    }
    os << " }";
  }
  os << " }";
  return os;
}

/// Read \a fileName, classify \a testPoints and, for every cell-attribute but
/// the shape, interpolate it there. Check that:
/// + exactly the points marked in \a expectedInside lie inside a cell;
/// + the values interpolated match \a expectedValues (keyed by attribute
///   name), one tuple per classified row ordered by test point, then cell type
///   and cell (a point on a boundary shared by several cells has several);
/// + passing the classification back in (see
///   vtkCellGridEvaluator::InterpolateCellParameters()) interpolates the same
///   values, row for row.
bool TestEvaluate(int argc, char* argv[], const char* fileName,
  const std::vector<std::array<double, 3>>& testPoints, const std::vector<bool>& expectedInside,
  const std::map<std::string, std::vector<Tuple>>& expectedValues)
{
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, fileName, 0);
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;
  auto* grid = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!grid)
  {
    std::cerr << "  ERROR: could not read \"" << fileName << "\".\n";
    return false;
  }
  std::cout << fileName << ":\n";

  vtkNew<vtkDoubleArray> coords;
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(static_cast<vtkIdType>(testPoints.size()));
  for (std::size_t ii = 0; ii < testPoints.size(); ++ii)
  {
    coords->SetTuple(static_cast<vtkIdType>(ii), testPoints[ii].data());
  }

  bool ok = true;
  auto* shapeAttribute = grid->GetShapeAttribute();
  for (const auto& attribute : grid->GetCellAttributeList())
  {
    if (attribute.GetPointer() == shapeAttribute)
    {
      continue;
    }
    const std::string attributeName = attribute->GetName().Data();

    vtkNew<vtkCellGridEvaluator> evaluator;
    evaluator->SetCellAttribute(attribute);
    // Some test points lie exactly on boundaries shared between neighboring
    // cells; use a small tolerance so they classify into every cell that
    // contains them.
    evaluator->SetClassifierTolerance(1e-8);
    evaluator->InterpolatePoints(coords);
    grid->Query(evaluator);
    auto* cellTypes = evaluator->GetClassifierCellTypes();
    auto* cellOffsets = evaluator->GetClassifierCellOffsets();
    auto* pointIDs = evaluator->GetClassifierPointIDs();
    auto* cellIndices = evaluator->GetClassifierCellIndices();
    auto* pointParams = evaluator->GetClassifierPointParameters();
    auto* values = evaluator->GetInterpolatedValues();
    const int numComponents = values->GetNumberOfComponents();

    // The classified rows (skipping those of the invalid cell type, which no
    // cell contains), in the evaluator's order: grouped by cell type.
    struct Row
    {
      vtkIdType Id;
      vtkTypeUInt64 Point;
      vtkTypeUInt32 CellType;
      vtkTypeUInt64 Cell;
    };
    std::vector<Row> rows;
    const vtkIdType numTypes = cellTypes->GetNumberOfTuples();
    for (vtkIdType tt = 0; tt < numTypes; ++tt)
    {
      const vtkTypeUInt32 cellType = cellTypes->GetValue(tt);
      if (cellType == vtkStringToken::InvalidHash())
      {
        continue;
      }
      const auto begin = static_cast<vtkIdType>(cellOffsets->GetValue(tt));
      const vtkIdType end = tt + 1 < numTypes
        ? static_cast<vtkIdType>(cellOffsets->GetValue(tt + 1))
        : pointIDs->GetNumberOfTuples();
      for (vtkIdType row = begin; row < end; ++row)
      {
        rows.push_back({ row, pointIDs->GetValue(row), cellType, cellIndices->GetValue(row) });
      }
    }

    // Which points lie inside a cell.
    std::vector<bool> inside(testPoints.size(), false);
    for (const auto& row : rows)
    {
      inside[row.Point] = true;
    }
    if (inside != expectedInside)
    {
      for (std::size_t ii = 0; ii < inside.size(); ++ii)
      {
        if (ii >= expectedInside.size() || inside[ii] != expectedInside[ii])
        {
          std::cerr << "    ERROR: \"" << attributeName << "\": point " << ii << " is "
                    << (inside[ii] ? "inside" : "outside") << ", expected otherwise.\n";
        }
      }
      ok = false;
    }

    // The values interpolated, ordered by test point, then cell type and cell.
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b)
      { return std::tie(a.Point, a.CellType, a.Cell) < std::tie(b.Point, b.CellType, b.Cell); });
    std::vector<Tuple> actualValues;
    for (const auto& row : rows)
    {
      Tuple tuple(numComponents);
      values->GetTuple(row.Id, tuple.data());
      actualValues.push_back(tuple);
    }
    std::cout << "  " << attributeName << ": " << actualValues.size() << " rows\n";
    auto it = expectedValues.find(attributeName);
    if (it == expectedValues.end())
    {
      std::cerr << "    ERROR: no expected values recorded for \"" << attributeName << "\"; got "
                << actualValues << ".\n";
      ok = false;
    }
    else if (it->second.size() != actualValues.size())
    {
      std::cerr << "    ERROR: \"" << attributeName << "\": expected " << it->second.size()
                << " rows, got " << actualValues << ".\n";
      ok = false;
    }
    else
    {
      for (std::size_t vv = 0; vv < actualValues.size(); ++vv)
      {
        const Tuple& expected = it->second[vv];
        const double scale = std::max(1.0,
          std::sqrt(std::inner_product(expected.begin(), expected.end(), expected.begin(), 0.0)));
        if (tupleDiffMag(actualValues[vv], expected, ok) > 1e-5 * scale)
        {
          std::cerr << "    ERROR: \"" << attributeName << "\": expected " << it->second << ", got "
                    << actualValues << ".\n";
          ok = false;
          break;
        }
      }
    }

    // Passing the classification back in skips it; interpolating must then
    // give the same values, row for row.
    vtkNew<vtkCellGridEvaluator> interpolator;
    interpolator->SetCellAttribute(attribute);
    interpolator->InterpolateCellParameters(cellTypes, cellOffsets, cellIndices, pointParams);
    grid->Query(interpolator);
    auto* reinterpolated = interpolator->GetInterpolatedValues();
    if (reinterpolated->GetNumberOfTuples() != values->GetNumberOfTuples() ||
      reinterpolated->GetNumberOfComponents() != numComponents)
    {
      std::cerr << "    ERROR: \"" << attributeName
                << "\" interpolated from its classification has "
                << reinterpolated->GetNumberOfTuples() << " values, expected "
                << values->GetNumberOfTuples() << ".\n";
      ok = false;
      continue;
    }
    Tuple tuple(numComponents);
    Tuple reinterpolatedTuple(numComponents);
    for (const auto& row : rows)
    {
      values->GetTuple(row.Id, tuple.data());
      reinterpolated->GetTuple(row.Id, reinterpolatedTuple.data());
      if (tupleDiffMag(tuple, reinterpolatedTuple, ok) > 1e-12)
      {
        std::cerr << "    ERROR: \"" << attributeName << "\" row " << row.Id
                  << " interpolated from its classification is " << reinterpolatedTuple[0]
                  << ", expected " << tuple[0] << ".\n";
        ok = false;
      }
    }
  }
  return ok;
}

} // anonymous namespace

int TestCellGridEvaluator(int argc, char* argv[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  bool ok = true;

  // clang-format off
  // Grouped by cell type, in vtkDGCell::Shape order (Edge, Triangle,
  // Quadrilateral, Tetrahedron, Hexahedron, Wedge, Pyramid), then Mixed.
  // vtkDGEvaluator classifies points by inverting a 3x3 Jacobian, so it finds
  // none inside curves or surfaces.

  // Edge
  ok &= TestEvaluate(argc, argv, "Data/dgEdges.dg",
    { { 0.5, 0., 0. }, { 0.25, 0., 0. }, { 0., 0., 0. }, { 0., 0.5, 0. }, { 2.5, 2.5, 2. } },
    { false, false, false, false, false }, {
    { "scalar0", { } },
    { "scalar1", { } },
    { "scalar2", { } },
    { "scalar3", { } },
  });
  ok &= TestEvaluate(argc, argv, "Data/dgEdge-GBasis.dg",
    { { 0.5, 0., 0. }, { 0.25, 0., 0. }, { 0., 0., 0. }, { 1.5, 0.15, 0. }, { 4.0224, 2.3224, 2.0224 } },
    { false, false, false, false, false }, {
    { "G1", { } },
    { "G2", { } },
    { "G3", { } },
    { "G4", { } },
    { "G5", { } },
  });

  // Triangle
  ok &= TestEvaluate(argc, argv, "Data/dgTriangle.dg",
    { { 0.3333, 0.3333, 0. }, { 0.1666, 0.1666, 0. }, { 0., 0., 0. }, { 0.5, 0.5, 0.1667 }, { 2.5, 2.5, 2. } },
    { false, false, false, false, false }, {
    { "scalar0", { } },
    { "scalar1", { } },
    { "scalar2", { } },
    { "scalar3", { } },
  });
  ok &= TestEvaluate(argc, argv, "Data/dgTriangle-GBasis.dg",
    { { 0.3333, 0.3333, 0. }, { 0.1666, 0.1666, 0. }, { 0., 0., 0. }, { 0.6667, 0.6667, 0. }, { 2.4142, 2.4142, 1.4142 } },
    { false, false, false, false, false }, {
    { "G1", { } },
    { "G2", { } },
    { "G3", { } },
    { "G4", { } },
    { "G5", { } },
  });

  // Quadrilateral
  ok &= TestEvaluate(argc, argv, "Data/dgQuadrilateral.dg",
    { { 0.5, 0.5, 0. }, { 0.25, 0.25, 0. }, { 0., 0., 0. }, { 1., 0.5, 0.5 }, { 2.7321, 2.7321, 2.7321 } },
    { false, false, false, false, false }, {
    { "scalar0", { } },
    { "scalar1", { } },
    { "scalar2", { } },
    { "scalar3", { } },
  });
  ok &= TestEvaluate(argc, argv, "Data/dgQuad-GBasis.dg",
    { { 0.5, 0.5, 0. }, { 0.25, 0.25, 0. }, { 0., 0., 0. }, { 1.5, 0.5, 0. }, { 4.2361, 3.2361, 2.2361 } },
    { false, false, false, false, false }, {
    { "G1", { } },
    { "G2", { } },
    { "G3", { } },
    { "G4", { } },
    { "G5", { } },
  });
  ok &= TestEvaluate(argc, argv, "Data/dgQuadraticQuadrilaterals.dg",
    { { 1., 1., 0. }, { 0.5, 0.5, 0. }, { 0., 0., 0. }, { 2., 1., 1. }, { 6.1231, 6.1231, 6.1231 } },
    { false, false, false, false, false }, {
    { "linear", { } },
    { "scalar0", { } },
    { "scalar1", { } },
    { "scalar2", { } },
    { "scalar3", { } },
  });

  // Tetrahedron
  ok &= TestEvaluate(argc, argv, "Data/dgTetrahedra.dg",
    { { 0.5, 0.5, 0. }, { 1., 0., 0. }, { 0.8, 0.8, 0.05 }, { 2.5, 0.5, 0.5 }, { 0.5, 0.4, 0.1 } },
    { true, true, true, false, true }, {
    { "G1", { { 0.5 }, { 0.5 }, { 1 }, { 0 }, { 1.45 }, { 0.3 } } },
    { "G2", { { -0.75 }, { 1 }, { 1 }, { 3 }, { -0.3 }, { -1.01 } } },
    { "G3", { { 0.125 }, { 1 }, { 1 }, { 1 }, { 1 }, { -0.045 } } },
    { "G4", { { 0.0625 }, { 1 }, { 1 }, { 1 }, { 1 }, { -0.0255 } } },
    { "G5", { { 0.03125 }, { 1 }, { 1 }, { 1 }, { 1 }, { -0.00477 } } },
    { "curl1", { { -1, 1, 0 }, { 1, -1, 4 }, { 2, 4, -2 }, { 4, 2, 6 }, { -0.9, 0.9, 4 }, { -0.2, 0.8, -0.2 } } },
    { "div1", { { 0, 0, 0 }, { -1, -1, -1 }, { 0, 0, 0 }, { 3, -5, -1 }, { 1.4, 1.4, -0.6 }, { 0, 0, 0 } } },
    { "scalar0", { { 0 }, { 1 }, { 0 }, { 1 }, { 1 }, { 0 } } },
    { "scalar1", { { 2.5 }, { 0.5 }, { 3 }, { 0 }, { 0.25 }, { 2 } } },
    { "scalar2", { { 0 }, { 2 }, { 0 }, { 2 }, { 1.9 }, { 0.2 } } },
    { "scalar3", { { 1 }, { 1 }, { 1 }, { 1 }, { 1.1 }, { 1.1 } } },
  });

  // Hexahedron
  ok &= TestEvaluate(argc, argv, "Data/dgHexahedra.dg",
    { { 0., 0., 0. }, { 1., 0., 0. }, { 1.5, 0.5, 0.5 }, { 2.5, 0.5, 0.5 }, { 0.3, 0.3, 0.3 } },
    { false, true, true, false, true }, {
    { "curl1", { { 2.66666667, -5.13581319e-33, -1.85037171e-16 }, { 0, 0, 0 }, { -0.0278871408, 0.444444444, -0.022844338 }, { 1.42919713, 0.183864677, 0.273103481 } } },
    { "div1", { { 0, -5.33333333, 1.48029737e-16 }, { 0, 0, 0 }, { 1.77777778, 0.111548563, 0 }, { 0.642957053, -3.96021054, -0.698524739 } } },
    { "gauss_cubic", { { 3.00000002 }, { 0.999999979 }, { 0.0069992338 }, { 0.975250573 } } },
    { "quadratic", { { -2.22044605e-16 }, { 4 }, { 3 }, { 2.66101984 } } },
    { "scalar0", { { 0 }, { 1 }, { 1 }, { 0 } } },
    { "scalar1", { { 3 }, { 0 }, { 1.5 }, { 1.10967 } } },
    { "scalar2", { { 0 }, { 2 }, { 2.5 }, { 0.273531325 } } },
    { "scalar3", { { 0 }, { 0 }, { 1 }, { 0.54706265 } } },
  });
  ok &= TestEvaluate(argc, argv, "Data/dgGhostHexahedra.dg",
    { { 0.5625, 0.5312, 0.5312 }, { 0.4062, 0.3906, 0.2656 }, { 0.25, 0.25, 0. }, { 1.5312, 0.5312, 0.5 }, { 4.5617, 3.8117, 3.5617 } },
    { true, true, true, true, false }, {
    { "curl1", { { 0.500120355, 6.07870613e-06, 7.95881964e-06 }, { 1.27044372, 0.147942568, 0.152019602 }, { 2.88888889, 0.666666667, 0.888888889 }, { -0.0314901144, 0.472396153, -0.0275510234 } } },
    { "quadratic", { { 3.00009909 }, { 2.77390486 }, { 8.8817842e-16 }, { 3 } } },
    { "scalar0", { { 0 }, { 0 }, { 0 }, { 1 } } },
    { "scalar1", { { 1.5 }, { 1.24471928 }, { 4.4408921e-16 }, { 1.5 } } },
    { "scalar2", { { 0.499950443 }, { 0.225858937 }, { 0 }, { 2.5 } } },
    { "scalar3", { { 0.999900886 }, { 0.451717873 }, { 0 }, { 1 } } },
  });
  ok &= TestEvaluate(argc, argv, "Data/dgHexahedra-GBasis.dg",
    { { 0.5625, 0.5312, 0.5312 }, { 0.4062, 0.3906, 0.2656 }, { 0.25, 0.25, 0. }, { 1.5312, 0.5312, 0.5 }, { 4.5617, 3.8117, 3.5617 } },
    { true, true, true, true, false }, {
    { "G1", { { 0.125045988 }, { 0.694344993 }, { 2.54903811 }, { 0.125040918 } } },
    { "G2", { { 4.2390241e-18 }, { 0.0908783303 }, { 3.23411343 }, { 0 } } },
    { "G3", { { -0.000787204428 }, { 0.000567253676 }, { 3.55906829 }, { -0.000787183059 } } },
    { "G4", { { -4.30519266e-19 }, { 5.95792409e-05 }, { 3.73403274 }, { 0 } } },
    { "G5", { { 4.44987522e-05 }, { -0.000244549175 }, { 3.83798571 }, { 4.44976392e-05 } } },
    { "curl1", { { 0.500120355, 6.07870613e-06, 7.95881964e-06 }, { 1.27044372, 0.147942568, 0.152019602 }, { 2.88888889, 0.666666667, 0.888888889 }, { -0.0314901144, 0.472396153, -0.0275510234 } } },
    { "div1", { { 2.80720953e-05, -2.12417648, -0.141626737 }, { 0.457931711, -3.39219467, -0.525764267 }, { 1.77777778, -5.33333333, -1.77777778 }, { 1.88958461, 0.125960458, 0 } } },
    { "gauss_cubic", { { 1.40062066e-08 }, { 0.438367279 }, { 3.00000002 }, { 1.91057651e-08 } } },
    { "quadratic", { { 3.00009909 }, { 2.77390486 }, { 8.8817842e-16 }, { 3 } } },
    { "scalar0", { { 0 }, { 0 }, { 0 }, { 1 } } },
    { "scalar1", { { 1.5 }, { 1.24471928 }, { 4.4408921e-16 }, { 1.5 } } },
    { "scalar2", { { 0.499950443 }, { 0.225858937 }, { 0 }, { 2.5 } } },
    { "scalar3", { { 0.999900886 }, { 0.451717873 }, { 0 }, { 1 } } },
  });
  ok &= TestEvaluate(argc, argv, "Data/fandisk.dg",
    { { 0.8372, 0.0293, 0.2404 }, { 0.8063, 0.0701, 0.2592 }, { 0.775308, 0.110836, 0.278078 }, { -0.5385, 0.0627, 0.2948 }, { 3.7975, 4.1355, 3.2995 } },
    { true, true, true, true, false }, {
    { "curlyq", { { 0.0483380774, 0.136626706, 0.184964783 }, { 0.0434041042, 0.125542235, 0.168946339 }, { 0.0384761385, 0.114394054, 0.152870192 }, { 0.0384761385, 0.114394054, 0.152870192 }, { 0.0384761385, 0.114394054, 0.152870192 }, { 0.0384761385, 0.114394054, 0.152870192 }, { 0.0384761385, 0.114394054, 0.152870192 }, { 0.0384761385, 0.114394054, 0.152870192 }, { 0.0384761385, 0.114394054, 0.152870192 }, { 0.0384761385, 0.114394054, 0.152870192 }, { 0.0542678335, -0.195278789, -0.141010956 } } },
  });

  // Wedge
  ok &= TestEvaluate(argc, argv, "Data/dgWedges.dg",
    { { 0.3333, 0.3333, 0.5 }, { 0.1666, 0.1666, 0.25 }, { 0., 0., 0. }, { 0.6667, 0.6667, 0.5 }, { 2.7321, 2.7321, 2.7321 } },
    { true, true, true, true, false }, {
    { "scalar0", { { 0 }, { 0 }, { 0 }, { 1 } } },
    { "scalar1", { { 0.9999 }, { 0.4998 }, { 0 }, { 1 } } },
    { "scalar2", { { 0.6666 }, { 0.3332 }, { 0 }, { 1.3334 } } },
    { "scalar3", { { 0.9999 }, { 0.4998 }, { 0 }, { 2.0001 } } },
  });
  ok &= TestEvaluate(argc, argv, "Data/dgWedges-GBasis.dg",
    { { 0.3333, 0.3333, 0.5 }, { 0.1666, 0.1666, 0.25 }, { 0., 0., 0. }, { 0.6667, 0.6667, 0.5 }, { 2.7321, 2.7321, 2.7321 } },
    { true, true, true, true, false }, {
    { "G1", { { 0.166733333 }, { 0.933261505 }, { 2.27670901 }, { 0.166733333 } } },
    { "G2", { { -3.30889129e-18 }, { 0.250965754 }, { 2.77082459 }, { -3.30889129e-18 } } },
    { "G3", { { 3.94494956e-06 }, { 0.0114589624 }, { 2.96239143 }, { 3.94494956e-06 } } },
    { "G4", { { -6.52705968e-19 }, { 0.00203222058 }, { 3.01607947 }, { -6.52705968e-19 } } },
    { "G5", { { -0.00120967279 }, { 0.0012110975 }, { 3.31770432 }, { -0.00120967279 } } },
    { "scalar0", { { 0 }, { 0 }, { 0 }, { 1 } } },
    { "scalar1", { { 0.9999 }, { 0.4998 }, { 0 }, { 1 } } },
    { "scalar2", { { 0.6666 }, { 0.3332 }, { 0 }, { 1.3334 } } },
    { "scalar3", { { 0.9999 }, { 0.4998 }, { 0 }, { 2.0001 } } },
  });

  // Pyramid
  ok &= TestEvaluate(argc, argv, "Data/dgPyramids.dg",
    { { 0.5, 0.5, 0.2 }, { 0.25, 0.25, 0.1 }, { 0., 0., 0. }, { 0.9, 0.5, 0.6 }, { 2.7321, 2.7321, 2.7321 } },
    { true, true, true, true, false }, {
    { "scalar0", { { 0 }, { 0 }, { 0 }, { 1 } } },
    { "scalar1", { { 1.2 }, { 0.711111111 }, { 0 }, { 1 } } },
    { "scalar2", { { 0.2 }, { 0.1 }, { 0 }, { 1.8 } } },
    { "scalar3", { { 0.6 }, { 0.3 }, { 0 }, { 1.4 } } },
  });
  ok &= TestEvaluate(argc, argv, "Data/dgPyramids-GBasis.dg",
    { { 0.5, 0.5, 0.2 }, { 0.25, 0.25, 0.1 }, { 0., 0., 0. }, { 0.9, 0.5, 0.6 }, { 2.7321, 2.7321, 2.7321 } },
    { true, true, true, true, false }, {
    { "G1", { { -1.77635684e-16 }, { -0.5 }, { -1 }, { 1 } } },
    { "G2", { { -0.32 }, { -0.03 }, { 1 }, { 1 } } },
    { "G3", { { 1.27897692e-16 }, { 0.295 }, { -1 }, { 1 } } },
    { "G4", { { 0.1536 }, { -0.2399 }, { 1 }, { 1 } } },
    { "G5", { { -3.02406988e-16 }, { 0.02475 }, { -1 }, { 1 } } },
  });

  // Mixed
  ok &= TestEvaluate(argc, argv, "Data/dgMixed.dg",
    { { 0.375, 0.375, 0.125 }, { 0.1875, 0.1875, 0.0625 }, { 0., 0., 0. }, { -0.5, 0.5, 0. }, { 3.2913, 3.2913, 2.7913 } },
    { true, true, true, false, false }, {
    { "scalar0", { { 0 }, { 0 }, { 0 } } },
    { "scalar1", { { 1.5 }, { 0.75 }, { 0 } } },
    { "scalar2", { { 0.25 }, { 0.125 }, { 0 } } },
    { "scalar3", { { 1 }, { 0.5 }, { 0 } } },
  });
  // clang-format on

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
