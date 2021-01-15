/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToPartitionedDataSetCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectToPartitionedDataSetCollection.h"

#include "vtkDataAssembly.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <cassert>
#include <functional>
#include <vector>

vtkStandardNewMacro(vtkDataObjectToPartitionedDataSetCollection);
//----------------------------------------------------------------------------
vtkDataObjectToPartitionedDataSetCollection::vtkDataObjectToPartitionedDataSetCollection() {}

//----------------------------------------------------------------------------
vtkDataObjectToPartitionedDataSetCollection::~vtkDataObjectToPartitionedDataSetCollection() {}

//----------------------------------------------------------------------------
int vtkDataObjectToPartitionedDataSetCollection::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkDataObjectToPartitionedDataSetCollection::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto output = vtkPartitionedDataSetCollection::GetData(outputVector, 0);
  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    // nothing to do, input is already a vtkPartitionedDataSetCollection.
    output->ShallowCopy(pdc);
    return 1;
  }
  if (auto pd = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    output->SetPartitionedDataSet(0, pd);
    return 1;
  }
  if (vtkCompositeDataSet::SafeDownCast(inputDO) == nullptr)
  {
    output->SetPartition(0, 0, inputDO);
    return 1;
  }

  if (auto amr = vtkUniformGridAMR::SafeDownCast(inputDO))
  {
    return this->ConvertFromAMR(amr, output);
  }
  else if (auto mb = vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    return this->ConvertFromMultiBlock(mb, output);
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkDataObjectToPartitionedDataSetCollection::ConvertFromMultiBlock(
  vtkMultiBlockDataSet* input, vtkPartitionedDataSetCollection* output)
{
  assert(input && output);
  vtkNew<vtkDataAssembly> assembly;
  assembly->Initialize();
  assembly->SetRootNodeName("Multiblock");
  output->SetDataAssembly(assembly);

  std::function<bool(vtkDataObject*, vtkInformation*, int)> f;
  f = [&f, output, &assembly](vtkDataObject* block, vtkInformation* metadata, int id) {
    if (auto mb = vtkMultiBlockDataSet::SafeDownCast(block))
    {
      for (unsigned int cc = 0, max = mb->GetNumberOfBlocks(); cc < max; ++cc)
      {
        auto mdata = mb->HasMetaData(cc) ? mb->GetMetaData(cc) : nullptr;
        std::string label("Block " + std::to_string(cc));
        if (mdata && mdata->Has(vtkCompositeDataSet::NAME()))
        {
          label = mdata->Get(vtkCompositeDataSet::NAME());
        }

        auto name = vtkDataAssembly::MakeValidNodeName(label.c_str());
        if (!f(mb->GetBlock(cc), mdata, assembly->AddNode(name.c_str(), id)))
        {
          return false;
        }
      }
      return true;
    }
    else if (auto mp = vtkMultiPieceDataSet::SafeDownCast(block))
    {
      vtkNew<vtkPartitionedDataSet> pds;
      pds->ShallowCopy(mp);

      auto idx = output->GetNumberOfPartitionedDataSets();
      output->SetPartitionedDataSet(idx, pds);
      assembly->AddDataSetIndex(id, idx);

      if (metadata)
      {
        output->GetMetaData(idx)->Copy(metadata);
      }
      return true;
    }
    else if (block == nullptr)
    {
      // we still add an empty vtkPartitionedDataSet here to handle the
      // distributed case where the block may be non-null on some other rank.
      vtkNew<vtkPartitionedDataSet> pds;

      auto idx = output->GetNumberOfPartitionedDataSets();
      output->SetPartitionedDataSet(idx, pds);
      assembly->AddDataSetIndex(id, idx);

      if (metadata)
      {
        output->GetMetaData(idx)->Copy(metadata);
      }
      return true;
    }
    else if (block != nullptr && vtkCompositeDataSet::SafeDownCast(block) == nullptr)
    {
      vtkNew<vtkPartitionedDataSet> pds;
      pds->SetPartition(0, block);

      auto idx = output->GetNumberOfPartitionedDataSets();
      output->SetPartitionedDataSet(idx, pds);
      assembly->AddDataSetIndex(id, idx);

      if (metadata)
      {
        output->GetMetaData(idx)->Copy(metadata);
      }
      return true;
    }

    vtkLogF(ERROR, "Unexpected data encountered in multiblock: '%s'", vtkLogIdentifier(block));
    return false;
  };
  return f(input, nullptr, 0) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkDataObjectToPartitionedDataSetCollection::ConvertFromAMR(
  vtkUniformGridAMR* input, vtkPartitionedDataSetCollection* output)
{
  assert(input && output);

  vtkNew<vtkDataAssembly> assembly;
  assembly->Initialize();
  assembly->SetRootNodeName("AMR");

  const unsigned int num_levels = input->GetNumberOfLevels();
  for (unsigned int level = 0; level < num_levels; ++level)
  {
    const unsigned int num_datasets = input->GetNumberOfDataSets(level);
    vtkNew<vtkPartitionedDataSet> pd;
    pd->SetNumberOfPartitions(num_datasets);
    for (unsigned int idx = 0; idx < num_datasets; ++idx)
    {
      pd->SetPartition(idx, input->GetDataSet(level, idx));
    }
    output->SetPartitionedDataSet(level, pd);
    auto nodeID = assembly->AddNode(("Level " + std::to_string(level)).c_str());
    assembly->AddDataSetIndex(nodeID, level);
  }

  output->SetDataAssembly(assembly);
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataObjectToPartitionedDataSetCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
