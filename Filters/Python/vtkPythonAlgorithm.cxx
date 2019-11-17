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
#include "vtkSmartPyObject.h"

vtkStandardNewMacro(vtkPythonAlgorithm);

void vtkPythonAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject str;
  if (this->Object)
  {
    str.TakeReference(PyObject_Str(this->Object));
  }

  os << indent << "Object: " << Object << std::endl;
  if (str)
  {
    os << indent << "Object (string): ";
#ifndef VTK_PY3K
    os << PyString_AsString(str);
#else
    PyObject* bytes = PyUnicode_EncodeLocale(str, VTK_PYUNICODE_ENC);
    if (bytes)
    {
      os << PyBytes_AsString(bytes);
      Py_DECREF(bytes);
    }
#endif
    os << std::endl;
  }
}

vtkPythonAlgorithm::vtkPythonAlgorithm()
{
  this->Object = nullptr;
}

vtkPythonAlgorithm::~vtkPythonAlgorithm()
{
  // we check if Python is still initialized since the Python interpreter may
  // have been finalized before the VTK object is released.
  if (Py_IsInitialized())
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    Py_XDECREF(this->Object);
  }
}

// This macro gets the method passed in as the parameter method
// from the PyObject passed in as the parameter obj and creates a
// vtkSmartPyObject variable with the name passed in as the parameter
// var containing that method's PyObject.  If obj is nullptr, obj.method
// does not exist or obj.method is not a callable method, this macro
// causes the function using it to return with the return value
// passed in as the parameter failValue
//    var - the name of the resulting vtkSmartPyObject with the
//          method object in it.  Can be used in the code following
//          the macro's use as the variable name
//    obj - the PyObject to get the method from
//    method - the name of the method to look for.  Should be a
//          C string.
//    failValue - the value to return if the lookup fails and the
//          function using the macro should return.  Pass in a
//          block comment /**/ for void functions using this macro
#define VTK_GET_METHOD(var, obj, method, failValue)                                                \
  if (!(obj))                                                                                      \
  {                                                                                                \
    return failValue;                                                                              \
  }                                                                                                \
  vtkSmartPyObject var(PyObject_GetAttrString(obj, method));                                       \
  if (!(var))                                                                                      \
  {                                                                                                \
    return failValue;                                                                              \
  }                                                                                                \
  if (!PyCallable_Check(var))                                                                      \
  {                                                                                                \
    return failValue;                                                                              \
  }

/// Return value: New reference.
static PyObject* VTKToPython(vtkObjectBase* obj)
{
  return vtkPythonUtil::GetObjectFromPointer(obj);
}

int vtkPythonAlgorithm::CheckResult(const char* method, const vtkSmartPyObject& res)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (!res)
  {
    vtkErrorMacro("Failure when calling method: \"" << method << "\":");
    if (PyErr_Occurred() != nullptr)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    return 0;
  }
  if (!PyInt_Check(res))
  {
    return 0;
  }

  int code = PyInt_AsLong(res);

  return code;
}

void vtkPythonAlgorithm::SetPythonObject(PyObject* obj)
{
  vtkPythonScopeGilEnsurer gilEnsurer;

  if (!obj)
  {
    return;
  }

  Py_XDECREF(this->Object);

  this->Object = obj;
  Py_INCREF(this->Object);

  char mname[] = "Initialize";
  VTK_GET_METHOD(method, this->Object, mname, /* no return */)

  vtkSmartPyObject args(PyTuple_New(1));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

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

vtkTypeBool vtkPythonAlgorithm::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "ProcessRequest";
  VTK_GET_METHOD(method, this->Object, mname, 0)

  vtkSmartPyObject args(PyTuple_New(4));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  PyObject* pyrequest = VTKToPython(request);
  PyTuple_SET_ITEM(args.GetPointer(), 1, pyrequest);

  int nports = this->GetNumberOfInputPorts();
  PyObject* pyininfos = PyTuple_New(nports);
  for (int i = 0; i < nports; ++i)
  {
    PyObject* pyininfo = VTKToPython(inInfo[i]);
    PyTuple_SET_ITEM(pyininfos, i, pyininfo);
  }
  PyTuple_SET_ITEM(args.GetPointer(), 2, pyininfos);

  PyObject* pyoutinfo = VTKToPython(outInfo);
  PyTuple_SET_ITEM(args.GetPointer(), 3, pyoutinfo);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return CheckResult(mname, result);
}

int vtkPythonAlgorithm::FillInputPortInformation(int port, vtkInformation* info)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "FillInputPortInformation";
  VTK_GET_METHOD(method, this->Object, mname, 0)

  vtkSmartPyObject args(PyTuple_New(3));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  PyObject* pyport = PyInt_FromLong(port);
  PyTuple_SET_ITEM(args.GetPointer(), 1, pyport);

  PyObject* pyinfo = VTKToPython(info);
  PyTuple_SET_ITEM(args.GetPointer(), 2, pyinfo);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return CheckResult(mname, result);
}

int vtkPythonAlgorithm::FillOutputPortInformation(int port, vtkInformation* info)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "FillOutputPortInformation";
  VTK_GET_METHOD(method, this->Object, mname, 0)

  vtkSmartPyObject args(PyTuple_New(3));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  PyObject* pyport = PyInt_FromLong(port);
  PyTuple_SET_ITEM(args.GetPointer(), 1, pyport);

  PyObject* pyinfo = VTKToPython(info);
  PyTuple_SET_ITEM(args.GetPointer(), 2, pyinfo);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return CheckResult(mname, result);
}
