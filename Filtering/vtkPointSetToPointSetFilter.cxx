/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToPointSetFilter.cxx
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
#include "vtkPointSetToPointSetFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkPointSetToPointSetFilter, "1.57");

//----------------------------------------------------------------------------
// Construct object.
vtkPointSetToPointSetFilter::vtkPointSetToPointSetFilter()
{
  this->NumberOfRequiredInputs = 1;
}

//----------------------------------------------------------------------------
vtkPointSetToPointSetFilter::~vtkPointSetToPointSetFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkPointSetToPointSetFilter::SetInput(vtkPointSet *input)
{
  vtkPointSet *oldInput = this->GetInput();
  
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

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkPointSet *vtkPointSetToPointSetFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkPointSet *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// Get the output of this filter. If output is NULL then input hasn't been set
// which is necessary for abstract objects.
vtkPointSet *vtkPointSetToPointSetFilter::GetOutput()
{
  if (this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before output can be retrieved");
    return NULL;
    }

  return this->vtkPointSetSource::GetOutput();
}

   
//----------------------------------------------------------------------------
// Get the output as vtkPolyData.
vtkPolyData *vtkPointSetToPointSetFilter::GetPolyDataOutput() 
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

//----------------------------------------------------------------------------
// Get the output as vtkStructuredGrid. Performs run-time checking.
vtkStructuredGrid *vtkPointSetToPointSetFilter::GetStructuredGridOutput()
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

//----------------------------------------------------------------------------
// Get the output as vtkUnstructuredGrid. Performs run-time checking.
vtkUnstructuredGrid *vtkPointSetToPointSetFilter::GetUnstructuredGridOutput()
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

//----------------------------------------------------------------------------
// Copy the update information across
void vtkPointSetToPointSetFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  input->SetUpdatePiece( output->GetUpdatePiece() );
  input->SetUpdateNumberOfPieces( output->GetUpdateNumberOfPieces() );
  input->SetUpdateGhostLevel( output->GetUpdateGhostLevel() );
  input->SetUpdateExtent( output->GetUpdateExtent() );
  input->RequestExactExtentOn();  
}















