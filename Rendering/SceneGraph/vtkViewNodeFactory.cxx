/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewNodeFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkViewNodeFactory.h"
#include "vtkObjectFactory.h"
#include "vtkViewNode.h"

#include <map>
#include <string>

//============================================================================
class vtkViewNodeFactory::vtkInternals
{
public:
  std::map<std::string, vtkViewNode* (*)()> Overrides;

  vtkInternals() {}

  ~vtkInternals() { this->Overrides.clear(); }
};

//============================================================================
vtkStandardNewMacro(vtkViewNodeFactory);

//----------------------------------------------------------------------------
vtkViewNodeFactory::vtkViewNodeFactory()
{
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkViewNodeFactory::~vtkViewNodeFactory()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkViewNodeFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkViewNode* vtkViewNodeFactory::CreateNode(vtkObject* who)
{
  if (!who)
  {
    return nullptr;
  }

  vtkViewNode* (*func)() = nullptr;

  // First, check if there is an exact match for override functions for this
  // object type.
  {
    auto fnOverrideIt = this->Internals->Overrides.find(who->GetClassName());
    if (fnOverrideIt != this->Internals->Overrides.end())
    {
      func = fnOverrideIt->second;
    }
  }

  // Next, check if there is an indirect match (one of the parents of this
  // object type has an override). If there is more than one override for
  // types in this object's hierarchy, choose the most derived one.
  if (func == nullptr)
  {
    vtkIdType closest = VTK_ID_MAX;
    for (auto it = this->Internals->Overrides.begin(); it != this->Internals->Overrides.end(); ++it)
    {
      vtkIdType numberOfGenerations = who->GetNumberOfGenerationsFromBase(it->first.c_str());
      if (numberOfGenerations >= 0 && numberOfGenerations < closest)
      {
        closest = numberOfGenerations;
        func = it->second;
      }
    }
  }

  // If neither are available, do not create a node for this object.
  if (func == nullptr)
  {
    return nullptr;
  }

  // Otherwise, create a node and initialize it.
  vtkViewNode* vn = func();
  vn->SetMyFactory(this);
  if (vn)
  {
    vn->SetRenderable(who);
  }

  return vn;
}

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
vtkViewNode* vtkViewNodeFactory::CreateNode(const char* forwhom)
{
  if (this->Internals->Overrides.find(forwhom) == this->Internals->Overrides.end())
  {
    return nullptr;
  }
  vtkViewNode* (*func)() = this->Internals->Overrides.find(forwhom)->second;
  vtkViewNode* vn = func();
  vn->SetMyFactory(this);
  return vn;
}
#endif

//----------------------------------------------------------------------------
void vtkViewNodeFactory::RegisterOverride(const char* name, vtkViewNode* (*func)())
{
  this->Internals->Overrides[name] = func;
}
