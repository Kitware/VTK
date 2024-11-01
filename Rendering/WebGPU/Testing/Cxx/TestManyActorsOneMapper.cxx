// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestManyActorsOneMapper(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(cone->GetOutputPort());

  double x = 0.0, y = 0.0, z = 0.0;
  double spacingX = 2.0, spacingY = 2.0, spacingZ = 2.0;
  for (int k = 0; k < 8; ++k)
  {
    for (int j = 0; j < 8; ++j)
    {
      for (int i = 0; i < 8; ++i)
      {
        x += spacingX;
        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        mapper->Update();
        mapper->SetStatic(1);
        actor->GetProperty()->SetEdgeVisibility(true);
        actor->GetProperty()->SetLineWidth(2);
        actor->GetProperty()->SetEdgeColor(1.0, 0.0, 0.0);
        actor->SetPosition(x, y, z);
        renderer->AddActor(actor);
      }
      x = 0.0;
      y += spacingY;
    }
    y = 0.0;
    z += spacingZ;
  }

  renderer->ResetCamera();
  renderer->SetBackground(0.1, 0.1, 0.1);
  renWin->Render();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);
  renWin->Render();

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
