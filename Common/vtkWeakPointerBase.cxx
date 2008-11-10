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

#include "vtkCommand.h"
//----------------------------------------------------------------------------
class vtkWeakPointerBase::vtkObserver : public vtkCommand
{
public:
  static vtkObserver* New()
    {
    return new vtkObserver;
    }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
    if (this->WeakReference && this->WeakReference->Object == caller)
      {
      this->WeakReference->Object = NULL;
      }
    }
  void SetTarget(vtkWeakPointerBase* ref)
    {
    this->WeakReference = ref;
    }
private:
  vtkObserver()
    {
    this->WeakReference = 0;
    }
  vtkWeakPointerBase* WeakReference;
};

//----------------------------------------------------------------------------
vtkWeakPointerBase::vtkWeakPointerBase():
  Object(0)
{
  this->Observer = vtkWeakPointerBase::vtkObserver::New();
  this->Observer->SetTarget(this);
  this->AddObserver();
}

//----------------------------------------------------------------------------
vtkWeakPointerBase::vtkWeakPointerBase(vtkObject* r):
  Object(r)
{
  this->Observer = vtkWeakPointerBase::vtkObserver::New();
  this->Observer->SetTarget(this);
  this->AddObserver();
}

//----------------------------------------------------------------------------
vtkWeakPointerBase::vtkWeakPointerBase(const vtkWeakPointerBase& r):
  Object(r.Object)
{
  this->Observer = vtkWeakPointerBase::vtkObserver::New();
  this->Observer->SetTarget(this);
  this->AddObserver();
}

//----------------------------------------------------------------------------
vtkWeakPointerBase::~vtkWeakPointerBase()
{
  this->RemoveObserver();
  this->Observer->SetTarget(0);
  this->Observer->Delete();
  this->Object = 0;
}

//----------------------------------------------------------------------------
vtkWeakPointerBase&
vtkWeakPointerBase::operator=(vtkObject* r)
{
  if(this->Object!=r)
    {
    this->RemoveObserver();
    this->Object = r;
    this->AddObserver();
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkWeakPointerBase&
vtkWeakPointerBase::operator=(const vtkWeakPointerBase& r)
{
  if(this!=&r)
    {
    if(this->Object!=r.Object)
      {
      this->RemoveObserver();
      this->Object = r.Object;
      this->AddObserver();
      }
    }
  return *this;
}

//----------------------------------------------------------------------------
void vtkWeakPointerBase::AddObserver()
{
  if (this->Object && this->Observer)
    {
    this->Object->AddObserver(vtkCommand::DeleteEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkWeakPointerBase::RemoveObserver()
{
  if (this->Object && this->Observer)
    {
    this->Object->RemoveObservers(vtkCommand::DeleteEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
ostream& operator << (ostream& os, const vtkWeakPointerBase& p)
{
  // Just print the pointer value into the stream.
  return os << static_cast<void*>(p.GetPointer());
}
