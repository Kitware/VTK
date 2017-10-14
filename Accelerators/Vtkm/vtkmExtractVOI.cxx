/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVOI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkmExtractVOI.h"

#include "vtkCellData.h"
#include "vtkExtractStructuredGridHelper.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/ImageDataConverter.h"
#include "vtkmlib/Storage.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include "vtkm/filter/ExtractStructured.h"


namespace {

struct InputFilterPolicy : public vtkmInputFilterPolicy
{
  using StructuredCellSetList =
    vtkm::ListTagBase<vtkm::cont::CellSetStructured<1>,
                      vtkm::cont::CellSetStructured<2>,
                      vtkm::cont::CellSetStructured<3>>;
};

}

vtkStandardNewMacro(vtkmExtractVOI)

//------------------------------------------------------------------------------
void vtkmExtractVOI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkmExtractVOI::vtkmExtractVOI() = default;
vtkmExtractVOI::~vtkmExtractVOI() = default;

//------------------------------------------------------------------------------
int vtkmExtractVOI::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData* input =
    vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* output =
    vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // convert the input dataset to a vtkm::cont::DataSet
  vtkm::cont::DataSet in = tovtkm::Convert(input,
                                           tovtkm::FieldsFlag::PointsAndCells);
  if (in.GetNumberOfCoordinateSystems() <= 0 || in.GetNumberOfCellSets() <= 0)
  {
    vtkErrorMacro(<< "Could not convert vtk dataset to vtkm dataset");
    return 0;
  }

  // transform VOI
  int inExtents[6], voi[6];
  input->GetExtent(inExtents);
  for (int i = 0; i < 6; i += 2)
  {
    voi[i] = this->VOI[i] - inExtents[i];
    voi[i + 1] = this->VOI[i + 1] - inExtents[i] + 1;
  }

  vtkm::filter::PolicyBase<InputFilterPolicy> policy;

  // apply the filter
  vtkm::filter::ExtractStructured filter;
  filter.SetVOI(voi[0], voi[1], voi[2], voi[3], voi[4], voi[5]);
  filter.SetSampleRate(this->SampleRate[0], this->SampleRate[1], this->SampleRate[2]);
  filter.SetIncludeBoundary( (this->IncludeBoundary != 0 ) );

  vtkm::filter::Result result = filter.Execute(in, policy);
  if (!result.IsDataSetValid())
  {
    vtkWarningMacro(<< "VTKm ExtractStructured algorithm failed to run."
                    << "Falling back to vtkExtractVOI.");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // Map fields:
  vtkm::Id numFields = static_cast<vtkm::Id>(in.GetNumberOfFields());
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields; ++fieldIdx)
  {
    const vtkm::cont::Field &field = in.GetField(fieldIdx);
    try
    {
      filter.MapFieldOntoOutput(result, field, policy);
    }
    catch (vtkm::cont::Error &e)
    {
      vtkWarningMacro(<< "Unable to use VTKm to convert field( "
                      << field.GetName() << " ) to the ExtractVOI"
                      << " output: " << e.what());
    }
  }

  // convert back to vtkImageData
  int outExtents[6];
  this->Internal->GetOutputWholeExtent(outExtents);
  if (!fromvtkm::Convert(result.GetDataSet(), outExtents, output, input))
  {
    vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
    return 0;
  }

  return 1;
}
