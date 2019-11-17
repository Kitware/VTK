/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayCompositePolyDataMapper2Node.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayCompositePolyDataMapper2Node.h"

#include "vtkActor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkOSPRayActorNode.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

//============================================================================
vtkStandardNewMacro(vtkOSPRayCompositePolyDataMapper2Node);

//----------------------------------------------------------------------------
vtkOSPRayCompositePolyDataMapper2Node::vtkOSPRayCompositePolyDataMapper2Node() {}

//----------------------------------------------------------------------------
vtkOSPRayCompositePolyDataMapper2Node::~vtkOSPRayCompositePolyDataMapper2Node() {}

//----------------------------------------------------------------------------
void vtkOSPRayCompositePolyDataMapper2Node::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayCompositePolyDataMapper2Node::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayCompositePolyDataMapper2Node::Render(bool prepass)
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
    double tstep = vtkOSPRayRendererNode::GetViewTime(orn->GetRenderer());
    vtkRenderer* ren = vtkRenderer::SafeDownCast(orn->GetRenderable());
    this->InstanceCache->SetSize(vtkOSPRayRendererNode::GetTimeCacheSize(ren));
    this->GeometryCache->SetSize(vtkOSPRayRendererNode::GetTimeCacheSize(ren));

    // if there are no changes, just reuse last result
    vtkMTimeType inTime = aNode->GetMTime();
    if (this->RenderTime >= inTime ||
      (this->UseInstanceCache && this->InstanceCache->Contains(tstep)) ||
      (this->UseGeometryCache && this->GeometryCache->Contains(tstep)))
    {
      this->RenderGeometries();
      return;
    }
    this->RenderTime = inTime;
    this->ClearGeometries();

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
    vtkCompositePolyDataMapper2* cpdm = vtkCompositePolyDataMapper2::SafeDownCast(act->GetMapper());
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

    this->PopulateCache();
    this->RenderGeometries();
  }
}

//-----------------------------------------------------------------------------
void vtkOSPRayCompositePolyDataMapper2Node::RenderBlock(vtkOSPRayRendererNode* orn,
  vtkCompositePolyDataMapper2* cpdm, vtkActor* actor, vtkDataObject* dobj, unsigned int& flat_index)
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

  // Advance flat-index. After this point, flat_index no longer points to this
  // block.
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
        // speeds things up when dealing with nullptr blocks (which is common with
        // AMRs).
        flat_index++;
        continue;
      }
      this->RenderBlock(orn, cpdm, actor, child, flat_index);
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
