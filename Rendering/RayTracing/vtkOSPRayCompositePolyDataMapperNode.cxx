// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOSPRayCompositePolyDataMapperNode.h"

#include "vtkActor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataObjectTree.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkSmartPointer.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOSPRayCompositePolyDataMapperNode);

//------------------------------------------------------------------------------
vtkOSPRayCompositePolyDataMapperNode::vtkOSPRayCompositePolyDataMapperNode() = default;

//------------------------------------------------------------------------------
vtkOSPRayCompositePolyDataMapperNode::~vtkOSPRayCompositePolyDataMapperNode() = default;

//------------------------------------------------------------------------------
void vtkOSPRayCompositePolyDataMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayCompositePolyDataMapperNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//------------------------------------------------------------------------------
void vtkOSPRayCompositePolyDataMapperNode::Render(bool prepass)
{
  if (prepass)
  {
    // we use a lot of params from our parent
    vtkOSPRayActorNode* aNode = vtkOSPRayActorNode::SafeDownCast(this->Parent);
    vtkActor* act = vtkActor::SafeDownCast(aNode->GetRenderable());

    if (act->GetVisibility() == false)
    {
      return;
    }

    vtkOSPRayRendererNode* orn =
      static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

    // if there are no changes, just reuse last result
    vtkMTimeType inTime = aNode->GetMTime();
    if (this->RenderTime >= inTime)
    {
      this->RenderGeometricModels();
      return;
    }
    this->RenderTime = inTime;
    this->ClearGeometricModels();

    vtkProperty* prop = act->GetProperty();

    // Push base-values on the state stack.
    this->BlockState.Visibility.push(true);
    this->BlockState.Opacity.push(prop->GetOpacity());
    this->BlockState.AmbientColor.push(vtkColor3d(prop->GetAmbientColor()));
    this->BlockState.DiffuseColor.push(vtkColor3d(prop->GetDiffuseColor()));
    this->BlockState.SpecularColor.push(vtkColor3d(prop->GetSpecularColor()));
    const char* mname = prop->GetMaterialName();
    if (mname != nullptr)
    {
      this->BlockState.Material.push(std::string(mname));
    }
    else
    {
      this->BlockState.Material.push(std::string(""));
    }

    // render using the composite data attributes
    unsigned int flat_index = 0;
    vtkCompositePolyDataMapper* cpdm = vtkCompositePolyDataMapper::SafeDownCast(act->GetMapper());
    vtkDataObject* dobj = nullptr;
    if (cpdm)
    {
      dobj = cpdm->GetInputDataObject(0, 0);
      if (dobj)
      {
        this->RenderBlock(orn, cpdm, act, dobj, flat_index);
      }
    }

    this->BlockState.Visibility.pop();
    this->BlockState.Opacity.pop();
    this->BlockState.AmbientColor.pop();
    this->BlockState.DiffuseColor.pop();
    this->BlockState.SpecularColor.pop();
    this->BlockState.Material.pop();

    this->RenderGeometricModels();
  }
}

//------------------------------------------------------------------------------
void vtkOSPRayCompositePolyDataMapperNode::RenderBlock(vtkOSPRayRendererNode* orn,
  vtkCompositePolyDataMapper* cpdm, vtkActor* actor, vtkDataObject* dobj, unsigned int& flat_index)
{
  vtkCompositeDataDisplayAttributes* cda = cpdm->GetCompositeDataDisplayAttributes();

  vtkProperty* prop = actor->GetProperty();
  // bool draw_surface_with_edges =
  //   (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);
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
        this->RenderBlock(orn, cpdm, actor, child, flat_index);
      }
      else
      {
        // speeds things up when dealing with nullptr blocks (which is common with AMRs).
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
      vtkOSPRayActorNode* aNode = vtkOSPRayActorNode::SafeDownCast(this->Parent);
      vtkColor3d& aColor = this->BlockState.AmbientColor.top();
      vtkColor3d& dColor = this->BlockState.DiffuseColor.top();
      std::string& material = this->BlockState.Material.top();
      cpdm->ClearColorArrays(); // prevents reuse of stale color arrays
      this->ORenderPoly(orn->GetORenderer(), aNode, ds, aColor.GetData(), dColor.GetData(),
        this->BlockState.Opacity.top(), material);
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
VTK_ABI_NAMESPACE_END
