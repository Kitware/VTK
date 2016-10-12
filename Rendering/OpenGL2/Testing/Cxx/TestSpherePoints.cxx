/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkSphereSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"
#include "vtkLight.h"

#include "vtkRenderWindowInteractor.h"

#include "vtkOpenGLRenderWindow.h"

//----------------------------------------------------------------------------
int TestSpherePoints(int argc, char *argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer.Get());
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());

  vtkNew<vtkLight>light;
  light->SetPosition(0,1,0);
  light->SetLightTypeToSceneLight();
//  renderer->AddLight(light.Get());

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->SetEndTheta(270.0);

  {
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  renderer->AddActor(actor.Get());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetDiffuseColor(0.4, 1.0, 1.0);
  vtkNew<vtkProperty> backProp;
  backProp->SetDiffuseColor(0.4, 0.65, 0.8);
  actor->SetBackfaceProperty(backProp.Get());
  actor->GetProperty()->EdgeVisibilityOn();
  actor->GetProperty()->SetLineWidth(7.0);
  actor->GetProperty()->RenderLinesAsTubesOn();
  actor->GetProperty()->SetEdgeColor(1.0,1.0,1.0);
//  actor->GetProperty()->SetRepresentationToWireframe();
  }

  {
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  renderer->AddActor(actor.Get());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->RenderPointsAsSpheresOn();
  actor->GetProperty()->SetPointSize(14.0);
  actor->GetProperty()->SetRepresentationToPoints();
  }

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
//  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
