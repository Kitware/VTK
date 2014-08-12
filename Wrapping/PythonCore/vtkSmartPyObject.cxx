
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmartPyObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPyObject.h"

#if defined(_MSC_VER) // Visual studio
// Ignore "constant expression" warnings from MSVC due to the "while (0)" in
// the Py_X{IN,DE}CREF macros.
#pragma warning(disable: 4127)
#endif

//--------------------------------------------------------------------
vtkSmartPyObject::vtkSmartPyObject(PyObject *obj) :
  Object(obj)
{}

//--------------------------------------------------------------------
vtkSmartPyObject::vtkSmartPyObject(const vtkSmartPyObject &other) :
  Object(other.Object)
{
  Py_XINCREF(this->Object);
}

//--------------------------------------------------------------------
vtkSmartPyObject::~vtkSmartPyObject()
{
  Py_XDECREF(this->Object);
}

//--------------------------------------------------------------------
vtkSmartPyObject &vtkSmartPyObject::operator=(const vtkSmartPyObject &other)
{
  Py_XDECREF(this->Object);
  this->Object = other.Object;
  Py_XINCREF(this->Object);
  return *this;
}

//--------------------------------------------------------------------
vtkSmartPyObject &vtkSmartPyObject::operator=(PyObject *obj)
{
  Py_XDECREF(this->Object);
  this->Object = obj;
  Py_XINCREF(this->Object);
  return *this;
}

//--------------------------------------------------------------------
void vtkSmartPyObject::TakeReference(PyObject *obj)
{
  Py_XDECREF(this->Object);
  this->Object = obj;
}

//--------------------------------------------------------------------
PyObject *vtkSmartPyObject::operator->() const
{
  return this->Object;
}

//--------------------------------------------------------------------
vtkSmartPyObject::operator PyObject*() const
{
  return this->Object;
}

//--------------------------------------------------------------------
vtkSmartPyObject::operator bool() const
{
  return this->Object != NULL;
}

//--------------------------------------------------------------------
PyObject *vtkSmartPyObject::ReleaseReference()
{
  PyObject *tmp = this->Object;
  this->Object = NULL;
  return tmp;
}

//--------------------------------------------------------------------
PyObject *vtkSmartPyObject::GetPointer() const
{
  return this->Object;
}

//--------------------------------------------------------------------
PyObject *vtkSmartPyObject::GetAndIncreaseReferenceCount()
{
  Py_XINCREF(this->Object);
  return this->Object;
}
