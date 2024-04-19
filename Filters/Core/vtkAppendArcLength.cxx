// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "vtkAppendArcLength.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAppendArcLength);
//------------------------------------------------------------------------------
vtkAppendArcLength::vtkAppendArcLength() = default;

//------------------------------------------------------------------------------
vtkAppendArcLength::~vtkAppendArcLength() = default;

//------------------------------------------------------------------------------
int vtkAppendArcLength::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);
  if (input->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  output->ShallowCopy(input);

  // Now add "arc_length" array.
  vtkDataArray* arc_length = nullptr;
  vtkPoints* points = output->GetPoints();
  vtkIdType numPoints = points->GetNumberOfPoints();
  if (points->GetDataType() == VTK_DOUBLE)
  {
    arc_length = vtkDoubleArray::New();
  }
  else
  {
    arc_length = vtkFloatArray::New();
  }
  arc_length->SetName("arc_length");
  arc_length->SetNumberOfComponents(1);
  arc_length->SetNumberOfTuples(numPoints);
  arc_length->FillComponent(0, 0.0);

  vtkCellArray* lines = output->GetLines();
  vtkIdType numCellPoints;
  const vtkIdType* cellPoints;
  lines->InitTraversal();
  vtkIdType checkAbortInterval = std::min(lines->GetNumberOfCells() / 10 + 1, (vtkIdType)1000);
  vtkIdType progressCounter = 0;
  while (lines->GetNextCell(numCellPoints, cellPoints))
  {
    if (progressCounter % checkAbortInterval == 0 && this->CheckAbort())
    {
      break;
    }
    if (numCellPoints == 0)
    {
      continue;
    }
    double arc_distance = 0.0;
    double prevPoint[3];
    points->GetPoint(cellPoints[0], prevPoint);
    for (vtkIdType cc = 1; cc < numCellPoints; cc++)
    {
      double curPoint[3];
      points->GetPoint(cellPoints[cc], curPoint);
      double distance = sqrt(vtkMath::Distance2BetweenPoints(curPoint, prevPoint));
      arc_distance += distance;
      arc_length->SetTuple1(cellPoints[cc], arc_distance);
      memcpy(prevPoint, curPoint, 3 * sizeof(double));
    }
    progressCounter++;
  }
  output->GetPointData()->AddArray(arc_length);
  arc_length->Delete();
  return 1;
}

//------------------------------------------------------------------------------
void vtkAppendArcLength::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
