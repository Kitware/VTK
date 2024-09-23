// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebAssemblyRenderWindowInteractor
 * @brief   Handles user interaction in web browsers.
 *
 * The interactor intercepts user interaction events from a HTML page in a web browser
 * and sends them to VTK using the Emscripten HTML5 C API.
 *
 * Contrary to the documentation of `Start`, this interactor's event loop
 * can be configured to not block in order to return control to the browser so that it can render
 * graphics, UI, etc. See
 * https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop See
 * vtkRenderWindowInteractor::InteractorManagesTheEventLoop
 */

#ifndef vtkWebAssemblyRenderWindowInteractor_h
#define vtkWebAssemblyRenderWindowInteractor_h

#ifndef __EMSCRIPTEN__
#error "vtkWebAssemblyRenderWindowInteractor requires the Emscripten SDK"
#endif

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderingUIModule.h" // For export macro
#include "vtkWrappingHints.h"     // For VTK_MARSHALAUTO
#include <memory>                 // for shared_ptr

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGUI_EXPORT VTK_MARSHALAUTO vtkWebAssemblyRenderWindowInteractor
  : public vtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkWebAssemblyRenderWindowInteractor* New();

  vtkTypeMacro(vtkWebAssemblyRenderWindowInteractor, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the event handler
   */
  void Initialize() override;

  /**
   * Process all user-interaction, timer events and return.
   * If there are no events, this method returns immediately.
   */
  void ProcessEvents() override;

  /**
   * This function is called on 'q','e' keypress if exitmethod is not
   * specified and should be overridden by platform dependent subclasses
   * to provide a termination procedure if one is required.
   */
  void TerminateApp() override;

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  void ExitCallback() override;

  /**
   * Specify the selector of the canvas element in the DOM.
   */
  vtkGetStringMacro(CanvasSelector);
  virtual void SetCanvasSelector(const char* value);

  /**
   * When true (default), the style of the parent element of canvas will be adjusted
   * allowing the canvas to take up entire space of the parent.
   */
  vtkGetMacro(ExpandCanvasToContainer, bool);
  vtkSetMacro(ExpandCanvasToContainer, bool);
  vtkBooleanMacro(ExpandCanvasToContainer, bool);

  /**
   * When true (default), a JavaScript `ResizeObserver` is installed on the parent element of
   * the canvas. The observer shall adjust the `width` and `height` of the canvas element
   * according the dimensions of the parent element.
   */
  vtkGetMacro(InstallHTMLResizeObserver, bool);
  vtkSetMacro(InstallHTMLResizeObserver, bool);
  vtkBooleanMacro(InstallHTMLResizeObserver, bool);

protected:
  vtkWebAssemblyRenderWindowInteractor();
  ~vtkWebAssemblyRenderWindowInteractor() override;

  ///@{
  /**
   * Register/UnRegister callback functions for all recognized events on the document.
   * This function calls `emscripten_set_xyz_callback_on_thread` with the `CanvasSelector` as the
   * target and the thread parameter equal to EM_CALLBACK_THREAD_CONTEXT_MAIN_RUNTIME_THREAD.
   *
   * Basically, the pumping process works like this, events are received on the main UI thread into
   * a queue. The event is then processed during the next `requestAnimationFrame` call.
   */
  void RegisterUICallbacks();
  void UnRegisterUICallbacks();
  ///@}

  void ProcessEvent(int type, const std::uint8_t* event);

  ///@{
  /**
   * Internal methods for creating and destroying timers that must be
   * implemented by subclasses. InternalCreateTimer() returns a
   * platform-specific timerId and InternalDestroyTimer() returns
   * non-zero value on success.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  ///@}

  /**
   * This will start up the event loop without blocking the main thread.
   */
  void StartEventLoop() override;

  char* CanvasSelector = nullptr;
  bool ExpandCanvasToContainer;
  bool InstallHTMLResizeObserver;

private:
  vtkWebAssemblyRenderWindowInteractor(const vtkWebAssemblyRenderWindowInteractor&) = delete;
  void operator=(const vtkWebAssemblyRenderWindowInteractor&) = delete;

  friend class vtkInternals;
  class vtkInternals;
  std::shared_ptr<vtkInternals> Internals; // the pointer is also shared with timer's callback data.
};

extern "C"
{
  typedef void (*vtkTimerCallbackFunc)(void*);
  int vtkCreateTimer(
    unsigned long duration, bool isOneShot, vtkTimerCallbackFunc callback, void* userData);
  void vtkDestroyTimer(int timerId, bool isOneShot);
  int* vtkGetParentElementBoundingRectSize(const char* selector);
  void vtkInitializeCanvasElement(const char* selector, bool applyStyle);
}

VTK_ABI_NAMESPACE_END
#endif
