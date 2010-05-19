/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToImageFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetToImageFilter.h"

#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"


//----------------------------------------------------------------------------
vtkDataSetToImageFilter::vtkDataSetToImageFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkDataSetToImageFilter::~vtkDataSetToImageFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetToImageFilter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetToImageFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkDataSet *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// All the DataSetToImageFilters require all their input.
void vtkDataSetToImageFilter::ComputeInputUpdateExtents(
  vtkDataObject *data)
{
  vtkImageData *output = static_cast<vtkImageData *>(data);
  vtkDataSet *input = this->GetInput();
  int *ext;
  
  if (input == NULL)
    {
    return;
    }
  
  // Lets just check to see if the outputs UpdateExtent is valid.
  ext = output->GetUpdateExtent();
  if (ext[0] > ext[1] || ext[2] > ext[3] || ext[4] > ext[5])
    {
    return;
    }
  
  input->SetUpdateExtent(0, 1, 0);
}

//----------------------------------------------------------------------------
int
vtkDataSetToImageFilter
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
void vtkDataSetToImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
