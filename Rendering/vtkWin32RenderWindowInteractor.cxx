/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32RenderWindowInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vtkWin32OpenGLRenderWindow.h"

// MSVC does the right thing without the forward declaration when it 
// sees it in the friend decl in vtkWin32RenderWindowInteractor, but
// GCC needs to see the declaration beforehand. It has to do with the
// CALLBACK attribute.
VTK_RENDERING_EXPORT LRESULT CALLBACK vtkHandleMessage(HWND,UINT,WPARAM,LPARAM);
VTK_RENDERING_EXPORT LRESULT CALLBACK vtkHandleMessage2(HWND,UINT,WPARAM,LPARAM,class vtkWin32RenderWindowInteractor*);

#include "vtkWin32RenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkActor.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#include <GL/gl.h>
#endif
#include "vtkObjectFactory.h"
#include "vtkCommand.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
//----------------------------------------------------------------------------
vtkWin32RenderWindowInteractor* vtkWin32RenderWindowInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32RenderWindowInteractor");
  if(ret)
    {
    return (vtkWin32RenderWindowInteractor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32RenderWindowInteractor;
}
#endif



void (*vtkWin32RenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkWin32RenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

// Construct object so that light follows camera motion.
vtkWin32RenderWindowInteractor::vtkWin32RenderWindowInteractor() 
{
  static int timerId           = 1;
  this->WindowId           = 0;
  this->TimerId            = timerId++;
  this->InstallMessageProc = 1;
  this->MouseInWindow = 0;
}

vtkWin32RenderWindowInteractor::~vtkWin32RenderWindowInteractor() 
{
  vtkWin32OpenGLRenderWindow *tmp;

  // we need to release any hold we have on a windows event loop
  if (this->WindowId && this->Enabled && this->InstallMessageProc) 
    {
    vtkWin32OpenGLRenderWindow *ren;
    ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
    tmp = (vtkWin32OpenGLRenderWindow *)GetWindowLong(this->WindowId,4);
    // watch for odd conditions
    if ((tmp != ren) && (ren != NULL)) 
      {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc,this->WindowId,WM_USER+14,28,(LONG)this->OldProc);
      }
    else 
      {
      SetWindowLong(this->WindowId,GWL_WNDPROC,(LONG)this->OldProc);
      }
    this->Enabled = 0;
    }
}

void  vtkWin32RenderWindowInteractor::Start() 
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent))
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    return;
    }

  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc) 
    {
    return;
    }
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) 
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
}

// Begin processing keyboard strokes.
void vtkWin32RenderWindowInteractor::Initialize() 
{
  static int any_initialized = 0;
  vtkWin32OpenGLRenderWindow *ren;
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
  ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
  ren->Start();
  size    = ren->GetSize();
  position= ren->GetPosition();
  this->WindowId = ren->GetWindowId();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

void vtkWin32RenderWindowInteractor::Enable() 
{
  vtkWin32OpenGLRenderWindow *ren;
  vtkWin32OpenGLRenderWindow *tmp;
  if (this->Enabled) 
    {
    return;
    }
  if (this->InstallMessageProc) 
    {
    // add our callback
    ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
    this->OldProc = (WNDPROC)GetWindowLong(this->WindowId,GWL_WNDPROC);
    tmp=(vtkWin32OpenGLRenderWindow *)GetWindowLong(this->WindowId,4);
    // watch for odd conditions
    if (tmp != ren) 
      {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc,this->WindowId,WM_USER+12,24,(LONG)vtkHandleMessage);
      }
    else 
      {
      SetWindowLong(this->WindowId,GWL_WNDPROC,(LONG)vtkHandleMessage);
      }
    // in case the size of the window has changed while we were away
    int *size;
    size = ren->GetSize();
    this->Size[0] = size[0];
    this->Size[1] = size[1];
    }
  this->Enabled = 1;
  this->Modified();
}


void vtkWin32RenderWindowInteractor::Disable() 
{
  vtkWin32OpenGLRenderWindow *tmp;
  if (!this->Enabled) 
    {
    return;
    }
  
  if (this->InstallMessageProc && this->Enabled && this->WindowId) 
    {
    // we need to release any hold we have on a windows event loop
    vtkWin32OpenGLRenderWindow *ren;
    ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
    tmp = (vtkWin32OpenGLRenderWindow *)GetWindowLong(this->WindowId,4);
    // watch for odd conditions
    if ((tmp != ren) && (ren != NULL)) 
      {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc,this->WindowId,WM_USER+14,28,(LONG)this->OldProc);
      }
    else 
      {
      SetWindowLong(this->WindowId,GWL_WNDPROC,(LONG)this->OldProc);
      }
    }
  this->Enabled = 0;
  this->Modified();
}

void vtkWin32RenderWindowInteractor::TerminateApp(void) 
{
  PostQuitMessage(0);
}

int vtkWin32RenderWindowInteractor::CreateTimer(int timertype) 
{
  if (timertype==VTKI_TIMER_FIRST) 
    {
    return SetTimer(this->WindowId,this->TimerId,10,NULL);
    }
  return 1;
}

int vtkWin32RenderWindowInteractor::DestroyTimer(void) 
{
  return KillTimer(this->WindowId,this->TimerId);
}

//-------------------------------------------------------------
// Virtual Key Code to Unix KeySym Conversion
//-------------------------------------------------------------

// this ascii code to keysym table is meant to mimic Tk

static char *AsciiToKeySymTable[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  "space", "exclam", "quotedbl", "numbersign", 
         "dollar", "percent", "ampersand", "quoteright", 
  "parenleft", "parenright", "asterisk", "plus", 
         "comma", "minus", "period", "slash",
  "0", "1", "2", "3", "4", "5", "6", "7", 
  "8", "9", "colon", "semicolon", "less", "equal", "greater", "question",
  "at", "A", "B", "C", "D", "E", "F", "G", 
  "H", "I", "J", "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T", "U", "V", "W",
  "X", "Y", "Z", "bracketleft", 
         "backslash", "bracketright", "asciicircum", "underscore",
  "quoteleft", "a", "b", "c", "d", "e", "f", "g",
  "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w",
  "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Delete",
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  
// this virtual key code to keysym table is meant to mimic Tk

static char *VKeyCodeToKeySymTable[] = {
  0, 0, 0, "Cancel", 0, 0, 0, 0,
  "BackSpace", "Tab", 0, 0, "Clear", "Return", 0, 0,
  "Shift_L", "Control_L", "Alt_L", "Pause", "Caps_Lock", 0,0,0, 
  0, 0, 0, "Escape", 0, 0, 0, 0,
  "space", "Prior", "Next", "End", "Home", "Left", "Up", "Right",
  "Down", "Select", 0, "Execute", "Snapshot", "Insert", "Delete", "Help",
  "0", "1", "2", "3", "4", "5", "6", "7", 
  "8", "9", 0, 0, 0, 0, 0, 0,
  0, "a", "b", "c", "d", "e", "f", "g",
  "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w",
  "x", "y", "z", "Win_L", "Win_R", "App", 0, 0,
  "KP_0", "KP_1", "KP_2", "KP_3", "KP_4", "KP_5", "KP_6", "KP_7",
  "KP_8", "KP_9", "asterisk", "plus", "bar", "minus", "period", "slash",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
  "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", 
  "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24",
  0, 0, 0, 0, 0, 0, 0, 0,
  "Num_Lock", "Scroll_Lock", 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//-------------------------------------------------------------
// Event loop handlers
//-------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnMouseMove(HWND wnd, UINT nFlags, 
                                                 int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }

  if (!this->MouseInWindow && 
      (X >= 0 && X < this->Size[0] && Y >= 0 && Y < this->Size[1]))
    {
    this->InteractorStyle->OnEnter(nFlags & MK_CONTROL, nFlags & MK_SHIFT,
				   X, this->Size[1] - Y - 1);
    this->MouseInWindow = 1;
    }
 
  if (this->MouseInWindow && 
      (X < 0 || X >= this->Size[0] || Y < 0 || Y >= this->Size[1]))
    {
    this->InteractorStyle->OnLeave(nFlags & MK_CONTROL, nFlags & MK_SHIFT,
				   X, this->Size[1] - Y - 1);
    this->MouseInWindow = 0;
    }

  this->InteractorStyle->OnMouseMove(nFlags & MK_CONTROL, nFlags & MK_SHIFT, 
				     X, this->Size[1] - Y - 1);

}

void vtkWin32RenderWindowInteractor::OnNCMouseMove(HWND wnd, UINT nFlags, 
						   int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }

  int *pos = this->RenderWindow->GetPosition();

  if (this->MouseInWindow)
    {
    this->InteractorStyle->OnLeave(nFlags & MK_CONTROL, nFlags & MK_SHIFT,
				   X-pos[0], this->Size[1] - (Y-pos[1]) - 1);
    this->MouseInWindow = 0;
    }
}

void vtkWin32RenderWindowInteractor::OnLButtonDown(HWND wnd,UINT nFlags, 
                                                   int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  SetCapture(wnd);
  this->InteractorStyle->OnLeftButtonDown(nFlags & MK_CONTROL, 
                                          nFlags & MK_SHIFT, 
                                          X, this->Size[1] - Y - 1);
}

void vtkWin32RenderWindowInteractor::OnLButtonUp(HWND wnd,UINT nFlags, 
                                                 int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InteractorStyle->OnLeftButtonUp(nFlags & MK_CONTROL, 
                                        nFlags & MK_SHIFT, 
                                        X, this->Size[1] - Y - 1);
  ReleaseCapture( );
}

void vtkWin32RenderWindowInteractor::OnMButtonDown(HWND wnd,UINT nFlags, 
                                                   int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  SetCapture(wnd);
  this->InteractorStyle->OnMiddleButtonDown(nFlags & MK_CONTROL, 
                                            nFlags & MK_SHIFT, 
                                            X, this->Size[1] - Y - 1);
}

void vtkWin32RenderWindowInteractor::OnMButtonUp(HWND wnd,UINT nFlags, 
                                                 int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InteractorStyle->OnMiddleButtonUp(nFlags & MK_CONTROL, 
                                          nFlags & MK_SHIFT, 
                                          X, this->Size[1] - Y - 1);
  ReleaseCapture( );
}

void vtkWin32RenderWindowInteractor::OnRButtonDown(HWND wnd,UINT nFlags, 
                                                   int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  SetCapture(wnd );
  this->InteractorStyle->OnRightButtonDown(nFlags & MK_CONTROL, 
                                           nFlags & MK_SHIFT, 
                                           X, this->Size[1] - Y - 1);
}

void vtkWin32RenderWindowInteractor::OnRButtonUp(HWND wnd,UINT nFlags, 
                                                 int X, int Y) 
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InteractorStyle->OnRightButtonUp(nFlags & MK_CONTROL, 
                                         nFlags & MK_SHIFT, 
                                         X, this->Size[1] - Y - 1);
  ReleaseCapture( );
}

void vtkWin32RenderWindowInteractor::OnSize(HWND wnd,UINT nType, int X, int Y) {
  this->UpdateSize(X,Y);
  if (this->Enabled)
    {
    this->InteractorStyle->OnConfigure(X,Y);
    }
}

void vtkWin32RenderWindowInteractor::OnTimer(HWND wnd,UINT nIDEvent) 
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InteractorStyle->OnTimer();
}

void vtkWin32RenderWindowInteractor::OnKeyDown(HWND wnd, UINT vCode, UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
    {
    return;
    }
  int ctrl  = GetKeyState(VK_CONTROL) & (~1);
  int shift = GetKeyState(VK_SHIFT) & (~1);
  WORD nChar = 0;
  {
    BYTE keyState[256];
    GetKeyboardState(keyState);
    if (ToAscii(vCode,nFlags & 0xff,keyState,&nChar,0) == 0)
      {
      nChar = 0;
      }
  } 
  char *keysym = AsciiToKeySymTable[(unsigned char)nChar];
  if (keysym == 0)
    {
    keysym = VKeyCodeToKeySymTable[(unsigned char)vCode];
    }
  if (keysym == 0)
    {
    keysym = "None";
    }
  this->InteractorStyle->OnKeyDown(ctrl, shift, (char)nChar, nRepCnt);
  this->InteractorStyle->OnKeyPress(ctrl, shift, (char)nChar, 
				    keysym, nRepCnt);
}

void vtkWin32RenderWindowInteractor::OnKeyUp(HWND wnd, UINT vCode, UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
    {
    return;
    }
  int ctrl  = GetKeyState(VK_CONTROL) & (~1);
  int shift = GetKeyState(VK_SHIFT) & (~1);
  WORD nChar = 0;
  {
    BYTE keyState[256];
    GetKeyboardState(keyState);
    if (ToAscii(vCode,nFlags & 0xff,keyState,&nChar,0) == 0)
      {
      nChar = 0;
      }
  }
  char *keysym = AsciiToKeySymTable[(unsigned char)nChar];
  if (keysym == 0)
    {
    keysym = VKeyCodeToKeySymTable[(unsigned char)vCode];
    }
  if (keysym == 0)
    {
    keysym = "None";
    }
  this->InteractorStyle->OnKeyUp(ctrl, shift, (char)nChar, nRepCnt);
  this->InteractorStyle->OnKeyRelease(ctrl, shift, (char)nChar, 
				      keysym, nRepCnt);
}

void vtkWin32RenderWindowInteractor::OnChar(HWND wnd,UINT nChar,
                                            UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
    {
    return;
    }
  int ctrl  = GetKeyState(VK_CONTROL) & (~1);
  int shift = GetKeyState(VK_SHIFT) & (~1);
  this->InteractorStyle->OnChar(ctrl, shift, (char)nChar, nRepCnt);
}

// This is only called when InstallMessageProc is true
LRESULT CALLBACK vtkHandleMessage(HWND hWnd,UINT uMsg, WPARAM wParam, 
                                  LPARAM lParam) 
{
  vtkWin32OpenGLRenderWindow *ren;
  vtkWin32RenderWindowInteractor *me;
  
  ren = (vtkWin32OpenGLRenderWindow *)GetWindowLong(hWnd,4);
  if (ren == NULL) 
    { 
    return 0; 
    }
  
  me = (vtkWin32RenderWindowInteractor *)ren->GetInteractor();
  
  if (me == NULL) 
    { 
    return 0; 
    }
  return vtkHandleMessage2(hWnd,uMsg,wParam, lParam, me);
}

LRESULT CALLBACK vtkHandleMessage2(HWND hWnd,UINT uMsg, WPARAM wParam, 
                                   LPARAM lParam, 
                                   vtkWin32RenderWindowInteractor *me) 
{
  if ((uMsg == WM_USER+13)&&(wParam == 26)) 
    {
    // someone is telling us to set our OldProc
    me->OldProc = (WNDPROC)lParam;
    return 1;
    }
  
  switch (uMsg) 
    {
    case WM_PAINT:
      me->GetRenderWindow()->Render();
      return CallWindowProc(me->OldProc,hWnd,uMsg,wParam,lParam);
      break;
      
    case WM_SIZE:
      me->OnSize(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      return CallWindowProc(me->OldProc,hWnd,uMsg,wParam,lParam);
      break;
      
    case WM_LBUTTONDOWN:
      me->OnLButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;
      
    case WM_LBUTTONUP:
      me->OnLButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;
      
    case WM_MBUTTONDOWN:
      me->OnMButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;
      
    case WM_MBUTTONUP:
      me->OnMButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;
      
    case WM_RBUTTONDOWN:
      me->OnRButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;
      
    case WM_RBUTTONUP:
      me->OnRButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;
      
    case WM_MOUSEMOVE:
      me->OnMouseMove(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_NCMOUSEMOVE:
      me->OnNCMouseMove(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_CLOSE:
      // Don't know what to put here ! Why so many callbacks ?
      if (me->HasObserver(vtkCommand::ExitEvent)) 
        {
        me->InvokeEvent(vtkCommand::ExitEvent,NULL);
        }
      else if (me->ClassExitMethod) 
        {
        (*me->ClassExitMethod)(me->ClassExitMethodArg);
        }
      else 
        {
        me->TerminateApp();
        }
      break;
      
    case WM_CHAR:
      me->OnChar(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;
      
    case WM_KEYDOWN:
      me->OnKeyDown(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_KEYUP:
      me->OnKeyUp(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_TIMER:
      me->OnTimer(hWnd,wParam);
    
      break;
    default:
      if (me) 
        {
        return CallWindowProc(me->OldProc,hWnd,uMsg,wParam,lParam);
        }
    };
  return 0;
}


// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void
vtkWin32RenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkWin32RenderWindowInteractor::ClassExitMethod
       || arg != vtkWin32RenderWindowInteractor::ClassExitMethodArg)
    {
    // delete the current arg if there is a delete method
    if ((vtkWin32RenderWindowInteractor::ClassExitMethodArg)
        && (vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete))
      {
      (*vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkWin32RenderWindowInteractor::ClassExitMethodArg);
      }
    vtkWin32RenderWindowInteractor::ClassExitMethod = f;
    vtkWin32RenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
    }
}


// Set the arg delete method.  This is used to free user memory.
void
vtkWin32RenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)
    {
    vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
    }
}

void vtkWin32RenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRenderWindowInteractor::PrintSelf(os,indent);
  os << indent << "InstallMessageProc: " << this->InstallMessageProc << endl;
}

void vtkWin32RenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent)) 
    {
    this->InvokeEvent(vtkCommand::ExitEvent,NULL);
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


