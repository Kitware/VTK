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

#include "vtkBoundingBox.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCompositeDataDisplayAttributes)

vtkCompositeDataDisplayAttributes::vtkCompositeDataDisplayAttributes()
{
}

vtkCompositeDataDisplayAttributes::~vtkCompositeDataDisplayAttributes()
{
}

void vtkCompositeDataDisplayAttributes::SetBlockVisibility(vtkDataObject* data_object, bool visible)
{
  this->BlockVisibilities[data_object] = visible;
}

bool vtkCompositeDataDisplayAttributes::GetBlockVisibility(vtkDataObject* data_object) const
{
  BoolMap::const_iterator iter = this->BlockVisibilities.find(data_object);
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

bool vtkCompositeDataDisplayAttributes::HasBlockVisibility(vtkDataObject* data_object) const
{
  return this->BlockVisibilities.count(data_object) == size_t(1);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockVisibility(vtkDataObject* data_object)
{
  this->BlockVisibilities.erase(data_object);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockVisibilities()
{
  this->BlockVisibilities.clear();
}

#ifndef VTK_LEGACY_REMOVE
void vtkCompositeDataDisplayAttributes::RemoveBlockVisibilites()
{
  VTK_LEGACY_REPLACED_BODY(vtkCompositeDataDisplayAttributes::RemoveBlockVisibilites, "VTK 8.1",
    vtkCompositeDataDisplayAttributes::RemoveBlockVisibilities());
  this->RemoveBlockVisibilities();
}
#endif

void vtkCompositeDataDisplayAttributes::SetBlockPickability(vtkDataObject* data_object, bool visible)
{
  this->BlockPickabilities[data_object] = visible;
}

bool vtkCompositeDataDisplayAttributes::GetBlockPickability(vtkDataObject* data_object) const
{
  BoolMap::const_iterator iter = this->BlockPickabilities.find(data_object);
  if(iter != this->BlockPickabilities.end())
  {
    return iter->second;
  }
  else
  {
    // default to true
    return true;
  }
}

bool vtkCompositeDataDisplayAttributes::HasBlockPickabilities() const
{
  return !this->BlockPickabilities.empty();
}

bool vtkCompositeDataDisplayAttributes::HasBlockPickability(vtkDataObject* data_object) const
{
  return this->BlockPickabilities.count(data_object) == size_t(1);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockPickability(vtkDataObject* data_object)
{
  this->BlockPickabilities.erase(data_object);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockPickabilities()
{
  this->BlockPickabilities.clear();
}

void vtkCompositeDataDisplayAttributes::SetBlockColor(
  vtkDataObject* data_object, const double color[3])
{
  this->BlockColors[data_object] = vtkColor3d(color[0], color[1], color[2]);
}

void vtkCompositeDataDisplayAttributes::GetBlockColor(
  vtkDataObject* data_object, double color[3]) const
{
  ColorMap::const_iterator iter = this->BlockColors.find(data_object);
  if(iter != this->BlockColors.end())
  {
    std::copy(&iter->second[0], &iter->second[3], color);
  }
}

vtkColor3d vtkCompositeDataDisplayAttributes::GetBlockColor(
  vtkDataObject* data_object) const
{
  ColorMap::const_iterator iter = this->BlockColors.find(data_object);  if(iter != this->BlockColors.end())
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
  vtkDataObject* data_object) const
{
  return this->BlockColors.count(data_object) == size_t(1);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockColor(
  vtkDataObject* data_object)
{
  this->BlockColors.erase(data_object);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockColors()
{
  this->BlockColors.clear();
}

void vtkCompositeDataDisplayAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCompositeDataDisplayAttributes::SetBlockOpacity(vtkDataObject* data_object, double opacity)
{
  this->BlockOpacities[data_object] = opacity;
}

double vtkCompositeDataDisplayAttributes::GetBlockOpacity(vtkDataObject* data_object) const
{
  DoubleMap::const_iterator iter = this->BlockOpacities.find(data_object);

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

bool vtkCompositeDataDisplayAttributes::HasBlockOpacity(vtkDataObject* data_object) const
{
  return this->BlockOpacities.find(data_object) != this->BlockOpacities.end();
}

void vtkCompositeDataDisplayAttributes::RemoveBlockOpacity(vtkDataObject* data_object)
{
  this->BlockOpacities.erase(data_object);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockOpacities()
{
  this->BlockOpacities.clear();
}

void vtkCompositeDataDisplayAttributes::SetBlockMaterial(vtkDataObject* data_object, const std::string& material)
{
  this->BlockMaterials[data_object] = material;
}

const std::string& vtkCompositeDataDisplayAttributes::GetBlockMaterial(vtkDataObject* data_object) const
{
  StringMap::const_iterator iter = this->BlockMaterials.find(data_object);

  if(iter != this->BlockMaterials.end())
  {
    return iter->second;
  }

  static const std::string nomat = "";
  return nomat;
}

bool vtkCompositeDataDisplayAttributes::HasBlockMaterials() const
{
  return !this->BlockMaterials.empty();
}

bool vtkCompositeDataDisplayAttributes::HasBlockMaterial(vtkDataObject* data_object) const
{
  return this->BlockMaterials.find(data_object) != this->BlockMaterials.end();
}

void vtkCompositeDataDisplayAttributes::RemoveBlockMaterial(vtkDataObject* data_object)
{
  this->BlockMaterials.erase(data_object);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockMaterials()
{
  this->BlockMaterials.clear();
}

void vtkCompositeDataDisplayAttributes::ComputeVisibleBounds(
  vtkCompositeDataDisplayAttributes* cda,
  vtkDataObject *dobj,
  double bounds[6])
{
  vtkMath::UninitializeBounds(bounds);
  // computing bounds with only visible blocks
  vtkBoundingBox bbox;
  vtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(
    cda, dobj, &bbox);
  if(bbox.IsValid())
  {
    bbox.GetBounds(bounds);
  }
}

void vtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(
  vtkCompositeDataDisplayAttributes* cda,
  vtkDataObject *dobj,
  vtkBoundingBox* bbox,
  bool parentVisible)
{
  if(!dobj || !bbox)
  {
    return;
  }

  // A block always *has* a visibility state, either explicitly set or inherited.
  bool blockVisible = (cda && cda->HasBlockVisibility(dobj)) ?
    cda->GetBlockVisibility(dobj) : parentVisible;

  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::SafeDownCast(dobj);
  vtkMultiPieceDataSet *mpds = vtkMultiPieceDataSet::SafeDownCast(dobj);
  if (mbds || mpds)
  {
    const unsigned int numChildren = mbds ? mbds->GetNumberOfBlocks() :
      mpds->GetNumberOfPieces();
    for (unsigned int cc = 0 ; cc < numChildren; cc++)
    {
      vtkDataObject* child = mbds ? mbds->GetBlock(cc) : mpds->GetPiece(cc);
      if (child == nullptr)
      {
        // Speeds things up when dealing with nullptr blocks (which is common with AMRs).
        continue;
      }
      vtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(
        cda, child, bbox, blockVisible);
    }
  }
  else if (dobj && blockVisible == true)
  {
    vtkDataSet *ds = vtkDataSet::SafeDownCast(dobj);
    if(ds)
    {
      double bounds[6];
      ds->GetBounds(bounds);
      bbox->AddBounds(bounds);
    }
  }
}

vtkDataObject* vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
  const unsigned int flat_index, vtkDataObject* parent_obj,
  unsigned int& current_flat_index)
{
  if (current_flat_index == flat_index)
  {
    return parent_obj;
  }
  current_flat_index++;

  auto multiBlock = vtkMultiBlockDataSet::SafeDownCast(parent_obj);
  auto multiPiece = vtkMultiPieceDataSet::SafeDownCast(parent_obj);
  if (multiBlock || multiPiece)
  {
    const unsigned int numChildren = multiBlock ?
      multiBlock->GetNumberOfBlocks() : multiPiece->GetNumberOfPieces();

    for (unsigned int cc = 0; cc < numChildren; cc++)
    {
      vtkDataObject* child = multiBlock ? multiBlock->GetBlock(cc) :
        multiPiece->GetPiece(cc);

      if (!child)
      {
        current_flat_index++;
        continue;
      }

      const auto data = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
        flat_index, child, current_flat_index);
      if (data)
      {
        return data;
      }
    }
  }

  return nullptr;
}
