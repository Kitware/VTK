// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSmartPyObject.h"
#include "vtkABINamespace.h"

#if defined(_MSC_VER) // Visual studio
// Ignore "constant expression" warnings from MSVC due to the "while (0)" in
// the Py_X{IN,DE}CREF macros.
#pragma warning(disable : 4127)
#endif

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkSmartPyObject::vtkSmartPyObject(PyObject* obj)
  : Object(obj)
{
}

//------------------------------------------------------------------------------
vtkSmartPyObject::vtkSmartPyObject(const vtkSmartPyObject& other)
  : Object(other.Object)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  Py_XINCREF(this->Object);
}

//------------------------------------------------------------------------------
vtkSmartPyObject::~vtkSmartPyObject()
{
  if (Py_IsInitialized())
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    Py_XDECREF(this->Object);
  }
}

//------------------------------------------------------------------------------
vtkSmartPyObject& vtkSmartPyObject::operator=(const vtkSmartPyObject& other)
{
  if (this == &other)
  {
    return *this;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  Py_XDECREF(this->Object);
  this->Object = other.Object;
  Py_XINCREF(this->Object);
  return *this;
}

//------------------------------------------------------------------------------
vtkSmartPyObject& vtkSmartPyObject::operator=(PyObject* obj)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  Py_XDECREF(this->Object);
  this->Object = obj;
  Py_XINCREF(this->Object);
  return *this;
}

//------------------------------------------------------------------------------
void vtkSmartPyObject::TakeReference(PyObject* obj)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  Py_XDECREF(this->Object);
  this->Object = obj;
}

//------------------------------------------------------------------------------
PyObject* vtkSmartPyObject::operator->() const
{
  return this->Object;
}

//------------------------------------------------------------------------------
vtkSmartPyObject::operator PyObject*() const
{
  return this->Object;
}

//------------------------------------------------------------------------------
vtkSmartPyObject::operator bool() const
{
  return this->Object != nullptr;
}

//------------------------------------------------------------------------------
PyObject* vtkSmartPyObject::ReleaseReference()
{
  PyObject* tmp = this->Object;
  this->Object = nullptr;
  return tmp;
}

//------------------------------------------------------------------------------
PyObject* vtkSmartPyObject::GetPointer() const
{
  return this->Object;
}

//------------------------------------------------------------------------------
PyObject* vtkSmartPyObject::GetAndIncreaseReferenceCount()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  Py_XINCREF(this->Object);
  return this->Object;
}
VTK_ABI_NAMESPACE_END
