/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonThreshold.h"

#include "vtkObjectFactory.h"
#include "vtkPistonDataObject.h"

vtkStandardNewMacro(vtkPistonThreshold);

namespace vtkpiston {
  // execution method found in vtkPistonThreshold.cu
  void ExecutePistonThreshold(vtkPistonDataObject *inData,
                                float minvalue, float maxvalue,
                                vtkPistonDataObject *outData);
}

//----------------------------------------------------------------------------
vtkPistonThreshold::vtkPistonThreshold()
{
  this->MinValue = 0.0;
  this->MaxValue = 0.0;
}

//----------------------------------------------------------------------------
vtkPistonThreshold::~vtkPistonThreshold()
{
}

//------------------------------------------------------------------------------
void vtkPistonThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MinValue: " << this->MinValue << endl;
  os << indent << "MaxValue: " << this->MaxValue << endl;
}

//------------------------------------------------------------------------------
int vtkPistonThreshold::RequestData(vtkInformation *request,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  vtkPistonDataObject *id = vtkPistonDataObject::GetData(inputVector[0]);
  vtkPistonDataObject *od = vtkPistonDataObject::GetData(outputVector);
  this->PassBoundsForward(id,od);

  float minvalue = this->MinValue;
  float maxvalue = this->MaxValue;

  // Call the GPU implementation of the algorithm
  vtkpiston::ExecutePistonThreshold(id, minvalue, maxvalue, od);

  return 1;
}
