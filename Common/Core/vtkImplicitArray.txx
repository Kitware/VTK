/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitArray.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkImplicitArray_txx
#define vtkImplicitArray_txx

#include "vtkImplicitArray.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
template <class BackendT>
struct vtkImplicitArray<BackendT>::vtkInternals
{
  vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>> Cache;
};

//-----------------------------------------------------------------------------
template <class BackendT>
vtkImplicitArray<BackendT>* vtkImplicitArray<BackendT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkImplicitArray<BackendT>);
}

//-----------------------------------------------------------------------------
template <class BackendT>
vtkImplicitArray<BackendT>::vtkImplicitArray()
  : Internals(new vtkInternals())
  , Backend(std::make_shared<BackendT>()){};

//-----------------------------------------------------------------------------
template <class BackendT>
vtkImplicitArray<BackendT>::~vtkImplicitArray(){};

//-----------------------------------------------------------------------------
template <class BackendT>
void vtkImplicitArray<BackendT>::SetValue(vtkIdType idx, ValueType value)
{
  vtkWarningMacro("Can not set value in read only vtkImplicitArray!");
}

//-----------------------------------------------------------------------------
template <class BackendT>
void vtkImplicitArray<BackendT>::SetTypedTuple(vtkIdType idx, const ValueType* tuple)
{
  vtkWarningMacro("Can not set tuple in read only vtkImplicitArray!");
}

//-----------------------------------------------------------------------------
template <class BackendT>
void* vtkImplicitArray<BackendT>::GetVoidPointer(vtkIdType idx)
{
  this->Internals->Cache = vtk::TakeSmartPointer(vtkAOSDataArrayTemplate<ValueType>::New());
  this->Internals->Cache->DeepCopy(this);
  return this->Internals->Cache->GetVoidPointer(idx);
}

//-----------------------------------------------------------------------------
template <class BackendT>
void vtkImplicitArray<BackendT>::Squeeze()
{
  this->Internals->Cache = nullptr;
}

#endif // vtkImplicitArray_txx
