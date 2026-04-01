// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellLocator.h"
#include "vtkCellTreeLocator.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkJumpAndWalkCellLocator.h"
#include "vtkModifiedBSPTree.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkStaticCellLocator.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

#include <iostream>

static bool TestCell(vtkDataSet* ds, vtkIdType cellId, double x1[3], double x2[3], double tol)
{
  double t = 0.0;
  double x[3] = { 0.0, 0.0, 0.0 };
  double pcoords[3] = { 0.0, 0.0, 0.0 };
  int subId = 0;
  vtkNew<vtkGenericCell> cell;
  ds->GetCell(cellId, cell);

  return static_cast<bool>(cell->IntersectWithLine(x1, x2, tol, t, x, pcoords, subId));
}

static bool TestLocatorIntersectWithLine(vtkDataSet* ds, vtkAbstractCellLocator* loc)
{
  std::cout << "\nTesting " << loc->GetClassName() << std::endl;
  loc->SetDataSet(ds);
  loc->CacheCellBoundsOn();
  loc->AutomaticOn();
  loc->BuildLocator();

  vtkNew<vtkGenericCell> cell;
  vtkNew<vtkIdList> cellList;
  double t = 0.0;
  double x[3] = { 0.0, 0.0, 0.0 };
  double pcoords[3] = { 0.0, 0.0, 0.0 };
  int subId = 0;
  vtkIdType cellId = -1;
  double tol = 1.0e-15;
  double x1[3] = { 0.437783024586950, 0.0263950841209563, 0.373722994626027 };
  double x2[3] = { 0.442140196830658, 0.0256207765183134, 0.374080391702881 };

  // This IntersectWithLine returns the intersected cell with the smallest parametric t.
  bool foundIntersectWithLineBest = false;
  loc->IntersectWithLine(x1, x2, tol, t, x, pcoords, subId, cellId, cell);
  if (cellId != -1)
  {
    std::cout << "IntersectWithLineBest: " << cellId << std::endl;
    foundIntersectWithLineBest = TestCell(ds, cellId, x1, x2, tol);
  }

  // This IntersectWithLine returns all the cells that intersected with the line
  bool foundIntersectWithLineAll = false;
  loc->IntersectWithLine(x1, x2, tol, nullptr, cellList, cell);
  for (vtkIdType i = 0; i < cellList->GetNumberOfIds(); ++i)
  {
    std::cout << "IntersectWithLineAll: " << cellList->GetId(i) << std::endl;
    foundIntersectWithLineAll |= TestCell(ds, cellList->GetId(i), x1, x2, tol);
  }

  // This FindCellAlongLine (which is actually the above version without passing a cell)
  // returns all the cells that their bounds intersected with the line
  bool foundFindCellAlongLine = false;
  loc->FindCellsAlongLine(x1, x2, tol, cellList);
  for (vtkIdType i = 0; i < cellList->GetNumberOfIds(); ++i)
  {
    std::cout << "FindCellAlongLine: " << cellList->GetId(i) << std::endl;
    foundFindCellAlongLine |= TestCell(ds, cellList->GetId(i), x1, x2, tol);
  }
  return foundIntersectWithLineBest && foundIntersectWithLineAll && foundFindCellAlongLine;
}

static bool TestLocatorFindClosestPointWithinRadius(vtkDataSet* ds, vtkAbstractCellLocator* loc)
{
  loc->SetDataSet(ds);
  loc->CacheCellBoundsOn();
  loc->BuildLocator();

  double test_point[] = { -5.091451e-02, -1.800857e-01, 1.153756e+00 };

  // expected result
  constexpr double dist2_exp = 1.658136e-01;
  constexpr double closest_point_exp[] = { -1.582647e-01, -5.475835e-01, 1.015066e+00 };
  constexpr int cell_id_exp = 1944;

  // threshold for floating point checking
  constexpr double thresh = 1e-5;

  constexpr double radius = 0.5;
  double closest_point[3];
  vtkIdType cell_id;
  int sub_id, inside;
  double dist2;
  vtkNew<vtkGenericCell> cell;

  loc->FindClosestPointWithinRadius(
    test_point, radius, closest_point, cell, cell_id, sub_id, dist2, inside);
  if (fabs(dist2 - dist2_exp) / fabs(dist2_exp) < thresh &&
    fabs(closest_point[0] - closest_point_exp[0]) / fabs(closest_point_exp[0]) < thresh &&
    fabs(closest_point[1] - closest_point_exp[1]) / fabs(closest_point_exp[1]) < thresh &&
    fabs(closest_point[2] - closest_point_exp[2]) / fabs(closest_point_exp[2]) < thresh &&
    cell_id == cell_id_exp)
    return true;
  return false;
}

static bool TestCellLocatorFindCell(vtkDataSet* ds, vtkAbstractCellLocator* loc)
{
  std::cout << "\nTesting " << loc->GetClassName() << "::FindCell" << std::endl;

  loc->SetDataSet(ds);
  loc->CacheCellBoundsOn();
  loc->AutomaticOn();
  loc->BuildLocator();

  vtkNew<vtkGenericCell> cell;
  int subId = 0;
  vtkIdType cellId = -1;
  double pcoords[3] = { 0.0, 0.0, 0.0 };
  double weights[3] = { 0.0, 0.0, 0.0 };

  double x1[3] = { -0.17, -0.19, 0.32 };
  double x2[3] = { -0.202, -0.210, 0.352 };
  constexpr double tol2 = 0.03 * 0.03;
  constexpr vtkIdType closestCellIdGT = 66;
  constexpr double weights1GT[3] = { 0.455247, 0.427978, 0.116775 };
  constexpr double weights2GT[3] = { 0.444189, 0.509807, 0.046004 };
  constexpr double largeTol2 = 0.08 * 0.08;
  constexpr vtkIdType altCIdLargeTol_a = 57;
  constexpr vtkIdType altCIdLargeTol_b = 56;
  constexpr double altW1_a[3] = { 0.037569, 0.506163, 0.456268 };
  constexpr double altW2_a[3] = { 0.013755, 0.540375, 0.445870 };
  constexpr double altW2_b[3] = { 0.459625, -0.247439, 0.787814 };

  bool allTestPassed = true;

  using vtk::format;
  cellId = loc->FindCell(x1, largeTol2, cell, subId, pcoords, weights);
  std::cout << "Find cell for first point and large tolerance: " << std::endl
            << "  cellId = " << cellId << " with weights = " << format("{}", weights) << std::endl;
  allTestPassed &=
    ((cellId == closestCellIdGT && vtkMath::Distance2BetweenPoints(weights, weights1GT) < 1e-12) ||
      (cellId == altCIdLargeTol_a && vtkMath::Distance2BetweenPoints(weights, altW1_a) < 1e-12));

  cellId = loc->FindCell(x2, largeTol2, cell, subId, pcoords, weights);
  std::cout << "Find cell for second point and large tolerance: " << std::endl
            << "  cellId = " << cellId << " with weights = " << format("{}", weights) << std::endl;
  allTestPassed &=
    ((cellId == closestCellIdGT && vtkMath::Distance2BetweenPoints(weights, weights2GT) < 1e-12) ||
      (cellId == altCIdLargeTol_a && vtkMath::Distance2BetweenPoints(weights, altW2_a) < 1e-12) ||
      (cellId == altCIdLargeTol_b && vtkMath::Distance2BetweenPoints(weights, altW2_b) < 1e-12));

  cellId = loc->FindCell(x1, tol2, cell, subId, pcoords, weights);
  std::cout << "Find cell for first point and medium tolerance: " << std::endl
            << "  cellId = " << cellId << std::endl;
  allTestPassed &= (cellId == -1);

  cellId = loc->FindCell(x2, tol2, cell, subId, pcoords, weights);
  std::cout << "Find cell for second point and medium tolerance: " << std::endl
            << "  cellId = " << cellId << " with weights = " << format("{}", weights) << std::endl;
  allTestPassed &=
    (cellId == closestCellIdGT && vtkMath::Distance2BetweenPoints(weights, weights2GT) < 1e-12);

  return allTestPassed;
}

int TestCellLocatorsEdgeCases(int argc, char* argv[])
{
  // create locators
  vtkNew<vtkCellLocator> cl;
  cl->SetNumberOfCellsPerNode(2);
  vtkNew<vtkStaticCellLocator> scl;
  vtkNew<vtkCellTreeLocator> ctl;
  vtkNew<vtkModifiedBSPTree> mbsp;
  vtkNew<vtkJumpAndWalkCellLocator> jwcl;

  //=====================
  // Create/Read DataSets
  //=====================
  vtkNew<vtkXMLPolyDataReader> test_surface_reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/test_surface.vtp");
  test_surface_reader->SetFileName(fname);
  delete[] fname;
  test_surface_reader->Update();
  vtkPolyData* test_surface = test_surface_reader->GetOutput();

  vtkNew<vtkXMLPolyDataReader> coneReader;
  char* fname2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cone.vtp");
  coneReader->SetFileName(fname2);
  delete[] fname2;
  coneReader->Update();
  vtkPolyData* cone = coneReader->GetOutput();

  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(0., 0., 0.);
  sphereSource->SetRadius(0.5);
  sphereSource->SetThetaResolution(8);
  sphereSource->SetPhiResolution(8);
  sphereSource->Update();
  vtkSmartPointer<vtkPolyData> sphere = sphereSource->GetOutput();

  bool allTestsPassed = true;

  // TestLocatorIntersectWithLine
  allTestsPassed &= TestLocatorIntersectWithLine(test_surface, cl);
  allTestsPassed &= TestLocatorIntersectWithLine(test_surface, scl);
  allTestsPassed &= TestLocatorIntersectWithLine(test_surface, ctl);
  // vtkJumpAndWalkCellLocator does not support IntersectWithLine
  allTestsPassed &= TestLocatorIntersectWithLine(test_surface, mbsp);

  // TestLocatorFindClosestPointWithinRadius
  allTestsPassed &= TestLocatorFindClosestPointWithinRadius(cone, cl);
  allTestsPassed &= TestLocatorFindClosestPointWithinRadius(cone, scl);
  // vtkCellTreeLocator does not support FindClosestPointWithinRadius
  // vtkModifiedBSPTree does not support FindClosestPointWithinRadius
  allTestsPassed &= TestLocatorFindClosestPointWithinRadius(cone, jwcl);

  // TestCellLocatorFindCell
  allTestsPassed &= TestCellLocatorFindCell(sphere, cl);
  allTestsPassed &= TestCellLocatorFindCell(sphere, scl);
  allTestsPassed &= TestCellLocatorFindCell(sphere, ctl);
  allTestsPassed &= TestCellLocatorFindCell(sphere, mbsp);
  allTestsPassed &= TestCellLocatorFindCell(sphere, jwcl);

  //====================
  // Final Tests Outcome
  //====================
  return allTestsPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
