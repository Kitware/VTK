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
#include "vtkBoundingBox.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSet.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCompositeDataDisplayAttributes);

vtkCompositeDataDisplayAttributes::vtkCompositeDataDisplayAttributes() = default;

vtkCompositeDataDisplayAttributes::~vtkCompositeDataDisplayAttributes() = default;

void vtkCompositeDataDisplayAttributes::SetBlockVisibility(vtkDataObject* data_object, bool visible)
{
  if (this->HasBlockVisibility(data_object) && this->GetBlockVisibility(data_object) == visible)
  {
    return;
  }
  this->BlockVisibilities[data_object] = visible;
  this->Modified();
}

bool vtkCompositeDataDisplayAttributes::GetBlockVisibility(vtkDataObject* data_object) const
{
  BoolMap::const_iterator iter = this->BlockVisibilities.find(data_object);
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
  if (this->HasBlockVisibilities())
  {
    this->Modified();
  }
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

void vtkCompositeDataDisplayAttributes::SetBlockPickability(
  vtkDataObject* data_object, bool visible)
{
  if (this->HasBlockPickability(data_object) && this->GetBlockPickability(data_object) == visible)
  {
    return;
  }
  this->BlockPickabilities[data_object] = visible;
  this->Modified();
}

bool vtkCompositeDataDisplayAttributes::GetBlockPickability(vtkDataObject* data_object) const
{
  BoolMap::const_iterator iter = this->BlockPickabilities.find(data_object);
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
  if (this->HasBlockPickabilities())
  {
    this->Modified();
  }
  this->BlockPickabilities.clear();
}

void vtkCompositeDataDisplayAttributes::SetBlockColor(
  vtkDataObject* data_object, const double color[3])
{
  if (this->HasBlockColor(data_object))
  {
    double currentColor[3];
    this->GetBlockColor(data_object, currentColor);
    if (color[0] == currentColor[0] && color[1] == currentColor[1] && color[2] == currentColor[2])
    {
      return;
    }
  }
  this->BlockColors[data_object] = vtkColor3d(color[0], color[1], color[2]);
  this->Modified();
}

void vtkCompositeDataDisplayAttributes::GetBlockColor(
  vtkDataObject* data_object, double color[3]) const
{
  ColorMap::const_iterator iter = this->BlockColors.find(data_object);
  if (iter != this->BlockColors.end())
  {
    std::copy(&iter->second[0], &iter->second[3], color);
  }
}

vtkColor3d vtkCompositeDataDisplayAttributes::GetBlockColor(vtkDataObject* data_object) const
{
  ColorMap::const_iterator iter = this->BlockColors.find(data_object);
  if (iter != this->BlockColors.end())
  {
    return iter->second;
  }
  return vtkColor3d();
}

bool vtkCompositeDataDisplayAttributes::HasBlockColors() const
{
  return !this->BlockColors.empty();
}

bool vtkCompositeDataDisplayAttributes::HasBlockColor(vtkDataObject* data_object) const
{
  return this->BlockColors.count(data_object) == size_t(1);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockColor(vtkDataObject* data_object)
{
  this->BlockColors.erase(data_object);
}

void vtkCompositeDataDisplayAttributes::RemoveBlockColors()
{
  if (this->HasBlockColors())
  {
    this->Modified();
  }
  this->BlockColors.clear();
}

void vtkCompositeDataDisplayAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkCompositeDataDisplayAttributes::SetBlockOpacity(vtkDataObject* data_object, double opacity)
{
  if (this->HasBlockOpacity(data_object) && this->GetBlockOpacity(data_object) == opacity)
  {
    return;
  }
  this->BlockOpacities[data_object] = opacity;
  this->Modified();
}

double vtkCompositeDataDisplayAttributes::GetBlockOpacity(vtkDataObject* data_object) const
{
  DoubleMap::const_iterator iter = this->BlockOpacities.find(data_object);

  if (iter != this->BlockOpacities.end())
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
  if (this->HasBlockOpacities())
  {
    this->Modified();
  }
  this->BlockOpacities.clear();
}

void vtkCompositeDataDisplayAttributes::SetBlockMaterial(
  vtkDataObject* data_object, const std::string& material)
{
  if (this->HasBlockMaterial(data_object) && this->GetBlockMaterial(data_object) == material)
  {
    return;
  }
  this->BlockMaterials[data_object] = material;
  this->Modified();
}

const std::string& vtkCompositeDataDisplayAttributes::GetBlockMaterial(
  vtkDataObject* data_object) const
{
  StringMap::const_iterator iter = this->BlockMaterials.find(data_object);

  if (iter != this->BlockMaterials.end())
  {
    return iter->second;
  }

  static const std::string nomat;
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
  if (this->HasBlockMaterials())
  {
    this->Modified();
  }
  this->BlockMaterials.clear();
}

void vtkCompositeDataDisplayAttributes::ComputeVisibleBounds(
  vtkCompositeDataDisplayAttributes* cda, vtkDataObject* dobj, double bounds[6])
{
  vtkMath::UninitializeBounds(bounds);
  // computing bounds with only visible blocks
  vtkBoundingBox bbox;
  vtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(cda, dobj, &bbox);
  if (bbox.IsValid())
  {
    bbox.GetBounds(bounds);
  }
}

void vtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(
  vtkCompositeDataDisplayAttributes* cda, vtkDataObject* dobj, vtkBoundingBox* bbox,
  bool parentVisible)
{
  if (!dobj || !bbox)
  {
    return;
  }

  // A block always *has* a visibility state, either explicitly set or inherited.
  bool blockVisible =
    (cda && cda->HasBlockVisibility(dobj)) ? cda->GetBlockVisibility(dobj) : parentVisible;

  vtkDataObjectTree* dObjTree = vtkDataObjectTree::SafeDownCast(dobj);
  if (dObjTree)
  {
    using Opts = vtk::DataObjectTreeOptions;
    for (vtkDataObject* child : vtk::Range(dObjTree, Opts::SkipEmptyNodes))
    {
      vtkCompositeDataDisplayAttributes::ComputeVisibleBoundsInternal(
        cda, child, bbox, blockVisible);
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

vtkDataObject* vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
  const unsigned int flat_index, vtkDataObject* parent_obj, unsigned int& current_flat_index)
{
  if (current_flat_index == flat_index)
  {
    return parent_obj;
  }
  current_flat_index++;

  // for leaf types quick continue, otherwise it recurses which
  // calls two more SafeDownCast which are expensive
  int dotype = parent_obj->GetDataObjectType();
  if (dotype < VTK_COMPOSITE_DATA_SET) // see vtkType.h
  {
    return nullptr;
  }

  vtkDataObjectTree* dObjTree = vtkDataObjectTree::SafeDownCast(parent_obj);
  if (dObjTree)
  {
    using Opts = vtk::DataObjectTreeOptions;
    for (vtkDataObject* child : vtk::Range(dObjTree, Opts::None))
    {
      if (child)
      {
        const auto data = vtkCompositeDataDisplayAttributes::DataObjectFromIndex(
          flat_index, child, current_flat_index);
        if (data)
        {
          return data;
        }
      }
    }
  }

  return nullptr;
}
