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
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

//----------------------------------------------------------------------------
// Test one can create an resize offscreen render windows.
int TestOffscreenRenderingResize(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> window;
  window->SetShowWindow(false);
  window->SetUseOffScreenBuffers(true);
  window->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(window);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.3, 0.3, 0.3);
  window->AddRenderer(ren);

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort(0));
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  ren->AddActor(actor);

  ren->ResetCamera();
  window->Render();

  window->SetSize(400, 300);
  window->Render();
  int retVal = vtkRegressionTestImage(window);
  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
