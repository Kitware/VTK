/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepresentationFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkSpatialRepresentationFilter.h"

vtkSpatialRepresentationFilter::vtkSpatialRepresentationFilter()
{
  this->SpatialRepresentation = NULL;
  this->Level = 0;
  this->TerminalNodesRequested = 0;
}

vtkSpatialRepresentationFilter::~vtkSpatialRepresentationFilter()
{
  if ( this->SpatialRepresentation )
    {
    this->SpatialRepresentation->UnRegister(this);
    this->SpatialRepresentation = NULL;
    }
}

vtkPolyData *vtkSpatialRepresentationFilter::GetOutput()
{
  if ( !this->TerminalNodesRequested )
    {
    this->TerminalNodesRequested = 1;
    this->Modified();
    }
  return this->vtkPolyDataSource::GetOutput();
}

vtkPolyData *vtkSpatialRepresentationFilter::GetOutput(int level)
{
  if ( level < 0 || !this->SpatialRepresentation || 
  level > this->SpatialRepresentation->GetMaxLevel() )
    {
    vtkErrorMacro(<<"Level requested is <0 or >= Locator's MaxLevel");
    return this->GetOutput();
    }

  if ( this->Outputs[level] == NULL )
    {
    this->vtkSource::SetOutput(level, vtkPolyData::New());
    this->Modified(); //asking for new output
    }

  return (vtkPolyData *)(this->Outputs[level]);
}

void vtkSpatialRepresentationFilter::ResetOutput()
{
  this->TerminalNodesRequested = 0;
  for ( int i=0; i <= VTK_MAX_SPATIAL_REP_LEVEL; i++)
    {
    this->vtkSource::SetOutput(i, NULL);
    }
}

    
// Update input to this filter and the filter itself.
void vtkSpatialRepresentationFilter::Update()
{
  vtkDataSet *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  
  // make sure input is available
  if ( ! input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }

  this->Updating = 1;
  input->Update();
  this->Updating = 0;

  // execute
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }
  output->Initialize(); //clear output
  // reset AbortExecute flag and Progress
  this->AbortExecute = 0;
  this->Progress = 0.0;
  this->Execute();
  if ( !this->AbortExecute )
    {
    this->UpdateProgress(1.0);
    }
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }

  // clean up
  if ( input->ShouldIReleaseData() )
    {
    input->ReleaseData();
    }
}


// Build OBB tree
void vtkSpatialRepresentationFilter::Execute()
{
  vtkDebugMacro(<<"Building OBB representation");

  this->SpatialRepresentation->SetDataSet(this->GetInput());
  this->SpatialRepresentation->Update();
  this->Level = this->SpatialRepresentation->GetLevel();

  vtkDebugMacro(<<"OBB deepest tree level: " << this->Level);
  this->GenerateOutput();
}

// Generate OBB representations at different requested levels.
void vtkSpatialRepresentationFilter::GenerateOutput()
{
  vtkDataSet *input = this->GetInput();
  vtkPolyData *output;
  int inputModified=(input->GetMTime() > this->GetMTime() ? 1 : 0);
  int i;

  //
  // If input to filter is modified, have to update all levels of OBB
  //
  if ( inputModified )
    {
    for ( i=0; i <= this->Level; i++ )
      {
      if ( i < this->NumberOfOutputs && this->Outputs[i] != NULL )
	{
	output = (vtkPolyData *)(this->Outputs[i]);
	output->Initialize();
	}
      }
    }

  //
  // Loop over all requested levels generating new levels as necessary
  //
  for ( i=0; i <= this->Level && i < this->NumberOfOutputs; i++ )
    {
    output = (vtkPolyData *)(this->Outputs[i]);
    if ( output != NULL && output->GetNumberOfPoints() < 1 ) //compute OBB
      {
      this->SpatialRepresentation->GenerateRepresentation(i, output);
      }
    }
  //
  // If terminal leafs requested, generate rep
  //
  if ( this->TerminalNodesRequested )
    {
    output = this->GetOutput();
    this->SpatialRepresentation->GenerateRepresentation(-1, output);
    }
}

void vtkSpatialRepresentationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "Level: " << this->Level << "\n";

  if ( this->SpatialRepresentation )
    {
    os << indent << "Spatial Representation: " << this->SpatialRepresentation
       << "\n";
    }
  else
    {
    os << indent << "Spatial Representation: (none)\n";
    }
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkSpatialRepresentationFilter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkSpatialRepresentationFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}
