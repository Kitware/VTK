/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32RenderWindowInteractor.h
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
// .NAME vtkWin32RenderWindowInteractor - implements Win32 specific functions
// required by vtkRenderWindowInteractor.
//
// .SECTION Description
// By default the interactor installs a MessageProc callback which
// intercepts windows' messages to the window and controls interactions by
// routing them to the InteractoStyle classes.
// MFC or BCB programs can prevent this and instead directly route any mouse/key
// messages into the event bindings by setting InstallMessageProc to false.
// This provides a minimal "Mapped" mode of interaction
//
#ifndef __vtkWin32RenderWindowInteractor_h
#define __vtkWin32RenderWindowInteractor_h

#include <stdlib.h>
#include "vtkRenderWindowInteractor.h"


class VTK_RENDERING_EXPORT vtkWin32RenderWindowInteractor : public vtkRenderWindowInteractor 
{
public:
  // Description:
  // Construct object so that light follows camera motion.
  static vtkWin32RenderWindowInteractor *New();

  vtkTypeMacro(vtkWin32RenderWindowInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the event handler
  virtual void Initialize();

  // Description:
  // Enable/Disable interactions.  By default interactors are enabled when
  // initialized.  Initialize() must be called prior to enabling/disabling
  // interaction. These methods are used when a window/widget is being
  // shared by multiple renderers and interactors.  This allows a "modal"
  // display where one interactor is active when its data is to be displayed
  // and all other interactors associated with the widget are disabled
  // when their data is not displayed.
  virtual void Enable();
  virtual void Disable();

  // Description:
  // This will start up the event loop and never return. If you
  // call this method it will loop processing events until the
  // application is exited.
  virtual void Start();

  // Description:
  // By default the interactor installs a MessageProc callback which
  // intercepts windows messages to the window and controls interactions.
  // MFC or BCB programs can prevent this and instead directly route any mouse/key
  // messages into the event bindings by setting InstallMessgeProc to false.
  vtkSetMacro(InstallMessageProc,int);
  vtkGetMacro(InstallMessageProc,int);
  vtkBooleanMacro(InstallMessageProc,int);

  // Description:
  // Win32 specific application terminate, calls ClassExitMethod then
  // calls PostQuitMessage(0) to terminate the application. An application can Specify
  // ExitMethod for alternative behavior (i.e. suppression of keyboard exit)
  void TerminateApp(void);

  // Description:
  // Win32 timer methods
  int CreateTimer(int timertype);
  int DestroyTimer(void);

  //BTX
  friend VTK_RENDERING_EXPORT LRESULT CALLBACK vtkHandleMessage(HWND hwnd,UINT uMsg, WPARAM w, LPARAM l);
  friend VTK_RENDERING_EXPORT LRESULT CALLBACK vtkHandleMessage2(HWND hwnd,UINT uMsg, WPARAM w, LPARAM l, vtkWin32RenderWindowInteractor *me);

  // Description:
  // Various methods that a Win32 window can redirect to this class to be 
  // handled.
  virtual void OnMouseMove  (HWND wnd, UINT nFlags, int X, int Y);
  virtual void OnNCMouseMove(HWND wnd, UINT nFlags, int X, int Y);
  virtual void OnRButtonDown(HWND wnd, UINT nFlags, int X, int Y);
  virtual void OnRButtonUp  (HWND wnd, UINT nFlags, int X, int Y);
  virtual void OnMButtonDown(HWND wnd, UINT nFlags, int X, int Y);
  virtual void OnMButtonUp  (HWND wnd, UINT nFlags, int X, int Y);
  virtual void OnLButtonDown(HWND wnd, UINT nFlags, int X, int Y);
  virtual void OnLButtonUp  (HWND wnd, UINT nFlags, int X, int Y);
  virtual void OnSize       (HWND wnd, UINT nType,  int X, int Y);
  virtual void OnTimer      (HWND wnd, UINT nIDEvent);
  virtual void OnKeyDown    (HWND wnd, UINT nChar, UINT nRepCnt, UINT nFlags);
  virtual void OnKeyUp      (HWND wnd, UINT nChar, UINT nRepCnt, UINT nFlags);
  virtual void OnChar       (HWND wnd, UINT nChar, UINT nRepCnt, UINT nFlags);
  //ETX

  // Description:
  // Methods to set the default exit method for the class. This method is
  // only used if no instance level ExitMethod has been defined.  It is
  // provided as a means to control how an interactor is exited given
  // the various language bindings (tcl, Win32, etc.).
  static void SetClassExitMethod(void (*f)(void *), void *arg);
  static void SetClassExitMethodArgDelete(void (*f)(void *));

  // Description:
  // These methods correspond to the the Exit, User and Pick
  // callbacks. They allow for the Style to invoke them.
  virtual void ExitCallback();

protected:
  vtkWin32RenderWindowInteractor();
  ~vtkWin32RenderWindowInteractor();

  HWND    WindowId;
  UINT    TimerId;
  WNDPROC OldProc;
  int     InstallMessageProc;

  int     MouseInWindow;

  //BTX
  // Description:
  // Class variables so an exit method can be defined for this class
  // (used to set different exit methods for various language bindings,
  // i.e. tcl, java, Win32)
  static void (*ClassExitMethod)(void *);
  static void (*ClassExitMethodArgDelete)(void *);
  static void *ClassExitMethodArg;
  //ETX

private:
  vtkWin32RenderWindowInteractor(const vtkWin32RenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkWin32RenderWindowInteractor&);  // Not implemented.
};

#endif

