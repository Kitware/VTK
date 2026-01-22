// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkImageFlip.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkJPEGReader.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLSkybox.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#include <cstdlib>

int TestSkyboxCubeRotation(int argc, char* argv[])
{
  vtkNew<vtkOpenGLRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkOpenGLSkybox> skybox;
  vtkNew<vtkTexture> texture;
  texture->CubeMapOn();

  const char* fpath[] = { "Data/skybox-px.jpg", "Data/skybox-nx.jpg", "Data/skybox-py.jpg",
    "Data/skybox-ny.jpg", "Data/skybox-pz.jpg", "Data/skybox-nz.jpg" };

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkJPEGReader> imgReader;
    const char* fName = vtkTestUtilities::ExpandDataFileName(argc, argv, fpath[i]);
    imgReader->SetFileName(fName);
    delete[] fName;
    vtkNew<vtkImageFlip> flip;
    flip->SetInputConnection(imgReader->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    texture->SetInputConnection(i, flip->GetOutputPort(0));
  }

  skybox->SetTexture(texture);
  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(texture);

  renderer->AddActor(skybox);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(75);
  sphere->SetPhiResolution(75);

  vtkNew<vtkPolyDataMapper> pdSphere;
  pdSphere->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actorSphere;
  actorSphere->SetMapper(pdSphere);
  actorSphere->GetProperty()->SetInterpolationToPBR();
  actorSphere->GetProperty()->SetMetallic(1.0);
  actorSphere->GetProperty()->SetRoughness(0.3);
  renderer->AddActor(actorSphere);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  if (retVal != vtkRegressionTester::PASSED)
  {
    vtkLog(ERROR, "Image comparison with default cube map settings failed.");
    return EXIT_FAILURE;
  }

  renderer->SetEnvironmentRight(0, 0, 1);
  renderer->SetEnvironmentUp(0, 1, 0);
  renWin->Render();

  retVal = vtkRegressionTestImage(renWin);
  if (retVal != vtkRegressionTester::PASSED)
  {
    vtkLog(ERROR, "Image comparison after rotation failed.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
