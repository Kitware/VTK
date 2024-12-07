// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkElevationFilter.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkLight.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

#include "vtkAnariRendererNode.h"
#include "vtkAnariTestUtilities.h"
#include "vtkAnariWindowNode.h"

int TestAnariWindow(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort(0));

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(elev->GetOutputPort(0));
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  vtkNew<vtkLight> light1;

  // Create the RenderWindow, Renderer and all Actors
  vtkNew<vtkRenderer> ren1;

  SetAnariRendererParameterDefaults(ren1, useDebugDevice, "TestAnariWindow");

  ren1->AddLight(light1);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sphereActor);
  ren1->SetBackground(.2, .3, .4);

  // render the image
  renWin->SetWindowName("TestAnariWindow");
  renWin->SetSize(600, 500);

  vtkNew<vtkAnariWindowNode> anariWindow;
  anariWindow->SetRenderable(renWin);
  anariWindow->TraverseAllPasses();

  // now get the result and display it
  int* size = anariWindow->GetSize();
  vtkNew<vtkImageData> image;
  image->SetDimensions(size[0], size[1], 1);
  image->GetPointData()->SetScalars(anariWindow->GetColorBuffer());

  // Create a new image actor and remove the geometry one
  vtkNew<vtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(image);
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(imageActor);
  // Background color white to distinguish image boundary
  renderer->SetBackground(1, 1, 1);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderWindowInteractor->Start();
  }

  return !retVal;
}
