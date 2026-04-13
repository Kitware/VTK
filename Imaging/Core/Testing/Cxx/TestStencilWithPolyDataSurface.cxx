// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkBoxMuellerRandomSequence.h"
#include "vtkCamera.h"
#include "vtkCutter.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageStencil.h"
#include "vtkInteractorStyleImage.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataToImageStencil.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkTriangleFilter.h"

#include <cmath>
#include <cstring>

int TestStencilWithPolyDataSurface(int, char*[])
{
  vtkNew<vtkImageData> image;
  double spacing[3] = { 0.9765625, 0.9765625, 3.0 };
  double origin[3] = { -124.51171875, -124.51171875, -105.0 };
  int extent[6] = { 0, 255, 0, 255, 0, 70 };
  image->SetSpacing(spacing);
  image->SetOrigin(origin);
  image->SetExtent(extent);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  unsigned char* vptr = static_cast<unsigned char*>(image->GetScalarPointer());
  memset(vptr, 255, image->GetNumberOfPoints());

  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetRadius(100);
  sphereSource->SetPhiResolution(21);
  sphereSource->SetThetaResolution(41);
  sphereSource->Update();

  vtkNew<vtkTriangleFilter> triangleFilter;
  triangleFilter->SetInputConnection(sphereSource->GetOutputPort());
  triangleFilter->Update();

  // add some noise to the point positions
  vtkNew<vtkBoxMuellerRandomSequence> randomSequence;
  vtkNew<vtkPolyData> polyData;
  polyData->DeepCopy(triangleFilter->GetOutput());
  vtkPoints* points = polyData->GetPoints();
  vtkNew<vtkPoints> newPoints;
  newPoints->SetNumberOfPoints(points->GetNumberOfPoints());
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double point[3];
    points->GetPoint(i, point);
    double r = exp(randomSequence->GetScaledValue(0.0, 0.1));
    randomSequence->Next();
    point[0] *= r;
    point[1] *= r;
    point[2] *= r;
    newPoints->SetPoint(i, point);
  }
  polyData->SetPoints(newPoints);

  // make sure triangle strips can be used as input
  vtkNew<vtkStripper> stripper;
  stripper->SetInputConnection(triangleFilter->GetOutputPort());

  vtkNew<vtkTransform> transform;
  transform->Scale(0.49, 0.5, 0.6);
  transform->Translate(9.111, -7.56, 1.0);
  transform->RotateWXYZ(30, 1.0, 0.5, 0.0);

  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetTransform(transform);
  transformFilter->SetInputConnection(stripper->GetOutputPort());

  // use append to make sure nested surfaces are handled
  vtkNew<vtkAppendPolyData> append;
  append->SetInputData(polyData);
  append->AddInputConnection(transformFilter->GetOutputPort());

  vtkNew<vtkPolyDataToImageStencil> stencilSource;
  stencilSource->SetOutputOrigin(origin);
  stencilSource->SetOutputSpacing(spacing);
  stencilSource->SetOutputWholeExtent(extent);
  stencilSource->SetInputConnection(append->GetOutputPort());

  vtkNew<vtkImageStencil> stencil;
  stencil->SetInputData(image);
  stencil->SetStencilConnection(stencilSource->GetOutputPort());
  stencil->Update();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(256 * 3, 256 * 2);

  vtkNew<vtkInteractorStyleImage> style;

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  for (int i = 0; i < 6; i++)
  {
    int zIdx = 3 + 11 * i;
    double z = zIdx * spacing[2] + origin[2];

    vtkNew<vtkPlane> plane;
    plane->SetNormal(0.0, 0.0, 1.0);
    plane->SetOrigin(0.0, 0.0, z);

    vtkNew<vtkCutter> cutter;
    cutter->SetInputConnection(append->GetOutputPort());
    cutter->SetCutFunction(plane);
    cutter->GenerateCutScalarsOff();

    vtkNew<vtkPolyDataMapper> polyMapper;
    polyMapper->SetInputConnection(cutter->GetOutputPort());
    polyMapper->ScalarVisibilityOff();

    vtkNew<vtkActor> polyActor;
    polyActor->SetMapper(polyMapper);
    polyActor->GetProperty()->SetDiffuse(0.0);
    polyActor->GetProperty()->SetAmbient(1.0);
    polyActor->GetProperty()->SetColor(0.1, 0.6, 0.1);
    polyActor->SetPosition(0.0, 0.0, 1.0); // zbuffer offset

    vtkNew<vtkImageSliceMapper> mapper;
    mapper->SetOrientation(2);
    mapper->SetSliceNumber(zIdx);
    mapper->SetInputConnection(stencil->GetOutputPort());

    vtkNew<vtkImageSlice> actor;
    actor->GetProperty()->SetColorWindow(255.0);
    actor->GetProperty()->SetColorLevel(127.5);
    actor->GetProperty()->SetInterpolationTypeToLinear();
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> renderer;
    int j = i % 3;
    int k = 1 - (i / 3);
    renderer->SetViewport(j / 3.0, k / 2.0, (j + 1) / 3.0, (k + 1) / 2.0);
    renderer->AddViewProp(actor);
    renderer->AddViewProp(polyActor);

    vtkCamera* camera = renderer->GetActiveCamera();
    camera->ParallelProjectionOn();
    camera->SetParallelScale(0.5 * spacing[1] * (extent[3] - extent[2]));
    camera->SetFocalPoint(0.0, 0.0, z);
    camera->SetPosition(0.0, 0.0, z + 10.0);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->SetClippingRange(5.0, 15.0);

    renWin->AddRenderer(renderer);
  }

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
