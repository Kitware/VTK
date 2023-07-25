// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkIntersectionPolyDataFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"
#include "vtkUnsignedCharArray.h"

#include <cassert>

bool TestSpherePlaneIntersection(bool joinSegments, bool addGhostArray)
{
  // Sphere
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(2.0f);
  sphereSource->SetPhiResolution(20);
  sphereSource->SetThetaResolution(20);
  sphereSource->Update();

  // Plane
  vtkSmartPointer<vtkPoints> PlanePoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> PlaneCells = vtkSmartPointer<vtkCellArray>::New();
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

  // Inserting a ghost
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
  intersectionPolyDataFilter->SetInputConnection(0, sphereSource->GetOutputPort());
  intersectionPolyDataFilter->SetInputData(1, Plane);
  intersectionPolyDataFilter->Update();

  vtkNew<vtkPolyData> sphere;
  sphere->ShallowCopy(intersectionPolyDataFilter->GetOutputDataObject(0));

  if (addGhostArray)
  {
    vtkNew<vtkUnsignedCharArray> ghosts;
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghosts->SetNumberOfValues(sphere->GetNumberOfCells());
    for (vtkIdType cellId = 0; cellId < sphere->GetNumberOfCells(); ++cellId)
    {
      ghosts->SetValue(cellId, cellId % 5 ? 0 : vtkDataSetAttributes::DUPLICATECELL);
    }
    sphere->GetCellData()->AddArray(ghosts);
  }

  // Get the polylines from the segments
  vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputData(sphere);

  if (joinSegments)
  {
    stripper->SetJoinContiguousSegments(true);
  }

  stripper->Update();
  vtkSmartPointer<vtkPolyDataMapper> intersectionMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  intersectionMapper->SetInputConnection(stripper->GetOutputPort());

  if (addGhostArray)
  {
    if (intersectionMapper->GetInput()->GetNumberOfLines() != 8)
    {
      return false;
    }
    return true;
  }
  if (joinSegments)
  {
    if (intersectionMapper->GetInput()->GetNumberOfLines() != 2)
    {
      return false;
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

int TestStripper(int, char*[])
{
  if (!TestSpherePlaneIntersection(false, false))
  {
    return EXIT_FAILURE;
  }

  if (!TestSpherePlaneIntersection(true, false))
  {
    return EXIT_FAILURE;
  }

  if (!TestSpherePlaneIntersection(false, true))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
