/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToUnstructuredGridFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredPointsToUnstructuredGridFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkStructuredPoints.h"


//----------------------------------------------------------------------------
vtkStructuredPointsToUnstructuredGridFilter::vtkStructuredPointsToUnstructuredGridFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkStructuredPointsToUnstructuredGridFilter::~vtkStructuredPointsToUnstructuredGridFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredPointsToUnstructuredGridFilter::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkStructuredPointsToUnstructuredGridFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkStructuredPoints *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToUnstructuredGridFilter::ComputeInputUpdateExtents( 
                                                        vtkDataObject *output)
{
  this->vtkUnstructuredGridSource::ComputeInputUpdateExtents(output);

  // assume that we cannot handle more than the requested extent.
  if (this->GetInput())
    {
    this->GetInput()->RequestExactExtentOn();
    }
}

//----------------------------------------------------------------------------
int
vtkStructuredPointsToUnstructuredGridFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkStructuredPointsToUnstructuredGridFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
