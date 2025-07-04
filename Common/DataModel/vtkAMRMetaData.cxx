// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAMRMetaData.h"

#include "vtkStructuredData.h"
#include "vtkUnsignedIntArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAMRMetaData);

//------------------------------------------------------------------------------
vtkAMRMetaData::vtkAMRMetaData() = default;

//------------------------------------------------------------------------------
vtkAMRMetaData::~vtkAMRMetaData() = default;

//------------------------------------------------------------------------------
void vtkAMRMetaData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Grid description: " << this->GetGridDescription() << "\n";
  os << indent << "Number of blocks per level: ";
  for (unsigned int i = 1; i < this->NumBlocks.size(); i++)
  {
    os << indent << this->NumBlocks[i] - this->NumBlocks[i - 1] << " ";
  }
  os << "\n";
}

// VTK_DEPRECATED_IN_9_6_0("Use Initialize(const std::vector<unsigned int>&) instead")
//------------------------------------------------------------------------------
void vtkAMRMetaData::Initialize(int numLevels, const int* blocksPerLevel)
{
  std::vector<unsigned int> vec;
  vec.assign(blocksPerLevel, blocksPerLevel + numLevels);
  this->Initialize(vec);
}

//------------------------------------------------------------------------------
void vtkAMRMetaData::Initialize(const std::vector<unsigned int>& blocksPerLevel)
{
  // allocate blocks
  this->NumBlocks.resize(blocksPerLevel.size() + 1, 0);
  for (std::size_t i = 0; i < blocksPerLevel.size(); i++)
  {
    this->NumBlocks[i + 1] = this->NumBlocks[i] + blocksPerLevel[i];
  }
}

//------------------------------------------------------------------------------
void vtkAMRMetaData::SetGridDescription(int description)
{
  if (description < vtkStructuredData::VTK_STRUCTURED_SINGLE_POINT ||
    description > vtkStructuredData::VTK_STRUCTURED_EMPTY)
  {
    vtkErrorMacro("Invalid grid description for a vtkUniformGrid.");
    return;
  }
  this->GridDescription = description;
}

//------------------------------------------------------------------------------
unsigned int vtkAMRMetaData::GetNumberOfLevels() const
{
  return static_cast<unsigned int>(this->NumBlocks.size() - 1);
}

//------------------------------------------------------------------------------
unsigned int vtkAMRMetaData::GetNumberOfBlocks(unsigned int level) const
{
  if (level >= this->GetNumberOfLevels())
  {
    return 0;
  }
  return this->NumBlocks[level + 1] - this->NumBlocks[level];
}

//------------------------------------------------------------------------------
unsigned int vtkAMRMetaData::GetNumberOfBlocks() const
{
  return this->NumBlocks.back();
}

//------------------------------------------------------------------------------
int vtkAMRMetaData::GetAbsoluteBlockIndex(unsigned int level, unsigned int relativeBlockIndex) const
{
  return this->NumBlocks[level] + relativeBlockIndex;
}

//------------------------------------------------------------------------------
void vtkAMRMetaData::ComputeIndexPair(unsigned int index, unsigned int& level, unsigned int& id)
{
  this->GenerateBlockLevel();
  level = this->BlockLevel->GetValue(static_cast<vtkIdType>(index));
  id = index - this->NumBlocks[level];
}

//------------------------------------------------------------------------------
void vtkAMRMetaData::GenerateBlockLevel()
{
  if (this->BlockLevel)
  {
    return;
  }
  this->BlockLevel = vtkSmartPointer<vtkUnsignedIntArray>::New();

  this->BlockLevel->SetNumberOfValues(static_cast<vtkIdType>(this->GetNumberOfBlocks()));

  assert(this->NumBlocks.size() == this->GetNumberOfLevels() + 1);

  vtkIdType index(0);
  for (std::size_t level = 0; level < this->NumBlocks.size() - 1; level++)
  {
    unsigned int begin = this->NumBlocks[level];
    unsigned int end = this->NumBlocks[level + 1];
    for (unsigned int id = begin; id != end; id++)
    {
      this->BlockLevel->SetValue(index++, static_cast<unsigned int>(level));
    }
  }
}

//------------------------------------------------------------------------------
bool vtkAMRMetaData::operator==(const vtkAMRMetaData& other) const
{
  if (this->GridDescription != other.GridDescription)
  {
    return false;
  }
  if (this->NumBlocks.size() != other.NumBlocks.size())
  {
    return false;
  }
  for (std::size_t i = 0; i < this->NumBlocks.size(); i++)
  {
    if (this->NumBlocks[i] != other.NumBlocks[i])
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkAMRMetaData::DeepCopy(vtkAMRMetaData* other)
{
  this->GridDescription = other->GridDescription;
  this->NumBlocks = other->NumBlocks;
}

VTK_ABI_NAMESPACE_END
