/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonItem.h"
#include "vtkObjectFactory.h"

#include "vtkContext2D.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

vtkStandardNewMacro(vtkPythonItem);

//------------------------------------------------------------------------------
void vtkPythonItem::PrintSelf(ostream& os, vtkIndent indent)
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

//------------------------------------------------------------------------------
vtkPythonItem::vtkPythonItem()
{
  this->Object = nullptr;
}

//------------------------------------------------------------------------------
vtkPythonItem::~vtkPythonItem()
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

//------------------------------------------------------------------------------
static PyObject* VTKToPython(vtkObjectBase* obj)
{
  // Return value: New reference.
  return vtkPythonUtil::GetObjectFromPointer(obj);
}

//------------------------------------------------------------------------------
bool vtkPythonItem::CheckResult(const char* method, const vtkSmartPyObject& res)
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
    return false;
  }
  if (!PyBool_Check(res))
  {
    vtkWarningMacro("The method \"" << method << "\" should have returned boolean but did not");
    return false;
  }

  if (res == Py_False)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkPythonItem::SetPythonObject(PyObject* obj)
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

//------------------------------------------------------------------------------
bool vtkPythonItem::Paint(vtkContext2D* painter)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "Paint";
  VTK_GET_METHOD(method, this->Object, mname, 0)

  vtkSmartPyObject args(PyTuple_New(2));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  PyObject* pypainter = VTKToPython(painter);
  PyTuple_SET_ITEM(args.GetPointer(), 1, pypainter);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return CheckResult(mname, result);
}
