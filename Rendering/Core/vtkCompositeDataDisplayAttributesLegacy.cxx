/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataDisplayAttributesLegacy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompositeDataDisplayAttributesLegacy.h"

#include "vtkBoundingBox.h"
#include "vtkDataSet.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCompositeDataDisplayAttributesLegacy);

vtkCompositeDataDisplayAttributesLegacy::vtkCompositeDataDisplayAttributesLegacy() = default;

vtkCompositeDataDisplayAttributesLegacy::~vtkCompositeDataDisplayAttributesLegacy() = default;

void vtkCompositeDataDisplayAttributesLegacy::SetBlockVisibility(
  unsigned int flat_index, bool visible)
{
  this->BlockVisibilities[flat_index] = visible;
}

bool vtkCompositeDataDisplayAttributesLegacy::GetBlockVisibility(unsigned int flat_index) const
{
  std::map<unsigned int, bool>::const_iterator iter = this->BlockVisibilities.find(flat_index);
  if (iter != this->BlockVisibilities.end())
  {
    return iter->second;
  }
  else
  {
    // default to true
    return true;
  }
}

bool vtkCompositeDataDisplayAttributesLegacy::HasBlockVisibilities() const
{
  return !this->BlockVisibilities.empty();
}

bool vtkCompositeDataDisplayAttributesLegacy::HasBlockVisibility(unsigned int flat_index) const
{
  return this->BlockVisibilities.count(flat_index) == size_t(1);
}

void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockVisibility(unsigned int flat_index)
{
  this->BlockVisibilities.erase(flat_index);
}

void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockVisibilities()
{
  this->BlockVisibilities.clear();
}

#ifndef VTK_LEGACY_REMOVE
void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockVisibilites()
{
  this->RemoveBlockVisibilities();
}
#endif

void vtkCompositeDataDisplayAttributesLegacy::SetBlockPickability(
  unsigned int flat_index, bool visible)
{
  this->BlockPickabilities[flat_index] = visible;
}

bool vtkCompositeDataDisplayAttributesLegacy::GetBlockPickability(unsigned int flat_index) const
{
  std::map<unsigned int, bool>::const_iterator iter = this->BlockPickabilities.find(flat_index);
  if (iter != this->BlockPickabilities.end())
  {
    return iter->second;
  }
  else
  {
    // default to true
    return true;
  }
}

bool vtkCompositeDataDisplayAttributesLegacy::HasBlockPickabilities() const
{
  return !this->BlockPickabilities.empty();
}

bool vtkCompositeDataDisplayAttributesLegacy::HasBlockPickability(unsigned int flat_index) const
{
  return this->BlockPickabilities.count(flat_index) == size_t(1);
}

void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockPickability(unsigned int flat_index)
{
  this->BlockPickabilities.erase(flat_index);
}

void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockPickabilities()
{
  this->BlockPickabilities.clear();
}

void vtkCompositeDataDisplayAttributesLegacy::SetBlockColor(
  unsigned int flat_index, const double color[3])
{
  this->BlockColors[flat_index] = vtkColor3d(color[0], color[1], color[2]);
}

void vtkCompositeDataDisplayAttributesLegacy::GetBlockColor(
  unsigned int flat_index, double color[3]) const
{
  std::map<unsigned int, vtkColor3d>::const_iterator iter = this->BlockColors.find(flat_index);
  if (iter != this->BlockColors.end())
  {
    std::copy(&iter->second[0], &iter->second[3], color);
  }
}

vtkColor3d vtkCompositeDataDisplayAttributesLegacy::GetBlockColor(unsigned int flat_index) const
{
  std::map<unsigned int, vtkColor3d>::const_iterator iter = this->BlockColors.find(flat_index);
  if (iter != this->BlockColors.end())
  {
    return iter->second;
  }
  return vtkColor3d();
}

bool vtkCompositeDataDisplayAttributesLegacy::HasBlockColors() const
{
  return !this->BlockColors.empty();
}

bool vtkCompositeDataDisplayAttributesLegacy::HasBlockColor(unsigned int flat_index) const
{
  return this->BlockColors.count(flat_index) == size_t(1);
}

void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockColor(unsigned int flat_index)
{
  this->BlockColors.erase(flat_index);
}

void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockColors()
{
  this->BlockColors.clear();
}

void vtkCompositeDataDisplayAttributesLegacy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCompositeDataDisplayAttributesLegacy::SetBlockOpacity(
  unsigned int flat_index, double opacity)
{
  this->BlockOpacities[flat_index] = opacity;
}

double vtkCompositeDataDisplayAttributesLegacy::GetBlockOpacity(unsigned int flat_index) const
{
  std::map<unsigned int, double>::const_iterator iter = this->BlockOpacities.find(flat_index);

  if (iter != this->BlockOpacities.end())
  {
    return iter->second;
  }

  return 0;
}

bool vtkCompositeDataDisplayAttributesLegacy::HasBlockOpacities() const
{
  return !this->BlockOpacities.empty();
}

bool vtkCompositeDataDisplayAttributesLegacy::HasBlockOpacity(unsigned int flat_index) const
{
  return this->BlockOpacities.find(flat_index) != this->BlockOpacities.end();
}

void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockOpacity(unsigned int flat_index)
{
  this->BlockOpacities.erase(flat_index);
}

void vtkCompositeDataDisplayAttributesLegacy::RemoveBlockOpacities()
{
  this->BlockOpacities.clear();
}

void vtkCompositeDataDisplayAttributesLegacy::ComputeVisibleBounds(
  vtkCompositeDataDisplayAttributesLegacy* cda, vtkDataObject* dobj, double bounds[6])
{
  vtkMath::UninitializeBounds(bounds);
  // computing bounds with only visible blocks
  vtkBoundingBox bbox;
  unsigned int flat_index = 0;
  vtkCompositeDataDisplayAttributesLegacy::ComputeVisibleBoundsInternal(
    cda, dobj, flat_index, &bbox);
  if (bbox.IsValid())
  {
    bbox.GetBounds(bounds);
  }
}

void vtkCompositeDataDisplayAttributesLegacy::ComputeVisibleBoundsInternal(
  vtkCompositeDataDisplayAttributesLegacy* cda, vtkDataObject* dobj, unsigned int& flat_index,
  vtkBoundingBox* bbox, bool parentVisible)
{
  if (!dobj || !bbox)
  {
    return;
  }

  // A block always *has* a visibility state, either explicitly set or inherited.
  bool blockVisible = (cda && cda->HasBlockVisibility(flat_index))
    ? cda->GetBlockVisibility(flat_index)
    : parentVisible;

  // Advance flat-index. After this point, flat_index no longer points to this block.
  flat_index++;

  vtkMultiBlockDataSet* mbds = vtkMultiBlockDataSet::SafeDownCast(dobj);
  vtkMultiPieceDataSet* mpds = vtkMultiPieceDataSet::SafeDownCast(dobj);
  if (mbds || mpds)
  {
    unsigned int numChildren = mbds ? mbds->GetNumberOfBlocks() : mpds->GetNumberOfPieces();
    for (unsigned int cc = 0; cc < numChildren; cc++)
    {
      vtkDataObject* child = mbds ? mbds->GetBlock(cc) : mpds->GetPiece(cc);
      if (child == nullptr)
      {
        // speeds things up when dealing with nullptr blocks (which is common with AMRs).
        flat_index++;
        continue;
      }
      vtkCompositeDataDisplayAttributesLegacy::ComputeVisibleBoundsInternal(
        cda, child, flat_index, bbox, blockVisible);
    }
  }
  else if (dobj && blockVisible == true)
  {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(dobj);
    if (ds)
    {
      double bounds[6];
      ds->GetBounds(bounds);
      bbox->AddBounds(bounds);
    }
  }
}
