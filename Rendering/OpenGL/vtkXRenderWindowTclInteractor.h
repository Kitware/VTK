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
// .NAME vtkXRenderWindowTclInteractor - a TCL event driven interface for a RenderWindow
// .SECTION Description
// vtkXRenderWindowTclInteractor is a convenience object that provides event
// bindings to common graphics functions. For example, camera and actor
// functions such as zoom-in/zoom-out, azimuth, roll, and pan. It is one of
// the window system specific subclasses of vtkRenderWindowInteractor. Please
// see vtkRenderWindowInteractor documentation for event bindings.
//
// .SECTION see also
// vtkRenderWindowInteractor vtkXRenderWindowInteractor vtkXRenderWindow

#ifndef __vtkXRenderWindowTclInteractor_h
#define __vtkXRenderWindowTclInteractor_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkXRenderWindowInteractor.h"

class vtkXRenderWindowTclInteractorInternals;

class VTKRENDERINGOPENGL_EXPORT vtkXRenderWindowTclInteractor : public vtkXRenderWindowInteractor
{
public:
  static vtkXRenderWindowTclInteractor *New();
  vtkTypeMacro(vtkXRenderWindowTclInteractor,vtkXRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes a Tcl/Tk specific event handler.
  virtual void Initialize();

  // Description:
  // Overridden only to eliminate the "virtual function hidden" warning.
  // Implementation delegates directly to the Superclass.
  virtual void Initialize(XtAppContext app);

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
  // This will start a Tcl/Tk event loop that only returns when the user
  // presses the 'q' or 'e' key or when some other event observer calls
  // our ExitCallback method.
  virtual void Start();

protected:
  vtkXRenderWindowTclInteractor();
  ~vtkXRenderWindowTclInteractor();

  // Description:
  // Tcl/Tk specific internal timer methods. See the superclass for detailed
  // documentation.
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);

private:
  vtkXRenderWindowTclInteractorInternals* Internal;

  vtkXRenderWindowTclInteractor(const vtkXRenderWindowTclInteractor&);  // Not implemented.
  void operator=(const vtkXRenderWindowTclInteractor&);  // Not implemented.
};

#endif
