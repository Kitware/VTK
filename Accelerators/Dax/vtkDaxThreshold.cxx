//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "vtkDaxThreshold.h"

#include "vtkDispatcher.h"
#include "vtkDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

namespace vtkDax {
  int Threshold(vtkDataSet* input,
                vtkUnstructuredGrid *output,
                vtkDataArray* field,
                double lower,
                double upper);
}

vtkStandardNewMacro(vtkDaxThreshold)

//------------------------------------------------------------------------------
vtkDaxThreshold::vtkDaxThreshold()
{
  VTK_LEGACY_BODY(vtkDaxContour::vtkDaxContour, "VTK 6.3");
}

//------------------------------------------------------------------------------
vtkDaxThreshold::~vtkDaxThreshold()
{
}

//------------------------------------------------------------------------------
void vtkDaxThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
int vtkDaxThreshold::RequestData(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));


  int result = vtkDax::Threshold(input,
                                 output,
                                 this->GetInputArrayToProcess(0,inputVector),
                                 this->GetLowerThreshold(),
                                 this->GetUpperThreshold());
  if(!result)
    {
    vtkWarningMacro(<< "Could not use Dax to make contour. "
                    << "Falling back to serial implementation.");
    result = this->Superclass::RequestData(request,inputVector,outputVector);
    }
  return result;
}
