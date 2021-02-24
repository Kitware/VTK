/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetCollectionToMultiBlockDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedDataSetCollectionToMultiBlockDataSet.h"

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyVisitor.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"

#include <set>
#include <utility>
#include <vector>

namespace
{
/**
 * Visitor to convert vtkDataAssembly to a vtkMultiBlockDataSet hierarchy
 * while traversing the assembly in depth-first order.
 *
 * Throws std::domain_error for cases where the data-assembly is richer than what can
 * be represented in a multiblock-dataset faithfully / easily.
 */
class vtkPDC2MBVisitor : public vtkDataAssemblyVisitor
{
public:
  static vtkPDC2MBVisitor* New();
  vtkTypeMacro(vtkPDC2MBVisitor, vtkDataAssemblyVisitor);

  void Visit(int nodeid) override
  {
    const auto dataset_indices = this->GetCurrentDataSetIndices();
    vtkLogF(TRACE, "visit %s, ds-count=%ld", this->GetCurrentNodeName(), dataset_indices.size());
    if (this->GetAssembly()->GetNumberOfChildren(nodeid) != 0)
    {
      if (!dataset_indices.empty())
      {
        // non-leaf node has dataset indices associated with it;
        // can't represent hierarchy in multiblock.
        throw std::domain_error("datasets associated with non-leaf nodes");
      }
      else
      {
        // non-leaf node, nothing to do.
        return;
      }
    }

    if (dataset_indices.size() > 1)
    {
      // multiple datasets (or partitioned datasets) associated with node.
      // can't represent hierarchy in multiblock.
      throw std::domain_error("multiple partitioned-datasets per node");
    }

    // track so we can skip traversing sub-tree for this node.
    this->Leaves.insert(nodeid);

    if (dataset_indices.size() == 1)
    {
      auto ptd = this->Input->GetPartitionedDataSet(dataset_indices[0]);
      if (ptd->GetNumberOfPartitions() == 1)
      {
        this->AddToBack(ptd->GetPartition(0), this->GetCurrentNodeName());
      }
      else
      {
        vtkNew<vtkMultiPieceDataSet> mp;
        mp->ShallowCopy(ptd);
        this->AddToBack(mp, this->GetCurrentNodeName());
      }
    }
    else
    {
      vtkNew<vtkMultiPieceDataSet> mp;
      this->AddToBack(mp, this->GetCurrentNodeName());
    }
  }

  bool GetTraverseSubtree(int nodeid) override
  {
    return (this->Leaves.find(nodeid) == this->Leaves.end());
  }

  void BeginSubTree(int nodeid) override
  {
    vtkLogF(TRACE, "begin %s", this->GetCurrentNodeName());
    if (nodeid == 0)
    {
      this->Stack.push_back(this->Root);
    }
    else
    {
      vtkNew<vtkMultiBlockDataSet> childMB;
      this->AddToBack(childMB, this->GetCurrentNodeName());
      this->Stack.push_back(childMB);
    }
  }

  void EndSubTree(int vtkNotUsed(nodeid)) override
  {
    this->Stack.pop_back();
    vtkLogF(TRACE, "end %s", this->GetCurrentNodeName());
  }

  void SetRoot(vtkMultiBlockDataSet* root) { this->Root = root; }
  void SetInput(vtkPartitionedDataSetCollection* input) { this->Input = input; }

protected:
  vtkPDC2MBVisitor() = default;
  ~vtkPDC2MBVisitor() override = default;

  void AddToBack(vtkDataObject* dobj, const char* name)
  {
    const auto idx = this->Stack.back()->GetNumberOfBlocks();
    this->Stack.back()->SetBlock(idx, dobj);
    this->Stack.back()->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), name);
  }

  std::vector<vtkSmartPointer<vtkMultiBlockDataSet>> Stack;
  vtkSmartPointer<vtkMultiBlockDataSet> Root;
  vtkSmartPointer<vtkPartitionedDataSetCollection> Input;
  std::set<int> Leaves;

private:
  vtkPDC2MBVisitor(const vtkPDC2MBVisitor&) = delete;
  void operator=(const vtkPDC2MBVisitor&) = delete;
};
vtkStandardNewMacro(vtkPDC2MBVisitor);
}

vtkObjectFactoryNewMacro(vtkPartitionedDataSetCollectionToMultiBlockDataSet);
//----------------------------------------------------------------------------
vtkPartitionedDataSetCollectionToMultiBlockDataSet::
  vtkPartitionedDataSetCollectionToMultiBlockDataSet() = default;

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollectionToMultiBlockDataSet::
  ~vtkPartitionedDataSetCollectionToMultiBlockDataSet() = default;

//------------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionToMultiBlockDataSet::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionToMultiBlockDataSet::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkPartitionedDataSetCollection::GetData(inputVector[0], 0);
  auto output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  return this->Execute(input, output) ? 1 : 0;
}

//----------------------------------------------------------------------------
bool vtkPartitionedDataSetCollectionToMultiBlockDataSet::Execute(
  vtkPartitionedDataSetCollection* input, vtkMultiBlockDataSet* output)
{
  if (auto assembly = input->GetDataAssembly())
  {
    vtkNew<::vtkPDC2MBVisitor> visitor;
    visitor->SetRoot(output);
    visitor->SetInput(input);
    try
    {
      assembly->Visit(visitor, vtkDataAssembly::TraversalOrder::DepthFirst);
      return true;
    }
    catch (std::domain_error& exception)
    {
      vtkWarningMacro("Cannot preserve data-assembly in multiblock: " << exception.what() << ".");
    }
  }

  // Just copy the hierarchical structure.
  output->SetNumberOfBlocks(input->GetNumberOfPartitionedDataSets());
  for (unsigned int cc = 0; cc < input->GetNumberOfPartitionedDataSets(); ++cc)
  {
    vtkNew<vtkMultiPieceDataSet> mp;
    mp->ShallowCopy(input->GetPartitionedDataSet(cc));
    output->SetBlock(cc, mp);
    if (input->HasMetaData(cc))
    {
      output->GetMetaData(cc)->Copy(input->GetMetaData(cc));
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionToMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
