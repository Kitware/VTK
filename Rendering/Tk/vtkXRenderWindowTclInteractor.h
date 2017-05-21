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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Initializes a Tcl/Tk specific event handler.
   */
  void Initialize() VTK_OVERRIDE;

  /**
   * Overridden only to eliminate the "virtual function hidden" warning.
   * Implementation delegates directly to the Superclass.
   */
  void Initialize(XtAppContext app) VTK_OVERRIDE;

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
  void Enable() VTK_OVERRIDE;
  void Disable() VTK_OVERRIDE;
  //@}

  /**
   * This will start a Tcl/Tk event loop that only returns when the user
   * presses the 'q' or 'e' key or when some other event observer calls
   * our ExitCallback method.
   */
  void Start() VTK_OVERRIDE;

protected:
  vtkXRenderWindowTclInteractor();
  ~vtkXRenderWindowTclInteractor() VTK_OVERRIDE;

  //@{
  /**
   * Tcl/Tk specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) VTK_OVERRIDE;
  int InternalDestroyTimer(int platformTimerId) VTK_OVERRIDE;
  //@}

private:
  vtkXRenderWindowTclInteractorInternals* TclInternal;

  vtkXRenderWindowTclInteractor(const vtkXRenderWindowTclInteractor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXRenderWindowTclInteractor&) VTK_DELETE_FUNCTION;
};

#endif
