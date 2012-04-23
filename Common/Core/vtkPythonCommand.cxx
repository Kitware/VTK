/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonCommand.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPythonCommand.h"
#include "vtkPythonUtil.h"
#include "vtkObject.h"


vtkPythonCommand::vtkPythonCommand()
{
  this->obj = NULL;
  this->ThreadState = NULL;
}

vtkPythonCommand::~vtkPythonCommand()
{
  if (this->obj && Py_IsInitialized())
    {
    Py_DECREF(this->obj);
    }
  this->obj = NULL;
}

void vtkPythonCommand::SetObject(PyObject *o)
{
  this->obj = o;
}

void vtkPythonCommand::SetThreadState(PyThreadState *ts)
{
  this->ThreadState = ts;
}

void vtkPythonCommand::Execute(vtkObject *ptr, unsigned long eventtype,
                               void *CallData)
{
  PyObject *arglist, *result, *obj2;
  const char *eventname;

  // Sometimes it is possible for the command to be invoked after
  // Py_Finalize is called, this will cause nasty errors so we return if
  // the interpreter is not initialized.
  if (Py_IsInitialized() == 0)
    {
    return;
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_STATE state = PyGILState_Ensure();
#endif
#endif

  // If a threadstate has been set using vtkPythonCommand::SetThreadState,
  // then swap it in here.  See the email to vtk-developers@vtk.org from
  // June 18, 2009 with subject "Py_NewInterpreter and vtkPythonCallback issue"
  PyThreadState* prevThreadState = NULL;
  if (this->ThreadState)
    {
    prevThreadState = PyThreadState_Swap(this->ThreadState);
    }

  if (ptr && ptr->GetReferenceCount() > 0)
    {
    obj2 = vtkPythonUtil::GetObjectFromPointer(ptr);
    }
  else
    {
    Py_INCREF(Py_None);
    obj2 = Py_None;
    }

  eventname = this->GetStringFromEventId(eventtype);

  // extension by Charl P. Botha so that CallData is available from Python:
  // * CallData used to be ignored completely: this is not entirely desirable,
  //   e.g. with catching ErrorEvent
  // * I have extended this code so that CallData can be caught whilst not
  //   affecting any existing VTK Python code
  // * make sure your observer python function has a CallDataType string
  //   attribute that describes how CallData should be passed through, e.g.:
  //   def handler(theObject, eventType, message):
  //      print "Error: %s" % (message)
  //   # we know that ErrorEvent passes a null-terminated string
  //   handler.CallDataType = "string0"
  //   someObject.AddObserver('ErrorEvent', handler)
  // * At the moment, only string0 is supported as that is what ErrorEvent
  //   generates.
  //
  char CallDataTypeLiteral[] = "CallDataType"; // Need char*, not const char*.
  PyObject *CallDataTypeObj = PyObject_GetAttrString(this->obj,
                                                     CallDataTypeLiteral);
  char *CallDataTypeString = NULL;
  if (CallDataTypeObj)
    {
    CallDataTypeString = PyString_AsString(CallDataTypeObj);
    if (CallDataTypeString)
        {
        if (strcmp(CallDataTypeString, "string0") == 0)
            {
            // this means the user wants the CallData cast as a string
            PyObject* CallDataAsString = PyString_FromString((char*)CallData);
            if (CallDataAsString)
                {
                arglist = Py_BuildValue((char*)"(NsN)", obj2, eventname, CallDataAsString);
                }
            else
                {
                PyErr_Clear();
                // we couldn't create a string, so we pass in None
                Py_INCREF(Py_None);
                arglist = Py_BuildValue((char*)"(NsN)", obj2, eventname, Py_None);
                }
            }
        else
            {
            // we don't handle this, so we pass in a None as the third parameter
            Py_INCREF(Py_None);
            arglist = Py_BuildValue((char*)"(NsN)", obj2, eventname, Py_None);
            }
        }
    else
        {
        // the handler object has a CallDataType attribute, but it's not a
        // string -- then we do traditional arguments
        arglist = Py_BuildValue((char*)"(Ns)",obj2,eventname);
        }

    // we have to do this
    Py_DECREF(CallDataTypeObj);
    }
  else
    {
    // this means there was no CallDataType attribute, so we do the
    // traditional obj(object, eventname) call
    PyErr_Clear();
    arglist = Py_BuildValue((char*)"(Ns)",obj2,eventname);
    }

  result = PyEval_CallObject(this->obj, arglist);
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

  // If we did the swap near the top of this function then swap back now.
  if (this->ThreadState)
    {
    PyThreadState_Swap(prevThreadState);
    }

#ifndef VTK_NO_PYTHON_THREADS
#if (PY_MAJOR_VERSION > 2) || \
((PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION >= 3))
  PyGILState_Release(state);
#endif
#endif
}
