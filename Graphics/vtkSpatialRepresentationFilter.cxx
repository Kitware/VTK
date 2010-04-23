/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepresentationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpatialRepresentationFilter.h"

#include "vtkLocator.h"
#include "vtkInformation.h"
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkSpatialRepresentationFilter);
vtkCxxSetObjectMacro(vtkSpatialRepresentationFilter,
                     SpatialRepresentation,vtkLocator);

vtkSpatialRepresentationFilter::vtkSpatialRepresentationFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
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

  if ( this->GetNumberOfOutputs() <= level ||
       this->Outputs[level] == NULL )
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
  this->Superclass::PrintSelf(os,indent);

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


//----------------------------------------------------------------------------
void vtkSpatialRepresentationFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // This filter shares our input and is therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->SpatialRepresentation,
                            "SpatialRepresentation");
}

//----------------------------------------------------------------------------
int vtkSpatialRepresentationFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
