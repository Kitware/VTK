/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImager.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkImager.h"
#include "vtkImageWindow.h"
#include "vtkImagingFactory.h"
#include "vtkCommand.h"

vtkImager* vtkImager::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkImagingFactory::CreateInstance("vtkImager");
  if(ret)
    {
    return (vtkImager*)ret;
    }
  // If the factory was unable to create the object, then create it here.
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

vtkImager::~vtkImager()
{
  this->SetImageWindow( NULL );
}

int vtkImager::RenderOpaqueGeometry()
{
  int       renderedSomething = 0;
  vtkProp*  tempActor;

  vtkDebugMacro (<< "vtkImager::RenderOpaque");
  
  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  
  for ( this->Props->InitTraversal(); 
	(tempActor = this->Props->GetNextProp());)
    {
    // Make sure that the actor is visible before rendering
    if (tempActor->GetVisibility() == 1)
      {
      renderedSomething += tempActor->RenderOpaqueGeometry(this);
      }
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething; 
}

int vtkImager::RenderTranslucentGeometry()
{
  int       renderedSomething = 0;
  vtkProp*  tempActor;

  vtkDebugMacro (<< "vtkImager::RenderTranslucent");
  
  for ( this->Props->InitTraversal(); 
	(tempActor = this->Props->GetNextProp());)
    {
    // Make sure that the actor is visible before rendering
    if (tempActor->GetVisibility() == 1)
      {
      renderedSomething += tempActor->RenderTranslucentGeometry(this);
      }
    }
  
  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething; 
}

int vtkImager::RenderOverlay()
{
  int       renderedSomething = 0;
  vtkProp  *tempActor;

  vtkDebugMacro (<< "vtkImager::RenderOverlay");
  
  for (this->Props->InitTraversal(); 
       (tempActor = this->Props->GetNextProp());)
    {
    // Make sure that the actor is visible before rendering
    if (tempActor->GetVisibility() == 1)
      {
      renderedSomething += tempActor->RenderOverlay(this);
      }
    }
  
  this->InvokeEvent(vtkCommand::EndEvent,NULL);
  
  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething; 
}

// Do not reference count.  
// It is to hard to detect win<->imager reference loop.
void vtkImager::SetImageWindow(vtkImageWindow* win)
{
  vtkProp *aProp;

  if (win != this->VTKWindow)
    {
    // This renderer is be dis-associated with its previous render window.
    // this information needs to be passed to the renderer's actors and
    // volumes so they can release and render window specific (or graphics
    // context specific) information (such as display lists and texture ids)
    this->Props->InitTraversal();
    for ( aProp = this->Props->GetNextProp();
	  aProp != NULL;
	  aProp = this->Props->GetNextProp() )
      {
      aProp->ReleaseGraphicsResources(this->VTKWindow);
      }
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
vtkAssemblyPath* vtkImager::PickProp(float vtkNotUsed(selectionX), 
                                     float vtkNotUsed(selectionY))
{
  return NULL;
}

float vtkImager::GetPickedZ()
{
  return 0;
}

void vtkImager::DevicePickRender()
{
}

void vtkImager::StartPick(unsigned int vtkNotUsed(pickFromSize))
{
}

void vtkImager::UpdatePickId()
{
}

void vtkImager::DonePick()
{
}

unsigned int vtkImager::GetPickedId()
{
  return 0;
}



