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

/**
 * @class   vtkTypeTemplate
 * @brief   Provides the equivalent of vtkTypeMacro
 * for use with template classes
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkTypeTemplate_h
#define vtkTypeTemplate_h

#include "vtkObject.h"
#include <string>
#include <typeinfo>

// This class is legacy. See vtkTemplateTypeMacro in vtkSetGet.h for the
// replacement.
#ifndef VTK_LEGACY_REMOVE

template<class ThisT, class BaseT>
class vtkTypeTemplate : public BaseT
{
public:
  typedef BaseT Superclass;

  ThisT* NewInstance() const
  {
    return ThisT::SafeDownCast(this->NewInstanceInternal());
  }

  static ThisT* SafeDownCast(vtkObjectBase* o)
  {
    if(o &&
       o->IsA(vtkTypeTemplate<ThisT, BaseT>::GetClassNameInternalCachedName()))
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
  static vtkTypeBool IsTypeOf(const char* type)
  {
    if (strcmp(vtkTypeTemplate<ThisT, BaseT>::GetClassNameInternalCachedName(),
               type) == 0)
    {
      return 1;
    }
    return BaseT::IsTypeOf(type);
  }

  // We don't expose this publicly, because the typename we generate
  // for our template instantiations isn't human-readable, unlike
  // "normal" VTK classes.
  vtkTypeBool IsA(const char *type) VTK_OVERRIDE
  {
    return this->IsTypeOf(type);
  }

  vtkTypeTemplate()
  {
    VTK_LEGACY_REPLACED_BODY(vtkTypeTemplate, "VTK 7.1",
                             vtkTemplateTypeMacro (vtkSetGet.h));
  }

private:
  vtkTypeTemplate(const vtkTypeTemplate<ThisT, BaseT>&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTypeTemplate<ThisT, BaseT>&) VTK_DELETE_FUNCTION;

  static const char* GetClassNameInternalCachedName()
  {
    static std::string thisType(typeid(ThisT).name());
    return thisType.c_str();
  }

  const char* GetClassNameInternal() const VTK_OVERRIDE
  {
    return this->GetClassNameInternalCachedName();
  }
};

#endif // VTK_LEGACY_REMOVE
#endif // header guard

// VTK-HeaderTest-Exclude: vtkTypeTemplate.h
