/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractBlockUsingDataAssembly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractBlockUsingDataAssembly.h"

#include "vtkAMRUtilities.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSetCollection.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <set>
#include <vector>

class vtkExtractBlockUsingDataAssembly::vtkInternals
{
public:
  std::set<std::string> Selectors;
};

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
    if (internals.Selectors.size() == 1 &&
      strcmp(internals.Selectors.begin()->c_str(), selector) == 0)
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
  if (this->AssemblyName == nullptr)
  {
    vtkErrorMacro("AssemblyName must be specified.");
    return 0;
  }

  auto inputCD = vtkCompositeDataSet::GetData(inputVector[0], 0);
  assert(inputCD != nullptr);

  if (strcmp(this->AssemblyName, vtkDataAssemblyUtilities::HierarchyName()) != 0)
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

    // Create output assembly.
    vtkNew<vtkDataAssembly> outAssembly;
    outputPDC->SetDataAssembly(outAssembly);
    return this->Execute(inputPDC, inAssembly, outputPDC, outAssembly) ? 1 : 0;
  }

  // Generate hierarchy for input dataset.
  vtkNew<vtkDataAssembly> hierarchy;
  vtkNew<vtkPartitionedDataSetCollection> xInput;
  vtkDataAssemblyUtilities::GenerateHierarchy(inputCD, hierarchy, xInput);
  if (auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputCD))
  {
    // shortcut for vtkPartitionedDataSetCollection.
    auto outputPDC = vtkPartitionedDataSetCollection::GetData(outputVector, 0);
    assert(outputPDC != nullptr);

    // if input has an assembly, we preserve (and remap) it.
    if (auto inAssembly = inputPDC->GetDataAssembly())
    {
      vtkNew<vtkDataAssembly> outAssembly;
      outAssembly->DeepCopy(inAssembly);
      outputPDC->SetDataAssembly(outAssembly);
    }

    return this->Execute(inputPDC, hierarchy, outputPDC, nullptr) ? 1 : 0;
  }
  else if (auto oamr = vtkOverlappingAMR::SafeDownCast(inputCD))
  {
    // since blanking information is no longer valid once blocks are extracted,
    // remove it.
    vtkNew<vtkOverlappingAMR> stripped;
    vtkAMRUtilities::StripGhostLayers(oamr, stripped);

    auto outputPDC = vtkPartitionedDataSetCollection::GetData(outputVector, 0);
    assert(outputPDC != nullptr);

    // Create a placeholder for the output as `vtkPartitionedDataSetCollection`.
    vtkNew<vtkDataAssembly> outHierarchy;
    outputPDC->SetDataAssembly(outHierarchy);
    return this->Execute(xInput, hierarchy, outputPDC, outHierarchy);
  }
  else
  {
    auto outputCD = vtkCompositeDataSet::GetData(outputVector, 0);
    assert(outputCD != nullptr);

    // Create a placeholder for the output as `vtkPartitionedDataSetCollection`.
    vtkNew<vtkPartitionedDataSetCollection> xOutput;
    vtkNew<vtkDataAssembly> outHierarchy;
    xOutput->SetDataAssembly(outHierarchy);
    if (!this->Execute(xInput, hierarchy, xOutput, outHierarchy))
    {
      return 0;
    }

    // now, convert xOutput back to a appropriate type
    if (auto result =
          vtkDataAssemblyUtilities::GenerateCompositeDataSetFromHierarchy(xOutput, outHierarchy))
    {
      outputCD->ShallowCopy(result);
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
bool vtkExtractBlockUsingDataAssembly::Execute(vtkPartitionedDataSetCollection* input,
  vtkDataAssembly* inAssembly, vtkPartitionedDataSetCollection* output,
  vtkDataAssembly* outAssembly) const
{
  const auto& internals = (*this->Internals);

  // get the collection of nodes to extract based on the user specified Selectors
  const std::vector<std::string> selectors(internals.Selectors.begin(), internals.Selectors.end());
  const auto selected_nodes = inAssembly->SelectNodes(selectors);

  // build the set of dataset (or partitioned dataset indices) to pass
  std::set<unsigned int> datasets_to_copy;
  for (auto nodeid : selected_nodes)
  {
    const auto datasets = inAssembly->GetDataSetIndices(nodeid,
      /*traverse_subtree=*/this->SelectSubtrees);
    datasets_to_copy.insert(datasets.begin(), datasets.end());
  }

  // pass the chosen datasets and build mapping from old index to new.
  std::map<unsigned int, unsigned int> output_indices;
  for (const auto& in_idx : datasets_to_copy)
  {
    const auto out_idx = output->GetNumberOfPartitionedDataSets();
    output->SetPartitionedDataSet(out_idx, input->GetPartitionedDataSet(in_idx));
    if (input->HasMetaData(in_idx))
    {
      output->GetMetaData(out_idx)->Copy(input->GetMetaData(in_idx));
    }
    output_indices[in_idx] = out_idx;
  }

  // copy/update data assembly in the output.
  if (outAssembly)
  {
    if (this->PruneDataAssembly)
    {
      outAssembly->SubsetCopy(inAssembly, selected_nodes);
    }
    else
    {
      outAssembly->DeepCopy(inAssembly);
    }
  }

  if (auto assembly = output->GetDataAssembly())
  {
    // remap dataset indices since we changed dataset indices in the output
    // and remove all unused dataset indices.
    assembly->RemapDataSetIndices(output_indices, /*remove_unmapped=*/true);
  }

  return true;
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
    os << indent.GetNextIndent() << selector.c_str() << endl;
  }
}
