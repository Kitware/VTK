/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmartPointerBase.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointerBase.h"

//----------------------------------------------------------------------------
vtkSmartPointerBase::vtkSmartPointerBase():
  Object(0)
{
  this->Register();
}

//----------------------------------------------------------------------------
vtkSmartPointerBase::vtkSmartPointerBase(vtkObjectBase* r):
  Object(r)
{
  this->Register();
}

//----------------------------------------------------------------------------
vtkSmartPointerBase::vtkSmartPointerBase(const vtkSmartPointerBase& r):
  Object(r.Object)
{
  this->Register();
}
  
//----------------------------------------------------------------------------
vtkSmartPointerBase::~vtkSmartPointerBase()
{
  this->UnRegister();
}
  
//----------------------------------------------------------------------------
vtkSmartPointerBase&
vtkSmartPointerBase::operator=(vtkObjectBase* r)
{
  vtkSmartPointerBase(r).Swap(*this);
  return *this;
}

//----------------------------------------------------------------------------
vtkSmartPointerBase&
vtkSmartPointerBase::operator=(const vtkSmartPointerBase& r)
{
  vtkSmartPointerBase(r).Swap(*this);
  return *this;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkSmartPointerBase::GetPointer() const
{
  return this->Object;
}

//----------------------------------------------------------------------------
void vtkSmartPointerBase::Swap(vtkSmartPointerBase& r)
{
  vtkObjectBase* temp = r.Object;
  r.Object = this->Object;
  this->Object = temp;
}

//----------------------------------------------------------------------------
void vtkSmartPointerBase::Register()
{
  if(this->Object)
    {
    this->Object->Register(0);
    }
}

//----------------------------------------------------------------------------
void vtkSmartPointerBase::UnRegister()
{
  if(this->Object)
    {
    this->Object->UnRegister(0);
    }
}

//----------------------------------------------------------------------------
#ifdef VTK_COMPILER_HAS_BOOL
# define VTK_BOOL bool
#else
# define VTK_BOOL int
#endif
#define VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(op) \
   VTK_BOOL operator op (const vtkSmartPointerBase& l, \
                         const vtkSmartPointerBase& r) \
    { \
    return (static_cast<void*>(l.GetPointer()) op \
            static_cast<void*>(r.GetPointer())); \
    } \
  VTK_BOOL operator op (vtkObjectBase* l, const vtkSmartPointerBase& r) \
    { \
    return (static_cast<void*>(l) op static_cast<void*>(r.GetPointer())); \
    } \
  VTK_BOOL operator op (const vtkSmartPointerBase& l, vtkObjectBase* r) \
    { \
    return (static_cast<void*>(l.GetPointer()) op static_cast<void*>(r)); \
    }
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(==)  
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(!=)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(<)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(<=)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(>)
VTK_SMART_POINTER_BASE_DEFINE_OPERATOR(>=)
#undef VTK_SMART_POINTER_BASE_DEFINE_OPERATOR

//----------------------------------------------------------------------------
ostream& operator << (ostream& os, const vtkSmartPointerBase& p)
{
  return os << static_cast<void*>(p.GetPointer());
}
