/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWorldPointPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWorldPointPicker.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkWorldPointPicker);

vtkWorldPointPicker::vtkWorldPointPicker()
{
}

// Perform pick operation with selection point provided. The z location
// is recovered from the zBuffer. Always returns 0 since no actors are picked.
int vtkWorldPointPicker::Pick(double selectionX, double selectionY, 
                              double selectionZ, vtkRenderer *renderer)
{
  vtkCamera *camera;
  double cameraFP[4];
  double display[3], *world;
  double *displayCoord;
  double z;

  // Initialize the picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = selectionZ;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);

  z = renderer->GetZ (static_cast<int>(selectionX),
                      static_cast<int>(selectionY));
  
  // if z is 1.0, we assume the user has picked a point on the
  // screen that has not been rendered into. Use the camera's focal
  // point for the z value. The test value .999999 has to be used
  // instead of 1.0 because for some reason our SGI Infinite Reality
  // engine won't return a 1.0 from the zbuffer
  if (z < 0.999999)
    {
    selectionZ = z;
    vtkDebugMacro(<< " z from zBuffer: " << selectionZ);
    }
  else
    {
    // Get camera focal point and position. Convert to display (screen) 
    // coordinates. We need a depth value for z-buffer.
    camera = renderer->GetActiveCamera();
    camera->GetFocalPoint(cameraFP); cameraFP[3] = 1.0;

    renderer->SetWorldPoint(cameraFP[0],cameraFP[1],cameraFP[2],cameraFP[3]);
    renderer->WorldToDisplay();
    displayCoord = renderer->GetDisplayPoint();
    selectionZ = displayCoord[2];
    vtkDebugMacro(<< "computed z from focal point: " << selectionZ);
    }

  // now convert the display point to world coordinates
  display[0] = selectionX;
  display[1] = selectionY;
  display[2] = selectionZ;

  renderer->SetDisplayPoint (display);
  renderer->DisplayToWorld ();
  world = renderer->GetWorldPoint ();
  
  for (int i=0; i < 3; i++) 
    {
    this->PickPosition[i] = world[i] / world[3];
    }

  // Invoke end pick method if defined
  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);

  return 0;
}

void vtkWorldPointPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
