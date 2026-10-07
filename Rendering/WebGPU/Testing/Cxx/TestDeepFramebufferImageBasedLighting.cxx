// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTexture.h"

int TestDeepFramebufferImageBasedLighting(int, char*[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(200, 200);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkImageData> image;
  image->SetDimensions(64, 32, 1);
  vtkNew<vtkFloatArray> scalars;
  scalars->SetNumberOfComponents(3);
  scalars->SetNumberOfTuples(64 * 32);
  for (vtkIdType i = 0; i < 64 * 32; ++i)
  {
    const float t = static_cast<float>(i % 64) / 63.0f;
    scalars->SetTuple3(i, t, 0.5f, 1.0f - t);
  }
  image->GetPointData()->SetScalars(scalars);
  vtkNew<vtkTexture> texture;
  texture->SetInputData(image);
  texture->SetColorModeToDirectScalars();
  texture->MipmapOn();
  texture->InterpolateOn();

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();
  actor->GetProperty()->SetOpacity(0.5);
  renderer->AddActor(actor);

  vtkNew<vtkActor> opaque;
  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetCenter(0.3, 0, -1);
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(sphere2->GetOutputPort());
  opaque->SetMapper(mapper2);
  renderer->AddActor(opaque);

  renWin->Render();
  renderer->SetEnvironmentTexture(texture);
  renderer->UseImageBasedLightingOn();
  renWin->Render();
  renWin->Render();
  renderer->UseImageBasedLightingOff();
  renWin->Render();
  renderer->UseImageBasedLightingOn();
  renWin->Render();

  return EXIT_SUCCESS;
}
