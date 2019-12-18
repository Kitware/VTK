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

#include "vtkmFilterPolicy.h"

#include "vtkm/filter/ExtractStructured.h"
#include "vtkm/filter/ExtractStructured.hxx"

namespace
{

struct InputFilterPolicy : public vtkmInputFilterPolicy
{
  using StructuredCellSetList = vtkm::List<vtkm::cont::CellSetStructured<1>,
    vtkm::cont::CellSetStructured<2>, vtkm::cont::CellSetStructured<3> >;
};

}

vtkStandardNewMacro(vtkmExtractVOI);

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
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  try
  {
    // convert the input dataset to a vtkm::cont::DataSet
    auto in = tovtkm::Convert(input, tovtkm::FieldsFlag::PointsAndCells);

    // transform VOI
    int inExtents[6], voi[6];
    input->GetExtent(inExtents);
    for (int i = 0; i < 6; i += 2)
    {
      voi[i] = this->VOI[i] - inExtents[i];
      voi[i + 1] = this->VOI[i + 1] - inExtents[i] + 1;
    }

    // apply the filter
    vtkm::filter::PolicyBase<InputFilterPolicy> policy;
    vtkm::filter::ExtractStructured filter;
    filter.SetVOI(voi[0], voi[1], voi[2], voi[3], voi[4], voi[5]);
    filter.SetSampleRate(this->SampleRate[0], this->SampleRate[1], this->SampleRate[2]);
    filter.SetIncludeBoundary((this->IncludeBoundary != 0));
    auto result = filter.Execute(in, policy);

    // convert back to vtkImageData
    int outExtents[6];
    this->Internal->GetOutputWholeExtent(outExtents);
    if (!fromvtkm::Convert(result, outExtents, output, input))
    {
      vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
      return 0;
    }
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage() << "Falling back to vtkExtractVOI");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return 1;
}
