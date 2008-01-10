/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupPolyDataMapper2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupPolyDataMapper2.h"

#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePainter.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataPainter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColorsPainter.h"
#include "vtkDisplayListPainter.h"
#include "vtkDefaultPainter.h"

vtkStandardNewMacro(vtkMultiGroupPolyDataMapper2);
vtkCxxRevisionMacro(vtkMultiGroupPolyDataMapper2, "1.2");
//----------------------------------------------------------------------------
vtkMultiGroupPolyDataMapper2::vtkMultiGroupPolyDataMapper2()
{
  this->ColorBlocks = 0;

  // Setup chain as
  // vtkDisplayListPainter -> vtkCompositePainter -> vtkDefaultPainter
  // -> vtkChooserPainter
  // vtkDefaultPainter no longer has another display list painter.
  vtkDefaultPainter* curPainter = 
    vtkDefaultPainter::SafeDownCast(this->GetPainter());
  curPainter->SetDisplayListPainter(0);

  vtkCompositePainter* cpainter = vtkCompositePainter::New();
  cpainter->SetDelegatePainter(curPainter);

  vtkDisplayListPainter* dlpainter = vtkDisplayListPainter::New();
  dlpainter->SetDelegatePainter(cpainter);
  this->SetPainter(dlpainter);
  
  cpainter->Delete();
  dlpainter->Delete();
}

//----------------------------------------------------------------------------
vtkMultiGroupPolyDataMapper2::~vtkMultiGroupPolyDataMapper2()
{
}

//----------------------------------------------------------------------------
int vtkMultiGroupPolyDataMapper2::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkMultiGroupPolyDataMapper2::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
void vtkMultiGroupPolyDataMapper2::RenderPiece(vtkRenderer* ren, vtkActor* act)
{
  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(inputDO);

  if (!inputCD)
    {
    this->Superclass::RenderPiece(ren, act);
    return;
    }

  //
  // make sure that we've been properly initialized
  //
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  if ( inputCD == NULL ) 
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  if (!this->Static)
    {
    inputCD->Update();
    }
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  // make sure our window is current
  ren->GetRenderWindow()->MakeCurrent();
  this->TimeToDraw = 0.0;
  if (this->Painter)
    {
    // Update Painter information if obsolete.
    if (this->PainterUpdateTime < this->GetMTime())
      {
      this->UpdatePainterInformation();
      this->PainterUpdateTime.Modified();
      }

    // Pass polydata if changed.
    if (this->Painter->GetInput() != inputDO)
      {
      this->Painter->SetInput(inputDO);
      }
    this->Painter->Render(ren, act, 0xff);
    this->TimeToDraw = this->Painter->GetTimeToDraw();
    }

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
    {
    this->TimeToDraw = 0.0001;
    }

  this->UpdateProgress(1.0);
}

//-----------------------------------------------------------------------------
void vtkMultiGroupPolyDataMapper2::Render(vtkRenderer *ren, vtkActor *act) 
{
  if (this->Static)
    {
    this->RenderPiece(ren,act);
    return;
    }
  
  int currentPiece, nPieces;
  vtkDataObject *input = this->GetInputDataObject(0, 0);
  
  if (input == NULL)
    {
    vtkErrorMacro("Mapper has no input.");
    return;
    }
  
  nPieces = this->NumberOfPieces * this->NumberOfSubPieces;

  for(int i=0; i<this->NumberOfSubPieces; i++)
    {
    // If more than one pieces, render in loop.
    currentPiece = this->NumberOfSubPieces * this->Piece + i;
    input->SetUpdateExtent(currentPiece, nPieces, this->GhostLevel);
    this->RenderPiece(ren, act);
    }
}

//-----------------------------------------------------------------------------
//Looks at each DataSet and finds the union of all the bounds
void vtkMultiGroupPolyDataMapper2::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    this->GetInputDataObject(0, 0));

  input->Update();

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if(!input) 
    {
    this->Superclass::GetBounds();
    return;
    }
  
  // We do have hierarchical data - so we need to loop over
  // it and get the total bounds.
  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->GoToFirstItem();  
  double bounds[6];
  int i;
  
  while (!iter->IsDoneWithTraversal())
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());    
    if (pd)
      {
      // If this isn't the first time through, expand bounds
      // we've compute so far based on the bounds of this
      // block
      if ( vtkMath::AreBoundsInitialized(this->Bounds) )
        {
        pd->GetBounds(bounds);
        for(i=0; i<3; i++)
          {
          this->Bounds[i*2] = 
            (bounds[i*2]<this->Bounds[i*2])?
            (bounds[i*2]):(this->Bounds[i*2]);
          this->Bounds[i*2+1] = 
            (bounds[i*2+1]>this->Bounds[i*2+1])?
            (bounds[i*2+1]):(this->Bounds[i*2+1]);
          }
        }
      // If this is our first time through, just get the bounds
      // of the data as the initial bounds
      else
        {
        pd->GetBounds(this->Bounds);
        }
      }
    iter->GoToNextItem();
    }
  iter->Delete();
  this->BoundsMTime.Modified();
}

//-----------------------------------------------------------------------------
double* vtkMultiGroupPolyDataMapper2::GetBounds()
{
  static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};
  
  if ( !this->GetExecutive()->GetInputData(0, 0) ) 
    {
    return bounds;
    }
  else
    {

    this->Update();
    
    //only compute bounds when the input data has changed
    vtkCompositeDataPipeline * executive = vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
    if( executive->GetPipelineMTime() > this->BoundsMTime.GetMTime() )
      {
      this->ComputeBounds();
      }
    
    return this->Bounds;
    }
}

//-----------------------------------------------------------------------------
void vtkMultiGroupPolyDataMapper2::UpdatePainterInformation()
{
  vtkInformation* info = this->PainterInformation;
  this->Superclass::UpdatePainterInformation();
  if (this->ColorBlocks)
    {
    info->Set(vtkScalarsToColorsPainter::SCALAR_VISIBILITY(), 0);
    }
  info->Set(vtkCompositePainter::COLOR_LEAVES(), this->ColorBlocks);
}

//----------------------------------------------------------------------------
void vtkMultiGroupPolyDataMapper2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorBlocks: " << this->ColorBlocks << endl;
}

