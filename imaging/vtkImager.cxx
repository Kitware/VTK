/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImager.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkImager.h"
#include "vtkImageWindow.h"

vtkImager* vtkImager::New()
{
  return new vtkImager;
}

// Create an imager with viewport (0, 0, 1, 1)
vtkImager::vtkImager()
{
  vtkDebugMacro(<< "vtkImager::vtkImager");

  this->Viewport[0] = 0.0; // min x
  this->Viewport[1] = 0.0; // min y
  this->Viewport[2] = 1.0; // max x
  this->Viewport[3] = 1.0; // max y
}

void vtkImager::RenderOpaqueGeometry()
{
  vtkProp* tempActor;

  vtkDebugMacro (<< "vtkImager::RenderOpaque");
  
  if (this->StartRenderMethod) 
    {
    (*this->StartRenderMethod)(this->StartRenderMethodArg);
    }
  
  for ( this->Props->InitTraversal(); 
	(tempActor = this->Props->GetNextItem());)
    {
    // Make sure that the actor is visible before rendering
    if (tempActor->GetVisibility() == 1)
      {
      tempActor->RenderOpaqueGeometry(this);
      }
    }
  return; 
}

void vtkImager::RenderTranslucentGeometry()
{
  vtkProp* tempActor;

  vtkDebugMacro (<< "vtkImager::RenderTranslucent");
  
  for ( this->Props->InitTraversal(); 
	(tempActor = this->Props->GetNextItem());)
    {
    // Make sure that the actor is visible before rendering
    if (tempActor->GetVisibility() == 1)
      {
      tempActor->RenderTranslucentGeometry(this);
      }
    }
  
  return; 
}

void vtkImager::RenderOverlay()
{
  vtkProp *tempActor;

  vtkDebugMacro (<< "vtkImager::RenderOverlay");
  
  for (this->Props->InitTraversal(); 
       (tempActor = this->Props->GetNextItem());)
    {
    // Make sure that the actor is visible before rendering
    if (tempActor->GetVisibility() == 1)
      {
      tempActor->RenderOverlay(this);
      }
    }
  
  if (this->EndRenderMethod) 
    {
    (*this->EndRenderMethod)(this->EndRenderMethodArg);
    }
  
  return; 
}

// Do not reference count.  
// It is to hard to detect win<->imager reference loop.
void vtkImager::SetImageWindow(vtkImageWindow* win)
{
  if (win != this->VTKWindow)
    {
    this->VTKWindow = (vtkWindow*) win;
    this->Modified();
    }
}

// Do not reference count.  
// It is to hard to detect win<->imager reference loop.
void vtkImager::SetVTKWindow(vtkWindow* win) 
{
  if (win != this->VTKWindow)
    {
    this->VTKWindow = (vtkWindow*) win;
    this->Modified();
    }
}
