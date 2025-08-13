// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDRReader.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLSkybox.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSkybox.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

int TestSkyboxRotation(int argc, char* argv[])
{
  vtkNew<vtkOpenGLRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkOpenGLSkybox> skybox;

  vtkNew<vtkHDRReader> reader;
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/spiaggia_di_mondello_1k.hdr");
  reader->SetFileName(fname);
  delete[] fname;
  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->MipmapOn();
  texture->InterpolateOn();
  texture->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->RotateX(25);
  transform->RotateY(10);
  transform->RotateZ(-90);

  vtkMatrix4x4* mat4 = transform->GetMatrix();
  vtkNew<vtkMatrix3x3> rotMat;
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      rotMat->SetElement(i, j, mat4->GetElement(i, j));
    }
  }

  renderer->SetEnvironmentRotationMatrix(rotMat);
  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(texture);

  skybox->SetFloorRight(0.0, 0.0, 1.0);
  skybox->SetProjection(vtkSkybox::Sphere);
  skybox->SetTexture(texture);

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

  return !retVal;
}
