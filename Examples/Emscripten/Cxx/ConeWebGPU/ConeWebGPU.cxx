/*=========================================================================
  Program:   Visualization Toolkit
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
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
  vtkNew<vtkRenderWindow> rwin;
  rwin->SetMultiSamples(0);
  rwin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(rwin);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  for (int i = 0; i < 100; ++i)
  {
    for (int j = 0; j < 100; ++j)
    {
      vtkNew<vtkConeSource> cone;
      cone->SetCenter(i, j, 0);
      vtkNew<vtkPolyDataMapper> mapper;
      mapper->SetInputConnection(cone->GetOutputPort());
      mapper->Update();
      // mapper->SetStatic(1);
      vtkNew<vtkActor> actor;
      actor->SetMapper(mapper);
      actor->GetProperty()->EdgeVisibilityOn();
      renderer->AddActor(actor);
    }
  }

  // Start rendering app
  renderer->SetBackground(0.2, 0.3, 0.4);
  rwin->SetSize(300, 300);

  // Start event loop
  iren->Start();

  return 0;
}
