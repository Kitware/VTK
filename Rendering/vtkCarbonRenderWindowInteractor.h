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
// The interactor interfaces with vtkCarbonWindow.mm and vtkCarbonGLView.mm
// to trap messages from the Carbon window manager and send them to vtk.
//
#ifndef __vtkCarbonRenderWindowInteractor_h
#define __vtkCarbonRenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"

#include <Carbon/Carbon.h> // Needed for Carbon types


class VTK_RENDERING_EXPORT vtkCarbonRenderWindowInteractor : public vtkRenderWindowInteractor {
public:
  // Description:
  // Construct object so that light follows camera motion.
  static vtkCarbonRenderWindowInteractor *New();

  vtkTypeRevisionMacro(vtkCarbonRenderWindowInteractor,vtkRenderWindowInteractor);
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

protected:
  vtkCarbonRenderWindowInteractor();
  ~vtkCarbonRenderWindowInteractor();

  EventHandlerUPP   ViewProcUPP;
  EventHandlerUPP   WindowProcUPP;
  int               InstallMessageProc;

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

private:
  vtkCarbonRenderWindowInteractor(const vtkCarbonRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkCarbonRenderWindowInteractor&);  // Not implemented.
};

#endif
