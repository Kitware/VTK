// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridWarp.h"

#include "vtkCellAttribute.h"
#include "vtkDataSetAttributes.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridWarp);
vtkStandardNewMacro(vtkCellGridWarp::Query);
vtkCxxSetObjectMacro(vtkCellGridWarp::Query, DeformationAttribute, vtkCellAttribute);

vtkCellGridWarp::Query::~Query()
{
  this->SetDeformationAttribute(nullptr);
}

void vtkCellGridWarp::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "DeformationAttribute: " << this->DeformationAttribute << " (\""
     << (this->DeformationAttribute ? this->DeformationAttribute->GetName().Data() : "null")
     << "\")\n";
}

vtkCellGridWarp::vtkCellGridWarp()
{
  static bool once = false;
  if (!once)
  {
    vtkFiltersCellGrid::RegisterCellsAndResponders();
  }
}

void vtkCellGridWarp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Query:\n";
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

vtkMTimeType vtkCellGridWarp::GetMTime()
{
  vtkMTimeType st = this->Superclass::GetMTime();
  vtkMTimeType rt = this->Request->GetMTime();
  return rt > st ? rt : st;
}

int vtkCellGridWarp::RequestData(
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

  // Copy the input; we'll add the new vtkCellAttribute later.
  output->ShallowCopy(input);

  if (auto* defAtt = this->GetInputCellAttributeToProcess(0, input))
  {
    this->Request->SetDeformationAttribute(defAtt);
  }
  else
  {
    // Succeed, but warn.
    vtkWarningMacro("No deformation attribute specified.");
    return 1;
  }

  // Run the query on the request.
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Input failed to respond to query.");
    return 0;
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
