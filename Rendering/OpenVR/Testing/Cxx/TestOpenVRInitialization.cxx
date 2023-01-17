/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenVRCamera.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"

//------------------------------------------------------------------------------
// Only initialize, requires a OpenVR implementation but do not render anything
int TestOpenVRInitialization(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkOpenVRRenderer> renderer;
  vtkNew<vtkOpenVRRenderWindow> renderWindow;
  vtkNew<vtkOpenVRCamera> cam;
  vtkNew<vtkOpenVRRenderWindowInteractor> iren;
  vtkNew<vtkActor> actor;

  renderer->SetActiveCamera(cam);
  renderer->AddActor(actor);
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);
  iren->SetActionManifestDirectory("../../");

  renderWindow->Initialize();
  if (renderWindow->GetHMD())
  {
    iren->Initialize();
  }
  else
  {
    return 1;
  }

  return 0;
}
