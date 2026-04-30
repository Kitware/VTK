// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCubeSource.h"
#include "vtkCylinderSource.h"
#include "vtkIntersectionPolyDataFilter.h"
#include "vtkLinearSubdivisionFilter.h"
#include "vtkLoopBooleanPolyDataFilter.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkTriangleFilter.h"

static vtkSmartPointer<vtkActor> GetCubeBooleanOperationActor(double x, int operation)
{
  vtkNew<vtkCubeSource> cube1;
  cube1->SetCenter(x, 4.0, 0.0);
  cube1->SetXLength(1.0);
  cube1->SetYLength(1.0);
  cube1->SetZLength(1.0);
  cube1->Update();

  vtkNew<vtkTriangleFilter> triangulator1;
  triangulator1->SetInputData(cube1->GetOutput());
  triangulator1->Update();

  vtkNew<vtkLinearSubdivisionFilter> subdivider1;
  subdivider1->SetInputData(triangulator1->GetOutput());
  subdivider1->Update();

  vtkNew<vtkCubeSource> cube2;
  cube2->SetCenter(x + 0.3, 4.3, 0.3);
  cube2->SetXLength(1.0);
  cube2->SetYLength(1.0);
  cube2->SetZLength(1.0);
  cube2->Update();

  vtkNew<vtkTriangleFilter> triangulator2;
  triangulator2->SetInputData(cube2->GetOutput());
  triangulator2->Update();

  vtkNew<vtkLinearSubdivisionFilter> subdivider2;
  subdivider2->SetInputData(triangulator2->GetOutput());
  subdivider2->Update();

  vtkNew<vtkLoopBooleanPolyDataFilter> boolFilter;
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, subdivider1->GetOutputPort());
  boolFilter->SetInputConnection(1, subdivider2->GetOutputPort());
  boolFilter->Update();

  vtkPolyData* output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  return actor;
}

static vtkSmartPointer<vtkActor> GetSphereBooleanOperationActor(double x, int operation)
{
  double centerSeparation = 0.15;
  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetCenter(-centerSeparation + x, 0.0, 0.0);

  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetCenter(centerSeparation + x, 0.0, 0.0);

  vtkNew<vtkLoopBooleanPolyDataFilter> boolFilter;
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, sphere1->GetOutputPort());
  boolFilter->SetInputConnection(1, sphere2->GetOutputPort());
  boolFilter->Update();

  vtkPolyData* output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkNew<vtkActor> actor;
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
  vtkNew<vtkCylinderSource> cylinder1;
  cylinder1->SetCenter(0.0, 0.0, 0.0);
  cylinder1->SetHeight(2.0);
  cylinder1->SetRadius(0.5);
  cylinder1->SetResolution(15);
  cylinder1->Update();
  double radangle = vtkMath::AngleBetweenVectors(axis, vec);
  double degangle = vtkMath::DegreesFromRadians(radangle);
  vtkNew<vtkTransform> rotator1;
  rotator1->RotateWXYZ(degangle, rotateaxis);

  vtkNew<vtkTransformFilter> polyDataRotator1;
  polyDataRotator1->SetInputData(cylinder1->GetOutput());
  polyDataRotator1->SetTransform(rotator1);
  polyDataRotator1->Update();

  vtkNew<vtkTransform> mover1;
  mover1->Translate(x, -4.0, 0.0);

  vtkNew<vtkTransformFilter> polyDataMover1;
  polyDataMover1->SetInputData(polyDataRotator1->GetOutput());
  polyDataMover1->SetTransform(mover1);
  polyDataMover1->Update();

  vtkNew<vtkTriangleFilter> triangulator1;
  triangulator1->SetInputData(polyDataMover1->GetOutput());
  triangulator1->Update();

  axis[0] = 1.0;
  axis[1] = 0.0;
  axis[2] = 0.0;
  vtkMath::Cross(axis, vec, rotateaxis);
  vtkNew<vtkCylinderSource> cylinder2;
  cylinder2->SetCenter(0.0, 0.0, 0.0);
  cylinder2->SetHeight(2.0);
  cylinder2->SetRadius(0.5);
  cylinder2->SetResolution(15);
  cylinder2->Update();
  radangle = vtkMath::AngleBetweenVectors(axis, vec);
  degangle = vtkMath::DegreesFromRadians(radangle);
  vtkNew<vtkTransform> rotator2;
  rotator2->RotateWXYZ(degangle, rotateaxis);

  vtkNew<vtkTransformFilter> polyDataRotator2;
  polyDataRotator2->SetInputData(cylinder2->GetOutput());
  polyDataRotator2->SetTransform(rotator2);
  polyDataRotator2->Update();

  vtkNew<vtkTransform> mover2;
  mover2->Translate(x, -4.0, 0.0);

  vtkNew<vtkTransformFilter> polyDataMover2;
  polyDataMover2->SetInputData(polyDataRotator2->GetOutput());
  polyDataMover2->SetTransform(mover2);
  polyDataMover2->Update();

  vtkNew<vtkTriangleFilter> triangulator2;
  triangulator2->SetInputData(polyDataMover2->GetOutput());
  triangulator2->Update();

  vtkNew<vtkLoopBooleanPolyDataFilter> boolFilter;
  boolFilter->SetOperation(operation);
  boolFilter->SetInputConnection(0, triangulator1->GetOutputPort());
  boolFilter->SetInputConnection(1, triangulator2->GetOutputPort());
  boolFilter->Update();

  vtkPolyData* output = boolFilter->GetOutput();
  output->GetCellData()->SetActiveScalars("FreeEdge");
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(output);
  mapper->SetScalarRange(0, 1);
  mapper->SetScalarModeToUseCellData();
  mapper->ScalarVisibilityOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  return actor;
}

int TestLoopBooleanPolyDataFilter(int, char*[])
{
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> renWinInteractor;
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
