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
#include "vtkDaxMarchingCubes.h"

#include <vtkDispatcher.h>
#include <vtkDataSet.h>
#include <vtkUnstructuredGrid.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>

vtkStandardNewMacro(vtkDaxMarchingCubes)

namespace vtkDax {
  int MarchingCubes(vtkDataSet* input,
                    vtkUnstructuredGrid *output,
                    vtkDataArray* field,
                    float isoValue);
}


//------------------------------------------------------------------------------
vtkDaxMarchingCubes::vtkDaxMarchingCubes()
  {
  }

//------------------------------------------------------------------------------
vtkDaxMarchingCubes::~vtkDaxMarchingCubes()
  {
  }

//------------------------------------------------------------------------------
void vtkDaxMarchingCubes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//------------------------------------------------------------------------------
int vtkDaxMarchingCubes::RequestData(vtkInformation *request,
                             vtkInformationVector **inputVector,
                             vtkInformationVector *outputVector)
  {
  std::cout << ">>>>>> vtkDaxMarchingCubes::RequestData()" << std::endl;
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
                                  outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray *scalars = input->GetPointData()->GetScalars();
  int result = 0;
  if(scalars)
    {
    result = vtkDax::MarchingCubes(input,
                                   output,
                                   scalars,
                                   this->GetValue(0));
    }

  if(!result)
    {
    result = this->Superclass::RequestData(request,inputVector,outputVector);
    std::cout << " >>>>>>> vtkMarchingCubes::RequestData()" << std::endl;
    }
  return result;
}
