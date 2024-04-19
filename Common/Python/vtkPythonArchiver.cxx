// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPythonArchiver.h"
#include "vtkObjectFactory.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPythonArchiver);

//------------------------------------------------------------------------------
vtkPythonArchiver::vtkPythonArchiver()
{
  this->Object = nullptr;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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
/// Return value: New reference.
static PyObject* VTKToPython(vtkObjectBase* obj)
{
  return vtkPythonUtil::GetObjectFromPointer(obj);
}

//------------------------------------------------------------------------------
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
  if (!PyLong_Check(res))
  {
    return 0;
  }

  int code = PyLong_AsLong(res);

  return code;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkPythonArchiver::OpenArchive()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  const char* mname = "OpenArchive";
  VTK_GET_METHOD(method, this->Object, mname, )

  PyObject* vtkself = VTKToPython(this);
  vtkSmartPyObject args(PyTuple_Pack(1, vtkself));
  Py_DECREF(vtkself);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  CheckResult(mname, result);
}

//------------------------------------------------------------------------------
void vtkPythonArchiver::CloseArchive()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  const char* mname = "CloseArchive";
  VTK_GET_METHOD(method, this->Object, mname, )

  PyObject* vtkself = VTKToPython(this);
  vtkSmartPyObject args(PyTuple_Pack(1, vtkself));
  Py_DECREF(vtkself);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  CheckResult(mname, result);
}

//------------------------------------------------------------------------------
void vtkPythonArchiver::InsertIntoArchive(
  const std::string& relativePath, const char* data, std::size_t size)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  const char* mname = "InsertIntoArchive";
  VTK_GET_METHOD(method, this->Object, mname, )

  PyObject* vtkself = VTKToPython(this);
  PyObject* pypath = PyUnicode_FromString(relativePath.c_str());
  PyObject* pydata = PyBytes_FromStringAndSize(data, size);
  PyObject* pysize = PyLong_FromSsize_t(size);
  vtkSmartPyObject args(PyTuple_Pack(4, vtkself, pypath, pydata, pysize));
  Py_DECREF(vtkself);
  Py_DECREF(pypath);
  Py_DECREF(pydata);
  Py_DECREF(pysize);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  CheckResult(mname, result);
}

//------------------------------------------------------------------------------
bool vtkPythonArchiver::Contains(const std::string& relativePath)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  const char* mname = "Contains";
  VTK_GET_METHOD(method, this->Object, mname, false)

  PyObject* vtkself = VTKToPython(this);
  PyObject* pypath = PyUnicode_FromString(relativePath.c_str());
  vtkSmartPyObject args(PyTuple_Pack(2, vtkself, pypath));
  Py_DECREF(vtkself);
  Py_DECREF(pypath);

  vtkSmartPyObject result(PyObject_Call(method, args, nullptr));

  return (CheckResult(mname, result) != 0);
}

//------------------------------------------------------------------------------
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
    PyObject* bytes = PyUnicode_EncodeLocale(str, VTK_PYUNICODE_ENC);
    if (bytes)
    {
      os << PyBytes_AsString(bytes);
      Py_DECREF(bytes);
    }
    os << std::endl;
  }
}
VTK_ABI_NAMESPACE_END
