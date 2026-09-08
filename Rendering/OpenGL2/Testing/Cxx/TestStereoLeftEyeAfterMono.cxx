// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Regression test for a bug where switching a render window directly from
// mono rendering into one-pass left-eye stereo rendering (VTK_STEREO_LEFT)
// produced an image identical to the mono render: the left eye's parallax
// shift was never applied because vtkCamera's LeftEye value is already 1 by
// default, so vtkRenderWindow::DoStereoRender()'s call to SetLeftEye(1) is a
// no-op that does not bump the camera's MTime, and vtkOpenGLCamera::Render()
// set the Stereo flag directly without calling Modified() either. As a
// result vtkOpenGLCamera::GetKeyMatrices() never noticed the camera needed
// its cached view/projection matrices recomputed, and the stale mono-frame
// matrices were reused for the "left eye" render. Rendering the right eye
// (VTK_STEREO_RIGHT) did not have this problem because it always changes
// LeftEye from 1 to 0, which does bump the camera's MTime.

// Saves the render currently in renWin's buffers to a PNG, so the two
// renders can be inspected visually if this test ever needs debugging.
// #define SAVE_IMAGE

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"

#ifdef SAVE_IMAGE
#include "vtkPNGWriter.h"
#include "vtkWindowToImageFilter.h"
#endif

#include <cmath>
#include <iostream>

namespace
{
vtkSmartPointer<vtkUnsignedCharArray> Capture(vtkRenderWindow* renWin)
{
  vtkNew<vtkUnsignedCharArray> pixels;
  int* size = renWin->GetSize();
  renWin->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 0, pixels.GetPointer(), 0);
  return pixels;
}

bool ImagesDiffer(vtkUnsignedCharArray* a, vtkUnsignedCharArray* b)
{
  if (a->GetNumberOfValues() != b->GetNumberOfValues())
  {
    return true;
  }
  for (vtkIdType i = 0; i < a->GetNumberOfValues(); ++i)
  {
    if (std::abs(a->GetValue(i) - b->GetValue(i)) > 2)
    {
      return true;
    }
  }
  return false;
}
#ifdef SAVE_IMAGE
void SaveImage(vtkRenderWindow* renWin, const char* filename)
{
  vtkNew<vtkWindowToImageFilter> w2i;
  w2i->SetInput(renWin);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->Update();

  vtkNew<vtkPNGWriter> writer;
  writer->SetFileName(filename);
  writer->SetInputConnection(w2i->GetOutputPort());
  writer->Write();
}
#endif
}

int TestStereoLeftEyeAfterMono(int, char*[])
{
  vtkNew<vtkCubeSource> cube;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cube->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetOffScreenRendering(1);
  renWin->AddRenderer(ren);
  renWin->SetSize(200, 200);
  renWin->SetStereoCapableWindow(1);

  // Render once in mono, mimicking a view that has already been rendered
  // interactively before the user requests a stereo screenshot.
  renWin->Render();
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(30);
  ren->GetActiveCamera()->Elevation(30);
  ren->ResetCameraClippingRange();
  renWin->Render();

  vtkSmartPointer<vtkUnsignedCharArray> mono = Capture(renWin);
#ifdef SAVE_IMAGE
  SaveImage(renWin, "mono.png");
#endif

  // Switch straight to one-pass left-eye stereo and render once, exactly as
  // vtkSMSaveScreenshotProxy does when capturing the left eye of a "Both
  // Eyes" screenshot.
  renWin->SetStereoRender(1);
  renWin->SetStereoType(VTK_STEREO_LEFT);
  renWin->Render();

  vtkSmartPointer<vtkUnsignedCharArray> left = Capture(renWin);
#ifdef SAVE_IMAGE
  SaveImage(renWin, "left.png");
#endif

#ifdef SAVE_IMAGE
  renWin->SetStereoType(VTK_STEREO_RIGHT);
  renWin->Render();

  vtkSmartPointer<vtkUnsignedCharArray> right = Capture(renWin);
  SaveImage(renWin, "right.png");
#endif

  if (!ImagesDiffer(mono, left))
  {
    std::cerr << "ERROR: left-eye stereo render was identical to the mono render; "
                 "the eye separation shear was not applied."
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
