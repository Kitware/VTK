/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the depth of field post-processing render pass.
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

#include "vtkDepthOfFieldPass.h"
#include "vtkRenderStepsPass.h"

#include "vtkCellArray.h"
#include "vtkTimerLog.h"

int TestDepthOfFieldPass(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin.Get());
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());

  vtkNew<vtkPolyDataMapper> mapper;
  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  mapper->SetInputConnection(reader->GetOutputPort());

  // create three dragons
  {
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(1.0, 0.0, 0.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.8, 0.3);
  actor->GetProperty()->SetSpecular(0.0);
  actor->GetProperty()->SetDiffuse(0.5);
  actor->GetProperty()->SetAmbient(0.3);
  actor->SetPosition(-0.1,0.0,-0.1);
  renderer->AddActor(actor.Get());
  }

  {
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(0.2, 1.0, 0.8);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.2);
  actor->GetProperty()->SetDiffuse(0.9);
  actor->GetProperty()->SetAmbient(0.1);
  actor->GetProperty()->SetSpecularPower(10.0);
  renderer->AddActor(actor.Get());
  }

  {
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetDiffuseColor(0.5, 0.65, 1.0);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.7);
  actor->GetProperty()->SetDiffuse(0.4);
  actor->GetProperty()->SetSpecularPower(60.0);
  actor->SetPosition(0.1,0.0,0.1);
  renderer->AddActor(actor.Get());
  }

  renderer->SetBackground(0.8,0.8,0.9);
  renderer->SetBackground2(1.0,1.0,1.0);
  renderer->GradientBackgroundOn();

  vtkOpenGLRenderer *glrenderer =
      vtkOpenGLRenderer::SafeDownCast(renderer.GetPointer());

  // create the basic VTK render steps
  vtkNew<vtkRenderStepsPass> basicPasses;

  // finally add the DOF passs
  vtkNew<vtkDepthOfFieldPass> dofp;
  dofp->SetDelegatePass(basicPasses.Get());
  dofp->AutomaticFocalDistanceOff();
  // tell the renderer to use our render pass pipeline
  glrenderer->SetPass(dofp.Get());

  renWin->SetSize(500,500);


  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetFocalDisk(renderer->GetActiveCamera()->GetDistance()*0.2);
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
  renderer->GetActiveCamera()->Azimuth(30.0);
  renderer->GetActiveCamera()->Zoom(1.8);
  renWin->Render();



  int retVal = vtkRegressionTestImage( renWin.Get() );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
