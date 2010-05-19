/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCarbonRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCarbonRenderWindowInteractor - implements Carbon specific functions
// required by vtkRenderWindowInteractor.
//
// .SECTION Description
// The interactor interfaces with vtkCarbonWindow
// to trap messages from the Carbon window manager and send them to vtk.
//
#ifndef __vtkCarbonRenderWindowInteractor_h
#define __vtkCarbonRenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"

#include "vtkTDxConfigure.h" // defines VTK_USE_TDX
#ifdef VTK_USE_TDX
class vtkTDxMacDevice;
#endif

// The 10.3.9 SDK (and older probably) have a bug in fp.h (in the Carbon
// umbrella framework) which this works around. Without this, there
// would be a compile error from the Carbon header if Python wrappings
// were enabled.
#include <AvailabilityMacros.h> // Needed for MAC_OS_X_VERSION_MAX_ALLOWED
#if MAC_OS_X_VERSION_MAX_ALLOWED < 1040
  #define scalb scalbn
#endif

#include <Carbon/Carbon.h> // Carbon and Mac specific

class VTK_RENDERING_EXPORT vtkCarbonRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  // Description:
  // Construct object so that light follows camera motion.
  static vtkCarbonRenderWindowInteractor *New();

  vtkTypeMacro(vtkCarbonRenderWindowInteractor,vtkRenderWindowInteractor);
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
  // Carbon specific application terminate, calls ClassExitMethod then
  // calls PostQuitMessage(0) to terminate app. An application can Specify
  // ExitMethod for alternative behaviour (i.e. suppresion of keyboard exit)
  void TerminateApp(void);

  // Description:
  // Methods to set the default exit method for the class. This method is
  // only used if no instance level ExitMethod has been defined.  It is
  // provided as a means to control how an interactor is exited given
  // the various language bindings (tcl, Carbon, etc.).
  static void SetClassExitMethod(void (*f)(void *), void *arg);
  static void SetClassExitMethodArgDelete(void (*f)(void *));

  // Description:
  // These methods correspond to the the Exit, User and Pick
  // callbacks. They allow for the Style to invoke them.
  virtual void ExitCallback();

  // Description:
  // This method is for internal use only.  Track the Carbon mouse deltas 
  // Carbon events that don't provide mouse position directly.
  void GetLastMouseDelta(int delta[2]) {
    delta[0] = this->LastMouseDelta[0]; delta[1] = this->LastMouseDelta[1]; };
  void SetLastMouseDelta(int deltaX, int deltaY) {
    this->LastMouseDelta[0] = deltaX; this->LastMouseDelta[1] = deltaY; };

  // Description:
  // This method is for internal use only.  This is a state variable used for
  // Enter/Leave events.  If the mouse is dragged outside of the window,
  // MouseInsideWindow will remain set until the mouse button is released
  // outside of the window.
  void SetMouseInsideWindow(int val) {
     this->MouseInsideWindow = val; };
  int GetMouseInsideWindow() {
     return this->MouseInsideWindow; };

  // Description:
  // This method is for internal use only.  This is a state variable used for
  // Enter/Leave events.  It keeps track of whether a mouse button is down.
  void SetMouseButtonDown(int val) {
     this->MouseButtonDown = val; };
  int GetMouseButtonDown() {
     return this->MouseButtonDown; };

protected:
  vtkCarbonRenderWindowInteractor();
  ~vtkCarbonRenderWindowInteractor();

  EventHandlerUPP   ViewProcUPP;
  EventHandlerUPP   WindowProcUPP;
  int               InstallMessageProc;

  // For generating event info that Carbon doesn't
  int   LastMouseDelta[2];
  int   LeaveCheckId;
  int   MouseInsideWindow;
  int   MouseButtonDown;

  //BTX
  // Description:
  // Class variables so an exit method can be defined for this class
  // (used to set different exit methods for various language bindings,
  // i.e. tcl, java, Carbon)
  static void (*ClassExitMethod)(void *);
  static void (*ClassExitMethodArgDelete)(void *);
  static void *ClassExitMethodArg;
  //ETX

  // Description: 
  // Carbon-specific internal timer methods. See the superclass for detailed
  // documentation.
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);

#ifdef VTK_USE_TDX
  vtkTDxMacDevice *Device;
#endif
  
private:
  vtkCarbonRenderWindowInteractor(const vtkCarbonRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkCarbonRenderWindowInteractor&);  // Not implemented.
};

#endif
