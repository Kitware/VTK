/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTypeTemplate.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTypeTemplate - Provides the equivalent of vtkTypeMacro
// for use with template classes
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkTypeTemplate_h
#define __vtkTypeTemplate_h

#include "vtkObjectBase.h"
#include <typeinfo>

template<class ThisT, class BaseT>
class vtkTypeTemplate :
  public BaseT
{
public:
  typedef BaseT Superclass;

  ThisT* NewInstance() const
  {
    return ThisT::SafeDownCast(this->NewInstanceInternal());
  }

  static ThisT* SafeDownCast(vtkObjectBase* o)
  {
    if(o && o->IsA(typeid(ThisT).name()))
      {
      return static_cast<ThisT*>(o);
      }

    return 0;
  }
  
protected:
  virtual vtkObjectBase* NewInstanceInternal() const
  {
    return ThisT::New();
  }

  // We don't expose this publicly, because the typename we generate
  // for our template instantiations isn't human-readable, unlike
  // "normal" VTK classes.
  static int IsTypeOf(const char* type)
  {
    if(!strcmp(typeid(ThisT).name(), type))
      {
      return 1;
      }
    return BaseT::IsTypeOf(type);
  }
  
  // We don't expose this publicly, because the typename we generate
  // for our template instantiations isn't human-readable, unlike
  // "normal" VTK classes.
  virtual int IsA(const char *type)
  {
    return this->IsTypeOf(type);
  }

private:
  virtual const char* GetClassNameInternal() const
  {
    return typeid(ThisT).name();
  }
};

#endif

