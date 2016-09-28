/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkExecutionTimer
 * @brief   Time filter execution
 *
 *
 *
 * This object monitors a single filter for StartEvent and EndEvent.
 * Each time it hears StartEvent it records the time.  Each time it
 * hears EndEvent it measures the elapsed time (both CPU and
 * wall-clock) since the most recent StartEvent.  Internally we use
 * vtkTimerLog for measurements.
 *
 * By default we simply store the elapsed time.  You are welcome to
 * subclass and override TimerFinished() to do anything you want.
*/

#ifndef vtkExecutionTimer_h
#define vtkExecutionTimer_h

#include "vtkObject.h"
#include "vtkFiltersCoreModule.h" // For export macro

class vtkAlgorithm;
class vtkCallbackCommand;

class VTKFILTERSCORE_EXPORT vtkExecutionTimer : public vtkObject
{
public:
  vtkTypeMacro(vtkExecutionTimer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct a new timer with no attached filter.  Use SetFilter()
   * to specify the vtkAlgorithm whose execution you want to time.
   */
  static vtkExecutionTimer* New();

  //@{
  /**
   * Set/get the filter to be monitored.  The only real constraint
   * here is that the vtkExecutive associated with the filter must
   * fire StartEvent and EndEvent before and after the filter is
   * executed.  All VTK executives should do this.
   */
  void SetFilter(vtkAlgorithm* filter);
  vtkGetObjectMacro(Filter, vtkAlgorithm);
  //@}

  //@{
  /**
   * Get the total CPU time (in seconds) that elapsed between
   * StartEvent and EndEvent.  This is undefined before the filter has
   * finished executing.
   */
  vtkGetMacro(ElapsedCPUTime, double);
  //@}

  //@{
  /**
   * Get the total wall clock time (in seconds) that elapsed between
   * StartEvent and EndEvent.  This is undefined before the filter has
   * finished executing.
   */
  vtkGetMacro(ElapsedWallClockTime, double);
  //@}

protected:
  vtkExecutionTimer();
  ~vtkExecutionTimer() VTK_OVERRIDE;

  // This is the observer that will catch StartEvent and hand off to
  // EventRelay
  vtkCallbackCommand* Callback;

  // This is the filter that will be timed
  vtkAlgorithm* Filter;

  // These are where we keep track of the timestamps for start/end
  double CPUStartTime;
  double CPUEndTime;

  double WallClockStartTime;
  double WallClockEndTime;

  double ElapsedCPUTime;
  double ElapsedWallClockTime;

  //@{
  /**
   * Convenience functions -- StartTimer clears out the elapsed times
   * and records start times; StopTimer records end times and computes
   * the elapsed time
   */
  void StartTimer();
  void StopTimer();
  //@}

  /**
   * This is where you can do anything you want with the progress
   * event.  By default this does nothing.
   */
  virtual void TimerFinished();

  /**
   * This is the callback that VTK will invoke when it sees StartEvent
   * and EndEvent.  Its responsibility is to pass the event on to an
   * instance of this observer class.
   */
  static void EventRelay(vtkObject* caller, unsigned long eventId, void* clientData, void* callData);

private:
  vtkExecutionTimer(const vtkExecutionTimer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExecutionTimer&) VTK_DELETE_FUNCTION;

};

#endif
