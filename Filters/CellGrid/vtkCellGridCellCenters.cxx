// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridCellCenters.h"

#include "vtkCellGridCopyQuery.h"
#include "vtkDataSetAttributes.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridCellCenters);
vtkStandardNewMacro(vtkCellGridCellCenters::Query);

void vtkCellGridCellCenters::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Output: " << this->Output << "\n";
  os << indent << "OutputOffsets: " << this->OutputOffsets.size() << " output cell types\n";
  vtkIndent i2 = indent.GetNextIndent();
  for (const auto& entry : this->OutputOffsets)
  {
    os << i2 << entry.first.Data() << " from " << entry.second.size() << " input cell types\n";
  }
  os << indent << "AttributeMap: " << this->AttributeMap.size() << " entries\n";
}

bool vtkCellGridCellCenters::Query::Initialize()
{
  this->Superclass::Initialize();
  this->OutputOffsets.clear();
  this->AttributeMap.clear();
  if (!this->Input || !this->Output)
  {
    return false;
  }

  // Always create a shape attribute:
  vtkNew<vtkCellAttribute> shapeAtt;
  shapeAtt->Initialize("shape"_token, "ℝ³", 3);
  this->Output->SetShapeAttribute(shapeAtt);
  this->AttributeMap[this->Input->GetShapeAttribute()] = shapeAtt.GetPointer();

  for (const auto& inputAtt : this->Input->GetCellAttributeList())
  {
    if (this->Input->GetShapeAttribute() == inputAtt)
    {
      continue;
    }

    vtkNew<vtkCellAttribute> outputAtt;
    outputAtt->Initialize(
      inputAtt->GetName(), inputAtt->GetSpace(), inputAtt->GetNumberOfComponents());
    this->Output->AddCellAttribute(outputAtt);
    this->AttributeMap[inputAtt] = outputAtt;
  }
  return true;
}

void vtkCellGridCellCenters::Query::AddOutputCenters(
  vtkStringToken inputCellType, vtkStringToken outputCellType, vtkIdType numberOfOutputs)
{
  this->OutputOffsets[outputCellType][inputCellType] = numberOfOutputs;
}

vtkCellAttribute* vtkCellGridCellCenters::Query::GetOutputAttribute(
  vtkCellAttribute* inputAttribute)
{
  if (!inputAttribute)
  {
    return nullptr;
  }

  auto nit = this->AttributeMap.find(inputAttribute);
  if (nit == this->AttributeMap.end())
  {
    return nullptr;
  }
  return nit->second;
}

vtkCellGridCellCenters::vtkCellGridCellCenters()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
}

void vtkCellGridCellCenters::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Query:\n";
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

int vtkCellGridCellCenters::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* ouInfo)
{
  auto* input = vtkCellGrid::GetData(inInfo[0]);
  auto* output = vtkCellGrid::GetData(ouInfo);
  if (!input)
  {
    vtkWarningMacro("Empty input.");
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }

  // Copy the input but leave it empty except for cell-attributes.
  vtkNew<vtkCellGridCopyQuery> copyQuery;
  copyQuery->SetSource(input);
  copyQuery->SetTarget(output);
  copyQuery->CopyCellTypesOff();
  copyQuery->CopyCellsOff();
  copyQuery->CopyOnlyShapeOff();
  copyQuery->CopyArraysOff();
  copyQuery->CopyArrayValuesOff();
  copyQuery->CopySchemaOn();
  // copyQuery->AddAllSourceCellAttributeIds();
  if (!input->Query(copyQuery))
  {
    vtkErrorMacro("Could not copy input to output.");
    return 0;
  }

  this->Request->Input = input;
  this->Request->Output = output;
  // Run the cell-center query on the request.
  if (!input->Query(this->Request))
  {
    vtkErrorMacro("Input failed to respond to query.");
    return 0;
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
