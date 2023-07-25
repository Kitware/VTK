// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPythonCommand.h"
#include "vtkABINamespace.h"
#include "vtkObject.h"
#include "vtkPythonUtil.h"

VTK_ABI_NAMESPACE_BEGIN
vtkPythonCommand::vtkPythonCommand()
{
  this->obj = nullptr;
  this->ThreadState = nullptr;
  vtkPythonUtil::RegisterPythonCommand(this);
}

vtkPythonCommand::~vtkPythonCommand()
{
  vtkPythonUtil::UnRegisterPythonCommand(this);
  if (this->obj && Py_IsInitialized())
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    Py_DECREF(this->obj);
  }
  this->obj = nullptr;
}

void vtkPythonCommand::SetObject(PyObject* o)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  Py_INCREF(o);
  this->obj = o;
}

void vtkPythonCommand::SetThreadState(PyThreadState* ts)
{
  this->ThreadState = ts;
}

namespace
{
PyObject* BuildCallDataArgList(
  PyObject* caller, const char* eventname, PyObject* callDataAsPyObject)
{
  PyObject* arglist;
  if (callDataAsPyObject)
  {
    arglist = Py_BuildValue("(NsN)", caller, eventname, callDataAsPyObject);
  }
  else
  {
    PyErr_Clear();
    /* we couldn't create a the expected python object, so we pass in None */
    Py_INCREF(Py_None);
    arglist = Py_BuildValue("(NsN)", caller, eventname, Py_None);
  }
  return arglist;
}
}

void vtkPythonCommand::Execute(vtkObject* ptr, unsigned long eventtype, void* callData)
{
  if (!this->obj)
  {
    return;
  }

  // Sometimes it is possible for the command to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
  {
    return;
  }

#ifndef VTK_NO_PYTHON_THREADS
  vtkPythonScopeGilEnsurer gilEnsurer(true);
#else
  // We only need to do this if we are not calling PyGILState_Ensure(), in fact
  // the code below is not safe if not executed on the same thread that the
  // AddObserver call was made on, as we will end up swapping in the wrong
  // thread state.
  //
  // If a threadstate has been set using vtkPythonCommand::SetThreadState,
  // then swap it in here.  See the email to vtk-developers@vtk.org from
  // June 18, 2009 with subject "Py_NewInterpreter and vtkPythonCallback issue"
  PyThreadState* prevThreadState = nullptr;
  if (this->ThreadState)
  {
    prevThreadState = PyThreadState_Swap(this->ThreadState);
  }
#endif

  PyObject* obj2 = nullptr;
  if (eventtype != vtkCommand::DeleteEvent && ptr && ptr->GetReferenceCount() > 0)
  {
    obj2 = vtkPythonUtil::GetObjectFromPointer(ptr);
  }
  else
  {
    Py_INCREF(Py_None);
    obj2 = Py_None;
  }

  const char* eventname = vtkPythonCommand::GetStringFromEventId(eventtype);

  // extension by Charl P. Botha so that callData is available from Python:
  // * callData used to be ignored completely: this is not entirely desirable,
  //   e.g. with catching ErrorEvent
  // * I have extended this code so that callData can be caught whilst not
  //   affecting any existing VTK Python code
  // * make sure your observer python function has a CallDataType string
  //   attribute that describes how callData should be passed through, e.g.:
  //   def handler(theObject, eventType, message):
  //      print "Error: %s" % (message)
  //   # we know that ErrorEvent passes a null-terminated string
  //   handler.CallDataType = "string0"
  //   someObject.AddObserver('ErrorEvent', handler)
  //
  // support for additional types has then been added by Jean-Christophe Fillion-Robin
  //
  const char* callDataTypeLiteral = "CallDataType"; // Need char*, not const char*.
  PyObject* callDataTypeObj = PyObject_GetAttrString(this->obj, callDataTypeLiteral);

  PyObject* arglist = nullptr;
  if (callData && callDataTypeObj)
  {
    if (PyLong_Check(callDataTypeObj))
    {
      long callDataTypeLong = PyLong_AsLong(callDataTypeObj);
      int invalid = (callDataTypeLong == -1) && PyErr_Occurred();
      if (!invalid)
      {
        if (callDataTypeLong == VTK_STRING)
        {
          // this means the user wants the callData cast as a string
          PyObject* callDataAsString = PyUnicode_FromString(reinterpret_cast<char*>(callData));
          arglist = BuildCallDataArgList(obj2, eventname, callDataAsString);
        }
        else if (callDataTypeLong == VTK_OBJECT)
        {
          // this means the user wants the callData cast as a vtkObject
          PyObject* callDataAsVTKObject =
            vtkPythonUtil::GetObjectFromPointer(reinterpret_cast<vtkObject*>(callData));
          arglist = BuildCallDataArgList(obj2, eventname, callDataAsVTKObject);
        }
        else if (callDataTypeLong == VTK_INT)
        {
          // this means the user wants the callData cast as an int
          PyObject* callDataAsInt = PyLong_FromLong(*reinterpret_cast<int*>(callData));
          arglist = BuildCallDataArgList(obj2, eventname, callDataAsInt);
        }
        else if (callDataTypeLong == VTK_LONG)
        {
          // this means the user wants the callData cast as a long
          PyObject* callDataAsInt = PyLong_FromLong(*reinterpret_cast<long*>(callData));
          arglist = BuildCallDataArgList(obj2, eventname, callDataAsInt);
        }
        else if (callDataTypeLong == VTK_DOUBLE)
        {
          // this means the user wants the callData cast as a double
          PyObject* callDataAsInt = PyFloat_FromDouble(*reinterpret_cast<double*>(callData));
          arglist = BuildCallDataArgList(obj2, eventname, callDataAsInt);
        }
        else if (callDataTypeLong == VTK_FLOAT)
        {
          // this means the user wants the callData cast as a float
          PyObject* callDataAsInt = PyFloat_FromDouble(*reinterpret_cast<float*>(callData));
          arglist = BuildCallDataArgList(obj2, eventname, callDataAsInt);
        }
      }
      else
      {
        // we don't handle this, so we pass in a None as the third parameter
        Py_INCREF(Py_None);
        arglist = Py_BuildValue("(NsN)", obj2, eventname, Py_None);
      }
    }
    else if (PyUnicode_Check(callDataTypeObj))
    {
      PyObject* bytes = PyUnicode_AsEncodedString(callDataTypeObj, nullptr, nullptr);
      const char* callDataTypeString = nullptr;
      if (bytes)
      {
        callDataTypeString = PyBytes_AsString(bytes);
      }
      if (callDataTypeString)
      {
        if (strcmp(callDataTypeString, "string0") == 0)
        {
          // this means the user wants the callData cast as a string
          PyObject* callDataAsString = PyUnicode_FromString(reinterpret_cast<char*>(callData));
          arglist = BuildCallDataArgList(obj2, eventname, callDataAsString);
        }
      }
      else
      {
        // we don't handle this, so we pass in a None as the third parameter
        Py_INCREF(Py_None);
        arglist = Py_BuildValue("(NsN)", obj2, eventname, Py_None);
      }
      Py_XDECREF(bytes);
    }
    else
    {
      // the handler object has a CallDataType attribute, but it's neither an
      // integer or a string -- then we do traditional arguments
      arglist = Py_BuildValue("(Ns)", obj2, eventname);
    }
    // we have to do this
    Py_DECREF(callDataTypeObj);
  }
  else
  {
    // this means there was no CallDataType attribute, so we do the
    // traditional obj(object, eventname) call
    PyErr_Clear();
    arglist = Py_BuildValue("(Ns)", obj2, eventname);
  }

  PyObject* result = PyObject_Call(this->obj, arglist, nullptr);
  Py_DECREF(arglist);

  if (result)
  {
    Py_DECREF(result);
  }
  else
  {
    if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
    {
      cerr << "Caught a Ctrl-C within python, exiting program.\n";
      Py_Exit(1);
    }
    PyErr_Print();
  }

#ifdef VTK_NO_PYTHON_THREADS
  // If we did the swap near the top of this function then swap back now.
  if (this->ThreadState)
  {
    PyThreadState_Swap(prevThreadState);
  }
#endif
}
VTK_ABI_NAMESPACE_END
