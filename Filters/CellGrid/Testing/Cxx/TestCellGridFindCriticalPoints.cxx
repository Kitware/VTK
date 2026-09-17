// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercise vtkCellGridFindCriticalPoints against every real data file
// TestCellGridExtractSurface.cxx reads - one shape per file - plus every
// other example .dg dataset, over every scalar cell-attribute each one
// carries, converted to Bezier first (via vtkCellGridChangeBasis, at the
// attribute's own order, which is exact) since none of them start out in
// that basis. The total critical points found and their breakdown by
// classification (minima, maxima, saddles) are checked against values
// recorded from the solver itself - a regression baseline, not a
// hand-derived one - since these fields have no simpler closed form to check
// against.

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridChangeBasis.h"
#include "vtkCellGridFindCriticalPoints.h"
#include "vtkCellGridReader.h"
#include "vtkDGCell.h"
#include "vtkFiltersCellGrid.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringToken.h"
#include "vtkTestUtilities.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

namespace
{
using namespace vtk::literals;

/// How many critical points one attribute has, broken down by
/// classification (indices matching the output's own "Type" convention:
/// 0 = minimum, 1 = maximum, 2 = saddle).
struct CriticalPointCounts
{
  vtkIdType Total{ 0 };
  std::array<vtkIdType, 3> ByType{ { 0, 0, 0 } };

  bool operator==(const CriticalPointCounts& other) const
  {
    return this->Total == other.Total && this->ByType == other.ByType;
  }
};

std::ostream& operator<<(std::ostream& os, const CriticalPointCounts& counts)
{
  os << counts.Total << " total (" << counts.ByType[0] << " min, " << counts.ByType[1] << " max, "
     << counts.ByType[2] << " saddle)";
  return os;
}

/// Run vtkCellGridFindCriticalPoints on \a attribute (converting it to
/// Bezier first, at its own order, if it is not already) and tally its
/// results.
bool CountCriticalPoints(
  vtkCellGrid* grid, vtkCellAttribute* attribute, CriticalPointCounts& counts)
{
  vtkSmartPointer<vtkCellGrid> bezierGrid = grid;
  vtkCellAttribute* bezierAttribute = attribute;

  // An attribute may not carry the same function space/basis/order on every
  // cell type it has info for; find one it actually applies to so we know
  // what to convert from.
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
      if (!bezierGrid)
      {
        std::cerr << "    ERROR: vtkCellGridChangeBasis produced no output for \""
                  << attribute->GetName().Data() << "\".\n";
        return false;
      }
      bezierAttribute = bezierGrid->GetCellAttributeByName(attribute->GetName().Data());
      if (!bezierAttribute)
      {
        std::cerr << "    ERROR: converted grid has no \"" << attribute->GetName().Data()
                  << "\" attribute.\n";
        return false;
      }
    }
    break;
  }

  vtkNew<vtkCellGridFindCriticalPoints> filter;
  filter->SetInputDataObject(bezierGrid);
  filter->SetCellAttribute(bezierAttribute);
  filter->Update();

  auto* output = filter->GetOutput();
  counts = CriticalPointCounts{};
  counts.Total = output ? output->GetNumberOfPoints() : 0;
  auto* types = output ? output->GetPointData()->GetArray("Type") : nullptr;
  for (vtkIdType ii = 0; ii < counts.Total; ++ii)
  {
    const int type = static_cast<int>(types->GetTuple1(ii));
    if (type < 0 || type > 2)
    {
      std::cerr << "    ERROR: unexpected classification " << type << ".\n";
      return false;
    }
    ++counts.ByType[type];
  }
  return true;
}

/// Read \a fileName and check every scalar cell-attribute's critical-point
/// counts against \a expected (keyed by attribute name).
bool TestCriticalPoints(int argc, char* argv[], const char* fileName,
  const std::map<std::string, CriticalPointCounts>& expected)
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
    CriticalPointCounts counts;
    if (!CountCriticalPoints(grid, attribute, counts))
    {
      ok = false;
      continue;
    }
    std::cout << "  " << attributeName << ": " << counts << "\n";

    auto it = expected.find(attributeName);
    if (it == expected.end())
    {
      std::cerr << "    ERROR: no expected counts recorded for \"" << attributeName << "\".\n";
      ok = false;
      continue;
    }
    if (!(counts == it->second))
    {
      std::cerr << "    ERROR: expected " << it->second << ", got " << counts << ".\n";
      ok = false;
    }
  }
  return ok;
}

} // anonymous namespace

int TestCellGridFindCriticalPoints(int argc, char* argv[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
  bool ok = true;

  // clang-format off
  // Grouped by cell type, in vtkDGCell::Shape order (Edge, Triangle,
  // Quadrilateral, Tetrahedron, Hexahedron, Wedge, Pyramid), then Mixed.

  // Edge
  ok &= TestCriticalPoints(argc, argv, "Data/dgEdges.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
  });
  ok &= TestCriticalPoints(argc, argv, "Data/dgEdge-GBasis.dg", {
    { "G1", {} },
    { "G2", { 2, { 2, 0, 0 } } },
    { "G3", { 4, { 2, 2, 0 } } },
    { "G4", { 6, { 4, 2, 0 } } },
    { "G5", { 8, { 4, 4, 0 } } },
  });

  // Triangle
  ok &= TestCriticalPoints(argc, argv, "Data/dgTriangle.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
  });
  ok &= TestCriticalPoints(argc, argv, "Data/dgTriangle-GBasis.dg", {
    { "G1", {} },
    { "G2", { 8, { 0, 0, 8 } } },
    { "G3", { 14, { 0, 2, 12 } } },
    { "G4", { 24, { 2, 2, 20 } } },
    { "G5", { 30, { 2, 4, 24 } } },
  });

  // Quadrilateral
  ok &= TestCriticalPoints(argc, argv, "Data/dgQuadrilateral.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
  });
  ok &= TestCriticalPoints(argc, argv, "Data/dgQuad-GBasis.dg", {
    { "G1", { 2, { 0, 0, 2 } } },
    { "G2", { 18, { 0, 2, 16 } } },
    { "G3", { 42, { 4, 4, 34 } } },
    { "G4", { 74, { 8, 10, 56 } } },
    { "G5", { 114, { 16, 16, 82 } } },
  });
  ok &= TestCriticalPoints(argc, argv, "Data/dgQuadraticQuadrilaterals.dg", {
    { "linear", {} },
    { "scalar0", {} },
    { "scalar1", { 9, { 0, 0, 9 } } },
    { "scalar2", { 6, { 1, 1, 4 } } },
    { "scalar3", { 14, { 1, 5, 8 } } },
  });

  // Tetrahedron
  ok &= TestCriticalPoints(argc, argv, "Data/dgTetrahedra.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
    { "G1", {} },
    { "G2", { 27, { 0, 0, 27 } } },
    { "G3", { 2, { 0, 0, 2 } } },
    { "G4", { 3, { 0, 0, 3 } } },
    { "G5", { 13, { 0, 0, 13 } } },
  });

  // Hexahedron
  ok &= TestCriticalPoints(argc, argv, "Data/dgHexahedra.dg", {
    { "scalar0", {} },
    { "scalar1", { 2, { 0, 0, 2 } } },
    { "scalar2", {} },
    { "scalar3", {} },
    { "quadratic", { 49, { 6, 6, 37 } } },
    { "gauss_cubic", { 46, { 10, 0, 36 } } },
  });
  ok &= TestCriticalPoints(argc, argv, "Data/dgGhostHexahedra.dg", {
    { "scalar0", {} },
    { "scalar1", { 2, { 0, 0, 2 } } },
    { "scalar2", {} },
    { "scalar3", {} },
    { "quadratic", { 49, { 6, 6, 37 } } },
  });
  ok &= TestCriticalPoints(argc, argv, "Data/dgHexahedra-GBasis.dg", {
    { "scalar0", {} },
    { "scalar1", { 2, { 0, 0, 2 } } },
    { "scalar2", {} },
    { "scalar3", {} },
    { "quadratic", { 49, { 6, 6, 37 } } },
    { "gauss_cubic", { 46, { 10, 0, 36 } } },
    { "G1", { 12, { 0, 0, 12 } } },
    { "G2", { 106, { 2, 0, 104 } } },
    { "G3", { 214, { 0, 2, 212 } } },
    { "G4", { 394, { 2, 0, 392 } } },
    { "G5", { 616, { 0, 2, 614 } } },
  });
  // fandisk.dg's only attribute ("curlyq") has 3 components, so nothing
  // here is scalar and there is nothing to check.
  ok &= TestCriticalPoints(argc, argv, "Data/fandisk.dg", {});

  // Wedge
  ok &= TestCriticalPoints(argc, argv, "Data/dgWedges.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
  });
  ok &= TestCriticalPoints(argc, argv, "Data/dgWedges-GBasis.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
    { "G1", { 4, { 0, 0, 4 } } },
    { "G2", { 48, { 0, 0, 48 } } },
    { "G3", { 135, { 1, 1, 133 } } },
    { "G4", { 216, { 2, 4, 210 } } },
    { "G5", { 281, { 2, 3, 276 } } },
  });

  // Pyramid.
  ok &= TestCriticalPoints(argc, argv, "Data/dgPyramids.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
  });
  ok &= TestCriticalPoints(argc, argv, "Data/dgPyramids-GBasis.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
    { "G1", {} },
    { "G2", { 2, { 0, 0, 2 } } },
    { "G3", { 4, { 0, 0, 4 } } },
    { "G4", { 6, { 0, 0, 6 } } },
    { "G5", { 65, { 1, 0, 64 } } },
  });

  // Mixed (tetrahedra and quadrilaterals): each attribute is one
  // vtkCellAttribute spanning both cell types, so its counts here are the
  // two shapes' combined total, not per-shape.
  ok &= TestCriticalPoints(argc, argv, "Data/dgMixed.dg", {
    { "scalar0", {} },
    { "scalar1", {} },
    { "scalar2", {} },
    { "scalar3", {} },
  });
  // clang-format on

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
