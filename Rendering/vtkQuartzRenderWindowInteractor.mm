/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzRenderWindowInteractor.mm
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vtkQuartzRenderWindow.h"
#include "vtkQuartzRenderWindowInteractor.h"
#include "vtkQuartzWindow.h"
#include "vtkInteractorStyle.h"
#include "vtkActor.h"
#include <OpenGL/gl.h>
#include "vtkObjectFactory.h"
#include "vtkQuartzGLView.h"

#import <Cocoa/Cocoa.h>

#define id Id

vtkCxxRevisionMacro(vtkQuartzRenderWindowInteractor, "1.8");
vtkStandardNewMacro(vtkQuartzRenderWindowInteractor);

void (*vtkQuartzRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkQuartzRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkQuartzRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

// Construct object so that light follows camera motion.
vtkQuartzRenderWindowInteractor::vtkQuartzRenderWindowInteractor() 
{
  this->WindowId           = 0;
  this->ApplicationId		=0;
  this->InstallMessageProc = 1;
}

vtkQuartzRenderWindowInteractor::~vtkQuartzRenderWindowInteractor() 
{
    this->Enabled = 0;
}

void  vtkQuartzRenderWindowInteractor::Start() 
{
  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc) 
    {
    return;
    }
  [NSApp run];
}

// Begin processing keyboard strokes.
void vtkQuartzRenderWindowInteractor::Initialize()
{
  vtkQuartzRenderWindow *ren;
  int *size;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow) 
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
  ren = (vtkQuartzRenderWindow *)(this->RenderWindow);
  ren->Start();
  size    = ren->GetSize();
  ren->GetPosition();
  this->WindowId = ren->GetWindowId();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

void vtkQuartzRenderWindowInteractor::Enable() 
{
  vtkQuartzRenderWindow *ren;
  vtkQuartzRenderWindow *tmp;
  if (this->Enabled) 
    {
    return;
    }
  [(vtkQuartzWindow *)this->WindowId setVTKRenderWindowInteractor:this];
  [[(vtkQuartzWindow *)this->WindowId getvtkQuartzGLView] setVTKRenderWindowInteractor:this];
  this->Enabled = 1;
  this->Modified();
}


void vtkQuartzRenderWindowInteractor::Disable() 
{
  vtkQuartzRenderWindow *tmp;
  if (!this->Enabled) 
    {
    return;
    }
  
  [(vtkQuartzWindow *)this->WindowId setVTKRenderWindowInteractor:0];
  [[(vtkQuartzWindow *)this->WindowId getvtkQuartzGLView] setVTKRenderWindowInteractor:0];
  this->Enabled = 0;
  this->Modified();
}

void vtkQuartzRenderWindowInteractor::TerminateApp(void) 
{
  [NSApp terminate:(vtkQuartzWindow *)this->WindowId];
}

int vtkQuartzRenderWindowInteractor::CreateTimer(int notUsed) 
{
    [NSEvent stopPeriodicEvents];
    [NSEvent startPeriodicEventsAfterDelay:0.01 withPeriod:0.01];
    return 1;
}

int vtkQuartzRenderWindowInteractor::DestroyTimer(void) 
{
    [NSEvent stopPeriodicEvents];
  return 1;
}

// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkQuartzRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkQuartzRenderWindowInteractor::ClassExitMethod
       || arg != vtkQuartzRenderWindowInteractor::ClassExitMethodArg)
    {
    // delete the current arg if there is a delete method
    if ((vtkQuartzRenderWindowInteractor::ClassExitMethodArg)
        && (vtkQuartzRenderWindowInteractor::ClassExitMethodArgDelete))
      {
      (*vtkQuartzRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkQuartzRenderWindowInteractor::ClassExitMethodArg);
      }
    vtkQuartzRenderWindowInteractor::ClassExitMethod = f;
    vtkQuartzRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
    }
}


// Set the arg delete method.  This is used to free user memory.
void
vtkQuartzRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkQuartzRenderWindowInteractor::ClassExitMethodArgDelete)
    {
    vtkQuartzRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
    }
}

void vtkQuartzRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRenderWindowInteractor::PrintSelf(os,indent);
  os << indent << "InstallMessageProc: " << this->InstallMessageProc << endl;
}

void vtkQuartzRenderWindowInteractor::ExitCallback()
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
