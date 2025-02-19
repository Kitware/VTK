// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridTotalVisibleVolumeStrategy.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkIndexedArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridTotalVisibleVolumeStrategy);

//------------------------------------------------------------------------------
vtkHyperTreeGridTotalVisibleVolumeStrategy::vtkHyperTreeGridTotalVisibleVolumeStrategy() = default;

//------------------------------------------------------------------------------
vtkHyperTreeGridTotalVisibleVolumeStrategy::~vtkHyperTreeGridTotalVisibleVolumeStrategy() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeGridTotalVisibleVolumeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TotalVisibleVolume: " << this->TotalVisibleVolume << "\n";
  os << indent << "TotalVisibleVolumeArray size: "
     << (this->TotalVisibleVolumeArray ? this->TotalVisibleVolumeArray->GetNumberOfTuples() : 0)
     << "\n";
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridTotalVisibleVolumeStrategy::Initialize(
  std::unordered_map<std::string, Field> fieldMap)
{
  if (!fieldMap["ValidCell"].enabled || !fieldMap["CellSize"].enabled)
  {
    vtkWarningMacro("ValidCell and CellSize arrays must be enabled to compute TotalVisibleVolume");
    return false;
  }

  this->TotalVisibleVolume = 0;
  this->TotalVisibleVolumeArray->SetNumberOfComponents(1);
  this->TotalVisibleVolumeArray->SetNumberOfTuples(1);
  this->TotalVisibleVolumeArray->SetName(this->ArrayName.c_str());

  return true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridTotalVisibleVolumeStrategy::Compute(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkCellData* cellData,
  std::unordered_map<std::string, Field> fieldMap)
{
  vtkAbstractArray* validCellArray = cellData->GetAbstractArray(fieldMap["ValidCell"].name.c_str());
  vtkAbstractArray* cellSizeArray = cellData->GetAbstractArray(fieldMap["CellSize"].name.c_str());

  if (!validCellArray || !cellSizeArray)
  {
    vtkLogF(ERROR, "ValidCell and CellSize arrays are required to compute TotalVisibleVolume!");
    return;
  }

  auto validCellBoolArray = vtkBitArray::SafeDownCast(validCellArray);

  // Type may change depending on the number of values
  auto cellSizeIndexedArray = vtkIndexedArray<double>::SafeDownCast(cellSizeArray);
  auto cellSizeDoubleArray = vtkDoubleArray::SafeDownCast(cellSizeArray);

  const vtkIdType currentId = cursor->GetGlobalNodeIndex();
  if (validCellBoolArray->GetValue(currentId))
  {
    this->TotalVisibleVolume += cellSizeIndexedArray ? cellSizeIndexedArray->GetValue(currentId)
                                                     : cellSizeDoubleArray->GetValue(currentId);
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkHyperTreeGridTotalVisibleVolumeStrategy::GetAndFinalizeArray()
{
  this->TotalVisibleVolumeArray->SetTuple1(0, this->TotalVisibleVolume);
  return this->TotalVisibleVolumeArray;
}

VTK_ABI_NAMESPACE_END
