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

// Create an imager with viewport (0, 0, 1, 1)
vtkImager::vtkImager()
{
  vtkDebugMacro(<< "vtkImager::vtkImager");

  this->Viewport[0] = 0.0; // min x
  this->Viewport[1] = 0.0; // min y
  this->Viewport[2] = 1.0; // max x
  this->Viewport[3] = 1.0; // max y

}

void vtkImager::Render()
{
  vtkActor2D* tempActor;

  vtkDebugMacro (<< "vtkImager::Render");
  
  if (this->StartRenderMethod) 
    {
    (*this->StartRenderMethod)(this->StartRenderMethodArg);
    }
  
  int numActors = this->Actors2D->GetNumberOfItems();
  if (numActors == 0)
    {
    vtkDebugMacro (<< "vtkImager::Render - No actors in collection");
    return;
    }
  
  vtkDebugMacro(<<"vtkImager::Render - " << numActors << " actors in collection");
  vtkDebugMacro(<<"vtkImager::Render - Sorting actor collection.");
  
  this->Actors2D->Sort();  

  for ( this->Actors2D->InitTraversal(); 
	(tempActor = this->Actors2D->GetNextItem());)
    {
    // Make sure that the actor is visible before rendering
    if (tempActor->GetVisibility() == 1)
      {
      tempActor->Render(this);
      }
    }
  
  if (this->EndRenderMethod) 
    {
    (*this->EndRenderMethod)(this->EndRenderMethodArg);
    }
  
  return;
  
}



