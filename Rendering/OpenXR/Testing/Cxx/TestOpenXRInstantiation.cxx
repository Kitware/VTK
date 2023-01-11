/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenXRCamera.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"

//------------------------------------------------------------------------------
// Only instanciates, do not requires a OpenXR implementation to run
int TestOpenXRInstantiation(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkOpenXRRenderer> renderer;
  vtkNew<vtkOpenXRRenderWindow> renderWindow;
  vtkNew<vtkOpenXRCamera> cam;
  vtkNew<vtkOpenXRRenderWindowInteractor> iren;
  vtkNew<vtkActor> actor;

  renderer->SetActiveCamera(cam);
  renderer->AddActor(actor);
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);

  return 0;
}
