/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

#include "vtkOpenGLRenderWindow.h"

//----------------------------------------------------------------------------
int TestSharedRenderWindow(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->SetEndTheta(270.0);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetDiffuseColor(0.4, 1.0, 1.0);

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  vtkNew<vtkRenderer> renderer2;
  renderer2->SetBackground(0.0, 0.0, 1.0);
  vtkNew<vtkRenderWindow> renderWindow2;
  renderWindow2->SetSize(300, 300);
  renderWindow2->AddRenderer(renderer2);
  vtkNew<vtkRenderWindowInteractor> iren2;
  iren2->SetRenderWindow(renderWindow2);
  renderWindow2->SetSharedRenderWindow(renderWindow);

  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor2;
  renderer2->AddActor(actor2);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetDiffuseColor(1.0, 1.0, 0.4);

  renderWindow2->SetMultiSamples(0);
  renderer2->ResetCamera();
  renderer2->GetActiveCamera()->Elevation(-45);
  renderer2->GetActiveCamera()->OrthogonalizeViewUp();
  renderer2->GetActiveCamera()->Zoom(1.5);
  renderer2->ResetCameraClippingRange();
  renderWindow2->Render();

  int retVal = vtkRegressionTestImage(renderWindow2);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
