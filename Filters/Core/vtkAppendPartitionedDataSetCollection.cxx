// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendPartitionedDataSetCollection.h"

#include "vtkAbstractArray.h"
#include "vtkAppendFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObjectTypes.h"
#include "vtkFieldData.h"
#include "vtkImageAppend.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStructuredGridAppend.h"

#include <string>
#include <unordered_set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAppendPartitionedDataSetCollection);

//----------------------------------------------------------------------------
vtkAppendPartitionedDataSetCollection::vtkAppendPartitionedDataSetCollection() = default;

//----------------------------------------------------------------------------
vtkAppendPartitionedDataSetCollection::~vtkAppendPartitionedDataSetCollection() = default;

//----------------------------------------------------------------------------
void vtkAppendPartitionedDataSetCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AppendMode: " << this->AppendMode << endl;
  os << indent << "AppendFieldData: " << this->AppendFieldData << endl;
}

//----------------------------------------------------------------------------
int vtkAppendPartitionedDataSetCollection::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendPartitionedDataSetCollection::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  const int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs <= 0)
  {
    // Fail silently when there are no inputs.
    return 1;
  }
  // get the output info object
  auto input0 = vtkPartitionedDataSetCollection::GetData(inputVector[0], 0);
  auto output = vtkPartitionedDataSetCollection::GetData(outputVector, 0);
  if (numInputs == 1)
  {
    // trivial case.
    output->CompositeShallowCopy(input0);
    return 1;
  }

  const unsigned int numPartitions = input0->GetNumberOfPartitionedDataSets();
  vtkDataAssembly* dataAssembly0 = input0->GetDataAssembly();
  const vtkNew<vtkDataAssembly> hierarchy0;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(input0, hierarchy0))
  {
    vtkErrorMacro("Failed to generate hierarchy.");
    return 0;
  }

  // perform some checks
  for (int i = 1; i < numInputs; ++i)
  {
    auto inputI = vtkPartitionedDataSetCollection::GetData(inputVector[0], i);
    // check if the number of partitions match
    if (inputI->GetNumberOfPartitionedDataSets() != numPartitions)
    {
      vtkErrorMacro("Number of partitions in input " << i << " does not match the first input.");
      return 0;
    }
    // check if the composite name matches
    for (unsigned int j = 0; j < numPartitions; ++j)
    {
      if (input0->HasMetaData(j) != inputI->HasMetaData(j))
      {
        vtkErrorMacro("Partition " << j << " in input " << i << " or 0 doesn't have meta-data.");
        return 0;
      }
      else if (input0->HasMetaData(j) && inputI->HasMetaData(j))
      {
        if (input0->GetMetaData(j)->Has(vtkCompositeDataSet::NAME()) !=
          inputI->GetMetaData(j)->Has(vtkCompositeDataSet::NAME()))
        {
          vtkErrorMacro("Partition " << j << " in input " << i << " or 0 doesn't have a name.");
          return 0;
        }
        else if (input0->GetMetaData(j)->Has(vtkCompositeDataSet::NAME()) &&
          inputI->GetMetaData(j)->Has(vtkCompositeDataSet::NAME()))
        {
          const char* name0 = input0->GetMetaData(j)->Get(vtkCompositeDataSet::NAME());
          const char* nameI = inputI->GetMetaData(j)->Get(vtkCompositeDataSet::NAME());
          if (name0 && nameI && strcmp(name0, nameI) != 0)
          {
            vtkErrorMacro("Partition " << j << " in input " << i << " has a different name.");
            return 0;
          }
        }
      }
    }

    vtkNew<vtkDataAssembly> hierarchyI;
    if (!vtkDataAssemblyUtilities::GenerateHierarchy(inputI, hierarchyI))
    {
      vtkErrorMacro("Failed to generate hierarchy for input " << i << ".");
      return 0;
    }
    auto dataAssemblyI = inputI->GetDataAssembly();

    // check that the hierarchy paths correspond to the same data assembly paths for each PDC
    if (dataAssembly0 && dataAssemblyI)
    {
      for (unsigned int j = 0; j < numPartitions; ++j)
      {
        const char* name0 =
          input0->HasMetaData(j) && input0->GetMetaData(j)->Has(vtkCompositeDataSet::NAME())
          ? input0->GetMetaData(j)->Get(vtkCompositeDataSet::NAME())
          : nullptr;
        const char* nameI =
          inputI->HasMetaData(j) && inputI->GetMetaData(j)->Has(vtkCompositeDataSet::NAME())
          ? inputI->GetMetaData(j)->Get(vtkCompositeDataSet::NAME())
          : nullptr;
        if (name0 && nameI)
        {
          const std::string path0 = "/Root/" + std::string(name0);
          const std::string pathI = "/Root/" + std::string(nameI);

          auto cidsHierarchy0 =
            vtkDataAssemblyUtilities::GetSelectedCompositeIds({ path0 }, hierarchy0, input0);
          auto cidsHierarchyI =
            vtkDataAssemblyUtilities::GetSelectedCompositeIds({ pathI }, hierarchyI, inputI);
          auto selectorsDataAssembly0 = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
            cidsHierarchy0, hierarchy0, dataAssembly0);
          auto selectorsDataAssemblyI = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
            cidsHierarchyI, hierarchyI, dataAssemblyI);
          // check if the selectors match
          if (selectorsDataAssembly0 != selectorsDataAssemblyI)
          {
            vtkErrorMacro("Selectors for partition " << j << " in input " << i << " do not match.");
            return 0;
          }
        }
      }
    }
  }
  // copy the structure of the first input
  output->CopyStructure(input0);

  // append the partitioned datasets
  for (unsigned int j = 0; j < numPartitions; ++j)
  {
    auto outputPDS = output->GetPartitionedDataSet(j);
    // initialize the output partitioned dataset
    outputPDS->SetNumberOfPartitions(0);
    if (this->AppendMode == AppendModes::APPEND_PARTITIONS)
    {
      for (int i = 0; i < numInputs; ++i)
      {
        auto inputI = vtkPartitionedDataSetCollection::GetData(inputVector[0], i);
        auto inputPDS = inputI->GetPartitionedDataSet(j);
        for (unsigned int k = 0; k < inputPDS->GetNumberOfPartitions(); ++k)
        {
          outputPDS->SetPartition(
            outputPDS->GetNumberOfPartitions(), inputPDS->GetPartitionAsDataObject(k));
        }
      }
    }
    else // if (this->AppendMode == AppendModes::MERGE_PARTITIONS)
    {
      // append the field data
      auto appendFieldData = [](std::vector<vtkDataObject*> inputDOs,
                               vtkDataObject* outputDO) -> void
      {
        auto outputFD = outputDO->GetFieldData();
        for (auto inputDO : inputDOs)
        {
          auto inputFD = inputDO->GetFieldData();
          const int numArrays = inputFD->GetNumberOfArrays();
          for (int a = 0; a < numArrays; ++a)
          {
            vtkAbstractArray* arr = inputFD->GetAbstractArray(a);
            if (!outputFD->HasArray(arr->GetName()))
            {
              outputFD->AddArray(arr);
            }
          }
        }
      };

      // collect the leaves to append
      std::vector<vtkDataObject*> leavesToAppend;
      for (int i = 0; i < numInputs; ++i)
      {
        auto inputI = vtkPartitionedDataSetCollection::GetData(inputVector[0], i);
        auto inputPDS = inputI->GetPartitionedDataSet(j);
        for (unsigned int k = 0; k < inputPDS->GetNumberOfPartitions(); ++k)
        {
          if (inputPDS->GetPartitionAsDataObject(k))
          {
            leavesToAppend.push_back(inputPDS->GetPartitionAsDataObject(k));
          }
        }
      }
      // collect the data object types
      std::unordered_set<int> dataObjectTypes;
      for (auto leaf : leavesToAppend)
      {
        dataObjectTypes.insert(leaf->GetDataObjectType());
      }
      // append the leaves
      if (dataObjectTypes.size() == 1)
      {
        const int dataType = *dataObjectTypes.begin();
        vtkSmartPointer<vtkAlgorithm> appender;
        bool appendSupported;
        switch (dataType)
        {
          case VTK_UNSTRUCTURED_GRID:
            appender = vtkSmartPointer<vtkAppendFilter>::New();
            appendSupported = true;
            break;
          case VTK_POLY_DATA:
            appender = vtkSmartPointer<vtkAppendPolyData>::New();
            appendSupported = true;
            break;
          case VTK_IMAGE_DATA:
          case VTK_STRUCTURED_POINTS:
          case VTK_UNIFORM_GRID:
            appender = vtkSmartPointer<vtkImageAppend>::New();
            appendSupported = true;
            break;
          case VTK_STRUCTURED_GRID:
            appender = vtkSmartPointer<vtkStructuredGridAppend>::New();
            appendSupported = true;
            break;
          default:
            appendSupported = false;
            break;
        }
        if (appendSupported)
        {
          appender->SetContainerAlgorithm(this);
          for (auto leaf : leavesToAppend)
          {
            appender->AddInputDataObject(leaf);
          }
          appender->Update();
          auto outputDO = appender->GetOutputDataObject(0);
          if (this->AppendFieldData)
          {
            appendFieldData(leavesToAppend, outputDO);
          }
          outputPDS->SetPartition(outputPDS->GetNumberOfPartitions(), outputDO);
        }
        else
        {
          if (leavesToAppend.size() > 1)
          {
            vtkWarningMacro("Data type "
              << vtkDataObjectTypes::GetClassNameFromTypeId(dataType)
              << " can't be appended. Only the first leaf will be kept.");
          }
          outputPDS->SetPartition(outputPDS->GetNumberOfPartitions(), leavesToAppend[0]);
          break;
        }
      }
      else if (dataObjectTypes.size() > 1)
      {
        vtkWarningMacro(
          "Multiple data types found in the leaves. Only the first leaf will be kept.");
        outputPDS->SetPartition(outputPDS->GetNumberOfPartitions(), leavesToAppend[0]);
      }
    }
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
