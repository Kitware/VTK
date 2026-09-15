// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTableProbe.h"

#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include "Private/vtkTableToCoordinatesInternal.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkTableProbe);

namespace
{
constexpr vtkIdType INVALID_ID = -1;
}

//------------------------------------------------------------------------------
vtkTableProbe::~vtkTableProbe() = default;

//------------------------------------------------------------------------------
vtkTableProbe::vtkTableProbe()
{
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
void vtkTableProbe::SetSourceConnection(vtkAlgorithmOutput* input)
{
  this->SetInputConnection(1, input);
}

//------------------------------------------------------------------------------
int vtkTableProbe::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkTableProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Attribute to processe: " << this->AttributeToProcess << " ("
     << vtkDataObject::GetAttributeTypeAsString(this->AttributeToProcess) << ")" << "\n";

  os << indent << "CoordinatesArray:\n";
  this->CoordinatesArray->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
vtkMTimeType vtkTableProbe::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->CoordinatesArray->GetMTime());
}

//------------------------------------------------------------------------------
int vtkTableProbe::RequestData(
  vtkInformation*, vtkInformationVector** inputInfo, vtkInformationVector* outInfo)
{
  vtkDataObject* outputData = vtkDataObject::GetData(outInfo);
  vtkDataObject* inputData = vtkDataObject::GetData(inputInfo[0]);
  outputData->ShallowCopy(inputData);

  auto inputAttributes = inputData->GetAttributes(this->AttributeToProcess);
  auto outputAttributes = outputData->GetAttributes(this->AttributeToProcess);

  if (!inputAttributes || !outputAttributes)
  {
    vtkWarningMacro("No attributes "
      << vtkDataObject::GetAttributeTypeAsString(this->AttributeToProcess) << " in input "
      << inputData->GetClassName() << ". Nothing done.");
    return 1;
  }

  auto sourceTable = vtkTable::GetData(inputInfo[1]);
  auto sourceColumns = sourceTable->GetRowData();

  vtkNew<vtkDataSetAttributes> filteredSourceTableData;
  auto fieldToProbe = this->GetInputArrayToProcess(0, sourceTable);
  std::string fieldName = fieldToProbe->GetName();

  this->GetCoordinatesArray(sourceColumns, filteredSourceTableData, fieldName);

  vtkNew<vtkFieldData> tableSpace;
  vtkTableToCoordinatesInternal::ConstructSpace(tableSpace, filteredSourceTableData);

  vtkNew<vtkIdTypeArray> spaceToSourceMap;
  vtkTableToCoordinatesInternal::MapSpaceToInput(
    tableSpace, filteredSourceTableData, spaceToSourceMap, ::INVALID_ID);

  // fill output array
  vtkIdType inputNbOfElements = inputAttributes->GetNumberOfTuples();
  auto interpolatedArray = vtk::TakeSmartPointer(fieldToProbe->NewInstance());
  interpolatedArray->SetName(fieldName.c_str());
  interpolatedArray->SetNumberOfTuples(inputNbOfElements);
  for (vtkIdType elementId = 0; elementId < inputNbOfElements; elementId++)
  {
    std::vector<double> coords(filteredSourceTableData->GetNumberOfArrays());
    for (vtkIdType colIdx = 0; colIdx < filteredSourceTableData->GetNumberOfArrays(); colIdx++)
    {
      // here we assume that input arrays have same name in both input!!
      auto array =
        this->GetInputArrayFromSource(inputAttributes, filteredSourceTableData->GetArray(colIdx));
      if (!array)
      {
        vtkLog(INFO, << "cannot retrieve array "
                     << filteredSourceTableData->GetArray(colIdx)->GetName());
        continue;
      }
      coords[colIdx] = array->GetTuple1(elementId);
    }

    vtkNew<vtkIdTypeArray> pointIds;
    vtkNew<vtkDoubleArray> weights;
    vtkTableToCoordinatesInternal::GetSurroundingPoints(tableSpace, coords, pointIds, weights);

    double weightedSum = 0;
    double totalWeights = 0;
    for (vtkIdType surroundingPt = 0; surroundingPt < pointIds->GetNumberOfTuples();
         surroundingPt++)
    {
      auto spacePtId = pointIds->GetValue(surroundingPt);
      auto inputId = spaceToSourceMap->GetValue(spacePtId);
      auto fieldWeight = weights->GetValue(surroundingPt);
      // surrounding points can be missing from the table
      if (inputId == ::INVALID_ID)
      {
        // if they should have a nul contribution, silently skip them
        // otherwise adds a NaN
        if (fieldWeight == 0)
        {
          continue;
        }
        else
        {
          weightedSum = std::nan("");
          totalWeights = 1.;
          break;
        }
      }

      auto fieldValue = fieldToProbe->GetTuple1(inputId);
      weightedSum += (fieldValue * fieldWeight);
      totalWeights += fieldWeight;
    }

    double interpolatedValue = totalWeights != 0 ? weightedSum / totalWeights : std::nan("");
    interpolatedArray->SetTuple1(elementId, interpolatedValue);
  }

  outputAttributes->AddArray(interpolatedArray);

  return 1;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkTableProbe::GetInputArrayFromSource(
  vtkDataSetAttributes* inputAttributes, vtkDataArray* sourceArray)
{
  return inputAttributes->GetArray(sourceArray->GetName());
}

//------------------------------------------------------------------------------
void vtkTableProbe::GetCoordinatesArray(
  vtkDataSetAttributes* source, vtkDataSetAttributes* filtered, const std::string& excludedField)
{
  for (vtkIdType colIdx = 0; colIdx < source->GetNumberOfArrays(); colIdx++)
  {
    auto sourceArray = source->GetArray(colIdx);
    std::string arrayName = sourceArray->GetName();
    if (excludedField == arrayName)
    {
      continue;
    }

    if (this->CoordinatesArray->ArrayExists(arrayName.c_str()) &&
      this->CoordinatesArray->ArrayIsEnabled(arrayName.c_str()))
    {
      filtered->AddArray(sourceArray);
    }
  }
}

VTK_ABI_NAMESPACE_END
