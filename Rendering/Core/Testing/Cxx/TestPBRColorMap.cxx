// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkColorTransferFunction.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestPBRColorMap(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.2, 0.3, 0.4);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // make a greyscale color map
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
  ctf->SetVectorModeToComponent();
  ctf->SetVectorComponent(0);

  vtkNew<vtkPlaneSource> plane;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());
  mapper->SetLookupTable(ctf);
  mapper->SetScalarRange(0.0, 1.0);
  mapper->SelectColorArray("TextureCoordinates");
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUsePointFieldData();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();
  actor->GetProperty()->SetBaseIOR(1.0); // remove specular

  renderer->AddActor(actor);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
