// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellLocator.h"
#include "vtkCellTreeLocator.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkStaticCellLocator.h"
#include "vtkXMLPolyDataReader.h"

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

static bool TestLocator(vtkDataSet* ds, vtkAbstractCellLocator* loc)
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

static bool TestCellLocatorEvaluatePosition(char* fname)
{
  vtkNew<vtkXMLPolyDataReader> poly_reader;
  poly_reader->SetFileName(fname);
  poly_reader->Update();

  vtkNew<vtkCellLocator> loc;
  loc->SetDataSet(poly_reader->GetOutput());
  loc->CacheCellBoundsOn();
  loc->SetNumberOfCellsPerNode(2);
  loc->BuildLocator();

  double test_point[] = { -5.091451e-02, -1.800857e-01, 1.153756e+00 };

  // expected result
  const double dist_exp = 1.658136e-01;
  const double closest_point_exp[] = { -1.582647e-01, -5.475835e-01, 1.015066e+00 };
  const int cell_id_exp = 1944;

  // threshold for floating point checking
  const double thresh = 1e-5;

  const double radius = 0.5;
  double closest_point[3];
  vtkIdType cell_id;
  int sub_id, inside;
  double dist;
  vtkNew<vtkGenericCell> cell;

  loc->FindClosestPointWithinRadius(
    test_point, radius, closest_point, cell, cell_id, sub_id, dist, inside);
  if (fabs(dist - dist_exp) / fabs(dist_exp) < thresh &&
    fabs(closest_point[0] - closest_point_exp[0]) / fabs(closest_point_exp[0]) < thresh &&
    fabs(closest_point[1] - closest_point_exp[1]) / fabs(closest_point_exp[1]) < thresh &&
    fabs(closest_point[2] - closest_point_exp[2]) / fabs(closest_point_exp[2]) < thresh &&
    cell_id == cell_id_exp)
    return true;
  return false;
}

int TestCellLocatorsEdgeCases(int argc, char* argv[])
{
  if (argc < 3)
  {
    cout << "Not enough arguments.";
    return EXIT_FAILURE;
  }

  //===========
  // Test Setup
  //===========
  vtkNew<vtkXMLPolyDataReader> reader;
  char* fname = argv[1];
  reader->SetFileName(fname);
  reader->Update();
  vtkDataSet* data = reader->GetOutput();

  bool allTestsPassed = true;
  vtkNew<vtkCellLocator> cl;
  allTestsPassed &= TestLocator(data, cl);
  vtkNew<vtkStaticCellLocator> scl;
  allTestsPassed &= TestLocator(data, scl);
  vtkNew<vtkCellTreeLocator> ctl;
  allTestsPassed &= TestLocator(data, ctl);
  // can't test vtkModifiedBSPTree because of the peculiarities
  // of how this test is executed
  // vtkNew<vtkModifiedBSPTree> mbsp;
  // allTestsPassed &= TestLocator(data, mbsp);
  allTestsPassed &= TestCellLocatorEvaluatePosition(argv[2]);

  //====================
  // Final Tests Outcome
  //====================
  return allTestsPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
