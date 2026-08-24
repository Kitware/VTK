// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendPolyData.h"
#include "vtkCellTypeSource.h"
#include "vtkDataObject.h"
#include "vtkDijkstraGraphGeodesicPath.h"
#include "vtkNew.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkXMLStructuredGridReader.h"

#include <cstdlib>
#include <iostream>

bool TestPolyData()
{
  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetCenter(10, 10, 10);
  sphere1->SetRadius(5.0);

  vtkNew<vtkAppendPolyData> appendFilter;
  appendFilter->AddInputConnection(sphere1->GetOutputPort());
  appendFilter->Update();

  vtkPolyData* polyData = appendFilter->GetOutput();

  vtkNew<vtkDijkstraGraphGeodesicPath> pathFilter;
  pathFilter->SetInputData(polyData);
  pathFilter->SetStartVertex(0);
  pathFilter->SetEndVertex(polyData->GetNumberOfPoints() - 1);
  pathFilter->Update();

  // Valid path from the first to last point on a single sphere
  vtkPolyData* path = pathFilter->GetOutput();
  if (!path || !path->GetPoints())
  {
    std::cerr << "Invalid output!" << std::endl;
    return false;
  }
  if (path->GetPoints()->GetNumberOfPoints() < 1)
  {
    std::cerr << "Could not find valid a path!" << std::endl;
    return false;
  }
  return true;
}

bool TestDisjointPolyData()
{
  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetCenter(10, 10, 10);
  sphere1->SetRadius(5.0);

  vtkNew<vtkAppendPolyData> appendFilter;
  appendFilter->AddInputConnection(sphere1->GetOutputPort());
  appendFilter->Update();

  vtkPolyData* polyData = appendFilter->GetOutput();

  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetCenter(-10, -10, -10);
  sphere2->SetRadius(2.0);
  appendFilter->AddInputConnection(sphere2->GetOutputPort());
  appendFilter->Update();

  vtkNew<vtkDijkstraGraphGeodesicPath> pathFilter;
  pathFilter->SetInputData(polyData);
  pathFilter->SetStartVertex(0);
  pathFilter->SetEndVertex(polyData->GetNumberOfPoints() - 1);
  pathFilter->Update();

  pathFilter->SetEndVertex(polyData->GetNumberOfPoints() - 1);
  pathFilter->Update();

  vtkPolyData* path = pathFilter->GetOutput();
  // No path should exist between the two separate spheres
  if (!path || !path->GetPoints())
  {
    std::cerr << "Invalid output!" << std::endl;
    return false;
  }
  if (path->GetPoints()->GetNumberOfPoints() > 0)
  {
    std::cerr << "Invalid path was expected, however a valid path was found!" << std::endl;
    return false;
  }
  return true;
}

bool TestUnstructuredGridWithCells()
{
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_TETRA);
  source->SetBlocksDimensions(10, 10, 10);

  vtkNew<vtkDijkstraGraphGeodesicPath> pathFilter;
  pathFilter->SetGraphType(vtkDataObject::AttributeTypes::CELL);
  pathFilter->SetInputConnection(source->GetOutputPort());
  pathFilter->SetEndVertex(11999);
  pathFilter->Update();

  vtkPolyData* path = pathFilter->GetOutput();
  if (!path || path->GetNumberOfPoints() != 57)
  {
    std::cerr << "Invalid path. 57 points were expected, got " << path->GetNumberOfPoints() << "."
              << std::endl;
    return false;
  }
  return true;
}

bool TestUnstructuredGridWithPoints()
{
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_TETRA);
  source->SetBlocksDimensions(10, 10, 10);

  vtkNew<vtkDijkstraGraphGeodesicPath> pathFilter;
  pathFilter->SetInputConnection(source->GetOutputPort());
  pathFilter->SetGraphType(vtkDataObject::AttributeTypes::POINT);
  pathFilter->SetEndVertex(2330);
  pathFilter->Update();

  vtkPolyData* path = pathFilter->GetOutput();
  if (!path || path->GetNumberOfPoints() != 20)
  {
    std::cerr << "Invalid path. 20 points were expected, got " << path->GetNumberOfPoints() << "."
              << std::endl;
    return false;
  }
  return true;
}

bool TestWeightedPath()
{
  vtkNew<vtkCellTypeSource> source;
  source->SetCellType(VTK_TETRA);
  source->SetBlocksDimensions(10, 10, 10);

  vtkNew<vtkDijkstraGraphGeodesicPath> pathFilter;
  pathFilter->SetInputConnection(source->GetOutputPort());
  pathFilter->SetGraphType(vtkDataObject::AttributeTypes::POINT);
  pathFilter->SetProcessedFieldArrayName("DistanceToCenter");
  pathFilter->SetUseScalarWeights(true);
  pathFilter->SetEndVertex(2330);
  pathFilter->Update();

  vtkPolyData* path = pathFilter->GetOutput();
  if (!path || path->GetNumberOfPoints() != 25)
  {
    std::cerr << "Invalid path. 25 points were expected, got " << path->GetNumberOfPoints() << "."
              << std::endl;
    return false;
  }
  return true;
}

int TestDijkstraGraphGeodesicPath(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool test = TestPolyData();
  test &= TestDisjointPolyData();
  test &= TestUnstructuredGridWithCells();
  test &= TestUnstructuredGridWithPoints();
  test &= TestWeightedPath();

  return test ? EXIT_SUCCESS : EXIT_FAILURE;
}
