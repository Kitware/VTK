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

#ifdef _WIN32
  #include "vtkOpenGLImager.h"
#endif
#ifdef VTK_USE_OGLR
  #include "vtkOpenGLImager.h"
#endif

vtkImager* vtkImager::New()
{
#ifdef _WIN32
#ifndef VTK_USE_NATIVE_IMAGING
  return vtkOpenGLImager::New();
#else
  return new vtkImager;
#endif
#else
#ifdef VTK_USE_OGLR
#ifndef VTK_USE_NATIVE_IMAGING
  return vtkOpenGLImager::New();
#else
  return new vtkImager;
#endif
#endif
#endif
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

int vtkImager::RenderOpaqueGeometry()
{
  int       renderedSomething = 0;
  vtkProp*  tempActor;

  vtkDebugMacro (<< "vtkImager::RenderOpaque");
  
  if (this->StartRenderMethod) 
    {
    (*this->StartRenderMethod)(this->StartRenderMethodArg);
    }
  
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
  
  if (this->EndRenderMethod) 
    {
    (*this->EndRenderMethod)(this->EndRenderMethodArg);
    }
  
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
