/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowTclInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXRenderWindowTclInteractor
 * @brief   a TCL event driven interface for a RenderWindow
 *
 * vtkXRenderWindowTclInteractor is a convenience object that provides event
 * bindings to common graphics functions. For example, camera and actor
 * functions such as zoom-in/zoom-out, azimuth, roll, and pan. It is one of
 * the window system specific subclasses of vtkRenderWindowInteractor. Please
 * see vtkRenderWindowInteractor documentation for event bindings.
 *
 * @sa
 * vtkRenderWindowInteractor vtkXRenderWindowInteractor vtkXRenderWindow
*/

#ifndef vtkXRenderWindowTclInteractor_h
#define vtkXRenderWindowTclInteractor_h

#include "vtkXRenderWindowInteractor.h"

class vtkXRenderWindowTclInteractorInternals;

class vtkXRenderWindowTclInteractor : public vtkXRenderWindowInteractor
{
public:
  static vtkXRenderWindowTclInteractor *New();
  vtkTypeMacro(vtkXRenderWindowTclInteractor,vtkXRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initializes a Tcl/Tk specific event handler.
   */
  void Initialize() override;

  /**
   * Overridden only to eliminate the "virtual function hidden" warning.
   * Implementation delegates directly to the Superclass.
   */
  void Initialize(XtAppContext app) override;

  //@{
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
  //@}

  /**
   * This will start a Tcl/Tk event loop that only returns when the user
   * presses the 'q' or 'e' key or when some other event observer calls
   * our ExitCallback method.
   */
  void Start() override;

protected:
  vtkXRenderWindowTclInteractor();
  ~vtkXRenderWindowTclInteractor() override;

  //@{
  /**
   * Tcl/Tk specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  int InternalDestroyTimer(int platformTimerId) override;
  //@}

private:
  vtkXRenderWindowTclInteractorInternals* TclInternal;

  vtkXRenderWindowTclInteractor(const vtkXRenderWindowTclInteractor&) = delete;
  void operator=(const vtkXRenderWindowTclInteractor&) = delete;
};

#endif
