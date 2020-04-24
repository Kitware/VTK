/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOutlineGlowPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test vtkOutlineGlowPass
// This test uses vtkOutlineGlowPass as intended with a layered renderer
// that draws he outline of a cone. The cone is rendered normally in the
// main renderer
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOutlineGlowPass.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"

int TestOutlineGlowPass(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);

  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderer> rendererOutline;
  rendererOutline->SetLayer(1);
  renWin->SetNumberOfLayers(2);
  renWin->AddRenderer(rendererOutline);
  renWin->AddRenderer(renderer);

  // Create the render pass
  vtkNew<vtkRenderStepsPass> basicPasses;
  vtkNew<vtkOutlineGlowPass> glowPass;
  glowPass->SetDelegatePass(basicPasses);

  // Apply the render pass to the highlight renderer
  rendererOutline->SetPass(glowPass);

  vtkNew<vtkConeSource> coneSource;

  // Create mapper and actor for the main renderer
  vtkNew<vtkPolyDataMapper> coneMapperMain;
  coneMapperMain->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> coneActorMain;
  coneActorMain->SetMapper(coneMapperMain);

  renderer->AddActor(coneActorMain);

  // Create mapper and actor for the outline
  vtkNew<vtkPolyDataMapper> coneMapperOutline;
  coneMapperOutline->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> coneActorOutline;
  coneActorOutline->SetMapper(coneMapperOutline);
  coneActorOutline->GetProperty()->SetColor(1.0, 0.0, 1.0);
  coneActorOutline->GetProperty()->LightingOff();

  rendererOutline->AddActor(coneActorOutline);

  renWin->SetSize(400, 400);

  int retVal;
  renderer->ResetCamera();
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renderer->ResetCamera();
  rendererOutline->SetActiveCamera(camera);
  renWin->Render();

  retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
