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
#import "vtkQuartzWindowController.h"
#include "vtkInteractorStyle.h"
#include "vtkActor.h"
#include <OpenGL/gl.h>
#include "vtkObjectFactory.h"

#import <Cocoa/Cocoa.h>

#define id Id

void VBTimerEvent(void *vtkClass)
{
    if (vtkClass) {
    ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->OnTimer();}
}

void DoMouseDragged(void *vtkClass, int shiftDown, int controlDown, int altDown, int commandDown, float xLoc, float yLoc)
{
    if (vtkClass) {
    ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->OnMouseMove(controlDown, shiftDown, xLoc, yLoc);}
}

void DoMouseMoved(void *vtkClass, int shiftDown, int controlDown, int altDown, int commandDown, float xLoc, float yLoc)
{
    if (vtkClass) {
    ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->OnMouseMove(controlDown, shiftDown, xLoc, yLoc);}
}

void DoMouseUp(void *vtkClass, int shiftDown, int controlDown, int altDown, int commandDown, float xLoc, float yLoc)
{
    if (vtkClass) {
        switch (((vtkQuartzRenderWindowInteractor *)vtkClass)->GetButtonDown())
        {
        case 1:
            ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->\
                OnLeftButtonUp(controlDown, shiftDown, xLoc, yLoc);
            break;
        case 2:
            ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->\
                OnMiddleButtonUp(controlDown, shiftDown, xLoc, yLoc);
            break;
        case 3:
            ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->\
                OnRightButtonUp(controlDown, shiftDown, xLoc, yLoc);
            break;
        default:
            break;
        }
         ((vtkQuartzRenderWindowInteractor *)vtkClass)->SetButtonDown(0);
    }
}

void DoMouseDown(void *vtkClass, int shiftDown, int controlDown, int altDown, int commandDown, float xLoc, float yLoc)
{
    int button=1;
    if (vtkClass){
        if (altDown) {button=2;}
        if (commandDown) {button=3;}
        ((vtkQuartzRenderWindowInteractor *)vtkClass)->SetButtonDown(button);
        switch (button)
        {
        case 1:
            ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->\
                OnLeftButtonDown(controlDown, shiftDown, xLoc, yLoc);
            break;
        case 2:
            ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->\
                OnMiddleButtonDown(controlDown, shiftDown, xLoc, yLoc);
            break;
        case 3:
            ((vtkQuartzRenderWindowInteractor *)vtkClass)->GetInteractorStyle()->\
                OnRightButtonDown(controlDown, shiftDown, xLoc, yLoc);
            break;
        default:
            break;
        }
    }
}



int vtkQuartzRenderWindowInteractor::GetButtonDown()
{
    return this->whichButtonDown;
}
void vtkQuartzRenderWindowInteractor::SetButtonDown(int button)
{
    this->whichButtonDown = button;
}


//------------------------------------------------------------------------------
vtkQuartzRenderWindowInteractor* vtkQuartzRenderWindowInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuartzRenderWindowInteractor");
  if(ret)
    {
    return (vtkQuartzRenderWindowInteractor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuartzRenderWindowInteractor;
}




void (*vtkQuartzRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkQuartzRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkQuartzRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

// Construct object so that light follows camera motion.
vtkQuartzRenderWindowInteractor::vtkQuartzRenderWindowInteractor() 
{
  static int timerId           = 1;
  this->WindowId           = 0;
  this->ApplicationId		=0;
  this->TimerId            = timerId++;
  this->InstallMessageProc = 1;
  this->whichButtonDown=0;
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
  static int any_initialized = 0;
  vtkQuartzRenderWindow *ren;
  int depth;
  int *size;
  int *position;
  int argc = 0;
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
  position= ren->GetPosition();
  this->WindowId = ren->GetWindowId();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
  [(vtkQuartzWindowController *)this->WindowId setVTKRenderWindowInteractor:this];
  ren->Render();
}

void vtkQuartzRenderWindowInteractor::Enable() 
{
  vtkQuartzRenderWindow *ren;
  vtkQuartzRenderWindow *tmp;
  if (this->Enabled) 
    {
    return;
    }
//DOQUARTZ stuff to connect windows to interactors
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
  
//DOQUARTZ to undo the above
  this->Enabled = 0;
  this->Modified();
}

void vtkQuartzRenderWindowInteractor::TerminateApp(void) 
{
  [NSApp terminate:(vtkQuartzWindowController *)this->WindowId];
}

int vtkQuartzRenderWindowInteractor::CreateTimer(int notUsed) 
{
    [NSEvent startPeriodicEventsAfterDelay:0.01 withPeriod:0.01];
    return 1;
}

int vtkQuartzRenderWindowInteractor::DestroyTimer(void) 
{
  //Timers die automatically
  return 1;
}

//-------------------------------------------------------------
// Event loop handlers
//-------------------------------------------------------------
//void vtkQuartzRenderWindowInteractor::OnMouseMove(void *wnd, int nFlags, 
//                                                 int X, int Y)
//{
//  if (!this->Enabled) 
//    {
//    return;
//    }
  //DOQUARTZInteractorStyle->OnMouseMove(nFlags & MK_CONTROL, nFlags & MK_SHIFT, 
           //                    X, this->Size[1] - Y - 1);
//}

void vtkQuartzRenderWindowInteractor::OnLButtonDown(void *wnd, int nFlags, 
                                                   int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  //DOQUARTZSetCapture(wnd);
  //DOQUARTZthis->InteractorStyle->OnLeftButtonDown(nFlags & MK_CONTROL, 
           //                               nFlags & MK_SHIFT, 
            //                              X, this->Size[1] - Y - 1);
}

void vtkQuartzRenderWindowInteractor::OnLButtonUp(void *wnd, int nFlags, 
                                                 int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  //DOQUARTZthis->InteractorStyle->OnLeftButtonUp(nFlags & MK_CONTROL, 
            //                            nFlags & MK_SHIFT, 
              //                          X, this->Size[1] - Y - 1);
  //ReleaseCapture( );
}

void vtkQuartzRenderWindowInteractor::OnMButtonDown(void *wnd, int nFlags, 
                                                   int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  //DOQUARTZSetCapture(wnd);
  //DOQUARTZthis->InteractorStyle->OnMiddleButtonDown(nFlags & MK_CONTROL, 
            //                                nFlags & MK_SHIFT, 
              //                              X, this->Size[1] - Y - 1);
}

void vtkQuartzRenderWindowInteractor::OnMButtonUp(void *wnd, int nFlags, 
                                                 int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  //DOQUARTZthis->InteractorStyle->OnMiddleButtonUp(nFlags & MK_CONTROL, 
            //                              nFlags & MK_SHIFT, 
              //                            X, this->Size[1] - Y - 1);
  //ReleaseCapture( );
}

void vtkQuartzRenderWindowInteractor::OnRButtonDown(void *wnd, int nFlags, 
                                                   int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  //DOQUARTZSetCapture(wnd );
  //DOQUARTZthis->InteractorStyle->OnRightButtonDown(nFlags & MK_CONTROL, 
            //                               nFlags & MK_SHIFT, 
             //                              X, this->Size[1] - Y - 1);
}

void vtkQuartzRenderWindowInteractor::OnRButtonUp(void *wnd, int nFlags, 
                                                 int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  //DOQUARTZthis->InteractorStyle->OnRightButtonUp(nFlags & MK_CONTROL, 
            //                             nFlags & MK_SHIFT, 
              //                           X, this->Size[1] - Y - 1);
  //ReleaseCapture( );
}

void vtkQuartzRenderWindowInteractor::OnSize(void *wnd, int nType, int X, int Y) {
  this->UpdateSize(X,Y);
}

void vtkQuartzRenderWindowInteractor::OnTimer(void *wnd, int nIDEvent) 
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InteractorStyle->OnTimer();
}

void vtkQuartzRenderWindowInteractor::OnChar(void *wnd, int nChar, 
                                            int nRepCnt, int nFlags) 
{
  if (!this->Enabled) 
    {
    return;
    }
  //bool ctrl  = GetKeyState(VK_CONTROL);
  //bool shift = GetKeyState(VK_SHIFT);
  //this->InteractorStyle->OnChar(ctrl, shift, (char)nChar, nRepCnt);
}


// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void
vtkQuartzRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
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
