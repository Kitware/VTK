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

bool vtkCompositeDataDisplayAttributes::HasBlockVisibilities() const
{
  return !this->BlockVisibilities.empty();
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

void vtkCompositeDataDisplayAttributes::SetBlockColor(
  unsigned int flat_index, const double color[3])
{
  this->BlockColors[flat_index] = vtkColor3d(color[0], color[1], color[2]);
}

void vtkCompositeDataDisplayAttributes::GetBlockColor(
  unsigned int flat_index, double color[3]) const
{
  std::map<unsigned int, vtkColor3d>::const_iterator
    iter = this->BlockColors.find(flat_index);
  if(iter != this->BlockColors.end())
    {
    std::copy(&iter->second[0], &iter->second[3], color);
    }
}

vtkColor3d vtkCompositeDataDisplayAttributes::GetBlockColor(
  unsigned int flat_index) const
{
  std::map<unsigned int, vtkColor3d>::const_iterator
    iter = this->BlockColors.find(flat_index);
  if(iter != this->BlockColors.end())
    {
    return iter->second;
    }
  return vtkColor3d();
}

bool vtkCompositeDataDisplayAttributes::HasBlockColors() const
{
  return !this->BlockColors.empty();
}

bool vtkCompositeDataDisplayAttributes::HasBlockColor(
  unsigned int flat_index) const
{
  return this->BlockColors.count(flat_index) == size_t(1);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockColor(
  unsigned int flat_index)
{
  this->BlockColors.erase(flat_index);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockColors()
{
  this->BlockColors.clear();
}

void vtkCompositeDataDisplayAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCompositeDataDisplayAttributes::SetBlockOpacity(unsigned int flat_index, double opacity)
{
  this->BlockOpacities[flat_index] = opacity;
}

double vtkCompositeDataDisplayAttributes::GetBlockOpacity(unsigned int flat_index) const
{
  std::map<unsigned int, double>::const_iterator iter = this->BlockOpacities.find(flat_index);

  if(iter != this->BlockOpacities.end())
    {
    return iter->second;
    }

  return 0;
}

bool vtkCompositeDataDisplayAttributes::HasBlockOpacities() const
{
  return !this->BlockOpacities.empty();
}

bool vtkCompositeDataDisplayAttributes::HasBlockOpacity(unsigned int flat_index) const
{
  return this->BlockOpacities.find(flat_index) != this->BlockOpacities.end();
}

void vtkCompositeDataDisplayAttributes::RemoveBlockOpacity(unsigned int flat_index)
{
  this->BlockOpacities.erase(flat_index);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockOpacities()
{
  this->BlockOpacities.clear();
}
