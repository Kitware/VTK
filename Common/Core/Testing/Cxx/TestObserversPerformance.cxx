/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartPointer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test speed of Observers.
// .SECTION Description
// Probe the speed of vtkObject::AddObserver, vtkObject::InvokeEvent and
// vtkObject::RemoveObserver

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

#include <map>
#include <vector>

// How many times the tests are run to average the elapsed time.
static const int STRESS_COUNT = 5;

// Description:
// Type of console outputs.
// CDash writes perfs as <DartMeasurement ...> for unit test regression
// Csv writes perfs as series of 2D table for easy plotting in spreadsheet apps
// Details writes more timing information
enum VerboseType
{
  None = 0x0,
  CDash = 0x1,
  Csv = 0x2,
  Details = 0x4
};

static const int VERBOSE_MODE = CDash;

//------------------------------------------------------------------------------
class vtkSimpleCommand : public vtkCommand
{
public:
  static vtkSimpleCommand* New() { return new vtkSimpleCommand();}
  vtkTypeMacro(vtkSimpleCommand, vtkCommand);

  void Execute(vtkObject*, unsigned long, void*) VTK_OVERRIDE
  {
      this->MTime.Modified();
  }
protected:
  static vtkTimeStamp MTime;
};

vtkTimeStamp vtkSimpleCommand::MTime;

//------------------------------------------------------------------------------
double TestStressInvoke(int observerCount, int eventCount, int invokeCount);

//------------------------------------------------------------------------------
int TestObserversPerformance(int, char*[])
{
  bool res = true;
  int maxInvokeCount = 1000;
  int maxEventCount = 100;
  int maxObserverCount = 1000;
  for (int eventCount = 1; eventCount <= maxEventCount; eventCount *= 10)
  {
    if (VERBOSE_MODE & Csv)
    {
      std::cout << eventCount << " events:" << "\n" << ",";
      for (int observerCount = 1; observerCount <= maxObserverCount; observerCount *= 10)
      {
        std::cout << observerCount << ",";
      }
      std::cout << std::endl;
    }
    for (int invokeCount = 1; invokeCount <= maxInvokeCount; invokeCount *= 10)
    {
      if (VERBOSE_MODE & Csv)
      {
        std::cout << invokeCount << ",";
      }
      for (int observerCount = 1; observerCount <= maxObserverCount; observerCount *= 10)
      {
        double time = TestStressInvoke(observerCount, eventCount, invokeCount);
        if (VERBOSE_MODE & Csv)
        {
          std::cout << time << ",";
        }
      }
      if (VERBOSE_MODE & Csv)
      {
        std::cout << std::endl;
      }
    }
  }
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

//------------------------------------------------------------------------------
double StressInvoke(const int observerCount, const int eventCount, const int invokeCount)
{
  if (VERBOSE_MODE & Details)
  {
    std::cout << "StressInvoke " << invokeCount << " invokes "
              << "on " << eventCount << " events observed by "
              << observerCount / eventCount << " observers each." << std::endl;
  }
  vtkObject* volcano = vtkObject::New();
  std::vector<vtkSmartPointer<vtkSimpleCommand> > observers;
  vtkNew<vtkTimerLog> totalTimer;
  vtkNew<vtkTimerLog> addTimer;
  vtkNew<vtkTimerLog> invokeTimer;
  vtkNew<vtkTimerLog> removeTimer;
  totalTimer->StartTimer();
  addTimer->StartTimer();
  for (int observerIDX = 0; observerIDX < observerCount; observerIDX += eventCount)
  {
    for (int event = 0; event < eventCount; ++event)
    {
      vtkNew<vtkSimpleCommand> observer;
      volcano->AddObserver(event + 1000, observer.GetPointer());
      observers.push_back(observer.GetPointer());
    }
  }
  addTimer->StopTimer();
  invokeTimer->StartTimer();
  for (int invoke = 0; invoke < invokeCount; invoke += eventCount)
  {
    for (int event = 0; event < eventCount; ++event)
    {
      volcano->InvokeEvent(event + 1000);
    }
  }
  invokeTimer->StopTimer();
  removeTimer->StartTimer();
  observers.clear();
  volcano->Delete();
  removeTimer->StopTimer();
  totalTimer->StopTimer();
  if (VERBOSE_MODE & Details)
  {
    std::cout << "     Add: " << addTimer->GetElapsedTime() << " seconds" << "\n"
              << "     Invoke: " << invokeTimer->GetElapsedTime() << " seconds" << "\n"
              << "     Remove: " << removeTimer->GetElapsedTime() << " seconds" << "\n"
              << ">>>> Total: " << totalTimer->GetElapsedTime() << " seconds" << "\n";
  }
  return totalTimer->GetElapsedTime();
}

//------------------------------------------------------------------------------
double TestStressInvoke(int observerCount, int eventCount, int invokeCount)
{
  double meanDuration = 0.0;
  for (int i = 0; i < STRESS_COUNT; ++i)
  {
    meanDuration += StressInvoke(observerCount, eventCount, invokeCount);
  }
  meanDuration /= STRESS_COUNT;
  if (VERBOSE_MODE == CDash)
  {
    std::cout << "<DartMeasurement name=\"StressInvoke-"
              << observerCount << "-" << eventCount << "-" << invokeCount
              << "\" type=\"numeric/double\">"
              << meanDuration << "</DartMeasurement>" << std::endl;
  }
  return meanDuration;
}
