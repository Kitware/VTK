/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAndroidRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAndroidRenderWindowInteractor - implements Win32 specific functions
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
#ifndef vtkAndroidRenderWindowInteractor_h
#define vtkAndroidRenderWindowInteractor_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderWindowInteractor.h"

struct AInputEvent;

class VTKRENDERINGOPENGL2_EXPORT vtkAndroidRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  // Description:
  // Construct object so that light follows camera motion.
  static vtkAndroidRenderWindowInteractor *New();

  vtkTypeMacro(vtkAndroidRenderWindowInteractor,vtkRenderWindowInteractor);
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
  // Android specific application terminate, calls ClassExitMethod then
  // calls PostQuitMessage(0) to terminate the application. An application can Specify
  // ExitMethod for alternative behavior (i.e. suppression of keyboard exit)
  void TerminateApp(void);

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

  virtual void SetAndroidApplication(struct android_app *app)
    { this->AndroidApplication = app; };

  // Description:
  // Handle key up/down events
  void HandleKeyEvent(bool down, int nChar, int metaState, int nRepCnt);

  // Description:
  // Handle motion events
  void HandleMotionEvent(int action, int eventPointer, int numPtrs,
    int *xPtr, int *yPtr, int *idPtr, int metaState);

  // Description:
  // used for converting keyCodes on Android
  const char *GetKeySym(int keyCode);

  void HandleCommand(int32_t cmd);
  int32_t HandleInput(AInputEvent* event);


protected:
  vtkAndroidRenderWindowInteractor();
  ~vtkAndroidRenderWindowInteractor();

  int     MouseInWindow;
  int     StartedMessageLoop;

  struct android_app *AndroidApplication;
  const char **KeyCodeToKeySymTable;

  bool Done;  // is the event loop done running

  // Description:
  // Class variables so an exit method can be defined for this class
  // (used to set different exit methods for various language bindings,
  // i.e. tcl, java, Win32)
  static void (*ClassExitMethod)(void *);
  static void (*ClassExitMethodArgDelete)(void *);
  static void *ClassExitMethodArg;

  // Description:
  // Win32-specific internal timer methods. See the superclass for detailed
  // documentation.
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);

  // Description:
  // This will start up the event loop and never return. If you
  // call this method it will loop processing events until the
  // application is exited.
  virtual void StartEventLoop();

private:
  vtkAndroidRenderWindowInteractor(const vtkAndroidRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkAndroidRenderWindowInteractor&);  // Not implemented.
};

#endif
