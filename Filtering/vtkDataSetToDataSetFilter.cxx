/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataSetFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetToDataSetFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"

vtkCxxRevisionMacro(vtkDataSetToDataSetFilter, "1.61");

// Construct object.
vtkDataSetToDataSetFilter::vtkDataSetToDataSetFilter()
{
  this->NumberOfRequiredInputs = 1;
}

vtkDataSetToDataSetFilter::~vtkDataSetToDataSetFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetToDataSetFilter::SetInput(vtkDataSet *input)
{
  vtkDataSet *oldInput = this->GetInput();
  
  if (oldInput != NULL)
    {
    if (input == NULL || 
        oldInput->GetDataObjectType() != input->GetDataObjectType())
      {
      vtkWarningMacro("Changing input type.  Deleting output");
      this->SetOutput(NULL);
      }
    }
  
  if (input != NULL && this->vtkSource::GetOutput(0) == NULL)
    {
    this->vtkSource::SetNthOutput(0, input->MakeObject());
    this->Outputs[0]->ReleaseData();
    this->Outputs[0]->Delete();
    }
  
  this->vtkProcessObject::SetNthInput(0, input);
}

// Get the output of this filter. If output is NULL then input hasn't been set
// which is necessary for abstract objects.
vtkDataSet *vtkDataSetToDataSetFilter::GetOutput()
{
  if (this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before output can be retrieved");
    return NULL;
    }

  // sanity check
  if (this->NumberOfOutputs < 1)
    {
    vtkErrorMacro("Sanity check failed. We should have an output");
    return NULL;
    }

  return (vtkDataSet *)this->Outputs[0];
}

// Get the output as vtkPolyData.
vtkPolyData *vtkDataSetToDataSetFilter::GetPolyDataOutput() 
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_POLY_DATA)
    {
    return (vtkPolyData *)ds;
    }
  return NULL;
}

// Get the output as vtkStructuredPoints.
vtkStructuredPoints *vtkDataSetToDataSetFilter::GetStructuredPointsOutput() 
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_STRUCTURED_POINTS)
    {
    return (vtkStructuredPoints *)ds;
    }
  return NULL;
}

// Get the output as vtkStructuredGrid.
vtkStructuredGrid *vtkDataSetToDataSetFilter::GetStructuredGridOutput()
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    return (vtkStructuredGrid *)ds;
    }
  return NULL;
}

// Get the output as vtkUnstructuredGrid.
vtkUnstructuredGrid *vtkDataSetToDataSetFilter::GetUnstructuredGridOutput()
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
    {
    return (vtkUnstructuredGrid *)ds;
    }
  return NULL;
}

// Get the output as vtkRectilinearGrid. 
vtkRectilinearGrid *vtkDataSetToDataSetFilter::GetRectilinearGridOutput()
{
  vtkDataSet *ds = this->GetOutput();
  if (!ds) 
    {
    return NULL;
    }
  if (ds->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    return (vtkRectilinearGrid *)ds;
    }
  return NULL;
}



//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetToDataSetFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// We know input and output match in type - call the type specific version of
// copy information
void vtkDataSetToDataSetFilter::ExecuteInformation()
{
  this->GetOutput()->CopyTypeSpecificInformation( this->GetInput() );
}

//----------------------------------------------------------------------------

// Copy the update information across
void vtkDataSetToDataSetFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  input->SetUpdatePiece( output->GetUpdatePiece() );
  input->SetUpdateNumberOfPieces( output->GetUpdateNumberOfPieces() );
  input->SetUpdateGhostLevel( output->GetUpdateGhostLevel() );
  input->SetUpdateExtent( output->GetUpdateExtent() );
  input->RequestExactExtentOn();
}
