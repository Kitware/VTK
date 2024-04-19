// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"

int TestTexture32Bits(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPlaneSource> plane;

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->AddRenderer(renderer);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  vtkNew<vtkImageData> image;
  image->SetExtent(0, 256, 0, 256, 0, 0);

  vtkNew<vtkFloatArray> pixels;
  pixels->SetNumberOfComponents(3);
  pixels->SetNumberOfTuples(65536);
  float* p = pixels->GetPointer(0);

  for (int i = 0; i < 65536; i++)
  {
    float v = i / 65536.f;
    *p++ = v;
    *p++ = 1.f - v;
    *p++ = 0.5f + v;
  }

  image->GetPointData()->SetScalars(pixels);

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->SetInputData(image);

  actor->SetTexture(texture);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.3);
  renderer->ResetCameraClippingRange();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
