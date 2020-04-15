/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonArchiver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonArchiver.h"
#include "vtkObjectFactory.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPythonArchiver);

//----------------------------------------------------------------------------
vtkPythonArchiver::vtkPythonArchiver()
{
  this->Object = nullptr;
}

//----------------------------------------------------------------------------
vtkPythonArchiver::~vtkPythonArchiver()
{
  // we check if Python is still initialized since the Python interpreter may
  // have been finalized before the VTK object is released.
  if (Py_IsInitialized())
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    Py_XDECREF(this->Object);
  }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
/// Return value: New reference.
static PyObject* VTKToPython(vtkObjectBase* obj)
{
  return vtkPythonUtil::GetObjectFromPointer(obj);
}

//----------------------------------------------------------------------------
int vtkPythonArchiver::CheckResult(const char* method, const vtkSmartPyObject& res)
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

//----------------------------------------------------------------------------
void vtkPythonArchiver::SetPythonObject(PyObject* obj)
{
  vtkPythonScopeGilEnsurer gilEnsurer;

  if (!obj)
  {
    return;
  }

  Py_XDECREF(this->Object);

  this->Object = obj;
  Py_INCREF(this->Object);
}

//----------------------------------------------------------------------------
void vtkPythonArchiver::OpenArchive()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "OpenArchive";
  VTK_GET_METHOD(method, this->Object, mname, )

  vtkSmartPyObject args(PyTuple_New(1));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  CheckResult(mname, result);
}

//----------------------------------------------------------------------------
void vtkPythonArchiver::CloseArchive()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "CloseArchive";
  VTK_GET_METHOD(method, this->Object, mname, )

  vtkSmartPyObject args(PyTuple_New(1));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  CheckResult(mname, result);
}

//----------------------------------------------------------------------------
void vtkPythonArchiver::InsertIntoArchive(
  const std::string& relativePath, const char* data, std::size_t size)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "InsertIntoArchive";
  VTK_GET_METHOD(method, this->Object, mname, )

  vtkSmartPyObject args(PyTuple_New(4));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  PyObject* pypath = PyString_FromString(relativePath.c_str());
  PyTuple_SET_ITEM(args.GetPointer(), 1, pypath);

#ifndef VTK_PY3K
  PyObject* pydata = PyString_FromStringAndSize(data, size);
  PyTuple_SET_ITEM(args.GetPointer(), 2, pydata);
#else
  PyObject* pydata = PyBytes_FromStringAndSize(data, size);
  PyTuple_SET_ITEM(args.GetPointer(), 2, pydata);
#endif

  PyObject* pysize = PyLong_FromSsize_t(size);
  PyTuple_SET_ITEM(args.GetPointer(), 3, pysize);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  CheckResult(mname, result);
}

//----------------------------------------------------------------------------
bool vtkPythonArchiver::Contains(const std::string& relativePath)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  char mname[] = "Contains";
  VTK_GET_METHOD(method, this->Object, mname, false)

  vtkSmartPyObject args(PyTuple_New(2));

  PyObject* vtkself = VTKToPython(this);
  PyTuple_SET_ITEM(args.GetPointer(), 0, vtkself);

  PyObject* pypath = PyString_FromString(relativePath.c_str());
  PyTuple_SET_ITEM(args.GetPointer(), 1, pypath);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return (CheckResult(mname, result) != 0);
}

//----------------------------------------------------------------------------
void vtkPythonArchiver::PrintSelf(ostream& os, vtkIndent indent)
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
