/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWeakPointerBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWeakPointerBase.h"

//----------------------------------------------------------------------------
class vtkWeakPointerBaseToObjectBaseFriendship
{
public:
  static void AddWeakPointer(vtkObjectBase *r, vtkWeakPointerBase *p);
  static void RemoveWeakPointer(vtkObjectBase *r, vtkWeakPointerBase *p);
};

//----------------------------------------------------------------------------
void vtkWeakPointerBaseToObjectBaseFriendship::AddWeakPointer(
  vtkObjectBase *r, vtkWeakPointerBase *p)
{
  if (r)
    {
    vtkWeakPointerBase **l = r->WeakPointers;
    if (l == 0)
      {
      // create a new list if none exists
      l = new vtkWeakPointerBase *[2];
      l[0] = p;
      l[1] = 0;
      r->WeakPointers = l;
      }
    else
      {
      size_t n = 0;
      while (l[n] != 0) { n++; }
      // if n+1 is a power of two, double the list size
      if ((n & (n+1)) == 0)
        {
        vtkWeakPointerBase **t = l;
        l = new vtkWeakPointerBase *[(n+1)*2];
        for (size_t i = 0; i < n; i++)
          {
          l[i] = t[i];
          }
        delete [] t;
        r->WeakPointers = l;
        }
      // make sure list is null-terminated
      l[n++] = p;
      l[n] = 0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkWeakPointerBaseToObjectBaseFriendship::RemoveWeakPointer(
  vtkObjectBase *r, vtkWeakPointerBase *p)
{
  if (r)
    {
    vtkWeakPointerBase **l = r->WeakPointers;
    if (l != 0)
      {
      size_t i = 0;
      while (l[i] != 0 && l[i] != p)
        {
        i++;
        }
      while (l[i] != 0)
        {
        l[i] = l[i+1];
        i++;
        }
      if (l[0] == 0)
        {
        delete [] l;
        r->WeakPointers = 0;
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkWeakPointerBase::vtkWeakPointerBase(vtkObjectBase* r) :
  Object(r)
{
  vtkWeakPointerBaseToObjectBaseFriendship::AddWeakPointer(r, this);
}

//----------------------------------------------------------------------------
vtkWeakPointerBase::vtkWeakPointerBase(const vtkWeakPointerBase& r) :
  Object(r.Object)
{
  vtkWeakPointerBaseToObjectBaseFriendship::AddWeakPointer(r.Object, this);
}

//----------------------------------------------------------------------------
vtkWeakPointerBase::~vtkWeakPointerBase()
{
  vtkWeakPointerBaseToObjectBaseFriendship::RemoveWeakPointer(
    this->Object, this);

  this->Object = 0;
}

//----------------------------------------------------------------------------
vtkWeakPointerBase&
vtkWeakPointerBase::operator=(vtkObjectBase* r)
{
  if (this->Object != r)
    {
    vtkWeakPointerBaseToObjectBaseFriendship::RemoveWeakPointer(
      this->Object, this);

    this->Object = r;

    vtkWeakPointerBaseToObjectBaseFriendship::AddWeakPointer(
      this->Object, this);
    }

  return *this;
}

//----------------------------------------------------------------------------
vtkWeakPointerBase&
vtkWeakPointerBase::operator=(const vtkWeakPointerBase& r)
{
  if (this != &r)
    {
    if (this->Object != r.Object)
      {
      vtkWeakPointerBaseToObjectBaseFriendship::RemoveWeakPointer(
        this->Object, this);

      this->Object = r.Object;

      vtkWeakPointerBaseToObjectBaseFriendship::AddWeakPointer(
        this->Object, this);
      }
    }

  return *this;
}

//----------------------------------------------------------------------------
ostream& operator << (ostream& os, const vtkWeakPointerBase& p)
{
  // Just print the pointer value into the stream.
  return os << static_cast<void*>(p.GetPointer());
}
