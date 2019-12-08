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
#include "vtkAvatar.h"
#include "vtkCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

#include "vtkLight.h"

//----------------------------------------------------------------------------
int TestAvatar(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 7.0, 1.0);
  renderer->AddLight(light);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);

  delete[] fileName;

  vtkNew<vtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(norms->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetPosition(0.4, 0, 0);
  actor->SetScale(3.0, 3.0, 3.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetSpecularPower(20);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.4);
  actor->GetProperty()->SetAmbientColor(0.4, 0.0, 1.0);
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  vtkNew<vtkAvatar> avatar;
  avatar->SetHeadPosition(-2.4, 0.2, 0);
  avatar->SetHeadOrientation(0, 20, 0);
  avatar->SetLeftHandPosition(-0.9, -0.3, -0.7);
  avatar->SetLeftHandOrientation(-10, -20, 15);
  avatar->SetRightHandPosition(-0.6, -0.4, 0.5);
  avatar->SetRightHandOrientation(0, 0, 0);
  avatar->GetProperty()->SetColor(0.8, 1.0, 0.8);
  // avatar->SetScale(0.3);
  renderer->AddActor(avatar);

  renderer->GetActiveCamera()->SetPosition(-1.0, 0.25, 5.0);
  renderer->GetActiveCamera()->SetFocalPoint(-1.0, 0.25, 0.0);
  renderer->GetActiveCamera()->SetViewAngle(55.0);
  renderer->GetActiveCamera()->Zoom(1.1);
  renderer->GetActiveCamera()->Azimuth(0);
  renderer->GetActiveCamera()->Elevation(15);
  // renderer->GetActiveCamera()->Roll(-10);
  renderer->SetBackground(0.6, 0.7, 1.0);
  renderer->ResetCameraClippingRange();
  renderer->SetClippingRangeExpansion(1.5);

  renderWindow->Render();

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindow->GetInteractor()->SetInteractorStyle(style);
  // style->SetAutoAdjustCameraClippingRange(true);

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
