// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkExplodeDataSet.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtractCells.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExplodeDataSet);

//------------------------------------------------------------------------------
vtkExplodeDataSet::vtkExplodeDataSet()
{
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
std::map<double, vtkSmartPointer<vtkIdList>> vtkExplodeDataSet::GetIdListsByValue(
  vtkDataArray* scalars)
{
  std::map<double, vtkSmartPointer<vtkIdList>> idsByValue;
  auto arrayRange = vtk::DataArrayValueRange(scalars);
  vtkIdType cellId = 0;
  for (auto value : arrayRange)
  {
    if (idsByValue.count(value) == 0)
    {
      idsByValue[value] = vtk::TakeSmartPointer(vtkIdList::New());
    }

    idsByValue[value]->InsertNextId(cellId);
    cellId++;
  }

  return idsByValue;
}

//------------------------------------------------------------------------------
vtkPointSet* vtkExplodeDataSet::CreatePartition(vtkDataSet* input, vtkIdList* partCellIds)
{
  vtkPolyData* inputPD = vtkPolyData::SafeDownCast(input);
  if (inputPD)
  {
    vtkPolyData* poly = vtkPolyData::New();
    poly->AllocateCopy(inputPD);
    // use input nb of points as a max. Squeeze it later.
    poly->GetPointData()->CopyAllocate(input->GetPointData(), input->GetNumberOfPoints());
    poly->GetCellData()->CopyAllocate(input->GetCellData(), partCellIds->GetNumberOfIds());
    poly->CopyCells(inputPD, partCellIds);
    poly->Squeeze();
    return poly;
  }

  vtkNew<vtkExtractCells> extraction;
  extraction->SetInputData(input);
  extraction->SetCellList(partCellIds);
  extraction->Update();
  vtkUnstructuredGrid* ugrid = extraction->GetOutput();
  ugrid->Register(nullptr);
  return ugrid;
}

//------------------------------------------------------------------------------
int vtkExplodeDataSet::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0], 0);
  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::GetData(outputVector, 0);

  if (!input)
  {
    vtkErrorMacro(<< "Input is not a vtkDataSet. Aborting.");
  }

  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inScalars)
  {
    vtkErrorMacro(<< "No scalar data to process.");
    return 0;
  }

  std::map<double, vtkSmartPointer<vtkIdList>> cellsPerValue = this->GetIdListsByValue(inScalars);
  output->SetNumberOfPartitionedDataSets(static_cast<unsigned int>(cellsPerValue.size()));

  vtkIdType partId = 0;
  std::string arrayName = inScalars->GetName();
  for (const std::pair<const double, vtkSmartPointer<vtkIdList>>& valueIter : cellsPerValue)
  {
    const double partValue = valueIter.first;
    vtkIdList* partCellIds = valueIter.second;

    vtkSmartPointer<vtkPointSet> part =
      vtk::TakeSmartPointer(this->CreatePartition(input, partCellIds));
    output->SetPartition(partId, 0, part);
    vtkNumberToString converter;
    std::string partName = arrayName + "_" + converter.Convert(partValue);
    output->GetMetaData(partId)->Set(vtkCompositeDataSet::NAME(), partName.c_str());
    partId++;

    vtkSmartPointer<vtkDataArray> fieldScalar = vtk::TakeSmartPointer(inScalars->NewInstance());
    fieldScalar->SetName(arrayName.c_str());
    fieldScalar->SetNumberOfTuples(1);
    fieldScalar->SetTuple1(0, partValue);
    part->GetFieldData()->AddArray(fieldScalar);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkExplodeDataSet::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkExplodeDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
