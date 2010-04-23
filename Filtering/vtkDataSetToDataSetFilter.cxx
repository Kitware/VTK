/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetToDataSetFilter.h"

#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"


// Construct object.
vtkDataSetToDataSetFilter::vtkDataSetToDataSetFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

vtkDataSetToDataSetFilter::~vtkDataSetToDataSetFilter()
{
}

int vtkDataSetToDataSetFilter::RequestDataObject(vtkInformation*,
                                            vtkInformationVector** inputVector,
                                            vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  if (input)
    {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataSet *output = vtkDataSet::SafeDownCast(
      info->Get(vtkDataObject::DATA_OBJECT()));
    
    if (!output || !output->IsA(input->GetClassName())) 
      {
      output = input->NewInstance();
      output->SetPipelineInformation(info);
      output->Delete();
      }
    return 1;
    }
  return 0;
}

int vtkDataSetToDataSetFilter::ProcessRequest(vtkInformation* request,
                                              vtkInformationVector** inputVector,
                                              vtkInformationVector* outputVector)
{
  // Handling REQUEST_DATA_OBJECT because if the filter is connected
  // with SetInputConnection() as opposed to SetInput(), the output
  // never gets created
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
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
    this->vtkSource::SetNthOutput(0, input->NewInstance());
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

  return this->Superclass::GetOutput(0);
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
    return static_cast<vtkPolyData *>(ds);
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
    return static_cast<vtkStructuredPoints *>(ds);
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
    return static_cast<vtkStructuredGrid *>(ds);
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
    return static_cast<vtkUnstructuredGrid *>(ds);
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
    return static_cast<vtkRectilinearGrid *>(ds);
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
  
  return static_cast<vtkDataSet *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// We know input and output match in type - call the type specific version of
// copy information
void vtkDataSetToDataSetFilter::ExecuteInformation()
{
  vtkDataSet* output = this->GetOutput();
  if(output)
    {
    output->CopyTypeSpecificInformation( this->GetInput() );
    }
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

//----------------------------------------------------------------------------
vtkDataSet *vtkDataSetToDataSetFilter::GetOutput(int idx)
{
  return this->vtkDataSetSource::GetOutput(idx);
}

//----------------------------------------------------------------------------
int
vtkDataSetToDataSetFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataSetToDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
