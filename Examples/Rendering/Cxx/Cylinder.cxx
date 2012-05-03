/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cylinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This simple example shows how to do basic rendering and pipeline
// creation using C++.
//
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"

int main()
{
  // This creates a polygonal cylinder model with eight circumferential facets.
  //
  vtkCylinderSource *cylinder = vtkCylinderSource::New();
  cylinder->SetResolution(8);

  // The mapper is responsible for pushing the geometry into the graphics
  // library. It may also do color mapping, if scalars or other attributes
  // are defined.
  //
  vtkPolyDataMapper *cylinderMapper = vtkPolyDataMapper::New();
  cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

  // The actor is a grouping mechanism: besides the geometry (mapper), it
  // also has a property, transformation matrix, and/or texture map.
  // Here we set its color and rotate it -22.5 degrees.
  vtkActor *cylinderActor = vtkActor::New();
  cylinderActor->SetMapper(cylinderMapper);
  cylinderActor->GetProperty()->SetColor(1.0000, 0.3882, 0.2784);
  cylinderActor->RotateX(30.0);
  cylinderActor->RotateY(-45.0);

  // Create the graphics structure. The renderer renders into the
  // render window. The render window interactor captures mouse events
  // and will perform appropriate camera or actor manipulation
  // depending on the nature of the events.
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(cylinderActor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(200, 200);

  // We'll zoom in a little by accessing the camera and invoking a "Zoom"
  // method on it.
  ren1->ResetCamera();
  ren1->GetActiveCamera()->Zoom(1.5);
  renWin->Render();

  // This starts the event loop and as a side effect causes an initial render.
  iren->Start();

  // Exiting from here, we have to delete all the instances that
  // have been created.
  cylinder->Delete();
  cylinderMapper->Delete();
  cylinderActor->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return 0;
}




