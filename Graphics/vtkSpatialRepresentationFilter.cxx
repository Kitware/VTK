/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepresentationFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkSpatialRepresentationFilter.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------------
vtkSpatialRepresentationFilter* vtkSpatialRepresentationFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSpatialRepresentationFilter");
  if(ret)
    {
    return (vtkSpatialRepresentationFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSpatialRepresentationFilter;
}




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
    this->vtkSource::SetNthOutput(level, vtkPolyData::New());
    this->Modified(); //asking for new output
    }

  return (vtkPolyData *)(this->Outputs[level]);
}

void vtkSpatialRepresentationFilter::ResetOutput()
{
  this->TerminalNodesRequested = 0;
  for ( int i=0; i <= VTK_MAX_SPATIAL_REP_LEVEL; i++)
    {
    this->vtkSource::SetNthOutput(i, NULL);
    }
}    


// Build OBB tree
void vtkSpatialRepresentationFilter::Execute()
{
  vtkDebugMacro(<<"Building OBB representation");

  if (this->SpatialRepresentation == NULL)
    {
    vtkErrorMacro(<< "SpatialRepresentation is NULL.");
    return;
    }

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
  if (!input)
    {
    return;
    }
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
  this->vtkProcessObject::SetNthInput(0, input);
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
