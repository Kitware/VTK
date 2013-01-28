/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataDisplayAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompositeDataDisplayAttributes.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCompositeDataDisplayAttributes)

vtkCompositeDataDisplayAttributes::vtkCompositeDataDisplayAttributes()
{
}

vtkCompositeDataDisplayAttributes::~vtkCompositeDataDisplayAttributes()
{
}

void vtkCompositeDataDisplayAttributes::SetBlockVisibility(unsigned int flat_index, bool visible)
{
  this->BlockVisibilities[flat_index] = visible;
}

bool vtkCompositeDataDisplayAttributes::GetBlockVisibility(unsigned int flat_index) const
{
  std::map<unsigned int, bool>::const_iterator iter =
    this->BlockVisibilities.find(flat_index);
  if(iter != this->BlockVisibilities.end())
    {
    return iter->second;
    }
  else
    {
    // default to true
    return true;
    }
}

bool vtkCompositeDataDisplayAttributes::HasBlockVisibility(unsigned int flat_index) const
{
  return this->BlockVisibilities.count(flat_index) == size_t(1);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockVisibility(unsigned int flat_index)
{
  this->BlockVisibilities.erase(flat_index);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockVisibilites()
{
  this->BlockVisibilities.clear();
}

void vtkCompositeDataDisplayAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
