// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// Test for the different texture wrap modes

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTexture.h"
#include "vtkTextureMapToSphere.h"

int TestBackfaceTexture(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");

  vtkNew<vtkJPEGReader> reader;
  reader->SetFileName(fname);
  reader->Update();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400); // Intentional NPOT size

  vtkNew<vtkSphereSource> source;
  source->SetPhiResolution(20);
  source->SetThetaResolution(40);
  source->SetEndTheta(270.0);

  vtkNew<vtkTextureMapToSphere> t2s;
  t2s->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkTexture> texture;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkActor> actor;

  renWin->AddRenderer(ren);

  texture->SetInputConnection(reader->GetOutputPort());
  texture->InterpolateOn();

  mapper->SetInputConnection(t2s->GetOutputPort());
  actor->SetMapper(mapper);
  actor->SetTexture(texture);
  actor->GetProperty()->ShowTexturesOnBackfaceOff();

  ren->AddActor(actor);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.4);
  ren->GetActiveCamera()->Elevation(-60);
  ren->GetActiveCamera()->Azimuth(-10);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  delete[] fname;
  return !retVal;
}
