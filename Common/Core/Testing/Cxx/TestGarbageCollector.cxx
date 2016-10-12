/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGarbageCollector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCallbackCommand.h"
#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkObject.h"
#include "vtkSmartPointer.h"

// A class that simulates a reference loop and participates in garbage
// collection.
class vtkTestReferenceLoop: public vtkObject
{
public:
  static vtkTestReferenceLoop* New()
  {
    vtkTestReferenceLoop *ret = new vtkTestReferenceLoop;
    ret->InitializeObjectBase();
    return ret;
  }
  vtkTypeMacro(vtkTestReferenceLoop, vtkObject);

  void Register(vtkObjectBase* o) VTK_OVERRIDE { this->RegisterInternal(o, 1); }
  void UnRegister(vtkObjectBase* o) VTK_OVERRIDE { this->UnRegisterInternal(o, 1); }

protected:
  vtkTestReferenceLoop()
  {
    this->Other = new vtkTestReferenceLoop(this);
    this->Other->InitializeObjectBase();
  }
  vtkTestReferenceLoop(vtkTestReferenceLoop* other)
  {
    this->Other = other;
    this->Other->Register(this);
  }
  ~vtkTestReferenceLoop() VTK_OVERRIDE
  {
    if(this->Other)
    {
      this->Other->UnRegister(this);
      this->Other = 0;
    }
  }

  void ReportReferences(vtkGarbageCollector* collector) VTK_OVERRIDE
  {
    vtkGarbageCollectorReport(collector, this->Other, "Other");
  }

  vtkTestReferenceLoop* Other;

private:
  vtkTestReferenceLoop(const vtkTestReferenceLoop&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTestReferenceLoop&) VTK_DELETE_FUNCTION;
};

// A callback that reports when it is called.
static int called = 0;
static void MyDeleteCallback(vtkObject*, unsigned long, void*, void*)
{
  called = 1;
}

// Main test function.
int TestGarbageCollector(int,char *[])
{
  // Create a callback that reports when it is called.
  vtkSmartPointer<vtkCallbackCommand> cc =
    vtkSmartPointer<vtkCallbackCommand>::New();
  cc->SetCallback(MyDeleteCallback);

  // Create an object and delete it immediately.  It should be
  // immediately collected.
  vtkTestReferenceLoop* obj = vtkTestReferenceLoop::New();
  obj->AddObserver(vtkCommand::DeleteEvent, cc);
  called = 0;
  obj->Delete();
  if(!called)
  {
    cerr << "Object not immediately collected." << endl;
    return 1;
  }

  // Create an object, enable deferred collection, and delete it.  It
  // should not be collected yet.
  obj = vtkTestReferenceLoop::New();
  obj->AddObserver(vtkCommand::DeleteEvent, cc);
  vtkGarbageCollector::DeferredCollectionPush();
  called = 0;
  obj->Delete();
  if(called)
  {
    cerr << "Object collection not deferred." << endl;
    return 1;
  }

  // Disable deferred collection.  The object should be deleted now.
  vtkGarbageCollector::DeferredCollectionPop();
  if(!called)
  {
    cerr << "Deferred collection did not collect object." << endl;
    return 1;
  }

  return 0;
}
