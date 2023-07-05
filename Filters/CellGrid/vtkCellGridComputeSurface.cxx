// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridComputeSurface.h"

#include "vtkCellGridSidesQuery.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals; // for ""_token

vtkStandardNewMacro(vtkCellGridComputeSurface);

void vtkCellGridComputeSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkStringToken vtkCellGridComputeSurface::GetSideAttribute()
{
  return vtkStringToken("Sides");
}

int vtkCellGridComputeSurface::RequestData(
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
  output->ShallowCopy(input);
  if (!output->Query(this->Request))
  {
    vtkErrorMacro("Input failed to respond to query.");
    return 0;
  }
  const auto& sides = this->Request->GetSides();
  for (const auto& cellTypeEntry : sides)
  {
    for (const auto& sideShapeEntry : cellTypeEntry.second)
    {
      vtkIdType sideCount = 0;
      for (const auto& entry : sideShapeEntry.second)
      {
        sideCount += entry.second.size();
      }
      vtkNew<vtkIdTypeArray> sideArray;
      sideArray->SetName("conn");
      sideArray->SetNumberOfComponents(2); // tuples are (cell ID, side ID)
      sideArray->SetNumberOfTuples(sideCount);
      vtkIdType sideId = 0;
      std::array<vtkIdType, 2> sideTuple;
      for (const auto& entry : sideShapeEntry.second)
      {
        sideTuple[0] = entry.first;
        for (const auto& ss : entry.second)
        {
          sideTuple[1] = ss;
          sideArray->SetTypedTuple(sideId, sideTuple.data());
          ++sideId;
        }
      }
      std::ostringstream sideAttrName;
      sideAttrName << sideShapeEntry.first.Data() << " sides of " << cellTypeEntry.first.Data();
      vtkStringToken sideAttrToken(sideAttrName.str());
      auto* attr = output->GetAttributes(sideAttrToken);
      attr->AddArray(sideArray);
      attr->SetScalars(sideArray);
    }
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
