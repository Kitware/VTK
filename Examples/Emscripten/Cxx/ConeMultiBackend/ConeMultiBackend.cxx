// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
  // Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindowInteractor->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  vtkNew<vtkMinimalStandardRandomSequence> seq;

  double spacingX = 2.0, spacingY = 2.0, spacingZ = 2.0;

  const int nx = std::atoi(argv[1]);
  const int ny = std::atoi(argv[2]);
  const int nz = std::atoi(argv[3]);
  const int mapperIsStatic = std::atoi(argv[4]);

  double x = 0.0, y = 0.0, z = 0.0;
  for (int k = 0; k < nz; ++k)
  {
    for (int j = 0; j < ny; ++j)
    {
      for (int i = 0; i < nx; ++i)
      {
        vtkNew<vtkConeSource> coneSrc;
        coneSrc->SetResolution(10);
        // position the cone
        coneSrc->SetCenter(x, y, z);

        coneSrc->Update();
        vtkPolyData* cone = coneSrc->GetOutput();

        // generate random colors for each face of the cone.
        vtkNew<vtkUnsignedCharArray> colors;
        colors->SetNumberOfComponents(4);
        seq->SetSeed(k * ny * nx + j * nx + i);
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
        mapper->SetStatic(mapperIsStatic);

        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        actor->GetProperty()->SetEdgeVisibility(1);
        actor->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
        mapper->Update();
        actor->SetOrigin(x, y, z);
        actor->RotateZ(i * j);
        renderer->AddActor(actor);

        x += spacingX;
      }
      x = 0.0;
      y += spacingY;
    }
    y = 0.0;
    z += spacingZ;
  }
  std::cout << "Created " << nx * ny * nz << " cones" << std::endl;

  // Start rendering app
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->SetSize(300, 300);
  renderWindow->Render();

  // Start event loop
  renderWindowInteractor->Start();

  return 0;
}
