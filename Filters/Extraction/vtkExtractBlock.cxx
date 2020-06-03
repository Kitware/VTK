/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractBlock.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"

#include <cassert>
#include <set>

class vtkExtractBlock::vtkSet : public std::set<unsigned int>
{
};

vtkStandardNewMacro(vtkExtractBlock);
vtkInformationKeyMacro(vtkExtractBlock, DONT_PRUNE, Integer);
//------------------------------------------------------------------------------
vtkExtractBlock::vtkExtractBlock()
{
  this->Indices = new vtkExtractBlock::vtkSet();
  this->PruneOutput = 1;
  this->MaintainStructure = 0;
}

//------------------------------------------------------------------------------
vtkExtractBlock::~vtkExtractBlock()
{
  delete this->Indices;
}

//------------------------------------------------------------------------------
void vtkExtractBlock::AddIndex(unsigned int index)
{
  if (this->Indices->insert(index).second)
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkExtractBlock::RemoveIndex(unsigned int index)
{
  if (this->Indices->erase(index) > 0)
  {
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkExtractBlock::RemoveAllIndices()
{
  if (this->Indices->size() > 0)
  {
    this->Indices->clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkExtractBlock::CopySubTree(vtkDataObjectTreeIterator* loc, vtkDataObjectTree* output,
  vtkDataObjectTree* input, vtkSet& activeIndices)
{
  vtkDataObject* inputNode = input->GetDataSet(loc);
  if (auto cinput = vtkDataObjectTree::SafeDownCast(inputNode))
  {
    auto coutput = vtkDataObjectTree::SafeDownCast(output->GetDataSet(loc));
    auto iter = cinput->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto curNode = iter->GetCurrentDataObject();
      auto clone = curNode->NewInstance();
      clone->ShallowCopy(curNode);
      coutput->SetDataSet(iter, clone);
      clone->FastDelete();
      activeIndices.erase(loc->GetCurrentFlatIndex() + iter->GetCurrentFlatIndex());
    }
    iter->Delete();
  }
  else if (inputNode)
  {
    vtkDataObject* clone = inputNode->NewInstance();
    clone->ShallowCopy(inputNode);
    output->SetDataSet(loc, clone);
    clone->FastDelete();
  }
}

//------------------------------------------------------------------------------
int vtkExtractBlock::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataObjectTree::GetData(inputVector[0], 0);
  auto output = vtkDataObjectTree::GetData(outputVector, 0);
  if (this->Indices->find(0) != this->Indices->end())
  {
    // trivial case.
    output->ShallowCopy(input);
    return 1;
  }

  // pruning is unnecessary for vtkPartitionedDataSetCollection and hence we
  // skip it.
  const bool prune =
    vtkPartitionedDataSetCollection::SafeDownCast(input) ? false : (this->PruneOutput != 0);

  output->CopyStructure(input);

  vtkSet activeIndices(*this->Indices);

  // Copy selected blocks over to the output.
  vtkDataObjectTreeIterator* iter = input->NewTreeIterator();
  iter->VisitOnlyLeavesOff();
  iter->SkipEmptyNodesOff();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal() && !activeIndices.empty();
       iter->GoToNextItem())
  {
    if (activeIndices.find(iter->GetCurrentFlatIndex()) != activeIndices.end())
    {
      activeIndices.erase(iter->GetCurrentFlatIndex());
      this->CopySubTree(iter, output, input, activeIndices);

      if (prune)
      {
        // add a "hint" to the output to help identify visited nodes.
        output->GetMetaData(iter)->Set(DONT_PRUNE(), 1);
      }
    }
  }
  iter->Delete();
  if (prune)
  {
    this->Prune(output);
  }
  return 1;
}

//------------------------------------------------------------------------------
bool vtkExtractBlock::Prune(vtkDataObject* branch)
{
  if (auto mb = vtkMultiBlockDataSet::SafeDownCast(branch))
  {
    return this->Prune(mb);
  }
  else if (auto mpc = vtkPartitionedDataSetCollection::SafeDownCast(branch))
  {
    return this->Prune(mpc);
  }
  else if (auto mp = vtkPartitionedDataSet::SafeDownCast(branch))
  {
    return this->Prune(mp);
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkExtractBlock::Prune(vtkPartitionedDataSet* mpartition)
{
  // * Remove any children on mpartition that don't have DONT_PRUNE set.
  unsigned int oindex = 0;
  for (unsigned int iindex = 0, max = mpartition->GetNumberOfPartitions(); iindex < max; ++iindex)
  {
    auto iinfo = mpartition->HasMetaData(iindex) ? mpartition->GetMetaData(iindex) : nullptr;

    if (iinfo != nullptr && iinfo->Has(DONT_PRUNE()))
    {
      iinfo->Remove(DONT_PRUNE());
      if (oindex != iindex)
      {
        assert(oindex < iindex);
        mpartition->SetPartition(oindex, mpartition->GetPartition(iindex));
        mpartition->GetMetaData(oindex)->Copy(iinfo);
      }
      ++oindex;
    }
  }
  mpartition->SetNumberOfPartitions(oindex);

  // tell caller to prune mpartition away if num of pieces is 0.
  return (oindex == 0);
}

//------------------------------------------------------------------------------
bool vtkExtractBlock::Prune(vtkMultiBlockDataSet* mblock)
{
  unsigned int oindex = 0;
  for (unsigned int iindex = 0, max = mblock->GetNumberOfBlocks(); iindex < max; ++iindex)
  {
    auto block = mblock->GetBlock(iindex);
    auto iinfo = mblock->HasMetaData(iindex) ? mblock->GetMetaData(iindex) : nullptr;
    if ((iinfo != nullptr && iinfo->Has(DONT_PRUNE())) || !this->Prune(block))
    {
      if (iinfo)
      {
        iinfo->Remove(DONT_PRUNE());
      }
      if (oindex != iindex)
      {
        assert(oindex < iindex);
        mblock->SetBlock(oindex, block);
        mblock->GetMetaData(oindex)->Copy(iinfo);
      }
      ++oindex;
    }
  }
  mblock->SetNumberOfBlocks(oindex);
  if (oindex == 1 && !this->MaintainStructure)
  {
    // let's see if we can prune the tree.
    vtkSmartPointer<vtkMultiBlockDataSet> block0 =
      vtkMultiBlockDataSet::SafeDownCast(mblock->GetBlock(0));
    if (block0)
    {
      mblock->ShallowCopy(block0);
    }
  }
  return (oindex == 0);
}

//------------------------------------------------------------------------------
int vtkExtractBlock::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractBlock::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PruneOutput: " << this->PruneOutput << endl;
  os << indent << "MaintainStructure: " << this->MaintainStructure << endl;
}
