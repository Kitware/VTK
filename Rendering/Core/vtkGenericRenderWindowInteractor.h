/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
  vtkTypeMacro(vtkGenericRenderWindowInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Fire TimerEvent. SetEventInformation should be called just prior
  // to calling any of these methods. These methods will Invoke the
  // corresponding vtk event.
  virtual void TimerEvent();

  // Description:
  // Flag that indicates whether the TimerEvent method should call ResetTimer
  // to simulate repeating timers with an endless stream of one shot timers.
  // By default this flag is on and all repeating timers are implemented as a
  // stream of sequential one shot timers. If the observer of
  // CreateTimerEvent actually creates a "natively repeating" timer, setting
  // this flag to off will prevent (perhaps many many) unnecessary calls to
  // ResetTimer. Having the flag on by default means that "natively one
  // shot" timers can be either one shot or repeating timers with no
  // additional work. Also, "natively repeating" timers still work with the
  // default setting, but with potentially many create and destroy calls.
  vtkSetMacro(TimerEventResetsTimer, int);
  vtkGetMacro(TimerEventResetsTimer, int);
  vtkBooleanMacro(TimerEventResetsTimer, int);

protected:
  vtkGenericRenderWindowInteractor();
  ~vtkGenericRenderWindowInteractor();

  // Description: 
  // Generic internal timer methods. See the superclass for detailed
  // documentation.
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);

  int TimerEventResetsTimer;

private:
  vtkGenericRenderWindowInteractor(const vtkGenericRenderWindowInteractor&);  // Not implemented.
  void operator=(const vtkGenericRenderWindowInteractor&);  // Not implemented.
};

#endif
