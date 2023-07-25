// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiBlockDataSet.h"

#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkUniformGridAMR.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMultiBlockDataSet);
//------------------------------------------------------------------------------
vtkMultiBlockDataSet::vtkMultiBlockDataSet() = default;

//------------------------------------------------------------------------------
vtkMultiBlockDataSet::~vtkMultiBlockDataSet() = default;

//------------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkMultiBlockDataSet::GetData(vtkInformation* info)
{
  return vtkMultiBlockDataSet::SafeDownCast(vtkDataObject::GetData(info));
}

//------------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkMultiBlockDataSet::GetData(vtkInformationVector* v, int i)
{
  return vtkMultiBlockDataSet::SafeDownCast(vtkDataObject::GetData(v, i));
}

//------------------------------------------------------------------------------
void vtkMultiBlockDataSet::SetNumberOfBlocks(unsigned int numBlocks)
{
  this->Superclass::SetNumberOfChildren(numBlocks);
}

//------------------------------------------------------------------------------
unsigned int vtkMultiBlockDataSet::GetNumberOfBlocks()
{
  return this->Superclass::GetNumberOfChildren();
}

//------------------------------------------------------------------------------
vtkDataObject* vtkMultiBlockDataSet::GetBlock(unsigned int blockno)
{
  return this->Superclass::GetChild(blockno);
}

//------------------------------------------------------------------------------
void vtkMultiBlockDataSet::SetBlock(unsigned int blockno, vtkDataObject* block)
{
  if (vtkUniformGridAMR::SafeDownCast(block) != nullptr)
  {
    vtkErrorMacro("vtkUniformGridAMR cannot be added as block.");
    return;
  }

  if (vtkPartitionedDataSet::SafeDownCast(block) != nullptr &&
    vtkMultiPieceDataSet::SafeDownCast(block) == nullptr)
  {
    vtkErrorMacro("vtkPartitionedDataSet cannot be added as a block.");
    return;
  }

  if (vtkPartitionedDataSetCollection::SafeDownCast(block) != nullptr)
  {
    vtkErrorMacro("vtkPartitionedDataSetCollection cannot be added as a block.");
    return;
  }

  this->Superclass::SetChild(blockno, block);
}

//------------------------------------------------------------------------------
void vtkMultiBlockDataSet::RemoveBlock(unsigned int blockno)
{
  this->Superclass::RemoveChild(blockno);
}

//------------------------------------------------------------------------------
vtkDataObjectTree* vtkMultiBlockDataSet::CreateForCopyStructure(vtkDataObjectTree* other)
{
  return vtkPartitionedDataSet::SafeDownCast(other)
    ? vtkMultiPieceDataSet::New()
    : this->Superclass::CreateForCopyStructure(other);
}

//------------------------------------------------------------------------------
void vtkMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
