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
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPlaneSource.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#include "vtkLight.h"

//----------------------------------------------------------------------------
int TestSRGB(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 400);
  // renderWindow->SetUseSRGBColorSpace(true); // not supported on all hardware
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.Get());

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/skybox/posz.jpg");
  vtkNew<vtkJPEGReader> imgReader;
  imgReader->SetFileName(fileName);

  delete[] fileName;

  vtkNew<vtkPlaneSource> plane;

  for (int i = 0; i < 2; i++)
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(i == 0 ? 0.0 : 0.5, 0.0, i == 0 ? 0.5 : 1.0, 1.0);
    renderer->SetBackground(0.3, 0.3, 0.3);
    renderWindow->AddRenderer(renderer.Get());

    {
      vtkNew<vtkLight> light;
      light->SetLightTypeToSceneLight();
      light->SetPosition(-1.73, -1.0, 2.0);
      light->PositionalOn();
      light->SetConeAngle(90);
      light->SetAttenuationValues(0, 1.0, 0);
      light->SetColor(4, 0, 0);
      light->SetExponent(0);
      renderer->AddLight(light.Get());
    }
    {
      vtkNew<vtkLight> light;
      light->SetLightTypeToSceneLight();
      light->SetPosition(1.73, -1.0, 2.0);
      light->PositionalOn();
      light->SetConeAngle(90);
      light->SetAttenuationValues(0, 0, 1.0);
      light->SetColor(0, 6, 0);
      light->SetExponent(0);
      renderer->AddLight(light.Get());
    }
    {
      vtkNew<vtkLight> light;
      light->SetLightTypeToSceneLight();
      light->SetPosition(0.0, 2.0, 2.0);
      light->PositionalOn();
      light->SetConeAngle(50);
      light->SetColor(0, 0, 4);
      light->SetAttenuationValues(1.0, 0.0, 0.0);
      light->SetExponent(0);
      renderer->AddLight(light.Get());
    }

    vtkNew<vtkTexture> texture;
    texture->InterpolateOn();
    texture->RepeatOff();
    texture->EdgeClampOn();
    texture->SetUseSRGBColorSpace(i == 0);
    texture->SetInputConnection(imgReader->GetOutputPort(0));

    vtkNew<vtkOpenGLPolyDataMapper> mapper;
    mapper->SetInputConnection(plane->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetPosition(0, 0, 0);
    actor->SetScale(6.0, 6.0, 6.0);
    actor->GetProperty()->SetSpecular(0.2);
    actor->GetProperty()->SetSpecularPower(20);
    actor->GetProperty()->SetDiffuse(0.9);
    actor->GetProperty()->SetAmbient(0.2);
    // actor->GetProperty()->SetDiffuse(0.0);
    // actor->GetProperty()->SetAmbient(1.0);
    renderer->AddActor(actor.Get());
    actor->SetTexture(texture.Get());
    actor->SetMapper(mapper.Get());

    renderer->ResetCamera();
    renderer->GetActiveCamera()->Zoom(1.3);
    renderer->ResetCameraClippingRange();
  }

  renderWindow->Render();
  cout << "Render window sRGB status: "
       << static_cast<vtkOpenGLRenderWindow*>(renderWindow.Get())->GetUsingSRGBColorSpace() << "\n";
  int retVal = vtkRegressionTestImage(renderWindow.Get());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
