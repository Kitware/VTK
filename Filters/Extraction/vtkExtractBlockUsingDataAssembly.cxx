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

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

class vtkExtractBlockUsingDataAssembly::vtkInternals
{
public:
  std::set<std::string> Paths;
};

vtkStandardNewMacro(vtkExtractBlockUsingDataAssembly);
//------------------------------------------------------------------------------
vtkExtractBlockUsingDataAssembly::vtkExtractBlockUsingDataAssembly()
  : Internals(new vtkExtractBlockUsingDataAssembly::vtkInternals())
  , SelectSubtrees(true)
  , PruneDataAssembly(true)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkExtractBlockUsingDataAssembly::~vtkExtractBlockUsingDataAssembly()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
bool vtkExtractBlockUsingDataAssembly::AddNodePath(const char* path)
{
  if (path)
  {
    auto& internals = *this->Internals;
    if (internals.Paths.insert(path).second)
    {
      this->Modified();
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkExtractBlockUsingDataAssembly::ClearNodePaths()
{
  auto& internals = *this->Internals;
  if (internals.Paths.size() > 0)
  {
    internals.Paths.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkExtractBlockUsingDataAssembly::SetNodePath(const char* path)
{
  if (path)
  {
    auto& internals = *this->Internals;
    if (internals.Paths.size() == 1 && strcmp(internals.Paths.begin()->c_str(), path) == 0)
    {
      return;
    }
    internals.Paths.clear();
    internals.Paths.insert(path);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkExtractBlockUsingDataAssembly::GetNumberOfPaths() const
{
  auto& internals = *this->Internals;
  return static_cast<int>(internals.Paths.size());
}

//------------------------------------------------------------------------------
const char* vtkExtractBlockUsingDataAssembly::GetNodePath(int index) const
{
  const auto& internals = *this->Internals;
  if (index >= 0 && index < static_cast<int>(internals.Paths.size()))
  {
    auto iter = std::next(internals.Paths.begin(), index);
    return iter->c_str();
  }

  vtkErrorMacro("Invalid index '" << index << "'.");
  return nullptr;
}

//------------------------------------------------------------------------------
int vtkExtractBlockUsingDataAssembly::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractBlockUsingDataAssembly::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractBlockUsingDataAssembly::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkPartitionedDataSetCollection::GetData(inputVector[0], 0);
  auto output = vtkPartitionedDataSetCollection::GetData(outputVector, 0);
  const auto inAssembly = input->GetDataAssembly();
  if (inAssembly == nullptr)
  {
    vtkWarningMacro("Input does have a DataAssembly. No output will be generated.");
    return 1;
  }

  vtkNew<vtkDataAssembly> outAssembly;

  const auto& internals = (*this->Internals);
  std::vector<std::string> paths(internals.Paths.begin(), internals.Paths.end());
  // get the collection of nodes to extract based on the user specified paths
  auto selected_nodes = inAssembly->SelectNodes(paths);

  if (selected_nodes.empty())
  {
    outAssembly->Initialize();
    outAssembly->SetRootNodeName(inAssembly->GetRootNodeName());
    output->SetDataAssembly(outAssembly);
    return 1;
  }

  // build the set of dataset (or partitioned dataset indices) to pass
  std::set<unsigned int> datasets_to_copy;
  for (auto nodeid : selected_nodes)
  {
    const auto datasets = inAssembly->GetDataSetIndices(nodeid,
      /*traverse_subtree=*/this->SelectSubtrees);
    std::copy(
      datasets.begin(), datasets.end(), std::inserter(datasets_to_copy, datasets_to_copy.end()));
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
  if (this->PruneDataAssembly)
  {
    outAssembly->SubsetCopy(inAssembly, selected_nodes);
  }
  else
  {
    outAssembly->DeepCopy(inAssembly);
  }

  // remap dataset indices since we changed dataset indices in the output
  // and remove all unnused dataset indices.
  outAssembly->RemapDataSetIndices(output_indices, /*remove_unmapped=*/true);
  output->SetDataAssembly(outAssembly);
  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractBlockUsingDataAssembly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SelectSubtrees: " << this->SelectSubtrees << endl;
  os << indent << "PruneDataAssembly: " << this->PruneDataAssembly << endl;
  os << indent << "Paths: " << endl;
  for (const auto& path : this->Internals->Paths)
  {
    os << indent.GetNextIndent() << path.c_str() << endl;
  }
}
