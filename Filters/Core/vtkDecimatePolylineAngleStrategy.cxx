// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDecimatePolylineAngleStrategy.h"
#include "vtkMath.h"
#include "vtkPointSet.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDecimatePolylineAngleStrategy);

//------------------------------------------------------------------------------
double vtkDecimatePolylineAngleStrategy::ComputeError(
  vtkPointSet* dataset, const vtkIdType originId, const vtkIdType p1Id, const vtkIdType p2Id)
{
  double origin[3], p1[3], p2[3];
  dataset->GetPoint(originId, origin);
  dataset->GetPoint(p1Id, p1);
  dataset->GetPoint(p2Id, p2);

  double u[3] = { 0, 0, 0 };
  vtkMath::Subtract(origin, p1, u);
  double v[3] = { 0, 0, 0 };
  vtkMath::Subtract(origin, p2, v);
  double normUV = vtkMath::Norm(u) * vtkMath::Norm(v);
  double computedError = VTK_DOUBLE_MAX;
  if (normUV > 0)
  {
    computedError = vtkMath::Dot(u, v) / normUV;
  }
  return computedError;
}

VTK_ABI_NAMESPACE_END
