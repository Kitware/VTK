// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of Observers.
// .SECTION Description
// Tests vtkObject::AddObserver templated API

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include <map>

class vtkHandler : public vtkObject
{
public:
  static std::map<int, int> EventCounts;
  static int VoidEventCounts;

  static vtkHandler* New();
  vtkTypeMacro(vtkHandler, vtkObject);

  void VoidCallback() { vtkHandler::VoidEventCounts++; }
  void CallbackWithArguments(vtkObject*, unsigned long event, void*)
  {
    vtkHandler::EventCounts[event]++;
  }
};
vtkStandardNewMacro(vtkHandler);

int vtkHandler::VoidEventCounts = 0;
std::map<int, int> vtkHandler::EventCounts;

class OtherHandler
{
public:
  static std::map<int, int> EventCounts;
  static int VoidEventCounts;

  void VoidCallback() { OtherHandler::VoidEventCounts++; }
  void CallbackWithArguments(vtkObject*, unsigned long event, void*)
  {
    OtherHandler::EventCounts[event]++;
  }
};

int OtherHandler::VoidEventCounts = 0;
std::map<int, int> OtherHandler::EventCounts;

class NestedHandler1
{
public:
  void CallbackWithArguments(vtkObject* self, unsigned long, void*) { self->InvokeEvent(1001); }
};
class NestedHandler2
{
public:
  void CallbackWithArguments(vtkObject* self, unsigned long, void*) { self->RemoveAllObservers(); }
};

int TestObservers(int, char*[])
{
  unsigned long event0 = 0;
  unsigned long event1 = 0;
  unsigned long event2 = 0;

  vtkObject* volcano = vtkObject::New();

  // Test nested callbacks invalidating iteration of observers
  // This will seg fault if the iterators are not handled properly
  NestedHandler1* handlerNested1 = new NestedHandler1();
  event0 = volcano->AddObserver(1000, handlerNested1, &NestedHandler1::CallbackWithArguments);
  NestedHandler2* handlerNested2 = new NestedHandler2();
  event1 = volcano->AddObserver(1001, handlerNested2, &NestedHandler2::CallbackWithArguments);
  volcano->InvokeEvent(1000);
  delete handlerNested1;
  delete handlerNested2;

  // Handle the base test, with a vtkObject pointer
  vtkHandler* handler = vtkHandler::New();

  event0 = volcano->AddObserver(1000, handler, &vtkHandler::VoidCallback);
  event1 = volcano->AddObserver(1001, handler, &vtkHandler::CallbackWithArguments);
  event2 = volcano->AddObserver(1002, handler, &vtkHandler::CallbackWithArguments);

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

  // remove the final observer
  volcano->RemoveObserver(event0);

  if (vtkHandler::VoidEventCounts == 2 && vtkHandler::EventCounts[1000] == 0 &&
    vtkHandler::EventCounts[1001] == 2 && vtkHandler::EventCounts[1002] == 1)
  {
    cout << "All vtkObject callback counts as expected." << endl;
  }
  else
  {
    cerr << "Mismatched callback counts for VTK observer." << endl;
    volcano->Delete();
    return 1;
  }

  // ---------------------------------
  // Test again, with smart pointers
  vtkHandler::VoidEventCounts = 0;

  // Make a scope for the smart pointer
  {
    vtkSmartPointer<vtkHandler> handler2 = vtkSmartPointer<vtkHandler>::New();

    event0 = volcano->AddObserver(1003, handler2, &vtkHandler::VoidCallback);
    event1 = volcano->AddObserver(1004, handler2, &vtkHandler::CallbackWithArguments);
    event2 = volcano->AddObserver(1005, handler2, &vtkHandler::CallbackWithArguments);

    volcano->InvokeEvent(1003);
    volcano->InvokeEvent(1004);
    volcano->InvokeEvent(1005);

    // let's see if removing an observer works
    volcano->RemoveObserver(event2);
    volcano->InvokeEvent(1003);
    volcano->InvokeEvent(1004);
    volcano->InvokeEvent(1005);

    // end the scope, which deletes the observer
  }

  // continue invoking, to make sure that
  // no events to to the deleted observer
  volcano->InvokeEvent(1003);
  volcano->InvokeEvent(1004);
  volcano->InvokeEvent(1005);

  // remove an observer after the handler2 has been deleted, should work.
  volcano->RemoveObserver(event1);
  volcano->InvokeEvent(1003);
  volcano->InvokeEvent(1004);
  volcano->InvokeEvent(1005);

  // remove the final observer
  volcano->RemoveObserver(event0);

  if (vtkHandler::VoidEventCounts == 2 && vtkHandler::EventCounts[1003] == 0 &&
    vtkHandler::EventCounts[1004] == 2 && vtkHandler::EventCounts[1005] == 1)
  {
    cout << "All smart pointer callback counts as expected." << endl;
  }
  else
  {
    cerr << "Mismatched callback counts for smart pointer observer." << endl;
    volcano->Delete();
    return 1;
  }

  // ---------------------------------
  // Test yet again, this time with a non-VTK object
  // (this _can_ leave dangling pointers!!!)

  OtherHandler* handler3 = new OtherHandler();

  event0 = volcano->AddObserver(1006, handler3, &OtherHandler::VoidCallback);
  event1 = volcano->AddObserver(1007, handler3, &OtherHandler::CallbackWithArguments);
  event2 = volcano->AddObserver(1008, handler3, &OtherHandler::CallbackWithArguments);

  volcano->InvokeEvent(1006);
  volcano->InvokeEvent(1007);
  volcano->InvokeEvent(1008);

  // let's see if removing an observer works
  volcano->RemoveObserver(event2);
  volcano->InvokeEvent(1006);
  volcano->InvokeEvent(1007);
  volcano->InvokeEvent(1008);

  // if we delete this non-vtkObject observer, we will
  // have dangling pointers and will see a crash...
  // so let's not do that until the events are removed

  volcano->RemoveObserver(event0);
  volcano->RemoveObserver(event1);
  delete handler3;

  // delete the observed object
  volcano->Delete();

  if (OtherHandler::VoidEventCounts == 2 && OtherHandler::EventCounts[1006] == 0 &&
    OtherHandler::EventCounts[1007] == 2 && OtherHandler::EventCounts[1008] == 1)
  {
    cout << "All non-VTK observer callback counts as expected." << endl;
    return 0;
  }

  cerr << "Mismatched callback counts for non-VTK observer." << endl;
  return 1;
}
