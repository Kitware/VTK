/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractArray.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayData.h"
#include "vtkCommand.h"
#include "vtkExtractArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

///////////////////////////////////////////////////////////////////////////////
// vtkExtractArray

vtkStandardNewMacro(vtkExtractArray);

vtkExtractArray::vtkExtractArray() :
  Index(0)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkExtractArray::~vtkExtractArray()
{
}

void vtkExtractArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Index: " << this->Index << endl;
}

int vtkExtractArray::FillInputPortInformation(int port, vtkInformation* info)
{
  switch(port)
    {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
    }

  return 0;
}

int vtkExtractArray::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);

  if(this->Index < 0 || this->Index >= input->GetNumberOfArrays())
    {
    vtkErrorMacro(<< "Array index " << this->Index << " out-of-range for vtkArrayData containing " << input->GetNumberOfArrays() << " arrays.");
    return 0;
    }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(input->GetArray(this->Index));

  return 1;
}

