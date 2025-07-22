// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkmHistogramSampling.h"

#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/DataSetUtils.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include <numeric>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/resampling/HistSampling.h>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkmHistogramSampling);

//------------------------------------------------------------------------------
int vtkmHistogramSampling::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkUnstructuredGrid* inUG = vtkUnstructuredGrid::GetData(inputVector[0]);

  if (inUG == nullptr) // must be any other type derived from vtkDataSet
  {
    vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkUnstructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

//------------------------------------------------------------------------------
int vtkmHistogramSampling::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfPoints() < 1)
  {
    vtkWarningMacro(<< "No input points");
    return 1;
  }

  // convert the input dataset to a viskores::cont::DataSet
  // Grab the input array to process to determine the field we want to apply histogramsampling
  int association = this->GetInputArrayAssociation(0, inputVector);
  auto fieldArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != vtkDataObject::FIELD_ASSOCIATION_POINTS || fieldArray == nullptr)
  {
    vtkErrorMacro(<< "Invalid field: Requires a point field with a valid name.");
    return 0;
  }

  const char* fieldName = fieldArray->GetName();

  try
  {
    viskores::cont::DataSet in = tovtkm::Convert(input);
    auto field = tovtkm::Convert(fieldArray, association);
    in.AddField(field);

    viskores::filter::resampling::HistSampling filter;
    filter.SetNumberOfBins(this->NumberOfBins);
    filter.SetSampleFraction(this->SampleFraction);
    filter.SetActiveField(fieldName, field.GetAssociation());
    auto result = filter.Execute(in);

    vtkPoints* newPts = fromvtkm::Convert(result.GetCoordinateSystem());
    output->SetPoints(newPts);
    newPts->Delete();
    vtkDataArray* histResult =
      fromvtkm::Convert(result.GetField(fieldName, viskores::cont::Field::Association::Points));
    output->GetPointData()->AddArray(histResult);
    vtkIdList* id_list = vtkIdList::New();
    unsigned int N = newPts->GetNumberOfPoints();
    id_list->SetNumberOfIds(N);
    std::iota(id_list->GetPointer(0), id_list->GetPointer(N), 0);
    output->Allocate(1);
    output->InsertNextCell(VTK_POLY_VERTEX, id_list);
    id_list->Delete();
    histResult->Delete();
  }
  catch (const viskores::cont::Error& e)
  {
    vtkErrorMacro(<< "Viskores error: " << e.GetMessage());
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkmHistogramSampling::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
