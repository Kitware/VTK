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
    return this->Name##ArrayName;                                                                  \
  }                                                                                                \
  void vtkHyperTreeGridGenerateFields::Set##Name##ArrayName(std::string _arg)                      \
  {                                                                                                \
    if (this->Name##ArrayName != _arg)                                                             \
    {                                                                                              \
      this->Name##ArrayName = _arg;                                                                \
      this->Modified();                                                                            \
    }                                                                                              \
  }                                                                                                \
  bool vtkHyperTreeGridGenerateFields::GetCompute##Name##Array() VTK_FUTURE_CONST                  \
  {                                                                                                \
    return this->Compute##Name##Array;                                                             \
  }                                                                                                \
  void vtkHyperTreeGridGenerateFields::SetCompute##Name##Array(bool _arg)                          \
  {                                                                                                \
    if (this->Compute##Name##Array != _arg)                                                        \
    {                                                                                              \
      this->Compute##Name##Array = _arg;                                                           \
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
  this->InitializeFields();
  this->AppropriateOutput = true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::InitializeFields()
{
  this->Fields.clear();

  // Cell Data

  if (this->ComputeCellSizeArray)
  {
    vtkNew<vtkHyperTreeGridCellSizeStrategy> cellSize;
    cellSize->SetArrayName(this->CellSizeArrayName);
    cellSize->SetArrayType(vtkDataObject::AttributeTypes::CELL);
    this->Fields["CellSize"] = { this->CellSizeArrayName, cellSize, true };
  }

  if (this->ComputeValidCellArray)
  {
    vtkNew<vtkHyperTreeGridValidCellStrategy> validCell;
    validCell->SetArrayName(this->ValidCellArrayName);
    validCell->SetArrayType(vtkDataObject::AttributeTypes::CELL);
    this->Fields["ValidCell"] = { this->ValidCellArrayName, validCell, true };
  }

  if (this->ComputeCellCenterArray)
  {
    vtkNew<vtkHyperTreeGridCellCenterStrategy> cellCenter;
    cellCenter->SetArrayName(this->CellCenterArrayName);
    cellCenter->SetArrayType(vtkDataObject::AttributeTypes::CELL);
    this->Fields["CellCenter"] = { this->CellCenterArrayName, cellCenter, true };
  }

  // Field Data

  if (this->ComputeTotalVisibleVolumeArray)
  {
    vtkNew<vtkHyperTreeGridTotalVisibleVolumeStrategy> totalVisibleVolume;
    totalVisibleVolume->SetArrayName(this->TotalVisibleVolumeArrayName);
    totalVisibleVolume->SetArrayType(vtkDataObject::AttributeTypes::FIELD);
    this->Fields["TotalVisibleVolume"] = { this->TotalVisibleVolumeArrayName, totalVisibleVolume,
      true };
  }
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
  if (input->GetNumberOfCells() != input->GetCellData()->GetNumberOfTuples())
  {
    vtkErrorMacro("Input has " << input->GetNumberOfCells() << " cells but has "
                               << input->GetCellData()->GetNumberOfTuples()
                               << " tuples  in cell data, cannot generate fields");
    return 0;
  }

  this->InitializeFields();

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
