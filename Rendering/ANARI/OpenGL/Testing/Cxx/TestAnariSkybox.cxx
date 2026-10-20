// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSkybox.h"
#include "vtkSphereSource.h"
#include "vtkTexture.h"

#include "vtkAnariOpenGLTestUtilities.h"
#include "vtkAnariPass.h"

int TestAnariSkybox(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> window;
  window->SetSize(320, 160);
  window->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  constexpr unsigned char byteColors[4][3] = { { 32, 64, 128 }, { 192, 64, 32 }, { 64, 192, 128 },
    { 128, 128, 192 } };
  constexpr float floatColors[4][3] = { { 0.02f, 0.2f, 2.f }, { 1.f, 0.5f, 0.1f },
    { 0.1f, 0.02f, 0.5f }, { 0.2f, 1.5f, 0.1f } };

  for (int view = 0; view < 2; ++view)
  {
    vtkNew<vtkImageData> image;
    image->SetDimensions(64, 32, 1);
    image->AllocateScalars(view == 0 ? VTK_UNSIGNED_CHAR : VTK_FLOAT, 3);
    for (int y = 0; y < 32; ++y)
    {
      for (int x = 0; x < 64; ++x)
      {
        for (int c = 0; c < 3; ++c)
        {
          const int band = (x / 16 + y / 16) % 4;
          image->SetScalarComponentFromFloat(
            x, y, 0, c, view == 0 ? byteColors[band][c] : floatColors[band][c]);
        }
      }
    }

    vtkNew<vtkTexture> texture;
    texture->SetInputData(image);
    texture->SetColorModeToDirectScalars();
    texture->SetUseSRGBColorSpace(view == 0);
    texture->InterpolateOn();

    vtkNew<vtkSkybox> skybox;
    skybox->SetProjectionToSphere();
    skybox->SetTexture(texture);
    skybox->GammaCorrectOn();

    vtkNew<vtkSphereSource> sphere;
    // A black foreground object checks skybox depth without depending on the
    // back-end's lighting model.
    sphere->SetRadius(0.4);
    sphere->SetThetaResolution(48);
    sphere->SetPhiResolution(48);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(sphere->GetOutputPort());
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetMaterialName("matte");
    actor->GetProperty()->SetColor(0, 0, 0);
    actor->GetProperty()->SetSpecular(0);

    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(view * 0.5, 0, (view + 1) * 0.5, 1);
    renderer->AddActor(skybox);
    renderer->AddActor(actor);
    window->AddRenderer(renderer);

    vtkNew<vtkAnariPass> pass;
    vtkAnariOpenGLTestUtilities::SetParameterDefaults(pass, renderer, false, "TestAnariSkybox");
    renderer->SetPass(pass);
    renderer->GetActiveCamera()->SetPosition(0, 0, 3);
    renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    renderer->GetActiveCamera()->SetClippingRange(0.1, 10);
  }

  window->Render();
  int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
