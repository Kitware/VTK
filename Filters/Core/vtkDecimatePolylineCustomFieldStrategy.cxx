// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDecimatePolylineCustomFieldStrategy.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDecimatePolylineCustomFieldStrategy);

//------------------------------------------------------------------------------
void vtkDecimatePolylineCustomFieldStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Field name : " << this->FieldName << std::endl;
}

//------------------------------------------------------------------------------
double vtkDecimatePolylineCustomFieldStrategy::ComputeError(
  vtkPointSet* dataset, const vtkIdType originId, const vtkIdType p1Id, const vtkIdType p2Id)
{
  double origin[3], p1[3], p2[3];
  dataset->GetPoint(originId, origin);
  dataset->GetPoint(p1Id, p1);
  dataset->GetPoint(p2Id, p2);

  if (dataset->GetPointData() == nullptr ||
    dataset->GetPointData()->GetArray(this->FieldName.c_str()) == nullptr)
  {
    return VTK_DOUBLE_MAX;
  }

  vtkDataArray* fieldArray = dataset->GetPointData()->GetArray(this->FieldName.c_str());
  // Can handle N dimension arrays
  int dimension = fieldArray->GetNumberOfComponents();
  std::vector<double> originData, p1Data, p2Data;
  originData.resize(dimension);
  p1Data.resize(dimension);
  p2Data.resize(dimension);
  fieldArray->GetTuple(originId, originData.data());
  fieldArray->GetTuple(p1Id, p1Data.data());
  fieldArray->GetTuple(p2Id, p2Data.data());

  // Check that all point data values are close enough
  double maxError = 0;
  for (int i = 0; i < dimension; i++)
  {
    double oP1 = std::abs(originData[i] - p1Data[i]);
    double oP2 = std::abs(originData[i] - p2Data[i]);
    double P1P2 = std::abs(p1Data[i] - p2Data[i]);
    double localMax = vtkMath::Max(oP1, oP2);
    localMax = vtkMath::Max(localMax, P1P2);
    maxError = vtkMath::Max(maxError, localMax);
  }
  return maxError;
}

VTK_ABI_NAMESPACE_END
