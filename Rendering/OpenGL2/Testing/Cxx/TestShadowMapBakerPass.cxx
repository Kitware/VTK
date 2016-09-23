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
#include "vtkCellArray.h"
#include "vtkLightKit.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLTexture.h"
#include "vtkPLYReader.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkTestUtilities.h"
#include "vtkTextureObject.h"
#include "vtkTimerLog.h"


//----------------------------------------------------------------------------
int TestShadowMapBakerPass(int argc, char *argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer->SetBackground(0.3, 0.4, 0.6);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());
  vtkNew<vtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(renderer.Get());

  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  mapper->SetInputConnection(reader->GetOutputPort());
  //mapper->SetInputConnection(norms->GetOutputPort());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
  //actor->GetProperty()->SetRepresentationToWireframe();

  renderWindow->SetMultiSamples(0);

  vtkNew<vtkShadowMapBakerPass> bakerPass;

  // tell the renderer to use our render pass pipeline
  vtkOpenGLRenderer *glrenderer =
      vtkOpenGLRenderer::SafeDownCast(renderer.GetPointer());
  glrenderer->SetPass(bakerPass.Get());

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "baking time: " << firstRender << endl;

  // get a shadow map
  vtkTextureObject *to = (*bakerPass->GetShadowMaps())[2];
  // by default the textures have depth comparison on
  // but for simple display we need to turn it off
  to->SetDepthTextureCompare(false);

  // now render this texture so we can see the depth map
  vtkNew<vtkActor> actor2;
  vtkNew<vtkPolyDataMapper> mapper2;
  vtkNew<vtkOpenGLTexture> texture;
  texture->SetTextureObject(to);
  actor2->SetTexture(texture.Get());
  actor2->SetMapper(mapper2.Get());

  vtkNew<vtkPlaneSource> plane;
  mapper2->SetInputConnection(plane->GetOutputPort());
  renderer->RemoveActor(actor.Get());
  renderer->AddActor(actor2.Get());
  glrenderer->SetPass(NULL);

  renderer->GetActiveCamera()->SetPosition(0,0,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  bakerPass->ReleaseGraphicsResources(renderWindow.Get());
  return EXIT_SUCCESS;
}
