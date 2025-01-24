// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWin32RenderWindowInteractor
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

#ifndef vtkWin32RenderWindowInteractor_h
#define vtkWin32RenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderingUIModule.h" // For export macro
#include "vtkWindows.h"           // For windows API.
#include "vtkWrappingHints.h"     // For VTK_MARSHALAUTO

#include <memory> // for std::unique_ptr

#include "vtkTDxConfigure.h" // defines VTK_USE_TDX
#ifdef VTK_USE_TDX
VTK_ABI_NAMESPACE_BEGIN
class vtkTDxWinDevice;
VTK_ABI_NAMESPACE_END
#endif

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGUI_EXPORT VTK_MARSHALAUTO vtkWin32RenderWindowInteractor
  : public vtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkWin32RenderWindowInteractor* New();

  vtkTypeMacro(vtkWin32RenderWindowInteractor, vtkRenderWindowInteractor);
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
   * Process all user-interaction, timer events and return.
   * If there are no events, this method returns immediately.
   */
  void ProcessEvents() override;

  ///@{
  /**
   * By default the interactor installs a MessageProc callback which
   * intercepts windows messages to the window and controls interactions.
   * MFC or BCB programs can prevent this and instead directly route any mouse/key
   * messages into the event bindings by setting InstallMessgeProc to false.
   */
  vtkSetMacro(InstallMessageProc, int);
  vtkGetMacro(InstallMessageProc, int);
  vtkBooleanMacro(InstallMessageProc, int);
  ///@}

  /**
   * Win32 specific application terminate, calls ClassExitMethod then
   * calls PostQuitMessage(0) to terminate the application. An application can Specify
   * ExitMethod for alternative behavior (i.e. suppression of keyboard exit)
   */
  void TerminateApp() override;

  friend VTKRENDERINGUI_EXPORT LRESULT CALLBACK vtkHandleMessage(
    HWND hwnd, UINT uMsg, WPARAM w, LPARAM l);
  friend VTKRENDERINGUI_EXPORT LRESULT CALLBACK vtkHandleMessage2(
    HWND hwnd, UINT uMsg, WPARAM w, LPARAM l, vtkWin32RenderWindowInteractor* me);

  ///@{
  /**
   * Various methods that a Win32 window can redirect to this class to be
   * handled.
   */
  virtual int OnMouseMove(HWND wnd, UINT nFlags, int X, int Y);
  virtual int OnNCMouseMove(HWND wnd, UINT nFlags, int X, int Y);
  virtual int OnRButtonDown(HWND wnd, UINT nFlags, int X, int Y, int repeat = 0);
  virtual int OnRButtonUp(HWND wnd, UINT nFlags, int X, int Y);
  virtual int OnMButtonDown(HWND wnd, UINT nFlags, int X, int Y, int repeat = 0);
  virtual int OnMButtonUp(HWND wnd, UINT nFlags, int X, int Y);
  virtual int OnLButtonDown(HWND wnd, UINT nFlags, int X, int Y, int repeat = 0);
  virtual int OnLButtonUp(HWND wnd, UINT nFlags, int X, int Y);
  virtual int OnSize(HWND wnd, UINT nType, int X, int Y);
  virtual int OnTimer(HWND wnd, UINT nIDEvent);
  virtual int OnKeyDown(HWND wnd, UINT nChar, UINT nRepCnt, UINT nFlags);
  virtual int OnKeyUp(HWND wnd, UINT nChar, UINT nRepCnt, UINT nFlags);
  virtual int OnChar(HWND wnd, UINT nChar, UINT nRepCnt, UINT nFlags);
  virtual int OnMouseWheelForward(HWND wnd, UINT nFlags, int X, int Y);
  virtual int OnMouseWheelBackward(HWND wnd, UINT nFlags, int X, int Y);
  virtual int OnFocus(HWND wnd, UINT nFlags);
  virtual int OnKillFocus(HWND wnd, UINT nFlags);
  virtual int OnTouch(HWND wnd, UINT wParam, UINT lParam);
  virtual int OnDropFiles(HWND wnd, WPARAM wParam);
  ///@}

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

protected:
  vtkWin32RenderWindowInteractor();
  ~vtkWin32RenderWindowInteractor() override;

  HWND WindowId;
  WNDPROC OldProc;
  int InstallMessageProc;
  int MouseInWindow;
  int StartedMessageLoop;

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

#ifdef VTK_USE_TDX
  vtkTDxWinDevice* Device;
#endif

private:
  vtkWin32RenderWindowInteractor(const vtkWin32RenderWindowInteractor&) = delete;
  void operator=(const vtkWin32RenderWindowInteractor&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
