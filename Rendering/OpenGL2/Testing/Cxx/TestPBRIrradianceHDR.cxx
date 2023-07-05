// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkHDRReader.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPBRIrradianceTexture.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

int TestPBRIrradianceHDR(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkOpenGLRenderer> renderer;
  renderer->UseSphericalHarmonicsOn();
  renderer->UseImageBasedLightingOn();
  renWin->AddRenderer(renderer);

  vtkNew<vtkHDRReader> reader;
  char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/spiaggia_di_mondello_1k.hdr");
  reader->SetFileName(fileName);
  delete[] fileName;

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->MipmapOn();
  texture->InterpolateOn();
  texture->SetInputConnection(reader->GetOutputPort());

  renderer->SetEnvironmentTexture(texture);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(30);
  sphere->SetPhiResolution(30);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->GetProperty()->SetInterpolationToPBR();
  actor->GetProperty()->SetRoughness(0.0);
  actor->GetProperty()->SetColor(0.7, 0.0, 0.2);
  actor->SetMapper(mapper);

  renderer->AddActor(actor);

  renWin->Render();

  renderer->GetActiveCamera()->Zoom(1.6);

  iren->Start();

  return 0;
}
