// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkArrayIteratorTemplate_txx
#define vtkArrayIteratorTemplate_txx

#include "vtkArrayIteratorTemplate.h"

#include "vtkAbstractArray.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class T>
vtkArrayIteratorTemplate<T>* vtkArrayIteratorTemplate<T>::New()
{
  VTK_STANDARD_NEW_BODY(vtkArrayIteratorTemplate<T>);
}

template <class T>
vtkCxxSetObjectMacro(vtkArrayIteratorTemplate<T>, Array, vtkAbstractArray);

//-----------------------------------------------------------------------------
template <class T>
vtkArrayIteratorTemplate<T>::vtkArrayIteratorTemplate()
{
  this->Array = nullptr;
  this->Pointer = nullptr;
}

//-----------------------------------------------------------------------------
template <class T>
vtkArrayIteratorTemplate<T>::~vtkArrayIteratorTemplate()
{
  this->SetArray(nullptr);
  this->Pointer = nullptr;
}

//-----------------------------------------------------------------------------
template <class T>
void vtkArrayIteratorTemplate<T>::Initialize(vtkAbstractArray* a)
{
  this->SetArray(a);
  this->Pointer = nullptr;
  if (this->Array)
  {
    this->Pointer = static_cast<T*>(this->Array->GetVoidPointer(0));
  }
}

//-----------------------------------------------------------------------------
template <class T>
vtkIdType vtkArrayIteratorTemplate<T>::GetNumberOfTuples() const
{
  if (this->Array)
  {
    return this->Array->GetNumberOfTuples();
  }
  return 0;
}

//-----------------------------------------------------------------------------
template <class T>
vtkIdType vtkArrayIteratorTemplate<T>::GetNumberOfValues() const
{
  if (this->Array)
  {
    return (this->Array->GetNumberOfTuples() * this->Array->GetNumberOfComponents());
  }
  return 0;
}

//-----------------------------------------------------------------------------
template <class T>
int vtkArrayIteratorTemplate<T>::GetNumberOfComponents() const
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
int vtkArrayIteratorTemplate<T>::GetDataType() const
{
  return this->Array->GetDataType();
}

//-----------------------------------------------------------------------------
template <class T>
int vtkArrayIteratorTemplate<T>::GetDataTypeSize() const
{
  return this->Array->GetDataTypeSize();
}

//-----------------------------------------------------------------------------
template <class T>
void vtkArrayIteratorTemplate<T>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Array: ";
  if (this->Array)
  {
    os << "\n";
    this->Array->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)"
       << "\n";
  }
}

VTK_ABI_NAMESPACE_END
#endif
