/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonSort.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonSort.h"

#include "vtkObjectFactory.h"
#include "vtkPistonDataObject.h"

vtkStandardNewMacro(vtkPistonSort);

namespace vtkpiston
{
  // execution method found in vtkPistonSort.cu
  void ExecutePistonSort(vtkPistonDataObject *in, vtkPistonDataObject *out);
}

//------------------------------------------------------------------------------
void vtkPistonSort::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPistonSort::RequestData(vtkInformation *request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector)
{
  vtkPistonDataObject *id = vtkPistonDataObject::GetData(inputVector[0]);
  vtkPistonDataObject *od = vtkPistonDataObject::GetData(outputVector);
  this->PassBoundsForward(id,od);

  // Call the GPU implementation of the algorithm
  vtkpiston::ExecutePistonSort(id, od);

  return 1;
}
