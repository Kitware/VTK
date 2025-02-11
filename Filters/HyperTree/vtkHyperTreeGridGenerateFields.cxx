// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGenerateFields.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayMeta.h" // for GetAPIType
#include "vtkDataObject.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridCellSizeStrategy.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridScales.h"
#include "vtkHyperTreeGridTotalVisibleVolumeStrategy.h"
#include "vtkHyperTreeGridValidCellStrategy.h"
#include "vtkImplicitArray.h"
#include "vtkIndent.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

#include <memory>
#include <vector>

#define vtkHTGGenerateFieldsGetFieldNameMacro(name)                                                \
  std::string vtkHyperTreeGridGenerateFields::Get##name##ArrayName() VTK_FUTURE_CONST              \
  {                                                                                                \
    return this->Fields[#name]->GetArrayName();                                                    \
  }

#define vtkHTGGenerateFieldsSetFieldNameMacro(name)                                                \
  void vtkHyperTreeGridGenerateFields::Set##name##ArrayName(std::string _arg)                      \
  {                                                                                                \
    if (this->Fields[#name]->GetArrayName() != _arg)                                               \
    {                                                                                              \
      this->Fields[#name]->SetArrayName(_arg);                                                     \
      this->FieldsNameMap[#name] = _arg;                                                           \
      this->Modified();                                                                            \
    }                                                                                              \
  }

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridGenerateFields)

  vtkHTGGenerateFieldsGetFieldNameMacro(CellSize);
vtkHTGGenerateFieldsSetFieldNameMacro(CellSize);
vtkHTGGenerateFieldsGetFieldNameMacro(ValidCell);
vtkHTGGenerateFieldsSetFieldNameMacro(ValidCell);
vtkHTGGenerateFieldsGetFieldNameMacro(TotalVisibleVolume);
vtkHTGGenerateFieldsSetFieldNameMacro(TotalVisibleVolume);

//------------------------------------------------------------------------------
vtkHyperTreeGridGenerateFields::vtkHyperTreeGridGenerateFields()
{
  // Cell Data

  vtkNew<vtkHyperTreeGridCellSizeStrategy> cellSize;
  cellSize->SetArrayName(this->DefaultCellSizeArrayName);
  cellSize->SetArrayType(DataArrayType::CELL_DATA);
  this->FieldsNameMap.emplace("CellSize", this->DefaultCellSizeArrayName);
  this->Fields.emplace("CellSize", cellSize);

  vtkNew<vtkHyperTreeGridValidCellStrategy> validCell;
  validCell->SetArrayName(this->DefaultValidCellArrayName);
  validCell->SetArrayType(DataArrayType::CELL_DATA);
  this->FieldsNameMap.emplace("ValidCell", this->DefaultValidCellArrayName);
  this->Fields.emplace("ValidCell", validCell);

  // Field Data

  vtkNew<vtkHyperTreeGridTotalVisibleVolumeStrategy> totalVisibleVolume;
  totalVisibleVolume->SetArrayName(this->DefaultTotalVisibleVolumeArrayName);
  totalVisibleVolume->SetArrayType(DataArrayType::FIELD_DATA);
  this->FieldsNameMap.emplace("TotalVisibleVolume", this->DefaultTotalVisibleVolumeArrayName);
  this->Fields.emplace("TotalVisibleVolume", totalVisibleVolume);

  this->AppropriateOutput = true;
};

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Fields:"
     << "\n";
  for (const auto& field : this->Fields)
  {
    os << indent << field.first << "\n";
    field.second->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::ProcessFields(
  vtkHyperTreeGrid* outputHTG, vtkHyperTreeGrid* input, const DataArrayType type)
{
  for (const auto& field : this->Fields)
  {
    if (field.second->GetArrayType() == type)
    {
      field.second->Initialize(input);
    }
  }

  // Iterate over all input and output hyper trees
  vtkIdType index = 0;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator iterator;
  outputHTG->InitializeTreeIterator(iterator);
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> outCursor;
  while (iterator.GetNextTree(index))
  {
    if (this->CheckAbort())
    {
      break;
    }
    outputHTG->InitializeNonOrientedGeometryCursor(outCursor, index);
    this->ProcessNode(outCursor, type, outputHTG->GetCellData());
  }

  // Append all field arrays to the output
  for (const auto& field : this->Fields)
  {
    if (field.second->GetArrayType() == type)
    {
      vtkDataArray* resultArray = field.second->GetAndFinalizeArray();
      if (type == DataArrayType::CELL_DATA)
      {
        outputHTG->GetCellData()->AddArray(resultArray);
      }
      else if (type == DataArrayType::FIELD_DATA)
      {
        outputHTG->GetFieldData()->AddArray(resultArray);
      }
    }
  }
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGenerateFields::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (outputHTG == nullptr)
  {
    vtkErrorMacro(
      "Incorrect type of output: " << outputDO->GetClassName() << ". Expected vtkHyperTreeGrid");
    return 0;
  }

  outputHTG->ShallowCopy(input);

  this->ProcessFields(outputHTG, input, DataArrayType::CELL_DATA);

  this->ProcessFields(outputHTG, input, DataArrayType::FIELD_DATA);

  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::ProcessNode(vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
  const DataArrayType type, vtkCellData* outputCellData)
{
  for (const auto& field : this->Fields)
  {
    if (field.second->GetArrayType() == type)
    {
      if (type == DataArrayType::CELL_DATA)
      {
        field.second->Compute(cursor);
      }
      else if (type == DataArrayType::FIELD_DATA)
      {
        field.second->Compute(cursor, outputCellData, this->FieldsNameMap);
      }
    }
  }

  // `IsLeaf` result can depend on whether a depth limiter has been applied on the tree.
  if (cursor->IsLeaf())
  {
    return;
  }

  if (cursor->IsMasked())
  {
    return; // Masked cells' children are automatically invalid
  }

  for (unsigned int childId = 0; childId < cursor->GetNumberOfChildren(); ++childId)
  {
    cursor->ToChild(childId);
    this->ProcessNode(cursor, type, outputCellData);
    cursor->ToParent();
  }
}

VTK_ABI_NAMESPACE_END
