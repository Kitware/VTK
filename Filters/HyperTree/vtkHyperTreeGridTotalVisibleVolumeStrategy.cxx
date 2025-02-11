// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridTotalVisibleVolumeStrategy.h"

#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkHyperTreeGridValidCellStrategy.h>
#include <vtkImplicitArray.h>
#include <vtkIndexedArray.h>
#include <vtkLogger.h>

#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridTotalVisibleVolumeStrategy)

  //------------------------------------------------------------------------------
  void vtkHyperTreeGridTotalVisibleVolumeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TotalVisibleVolume: " << this->TotalVisibleVolume << "\n";
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridTotalVisibleVolumeStrategy::Initialize(vtkHyperTreeGrid* vtkNotUsed(inputHTG))
{
  this->TotalVisibleVolume = 0;
  this->TotalVisibleVolumeArray->SetNumberOfComponents(1);
  this->TotalVisibleVolumeArray->SetNumberOfTuples(1);
  this->TotalVisibleVolumeArray->SetName(this->ArrayName.c_str());
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridTotalVisibleVolumeStrategy::Compute(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkCellData* cellData,
  std::unordered_map<std::string, std::string> nameMap)
{
  vtkAbstractArray* validCellArray = cellData->GetAbstractArray(nameMap["ValidCell"].c_str());
  vtkAbstractArray* cellSizeArray = cellData->GetAbstractArray(nameMap["CellSize"].c_str());

  if (!validCellArray || !cellSizeArray)
  {
    vtkLogF(ERROR, "ValidCell and CellSize arrays are required to compute TotalVisibleVolume!");
    return;
  }

  auto validCellBoolArray =
    vtkImplicitArray<vtkScalarBooleanImplicitBackend<double>>::SafeDownCast(validCellArray);

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
