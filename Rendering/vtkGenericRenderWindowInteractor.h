/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericRenderWindowInteractor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericRenderWindowInteractor - platform-independent programmable render window interactor.

// .SECTION Description
// vtkGenericRenderWindowInteractor provides a way to translate native
// mouse and keyboard events into vtk Events.   By calling the methods on
// this class, vtk events will be invoked.   This will allow scripting
// languages to use vtkInteractorStyles and 3D widgets.



#ifndef __vtkGenericRenderWindowInteractor_h
#define __vtkGenericRenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"

class VTK_RENDERING_EXPORT vtkGenericRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkGenericRenderWindowInteractor *New();
  vtkTypeRevisionMacro(vtkGenericRenderWindowInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Fire various events, SetEventInformation should be called just prior
  // to calling any of these methods.  This methods will Invoke the 
  // corresponding vtk event.
  virtual void MouseMoveEvent();
  virtual void RightButtonPressEvent();
  virtual void RightButtonReleaseEvent();
  virtual void LeftButtonPressEvent();
  virtual void LeftButtonReleaseEvent();
  virtual void MiddleButtonPressEvent();
  virtual void MiddleButtonReleaseEvent();
  virtual void ExposeEvent();
  virtual void ConfigureEvent();
  virtual void EnterEvent();
  virtual void LeaveEvent();
  virtual void TimerEvent();
  virtual void KeyPressEvent();
  virtual void KeyReleaseEvent();
  virtual void CharEvent();
  virtual void ExitEvent();
  
  // Description:
  // Allow users of the class to add callbacks to handle the creation and 
  // destruction of timers.   CreateTimer should create a timer event of 10 milliseconds,
  // and at the end of that time, it should call TimerEvent on this class.
  virtual int CreateTimer(int );
  virtual int DestroyTimer();
protected:
  vtkGenericRenderWindowInteractor();
  ~vtkGenericRenderWindowInteractor();
private:
  vtkGenericRenderWindowInteractor(const vtkGenericRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkGenericRenderWindowInteractor&);  // Not implemented.
};

#endif
