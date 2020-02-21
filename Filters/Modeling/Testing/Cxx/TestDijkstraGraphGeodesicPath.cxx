/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDijkstraGraphGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAppendPolyData.h"
#include "vtkDijkstraGraphGeodesicPath.h"
#include "vtkNew.h"
#include "vtkSphereSource.h"

int TestDijkstraGraphGeodesicPath(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
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
  vtkPolyData* path1 = pathFilter->GetOutput();
  if (!path1 || !path1->GetPoints())
  {
    std::cerr << "Invalid output!" << std::endl;
    return EXIT_FAILURE;
  }
  if (path1->GetPoints()->GetNumberOfPoints() < 1)
  {
    std::cerr << "Could not find valid a path!" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetCenter(-10, -10, -10);
  sphere2->SetRadius(2.0);
  appendFilter->AddInputConnection(sphere2->GetOutputPort());
  appendFilter->Update();

  polyData = appendFilter->GetOutput();
  pathFilter->SetEndVertex(polyData->GetNumberOfPoints() - 1);
  pathFilter->Update();

  // No path should exist between the two separate spheres
  vtkPolyData* path2 = pathFilter->GetOutput();
  if (!path2 || !path2->GetPoints())
  {
    std::cerr << "Invalid output!" << std::endl;
    return EXIT_FAILURE;
  }
  if (path2->GetPoints()->GetNumberOfPoints() > 0)
  {
    std::cerr << "Invalid path was expected, however a valid path was found!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
