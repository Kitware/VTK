/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXRenderWindowInteractor - an X event driven interface for a RenderWindow
// .SECTION Description
// vtkXRenderWindowInteractor is a convenience object that provides event
// bindings to common graphics functions. For example, camera and actor
// functions such as zoom-in/zoom-out, azimuth, roll, and pan. IT is one of
// the window system specific subclasses of vtkRenderWindowInteractor. Please
// see vtkRenderWindowInteractor documentation for event bindings.
//
// .SECTION see also
// vtkRenderWindowInteractor vtkXRenderWindow

// I've been though this and deleted all I think should go, tried to create
// the basic structure and if you're lucky it might even work!
// but frankly I doubt it

#ifndef __vtkXRenderWindowInteractor_h
#define __vtkXRenderWindowInteractor_h

//===========================================================
// now we define the C++ class

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderWindowInteractor.h"
#include <X11/StringDefs.h> // Needed for X types in the public interface
#include <X11/Intrinsic.h> // Needed for X types in the public interface

class vtkCallbackCommand;
class vtkXRenderWindowInteractorInternals;

//BTX
// Forward declare internal friend functions.
void VTKRENDERINGOPENGL_EXPORT vtkXRenderWindowInteractorCallback(Widget,XtPointer, XEvent *,Boolean *);
void VTKRENDERINGOPENGL_EXPORT vtkXRenderWindowInteractorTimer(XtPointer,XtIntervalId *);
//ETX

class VTKRENDERINGOPENGL_EXPORT vtkXRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkXRenderWindowInteractor *New();
  vtkTypeMacro(vtkXRenderWindowInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes the event handlers without an XtAppContext.  This is
  // good for when you don't have a user interface, but you still
  // want to have mouse interaction.
  virtual void Initialize();

  // Description:
  // Break the event loop on 'q','e' keypress. Want more ???
  void TerminateApp();

  // Description:
  // The BreakLoopFlag is checked in the Start() method.
  // Setting it to anything other than zero will cause
  // the interactor loop to terminate and return to the
  // calling function.
  vtkGetMacro(BreakLoopFlag, int);
  void SetBreakLoopFlag(int);
  void BreakLoopFlagOff();
  void BreakLoopFlagOn();

  // Description:
  // Initializes the event handlers using an XtAppContext that you have
  // provided.  This assumes that you want to own the event loop.
  virtual void Initialize(XtAppContext app);
  vtkGetMacro( App, XtAppContext );

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
  // This will start up the X event loop and never return. If you
  // call this method it will loop processing X events until the
  // application is exited.
  virtual void Start();

  // Description:
  // Update the Size data member and set the associated RenderWindow's
  // size.
  virtual void UpdateSize(int,int);

  // Description:
  // Specify the Xt widget to use for interaction. This method is
  // one of a couple steps that are required for setting up a
  // vtkRenderWindowInteractor as a widget inside of another user
  // interface. You do not need to use this method if the render window
  // will be a stand-alone window. This is only used when you want the
  // render window to be a subwindow within a larger user interface.
  // In that case, you must tell the render window what X display id
  // to use, and then ask the render window what depth, visual and
  // colormap it wants. Then, you must create an Xt TopLevelShell with
  // those settings. Then you can create the rest of your user interface
  // as a child of the TopLevelShell you created. Eventually, you will
  // create a drawing area or some other widget to serve as the rendering
  // window. You must use the SetWidget method to tell this Interactor
  // about that widget. It's X and it's not terribly easy, but it looks cool.
  virtual void SetWidget(Widget);
  Widget GetWidget() {return this->Top;};

  // Description
  // This method will store the top level shell widget for the interactor.
  // This method and the method invocation sequence applies for:
  //     1 vtkRenderWindow-Interactor pair in a nested widget hierarchy
  //     multiple vtkRenderWindow-Interactor pairs in the same top level shell
  // It is not needed for
  //     1 vtkRenderWindow-Interactor pair as the direct child of a top level shell
  //     multiple vtkRenderWindow-Interactor pairs, each in its own top level shell
  //
  // The method, along with EnterNotify event, changes the keyboard focus among
  // the widgets/vtkRenderWindow(s) so the Interactor(s) can receive the proper
  // keyboard events. The following calls need to be made:
  //     vtkRenderWindow's display ID need to be set to the top level shell's
  //           display ID.
  //     vtkXRenderWindowInteractor's Widget has to be set to the vtkRenderWindow's
  //           container widget
  //     vtkXRenderWindowInteractor's TopLevel has to be set to the top level
  //           shell widget
  // note that the procedure for setting up render window in a widget needs to
  // be followed.  See vtkRenderWindowInteractor's SetWidget method.
  //
  // If multiple vtkRenderWindow-Interactor pairs in SEPARATE windows are desired,
  // do not set the display ID (Interactor will create them as needed.  Alternatively,
  // create and set distinct DisplayID for each vtkRenderWindow. Using the same
  // display ID without setting the parent widgets will cause the display to be
  // reinitialized every time an interactor is initialized), do not set the
  // widgets (so the render windows would be in their own windows), and do
  // not set TopLevelShell (each has its own top level shell already)
  virtual void SetTopLevelShell(Widget);
  Widget GetTopLevelShell() {return this->TopLevelShell;};

  // Description:
  // Re-defines virtual function to get mouse position by querying X-server.
  virtual void GetMousePosition(int *x, int *y);

  // Description:
  // Functions that are used internally.
  friend void vtkXRenderWindowInteractorCallback(Widget,XtPointer,
                                                 XEvent *,Boolean *);
  friend void vtkXRenderWindowInteractorTimer(XtPointer,XtIntervalId *);

protected:
  vtkXRenderWindowInteractor();
  ~vtkXRenderWindowInteractor();

  //Using static here to avoid detroying context when many apps are open:
  static XtAppContext App;
  static int NumAppInitialized;

  Display *DisplayId;
  Window WindowId;
  Atom KillAtom;
  Widget Top;
  int OwnTop;
  int OwnApp;
  int PositionBeforeStereo[2];
  Widget TopLevelShell;
  int TimerId;
  vtkXRenderWindowInteractorInternals* Internal;

  // Description:
  // X-specific internal timer methods. See the superclass for detailed
  // documentation.
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);

  XtIntervalId AddTimeOut(XtAppContext app_context, unsigned long interval,
                          XtTimerCallbackProc proc, XtPointer client_data) ;
  void Timer(XtPointer client_data, XtIntervalId *id);
  void Callback(Widget w, XtPointer client_data, XEvent *event, Boolean *ctd);

  static int BreakLoopFlag;

private:
  vtkXRenderWindowInteractor(const vtkXRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkXRenderWindowInteractor&);  // Not implemented.
};

#endif
