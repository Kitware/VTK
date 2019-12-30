/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinearCellExtrusion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkLinearCellExtrusionFilter

#include "vtkCellData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkLinearCellExtrusionFilter.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

//----------------------------------------------------------------------------
int TestLinearCellExtrusion(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkPolyData> polyData;
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> polys;

  polyData->SetPoints(points);
  polyData->SetPolys(polys);

  points->InsertNextPoint(0.1, 0.0, 0.0);
  points->InsertNextPoint(0.5, 0.0, 0.0);
  points->InsertNextPoint(0.6, 0.0, 0.2);
  points->InsertNextPoint(0.3, 0.0, 0.5);
  points->InsertNextPoint(0.1, 0.0, 0.2);
  points->InsertNextPoint(0.7, 0.0, 0.5);
  points->InsertNextPoint(0.6, 0.0, 0.7);
  points->InsertNextPoint(0.8, 0.0, 0.8);

  vtkIdType pentagon[] = { 0, 1, 2, 3, 4 };
  polys->InsertNextCell(5, pentagon);

  vtkIdType quad[] = { 3, 2, 5, 6 };
  polys->InsertNextCell(4, quad);

  vtkIdType triangle[] = { 5, 6, 7 };
  polys->InsertNextCell(3, triangle);

  vtkNew<vtkDoubleArray> array;
  array->SetNumberOfTuples(3);
  array->SetName("Values");

  array->SetTypedComponent(0, 0, 0.1);
  array->SetTypedComponent(1, 0, -0.2);
  array->SetTypedComponent(2, 0, 0.3);

  polyData->GetCellData()->SetScalars(array);

  vtkNew<vtkLinearCellExtrusionFilter> extrusion;
  extrusion->SetInputData(polyData);
  extrusion->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Values");
  extrusion->SetScaleFactor(1.0);

  vtkNew<vtkLinearCellExtrusionFilter> extrusionUser;
  extrusionUser->SetInputData(polyData);
  extrusionUser->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Values");
  extrusionUser->SetScaleFactor(0.8);
  extrusionUser->UseUserVectorOn();
  extrusionUser->SetUserVector(0.707107, 0.707107, 0.0);

  vtkNew<vtkDataSetSurfaceFilter> surf2;
  surf2->SetInputConnection(extrusion->GetOutputPort());

  vtkNew<vtkDataSetSurfaceFilter> surf3;
  surf3->SetInputConnection(extrusionUser->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surf2->GetOutputPort());
  mapper->SelectColorArray("Values");
  mapper->SetScalarRange(0.0, 1.0);
  mapper->SetColorModeToMapScalars();

  vtkNew<vtkPolyDataMapper> mapperUser;
  mapperUser->SetInputConnection(surf3->GetOutputPort());
  mapperUser->SelectColorArray("Values");
  mapperUser->SetScalarRange(0.0, 1.0);
  mapperUser->SetColorModeToMapScalars();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkActor> actorUser;
  actorUser->SetPosition(0.0, 0.5, 0.0);
  actorUser->SetMapper(mapperUser);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);
  ren->AddActor(actorUser);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  iren->Start();

  return 0;
}
