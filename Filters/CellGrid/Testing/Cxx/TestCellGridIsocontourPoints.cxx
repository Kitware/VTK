// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise vtkCellGridIsocontourPoints against the example .dg datasets, one
// shape per file, over every scalar cell-attribute each carries, converted to
// Bezier first (via vtkCellGridChangeBasis, at the attribute's own order,
// which is exact) since none start out in that basis.
//
// Every point the filter reports must lie on the isocontour. That is checked
// against the value the filter itself reports and, for grids with volumetric
// cells (the only ones vtkCellGridPointProbe can classify points in),
// independently of the Bezier conversion, the parametric coordinates and the
// placement in world space, by probing the original, unconverted attribute at
// each point's world position.
//
// Searching cells of one dimension at a time (see Include1DCells,
// Include2DCells and Include3DCells) must find the same points in total.
//
// The number of points and of output cells (one poly-vertex per source cell
// that found points) for each attribute at each isovalue is checked against
// values recorded from the filter itself - a regression baseline, not a
// hand-derived one. They are recorded with the arithmetic of x86 CPUs and
// compared only there: other CPUs (arm64, whose compilers fuse multiplications
// and additions) round differently, which moves some of the roots the filter
// finds, so there they are only reported.

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridChangeBasis.h"
#include "vtkCellGridIsocontourPoints.h"
#include "vtkCellGridPointProbe.h"
#include "vtkCellGridReader.h"
#include "vtkDGCell.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkFiltersCellGrid.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringToken.h"
#include "vtkTestUtilities.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace
{
using namespace vtk::literals;

/// Return the range of every Bezier coefficient \a attribute holds, which
/// bounds the attribute's values.
bool CoefficientRange(vtkCellGrid* grid, vtkCellAttribute* attribute, double range[2])
{
  range[0] = 1e300;
  range[1] = -1e300;
  bool found = false;
  for (const auto& cellTypeName : grid->CellTypeArray())
  {
    auto info = attribute->GetCellTypeInfo(cellTypeName);
    auto* values = info.GetArrayForRoleAs<vtkDataArray>("values"_token);
    if (!values)
    {
      continue;
    }
    // A cell's coefficients may all sit in one tuple (one component each), so
    // take the range over every component.
    for (int component = 0; component < values->GetNumberOfComponents(); ++component)
    {
      double r[2];
      values->GetRange(r, component);
      range[0] = std::min(range[0], r[0]);
      range[1] = std::max(range[1], r[1]);
    }
    found = true;
  }
  return found;
}

/// How many points one attribute's isocontour has at one isovalue, and how
/// many output cells (poly-vertices, one per source cell that found points)
/// group them.
struct IsocontourCounts
{
  vtkIdType Points{ 0 };
  vtkIdType Cells{ 0 };

  bool operator==(const IsocontourCounts& other) const
  {
    return this->Points == other.Points && this->Cells == other.Cells;
  }
};

std::ostream& operator<<(std::ostream& os, const IsocontourCounts& counts)
{
  os << counts.Points << " points in " << counts.Cells << " cells";
  return os;
}

/// Verify \a output's poly-vertices: together they hold every point once, in
/// order, each groups points from a single source cell, and no two group the
/// same source cell.
bool CheckCells(vtkPolyData* output)
{
  auto* cellTypes = output->GetPointData()->GetArray("CellType");
  auto* cellIds = output->GetPointData()->GetArray("CellId");
  std::set<std::pair<vtkTypeUInt32, vtkIdType>> sources;
  vtkIdType next = 0;
  for (vtkIdType cc = 0; cc < output->GetNumberOfCells(); ++cc)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    output->GetCellPoints(cc, npts, pts);
    if (npts == 0)
    {
      std::cerr << "    ERROR: output cell " << cc << " is empty.\n";
      return false;
    }
    const auto source = std::make_pair(static_cast<vtkTypeUInt32>(cellTypes->GetTuple1(pts[0])),
      static_cast<vtkIdType>(cellIds->GetTuple1(pts[0])));
    for (vtkIdType ii = 0; ii < npts; ++ii)
    {
      if (pts[ii] != next++ ||
        static_cast<vtkTypeUInt32>(cellTypes->GetTuple1(pts[ii])) != source.first ||
        static_cast<vtkIdType>(cellIds->GetTuple1(pts[ii])) != source.second)
      {
        std::cerr << "    ERROR: output cell " << cc
                  << " does not group consecutive points of one source cell.\n";
        return false;
      }
    }
    if (!sources.insert(source).second)
    {
      std::cerr << "    ERROR: output cell " << cc << " repeats a source cell.\n";
      return false;
    }
  }
  if (next != output->GetNumberOfPoints())
  {
    std::cerr << "    ERROR: output cells hold " << next << " of " << output->GetNumberOfPoints()
              << " points.\n";
    return false;
  }
  return true;
}

/// Contour \a attribute at \a fraction of its range and verify the result.
/// \a counts returns how many points and output cells were found.
bool CheckIsocontourPoints(
  vtkCellGrid* grid, vtkCellAttribute* attribute, double fraction, IsocontourCounts& counts)
{
  vtkSmartPointer<vtkCellGrid> bezierGrid = grid;
  vtkCellAttribute* bezierAttribute = attribute;
  for (const auto& cellTypeName : grid->CellTypeArray())
  {
    auto info = attribute->GetCellTypeInfo(cellTypeName);
    if (!info.FunctionSpace.IsValid())
    {
      continue;
    }
    if (info.FunctionSpace != "Bezier"_token)
    {
      vtkNew<vtkCellGridChangeBasis> converter;
      converter->SetInputDataObject(grid);
      converter->SetCellAttribute(attribute);
      converter->SetSourceFunctionSpace(info.FunctionSpace.Data().c_str());
      converter->SetSourceBasis(info.Basis.Data().c_str());
      converter->SetSourceOrder(info.Order);
      converter->SetTargetFunctionSpace("Bezier");
      converter->SetTargetBasis("A");
      converter->SetTargetOrder(info.Order);
      converter->Update();
      bezierGrid = vtkCellGrid::SafeDownCast(converter->GetOutputDataObject(0));
      bezierAttribute =
        bezierGrid ? bezierGrid->GetCellAttributeByName(attribute->GetName().Data()) : nullptr;
      if (!bezierAttribute)
      {
        std::cerr << "    ERROR: could not convert \"" << attribute->GetName().Data()
                  << "\" to Bezier.\n";
        return false;
      }
    }
    break;
  }

  double range[2];
  if (!CoefficientRange(bezierGrid, bezierAttribute, range))
  {
    std::cerr << "    ERROR: no coefficients for \"" << attribute->GetName().Data() << "\".\n";
    return false;
  }
  const double scale = std::max({ 1., std::abs(range[0]), std::abs(range[1]) });
  const double isovalue = range[0] + fraction * (range[1] - range[0]);

  vtkNew<vtkCellGridIsocontourPoints> filter;
  filter->SetInputDataObject(bezierGrid);
  filter->SetCellAttribute(bezierAttribute);
  filter->SetIsovalue(isovalue);
  filter->Update();
  auto* output = filter->GetOutput();
  const vtkIdType numPoints = output ? output->GetNumberOfPoints() : 0;
  counts = { numPoints, output ? output->GetNumberOfCells() : 0 };
  if (numPoints == 0)
  {
    return counts.Cells == 0;
  }

  bool ok = CheckCells(output);

  // Searching cells of one dimension at a time must find the same points in
  // total: every cell has exactly one dimension.
  if (fraction == 0.5)
  {
    vtkIdType total = 0;
    for (int dimension = 1; dimension <= 3; ++dimension)
    {
      vtkNew<vtkCellGridIsocontourPoints> oneDimension;
      oneDimension->SetInputDataObject(bezierGrid);
      oneDimension->SetCellAttribute(bezierAttribute);
      oneDimension->SetIsovalue(isovalue);
      oneDimension->SetInclude1DCells(dimension == 1);
      oneDimension->SetInclude2DCells(dimension == 2);
      oneDimension->SetInclude3DCells(dimension == 3);
      oneDimension->Update();
      total += oneDimension->GetOutput()->GetNumberOfPoints();
    }
    if (total != numPoints)
    {
      std::cerr << "    ERROR: searching one dimension at a time finds " << total << " points, not "
                << numPoints << ".\n";
      ok = false;
    }
  }
  auto* values = output->GetPointData()->GetArray("Value");
  for (vtkIdType ii = 0; ii < numPoints; ++ii)
  {
    if (std::abs(values->GetTuple1(ii) - isovalue) > 1e-9 * scale)
    {
      std::cerr << "    ERROR: point " << ii << " has value " << values->GetTuple1(ii)
                << ", not the isovalue " << isovalue << ".\n";
      ok = false;
      break;
    }
  }

  // Re-evaluate the original attribute at the points' world positions.
  // vtkCellGridPointProbe only classifies points inside volumetric cells and
  // reports the ID, but not reliably the type, of the containing cell, so
  // this check needs a grid with exactly one volumetric cell type and covers
  // only the points found in cells of that type.
  vtkTypeUInt32 volumeType = 0;
  int numVolumeTypes = 0;
  for (const auto& cellTypeName : grid->CellTypeArray())
  {
    auto* dgCell = vtkDGCell::SafeDownCast(grid->GetCellType(cellTypeName));
    if (dgCell && dgCell->GetDimension() == 3)
    {
      volumeType = cellTypeName.GetId();
      ++numVolumeTypes;
    }
  }
  if (numVolumeTypes != 1)
  {
    return ok;
  }

  // The datasets are discontinuous between cells, so a point on a shared face
  // or edge is also found inside its neighbors, where the field differs: only
  // the probe hits in the cell that produced the point are checked.
  std::map<std::array<double, 3>, std::set<vtkIdType>> originsByPosition;
  auto* cellTypes = output->GetPointData()->GetArray("CellType");
  auto* cellIds = output->GetPointData()->GetArray("CellId");
  vtkIdType numVolumePoints = 0;
  for (vtkIdType ii = 0; ii < numPoints; ++ii)
  {
    if (static_cast<vtkTypeUInt32>(cellTypes->GetTuple1(ii)) != volumeType)
    {
      continue;
    }
    std::array<double, 3> xyz;
    output->GetPoint(ii, xyz.data());
    originsByPosition[xyz].insert(static_cast<vtkIdType>(cellIds->GetTuple1(ii)));
    ++numVolumePoints;
  }

  vtkNew<vtkCellGridPointProbe> probe;
  probe->SetInputData(0, output);
  probe->SetInputData(1, grid);
  probe->GetAttributeSelection()->EnableArray(attribute->GetName().Data().c_str());
  probe->Update();
  auto* probed = vtkPolyData::SafeDownCast(probe->GetOutputDataObject(0));
  const vtkIdType numProbed = probed ? probed->GetNumberOfPoints() : 0;
  vtkIdType numChecked = 0;
  if (numProbed > 0)
  {
    auto* probedValues = probed->GetPointData()->GetScalars();
    auto* probedIds = probed->GetPointData()->GetArray("ContainingCellID");
    for (vtkIdType ii = 0; ii < numProbed; ++ii)
    {
      std::array<double, 3> xyz;
      probed->GetPoint(ii, xyz.data());
      auto it = originsByPosition.find(xyz);
      if (it == originsByPosition.end() ||
        !it->second.count(static_cast<vtkIdType>(probedIds->GetTuple1(ii))))
      {
        continue;
      }
      ++numChecked;
      if (std::abs(probedValues->GetTuple1(ii) - isovalue) > 1e-6 * scale)
      {
        std::cerr << "    ERROR: probing the grid at point " << ii << " gives "
                  << probedValues->GetTuple1(ii) << ", not the isovalue " << isovalue << ".\n";
        ok = false;
        break;
      }
    }
  }
  if (numChecked < numVolumePoints / 2)
  {
    std::cerr << "    ERROR: only " << numChecked << " of " << numVolumePoints
              << " isocontour points could be checked by probing the grid.\n";
    ok = false;
  }
  return ok;
}

/// The isovalues each attribute is contoured at, as fractions of the range of
/// its coefficients.
constexpr std::array<double, 3> Fractions{ { 0.25, 0.5, 0.75 } };

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
constexpr bool CompareCounts = true;
#else
constexpr bool CompareCounts = false;
#endif

/// Read \a fileName and verify the isocontour points of every scalar
/// cell-attribute at each of Fractions, checking their counts against
/// \a expected (keyed by attribute name, one entry per fraction).
bool TestIsocontourPoints(int argc, char* argv[], const char* fileName,
  const std::map<std::string, std::array<IsocontourCounts, 3>>& expected)
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
  bool ok = true;
  auto* shapeAttribute = grid->GetShapeAttribute();
  for (const auto& attribute : grid->GetCellAttributeList())
  {
    if (attribute.GetPointer() == shapeAttribute || attribute->GetNumberOfComponents() != 1)
    {
      continue;
    }
    const std::string attributeName = attribute->GetName().Data();
    auto it = expected.find(attributeName);
    if (it == expected.end())
    {
      std::cerr << "  ERROR: no expected counts recorded for \"" << attributeName << "\".\n";
      ok = false;
    }
    for (std::size_t ff = 0; ff < Fractions.size(); ++ff)
    {
      IsocontourCounts counts;
      ok &= CheckIsocontourPoints(grid, attribute, Fractions[ff], counts);
      std::cout << "  " << attributeName << " @ " << Fractions[ff] << ": " << counts << "\n";
      if (CompareCounts && it != expected.end() && !(counts == it->second[ff]))
      {
        std::cerr << "    ERROR: expected " << it->second[ff] << ", got " << counts << ".\n";
        ok = false;
      }
    }
  }
  return ok;
}

} // anonymous namespace

int TestCellGridIsocontourPoints(int argc, char* argv[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  bool ok = true;

  // clang-format off
  // Grouped by cell type, in vtkDGCell::Shape order (Edge, Triangle,
  // Quadrilateral, Tetrahedron, Hexahedron, Wedge, Pyramid), then Mixed.
  // Each attribute's counts at the isovalues in Fractions: { points, cells }.

  // Edge
  ok &= TestIsocontourPoints(argc, argv, "Data/dgEdges.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 5, 5 }, { 5, 5 }, { 4, 4 } } } },
    { "scalar2", { { { 1, 1 }, { 2, 2 }, { 2, 2 } } } },
    { "scalar3", { { { 3, 3 }, { 3, 3 }, { 2, 2 } } } },
  });
  ok &= TestIsocontourPoints(argc, argv, "Data/dgEdge-GBasis.dg", {
    { "G1", { { { 2, 2 }, { 2, 2 }, { 2, 2 } } } },
    { "G2", { { { 0, 0 }, { 4, 2 }, { 4, 2 } } } },
    { "G3", { { { 0, 0 }, { 6, 2 }, { 6, 2 } } } },
    { "G4", { { { 0, 0 }, { 0, 0 }, { 8, 2 } } } },
    { "G5", { { { 0, 0 }, { 0, 0 }, { 10, 2 } } } },
  });

  // Triangle
  ok &= TestIsocontourPoints(argc, argv, "Data/dgTriangle.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 4, 2 }, { 2, 1 }, { 2, 1 } } } },
    { "scalar2", { { { 2, 1 }, { 2, 1 }, { 2, 1 } } } },
    { "scalar3", { { { 2, 1 }, { 2, 1 }, { 2, 1 } } } },
  });
  ok &= TestIsocontourPoints(argc, argv, "Data/dgTriangle-GBasis.dg", {
    { "G1", { { { 4, 2 }, { 4, 2 }, { 4, 2 } } } },
    { "G2", { { { 0, 0 }, { 12, 2 }, { 12, 2 } } } },
    { "G3", { { { 0, 0 }, { 18, 2 }, { 18, 2 } } } },
    { "G4", { { { 0, 0 }, { 52, 2 }, { 28, 2 } } } },
    { "G5", { { { 0, 0 }, { 34, 2 }, { 34, 2 } } } },
  });

  // Quadrilateral
  ok &= TestIsocontourPoints(argc, argv, "Data/dgQuadrilateral.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 7, 2 }, { 4, 1 }, { 3, 1 } } } },
    { "scalar2", { { { 3, 1 }, { 3, 1 }, { 3, 1 } } } },
    { "scalar3", { { { 4, 1 }, { 4, 1 }, { 4, 1 } } } },
  });
  ok &= TestIsocontourPoints(argc, argv, "Data/dgQuad-GBasis.dg", {
    { "G1", { { { 12, 2 }, { 8, 2 }, { 8, 2 } } } },
    { "G2", { { { 0, 0 }, { 24, 2 }, { 24, 2 } } } },
    { "G3", { { { 0, 0 }, { 48, 2 }, { 48, 2 } } } },
    { "G4", { { { 0, 0 }, { 80, 2 }, { 80, 2 } } } },
    { "G5", { { { 0, 0 }, { 120, 2 }, { 0, 0 } } } },
  });
  ok &= TestIsocontourPoints(argc, argv, "Data/dgQuadraticQuadrilaterals.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 21, 2 }, { 32, 2 }, { 0, 0 } } } },
    { "scalar2", { { { 0, 0 }, { 22, 2 }, { 0, 0 } } } },
    { "scalar3", { { { 0, 0 }, { 56, 2 }, { 0, 0 } } } },
    { "linear", { { { 3, 1 }, { 3, 1 }, { 3, 1 } } } },
  });

  // Tetrahedron
  ok &= TestIsocontourPoints(argc, argv, "Data/dgTetrahedra.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 7, 2 }, { 4, 1 }, { 3, 1 } } } },
    { "scalar2", { { { 3, 1 }, { 3, 1 }, { 3, 1 } } } },
    { "scalar3", { { { 3, 1 }, { 3, 1 }, { 6, 2 } } } },
    { "G1", { { { 3, 1 }, { 6, 2 }, { 3, 1 } } } },
    { "G2", { { { 0, 0 }, { 62, 2 }, { 33, 2 } } } },
    { "G3", { { { 0, 0 }, { 8, 1 }, { 0, 0 } } } },
    { "G4", { { { 0, 0 }, { 9, 2 }, { 0, 0 } } } },
    { "G5", { { { 0, 0 }, { 42, 1 }, { 0, 0 } } } },
  });

  // Hexahedron
  ok &= TestIsocontourPoints(argc, argv, "Data/dgHexahedra.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 28, 2 }, { 40, 2 }, { 28, 2 } } } },
    { "scalar2", { { { 16, 1 }, { 0, 0 }, { 16, 1 } } } },
    { "scalar3", { { { 28, 2 }, { 28, 2 }, { 28, 2 } } } },
    { "gauss_cubic", { { { 116, 2 }, { 476, 2 }, { 300, 2 } } } },
    { "quadratic", { { { 135, 1 }, { 392, 2 }, { 140, 1 } } } },
  });
  ok &= TestIsocontourPoints(argc, argv, "Data/dgGhostHexahedra.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 28, 2 }, { 40, 2 }, { 28, 2 } } } },
    { "scalar2", { { { 16, 1 }, { 0, 0 }, { 16, 1 } } } },
    { "scalar3", { { { 28, 2 }, { 28, 2 }, { 28, 2 } } } },
    { "quadratic", { { { 135, 1 }, { 392, 2 }, { 140, 1 } } } },
  });
  ok &= TestIsocontourPoints(argc, argv, "Data/dgHexahedra-GBasis.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 28, 2 }, { 40, 2 }, { 28, 2 } } } },
    { "scalar2", { { { 16, 1 }, { 0, 0 }, { 16, 1 } } } },
    { "scalar3", { { { 28, 2 }, { 28, 2 }, { 28, 2 } } } },
    { "G1", { { { 80, 2 }, { 26, 2 }, { 26, 2 } } } },
    { "G2", { { { 0, 0 }, { 120, 2 }, { 120, 2 } } } },
    { "G3", { { { 0, 0 }, { 228, 2 }, { 228, 2 } } } },
    { "G4", { { { 0, 0 }, { 0, 0 }, { 408, 2 } } } },
    { "G5", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "gauss_cubic", { { { 116, 2 }, { 476, 2 }, { 300, 2 } } } },
    { "quadratic", { { { 135, 1 }, { 392, 2 }, { 140, 1 } } } },
  });
  // fandisk.dg's only attribute ("curlyq") has 3 components, so nothing
  // here is scalar and there is nothing to check.
  ok &= TestIsocontourPoints(argc, argv, "Data/fandisk.dg", {});

  // Wedge
  ok &= TestIsocontourPoints(argc, argv, "Data/dgWedges.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 17, 2 }, { 17, 2 }, { 17, 2 } } } },
    { "scalar2", { { { 8, 1 }, { 8, 1 }, { 8, 1 } } } },
    { "scalar3", { { { 8, 1 }, { 16, 2 }, { 8, 1 } } } },
  });
  ok &= TestIsocontourPoints(argc, argv, "Data/dgWedges-GBasis.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 17, 2 }, { 17, 2 }, { 17, 2 } } } },
    { "scalar2", { { { 8, 1 }, { 8, 1 }, { 8, 1 } } } },
    { "scalar3", { { { 8, 1 }, { 16, 2 }, { 8, 1 } } } },
    { "G1", { { { 30, 2 }, { 14, 2 }, { 14, 2 } } } },
    { "G2", { { { 0, 0 }, { 58, 2 }, { 58, 2 } } } },
    { "G3", { { { 0, 0 }, { 145, 2 }, { 145, 2 } } } },
    { "G4", { { { 0, 0 }, { 666, 2 }, { 226, 2 } } } },
    { "G5", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
  });

  // Pyramid
  ok &= TestIsocontourPoints(argc, argv, "Data/dgPyramids.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 12, 2 }, { 10, 2 }, { 8, 2 } } } },
    { "scalar2", { { { 4, 1 }, { 0, 0 }, { 4, 1 } } } },
    { "scalar3", { { { 6, 1 }, { 6, 1 }, { 6, 1 } } } },
  });
  ok &= TestIsocontourPoints(argc, argv, "Data/dgPyramids-GBasis.dg", {
    { "G1", { { { 6, 1 }, { 6, 1 }, { 6, 1 } } } },
    { "G2", { { { 0, 0 }, { 10, 1 }, { 12, 1 } } } },
    { "G3", { { { 0, 0 }, { 20, 1 }, { 0, 0 } } } },
    { "G4", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "G5", { { { 0, 0 }, { 110, 1 }, { 0, 0 } } } },
  });

  // Mixed (tetrahedra and quadrilaterals): each attribute is one
  // vtkCellAttribute spanning both cell types, so its counts here are the
  // two shapes' combined total, not per-shape.
  ok &= TestIsocontourPoints(argc, argv, "Data/dgMixed.dg", {
    { "scalar0", { { { 0, 0 }, { 0, 0 }, { 0, 0 } } } },
    { "scalar1", { { { 10, 3 }, { 8, 2 }, { 6, 2 } } } },
    { "scalar2", { { { 3, 1 }, { 3, 1 }, { 6, 2 } } } },
    { "scalar3", { { { 7, 2 }, { 7, 2 }, { 6, 2 } } } },
  });
  // clang-format on

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
