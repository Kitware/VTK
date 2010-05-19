/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridToPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStructuredGridToPolyDataFilter.h"

#include "vtkInformation.h"
#include "vtkStructuredGrid.h"


//----------------------------------------------------------------------------
vtkStructuredGridToPolyDataFilter::vtkStructuredGridToPolyDataFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkStructuredGridToPolyDataFilter::~vtkStructuredGridToPolyDataFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkStructuredGridToPolyDataFilter::SetInput(vtkStructuredGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkStructuredGrid *vtkStructuredGridToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkStructuredGrid *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
int
vtkStructuredGridToPolyDataFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkStructuredGridToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
