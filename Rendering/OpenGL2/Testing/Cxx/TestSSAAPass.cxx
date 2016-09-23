/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGaussianBlurPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the gaussian blur post-processing render pass.
// It renders an actor with a translucent LUT and depth
// peeling using the multi renderpass classes. The mapper uses color
// interpolation (poor quality).
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"

#include "vtkSSAAPass.h"
#include "vtkRenderStepsPass.h"

#include "vtkCellArray.h"
#include "vtkTimerLog.h"

int TestSSAAPass(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin.Get());
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());

  vtkNew<vtkActor> actor;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer->AddActor(actor.Get());
  actor->SetMapper(mapper.Get());

  vtkOpenGLRenderer *glrenderer =
      vtkOpenGLRenderer::SafeDownCast(renderer.GetPointer());

  // create the basic VTK render steps
  vtkNew<vtkRenderStepsPass> basicPasses;

  // finally blur the resulting image
  // The blur delegates rendering the unblured image
  // to the basicPasses
  vtkNew<vtkSSAAPass> ssaa;
  ssaa->SetDelegatePass(basicPasses.Get());

  // tell the renderer to use our render pass pipeline
  glrenderer->SetPass(ssaa.Get());
//  glrenderer->SetPass(basicPasses.Get());

  renWin->SetSize(500,500);

  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  mapper->SetInputConnection(reader->GetOutputPort());
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);

 vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  renWin->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  timer->StartTimer();
  int numRenders = 4;
  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(80.0/numRenders);
    renderer->GetActiveCamera()->Elevation(88.0/numRenders);
    renWin->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  unsigned int numTris = reader->GetOutput()->GetPolys()->GetNumberOfCells();
  cerr << "number of triangles: " <<  numTris << endl;
  cerr << "triangles per second: " <<  numTris*(numRenders/elapsed) << endl;

  renderer->GetActiveCamera()->SetPosition(0,0,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin.Get() );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
