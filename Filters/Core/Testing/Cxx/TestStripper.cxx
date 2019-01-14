/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStripper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkCellArray.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkIntersectionPolyDataFilter.h"
#include "vtkStripper.h"
#include <cassert>

bool
TestSpherePlaneIntersection(bool joinSegments)
{
  // Sphere
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<
      vtkSphereSource>::New();
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(2.0f);
  sphereSource->SetPhiResolution(20);
  sphereSource->SetThetaResolution(20);
  sphereSource->Update();

  // Plane
  vtkSmartPointer<vtkPoints> PlanePoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> PlaneCells =
      vtkSmartPointer<vtkCellArray>::New();
  // 4 points
  PlanePoints->InsertNextPoint(-3, -1, 0);
  PlanePoints->InsertNextPoint(3, -1, 0);
  PlanePoints->InsertNextPoint(-3, 1, 0);
  PlanePoints->InsertNextPoint(3, 1, 0);
  // 2 triangles
  PlaneCells->InsertNextCell(3);
  PlaneCells->InsertCellPoint(0);
  PlaneCells->InsertCellPoint(1);
  PlaneCells->InsertCellPoint(2);

  PlaneCells->InsertNextCell(3);
  PlaneCells->InsertCellPoint(1);
  PlaneCells->InsertCellPoint(3);
  PlaneCells->InsertCellPoint(2);

  // Create the polydata from points and faces
  vtkSmartPointer<vtkPolyData> Plane = vtkSmartPointer<vtkPolyData>::New();
  Plane->SetPoints(PlanePoints);
  Plane->SetPolys(PlaneCells);

  // Intersect plane with sphere, get lines
  vtkSmartPointer<vtkIntersectionPolyDataFilter> intersectionPolyDataFilter =
      vtkSmartPointer<vtkIntersectionPolyDataFilter>::New();
  intersectionPolyDataFilter->SplitFirstOutputOff();
  intersectionPolyDataFilter->SplitSecondOutputOff();
  intersectionPolyDataFilter->SetInputConnection(0,
      sphereSource->GetOutputPort());
  intersectionPolyDataFilter->SetInputData(1, Plane);
  intersectionPolyDataFilter->Update();

  // Get the polylines from the segments
  vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(intersectionPolyDataFilter->GetOutputPort());

  if (joinSegments)
  {
    stripper->SetJoinContiguousSegments(true);
  }

  stripper->Update();
  vtkSmartPointer<vtkPolyDataMapper> intersectionMapper = vtkSmartPointer<
      vtkPolyDataMapper>::New();
  intersectionMapper->SetInputConnection(stripper->GetOutputPort());

  if (joinSegments)
  {
    if (intersectionMapper->GetInput()->GetNumberOfLines() != 2)
    {
      return true;
    }
  }
  else
  {
    if (intersectionMapper->GetInput()->GetNumberOfLines() != 6)
    {
      return false;
    }
  }

  return true;
}

int
TestStripper(int, char *[])
{
  if (!TestSpherePlaneIntersection(false))
  {
    return EXIT_FAILURE;
  }

  if (!TestSpherePlaneIntersection(true))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
