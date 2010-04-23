/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsToStructuredPointsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredPointsToStructuredPointsFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkStructuredPoints.h"


// ----------------------------------------------------------------------------
vtkStructuredPointsToStructuredPointsFilter::vtkStructuredPointsToStructuredPointsFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

// ----------------------------------------------------------------------------
vtkStructuredPointsToStructuredPointsFilter::~vtkStructuredPointsToStructuredPointsFilter()
{
}

// ----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredPointsToStructuredPointsFilter::SetInput(
                                                   vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

// ----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkStructuredPointsToStructuredPointsFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkStructuredPoints *>(this->Inputs[0]);
}


// ----------------------------------------------------------------------------
// Copy WholeExtent, Spacing and Origin.
void vtkStructuredPointsToStructuredPointsFilter::ExecuteInformation()
{
  vtkImageData *input = this->GetInput();
  vtkStructuredPoints *output = this->GetOutput();
  
  if (output == NULL || input == NULL)
    {
    return;
    }
  
  output->SetWholeExtent(input->GetWholeExtent());
  output->SetSpacing(input->GetSpacing());
  output->SetOrigin(input->GetOrigin());
}


// ----------------------------------------------------------------------------
void vtkStructuredPointsToStructuredPointsFilter::ComputeInputUpdateExtents( 
                                                        vtkDataObject *output)
{
  this->vtkStructuredPointsSource::ComputeInputUpdateExtents(output);

  // assume that we cannot handle more than the requested extent.
  this->GetInput()->RequestExactExtentOn();
}

// ----------------------------------------------------------------------------
int
vtkStructuredPointsToStructuredPointsFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

// ----------------------------------------------------------------------------
void vtkStructuredPointsToStructuredPointsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
