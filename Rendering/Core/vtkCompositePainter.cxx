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

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkHardwareSelector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkPolyData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkCompositeDataDisplayAttributes.h"

vtkStandardNewMacro(vtkCompositePainter);
//----------------------------------------------------------------------------
vtkCompositePainter::vtkCompositePainter()
{
  this->OutputData = 0;
}

//----------------------------------------------------------------------------
vtkCompositePainter::~vtkCompositePainter()
{
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

  if(this->CompositeDataDisplayAttributes)
    {
    // render using the composite data attributes
    unsigned int flat_index = 0;
    bool visible = true;
    this->RenderBlock(renderer, actor, typeflags, forceCompileOnly, input, flat_index, visible);
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
                                      bool &visible)
{
    vtkHardwareSelector *selector = renderer->GetSelector();

    // push display attributes
    bool prev_visible = visible;
    if(this->CompositeDataDisplayAttributes->HasBlockVisibility(flat_index))
      {
      visible = this->CompositeDataDisplayAttributes->GetBlockVisibility(flat_index);
      }

    vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::SafeDownCast(dobj);
    vtkMultiPieceDataSet *mpds = vtkMultiPieceDataSet::SafeDownCast(dobj);
    if(mbds || mpds)
      {
      // recurse down to child blocks
      unsigned int childCount =
        mbds ? mbds->GetNumberOfBlocks() : mpds->GetNumberOfPieces();
      for(unsigned int i = 0; i < childCount; i++)
        {
        flat_index++;
        this->RenderBlock(renderer,
                          actor,
                          typeflags,
                          forceCompileOnly,
                          mbds ? mbds->GetBlock(i) : mpds->GetPiece(i),
                          flat_index,
                          visible);
        }

      // pop display attributes
      if(this->CompositeDataDisplayAttributes->HasBlockVisibility(flat_index))
        {
        visible = prev_visible;
        }
      flat_index++;
      }
    else if(dobj)
      {
      // render leaf-node
      if(visible)
        {
        if(selector)
          {
          selector->BeginRenderProp();
          // If hardware selection is in progress, we need to pass the composite
          // index to the selection framework,
          selector->RenderCompositeIndex(flat_index);
          }

        this->DelegatePainter->SetInput(dobj);
        this->OutputData = dobj;
        this->Superclass::RenderInternal(renderer,
                                         actor,
                                         typeflags,
                                         forceCompileOnly);
        this->OutputData = 0;

        if(selector)
          {
          selector->EndRenderProp();
          }
        }

        // pop display attributes
        if(this->CompositeDataDisplayAttributes->HasBlockVisibility(flat_index))
          {
          visible = prev_visible;
          }
      }
}

//-----------------------------------------------------------------------------
void vtkCompositePainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->OutputData, "Output");
}

//----------------------------------------------------------------------------
void vtkCompositePainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

