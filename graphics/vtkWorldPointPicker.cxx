/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWorldPointPicker.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkWorldPointPicker.h"

vtkWorldPointPicker::vtkWorldPointPicker()
{
  this->PointId = -1;
}

void vtkWorldPointPicker::Initialize()
{
  this->PointId = (-1);
  this->vtkPicker::Initialize();
}

// Description:
// Perform pick operation with selection point provided. The z location
// is recovered from the zBuffer. Always returns 0 since no actors are picked.
int vtkWorldPointPicker::Pick(float selectionX, float selectionY, float selectionZ,
                   vtkRenderer *renderer)
{
  vtkCamera *camera;
  float cameraFP[4];
  float display[3], *world;
  float *displayCoord;
  float z;

  z = renderer->GetZ ((int) selectionX, (int) selectionY);

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
    //
    // Get camera focal point and position. Convert to display (screen) 
    // coordinates. We need a depth value for z-buffer.
    //
    camera = renderer->GetActiveCamera();
    camera->GetFocalPoint((float *)cameraFP); cameraFP[3] = 1.0;

    renderer->SetWorldPoint(cameraFP);
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
    this->PickPosition[i] = world[i] / world[3];

  return 0;
}

void vtkWorldPointPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkPicker::PrintSelf(os,indent);

  os << indent << "Point Id: " << this->PointId << "\n";
}
