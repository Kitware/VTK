/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTwoInputFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageTwoInputFilter.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkImageTwoInputFilter::vtkImageTwoInputFilter()
{
  this->NumberOfRequiredInputs = 2;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
// Set the Input1 of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageTwoInputFilter::SetInput1(vtkImageData *input)
{
  this->vtkImageMultipleInputFilter::SetNthInput(0,input);
}

//----------------------------------------------------------------------------
// Set the Input2 of this filter. If a ScalarType has not been set,
// then the ScalarType of the input is used.
void vtkImageTwoInputFilter::SetInput2(vtkImageData *input)
{
  this->vtkImageMultipleInputFilter::SetNthInput(1,input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageTwoInputFilter::GetInput1()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  return static_cast<vtkImageData *>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageTwoInputFilter::GetInput2()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  return static_cast<vtkImageData *>(this->Inputs[1]);
}

//----------------------------------------------------------------------------
void vtkImageTwoInputFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
