/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayIteratorTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkArrayIteratorTemplate_txx
#define vtkArrayIteratorTemplate_txx

#include "vtkArrayIteratorTemplate.h"

#include "vtkAbstractArray.h"
#include "vtkObjectFactory.h"

template <class T>
vtkArrayIteratorTemplate<T>* vtkArrayIteratorTemplate<T>::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkArrayIteratorTemplate");
  if (ret)
    {
    return static_cast<vtkArrayIteratorTemplate<T>*> (ret);
    }
  return new vtkArrayIteratorTemplate<T>;
}

template <class T>
vtkCxxSetObjectMacro(vtkArrayIteratorTemplate<T>, Array, vtkAbstractArray);
//-----------------------------------------------------------------------------
template <class T>
vtkArrayIteratorTemplate<T>::vtkArrayIteratorTemplate()
{
  this->Array = 0;
  this->Pointer = 0;
}

//-----------------------------------------------------------------------------
template <class T>
vtkArrayIteratorTemplate<T>::~vtkArrayIteratorTemplate()
{
  this->SetArray(0);
  this->Pointer = 0;
}

//-----------------------------------------------------------------------------
template <class T>
void vtkArrayIteratorTemplate<T>::Initialize(vtkAbstractArray* a)
{
  this->SetArray(a);
  this->Pointer = 0;
  if (this->Array)
    {
    this->Pointer = static_cast<T*>(this->Array->GetVoidPointer(0));
    }
}

//-----------------------------------------------------------------------------
template <class T>
vtkIdType vtkArrayIteratorTemplate<T>::GetNumberOfTuples()
{
  if (this->Array)
    {
    return this->Array->GetNumberOfTuples();
    }
  return 0;
}

//-----------------------------------------------------------------------------
template <class T>
vtkIdType vtkArrayIteratorTemplate<T>::GetNumberOfValues()
{
  if (this->Array)
    {
    return (this->Array->GetNumberOfTuples() * this->Array->GetNumberOfComponents());
    }
  return 0;
}

//-----------------------------------------------------------------------------
template <class T>
int vtkArrayIteratorTemplate<T>::GetNumberOfComponents()
{
  if (this->Array)
    {
    return this->Array->GetNumberOfComponents();
    }
  return 0;
}

//-----------------------------------------------------------------------------
template <class T>
T* vtkArrayIteratorTemplate<T>::GetTuple(vtkIdType id)
{
  return &this->Pointer[id * this->Array->GetNumberOfComponents()];
}

//-----------------------------------------------------------------------------
template <class T>
int vtkArrayIteratorTemplate<T>::GetDataType()
{
  return this->Array->GetDataType();
}

//-----------------------------------------------------------------------------
template <class T>
int vtkArrayIteratorTemplate<T>::GetDataTypeSize()
{
  return this->Array->GetDataTypeSize();
}

//-----------------------------------------------------------------------------
template <class T>
void vtkArrayIteratorTemplate<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Array: " ;
  if (this->Array)
    {
    os << "\n";
    this->Array->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << "\n";
    }
}

#endif

