// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractBlockUsingDataAssembly.h"

#include "vtkAMRUtilities.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
class vtkScopedFieldDataCopier
{
  vtkFieldData* InputFD;
  vtkFieldData* OutputFD;

public:
  vtkScopedFieldDataCopier(vtkDataObject* input, vtkDataObject* output)
    : InputFD(input->GetFieldData())
    , OutputFD(output->GetFieldData())
  {
  }

  ~vtkScopedFieldDataCopier() { this->OutputFD->PassData(this->InputFD); }
};
}

class vtkExtractBlockUsingDataAssembly::vtkInternals
{
public:
  std::set<std::string> Selectors;

  bool Execute(vtkPartitionedDataSetCollection* input, vtkDataAssembly* assembly,
    const std::vector<vtkSmartPointer<vtkDataAssembly>>& assemblies_to_map,
    vtkPartitionedDataSetCollection* output,
    std::vector<vtkSmartPointer<vtkDataAssembly>>& mapped_assemblies,
    vtkExtractBlockUsingDataAssembly* self) const;
};

//------------------------------------------------------------------------------
bool vtkExtractBlockUsingDataAssembly::vtkInternals::Execute(vtkPartitionedDataSetCollection* input,
  vtkDataAssembly* assembly, const std::vector<vtkSmartPointer<vtkDataAssembly>>& assemblies_to_map,
  vtkPartitionedDataSetCollection* output,
  std::vector<vtkSmartPointer<vtkDataAssembly>>& mapped_assemblies,
  vtkExtractBlockUsingDataAssembly* self) const
{
  // get the collection of nodes to extract based on the user specified Selectors
  const std::vector<std::string> selectors(this->Selectors.begin(), this->Selectors.end());
  const auto selected_nodes = assembly->SelectNodes(selectors);

  // build the set of dataset (or partitioned dataset indices) to pass
  std::set<unsigned int> datasets_to_copy;
  for (auto nodeid : selected_nodes)
  {
    if (self->CheckAbort())
    {
      break;
    }
    const auto datasets = assembly->GetDataSetIndices(nodeid,
      /*traverse_subtree=*/self->GetSelectSubtrees());
    datasets_to_copy.insert(datasets.begin(), datasets.end());
  }

  // pass the chosen datasets and build mapping from old index to new.
  std::map<unsigned int, unsigned int> output_indices;
  for (const auto& in_idx : datasets_to_copy)
  {
    if (self->CheckAbort())
    {
      break;
    }
    const auto out_idx = output->GetNumberOfPartitionedDataSets();
    output->SetPartitionedDataSet(out_idx, input->GetPartitionedDataSet(in_idx));
    if (input->HasMetaData(in_idx))
    {
      output->GetMetaData(out_idx)->Copy(input->GetMetaData(in_idx));
    }
    output_indices[in_idx] = out_idx;
  }

  vtkNew<vtkDataAssembly> outAssembly;
  if (self->GetPruneDataAssembly())
  {
    outAssembly->SubsetCopy(assembly, selected_nodes);
  }
  else
  {
    outAssembly->DeepCopy(assembly);
  }
  outAssembly->RemapDataSetIndices(output_indices, /*remove_unmapped=*/true);
  output->SetDataAssembly(outAssembly);

  // now map each of the other input assemblies.
  for (auto& iAssembly : assemblies_to_map)
  {
    if (self->CheckAbort())
    {
      break;
    }
    vtkNew<vtkDataAssembly> oAssembly;
    oAssembly->DeepCopy(iAssembly);
    oAssembly->RemapDataSetIndices(output_indices, /*remove_unmapped=*/true);
    mapped_assemblies.emplace_back(oAssembly);
  }
  return true;
}

vtkStandardNewMacro(vtkExtractBlockUsingDataAssembly);
//------------------------------------------------------------------------------
vtkExtractBlockUsingDataAssembly::vtkExtractBlockUsingDataAssembly()
  : Internals(new vtkExtractBlockUsingDataAssembly::vtkInternals())
  , SelectSubtrees(true)
  , PruneDataAssembly(true)
  , AssemblyName(nullptr)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->SetAssemblyName(vtkDataAssemblyUtilities::HierarchyName());
}

//------------------------------------------------------------------------------
vtkExtractBlockUsingDataAssembly::~vtkExtractBlockUsingDataAssembly()
{
  delete this->Internals;
  delete[] this->AssemblyName;
}

//------------------------------------------------------------------------------
bool vtkExtractBlockUsingDataAssembly::AddSelector(const char* selector)
{
  if (selector)
  {
    auto& internals = *this->Internals;
    if (internals.Selectors.insert(selector).second)
    {
      this->Modified();
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkExtractBlockUsingDataAssembly::ClearSelectors()
{
  auto& internals = *this->Internals;
  if (!internals.Selectors.empty())
  {
    internals.Selectors.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkExtractBlockUsingDataAssembly::SetSelector(const char* selector)
{
  if (selector)
  {
    auto& internals = *this->Internals;
    if (internals.Selectors.size() == 1 && *internals.Selectors.begin() == selector)
    {
      return;
    }
    internals.Selectors.clear();
    internals.Selectors.insert(selector);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkExtractBlockUsingDataAssembly::GetNumberOfSelectors() const
{
  auto& internals = *this->Internals;
  return static_cast<int>(internals.Selectors.size());
}

//------------------------------------------------------------------------------
const char* vtkExtractBlockUsingDataAssembly::GetSelector(int index) const
{
  const auto& internals = *this->Internals;
  if (index >= 0 && index < static_cast<int>(internals.Selectors.size()))
  {
    auto iter = std::next(internals.Selectors.begin(), index);
    return iter->c_str();
  }

  vtkErrorMacro("Invalid index '" << index << "'.");
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkExtractBlockUsingDataAssembly::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractBlockUsingDataAssembly::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (!input)
  {
    vtkErrorMacro("Input is missing.");
    return 0;
  }

  auto output = vtkCompositeDataSet::GetData(outputVector, 0);
  if (vtkOverlappingAMR::SafeDownCast(input) != nullptr)
  {
    if (vtkPartitionedDataSetCollection::SafeDownCast(output) == nullptr)
    {
      // for vtkOverlappingAMR, we can't guarantee the output will be a valid
      // overlapping AMR and hence we create a vtkPartitionedDataSetCollection
      // instead.
      output = vtkPartitionedDataSetCollection::New();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
  }
  else if (output == nullptr || output->GetDataObjectType() != input->GetDataObjectType())
  {
    output = input->NewInstance();
    outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
    output->FastDelete();
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractBlockUsingDataAssembly::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  const auto& internals = (*this->Internals);

  if (this->AssemblyName == nullptr)
  {
    vtkErrorMacro("AssemblyName must be specified.");
    return 0;
  }

  auto inputCD = vtkCompositeDataSet::GetData(inputVector[0], 0);
  assert(inputCD != nullptr);

  auto outputCD = vtkCompositeDataSet::GetData(outputVector, 0);

  // Ensure field data from input is copied to output when this method returns.
  vtkScopedFieldDataCopier copier(inputCD, outputCD);

  auto assembly = vtkDataAssemblyUtilities::GetDataAssembly(this->AssemblyName, inputCD);
  if (!assembly)
  {
    vtkErrorMacro("Invalid assembly name '" << this->AssemblyName
                                            << "' for input "
                                               "of type "
                                            << inputCD->GetClassName());
    return 0;
  }

  const bool useHierarchy =
    (strcmp(this->AssemblyName, vtkDataAssemblyUtilities::HierarchyName()) == 0);

  if (useHierarchy)
  {
    vtkNew<vtkPartitionedDataSetCollection> xformedInput;
    vtkNew<vtkDataAssembly> tmpHierarchy;
    if (!vtkDataAssemblyUtilities::GenerateHierarchy(inputCD, tmpHierarchy, xformedInput))
    {
      vtkErrorMacro("Failed to generate hierarchy for input of type " << inputCD->GetClassName());
      return 0;
    }

    auto hierarchy = xformedInput->GetDataAssembly();
    assert(hierarchy != nullptr);

    if (auto outputPDC = vtkPartitionedDataSetCollection::GetData(outputVector, 0))
    {
      std::vector<vtkSmartPointer<vtkDataAssembly>> input_assemblies, output_assemblies;
      if (auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputCD))
      {
        if (inputPDC->GetDataAssembly() != nullptr)
        {
          // eventually, here we'll add all data assemblies defined on the input
          // so they can be mapped to the output.
          input_assemblies.emplace_back(inputPDC->GetDataAssembly());
        }
      }

      if (!internals.Execute(
            xformedInput, hierarchy, input_assemblies, outputPDC, output_assemblies, this))
      {
        return 0;
      }

      if (output_assemblies.size() == 1)
      {
        outputPDC->SetDataAssembly(output_assemblies[0]);
      }

      return 1;
    }
    else
    {
      vtkNew<vtkPartitionedDataSetCollection> xformedOutput;
      std::vector<vtkSmartPointer<vtkDataAssembly>> output_assemblies;
      if (!internals.Execute(xformedInput, hierarchy, {}, xformedOutput, output_assemblies, this))
      {
        return 0;
      }

      // convert xformedOutput to suitable output type.
      if (auto result = vtkDataAssemblyUtilities::GenerateCompositeDataSetFromHierarchy(
            xformedOutput, xformedOutput->GetDataAssembly()))
      {
        outputCD->CompositeShallowCopy(result);
        return 1;
      }

      return 0;
    }
  }
  else
  {
    auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputCD);
    if (!inputPDC)
    {
      vtkErrorMacro("Invalid assembly name: " << this->AssemblyName);
      return 0;
    }

    // todo: actually use the assembly name specified.
    auto inAssembly = inputPDC->GetDataAssembly();
    if (inAssembly == nullptr)
    {
      vtkErrorMacro("Invalid assembly name: " << this->AssemblyName);
      return 0;
    }

    auto outputPDC = vtkPartitionedDataSetCollection::GetData(outputVector, 0);
    assert(outputPDC != nullptr);

    std::vector<vtkSmartPointer<vtkDataAssembly>> output_assemblies;
    return internals.Execute(inputPDC, inAssembly, {}, outputPDC, output_assemblies, this) ? 1 : 0;
  }
}

//------------------------------------------------------------------------------
void vtkExtractBlockUsingDataAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AssemblyName: " << (this->AssemblyName ? this->AssemblyName : "(nullptr)")
     << endl;
  os << indent << "SelectSubtrees: " << this->SelectSubtrees << endl;
  os << indent << "PruneDataAssembly: " << this->PruneDataAssembly << endl;
  os << indent << "Selectors: " << endl;
  for (const auto& selector : this->Internals->Selectors)
  {
    os << indent.GetNextIndent() << selector << endl;
  }
}
VTK_ABI_NAMESPACE_END
