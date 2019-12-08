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
#include "vtkmLevelOfDetail.h"
#include "vtkmConfig.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/filter/VertexClustering.h>
// To handle computing custom coordinate sets bounds we need to include
// the following
#include <vtkm/cont/ArrayRangeCompute.hxx>

vtkStandardNewMacro(vtkmLevelOfDetail);

//------------------------------------------------------------------------------
vtkmLevelOfDetail::vtkmLevelOfDetail()
{
  this->NumberOfDivisions[0] = 512;
  this->NumberOfDivisions[1] = 512;
  this->NumberOfDivisions[2] = 512;
}

//------------------------------------------------------------------------------
vtkmLevelOfDetail::~vtkmLevelOfDetail() {}

//------------------------------------------------------------------------------
void vtkmLevelOfDetail::SetNumberOfXDivisions(int num)
{
  this->Modified();
  this->NumberOfDivisions[0] = num;
}

//------------------------------------------------------------------------------
void vtkmLevelOfDetail::SetNumberOfYDivisions(int num)
{
  this->Modified();
  this->NumberOfDivisions[1] = num;
}

//------------------------------------------------------------------------------
void vtkmLevelOfDetail::SetNumberOfZDivisions(int num)
{
  this->Modified();
  this->NumberOfDivisions[2] = num;
}

//------------------------------------------------------------------------------
int vtkmLevelOfDetail::GetNumberOfXDivisions()
{
  return this->NumberOfDivisions[0];
}

//------------------------------------------------------------------------------
int vtkmLevelOfDetail::GetNumberOfYDivisions()
{
  return this->NumberOfDivisions[1];
}

//------------------------------------------------------------------------------
int vtkmLevelOfDetail::GetNumberOfZDivisions()
{
  return this->NumberOfDivisions[2];
}

//------------------------------------------------------------------------------
void vtkmLevelOfDetail::SetNumberOfDivisions(int div0, int div1, int div2)
{
  this->Modified();
  this->NumberOfDivisions[0] = div0;
  this->NumberOfDivisions[1] = div1;
  this->NumberOfDivisions[2] = div2;
}

//------------------------------------------------------------------------------
const int* vtkmLevelOfDetail::GetNumberOfDivisions()
{
  return this->NumberOfDivisions;
}

//------------------------------------------------------------------------------
void vtkmLevelOfDetail::GetNumberOfDivisions(int div[3])
{
  div[0] = this->NumberOfDivisions[0];
  div[1] = this->NumberOfDivisions[1];
  div[2] = this->NumberOfDivisions[1];
}

//------------------------------------------------------------------------------
int vtkmLevelOfDetail::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!input || input->GetNumberOfPoints() == 0)
  {
    // empty output for empty inputs
    return 1;
  }

  try
  {
    // convert the input dataset to a vtkm::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);
    if (in.GetNumberOfCells() == 0 || in.GetNumberOfPoints() == 0)
    {
      return 0;
    }

    vtkmInputFilterPolicy policy;
    vtkm::filter::VertexClustering filter;
    filter.SetNumberOfDivisions(vtkm::make_Vec(
      this->NumberOfDivisions[0], this->NumberOfDivisions[1], this->NumberOfDivisions[2]));

    auto result = filter.Execute(in, policy);

    // convert back the dataset to VTK
    if (!fromvtkm::Convert(result, output, input))
    {
      vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
      return 0;
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkmLevelOfDetail::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of X Divisions: " << this->NumberOfDivisions[0] << "\n";
  os << indent << "Number of Y Divisions: " << this->NumberOfDivisions[1] << "\n";
  os << indent << "Number of Z Divisions: " << this->NumberOfDivisions[2] << "\n";
}
