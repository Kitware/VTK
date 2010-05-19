/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredPointsToPolyDataFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"


//----------------------------------------------------------------------------
vtkStructuredPointsToPolyDataFilter::vtkStructuredPointsToPolyDataFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkStructuredPointsToPolyDataFilter::~vtkStructuredPointsToPolyDataFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredPointsToPolyDataFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkStructuredPointsToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkImageData *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToPolyDataFilter::ComputeInputUpdateExtents( 
                                                        vtkDataObject *output)
{
  this->vtkPolyDataSource::ComputeInputUpdateExtents(output);

  if (!this->GetInput())
    {
    return;
    }
  // assume that we cannot handle more than the requested extent.
  this->GetInput()->RequestExactExtentOn();
}

//----------------------------------------------------------------------------
int vtkStructuredPointsToPolyDataFilter::FillInputPortInformation(
  int port,
  vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
