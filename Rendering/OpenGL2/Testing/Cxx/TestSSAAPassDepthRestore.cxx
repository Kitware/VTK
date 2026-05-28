// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the SSAA pass and ensures that it restores the depth
// buffer correctly for subsequent passes that need it.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkRenderStepsPass.h"
#include "vtkSSAAPass.h"
#include "vtkSobelGradientMagnitudePass.h"

int TestSSAAPassDepthRestore(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkActor> actor;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  vtkOpenGLRenderer* glren = vtkOpenGLRenderer::SafeDownCast(renderer);

  // create the basic VTK render steps
  vtkNew<vtkRenderStepsPass> basicPasses;

  // ssaa
  vtkNew<vtkSSAAPass> ssaa;
  ssaa->SetDelegatePass(basicPasses);

  // sgm pass requires depth buffer
  vtkNew<vtkSobelGradientMagnitudePass> sgm;
  sgm->SetDelegatePass(ssaa);

  // tell the renderer to use our render pass pipeline
  glren->SetPass(sgm);

  renWin->SetSize(500, 500);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  mapper->SetInputConnection(reader->GetOutputPort());

  actor->GetProperty()->SetAmbientColor(0.2, 0.4, 1.0);
  actor->GetProperty()->SetDiffuseColor(0.8, 1.0, 0.5);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.8);
  actor->GetProperty()->SetDiffuse(0.8);
  actor->GetProperty()->SetAmbient(0.3);
  actor->GetProperty()->SetSpecularPower(50.0);
  actor->GetProperty()->SetOpacity(1.0);

  renWin->Render();

  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
