/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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

#include "vtkMapper2D.h"

void vtkMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
	this->vtkReferenceCount::PrintSelf(os, indent);
}

// Description:
// Calculates the size of a viewport in pixels.
void vtkMapper2D::GetViewportClipSize(vtkViewport* viewport, int* clipWidth, int* clipHeight)
{
  // Get the viewport coordinates
  float* vpt = viewport->GetViewport(); 

  // Get the window size
  vtkWindow* window = viewport->GetVTKWindow();
  int* winSize = window->GetSize();

  // Calculate a clip width and height for the image based on the 
  // size of the viewport
  float vptWidth = vpt[XMAX] - vpt[XMIN];
  float vptHeight = vpt[YMAX] - vpt[YMIN];
  *clipWidth = (int) (vptWidth * (float) winSize[0]);
  *clipHeight = (int) (vptHeight * (float) winSize[1]);

}

// Description:
// Calculates the width and height of an actor2D taking into 
// account the clipping boundaries of a viewport.
void vtkMapper2D::GetActorClipSize(vtkViewport* viewport, vtkActor2D* actor, int* clipWidth, int* clipHeight)
{
  float actorPos[2] = {0};

  // Get the clipping width and height of the viewport
  this->GetViewportClipSize(viewport, clipWidth, clipHeight);

  // Now get the actor position
  float* actorPos1 = actor->GetViewPosition();

  // Change the view coordinates to normalized device coordinates
  actorPos[0] = (actorPos1[0] + 1.0) / 2.0;
  actorPos[1] = (actorPos1[1] + 1.0) / 2.0;

  vtkDebugMacro(<<"NDC actorPos: " << actorPos[0] << "," << actorPos[1]);

  // Change the clipping size based on the width of the viewport
  // and the position of the actor (actorPos).  The region needed is 
  // the size of the viewport minus the starting coordinate of the
  // actor
  //  0 <= actorPos <= 1, so multiply by viewport width

  *clipWidth = *clipWidth - actorPos[0] * (*clipWidth);
  *clipHeight = *clipHeight - actorPos[1] * (*clipHeight);
  
  // Now take into account the actor's scale

  float* actorScale = actor->GetScale();

  // The region of data we need is the width_of_viewport / scale_in_x
  // and height_of_viewport / scale_in_y since 
  // region_width * scale_in_x = width_of_viewport and
  // region_height * scale_in_y = height_of_viewport

  *clipWidth = ((float) *clipWidth) / actorScale[0];
  *clipHeight = ((float) *clipHeight) / actorScale[1];
}

