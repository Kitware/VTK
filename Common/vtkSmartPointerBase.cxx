/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmartPointerBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
vtkSmartPointerBase::vtkSmartPointerBase(vtkObjectBase* r, const NoReference&):
  Object(r)
{
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
ostream& operator << (ostream& os, const vtkSmartPointerBase& p)
{
  return os << static_cast<void*>(p.GetPointer());
}
