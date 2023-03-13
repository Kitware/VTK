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

#include "vtkAOSDataArrayTemplate.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
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
{
  this->Initialize();
}

//-----------------------------------------------------------------------------
template <class BackendT>
vtkImplicitArray<BackendT>::~vtkImplicitArray() = default;

//-----------------------------------------------------------------------------
template <class BackendT>
void vtkImplicitArray<BackendT>::SetValue(vtkIdType vtkNotUsed(idx), ValueType vtkNotUsed(value))
{
}

//-----------------------------------------------------------------------------
template <class BackendT>
void vtkImplicitArray<BackendT>::SetTypedTuple(
  vtkIdType vtkNotUsed(idx), const ValueType* vtkNotUsed(tuple))
{
}

//-----------------------------------------------------------------------------
template <class BackendT>
void vtkImplicitArray<BackendT>::SetTypedComponent(
  vtkIdType vtkNotUsed(idx), int vtkNotUsed(comp), ValueType vtkNotUsed(value))
{
}

//-----------------------------------------------------------------------------
template <class BackendT>
void* vtkImplicitArray<BackendT>::GetVoidPointer(vtkIdType idx)
{
  if (!this->Internals->Cache)
  {
    this->Internals->Cache = vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>>::New();
    this->Internals->Cache->DeepCopy(this);
  }
  return this->Internals->Cache->GetVoidPointer(idx);
}

//-----------------------------------------------------------------------------
template <class BackendT>
void vtkImplicitArray<BackendT>::Squeeze()
{
  this->Internals->Cache = nullptr;
}

//-----------------------------------------------------------------------------
template <class BackendT>
vtkImplicitArray<BackendT>* vtkImplicitArray<BackendT>::FastDownCast(vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkAbstractArray::ImplicitArray:
        if (vtkDataTypesCompare(source->GetDataType(), vtkTypeTraits<ValueType>::VTK_TYPE_ID))
        {
          // In a perfect world, this part should do something like
          //
          // return static_cast<vtkImplicitArray<BackendT>*>(source);
          //
          // The problem here is that we do not know what type of backend to use and any pointer to
          // an implicit array will down cast to any other pointer to an implicit array. Barring
          // something better to do here, we use the SafeDownCast mechanism to ensure safety at the
          // cost of performance.
          return vtkImplicitArray<BackendT>::SafeDownCast(source);
        }
        break;
    }
  }
  return nullptr;
}

VTK_ABI_NAMESPACE_END

#endif // vtkImplicitArray_txx
