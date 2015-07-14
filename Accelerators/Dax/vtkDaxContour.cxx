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
#include "vtkDaxContour.h"

#include "vtkDataSet.h"
#include "vtkDispatcher.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkDaxContour)

namespace vtkDax {
  int Contour(vtkDataSet* input,
              vtkPolyData *output,
              vtkDataArray* field,
              float isoValue,
              bool computeScalars);
}


//------------------------------------------------------------------------------
vtkDaxContour::vtkDaxContour()
  {
  VTK_LEGACY_BODY(vtkDaxContour::vtkDaxContour, "VTK 6.3");
  }

//------------------------------------------------------------------------------
vtkDaxContour::~vtkDaxContour()
  {
  }

//------------------------------------------------------------------------------
void vtkDaxContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
int vtkDaxContour::RequestData(vtkInformation *request,
                             vtkInformationVector **inputVector,
                             vtkInformationVector *outputVector)
  {
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData* output = vtkPolyData::SafeDownCast(
                                  outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataArray *scalars = this->GetInputArrayToProcess(0, inputVector);
  int result = 0;
  if(scalars)
    {
    if (this->GetNumberOfContours() == 1)
      {
      result = vtkDax::Contour(input,
                               output,
                               scalars,
                               this->GetValue(0),
                               this->GetComputeScalars());
      }
    else
      {
      vtkWarningMacro(
            << "Dax implementation currently only supports one contour.");
      }
    }

  if(!result)
    {
    vtkWarningMacro(<< "Could not use Dax to make contour. "
                    << "Falling back to serial implementation.");
    result = this->Superclass::RequestData(request,inputVector,outputVector);
    }
  return result;
}
