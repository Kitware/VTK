/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStaticCellLocatorEdgeCases.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCell.h>
#include <vtkGenericCell.h>
#include <vtkIdList.h>
#include <vtkLineSource.h>
#include <vtkNew.h>
#include <vtkStaticCellLocator.h>
#include <vtkXMLPolyDataReader.h>

int TestCell(vtkDataSet* ds, int cellId, double x1[3], double x2[3], double tol)
{
  double t = 0.0;
  double x[3] = { 0.0, 0.0, 0.0 };
  double pcoords[3] = { 0.0, 0.0, 0.0 };
  int subId = 0;
  vtkCell* cell = ds->GetCell(cellId);

  return cell->IntersectWithLine(x1, x2, tol, t, x, pcoords, subId);
}

int TestStaticCellLocatorEdgeCases(int argc, char* argv[])
{
  if (argc < 2)
  {
    cout << "Not enough arguments.";
    return EXIT_FAILURE;
  }

  //===========
  // Test Setup
  //===========
  int numFailed = 0;
  double tol = 1E-15; // tolerance only used in TestCell

  vtkNew<vtkXMLPolyDataReader> reader;
  char* fname = argv[1];
  reader->SetFileName(fname);
  reader->Update();
  vtkDataSet* data = reader->GetOutput();

  vtkNew<vtkStaticCellLocator> locator;
  locator->SetDataSet(data);
  locator->CacheCellBoundsOn();
  locator->AutomaticOn();
  locator->BuildLocator();

  //========================
  // Test FindCellsAlongLine
  //========================
  double x1[3] = { 0.437783024586950, 0.0263950841209563, 0.373722994626027 };
  double x2[3] = { 0.442140196830658, 0.0256207765183134, 0.374080391702881 };
  vtkNew<vtkIdList> cellList;
  locator->FindCellsAlongLine(x1, x2, tol, cellList);

  int found = 0;
  for (vtkIdType i = 0; i < cellList->GetNumberOfIds(); ++i)
  {
    found |= TestCell(data, cellList->GetId(i), x1, x2, tol);
  }

  if (found == 0)
  {
    std::cerr << "FindCellsAlongLine: No valid cell intersections found!" << std::endl;
    ++numFailed;
  }

  //==================================
  // Test FindClosestPointWithinRadius
  //==================================
  vtkNew<vtkGenericCell> cell;
  vtkIdType cellId;
  double dist2 = 0.0;
  int subId = 0;
  double closestPoint[3] = { 0.0, 0.0, 0.0 };
  double radius = 0.00058385;
  double x3[3] = { 0.44179561594301064, -0.017842554788570667, 0.28626203407677214 };
  found =
    locator->FindClosestPointWithinRadius(x3, radius, closestPoint, cell, cellId, subId, dist2);

  if (found == 0)
  {
    std::cerr << "FindClosestPointWithinRadius: No valid cells found within given radius!"
              << std::endl;
    ++numFailed;
  }

  //====================
  // Final Tests Outcome
  //====================
  return (numFailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
