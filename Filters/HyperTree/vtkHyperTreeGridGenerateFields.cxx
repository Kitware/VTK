// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGenerateFields.h"

#include "vtkCellData.h"
#include "vtkHyperTreeGridCellCenterStrategy.h"
#include "vtkHyperTreeGridCellSizeStrategy.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridTotalVisibleVolumeStrategy.h"
#include "vtkHyperTreeGridValidCellStrategy.h"

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
vtkStandardNewMacro(vtkHyperTreeGridGenerateFields);

vtkHTGGenerateFieldsGetFieldNameMacro(CellSize);
vtkHTGGenerateFieldsSetFieldNameMacro(CellSize);
vtkHTGGenerateFieldsGetFieldNameMacro(ValidCell);
vtkHTGGenerateFieldsSetFieldNameMacro(ValidCell);
vtkHTGGenerateFieldsGetFieldNameMacro(CellCenter);
vtkHTGGenerateFieldsSetFieldNameMacro(CellCenter);
vtkHTGGenerateFieldsGetFieldNameMacro(TotalVisibleVolume);
vtkHTGGenerateFieldsSetFieldNameMacro(TotalVisibleVolume);

//------------------------------------------------------------------------------
vtkHyperTreeGridGenerateFields::vtkHyperTreeGridGenerateFields()
{
  // Cell Data

  vtkNew<vtkHyperTreeGridCellSizeStrategy> cellSize;
  cellSize->SetArrayName(this->DefaultCellSizeArrayName);
  cellSize->SetArrayType(vtkDataObject::AttributeTypes::CELL);
  this->FieldsNameMap.emplace("CellSize", this->DefaultCellSizeArrayName);
  this->Fields.emplace("CellSize", cellSize);

  vtkNew<vtkHyperTreeGridValidCellStrategy> validCell;
  validCell->SetArrayName(this->DefaultValidCellArrayName);
  validCell->SetArrayType(vtkDataObject::AttributeTypes::CELL);
  this->FieldsNameMap.emplace("ValidCell", this->DefaultValidCellArrayName);
  this->Fields.emplace("ValidCell", validCell);

  vtkNew<vtkHyperTreeGridCellCenterStrategy> cellCenter;
  cellCenter->SetArrayName(this->DefaultCellCenterArrayName);
  cellCenter->SetArrayType(vtkDataObject::AttributeTypes::CELL);
  this->FieldsNameMap.emplace("CellCenter", this->DefaultCellCenterArrayName);
  this->Fields.emplace("CellCenter", cellCenter);

  // Field Data

  vtkNew<vtkHyperTreeGridTotalVisibleVolumeStrategy> totalVisibleVolume;
  totalVisibleVolume->SetArrayName(this->DefaultTotalVisibleVolumeArrayName);
  totalVisibleVolume->SetArrayType(vtkDataObject::AttributeTypes::FIELD);
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
  vtkHyperTreeGrid* outputHTG, vtkHyperTreeGrid* input, const vtkDataObject::AttributeTypes type)
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
      if (type == vtkDataObject::AttributeTypes::CELL)
      {
        outputHTG->GetCellData()->AddArray(resultArray);
      }
      else if (type == vtkDataObject::AttributeTypes::FIELD)
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

  this->ProcessFields(outputHTG, input, vtkDataObject::AttributeTypes::CELL);

  this->ProcessFields(outputHTG, input, vtkDataObject::AttributeTypes::FIELD);

  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::ProcessNode(vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
  const vtkDataObject::AttributeTypes type, vtkCellData* outputCellData)
{
  for (const auto& field : this->Fields)
  {
    if (field.second->GetArrayType() == type)
    {
      if (type == vtkDataObject::AttributeTypes::CELL)
      {
        field.second->Compute(cursor);
      }
      else if (type == vtkDataObject::AttributeTypes::FIELD)
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
