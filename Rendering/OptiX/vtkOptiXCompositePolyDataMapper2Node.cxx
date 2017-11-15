/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXCompositePolyDataMapper2Node.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOptiXCompositePolyDataMapper2Node.h"

#include "vtkActor.h"
#include "vtkOptiXActorNode.h"
#include "vtkOptiXRendererNode.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkTexture.h"
#include "vtkSmartPointer.h"

#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"

#include <map>

//============================================================================
vtkStandardNewMacro(vtkOptiXCompositePolyDataMapper2Node);

//------------------------------------------------------------------------------
vtkOptiXCompositePolyDataMapper2Node::vtkOptiXCompositePolyDataMapper2Node()
{
}

//------------------------------------------------------------------------------
vtkOptiXCompositePolyDataMapper2Node::~vtkOptiXCompositePolyDataMapper2Node()
{
}

//------------------------------------------------------------------------------
void vtkOptiXCompositePolyDataMapper2Node::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOptiXCompositePolyDataMapper2Node::Render(bool prepass)
{
  if (prepass)
  {
    // we use a lot of params from our parent
    vtkOptiXActorNode *aNode = vtkOptiXActorNode::SafeDownCast(this->Parent);
    vtkActor *act = vtkActor::SafeDownCast(aNode->GetRenderable());

    if (act->GetVisibility() == false)
    {
      return;
    }

    vtkOptiXRendererNode *orn =
      static_cast<vtkOptiXRendererNode *>(
        this->GetFirstAncestorOfType("vtkOptiXRendererNode"));

    //if there are no changes, just reuse last result
    vtkMTimeType inTime = aNode->GetMTime();
    if (this->RenderTime >= inTime)
    {
      AddGeomToGroup(orn);
      return;
    }

    this->RenderTime = inTime;

    //something changed so make new meshes
    this->CreateNewMeshes();

    vtkProperty* prop = act->GetProperty();

    // Push base-values on the state stack.
    this->BlockState.Visibility.push(true);
    this->BlockState.Opacity.push(prop->GetOpacity());
    this->BlockState.AmbientColor.push(vtkColor3d(prop->GetAmbientColor()));
    this->BlockState.DiffuseColor.push(vtkColor3d(prop->GetDiffuseColor()));
    this->BlockState.SpecularColor.push(vtkColor3d(prop->GetSpecularColor()));

    // render using the composite data attributes
    unsigned int flat_index = 0;
    vtkCompositePolyDataMapper2 *cpdm =
      vtkCompositePolyDataMapper2::SafeDownCast(act->GetMapper());
    vtkDataObject * dobj = nullptr;
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

    AddGeomToGroup(orn);
  }
}



//------------------------------------------------------------------------------
void vtkOptiXCompositePolyDataMapper2Node::RenderBlock(
    vtkOptiXRendererNode *orn,
    vtkCompositePolyDataMapper2 *cpdm,
    vtkActor *actor,
    vtkDataObject *dobj,
    unsigned int &flat_index)
{
  vtkCompositeDataDisplayAttributes* cda = cpdm->GetCompositeDataDisplayAttributes();

  vtkProperty *prop = actor->GetProperty();
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

  // Advance flat-index. After this point, flat_index no longer points to this
  // block.
  flat_index++;

  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::SafeDownCast(dobj);
  vtkMultiPieceDataSet *mpds = vtkMultiPieceDataSet::SafeDownCast(dobj);
  if (mbds || mpds)
  {
    unsigned int numChildren = mbds? mbds->GetNumberOfBlocks() :
      mpds->GetNumberOfPieces();
    for (unsigned int cc=0 ; cc < numChildren; cc++)
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
    vtkPolyData *ds = vtkPolyData::SafeDownCast(dobj);
    if (ds)
    {
      vtkOptiXActorNode *aNode = vtkOptiXActorNode::SafeDownCast(this->Parent);
      vtkColor3d &aColor = this->BlockState.AmbientColor.top();
      vtkColor3d &dColor = this->BlockState.DiffuseColor.top();
      cpdm->ClearColorArrays(); //prevents reuse of stale color arrays
      this->RenderPoly(orn, aNode, ds, aColor.GetData(), dColor.GetData());
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
}
