/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzRenderWindowInteractor.h
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
// .NAME vtkQuartzRenderWindowInteractor - implements Quartz specific functions
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
#ifndef __vtkQuartzRenderWindowInteractor_h
#define __vtkQuartzRenderWindowInteractor_h

#include <stdlib.h>
#include "vtkRenderWindowInteractor.h"


#ifdef __cplusplus
extern "C" {
#endif

//int DrawSomeGL();
//void VBTimerEvent(void *vtkClass);
//void DoMouseMoved(void *vtkClass, int shiftDown, int controlDown, int altDown, int commandDown, float xLoc, float yLoc);
//void DoMouseDragged(void *vtkClass, int shiftDown, int controlDown, int altDown, int commandDown, float xLoc, float yLoc);
//void DoMouseUp(void *vtkClass, int shiftDown, int controlDown, int altDown, int commandDown, float xLoc, float yLoc);
//void DoMouseDown(void *vtkClass, int shiftDown, int controlDown, int altDown, int commandDown, float xLoc, float yLoc);

#ifdef __cplusplus
};
#endif



class VTK_RENDERING_EXPORT vtkQuartzRenderWindowInteractor : public vtkRenderWindowInteractor {
public:
  // Description:
  // Construct object so that light follows camera motion.
  static vtkQuartzRenderWindowInteractor *New();

  vtkTypeRevisionMacro(vtkQuartzRenderWindowInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the even handler
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
  // Quartz specific application terminate, calls ClassExitMethod then
  // calls PostQuitMessage(0) to terminate app. An application can Specify
  // ExitMethod for alternative behaviour (i.e. suppresion of keyboard exit)
  void TerminateApp(void);

  // Description:
  // Quartz timer methods
  int CreateTimer(int timertype);
  int DestroyTimer(void);

  //BTX
  // Description:
  // Various methods that a Quartz window can redirect to this class to be 
  // handled.
//  virtual void OnMouseMove  (void *wnd, int nFlags, int X, int Y);
  virtual void OnRButtonDown(void *wnd, int nFlags, int X, int Y);
  virtual void OnRButtonUp  (void *wnd, int nFlags, int X, int Y);
  virtual void OnMButtonDown(void *wnd, int nFlags, int X, int Y);
  virtual void OnMButtonUp  (void *wnd, int nFlags, int X, int Y);
  virtual void OnLButtonDown(void *wnd, int nFlags, int X, int Y);
  virtual void OnLButtonUp  (void *wnd, int nFlags, int X, int Y);
  virtual void OnSize       (void *wnd, int nType,  int X, int Y);
  virtual void OnTimer      (void *wnd, int nIDEvent);
  virtual void OnChar       (void *wnd, int nChar, int nRepCnt, int nFlags);
  //ETX

  // Description:
  // Methods to set the default exit method for the class. This method is
  // only used if no instance level ExitMethod has been defined.  It is
  // provided as a means to control how an interactor is exited given
  // the various language bindings (tcl, Quartz, etc.).
  static void SetClassExitMethod(void (*f)(void *), void *arg);
  static void SetClassExitMethodArgDelete(void (*f)(void *));

  // Description:
  // These methods correspond to the the Exit, User and Pick
  // callbacks. They allow for the Style to invoke them.
  virtual void ExitCallback();
  
  int GetButtonDown();
  void SetButtonDown(int button);

protected:
  vtkQuartzRenderWindowInteractor();
  ~vtkQuartzRenderWindowInteractor();

  void   *WindowId;
  void   *ApplicationId;
  int     TimerId;
  void   *OldProc;
  int     InstallMessageProc;
  int   whichButtonDown; //0=none, 1=left, 2=middle, 3=right

  //BTX
  // Description:
  // Class variables so an exit method can be defined for this class
  // (used to set different exit methods for various language bindings,
  // i.e. tcl, java, Quartz)
  static void (*ClassExitMethod)(void *);
  static void (*ClassExitMethodArgDelete)(void *);
  static void *ClassExitMethodArg;
  //ETX
  
private:
  vtkQuartzRenderWindowInteractor(const vtkQuartzRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkQuartzRenderWindowInteractor&);  // Not implemented.
};

#endif


