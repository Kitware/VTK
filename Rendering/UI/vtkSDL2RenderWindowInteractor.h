/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSDL2RenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSDL2RenderWindowInteractor
 * @brief   implements SDL2 specific functions
 * required by vtkRenderWindowInteractor.
 *
 */

#ifndef vtkSDL2RenderWindowInteractor_h
#define vtkSDL2RenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderingUIModule.h" // For export macro
#include <map>                    // for ivar

class VTKRENDERINGUI_EXPORT vtkSDL2RenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkSDL2RenderWindowInteractor* New();

  vtkTypeMacro(vtkSDL2RenderWindowInteractor, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the event handler
   */
  virtual void Initialize() override;

  /**
   * Run the event loop and return. This is provided so that you can
   * implement your own event loop but yet use the vtk event handling as
   * well.
   */
  void ProcessEvents() override;

  /**
   * SDL2 specific application terminate, calls ClassExitMethod then
   * calls PostQuitMessage(0) to terminate the application. An application can Specify
   * ExitMethod for alternative behavior (i.e. suppression of keyboard exit)
   */
  void TerminateApp(void) override;

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  void ExitCallback() override;

  // When using emscripten this adds the event hander
  // and then returns without blocking or aborting.
  // TerminateApp will remove the event hanbdler.
  void AddEventHandler();

protected:
  vtkSDL2RenderWindowInteractor();
  ~vtkSDL2RenderWindowInteractor() override;

  bool ProcessEvent(void* event);

  ///@{
  /**
   * SDL2-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  ///@}

  std::map<int, int> VTKToPlatformTimerMap;

  /**
   * This will start up the event loop and never return. If you
   * call this method it will loop processing events until the
   * application is exited.
   */
  void StartEventLoop() override;

  bool StartedMessageLoop;

private:
  vtkSDL2RenderWindowInteractor(const vtkSDL2RenderWindowInteractor&) = delete;
  void operator=(const vtkSDL2RenderWindowInteractor&) = delete;
};

#endif
