/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxToHierarchicalBoxFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxToHierarchicalBoxFilter.h"

#include "vtkHierarchicalBoxDataSet.h"

vtkCxxRevisionMacro(vtkHierarchicalBoxToHierarchicalBoxFilter, 
                    "1.2");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkHierarchicalBoxToHierarchicalBoxFilter::SetInput(
  vtkHierarchicalBoxDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkHierarchicalBoxDataSet *vtkHierarchicalBoxToHierarchicalBoxFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkHierarchicalBoxDataSet *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxToHierarchicalBoxFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
