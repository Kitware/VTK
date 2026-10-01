// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// A Python wrapper that is discarded while its VTK object is still alive is
// kept as a ghost by vtkPythonUtil, and a DeleteEvent observer evicts the
// ghost when the object is deleted.  This test covers the cases in which that
// observer does not run: other C++ code removed it, or a DeleteEvent observer
// with a higher priority aborted the event.  The ghost must then be released
// by the check that vtkPythonUtil makes when the next ghost is added.  It also
// checks that a ghosted object can be deleted after the interpreter is gone.

#include "vtkPythonInterpreter.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"
#include "vtkWeakPointer.h"

#include <iostream>

namespace
{

PyObject* MainDict = nullptr;
int Failures = 0;

void Check(bool condition, const char* what)
{
  if (!condition)
  {
    std::cerr << "FAIL: " << what << std::endl;
    ++Failures;
  }
}

void Run(const char* code)
{
  vtkSmartPyObject result;
  result.TakeReference(PyRun_String(code, Py_file_input, MainDict, MainDict));
  if (!result)
  {
    PyErr_Print();
    ++Failures;
  }
}

bool Eval(const char* expression)
{
  vtkSmartPyObject result;
  result.TakeReference(PyRun_String(expression, Py_eval_input, MainDict, MainDict));
  if (!result)
  {
    PyErr_Print();
    ++Failures;
    return false;
  }
  return PyObject_IsTrue(result) == 1;
}

// The object held by the vtkVariantArray with this name in __main__
vtkObject* HeldObject(const char* name)
{
  PyObject* holder = PyDict_GetItemString(MainDict, name);
  vtkVariantArray* array =
    vtkVariantArray::SafeDownCast(vtkPythonUtil::GetPointerFromObject(holder, "vtkVariantArray"));
  return array ? vtkObject::SafeDownCast(array->GetValue(0).ToVTKObject()) : nullptr;
}

void NoOp(vtkObject*, unsigned long, void*, void*) {}

}

int TestPythonGhostObserverRemoval(int, char*[])
{
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer(true, true);
  MainDict = PyModule_GetDict(PyImport_AddModule("__main__"));

  Run("import gc, weakref\n"
      "from vtkmodules.vtkCommonCore import vtkObject, vtkVariant, vtkVariantArray\n"
      "\n"
      "class Payload:\n"
      "    pass\n"
      "\n"
      "def ghost(name):\n"
      "    # A vtkObject with a Python attribute, held only by a C++ container,\n"
      "    # whose wrapper is discarded: vtkPythonUtil keeps its dict as a ghost\n"
      "    o = vtkObject()\n"
      "    o.payload = Payload()\n"
      "    ref = weakref.ref(o.payload)\n"
      "    holder = vtkVariantArray()\n"
      "    holder.InsertNextValue(vtkVariant(o))\n"
      "    globals()[name] = holder\n"
      "    return ref\n"
      "\n"
      "def add_ghost():\n"
      "    # Adding a ghost checks the ghosts that have no DeleteEvent observer\n"
      "    holder = vtkVariantArray()\n"
      "    o = vtkObject()\n"
      "    o.x = 1\n"
      "    holder.InsertNextValue(vtkVariant(o))\n"
      "    del o\n"
      "    gc.collect()\n");

  // Other C++ code removes the observer from a ghosted object
  Run("ref1 = ghost('holder1')");
  vtkObject* object1 = HeldObject("holder1");
  Check(object1 && object1->HasObserver(vtkCommand::DeleteEvent), "ghost has an observer");
  object1->RemoveAllObservers();
  Check(Eval("holder1.GetValue(0).ToVTKObject().payload is ref1()"),
    "a ghost without an observer keeps its attributes");
  Check(
    object1->HasObserver(vtkCommand::DeleteEvent), "discarding the wrapper again adds an observer");
  object1->RemoveAllObservers();
  vtkWeakPointer<vtkObject> weak1 = object1;
  Run("holder1.SetValue(0, vtkVariant())\n"
      "gc.collect()\n");
  Check(weak1.GetPointer() == nullptr, "the object without an observer was deleted");
  Check(Eval("ref1() is not None"), "its ghost is kept until a ghost is added");
  Run("add_ghost()");
  Check(Eval("ref1() is None"), "adding a ghost released the ghost without an observer");

  // A DeleteEvent observer with a higher priority aborts the event, so the
  // observer that evicts the ghost does not run
  Run("ref2 = ghost('holder2')");
  vtkObject* object2 = HeldObject("holder2");
  Check(object2 && object2->HasObserver(vtkCommand::DeleteEvent), "second ghost has an observer");
  vtkNew<vtkCallbackCommand> abort;
  abort->SetCallback(NoOp);
  abort->SetAbortFlagOnExecute(1);
  object2->AddObserver(vtkCommand::DeleteEvent, abort, 1.0f);
  vtkWeakPointer<vtkObject> weak2 = object2;
  Run("holder2.SetValue(0, vtkVariant())\n"
      "gc.collect()\n");
  Check(weak2.GetPointer() == nullptr, "the object with the aborting observer was deleted");
  Run("add_ghost()");
  Check(Eval("ref2() is None"), "adding a ghost released the ghost whose eviction was aborted");

  // A ghosted object that outlives the interpreter
  Run("ref3 = ghost('holder3')");
  vtkObject* object3 = HeldObject("holder3");
  Check(object3 && object3->HasObserver(vtkCommand::DeleteEvent), "third ghost has an observer");
  object3->Register(nullptr);
  Run("del holder3");

  vtkPythonInterpreter::Finalize();
  object3->UnRegister(nullptr);

  return Failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
