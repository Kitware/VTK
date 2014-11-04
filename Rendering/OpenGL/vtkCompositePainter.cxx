/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositePainter.h"

#include "vtkColor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

#include <cassert>

// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkCompositePainter)

vtkInformationKeyMacro(vtkCompositePainter, DISPLAY_ATTRIBUTES, ObjectBase);
vtkCxxSetObjectMacro(vtkCompositePainter, CompositeDataDisplayAttributes, vtkCompositeDataDisplayAttributes);
//----------------------------------------------------------------------------
vtkCompositePainter::vtkCompositePainter()
{
  this->OutputData = 0;
  this->CompositeDataDisplayAttributes = 0;
}

//----------------------------------------------------------------------------
vtkCompositePainter::~vtkCompositePainter()
{
  this->SetCompositeDataDisplayAttributes(0);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositePainter::GetOutput()
{
  return this->OutputData? this->OutputData : this->GetInput();
}

//----------------------------------------------------------------------------
void vtkCompositePainter::RenderInternal(vtkRenderer* renderer,
                                         vtkActor* actor,
                                         unsigned long typeflags,
                                         bool forceCompileOnly)
{
  vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(this->GetInput());
  if (!input || !this->DelegatePainter)
    {
    this->Superclass::RenderInternal(renderer, actor, typeflags,
                                     forceCompileOnly);
    return;
    }

  vtkHardwareSelector* selector = renderer->GetSelector();

  if (this->CompositeDataDisplayAttributes &&
      (this->CompositeDataDisplayAttributes->HasBlockOpacities() ||
       this->CompositeDataDisplayAttributes->HasBlockVisibilities() ||
       this->CompositeDataDisplayAttributes->HasBlockColors()))
    {
    vtkProperty* prop = actor->GetProperty();
    RenderBlockState state;

    // Push base-values on the state stack.
    state.Visibility.push(true);
    state.Opacity.push(prop->GetOpacity());
    state.AmbientColor.push(vtkColor3d(prop->GetAmbientColor()));
    state.DiffuseColor.push(vtkColor3d(prop->GetDiffuseColor()));
    state.SpecularColor.push(vtkColor3d(prop->GetSpecularColor()));

    // OpenGL currently knows how to render *this* state.
    state.RenderedOpacity = state.Opacity.top();
    state.RenderedAmbientColor = state.AmbientColor.top();
    state.RenderedDiffuseColor = state.DiffuseColor.top();
    state.RenderedSpecularColor = state.SpecularColor.top();

    // render using the composite data attributes
    unsigned int flat_index = 0;
    this->RenderBlock(renderer,
                      actor,
                      typeflags,
                      forceCompileOnly,
                      input,
                      flat_index,
                      state);

    // restore OpenGL state, if it was changed.
    this->UpdateRenderingState(renderer->GetRenderWindow(), actor->GetProperty(), state);
    }
  else
    {
    // render using the multi-block structure itself
    vtkCompositeDataIterator* iter = input->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataObject* dobj = iter->GetCurrentDataObject();
      if (dobj)
        {
        if (selector)
          {
          selector->BeginRenderProp();
          // If hardware selection is in progress, we need to pass the composite
          // index to the selection framework,
          selector->RenderCompositeIndex(iter->GetCurrentFlatIndex());
          }

        this->DelegatePainter->SetInput(dobj);
        this->OutputData = dobj;
        this->Superclass::RenderInternal(renderer, actor, typeflags,
                                         forceCompileOnly);
        this->OutputData = 0;

        if (selector)
          {
          selector->EndRenderProp();
          }
        }
      }
    iter->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkCompositePainter::RenderBlock(vtkRenderer *renderer,
                                      vtkActor *actor,
                                      unsigned long typeflags,
                                      bool forceCompileOnly,
                                      vtkDataObject *dobj,
                                      unsigned int &flat_index,
                                      vtkCompositePainter::RenderBlockState &state)
{
  assert("Sanity Check" &&
    (state.Visibility.size() > 0) &&
    (state.Opacity.size() > 0) &&
    (state.AmbientColor.size() > 0) &&
    (state.DiffuseColor.size() > 0) &&
    (state.SpecularColor.size() > 0));

  vtkHardwareSelector *selector = renderer->GetSelector();
  vtkProperty *property = actor->GetProperty();
  vtkCompositeDataDisplayAttributes* cda = this->CompositeDataDisplayAttributes;

  // A block always *has* a visibility state, either explicitly set or
  // inherited.
  state.Visibility.push(
    cda->HasBlockVisibility(flat_index) ?
    cda->GetBlockVisibility(flat_index) : state.Visibility.top());

  bool overrides_opacity = cda->HasBlockOpacity(flat_index);
  if (overrides_opacity)
    {
    state.Opacity.push(cda->GetBlockOpacity(flat_index));
    }

  bool overrides_color = cda->HasBlockColor(flat_index);
  if (overrides_color)
    {
    vtkColor3d color = cda->GetBlockColor(flat_index);
    state.AmbientColor.push(color);
    state.DiffuseColor.push(color);
    state.SpecularColor.push(color);
    }

  unsigned int my_flat_index = flat_index;
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
      if (child == NULL)
        {
        // speeds things up when dealing with NULL blocks (which is common with
        // AMRs).
        flat_index++;
        continue;
        }
      this->RenderBlock(renderer, actor, typeflags, forceCompileOnly,
        child, flat_index, state);
      }
    }
  else if (dobj && state.Visibility.top() == true && state.Opacity.top() > 0.0)
    {
    // Implies that the block is a non-null leaf node.
    // The top of the "stacks" have the state that this block must be rendered
    // with.
    if (selector)
      {
      selector->BeginRenderProp();
      selector->RenderCompositeIndex(my_flat_index);
      }
    else
      {
      // Not selecting, render the colors and stuff correctly for this block.
      // For that.
      this->UpdateRenderingState(renderer->GetRenderWindow(), property, state);
      }

    this->DelegatePainter->SetInput(dobj);
    this->OutputData = dobj;
    this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);
    this->OutputData = 0;
    if (selector)
      {
      selector->EndRenderProp();
      }
    }

  if (overrides_color)
    {
    state.AmbientColor.pop();
    state.DiffuseColor.pop();
    state.SpecularColor.pop();
    }
  if (overrides_opacity)
    {
    state.Opacity.pop();
    }
  state.Visibility.pop();
}

//-----------------------------------------------------------------------------
void vtkCompositePainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->OutputData, "Output");
}

//----------------------------------------------------------------------------
void vtkCompositePainter::ProcessInformation(vtkInformation* info)
{
  this->Superclass::ProcessInformation(info);

  if (info->Has(DISPLAY_ATTRIBUTES()))
    {
    this->SetCompositeDataDisplayAttributes(
      vtkCompositeDataDisplayAttributes::SafeDownCast(
        info->Get(DISPLAY_ATTRIBUTES())));
    }
}

//----------------------------------------------------------------------------
void vtkCompositePainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CompositeDataDisplayAttributes: " ;
  if (this->CompositeDataDisplayAttributes)
    {
    os << endl;
    this->CompositeDataDisplayAttributes->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
