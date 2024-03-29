// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEmptyCell.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkEmptyCell);

//------------------------------------------------------------------------------
int vtkEmptyCell::EvaluatePosition(const double vtkNotUsed(x)[3], double closestPoint[3],
  int& subId, double pcoords[3], double& dist2, double vtkNotUsed(weights)[])
{
  pcoords[0] = pcoords[1] = pcoords[2] = -1.0;
  subId = 0;
  if (closestPoint != nullptr)
  {
    closestPoint[0] = closestPoint[1] = closestPoint[2] = 0.0;
    dist2 = -1.0;
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkEmptyCell::EvaluateLocation(int& vtkNotUsed(subId), const double vtkNotUsed(pcoords)[3],
  double x[3], double* vtkNotUsed(weights))
{
  x[0] = x[1] = x[2] = 0.0;
}

//------------------------------------------------------------------------------
int vtkEmptyCell::CellBoundary(
  int vtkNotUsed(subId), const double vtkNotUsed(pcoords)[3], vtkIdList* pts)
{
  pts->Reset();
  return 0;
}

//------------------------------------------------------------------------------
void vtkEmptyCell::Contour(double vtkNotUsed(value), vtkDataArray* vtkNotUsed(cellScalars),
  vtkIncrementalPointLocator* vtkNotUsed(locator), vtkCellArray* vtkNotUsed(verts),
  vtkCellArray* vtkNotUsed(lines), vtkCellArray* vtkNotUsed(polys), vtkPointData* vtkNotUsed(inPd),
  vtkPointData* vtkNotUsed(outPd), vtkCellData* vtkNotUsed(inCd), vtkIdType vtkNotUsed(cellId),
  vtkCellData* vtkNotUsed(outCd))
{
}

//------------------------------------------------------------------------------
// Project point on line. If it lies between 0<=t<=1 and distance off line
// is less than tolerance, intersection detected.
int vtkEmptyCell::IntersectWithLine(const double vtkNotUsed(p1)[3], const double vtkNotUsed(p2)[3],
  double vtkNotUsed(tol), double& vtkNotUsed(t), double vtkNotUsed(x)[3],
  double vtkNotUsed(pcoords)[3], int& vtkNotUsed(subId))
{
  return 0;
}

//------------------------------------------------------------------------------
int vtkEmptyCell::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  ptIds->Reset();
  return 1;
}

//------------------------------------------------------------------------------
void vtkEmptyCell::Derivatives(int vtkNotUsed(subId), const double vtkNotUsed(pcoords)[3],
  const double* vtkNotUsed(values), int vtkNotUsed(dim), double* vtkNotUsed(derivs))
{
}

//------------------------------------------------------------------------------
void vtkEmptyCell::Clip(double vtkNotUsed(value), vtkDataArray* vtkNotUsed(cellScalars),
  vtkIncrementalPointLocator* vtkNotUsed(locator), vtkCellArray* vtkNotUsed(verts),
  vtkPointData* vtkNotUsed(inPD), vtkPointData* vtkNotUsed(outPD), vtkCellData* vtkNotUsed(inCD),
  vtkIdType vtkNotUsed(cellId), vtkCellData* vtkNotUsed(outCD), int vtkNotUsed(insideOut))
{
}

//------------------------------------------------------------------------------
void vtkEmptyCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
