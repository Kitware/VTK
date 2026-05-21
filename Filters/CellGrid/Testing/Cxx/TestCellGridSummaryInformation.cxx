// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridReader.h"
#include "vtkCellGridSummaryInformationQuery.h"
#include "vtkFiltersCellGrid.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"

#include <iostream>

namespace
{

struct SummaryExpectation
{
  std::string Name;
  int MinOrder = -1;
  int MaxOrder = -1;
  vtkIdType DOF = -1;
};

struct SummaryExpectations
{
  std::vector<SummaryExpectation> Attributes;
};

bool TestSummary(const char* filename, const SummaryExpectations& expected)
{
  if (!filename)
  {
    return false;
  }

  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;

  auto* grid = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!grid)
  {
    std::cerr << "ERROR: Could not read cell-grid.\n";
    return false;
  }

  vtkNew<vtkCellGridSummaryInformationQuery> query;
  if (!grid->Query(query))
  {
    std::cerr << "ERROR: Query returned false.\n";
    return false;
  }

  bool ok = true;

  for (const auto& attExp : expected.Attributes)
  {
    auto* att = grid->GetCellAttributeByName(attExp.Name);
    if (!att)
    {
      std::cerr << "ERROR: Attribute \"" << attExp.Name << "\" not found.\n";
      ok = false;
      continue;
    }

    // --- Polynomial order range ---
    const auto& orderRange = query->GetOrderRange(att);
    std::cout << "  " << attExp.Name << ": order [" << orderRange[0] << ", " << orderRange[1]
              << "]";
    if (orderRange[0] != attExp.MinOrder || orderRange[1] != attExp.MaxOrder)
    {
      std::cerr << "\nERROR: Attribute \"" << attExp.Name << "\" expected order ["
                << attExp.MinOrder << ", " << attExp.MaxOrder << "] but got [" << orderRange[0]
                << ", " << orderRange[1] << "].\n";
      ok = false;
    }

    // --- Degrees of freedom ---
    vtkIdType dof = query->GetNumberOfDOF(att);
    std::cout << ", DOF " << dof << "\n";
    if (dof != attExp.DOF)
    {
      std::cerr << "ERROR: Attribute \"" << attExp.Name << "\" expected " << attExp.DOF
                << " DOF(s), got " << dof << ".\n";
      ok = false;
    }
  }

  return ok;
}

} // anonymous namespace

int TestCellGridSummaryInformation(int argc, char* argv[])
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();

  bool ok = true;

  // shape/scalar3: shared, DOF = unique non-ghost point IDs via connectivity.
  // scalar0/1/2: discontinuous, DOF = tuples × components.
  // div1/curl1/quadratic/linear: discontinuous, DOF = tuples × components.

  std::cout << "## dgTetrahedra.dg\n";
  // 2 tets: shape/scalar3 share conn (5 unique pts), scalar0/1/2 = 2×4=8,
  // div1 = 2×4=8, curl1 = 2×6=12.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTetrahedra.dg"),
    { { { "shape", 1, 1, 5 }, { "scalar0", 1, 1, 8 }, { "scalar1", 1, 1, 8 },
      { "scalar2", 1, 1, 8 }, { "scalar3", 1, 1, 5 }, { "div1", 1, 1, 8 },
      { "curl1", 1, 1, 12 } } });

  std::cout << "## dgHexahedra.dg\n";
  // 2 hexes: shape/scalar3 share conn (12 unique pts), scalar0/1/2 = 2×8=16,
  // quadratic = 2×20=40, curl1 = 2×12=24, div1 = 2×6=12.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg"),
    { { { "shape", 1, 1, 12 }, { "scalar0", 1, 1, 16 }, { "scalar1", 1, 1, 16 },
      { "scalar2", 1, 1, 16 }, { "scalar3", 1, 1, 12 }, { "quadratic", 2, 2, 40 },
      { "curl1", 1, 1, 24 }, { "div1", 1, 1, 12 } } });

  std::cout << "## dgGhostHexahedra.dg\n";
  // 2 hexes with ghost points: shape/scalar3 share conn (4 non-ghost unique pts),
  // scalar0/1/2 = 2×8=16, quadratic = 2×20=40, curl1 = 2×12=24.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgGhostHexahedra.dg"),
    { { { "shape", 1, 1, 4 }, { "scalar0", 1, 1, 16 }, { "scalar1", 1, 1, 16 },
      { "scalar2", 1, 1, 16 }, { "scalar3", 1, 1, 4 }, { "quadratic", 2, 2, 40 },
      { "curl1", 1, 1, 24 } } });

  std::cout << "## dgEdges.dg\n";
  // 5 edges: shape/scalar3 share conn (4 unique pts), scalar0/1/2 = 5×2=10.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgEdges.dg"),
    { { { "shape", 1, 1, 4 }, { "scalar0", 1, 1, 10 }, { "scalar1", 1, 1, 10 },
      { "scalar2", 1, 1, 10 }, { "scalar3", 1, 1, 4 } } });

  std::cout << "## dgMixed.dg\n";
  // Mixed tets+quads: shape/scalar3 share conn (9 unique pts across both cell types),
  // scalar0/1/2 = 2×4+1×4=12.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgMixed.dg"),
    { { { "shape", 1, 1, 9 }, { "scalar0", 1, 1, 12 }, { "scalar1", 1, 1, 12 },
      { "scalar2", 1, 1, 12 }, { "scalar3", 1, 1, 9 } } });

  std::cout << "## dgPyramid19.dg\n";
  // 1 quadratic pyramid (19-node): shape/scalar3 share conn (19 unique pts), order=2.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgPyramid19.dg"),
    { { { "shape", 2, 2, 19 }, { "scalar3", 2, 2, 19 } } });

  std::cout << "## dgPyramids.dg\n";
  // 2 linear pyramids: shape/scalar3 share conn (7 unique pts), scalar0/1/2 = 2×5=10.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgPyramids.dg"),
    { { { "shape", 1, 1, 7 }, { "scalar0", 1, 1, 10 }, { "scalar1", 1, 1, 10 },
      { "scalar2", 1, 1, 10 }, { "scalar3", 1, 1, 7 } } });

  std::cout << "## dgQuadraticQuadrilaterals.dg\n";
  // 2 quadratic quads: shape/scalar3 share conn (15 unique pts), order=2.
  // scalar0/1/2 = 2×9=18, linear = 2×4=8 (order 1).
  ok &= TestSummary(
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgQuadraticQuadrilaterals.dg"),
    { { { "shape", 2, 2, 15 }, { "scalar0", 2, 2, 18 }, { "scalar1", 2, 2, 18 },
      { "scalar2", 2, 2, 18 }, { "scalar3", 2, 2, 15 }, { "linear", 1, 1, 8 } } });

  std::cout << "## dgQuadrilateral.dg\n";
  // 2 linear quads: shape/scalar3 share conn (6 unique pts), scalar0/1/2 = 2×4=8.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgQuadrilateral.dg"),
    { { { "shape", 1, 1, 6 }, { "scalar0", 1, 1, 8 }, { "scalar1", 1, 1, 8 },
      { "scalar2", 1, 1, 8 }, { "scalar3", 1, 1, 6 } } });

  std::cout << "## dgTriangle.dg\n";
  // 2 triangles: shape/scalar3 share conn (4 unique pts), scalar0/1/2 = 2×3=6.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTriangle.dg"),
    { { { "shape", 1, 1, 4 }, { "scalar0", 1, 1, 6 }, { "scalar1", 1, 1, 6 },
      { "scalar2", 1, 1, 6 }, { "scalar3", 1, 1, 4 } } });

  std::cout << "## dgWedges.dg\n";
  // 2 wedges: shape/scalar3 share conn (8 unique pts), scalar0/1/2 = 2×6=12.
  ok &= TestSummary(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgWedges.dg"),
    { { { "shape", 1, 1, 8 }, { "scalar0", 1, 1, 12 }, { "scalar1", 1, 1, 12 },
      { "scalar2", 1, 1, 12 }, { "scalar3", 1, 1, 8 } } });

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
