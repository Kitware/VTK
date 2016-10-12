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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  // for trackmouseevent support, 0x0501 means target Windows XP or later
//#define _WIN32_WINNT 0x0601  // for touch support, 0x0601 means target Windows 7 or later
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
VTKRENDERINGOPENGL2_EXPORT LRESULT CALLBACK vtkHandleMessage(HWND,UINT,WPARAM,LPARAM);
VTKRENDERINGOPENGL2_EXPORT LRESULT CALLBACK vtkHandleMessage2(HWND,UINT,WPARAM,LPARAM,class vtkWin32RenderWindowInteractor*);

#include "vtkWin32RenderWindowInteractor.h"
#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#ifdef VTK_USE_TDX
#include "vtkTDxWinDevice.h"
#endif

// we hard define the touch structures we use since we cannot take them
// from the header without requiring windows 7 (stupid stupid stupid!)
// so we define them and then do a runtime checks and function pointers
// to avoid a link requirement on Windows 7
#define MOUSEEVENTF_FROMTOUCH 0xFF515700
#define WM_TOUCH              0x0240
#define TOUCH_COORD_TO_PIXEL(l)  ((l) / 100)
typedef struct _TOUCHINPUT {
  LONG      x;
  LONG      y;
  HANDLE    hSource;
  DWORD     dwID;
  DWORD     dwFlags;
  DWORD     dwMask;
  DWORD     dwTime;
  ULONG_PTR dwExtraInfo;
  DWORD     cxContact;
  DWORD     cyContact;
} TOUCHINPUT, *PTOUCHINPUT;
DECLARE_HANDLE(HTOUCHINPUT);
//#define HTOUCHINPUT ULONG
#define TOUCHEVENTF_MOVE  0x0001
#define TOUCHEVENTF_DOWN  0x0002
#define TOUCHEVENTF_UP  0x0004
typedef bool (WINAPI *RegisterTouchWindowType)(HWND,ULONG);
typedef bool (WINAPI *GetTouchInputInfoType)(HTOUCHINPUT,UINT,PTOUCHINPUT,int);
typedef bool (WINAPI *CloseTouchInputHandleType)(HTOUCHINPUT);

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
void vtkWin32RenderWindowInteractor::StartEventLoop()
{
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

    // Check for windows multitouch support at runtime
    RegisterTouchWindowType RTW =
      (RegisterTouchWindowType)GetProcAddress(GetModuleHandle(TEXT("user32")), "RegisterTouchWindow");
    if(RTW != NULL)
    {
      RTW(this->WindowId, 0);
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
int vtkWin32RenderWindowInteractor::OnMouseMove(HWND hWnd, UINT nFlags,
                                                int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }

  // touch events handled by WM_TOUCH
  if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH)
  {
    return 0;
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

  return this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnNCMouseMove(HWND, UINT nFlags,
                                                  int X, int Y)
{
  if (!this->Enabled || !this->MouseInWindow)
  {
    return 0;
  }

  int *pos = this->RenderWindow->GetPosition();
  this->SetEventInformationFlipY(X - pos[0],
                                 Y - pos[1],
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  const int ret = this->InvokeEvent(vtkCommand::LeaveEvent, NULL);
  this->MouseInWindow = 0;
  return ret;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMouseWheelForward(HWND, UINT nFlags,
                                                        int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  return this->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMouseWheelBackward(HWND, UINT nFlags,
                                                         int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  return this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnLButtonDown(HWND wnd, UINT nFlags,
                                                  int X, int Y, int repeat)
{
  if (!this->Enabled)
  {
    return 0;
  }

  // touch events handled by WM_TOUCH
  if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH)
  {
    return 0;
  }

  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT,
                                 0, repeat);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  return this->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnLButtonUp(HWND, UINT nFlags,
                                                int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  // touch events handled by WM_TOUCH
  if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  const int ret = this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
  ReleaseCapture( );
  return ret;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMButtonDown(HWND wnd, UINT nFlags,
                                                  int X, int Y, int repeat)
{
  if (!this->Enabled)
  {
    return 0;
  }
  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT,
                                 0, repeat);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  return this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnMButtonUp(HWND, UINT nFlags,
                                                int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  const int ret = this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent,NULL);
  ReleaseCapture( );
  return ret;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnRButtonDown(HWND wnd, UINT nFlags,
                                                  int X, int Y, int repeat)
{
  if (!this->Enabled)
  {
    return 0;
  }
  SetFocus(wnd);
  SetCapture(wnd);
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT,
                                 0, repeat);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  return this->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnRButtonUp(HWND, UINT nFlags,
                                                int X, int Y)
{
  if (!this->Enabled)
  {
    return 0;
  }
  this->SetEventInformationFlipY(X,
                                 Y,
                                 nFlags & MK_CONTROL,
                                 nFlags & MK_SHIFT);
  this->SetAltKey(GetKeyState(VK_MENU) & (~1));
  const int ret = this->InvokeEvent(vtkCommand::RightButtonReleaseEvent,NULL);
  ReleaseCapture( );
  return ret;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnSize(HWND, UINT, int X, int Y)
{
  this->UpdateSize(X,Y);
  if (this->Enabled)
  {
     return this->InvokeEvent(vtkCommand::ConfigureEvent, NULL);
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnTimer(HWND, UINT timerId)
{
  if (!this->Enabled)
  {
    return 0;
  }
  int tid = static_cast<int>(timerId);
  const int ret = this->InvokeEvent(vtkCommand::TimerEvent,(void*)&tid);

  // Here we deal with one-shot versus repeating timers
  if ( this->IsOneShotTimer(tid) )
  {
    KillTimer(this->WindowId,tid); //'cause windows timers are always repeating
  }

  return ret;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnKeyDown(HWND, UINT vCode, UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
  {
    return 0;
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
  return this->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnKeyUp(HWND, UINT vCode, UINT nRepCnt, UINT nFlags)
{
  if (!this->Enabled)
  {
    return 0;
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
  return this->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnChar(HWND, UINT nChar,
                                           UINT nRepCnt, UINT)
{
  if (!this->Enabled)
  {
    return 0;
  }
  int ctrl  = GetKeyState(VK_CONTROL) & (~1);
  int shift = GetKeyState(VK_SHIFT) & (~1);
  int alt = GetKeyState(VK_MENU) & (~1);
  this->SetKeyEventInformation(ctrl,
                               shift,
                               nChar,
                               nRepCnt);
  this->SetAltKey(alt);
  return this->InvokeEvent(vtkCommand::CharEvent, NULL);
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnFocus(HWND, UINT)
{
  if (!this->Enabled)
  {
    return 0;
  }

#ifdef VTK_USE_TDX
  if(this->Device->GetInitialized() && !this->Device->GetIsListening())
  {
    this->Device->StartListening();
    return 1;
  }
#endif

  return 0;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnKillFocus(HWND, UINT)
{
  if (!this->Enabled)
  {
    return 0;
  }
#ifdef VTK_USE_TDX
  if(this->Device->GetInitialized() && this->Device->GetIsListening())
  {
    this->Device->StopListening();
    return 1;
  }
#endif

  return 0;
}

//----------------------------------------------------------------------------
int vtkWin32RenderWindowInteractor::OnTouch(HWND hWnd, UINT wParam, UINT lParam)
{
  if (!this->Enabled)
  {
    return 0;
  }

  int ret(0);
  UINT cInputs = LOWORD(wParam);
  PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
  if (pInputs)
  {
    int ctrl  = GetKeyState(VK_CONTROL) & (~1);
    int shift = GetKeyState(VK_SHIFT) & (~1);
    this->SetAltKey(GetKeyState(VK_MENU) & (~1));
    GetTouchInputInfoType GTII =
      (GetTouchInputInfoType)GetProcAddress(GetModuleHandle(TEXT("user32")), "GetTouchInputInfo");
    if (GTII((HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT)))
    {
      POINT ptInput;
      for (UINT i=0; i < cInputs; i++)
      {
        TOUCHINPUT ti = pInputs[i];
        int index = this->GetPointerIndexForContact(ti.dwID);
        if (ti.dwID != 0 && index < VTKI_MAX_POINTERS)
        {
            // Do something with your touch input handle
            ptInput.x = TOUCH_COORD_TO_PIXEL(ti.x);
            ptInput.y = TOUCH_COORD_TO_PIXEL(ti.y);
            ScreenToClient(hWnd, &ptInput);
            this->SetEventInformationFlipY(ptInput.x,
                                           ptInput.y,
                                           ctrl,
                                           shift,
                                           0,0,0,
                                           index);
        }
      }
      bool didUpOrDown = false;
      for (UINT i=0; i < cInputs; i++)
      {
        TOUCHINPUT ti = pInputs[i];
        int index = this->GetPointerIndexForContact(ti.dwID);
        if (ti.dwID != 0 && index < VTKI_MAX_POINTERS)
        {
          if (ti.dwFlags & TOUCHEVENTF_UP)
          {
            this->SetPointerIndex(index);
            didUpOrDown = true;
            this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
            this->ClearPointerIndex(index);
          }
          if (ti.dwFlags & TOUCHEVENTF_DOWN)
          {
            this->SetPointerIndex(index);
            didUpOrDown = true;
            this->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
          }
          this->SetPointerIndex(index);
        }
      }
      if (!didUpOrDown)
      {
        ret = this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
      }
      else
      {
        ret = 1;
      }
    }
    CloseTouchInputHandleType CTIH =
      (CloseTouchInputHandleType)GetProcAddress(GetModuleHandle(TEXT("user32")), "CloseTouchInputHandle");
    CTIH((HTOUCHINPUT)lParam);
    delete [] pInputs;
  }

  return ret;
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

  int handled(0);

  switch (uMsg)
  {
    case WM_PAINT:
    {
      const LRESULT ret(CallWindowProc(me->OldProc, hWnd, uMsg, wParam, lParam));
      me->InvokeEvent(vtkCommand::RenderEvent, NULL);
      return ret;
    }

    case WM_SIZE:
      handled = me->OnSize(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_LBUTTONDBLCLK:
      handled = me->OnLButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 1);
      break;

    case WM_LBUTTONDOWN:
      handled = me->OnLButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 0);
      break;

    case WM_LBUTTONUP:
      handled = me->OnLButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_MBUTTONDBLCLK:
      handled = me->OnMButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 1);
      break;

    case WM_MBUTTONDOWN:
      handled = me->OnMButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 0);
      break;

    case WM_MBUTTONUP:
      handled = me->OnMButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_RBUTTONDBLCLK:
      handled = me->OnRButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 1);
      break;

    case WM_RBUTTONDOWN:
      handled = me->OnRButtonDown(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y, 0);
      break;

    case WM_RBUTTONUP:
      handled = me->OnRButtonUp(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_MOUSELEAVE:
      me->InvokeEvent(vtkCommand::LeaveEvent, NULL);
      me->MouseInWindow = 0;
      break;

    case WM_MOUSEMOVE:
      handled = me->OnMouseMove(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;

    case WM_MOUSEWHEEL:
    {
      POINT pt;
      pt.x = MAKEPOINTS(lParam).x;
      pt.y = MAKEPOINTS(lParam).y;
      ::ScreenToClient(hWnd, &pt);
      if( GET_WHEEL_DELTA_WPARAM(wParam) > 0)
        handled = me->OnMouseWheelForward(hWnd,wParam,pt.x,pt.y);
      else
        handled = me->OnMouseWheelBackward(hWnd,wParam,pt.x,pt.y);
    }
      break;

#ifdef WM_MCVMOUSEMOVE
    case WM_NCMOUSEMOVE:
      handled = me->OnNCMouseMove(hWnd,wParam,MAKEPOINTS(lParam).x,MAKEPOINTS(lParam).y);
      break;
#endif

    case WM_CLOSE:
      me->ExitCallback();
      break;

    case WM_CHAR:
      handled = me->OnChar(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      handled = me->OnKeyDown(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      handled = me->OnKeyUp(hWnd,wParam,LOWORD(lParam),HIWORD(lParam));
      break;

    case WM_TIMER:
      handled = me->OnTimer(hWnd,wParam);
      break;

    case WM_ACTIVATE:
      if(wParam==WA_INACTIVE)
      {
        handled = me->OnKillFocus(hWnd,wParam);
      }
      else
      {
        handled = me->OnFocus(hWnd,wParam);
      }
      break;

    case WM_SETFOCUS:
      // occurs when SetFocus() is called on the current window
      handled = me->OnFocus(hWnd,wParam);
      break;

    case WM_KILLFOCUS:
      // occurs when the focus was on the current window and SetFocus() is
      // called on another window.
      handled = me->OnKillFocus(hWnd,wParam);
      break;

    case WM_TOUCH:
      handled = me->OnTouch(hWnd,wParam,lParam);
      break;

    default:
      break;
  };

  if (0 == handled)
  {
    return CallWindowProc(me->OldProc, hWnd, uMsg, wParam, lParam);
  }

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
