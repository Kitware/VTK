// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test of an RGBA texture on a vtkActor.
// .SECTION Description
// this program tests the rendering of an vtkActor with a translucent texture
// with depth peeling.

#include "vtkActor.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

int TestTextureRGBADepthPeeling(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  if (renWin->IsA("vtkWebAssemblyOpenGLRenderWindow"))
  {
    // WebAssembly OpenGL requires additional steps for dual depth peeling. See
    // TestFramebufferPass.cxx for details.
    std::cout << "Skipping test with dual-depth peeling for WebAssembly OpenGL\n";
    return VTK_SKIP_RETURN_CODE;
  }
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/textureRGBA.png");

  vtkNew<vtkPNGReader> PNGReader;
  PNGReader->SetFileName(fname);
  PNGReader->Update();

  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(PNGReader->GetOutputPort());
  texture->InterpolateOn();

  vtkNew<vtkPlaneSource> planeSource;
  planeSource->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(planeSource->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetTexture(texture);
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.7, 0.7);

  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  renderer->SetUseDepthPeeling(1);
  renderer->SetMaximumNumberOfPeels(200);
  renderer->SetOcclusionRatio(0.1);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);

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

  interactor->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  delete[] fname;

  return !retVal;
}
