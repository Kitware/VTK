// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkConeSource.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedCharArray.h"

int TestConesBenchmark(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  double x = 0.0, y = 0.0, z = 0.0;
  vtkNew<vtkMinimalStandardRandomSequence> seq;
  double spacingX = 2.0, spacingY = 2.0, spacingZ = 2.0;
  int nx = 100, ny = 10, nz = 10;
  for (int k = 0; k < nz; ++k)
  {
    for (int j = 0; j < ny; ++j)
    {
      for (int i = 0; i < nx; ++i)
      {
        vtkNew<vtkConeSource> coneSrc;
        coneSrc->SetResolution(10);
        coneSrc->SetCenter(x, y, z);
        x += spacingX;

        coneSrc->Update();
        vtkPolyData* cone = coneSrc->GetOutput();

        seq->SetSeed(k * ny * nx + j * nx + i);
        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetNumberOfComponents(4);
        for (vtkIdType cellId = 0; cellId < cone->GetNumberOfPolys(); ++cellId)
        {
          double red = seq->GetNextRangeValue(0, 255.);
          double green = seq->GetNextRangeValue(0, 255.);
          double blue = seq->GetNextRangeValue(0, 255.);
          colors->InsertNextTuple4(red, green, blue, 255);
        }
        cone->GetCellData()->SetScalars(colors);

        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(cone);
        mapper->Update();
        mapper->SetStatic(true);
        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        actor->GetProperty()->SetEdgeVisibility(1);
        actor->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
        mapper->Update();
        actor->SetOrigin(x, y, z);
        actor->RotateZ(i * j);
        renderer->AddActor(actor);
      }
      x = 0.0;
      y += spacingY;
    }
    y = 0.0;
    z += spacingZ;
  }

  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->Render();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);
  renWin->Render();

  iren->Start();
  return 0;
}
