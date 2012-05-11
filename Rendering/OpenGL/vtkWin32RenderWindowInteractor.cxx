/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32RenderWindowInteractor.cxx

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


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400  // for trackmouseevent support  requires Win95 with IE 3.0 or greater.
#endif

#include "vtkWin32OpenGLRenderWindow.h"

// Mouse wheel support
// In an ideal world we would just have to include <zmouse.h>, but it is not
// always available with all compilers/headers
#ifndef WM_MOUSEWHEEL
#  define WM_MOUSEWHEEL                   0x020A
#endif  //WM_MOUSEWHEEL
#ifndef GET_WHEEL_DELTA_WPARAM
#  define GET_WHEEL_DELTA_WPARAM(wparam) ((short)HIWORD (wparam))
#endif  //GET_WHEEL_DELTA_WPARAM

// MSVC does the right thing without the forward declaration when it
// sees it in the friend decl in vtkWin32RenderWindowInteractor, but
// GCC needs to see the declaration beforehand. It has to do with the
// CALLBACK attribute.
VTKRENDERINGOPENGL_EXPORT LRESULT CALLBACK vtkHandleMessage(HWND,UINT,WPARAM,LPARAM);
VTKRENDERINGOPENGL_EXPORT LRESULT CALLBACK vtkHandleMessage2(HWND,UINT,WPARAM,LPARAM,class vtkWin32RenderWindowInteractor*);

#include "vtkWin32RenderWindowInteractor.h"
#include "vtkActor.h"
#include "vtkOpenGL.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#ifdef VTK_USE_TDX
#include "vtkTDxWinDevice.h"
#endif

vtkStandardNewMacro(vtkWin32RenderWindowInteractor);

void (*vtkWin32RenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkWin32RenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkWin32RenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

//----------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkWin32RenderWindowInteractor::vtkWin32RenderWindowInteractor()
{
  this->WindowId           = 0;
  this->InstallMessageProc = 1;
  this->MouseInWindow = 0;
  this->StartedMessageLoop = 0;

#ifdef VTK_USE_TDX
  this->Device=vtkTDxWinDevice::New();
#endif
}

//----------------------------------------------------------------------------
vtkWin32RenderWindowInteractor::~vtkWin32RenderWindowInteractor()
{
  vtkWin32OpenGLRenderWindow *tmp;

  // we need to release any hold we have on a windows event loop
  if (this->WindowId && this->Enabled && this->InstallMessageProc)
    {
    vtkWin32OpenGLRenderWindow *ren;
    ren = static_cast<vtkWin32OpenGLRenderWindow *>(this->RenderWindow);
    tmp = (vtkWin32OpenGLRenderWindow *)(vtkGetWindowLong(this->WindowId,sizeof(vtkLONG)));
    // watch for odd conditions
    if ((tmp != ren) && (ren != NULL))
      {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc,this->WindowId,WM_USER+14,28,(intptr_t)this->OldProc);
      }
    else
      {
      vtkSetWindowLong(this->WindowId,vtkGWL_WNDPROC,(intptr_t)this->OldProc);
      }
    this->Enabled = 0;
    }
#ifdef VTK_USE_TDX
  this->Device->Delete();
#endif
}

//----------------------------------------------------------------------------
void  vtkWin32RenderWindowInteractor::Start()
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent) && !this->HandleEventLoop)
    {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    return;
    }

  // No need to do anything if this is a 'mapped' interactor
  if (!this->Enabled || !this->InstallMessageProc)
    {
    return;
    }

  this->StartedMessageLoop = 1;

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
}

//----------------------------------------------------------------------------
// Begin processing keyboard strokes.
void vtkWin32RenderWindowInteractor::Initialize()
{
  vtkWin32OpenGLRenderWindow *ren;
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
  ren = (vtkWin32OpenGLRenderWindow *)(this->RenderWindow);
  ren->Start();
  size    = ren->GetSize();
  ren->GetPosition();
  this->WindowId = ren->GetWindowId();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
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
    this->OldProc = (WNDPROC)vtkGetWindowLong(this->WindowId,vtkGWL_WNDPROC);
    tmp=(vtkWin32OpenGLRenderWindow *)vtkGetWindowLong(this->WindowId,sizeof(vtkLONG));
    // watch for odd conditions
    if (tmp != ren)
      {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc,this->WindowId,WM_USER+12,24,(intptr_t)vtkHandleMessage);
      }
    else
      {
      vtkSetWindowLong(this->WindowId,vtkGWL_WNDPROC,(intptr_t)vtkHandleMessage);
      }

#ifdef VTK_USE_TDX
    if(this->UseTDx)
      {
      this->Device->SetInteractor(this);
      this->Device->Initialize();
      this->Device->StartListening();
      }
#endif

    // in case the size of the window has changed while we were away
    int *size;
    size = ren->GetSize();
    this->Size[0] = size[0];
    this->Size[1] = size[1];
    }
  this->Enabled = 1;
  this->Modified();
}


//----------------------------------------------------------------------------
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
    tmp = (vtkWin32OpenGLRenderWindow *)vtkGetWindowLong(this->WindowId,sizeof(vtkLONG));
    // watch for odd conditions
    if ((tmp != ren) && (ren != NULL))
      {
      // OK someone else has a hold on our event handler
      // so lets have them handle this stuff
      // well send a USER message to the other
      // event handler so that it can properly
      // call this event handler if required
      CallWindowProc(this->OldProc,this->WindowId,WM_USER+14,28,(intptr_t)this->OldProc);
      }
    else
      {
      vtkSetWindowLong(this->WindowId,vtkGWL_WNDPROC,(intptr_t)this->OldProc);
      }
#ifdef VTK_USE_TDX
    if(this->Device->GetInitialized())
      {
      this->Device->Close();
      }
#endif
    }
  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::TerminateApp(void)
{
  // Only post a quit message if Start was called...
  //
  if (this->StartedMessageLoop)
    {
    PostQuitMessage(0);
    }
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::InternalCreateTimer(int timerId, int vtkNotUsed(timerType),
                                                        unsigned long duration)
{
  // Win32 always creates repeating timers
  SetTimer(this->WindowId,timerId,duration,NULL);
  return timerId;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  return KillTimer(this->WindowId,platformTimerId);
}

//-------------------------------------------------------------
// Virtual Key Code to Unix KeySym Conversion
//-------------------------------------------------------------

// this ascii code to keysym table is meant to mimic Tk

static const char *AsciiToKeySymTable[] = {
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

static const char *VKeyCodeToKeySymTable[] = {
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
void vtkWin32RenderWindowInteractor::OnMouseMove(HWND hWnd, UINT nFlags,
                                                 int X, int Y)
{
  if (!this->Enabled)
    {
    return;
    }

  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  if (!this->MouseInWindow &&
      (X >= 0 && X < this->Size[0] && Y >= 0 && Y < this->Size[1]))
    {
    this->InvokeEvent(vtkCommand::EnterEvent, NULL);
    this->MouseInWindow = 1;
    // request WM_MOUSELEAVE generation
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = hWnd;
    TrackMouseEvent(&tme);
    }

  this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnNCMouseMove(HWND, UINT nFlags,
                                                   int X, int Y)
{
  if (!this->Enabled)
    {
    return;
    }

  int *pos = this->RenderWindow->GetPosition();
  if (this->MouseInWindow)
    {
    this->SetEventInformationFlipY(X - pos[0],
                                   Y - pos[1],
                                   nFlags & MK_CONTROL,
                                   nFlags & MK_SHIFT);
    this->SetAltKey(GetKeyState(VK_MENU) & (~1));
    this->InvokeEvent(vtkCommand::LeaveEvent, NULL);
    this->MouseInWindow = 0;
    }
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnMouseWheelForward(HWND,UINT nFlags,
                                                   int X, int Y)
{
  if (!this->Enabled)
    {
    return;
    }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  this->InvokeEvent(vtkCommand::MouseWheelForwardEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnMouseWheelBackward(HWND,UINT nFlags,
                                                   int X, int Y)
{
  if (!this->Enabled)
    {
    return;
    }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnLButtonDown(HWND wnd,UINT nFlags,
                                                   int X, int Y, int repeat)
{
  if (!this->Enabled)
    {
    return;
    }
  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT,
                                 0, repeat);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  this->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnLButtonUp(HWND,UINT nFlags,
                                                 int X, int Y)
{
  if (!this->Enabled)
    {
    return;
    }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
  ReleaseCapture( );
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnMButtonDown(HWND wnd,UINT nFlags,
                                                   int X, int Y, int repeat)
{
  if (!this->Enabled)
    {
    return;
    }
  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT,
                                 0, repeat);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  this->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnMButtonUp(HWND,UINT nFlags,
                                                 int X, int Y)
{
  if (!this->Enabled)
    {
    return;
    }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent,NULL);
  ReleaseCapture( );
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnRButtonDown(HWND wnd,UINT nFlags,
                                                   int X, int Y, int repeat)
{
  if (!this->Enabled)
    {
    return;
    }
  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT,
                                 0, repeat);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  this->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnRButtonUp(HWND,UINT nFlags,
                                                 int X, int Y)
{
  if (!this->Enabled)
    {
    return;
    }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  this->InvokeEvent(vtkCommand::RightButtonReleaseEvent,NULL);
  ReleaseCapture( );
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnSize(HWND,UINT, int X, int Y) {
  this->UpdateSize(X,Y);
  if (this->Enabled)
    {
    this->InvokeEvent(vtkCommand::ConfigureEvent,NULL);
    }
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnTimer(HWND,UINT timerId)
{
  if (!this->Enabled)
    {
    return;
    }
  int tid = static_cast<int>(timerId);
  this->InvokeEvent(vtkCommand::TimerEvent,(void*)&tid);

  // Here we deal with one-shot versus repeating timers
  if ( this->IsOneShotTimer(tid) )
    {
    KillTimer(this->WindowId,tid); //'cause windows timers are always repeating
    }
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnKeyDown(HWND, UINT vCode, UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
    {
    return;
    }
  int ctrl  = GetKeyState(VK_CONTROL) & (~1);
  int shift = GetKeyState(VK_SHIFT) & (~1);
  int alt = GetKeyState(VK_MENU) & (~1);
  WORD nChar = 0;
  {
#ifndef _WIN32_WCE
    BYTE keyState[256];
    GetKeyboardState(keyState);
    if (ToAscii(vCode,nFlags & 0xff,keyState,&nChar,0) == 0)
      {
      nChar = 0;
      }
#endif
  }
  const char *keysym = AsciiToKeySymTable[(unsigned char)nChar];
  if (keysym == 0)
    {
    keysym = VKeyCodeToKeySymTable[(unsigned char)vCode];
    }
  if (keysym == 0)
    {
    keysym = "None";
    }
  this->SetKeyEventInformation(ctrl,
                               shift,
                               nChar,
                               nRepCnt,
                               keysym);
  this->SetAltKey(alt);
  this->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnKeyUp(HWND, UINT vCode, UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
    {
    return;
    }
  int ctrl  = GetKeyState(VK_CONTROL) & (~1);
  int shift = GetKeyState(VK_SHIFT) & (~1);
  int alt = GetKeyState(VK_MENU) & (~1);
  WORD nChar = 0;
  {
    BYTE keyState[256];
#ifndef _WIN32_WCE
    GetKeyboardState(keyState);
    if (ToAscii(vCode,nFlags & 0xff,keyState,&nChar,0) == 0)
      {
      nChar = 0;
      }
#endif
  }
  const char *keysym = AsciiToKeySymTable[(unsigned char)nChar];
  if (keysym == 0)
    {
    keysym = VKeyCodeToKeySymTable[(unsigned char)vCode];
    }
  if (keysym == 0)
    {
    keysym = "None";
    }
  this->SetKeyEventInformation(ctrl,
                               shift,
                               nChar,
                               nRepCnt,
                               keysym);
  this->SetAltKey(alt);
  this->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnChar(HWND,UINT nChar,
                                            UINT nRepCnt, UINT)
{
  if (!this->Enabled)
    {
    return;
    }
  int ctrl  = GetKeyState(VK_CONTROL) & (~1);
  int shift = GetKeyState(VK_SHIFT) & (~1);
  int alt = GetKeyState(VK_MENU) & (~1);
  this->SetKeyEventInformation(ctrl,
                               shift,
                               nChar,
                               nRepCnt);
  this->SetAltKey(alt);
  this->InvokeEvent(vtkCommand::CharEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnFocus(HWND,UINT)
{
  if (!this->Enabled)
    {
    return;
    }

#ifdef VTK_USE_TDX
  if(this->Device->GetInitialized() && !this->Device->GetIsListening())
    {
    this->Device->StartListening();
    }
#endif
}

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::OnKillFocus(HWND,UINT)
{
  if (!this->Enabled)
    {
    return;
    }
#ifdef VTK_USE_TDX
  if(this->Device->GetInitialized() && this->Device->GetIsListening())
    {
    this->Device->StopListening();
    }
#endif
}

//----------------------------------------------------------------------------
// This is only called when InstallMessageProc is true
LRESULT CALLBACK vtkHandleMessage(HWND hWnd,UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
  LRESULT res = 0;
  vtkWin32OpenGLRenderWindow *ren;
  vtkWin32RenderWindowInteractor *me = 0;

  ren = (vtkWin32OpenGLRenderWindow *)vtkGetWindowLong(hWnd,sizeof(vtkLONG));

  if (ren)
    {
    me = (vtkWin32RenderWindowInteractor *)ren->GetInteractor();
    }

  if (me && me->GetReferenceCount()>0)
    {
    me->Register(me);
    res = vtkHandleMessage2(hWnd,uMsg,wParam,lParam,me);
    me->UnRegister(me);
    }

  return res;
}

#ifndef MAKEPOINTS
#define MAKEPOINTS(l)   (*((POINTS FAR *) & (l)))
#endif

//----------------------------------------------------------------------------
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
      me->Render();
      return CallWindowProc(me->OldProc,hWnd,uMsg,wParam,lParam);
      break;

    case WM_SIZE:
      me->OnSize(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      return CallWindowProc(me->OldProc,hWnd,uMsg,wParam,lParam);
      break;

    case WM_LBUTTONDBLCLK:
      me->OnLButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 1);
      break;

    case WM_LBUTTONDOWN:
      me->OnLButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 0);
      break;

    case WM_LBUTTONUP:
      me->OnLButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_MBUTTONDBLCLK:
      me->OnMButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 1);
      break;

    case WM_MBUTTONDOWN:
      me->OnMButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 0);
      break;

    case WM_MBUTTONUP:
      me->OnMButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_RBUTTONDBLCLK:
      me->OnRButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 1);
      break;

    case WM_RBUTTONDOWN:
      me->OnRButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 0);
      break;

    case WM_RBUTTONUP:
      me->OnRButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_MOUSELEAVE:
      me->InvokeEvent(vtkCommand::LeaveEvent, NULL);
      me->MouseInWindow = 0;
      break;

    case WM_MOUSEMOVE:
      me->OnMouseMove(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_MOUSEWHEEL:
      {
      POINT pt;
      pt.x = MAKEPOINTS(lParam).x;
      pt.y = MAKEPOINTS(lParam).y;
      ::ScreenToClient(hWnd, &pt);
      if( GET_WHEEL_DELTA_WPARAM(wParam) > 0)
        me->OnMouseWheelForward(hWnd,wParam,pt.x,pt.y);
      else
        me->OnMouseWheelBackward(hWnd,wParam,pt.x,pt.y);
      }
      break;

#ifdef WM_MCVMOUSEMOVE
    case WM_NCMOUSEMOVE:
      me->OnNCMouseMove(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;
#endif

    case WM_CLOSE:
      me->ExitCallback();
      break;

    case WM_CHAR:
      me->OnChar(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      me->OnKeyDown(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      me->OnKeyUp(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_TIMER:
      me->OnTimer(hWnd,wParam);
      break;

    case WM_ACTIVATE:
      if(wParam==WA_INACTIVE)
        {
        me->OnKillFocus(hWnd,wParam);
        }
      else
        {
        me->OnFocus(hWnd,wParam);
        }
      break;

    case WM_SETFOCUS:
      // occurs when SetFocus() is called on the current window
      me->OnFocus(hWnd,wParam);
      break;

    case WM_KILLFOCUS:
      // occurs when the focus was on the current window and SetFocus() is
      // called on another window.
      me->OnKillFocus(hWnd,wParam);
      break;

    default:
      if (me)
        {
        return CallWindowProc(me->OldProc,hWnd,uMsg,wParam,lParam);
        }
    };

  return 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkWin32RenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InstallMessageProc: " << this->InstallMessageProc << endl;
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//----------------------------------------------------------------------------
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

  this->TerminateApp();
}
