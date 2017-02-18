/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// test baking shadow maps
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkCellArray.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLTexture.h"
#include "vtkPLYReader.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSequencePass.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkShadowMapPass.h"
#include "vtkTestUtilities.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
int TestShadowMapPass(int argc, char *argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.4, 0.6);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer.Get());
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());

  vtkNew<vtkLight> light1;
  light1->SetFocalPoint(0,0,0);
  light1->SetPosition(0,1,0.2);
  light1->SetColor(0.95,0.97,1.0);
  light1->SetIntensity(0.8);
  renderer->AddLight(light1.Get());

  vtkNew<vtkLight> light2;
  light2->SetFocalPoint(0,0,0);
  light2->SetPosition(1.0,1.0,1.0);
  light2->SetColor(1.0,0.8,0.7);
  light2->SetIntensity(0.3);
  renderer->AddLight(light2.Get());

  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(0.135, 0.2225, 0.3);
  actor->GetProperty()->SetDiffuseColor(0.54, 0.89, 0.63);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.51);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.7);
  actor->GetProperty()->SetSpecularPower(30.0);
  actor->GetProperty()->SetOpacity(1.0);
  renderer->AddActor(actor.Get());
  //actor->GetProperty()->SetRepresentationToWireframe();

  // add a plane
  vtkNew<vtkPlaneSource> plane;
  const double *plybounds = mapper->GetBounds();
  plane->SetOrigin(-0.2, plybounds[2], -0.2);
  plane->SetPoint1( 0.2, plybounds[2], -0.2);
  plane->SetPoint2(-0.2, plybounds[2],  0.2);
  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());
  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper.Get());
  renderer->AddActor(planeActor.Get());

  renderWindow->SetMultiSamples(0);

  vtkNew<vtkShadowMapPass> shadows;

  vtkNew<vtkSequencePass> seq;
  vtkNew<vtkRenderPassCollection> passes;
  passes->AddItem(shadows->GetShadowMapBakerPass());
  passes->AddItem(shadows.Get());
  seq->SetPasses(passes.Get());

  vtkNew<vtkCameraPass> cameraP;
  cameraP->SetDelegatePass(seq.Get());

  // tell the renderer to use our render pass pipeline
  vtkOpenGLRenderer *glrenderer =
    vtkOpenGLRenderer::SafeDownCast(renderer.GetPointer());
  glrenderer->SetPass(cameraP.Get());

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;

  timer->StartTimer();
  int numRenders = 8;
  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(80.0/numRenders);
    renderer->GetActiveCamera()->Elevation(80.0/numRenders);
    renderWindow->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  unsigned int numTris = reader->GetOutput()->GetPolys()->GetNumberOfCells();
  cerr << "number of triangles: " <<  numTris << endl;
  cerr << "triangles per second: " <<  numTris*(numRenders/elapsed) << endl;

  renderer->GetActiveCamera()->SetPosition(-0.2,0.2,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.5);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
