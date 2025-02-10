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
#include "vtkHyperTreeGridGenerateFieldCellSize.h"
#include "vtkHyperTreeGridGenerateFieldValidCell.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridScales.h"
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
      this->Modified();                                                                            \
    }                                                                                              \
  }

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridGenerateFields)

  vtkHTGGenerateFieldsGetFieldNameMacro(CellSize);
vtkHTGGenerateFieldsSetFieldNameMacro(CellSize);
vtkHTGGenerateFieldsGetFieldNameMacro(ValidCell);
vtkHTGGenerateFieldsSetFieldNameMacro(ValidCell);

//------------------------------------------------------------------------------
vtkHyperTreeGridGenerateFields::vtkHyperTreeGridGenerateFields()
{
  vtkNew<vtkHyperTreeGridGenerateFieldCellSize> cellSize;
  cellSize->SetArrayName("CellSize");
  this->Fields.emplace("CellSize", cellSize);

  vtkNew<vtkHyperTreeGridGenerateFieldValidCell> validCell;
  validCell->SetArrayName("ValidCell");
  this->Fields.emplace("ValidCell", validCell);

  this->AppropriateOutput = true;
};

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (const auto& field : this->Fields)
  {
    os << indent << field.first << "\n";
    field.second->PrintSelf(os, indent.GetNextIndent());
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

  for (const auto& field : this->Fields)
  {
    field.second->Initialize(input);
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
    this->ProcessNode(outCursor);
  }

  // Append all field arrays to the output
  for (const auto& field : this->Fields)
  {
    outputHTG->GetCellData()->AddArray(field.second->GetAndFinalizeArray());
  }

  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGenerateFields::ProcessNode(
  vtkHyperTreeGridNonOrientedGeometryCursor* outCursor)
{
  for (const auto& field : this->Fields)
  {
    field.second->Compute(outCursor);
  }

  // `IsLeaf` result can depend on whether a depth limiter has been applied on the tree.
  if (outCursor->IsLeaf())
  {
    return;
  }

  if (outCursor->IsMasked())
  {
    return; // Masked cells' children are automatically invalid
  }

  for (unsigned int childId = 0; childId < outCursor->GetNumberOfChildren(); ++childId)
  {
    outCursor->ToChild(childId);
    this->ProcessNode(outCursor);
    outCursor->ToParent();
  }
}

VTK_ABI_NAMESPACE_END
