/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonAlgorithm.h"
#include "vtkObjectFactory.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPythonUtil.h"

vtkStandardNewMacro(vtkPythonAlgorithm);

void vtkPythonAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  PyObject* str = NULL;
  if (this->Object)
    {
    str = PyObject_Str(this->Object);
    }

  os << indent << "Object: " << Object << std::endl;
  if (str)
    {
    os << indent << "Object (string): " << PyString_AsString(str) << std::endl;
    Py_DECREF(str);
    }
}

vtkPythonAlgorithm::vtkPythonAlgorithm()
{
  this->Object = NULL;
}

vtkPythonAlgorithm::~vtkPythonAlgorithm()
{
  Py_XDECREF(this->Object);
}

#define VTK_GET_METHOD(var, obj, method, failValue)    \
  if (!obj)                                            \
    {                                                  \
    return failValue;                                  \
    }                                                  \
  PyObject* var = PyObject_GetAttrString(obj, method); \
  if (!var)                                            \
    {                                                  \
    return failValue;                                  \
    }                                                  \
  if (!PyCallable_Check(var))                          \
    {                                                  \
    Py_DECREF(var);                                    \
    return failValue;                                  \
    }

static PyObject* VTKToPython(vtkObjectBase* obj)
{
  return vtkPythonUtil::GetObjectFromPointer(obj);
}

static std::string GetPythonErrorString()
{
  PyObject* type;
  PyObject* value;
  PyObject* traceback;

  // Increments refcounts for returns.
  PyErr_Fetch(&type, &value, &traceback);

  if (!type)
    {
    return "No error from Python?!";
    }

  PyObject* pyexc_string = PyObject_Str(value);
  std::string exc_string;
  if (pyexc_string)
    {
    exc_string = PyString_AsString(pyexc_string);
    Py_DECREF(pyexc_string);
    }
  else
    {
    exc_string = "<Unable to convert Python error to string>";
    }

  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(traceback);

  PyErr_Clear();

  return exc_string;
}

int vtkPythonAlgorithm::CheckResult(const char* method, PyObject* res)
{
  if (!res)
    {
    std::string pymsg = GetPythonErrorString();
    vtkErrorMacro("Failure when calling method: \""
      << method << "\": " << pymsg << ".");
    return 0;
    }
  if (!PyInt_Check(res))
    {
    Py_DECREF(res);
    return 0;
    }

  int code = PyInt_AsLong(res);
  Py_DECREF(res);

  return code;
}

void vtkPythonAlgorithm::SetPythonObject(PyObject* obj)
{
  if (!obj)
    {
    return;
    }

  Py_XDECREF(this->Object);

  this->Object = obj;
  Py_INCREF(this->Object);

  char mname[] = "Initialize";
  VTK_GET_METHOD(method, this->Object, mname, /* no return */)

  PyObject* args = PyTuple_New(1);

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args, 0, vtkself);

  PyObject* result = PyObject_Call(method, args, NULL);
  Py_DECREF(args);
  Py_DECREF(method);

  CheckResult(mname, result);
}

void vtkPythonAlgorithm::SetNumberOfInputPorts(int n)
{
  this->Superclass::SetNumberOfInputPorts(n);
}

void vtkPythonAlgorithm::SetNumberOfOutputPorts(int n)
{
  this->Superclass::SetNumberOfOutputPorts(n);
}

int vtkPythonAlgorithm::ProcessRequest(vtkInformation* request,
                                       vtkInformationVector** inInfo,
                                       vtkInformationVector* outInfo)
{
  char mname[] = "ProcessRequest";
  VTK_GET_METHOD(method, this->Object, mname, 0)

  PyObject* args = PyTuple_New(4);

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args, 0, vtkself);

  PyObject* pyrequest = VTKToPython(request);
  PyTuple_SET_ITEM(args, 1, pyrequest);

  int nports = this->GetNumberOfInputPorts();
  PyObject* pyininfos = PyTuple_New(nports);
  for (int i = 0; i < nports; ++i)
    {
    PyObject* pyininfo = VTKToPython(inInfo[i]);
    PyTuple_SET_ITEM(pyininfos, i, pyininfo);
    }
  PyTuple_SET_ITEM(args, 2, pyininfos);

  PyObject* pyoutinfo = VTKToPython(outInfo);
  PyTuple_SET_ITEM(args, 3, pyoutinfo);

  PyObject* result = PyObject_Call(method, args, NULL);
  Py_DECREF(method);
  Py_DECREF(args);

  return CheckResult(mname, result);
}

int vtkPythonAlgorithm::FillInputPortInformation(int port, vtkInformation* info)
{
  char mname[] = "FillInputPortInformation";
  VTK_GET_METHOD(method, this->Object, mname, 0)

  PyObject* args = PyTuple_New(3);

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args, 0, vtkself);

  PyObject* pyport = PyInt_FromLong(port);
  PyTuple_SET_ITEM(args, 1, pyport);

  PyObject* pyinfo = VTKToPython(info);
  PyTuple_SET_ITEM(args, 2, pyinfo);

  PyObject* result = PyObject_Call(method, args, NULL);
  Py_DECREF(method);
  Py_DECREF(args);

  return CheckResult(mname, result);
}

int vtkPythonAlgorithm::FillOutputPortInformation(int port, vtkInformation* info)
{
  char mname[] = "FillOutputPortInformation";
  VTK_GET_METHOD(method, this->Object, mname, 0)

  PyObject* args = PyTuple_New(3);

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args, 0, vtkself);

  PyObject* pyport = PyInt_FromLong(port);
  PyTuple_SET_ITEM(args, 1, pyport);

  PyObject* pyinfo = VTKToPython(info);
  PyTuple_SET_ITEM(args, 2, pyinfo);

  PyObject* result = PyObject_Call(method, args, NULL);
  Py_DECREF(method);
  Py_DECREF(args);

  return CheckResult(mname, result);
}
