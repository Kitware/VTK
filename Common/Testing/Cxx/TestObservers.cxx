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
// .NAME Test of Observers.
// .SECTION Description
// Tests vtkObject::AddObserver templated API

#include "vtkObjectFactory.h"
#include <vtkstd/map>

class vtkHandler : public vtkObject
{
public:
  static vtkstd::map<int, int> EventCounts;
  static int VoidEventCounts;
public:
  static vtkHandler* New();
  vtkTypeMacro(vtkHandler, vtkObject);

  void VoidCallback() { this->VoidEventCounts++; }
  void CallbackWithArguments(vtkObject*, unsigned long event, void*)
    {
    this->EventCounts[event]++;
    }
};
vtkStandardNewMacro(vtkHandler);

int vtkHandler::VoidEventCounts = 0;
vtkstd::map<int, int> vtkHandler::EventCounts;

int TestObservers(int, char*[])
{
  vtkHandler* handler = vtkHandler::New();

  vtkObject* volcano = vtkObject::New();
  unsigned long event0 = volcano->AddObserver(
    1000, handler, &vtkHandler::VoidCallback);
  unsigned long event1 = volcano->AddObserver(
    1001, handler, &vtkHandler::CallbackWithArguments);
  unsigned long event2 = volcano->AddObserver(
    1002, handler, &vtkHandler::CallbackWithArguments);

  volcano->InvokeEvent(1000);
  volcano->InvokeEvent(1001);
  volcano->InvokeEvent(1002);

  // let's see if removing an observer works
  volcano->RemoveObserver(event2);
  volcano->InvokeEvent(1000);
  volcano->InvokeEvent(1001);
  volcano->InvokeEvent(1002);

  // now delete the observer, we shouldn't have any dangling pointers.
  handler->Delete();

  volcano->InvokeEvent(1000);
  volcano->InvokeEvent(1001);
  volcano->InvokeEvent(1002);

  // remove an observer after the handler has been deleted, should work.
  volcano->RemoveObserver(event1);
  volcano->InvokeEvent(1000);
  volcano->InvokeEvent(1001);
  volcano->InvokeEvent(1002);

  volcano->Delete();

  if (vtkHandler::VoidEventCounts == 2 &&
    vtkHandler::EventCounts[1000] == 0 &&
    vtkHandler::EventCounts[1001] == 2 &&
    vtkHandler::EventCounts[1002] == 1)
    {
    cout << "All callback counts as expected." << endl;
    return 0;
    }
  cerr << "Mismatched callback counts" << endl;
  return 1;
}
