/*
 * Copyright 2012 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkExecutionTimer - Time filter execution
//
// .SECTION Description
//
// This object monitors a single filter for StartEvent and EndEvent.
// Each time it hears StartEvent it records the time.  Each time it
// hears EndEvent it measures the elapsed time (both CPU and
// wall-clock) since the most recent StartEvent.  Internally we use
// vtkTimerLog for measurements.
//
// By default we simply store the elapsed time.  You are welcome to
// subclass and override TimerFinished() to do anything you want.

#ifndef __vtkExecutionTimer_h
#define __vtkExecutionTimer_h

#include <vtkObject.h>
#include "vtkFiltersCoreModule.h" // For export macro

class vtkAlgorithm;
class vtkCallbackCommand;

class VTKFILTERSCORE_EXPORT vtkExecutionTimer : public vtkObject
{
public:
  vtkTypeMacro(vtkExecutionTimer, vtkObject);
  static vtkExecutionTimer* New();
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetFilter(vtkAlgorithm* filter);
  vtkGetObjectMacro(Filter, vtkAlgorithm);

  vtkGetMacro(ElapsedCPUTime, double);
  vtkGetMacro(ElapsedWallClockTime, double);

protected:
  vtkExecutionTimer();
  ~vtkExecutionTimer();

  vtkCallbackCommand* Callback;
  vtkAlgorithm* Filter;

  double CPUStartTime;
  double CPUEndTime;

  double WallClockStartTime;
  double WallClockEndTime;

  double ElapsedCPUTime;
  double ElapsedWallClockTime;

  void StartTimer();
  void StopTimer();

  // Description:
  // This is where you can do anything you want with the progress
  // event.  By default this does nothing.
  virtual void TimerFinished();

  // Description:
  // This is the callback that VTK will invoke when it sees StartEvent
  // and EndEvent.  Its responsibility is to pass the event on to an
  // instance of this observer class.
  static void EventRelay(vtkObject* caller, unsigned long eventId, void* clientData, void* callData);

private:
  vtkExecutionTimer(const vtkExecutionTimer&);
  void operator=(const vtkExecutionTimer&);

};

#endif
