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

int TestRemoveActors(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  double x = 0.0, y = 0.0, z = 0.0;
  double spacingX = 2.0, spacingY = 2.0, spacingZ = 2.0;
  for (int k = 0; k < 3; ++k)
  {
    for (int j = 0; j < 3; ++j)
    {
      for (int i = 0; i < 3; ++i)
      {
        vtkNew<vtkConeSource> cone;
        cone->SetCenter(x, y, z);
        x += spacingX;
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputConnection(cone->GetOutputPort());
        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        mapper->DebugOn();
        mapper->Update();
        actor->GetProperty()->SetEdgeVisibility(true);
        actor->GetProperty()->SetLineWidth(2);
        actor->GetProperty()->SetEdgeColor(1.0, 0.0, 0.0);
        renderer->AddActor(actor);
      }
      x = 0.0;
      y += spacingY;
    }
    y = 0.0;
    z += spacingZ;
  }

  renderer->ResetCamera();
  renderer->SetBackground(1, 1, 1);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);
  renWin->Render();

  renderer->RemoveAllViewProps();
  renWin->Render();
  vtkNew<vtkUnsignedCharArray> pixels;
  renWin->GetRGBACharPixelData(0, 0, renWin->GetSize()[0] - 1, renWin->GetSize()[1] - 1, 0, pixels);
  for (vtkIdType i = 0; i < pixels->GetNumberOfTuples(); ++i)
  {
    for (int c = 0; c < pixels->GetNumberOfComponents() - 1; ++c)
    {
      const auto value = pixels->GetComponent(i, c);
      if (value != 255)
      {
        std::cerr << "Unexpected pixel value " << int(value) << '\n';
        return EXIT_FAILURE;
      }
    }
  }

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
