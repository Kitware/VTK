// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXRenderWindowInteractor
 * @brief   an X event driven interface for a RenderWindow
 *
 * vtkXRenderWindowInteractor is a convenience object that provides event
 * bindings to common graphics functions. For example, camera and actor
 * functions such as zoom-in/zoom-out, azimuth, roll, and pan. IT is one of
 * the window system specific subclasses of vtkRenderWindowInteractor. Please
 * see vtkRenderWindowInteractor documentation for event bindings.
 *
 * @sa
 * vtkRenderWindowInteractor
 */

#ifndef vtkXRenderWindowInteractor_h
#define vtkXRenderWindowInteractor_h

//===========================================================
// now we define the C++ class

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderingUIModule.h" // For export macro
#include "vtkWrappingHints.h"     // For VTK_MARSHALAUTO
#include <X11/Xlib.h>             // Needed for X types in the public interface

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkXRenderWindowInteractorInternals;

class VTKRENDERINGUI_EXPORT VTK_MARSHALAUTO vtkXRenderWindowInteractor
  : public vtkRenderWindowInteractor
{
public:
  static vtkXRenderWindowInteractor* New();
  vtkTypeMacro(vtkXRenderWindowInteractor, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initializes the event handlers without an XtAppContext.  This is
   * good for when you don't have a user interface, but you still
   * want to have mouse interaction.
   */
  void Initialize() override;

  /**
   * Break the event loop on 'q','e' keypress. Want more ???
   */
  void TerminateApp() override;

  /**
   * Process all user-interaction, timer events and return.
   * If there are no events, this method returns immediately.
   */
  void ProcessEvents() override;

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
   * Update the Size data member and set the associated RenderWindow's
   * size.
   */
  void UpdateSize(int, int) override;

  /**
   * Re-defines virtual function to get mouse position by querying X-server.
   */
  void GetMousePosition(int* x, int* y) override;

  /**
   * A X11 specific method to recover mouse position and modifier keys
   * keys is a Xorg specified mask of modifier states
   */
  void GetMousePositionAndModifierKeysState(int* x, int* y, unsigned int* keys);

  void DispatchEvent(XEvent*);

protected:
  vtkXRenderWindowInteractor();
  ~vtkXRenderWindowInteractor() override;

  /**
   * Update the Size data member and set the associated RenderWindow's
   * size but do not resize the XWindow.
   */
  void UpdateSizeNoXResize(int, int);

  // Using static here to avoid destroying context when many apps are open:
  static int NumAppInitialized;

  Display* DisplayId;
  Window WindowId;
  Atom KillAtom;
  int PositionBeforeStereo[2];
  vtkXRenderWindowInteractorInternals* Internal;

  // Drag and drop related
  int XdndSourceVersion;
  Window XdndSource;
  Atom XdndFormatAtom;
  Atom XdndURIListAtom;
  Atom XdndTypeListAtom;
  Atom XdndEnterAtom;
  Atom XdndPositionAtom;
  Atom XdndDropAtom;
  Atom XdndActionCopyAtom;
  Atom XdndStatusAtom;
  Atom XdndFinishedAtom;

  ///@{
  /**
   * X-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  ///@}

  void FireTimers();

  /**
   * This will start up the X event loop and never return. If you
   * call this method it will loop processing X events until the
   * application is exited.
   */
  void StartEventLoop() override;

  /**
   * Wait for new events
   */
  void WaitForEvents();

  /**
   * Check if a display connection is in use by any windows.
   */
  bool CheckDisplayId(Display* dpy);

  /**
   * Deallocate X resource that may have been allocated
   * Also calls finalize on the render window if available
   */
  void Finalize();

private:
  vtkXRenderWindowInteractor(const vtkXRenderWindowInteractor&) = delete;
  void operator=(const vtkXRenderWindowInteractor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
