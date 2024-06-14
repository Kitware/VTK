// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**Description
 * Tests the skybox floor projection with coincident polygonal geometry
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSkybox.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTexture.h"

int TestCoincidentFloor(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkLight> light;

  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 7.0, 1.0);
  renderer->AddLight(light);
  vtkNew<vtkLight> light1;

  light1->SetLightTypeToHeadlight();
  light1->SetColor(1.0, 0.8, 1.0);
  light1->SetIntensity(0.5);
  renderer->AddLight(light1);

  const char* jpgname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");
  vtkNew<vtkJPEGReader> jpg;
  jpg->SetFileName(jpgname);
  delete[] jpgname;

  const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkTexture> texture;
  texture->InterpolateOn();
  texture->RepeatOn();
  texture->MipmapOn();
  texture->SetInputConnection(jpg->GetOutputPort(0));

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetPosition(0, -0.2, 0);
  actor->SetScale(6.0, 6.0, 6.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetSpecularPower(20);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.4);
  actor->GetProperty()->SetAmbientColor(0.4, 0.0, 1.0);
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  vtkNew<vtkPlaneSource> plane;
  plane->SetOrigin(-0.5, 0.0, -0.5);
  plane->SetPoint1(0.5, 0.0, -0.5);
  plane->SetPoint2(-0.5, 0.0, 0.5);

  vtkNew<vtkPolyDataMapper> pm;
  pm->SetInputConnection(plane->GetOutputPort());

  vtkNew<vtkActor> pa;
  pa->SetMapper(pm);
  pa->GetProperty()->SetColor(0.5, 0.23, 0.45);
  renderer->AddActor(pa);

  vtkNew<vtkSkybox> floor;
  floor->SetFloorPlane(0, 1, 0, 0.0);
  floor->SetFloorRight(0, 0, 1);
  // Scale the texture coordinates;
  floor->SetFloorTexCoordScale(1.2, 0.9);
  floor->SetProjectionToFloor();
  floor->SetTexture(texture);
  renderer->AddActor(floor);

  renderer->GetActiveCamera()->SetPosition(0.0, 0.55, 3.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.55, 0.0);
  renderer->SetBackground(0.6, 0.7, 1.0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
