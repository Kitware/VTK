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
#include "vtkPLYReader.h"
#include "vtkNew.h"
#include "vtkProperty.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

#include "vtkOpenGLRenderWindow.h"

#include "vtkCullerCollection.h"
#include "vtkLight.h"

#include "vtkOculusCamera.h"
#include "vtkOculusRenderer.h"
#include "vtkOculusRenderWindow.h"
#include "vtkOculusRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestDragon(int argc, char *argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkOculusRenderer> renderer;
  renderer->SetBackground(0.2, 0.3, 0.4);
  vtkNew<vtkOculusRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  vtkNew<vtkOculusRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());
  vtkNew<vtkOculusCamera> cam;
  renderer->SetActiveCamera(cam.Get());

  // crazy frame rate requirement
  // need to look into that at some point
  renderWindow->SetDesiredUpdateRate(350.0);
  iren->SetDesiredUpdateRate(350.0);
  iren->SetStillUpdateRate(350.0);

  renderer->RemoveCuller(renderer->GetCullers()->GetLastItem());

  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(0.0, 1.0, 0.3);
  renderer->AddLight(light.Get());

  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(
      argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
//  actor->GetProperty()->SetRepresentationToWireframe();

  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
