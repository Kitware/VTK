/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImager.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImager.h"
#include "vtkImageWindow.h"
#include "vtkImagingFactory.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkImager, "1.30");

vtkImager* vtkImager::New()
{
  vtkGenericWarningMacro("vtkImager is being deprecated in version 4.1 please use vtkRenderer instead.");
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


vtkImageWindow *vtkImager::GetImageWindow()
{
  return static_cast<vtkImageWindow*>(this->VTKWindow);
}

