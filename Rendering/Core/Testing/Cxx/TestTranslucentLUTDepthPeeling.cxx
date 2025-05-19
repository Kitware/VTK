// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers rendering of an actor with a translucent LUT and depth
// peeling. The mapper uses color interpolation (poor quality).
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkImageSinusoidSource.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"

int TestTranslucentLUTDepthPeeling(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  if (renWin->IsA("vtkWebAssemblyOpenGLRenderWindow"))
  {
    // WebAssembly OpenGL requires additional steps for dual depth peeling. See
    // TestFramebufferPass.cxx for details.
    std::cout << "Skipping test with dual-depth peeling for WebAssembly OpenGL\n";
    return VTK_SKIP_RETURN_CODE;
  }
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(200);
  renderer->SetOcclusionRatio(0.1);

  vtkNew<vtkImageSinusoidSource> imageSource;
  imageSource->SetWholeExtent(0, 9, 0, 9, 0, 9);
  imageSource->SetPeriod(5);
  imageSource->Update();

  auto* image = imageSource->GetOutput();
  double range[2];
  image->GetScalarRange(range);

  vtkNew<vtkDataSetSurfaceFilter> surface;

  surface->SetInputConnection(imageSource->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());

  vtkNew<vtkLookupTable> lut;
  lut->SetTableRange(range);
  lut->SetAlphaRange(0.5, 0.5);
  lut->SetHueRange(0.2, 0.7);
  lut->SetNumberOfTableValues(256);
  lut->Build();

  mapper->SetScalarVisibility(1);
  mapper->SetLookupTable(lut);

  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  renderer->SetBackground(0.1, 0.3, 0.0);
  renWin->SetSize(400, 400);

  renWin->Render();
  if (renderer->GetLastRenderingUsedDepthPeeling())
  {
    cout << "depth peeling was used" << endl;
  }
  else
  {
    cout << "depth peeling was not used (alpha blending instead)" << endl;
  }
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
