// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkExplodeDataSet.h"

#include "vtkAffineArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtractCells.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
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
std::string vtkExplodeDataSet::GetPartitionName(vtkIdType partId,
  const std::map<double, std::string>& providedNames, double value, std::string defaultBasename)
{
  const auto nameIter = providedNames.find(value);
  if (nameIter != providedNames.end())
  {
    return nameIter->second;
  }
  return defaultBasename + "_" + vtk::to_string(partId);
}

//------------------------------------------------------------------------------
vtkStringArray* vtkExplodeDataSet::GetNamesArray(vtkDataSet* input)
{
  vtkFieldData* inputField = input->GetFieldData();
  if (!inputField->HasArray(this->PartitionNamesArray.c_str()))
  {
    vtkWarningMacro("No PartitionNamesArray found with name \""
      << this->PartitionNamesArray << "\". Will generate default names for output partitions.");
    return nullptr;
  }

  vtkStringArray* nameArray =
    vtkStringArray::SafeDownCast(inputField->GetAbstractArray(this->PartitionNamesArray.c_str()));
  if (!nameArray)
  {
    vtkWarningMacro("No vtkStringArray found with name \""
      << this->PartitionNamesArray
      << "\" to use as PartitionNamesArray. Will generate default names for output partitions.");
  }

  return nameArray;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkExplodeDataSet::GetValuesArray(
  vtkDataSet* input, vtkIdType expectedSize)
{
  vtkSmartPointer<vtkDataArray> valueArray;
  vtkFieldData* inputField = input->GetFieldData();
  valueArray = inputField->GetArray(this->PartitionValuesArray.c_str());
  if (!valueArray)
  {
    if (!this->PartitionValuesArray.empty())
    {
      vtkWarningMacro("No PartitionValuesArray found with name \""
        << this->PartitionValuesArray << "\". Will assume linear integral values for scalars.");
    }

    // assume a linear integral list of scalars values.
    vtkNew<vtkAffineArray<int>> linear;
    linear->SetName("vtkScalarValues");
    linear->SetNumberOfTuples(expectedSize);
    linear->ConstructBackend(1, 0);
    valueArray = linear;
  }

  return valueArray;
}

//------------------------------------------------------------------------------
std::map<double, std::string> vtkExplodeDataSet::GetProvidedNames(vtkDataSet* input)
{
  if (!this->UsePartitionNamesFromFieldData)
  {
    return std::map<double, std::string>();
  }
  vtkStringArray* nameArray = this->GetNamesArray(input);
  if (!nameArray)
  {
    return std::map<double, std::string>();
  }

  vtkSmartPointer<vtkDataArray> valueArray =
    this->GetValuesArray(input, nameArray->GetNumberOfValues());

  if (valueArray->GetNumberOfTuples() != nameArray->GetNumberOfTuples())
  {
    vtkLog(TRACE,
      "PartitionNamesArray has "
        << nameArray->GetNumberOfTuples() << " entries but PartitionValuesArray has "
        << valueArray->GetNumberOfTuples()
        << ". This may lead to generation of default names for some partitions.");
  }

  std::map<double, std::string> map;
  const vtkIdType minSize =
    std::min(nameArray->GetNumberOfTuples(), valueArray->GetNumberOfTuples());
  for (vtkIdType index = 0; index < minSize; index++)
  {
    map[valueArray->GetTuple1(index)] = nameArray->GetValue(index);
  }

  return map;
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

  const std::map<double, std::string> providedNames = this->GetProvidedNames(input);

  vtkIdType partId = 0;
  std::string arrayName = inScalars->GetName();
  for (const std::pair<const double, vtkSmartPointer<vtkIdList>>& valueIter : cellsPerValue)
  {
    const double partValue = valueIter.first;
    vtkIdList* partCellIds = valueIter.second;

    vtkSmartPointer<vtkPointSet> part =
      vtk::TakeSmartPointer(this->CreatePartition(input, partCellIds));
    output->SetPartition(partId, 0, part);

    std::string partName = this->GetPartitionName(partId, providedNames, partValue, arrayName);
    output->GetMetaData(partId)->Set(vtkCompositeDataSet::NAME(), partName.c_str());
    partId++;

    vtkSmartPointer<vtkDataArray> fieldScalar = vtk::TakeSmartPointer(inScalars->NewInstance());
    fieldScalar->SetName(arrayName.c_str());
    fieldScalar->SetNumberOfTuples(1);
    fieldScalar->SetTuple1(0, partValue);
    part->GetFieldData()->AddArray(fieldScalar);
  }

  vtkSmartPointer<vtkDataArray> fieldScalar = vtk::TakeSmartPointer(inScalars->NewInstance());
  fieldScalar->SetName(arrayName.c_str());
  output->GetFieldData()->AddArray(fieldScalar);

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
