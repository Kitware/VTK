/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxToPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxToPolyDataFilter.h"

#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"

vtkCxxRevisionMacro(vtkHierarchicalBoxToPolyDataFilter, "1.4");

//----------------------------------------------------------------------------
vtkHierarchicalBoxToPolyDataFilter::vtkHierarchicalBoxToPolyDataFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxToPolyDataFilter::~vtkHierarchicalBoxToPolyDataFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkHierarchicalBoxToPolyDataFilter::SetInput(
  vtkHierarchicalBoxDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkHierarchicalBoxDataSet *vtkHierarchicalBoxToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkHierarchicalBoxDataSet *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
int
vtkHierarchicalBoxToPolyDataFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkInformation::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
