// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAndroidRenderWindowInteractor
 * @brief   implements Win32 specific functions
 * required by vtkRenderWindowInteractor.
 *
 *
 * By default the interactor installs a MessageProc callback which
 * intercepts windows' messages to the window and controls interactions by
 * routing them to the InteractoStyle classes.
 * MFC or BCB programs can prevent this and instead directly route any mouse/key
 * messages into the event bindings by setting InstallMessageProc to false.
 * This provides a minimal "Mapped" mode of interaction
 *
 */

#ifndef vtkAndroidRenderWindowInteractor_h
#define vtkAndroidRenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderingUIModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
struct AInputEvent;

class VTKRENDERINGUI_EXPORT vtkAndroidRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkAndroidRenderWindowInteractor* New();

  vtkTypeMacro(vtkAndroidRenderWindowInteractor, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the event handler
   */
  void Initialize() override;

  ///@{
  /**
   * Enable/Disable interactions.  By default interactors are enabled when
   * initialized.  Initialize() must be called prior to enabling/disabling
   * interaction. These methods are used when a window/widget is being
   * shared by multiple renderers and interactors.  This allows a "modal"
   * display where one interactor is active when its data is to be displayed
   * and all other interactors associated with the widget are disabled
   * when their data is not displayed.
   */
  void Enable() override;
  void Disable() override;
  ///@}

  /**
   * Android specific application terminate, calls ClassExitMethod then
   * calls PostQuitMessage(0) to terminate the application. An application can Specify
   * ExitMethod for alternative behavior (i.e. suppression of keyboard exit)
   */
  void TerminateApp() override;

  ///@{
  /**
   * Methods to set the default exit method for the class. This method is
   * only used if no instance level ExitMethod has been defined.  It is
   * provided as a means to control how an interactor is exited given
   * the various language bindings (Win32, etc.).
   */
  static void SetClassExitMethod(void (*f)(void*), void* arg);
  static void SetClassExitMethodArgDelete(void (*f)(void*));
  ///@}

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  void ExitCallback() override;

  virtual void SetAndroidApplication(struct android_app* app) { this->AndroidApplication = app; }

  /**
   * Handle key up/down events
   */
  void HandleKeyEvent(bool down, int nChar, int metaState, int nRepCnt);

  /**
   * Handle motion events
   */
  void HandleMotionEvent(
    int actionType, int actionId, int numPtrs, int* xPtr, int* yPtr, int* idPtr, int metaState);

  /**
   * used for converting keyCodes on Android
   */
  const char* GetKeySym(int keyCode);

  void HandleCommand(int32_t cmd);
  int32_t HandleInput(AInputEvent* event);

  ///@{
  /**
   * Returns true if the window is owned by VTK.
   */
  vtkSetMacro(OwnWindow, bool);
  vtkGetMacro(OwnWindow, bool);
  ///@}

protected:
  vtkAndroidRenderWindowInteractor();
  ~vtkAndroidRenderWindowInteractor() override;

  int MouseInWindow;
  int StartedMessageLoop;

  struct android_app* AndroidApplication;
  const char** KeyCodeToKeySymTable;

  bool Done; // is the event loop done running

  ///@{
  /**
   * Class variables so an exit method can be defined for this class
   * (used to set different exit methods for various language bindings,
   * i.e. java, Win32)
   */
  static void (*ClassExitMethod)(void*);
  static void (*ClassExitMethodArgDelete)(void*);
  static void* ClassExitMethodArg;
  ///@}

  ///@{
  /**
   * Win32-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  ///@}

  /**
   * This will start up the event loop and never return. If you
   * call this method it will loop processing events until the
   * application is exited.
   */
  void StartEventLoop() override;

  vtkTypeBool OwnWindow;

private:
  vtkAndroidRenderWindowInteractor(const vtkAndroidRenderWindowInteractor&) = delete;
  void operator=(const vtkAndroidRenderWindowInteractor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
