// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDecimatePolylineDistanceStrategy.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkPointSet.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDecimatePolylineDistanceStrategy);

//------------------------------------------------------------------------------
double vtkDecimatePolylineDistanceStrategy::ComputeError(
  vtkPointSet* dataset, const vtkIdType originId, const vtkIdType p1Id, const vtkIdType p2Id)
{
  double origin[3], p1[3], p2[3];

  dataset->GetPoint(originId, origin);
  dataset->GetPoint(p1Id, p1);
  dataset->GetPoint(p2Id, p2);

  if (vtkMath::Distance2BetweenPoints(p1, p2) == 0.0)
  {
    return 0.0;
  }
  else
  {
    return vtkLine::DistanceToLine(origin, p1, p2);
  }
}

VTK_ABI_NAMESPACE_END
