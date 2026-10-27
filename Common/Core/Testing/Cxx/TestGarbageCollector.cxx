// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCallbackCommand.h"
#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkSmartPointer.h"

#include <atomic>
#include <iostream>
#include <thread>

// A class that simulates a reference loop and participates in garbage
// collection.
class vtkTestReferenceLoop : public vtkObject
{
public:
  static vtkTestReferenceLoop* New()
  {
    vtkTestReferenceLoop* ret = new vtkTestReferenceLoop;
    ret->InitializeObjectBase();
    return ret;
  }
  vtkTypeMacro(vtkTestReferenceLoop, vtkObject);

  bool UsesGarbageCollector() const override { return true; }

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
  ~vtkTestReferenceLoop() override
  {
    if (this->Other)
    {
      this->Other->UnRegister(this);
      this->Other = nullptr;
    }
  }

  void ReportReferences(vtkGarbageCollector* collector) override
  {
    vtkGarbageCollectorReport(collector, this->Other, "Other");
  }

  vtkTestReferenceLoop* Other;

private:
  vtkTestReferenceLoop(const vtkTestReferenceLoop&) = delete;
  void operator=(const vtkTestReferenceLoop&) = delete;
};

// A callback that reports when it is called.
static int called = 0;
static void MyDeleteCallback(vtkObject*, unsigned long, void*, void*)
{
  called = 1;
}

// A callback that sets the flag given as client data when it is called.
static void SetFlagCallback(vtkObject*, unsigned long, void* clientData, void*)
{
  static_cast<std::atomic<bool>*>(clientData)->store(true);
}

// Deferred collection is only done in the main thread. In other threads,
// pushing, popping and collecting must neither defer collection nor affect the
// collection deferred in the main thread.
static int TestOtherThread()
{
  std::atomic<bool> mainCollected{ false };
  vtkNew<vtkCallbackCommand> mainCallback;
  mainCallback->SetCallback(SetFlagCallback);
  mainCallback->SetClientData(&mainCollected);

  vtkTestReferenceLoop* mainObj = vtkTestReferenceLoop::New();
  mainObj->AddObserver(vtkCommand::DeleteEvent, mainCallback);
  vtkGarbageCollector::DeferredCollectionPush();
  mainObj->Delete();

  bool otherCollected = false;
  std::thread other(
    [&otherCollected]()
    {
      std::atomic<bool> collected{ false };
      vtkNew<vtkCallbackCommand> callback;
      callback->SetCallback(SetFlagCallback);
      callback->SetClientData(&collected);

      vtkGarbageCollector::DeferredCollectionPush();
      vtkTestReferenceLoop* obj = vtkTestReferenceLoop::New();
      obj->AddObserver(vtkCommand::DeleteEvent, callback);
      obj->Delete();
      otherCollected = collected;
      vtkGarbageCollector::DeferredCollectionPop();
      vtkGarbageCollector::Collect();
    });
  other.join();

  int status = 0;
  if (!otherCollected)
  {
    std::cerr << "Object not immediately collected in another thread." << std::endl;
    status = 1;
  }
  if (mainCollected)
  {
    std::cerr << "Collection deferred in the main thread done by another thread." << std::endl;
    status = 1;
  }

  vtkGarbageCollector::DeferredCollectionPop();
  if (!mainCollected)
  {
    std::cerr << "Deferred collection did not collect object." << std::endl;
    status = 1;
  }
  return status;
}

// Main test function.
int TestGarbageCollector(int, char*[])
{
  // Create a callback that reports when it is called.
  vtkSmartPointer<vtkCallbackCommand> cc = vtkSmartPointer<vtkCallbackCommand>::New();
  cc->SetCallback(MyDeleteCallback);

  // Create an object and delete it immediately.  It should be
  // immediately collected.
  vtkTestReferenceLoop* obj = vtkTestReferenceLoop::New();
  obj->AddObserver(vtkCommand::DeleteEvent, cc);
  called = 0;
  obj->Delete();
  if (!called)
  {
    std::cerr << "Object not immediately collected." << std::endl;
    return 1;
  }

  // Create an object, enable deferred collection, and delete it.  It
  // should not be collected yet.
  obj = vtkTestReferenceLoop::New();
  obj->AddObserver(vtkCommand::DeleteEvent, cc);
  vtkGarbageCollector::DeferredCollectionPush();
  called = 0;
  obj->Delete();
  if (called)
  {
    std::cerr << "Object collection not deferred." << std::endl;
    return 1;
  }

  // Disable deferred collection.  The object should be deleted now.
  vtkGarbageCollector::DeferredCollectionPop();
  if (!called)
  {
    std::cerr << "Deferred collection did not collect object." << std::endl;
    return 1;
  }

  return TestOtherThread();
}
