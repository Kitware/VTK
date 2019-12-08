/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLoopOperationPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCubeSource.h"
#include "vtkCylinderSource.h"
#include "vtkIntersectionPolyDataFilter.h"
#include "vtkLinearSubdivisionFilter.h"
#include "vtkLoopBooleanPolyDataFilter.h"
#include "vtkMath.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTriangleFilter.h"

static vtkSmartPointer<vtkActor> GetCubeBooleanOperationActor(double x, int operation)
{
  vtkSmartPointer<vtkCubeSource> cube1 = vtkSmartPointer<vtkCubeSource>::New();
  cube1->SetCenter(x, 4.0, 0.0);
  cube1->SetXLength(1.0);
  cube1->SetYLength(1.0);
  cube1->SetZLength(1.0);
  cube1->Update();

  vtkSmartPointer<vtkTriangleFilter> triangulator1 = vtkSmartPointer<vtkTriangleFilter>::New();
  triangulator1->SetInputData(cube1->GetOutput());
  triangulator1->Update();

  vtkSmartPointer<vtkLinearSubdivisionFilter> subdivider1 =
    vtkSmartPointer<vtkLinearSubdivisionFilter>::New();
  subdivider1->SetInputData(triangulator1->GetOutput());
  subdivider1->Update();

  vtkSmartPointer<vtkCubeSource> cube2 = vtkSmartPointer<vtkCubeSource>::New();
  cube2->SetCenter(x + 0.3, 4.3, 0.3);
  cube2->SetXLength(1.0);
  cube2->SetYLength(1.0);
  cube2->SetZLength(1.0);
  cube2->Update();

  vtkSmartPointer<vtkTriangleFilter> triangulator2 = vtkSmartPointer<vtkTriangleFilter>::New();
  triangulator2->SetInputData(cube2->GetOutput());
  triangulator2->Update();

  vtkSmartPointer<vtkLinearSubdivisionFilter> subdivider2 =
    vtkSmartPointer<vtkLinearSubdivisionFilter>::New();
  subdivider2->SetInputData(triangulator2->GetOutput());
  subdivider2->Update();

  vtkSmartPointer<vtkLoopBooleanPolyDataFilter> boolFilter =
    vtkSmartPointer<vtkLoopBooleanPolyDataFilter>::New();
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, subdivider1->GetOutputPort());
  boolFilter->SetInputConnection(1, subdivider2->GetOutputPort());
  boolFilter->Update();

  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
  output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

static vtkSmartPointer<vtkActor> GetSphereBooleanOperationActor(double x, int operation)
{
  double centerSeparation = 0.15;
  vtkSmartPointer<vtkSphereSource> sphere1 = vtkSmartPointer<vtkSphereSource>::New();
  sphere1->SetCenter(-centerSeparation + x, 0.0, 0.0);

  vtkSmartPointer<vtkSphereSource> sphere2 = vtkSmartPointer<vtkSphereSource>::New();
  sphere2->SetCenter(centerSeparation + x, 0.0, 0.0);

  vtkSmartPointer<vtkLoopBooleanPolyDataFilter> boolFilter =
    vtkSmartPointer<vtkLoopBooleanPolyDataFilter>::New();
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, sphere1->GetOutputPort());
  boolFilter->SetInputConnection(1, sphere2->GetOutputPort());
  boolFilter->Update();

  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
  output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

static vtkSmartPointer<vtkActor> GetCylinderBooleanOperationActor(double x, int operation)
{
  double axis[3];
  axis[0] = 0.0;
  axis[1] = 1.0;
  axis[2] = 0.0;
  double vec[3];
  vec[0] = 0.0;
  vec[1] = 1.0;
  vec[2] = 0.0;
  double rotateaxis[3];
  vtkMath::Cross(axis, vec, rotateaxis);
  vtkSmartPointer<vtkCylinderSource> cylinder1 = vtkSmartPointer<vtkCylinderSource>::New();
  cylinder1->SetCenter(0.0, 0.0, 0.0);
  cylinder1->SetHeight(2.0);
  cylinder1->SetRadius(0.5);
  cylinder1->SetResolution(15);
  cylinder1->Update();
  double radangle = vtkMath::AngleBetweenVectors(axis, vec);
  double degangle = vtkMath::DegreesFromRadians(radangle);
  vtkSmartPointer<vtkTransform> rotator1 = vtkSmartPointer<vtkTransform>::New();
  rotator1->RotateWXYZ(degangle, rotateaxis);

  vtkSmartPointer<vtkTransformPolyDataFilter> polyDataRotator1 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  polyDataRotator1->SetInputData(cylinder1->GetOutput());
  polyDataRotator1->SetTransform(rotator1);
  polyDataRotator1->Update();

  vtkSmartPointer<vtkTransform> mover1 = vtkSmartPointer<vtkTransform>::New();
  mover1->Translate(x, -4.0, 0.0);

  vtkSmartPointer<vtkTransformPolyDataFilter> polyDataMover1 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  polyDataMover1->SetInputData(polyDataRotator1->GetOutput());
  polyDataMover1->SetTransform(mover1);
  polyDataMover1->Update();

  vtkSmartPointer<vtkTriangleFilter> triangulator1 = vtkSmartPointer<vtkTriangleFilter>::New();
  triangulator1->SetInputData(polyDataMover1->GetOutput());
  triangulator1->Update();

  axis[0] = 1.0;
  axis[1] = 0.0;
  axis[2] = 0.0;
  vtkMath::Cross(axis, vec, rotateaxis);
  vtkSmartPointer<vtkCylinderSource> cylinder2 = vtkSmartPointer<vtkCylinderSource>::New();
  cylinder2->SetCenter(0.0, 0.0, 0.0);
  cylinder2->SetHeight(2.0);
  cylinder2->SetRadius(0.5);
  cylinder2->SetResolution(15);
  cylinder2->Update();
  radangle = vtkMath::AngleBetweenVectors(axis, vec);
  degangle = vtkMath::DegreesFromRadians(radangle);
  vtkSmartPointer<vtkTransform> rotator2 = vtkSmartPointer<vtkTransform>::New();
  rotator2->RotateWXYZ(degangle, rotateaxis);

  vtkSmartPointer<vtkTransformPolyDataFilter> polyDataRotator2 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  polyDataRotator2->SetInputData(cylinder2->GetOutput());
  polyDataRotator2->SetTransform(rotator2);
  polyDataRotator2->Update();

  vtkSmartPointer<vtkTransform> mover2 = vtkSmartPointer<vtkTransform>::New();
  mover2->Translate(x, -4.0, 0.0);

  vtkSmartPointer<vtkTransformPolyDataFilter> polyDataMover2 =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  polyDataMover2->SetInputData(polyDataRotator2->GetOutput());
  polyDataMover2->SetTransform(mover2);
  polyDataMover2->Update();

  vtkSmartPointer<vtkTriangleFilter> triangulator2 = vtkSmartPointer<vtkTriangleFilter>::New();
  triangulator2->SetInputData(polyDataMover2->GetOutput());
  triangulator2->Update();

  vtkSmartPointer<vtkLoopBooleanPolyDataFilter> boolFilter =
    vtkSmartPointer<vtkLoopBooleanPolyDataFilter>::New();
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, triangulator1->GetOutputPort());
  boolFilter->SetInputConnection(1, triangulator2->GetOutputPort());
  boolFilter->Update();

  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
  output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  return actor;
}

int TestLoopBooleanPolyDataFilter(int, char*[])
{
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> renWinInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renWinInteractor->SetRenderWindow(renWin);

  // Sphere
  vtkSmartPointer<vtkActor> unionActor =
    GetSphereBooleanOperationActor(-2.0, vtkLoopBooleanPolyDataFilter::VTK_UNION);
  renderer->AddActor(unionActor);

  vtkSmartPointer<vtkActor> intersectionActor =
    GetSphereBooleanOperationActor(0.0, vtkLoopBooleanPolyDataFilter::VTK_INTERSECTION);
  renderer->AddActor(intersectionActor);

  vtkSmartPointer<vtkActor> differenceActor =
    GetSphereBooleanOperationActor(2.0, vtkLoopBooleanPolyDataFilter::VTK_DIFFERENCE);
  renderer->AddActor(differenceActor);

  // Cube
  vtkSmartPointer<vtkActor> unionCubeActor =
    GetCubeBooleanOperationActor(-2.0, vtkLoopBooleanPolyDataFilter::VTK_UNION);
  renderer->AddActor(unionCubeActor);

  vtkSmartPointer<vtkActor> intersectionCubeActor =
    GetCubeBooleanOperationActor(0.0, vtkLoopBooleanPolyDataFilter::VTK_INTERSECTION);
  renderer->AddActor(intersectionCubeActor);

  vtkSmartPointer<vtkActor> differenceCubeActor =
    GetCubeBooleanOperationActor(2.0, vtkLoopBooleanPolyDataFilter::VTK_DIFFERENCE);
  renderer->AddActor(differenceCubeActor);

  // Cylinder
  vtkSmartPointer<vtkActor> unionCylinderActor =
    GetCylinderBooleanOperationActor(-2.0, vtkLoopBooleanPolyDataFilter::VTK_UNION);
  renderer->AddActor(unionCylinderActor);

  vtkSmartPointer<vtkActor> intersectionCylinderActor =
    GetCylinderBooleanOperationActor(0.0, vtkLoopBooleanPolyDataFilter::VTK_INTERSECTION);
  renderer->AddActor(intersectionCylinderActor);

  vtkSmartPointer<vtkActor> differenceCylinderActor =
    GetCylinderBooleanOperationActor(2.0, vtkLoopBooleanPolyDataFilter::VTK_DIFFERENCE);
  renderer->AddActor(differenceCylinderActor);

  renderer->SetBackground(0.4392, 0.5020, 0.5647);
  renWin->Render();
  renWinInteractor->Start();

  return EXIT_SUCCESS;
}
