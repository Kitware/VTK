// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariCompositePolyDataMapperNode.h"
#include "vtkAnariActorNode.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariSceneGraph.h"

#include "vtkActor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTree.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"

VTK_ABI_NAMESPACE_BEGIN

//============================================================================
vtkStandardNewMacro(vtkAnariCompositePolyDataMapperNode);

//------------------------------------------------------------------------------
void vtkAnariCompositePolyDataMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkAnariCompositePolyDataMapperNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//------------------------------------------------------------------------------
void vtkAnariCompositePolyDataMapperNode::Synchronize(bool prepass)
{
  vtkAnariProfiling startProfiling(
    "vtkAnariCompositePolyDataMapperNode::Render", vtkAnariProfiling::BROWN);

  if (!prepass || !ActorWasModified())
  {
    return;
  }

  this->RenderTime = this->GetVtkActor()->GetMTime();
  this->ClearSurfaces();

  vtkActor* act = this->GetVtkActor();
  if (!act->GetVisibility())
  {
    return;
  }

  this->SetActorNodeName();

  vtkProperty* prop = act->GetProperty();

  // Push base-values on the state stack.
  this->BlockState.Visibility.push(true);
  this->BlockState.Opacity.push(prop->GetOpacity());
  this->BlockState.AmbientColor.push(vtkColor3d(prop->GetAmbientColor()));
  this->BlockState.DiffuseColor.push(vtkColor3d(prop->GetDiffuseColor()));
  this->BlockState.SpecularColor.push(vtkColor3d(prop->GetSpecularColor()));

  const char* materialName = prop->GetMaterialName();

  if (materialName != nullptr)
  {
    this->BlockState.Material.push(std::string(materialName));
  }
  else
  {
    this->BlockState.Material.push(std::string("matte"));
  }

  // render using the composite data attributes
  unsigned int flat_index = 0;
  vtkMapper* baseMapper = vtkMapper::SafeDownCast(this->GetRenderable());
  vtkDataObject* dobj = nullptr;

  if (baseMapper)
  {
    dobj = baseMapper->GetInputDataObject(0, 0);

    if (dobj)
    {
      this->SynchronizeBlock(baseMapper, act, dobj, flat_index);
    }
  }

  this->BlockState.Visibility.pop();
  this->BlockState.Opacity.pop();
  this->BlockState.AmbientColor.pop();
  this->BlockState.DiffuseColor.pop();
  this->BlockState.SpecularColor.pop();
  this->BlockState.Material.pop();
}

//------------------------------------------------------------------------------
void vtkAnariCompositePolyDataMapperNode::SynchronizeBlock(
  vtkMapper* baseMapper, vtkActor* actor, vtkDataObject* dobj, unsigned int& flat_index)
{
  vtkAnariProfiling startProfiling(
    "vtkAnariCompositePolyDataMapperNode::SynchronizeBlock", vtkAnariProfiling::BROWN);

  vtkCompositeDataDisplayAttributes* cda = this->GetCompositeDisplayAttributes();

  vtkProperty* prop = actor->GetProperty();
  vtkColor3d ecolor(prop->GetEdgeColor());

  bool overrides_visibility = (cda && cda->HasBlockVisibility(dobj));
  if (overrides_visibility)
  {
    this->BlockState.Visibility.push(cda->GetBlockVisibility(dobj));
  }

  bool overrides_opacity = (cda && cda->HasBlockOpacity(dobj));
  if (overrides_opacity)
  {
    this->BlockState.Opacity.push(cda->GetBlockOpacity(dobj));
  }

  bool overrides_color = (cda && cda->HasBlockColor(dobj));
  if (overrides_color)
  {
    vtkColor3d color = cda->GetBlockColor(dobj);
    this->BlockState.AmbientColor.push(color);
    this->BlockState.DiffuseColor.push(color);
    this->BlockState.SpecularColor.push(color);
  }

  bool overrides_material = (cda && cda->HasBlockMaterial(dobj));
  if (overrides_material)
  {
    std::string material = cda->GetBlockMaterial(dobj);
    this->BlockState.Material.push(material);
  }

  // Advance flat-index. After this point, flat_index no longer points to this block.
  flat_index++;

  if (auto dataObjTree = vtkDataObjectTree::SafeDownCast(dobj))
  {
    for (unsigned int i = 0, numChildren = dataObjTree->GetNumberOfChildren(); i < numChildren; ++i)
    {
      if (auto child = dataObjTree->GetChild(i))
      {
        this->SynchronizeBlock(baseMapper, actor, child, flat_index);
      }
      else
      {
        // speeds things up when dealing with nullptr blocks (which is common with
        // AMRs).
        flat_index++;
      }
    }
  }
  else if (dobj && this->BlockState.Visibility.top() == true &&
    this->BlockState.Opacity.top() > 0.0)
  {
    // do we have a entry for this dataset?
    // make sure we have an entry for this dataset
    vtkPolyData* ds = vtkPolyData::SafeDownCast(dobj);

    if (ds)
    {
      vtkAnariActorNode* aNode = vtkAnariActorNode::SafeDownCast(this->Parent);
      vtkColor3d& aColor = this->BlockState.AmbientColor.top();
      vtkColor3d& dColor = this->BlockState.DiffuseColor.top();
      std::string& material = this->BlockState.Material.top();
      baseMapper->ClearColorArrays(); // prevents reuse of stale color arrays

      double color[3] = { aColor.GetRed() * dColor.GetRed(), aColor.GetGreen() * dColor.GetGreen(),
        aColor.GetBlue() * dColor.GetBlue() };
      this->AnariRenderPoly(aNode, ds, color, this->BlockState.Opacity.top(), material);
    }
  }

  if (overrides_color)
  {
    this->BlockState.AmbientColor.pop();
    this->BlockState.DiffuseColor.pop();
    this->BlockState.SpecularColor.pop();
  }
  if (overrides_opacity)
  {
    this->BlockState.Opacity.pop();
  }
  if (overrides_visibility)
  {
    this->BlockState.Visibility.pop();
  }
  if (overrides_material)
  {
    this->BlockState.Material.pop();
  }
}

//----------------------------------------------------------------------------
vtkCompositeDataDisplayAttributes*
vtkAnariCompositePolyDataMapperNode::GetCompositeDisplayAttributes()
{
  vtkCompositePolyDataMapper* cpdm =
    vtkCompositePolyDataMapper::SafeDownCast(this->GetRenderable());
  return cpdm ? cpdm->GetCompositeDataDisplayAttributes() : nullptr;
}

VTK_ABI_NAMESPACE_END
