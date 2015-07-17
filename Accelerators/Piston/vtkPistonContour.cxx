/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonContour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonContour.h"

#include "vtkObjectFactory.h"
#include "vtkPistonDataObject.h"

vtkStandardNewMacro(vtkPistonContour);

namespace vtkpiston {
  // execution method found in vtkPistonContour.cu
  void ExecutePistonContour(vtkPistonDataObject *inData,
                            float isovalue,
                            vtkPistonDataObject *outData);
}

//----------------------------------------------------------------------------
vtkPistonContour::vtkPistonContour()
{
  VTK_LEGACY_BODY(vtkPistonContour::vtkPistonContour, "VTK 6.3");
  this->IsoValue = 0.0;
}

//----------------------------------------------------------------------------
vtkPistonContour::~vtkPistonContour()
{
}

//----------------------------------------------------------------------------
void vtkPistonContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IsoValue: " << this->IsoValue << endl;
}

//----------------------------------------------------------------------------
int vtkPistonContour::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{
  vtkPistonDataObject *id = vtkPistonDataObject::GetData(inputVector[0]);
  vtkPistonDataObject *od = vtkPistonDataObject::GetData(outputVector);
  this->PassBoundsForward(id,od);

  float isovalue = this->IsoValue;

  // Call the GPU implementation of the algorithm
  vtkpiston::ExecutePistonContour(id, isovalue, od);

  return 1;
}
