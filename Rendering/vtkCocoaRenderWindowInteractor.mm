/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaRenderWindowInteractor.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "vtkCocoaRenderWindow.h"
#include "vtkCocoaRenderWindowInteractor.h"
#include "vtkCocoaWindow.h"
#include "vtkCocoaGLView.h"

#import <Cocoa/Cocoa.h>

#define id Id

#include "vtkInteractorStyle.h"
#include "vtkActor.h"
#include <OpenGL/gl.h>
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkCocoaRenderWindowInteractor, "1.6");
vtkStandardNewMacro(vtkCocoaRenderWindowInteractor);

//----------------------------------------------------------------------------
void (*vtkCocoaRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkCocoaRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

//----------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkCocoaRenderWindowInteractor::vtkCocoaRenderWindowInteractor() 
{
  this->WindowId           = 0;
  this->ApplicationId      = 0;
  this->InstallMessageProc = 1;
}

//----------------------------------------------------------------------------
vtkCocoaRenderWindowInteractor::~vtkCocoaRenderWindowInteractor() 
{
  this->Enabled = 0;
}

//----------------------------------------------------------------------------
void  vtkCocoaRenderWindowInteractor::Start() 
{
  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc) 
    {
    return;
    }
  [NSApp run];
}

//----------------------------------------------------------------------------
// Begin processing keyboard strokes.
void vtkCocoaRenderWindowInteractor::Initialize()
{
  vtkCocoaRenderWindow *ren;
  int *size;

  // make sure we have a RenderWindow and camera
  if ( !this->RenderWindow ) 
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }
  if (this->Initialized) 
    {
    return;
    }
  this->Initialized = 1;
  // get the info we need from the RenderingWindow
  ren = (vtkCocoaRenderWindow *)(this->RenderWindow);
  ren->Start();
  size = ren->GetSize();
  ren->GetPosition();
  this->WindowId = ren->GetWindowId();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::Enable() 
{
  if (this->Enabled) 
    {
    return;
    }
  [(vtkCocoaWindow *)this->WindowId setVTKRenderWindowInteractor:this];
  [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] setVTKRenderWindowInteractor:this];
  this->Enabled = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::Disable() 
{
  if (!this->Enabled) 
    {
    return;
    }
  
  [(vtkCocoaWindow *)this->WindowId setVTKRenderWindowInteractor:0];
  [[(vtkCocoaWindow *)this->WindowId getvtkCocoaGLView] setVTKRenderWindowInteractor:0];
  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::TerminateApp() 
{
  [NSApp terminate:(vtkCocoaWindow *)this->WindowId];
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindowInteractor::CreateTimer(int) 
{
  [NSEvent stopPeriodicEvents];
  [NSEvent startPeriodicEventsAfterDelay:0.01 withPeriod:0.01];

  return 1;
}

//----------------------------------------------------------------------------
int vtkCocoaRenderWindowInteractor::DestroyTimer()
{
  [NSEvent stopPeriodicEvents];
  return 1;
}

//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkCocoaRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkCocoaRenderWindowInteractor::ClassExitMethod
  || arg != vtkCocoaRenderWindowInteractor::ClassExitMethodArg)
    {
    // delete the current arg if there is a delete method
    if ((vtkCocoaRenderWindowInteractor::ClassExitMethodArg)
     && (vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete))
      {
      (*vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkCocoaRenderWindowInteractor::ClassExitMethodArg);
      }
    vtkCocoaRenderWindowInteractor::ClassExitMethod = f;
    vtkCocoaRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
    }
}


//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void vtkCocoaRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete)
    {
    vtkCocoaRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
    }
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InstallMessageProc: " << this->InstallMessageProc << endl;
}

//----------------------------------------------------------------------------
void vtkCocoaRenderWindowInteractor::ExitCallback()
{
  if (this->ClassExitMethod)
    {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
    }
  else if (this->ClassExitMethod)
    {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
    }
  else
    {
    this->TerminateApp();
    }
}
