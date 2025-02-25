// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGenerateFields.h"

#include "vtkCellData.h"
#include "vtkHyperTreeGridCellCenterStrategy.h"
#include "vtkHyperTreeGridCellSizeStrategy.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridTotalVisibleVolumeStrategy.h"
#include "vtkHyperTreeGridValidCellStrategy.h"

#define fieldMacros(Name)                                                                          \
  std::string vtkHyperTreeGridGenerateFields::Get##Name##ArrayName() VTK_FUTURE_CONST              \
  {                                                                                                \
    return this->Fields.at(#Name).strategy->GetArrayName();                                        \
  }                                                                                                \
  void vtkHyperTreeGridGenerateFields::Set##Name##ArrayName(std::string _arg)                      \
  {                                                                                                \
    if (this->Fields[#Name].strategy->GetArrayName() != _arg)                                      \
    {                                                                                              \
      this->Fields[#Name].name = _arg;                                                             \
      this->Fields[#Name].strategy->SetArrayName(_arg);                                            \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  bool vtkHyperTreeGridGenerateFields::GetCompute##Name##Array() VTK_FUTURE_CONST                  \
  {                                                                                                \
    return this->Fields.at(#Name).enabled;                                                         \
  }                                                                                                \
  void vtkHyperTreeGridGenerateFields::SetCompute##Name##Array(bool _arg)                          \
  {                                                                                                \
    if (this->Fields[#Name].enabled != _arg)                                                       \
    {                                                                                              \
      this->Fields[#Name].enabled = _arg;                                                          \
      this->Modified();                                                                            \
    }                                                                                              \
  }

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridGenerateFields);

fieldMacros(CellSize);
fieldMacros(ValidCell);
fieldMacros(CellCenter);
fieldMacros(TotalVisibleVolume);

//------------------------------------------------------------------------------
vtkHyperTreeGridGenerateFields::vtkHyperTreeGridGenerateFields()
{
  // Cell Data

  vtkNew<vtkHyperTreeGridCellSizeStrategy> cellSize;
  cellSize->SetArrayName(this->DefaultCellSizeArrayName);
  cellSize->SetArrayType(vtkDataObject::AttributeTypes::CELL);
  this->Fields["CellSize"] = { this->DefaultCellSizeArrayName, cellSize, true };

  vtkNew<vtkHyperTreeGridValidCellStrategy> validCell;
  validCell->SetArrayName(this->DefaultValidCellArrayName);
  validCell->SetArrayType(vtkDataObject::AttributeTypes::CELL);
  this->Fields["ValidCell"] = { this->DefaultValidCellArrayName, validCell, true };

  vtkNew<vtkHyperTreeGridCellCenterStrategy> cellCenter;
  cellCenter->SetArrayName(this->DefaultCellCenterArrayName);
  cellCenter->SetArrayType(vtkDataObject::AttributeTypes::CELL);
  this->Fields["CellCenter"] = { this->DefaultCellCenterArrayName, cellCenter, true };

  // Field Data

  vtkNew<vtkHyperTreeGridTotalVisibleVolumeStrategy> totalVisibleVolume;
  totalVisibleVolume->SetArrayName(this->DefaultTotalVisibleVolumeArrayName);
  totalVisibleVolume->SetArrayType(vtkDataObject::AttributeTypes::FIELD);
  this->Fields["TotalVisibleVolume"] = { this->DefaultTotalVisibleVolumeArrayName,
    totalVisibleVolume, true };

  this->AppropriateOutput = true;
};

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Fields:"
     << "\n";
  for (const auto& [key, field] : this->Fields)
  {
    os << indent << key << "\n";
    indent = indent.GetNextIndent();
    os << indent << "Enabled: " << field.enabled << "\n";
    os << indent << "Valid: " << field.valid << "\n";
    field.strategy->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::ProcessFields(
  vtkHyperTreeGrid* outputHTG, vtkHyperTreeGrid* input, const vtkDataObject::AttributeTypes type)
{
  for (auto& [key, field] : this->Fields)
  {
    field.valid = false;
    if (field.enabled && field.strategy->GetArrayType() == type)
    {
      if (type == vtkDataObject::AttributeTypes::CELL)
      {
        field.strategy->Initialize(input);
        field.valid = true;
      }
      else if (type == vtkDataObject::AttributeTypes::FIELD)
      {
        field.valid = field.strategy->Initialize(this->Fields);
      }
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
  for (const auto& [key, field] : this->Fields)
  {
    if (field.valid && field.strategy->GetArrayType() == type)
    {
      vtkDataArray* resultArray = field.strategy->GetAndFinalizeArray();
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
  for (const auto& [key, field] : this->Fields)
  {
    if (field.valid && field.strategy->GetArrayType() == type)
    {
      if (type == vtkDataObject::AttributeTypes::CELL)
      {
        field.strategy->Compute(cursor);
      }
      else if (type == vtkDataObject::AttributeTypes::FIELD)
      {
        field.strategy->Compute(cursor, outputCellData, this->Fields);
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
