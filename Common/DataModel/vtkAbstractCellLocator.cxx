// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAbstractCellLocator.h"

#include "vtkCellArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkUnstructuredGrid.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkAbstractCellLocator::vtkAbstractCellLocator()
{
  this->CacheCellBounds = 1;
  this->CellBounds = nullptr;
  this->MaxLevel = 8;
  this->Level = 0;
  this->RetainCellLists = 1;
  this->NumberOfCellsPerNode = 32;
  this->UseExistingSearchStructure = 0;
}

//------------------------------------------------------------------------------
vtkAbstractCellLocator::~vtkAbstractCellLocator() = default;

//------------------------------------------------------------------------------
bool vtkAbstractCellLocator::StoreCellBounds()
{
  if (this->CellBounds)
  {
    return false;
  }
  if (!this->DataSet)
  {
    return false;
  }
  // Allocate space for cell bounds storage, then fill
  vtkIdType numCells = this->DataSet->GetNumberOfCells();
  this->CellBoundsSharedPtr = std::make_shared<std::vector<double>>(numCells * 6);
  this->CellBounds = this->CellBoundsSharedPtr->data();

  // This is done to cause non-thread safe initialization to occur due to
  // side effects from GetCellBounds().
  this->DataSet->GetCellBounds(0, &this->CellBounds[0]);

  vtkSMPTools::For(1, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin; cellId < end; cellId++)
      {
        this->DataSet->GetCellBounds(cellId, &this->CellBounds[cellId * 6]);
      }
    });
  return true;
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::FreeCellBounds()
{
  this->CellBoundsSharedPtr.reset();
  this->CellBounds = nullptr;
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::ComputeCellBounds()
{
  if (this->CacheCellBounds)
  {
    this->FreeCellBounds();
    this->StoreCellBounds();
  }
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::UpdateInternalWeights()
{
  if (this->WeightsTime > this->MTime || !this->DataSet)
  {
    return;
  }

  this->Weights.resize(this->DataSet->GetMaxCellSize());
  this->WeightsTime.Modified();
}

//------------------------------------------------------------------------------
bool vtkAbstractCellLocator::IsInBounds(const double bounds[6], const double x[3], double tol)
{
  return (bounds[0] - tol) <= x[0] && x[0] <= (bounds[1] + tol) && (bounds[2] - tol) <= x[1] &&
    x[1] <= (bounds[3] + tol) && (bounds[4] - tol) <= x[2] && x[2] <= (bounds[5] + tol);
}

//------------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId)
{
  vtkIdType cellId = -1;
  return this->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId);
}

//------------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId)
{
  int returnVal;
  returnVal = this->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId, this->GenericCell);
  return returnVal;
}

//------------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double vtkNotUsed(p1)[3],
  const double vtkNotUsed(p2)[3], double vtkNotUsed(tol), double& vtkNotUsed(t),
  double vtkNotUsed(x)[3], double vtkNotUsed(pcoords)[3], int& vtkNotUsed(subId),
  vtkIdType& vtkNotUsed(cellId), vtkGenericCell* vtkNotUsed(cell))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support IntersectWithLine");
  return 0;
}

//------------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double vtkNotUsed(p1)[3],
  const double vtkNotUsed(p2)[3], vtkPoints* vtkNotUsed(points), vtkIdList* vtkNotUsed(cellIds))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support this IntersectWithLine interface");
  return 0;
}

//------------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(
  const double p1[3], const double p2[3], double tol, vtkPoints* points, vtkIdList* cellIds)
{
  return this->IntersectWithLine(p1, p2, tol, points, cellIds, this->GenericCell);
}

//------------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double vtkNotUsed(p1)[3],
  const double vtkNotUsed(p2)[3], double vtkNotUsed(tol), vtkPoints* vtkNotUsed(points),
  vtkIdList* vtkNotUsed(cellIds), vtkGenericCell* vtkNotUsed(cell))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support this IntersectWithLine interface");
  return 0;
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::FindClosestPoint(
  const double x[3], double closestPoint[3], vtkIdType& cellId, int& subId, double& dist2)
{
  this->FindClosestPoint(x, closestPoint, this->GenericCell, cellId, subId, dist2);
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::FindClosestPoint(const double x[3], double closestPoint[3],
  vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2)
{
  int inside;
  double radius = vtkMath::Inf();
  double point[3] = { x[0], x[1], x[2] };
  this->FindClosestPointWithinRadius(
    point, radius, closestPoint, cell, cellId, subId, dist2, inside);
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindClosestPointWithinRadius(double x[3], double radius,
  double closestPoint[3], vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2)
{
  int inside;
  return this->FindClosestPointWithinRadius(
    x, radius, closestPoint, cell, cellId, subId, dist2, inside);
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindClosestPointWithinRadius(
  double x[3], double radius, double closestPoint[3], vtkIdType& cellId, int& subId, double& dist2)
{
  int inside;
  return this->FindClosestPointWithinRadius(
    x, radius, closestPoint, this->GenericCell, cellId, subId, dist2, inside);
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindClosestPointWithinRadius(double vtkNotUsed(x)[3],
  double vtkNotUsed(radius), double vtkNotUsed(closestPoint)[3], vtkGenericCell* vtkNotUsed(cell),
  vtkIdType& vtkNotUsed(cellId), int& vtkNotUsed(subId), double& vtkNotUsed(dist2),
  int& vtkNotUsed(inside))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support FindClosestPointWithinRadius");
  return 0;
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::FindCellsWithinBounds(
  double* vtkNotUsed(bbox), vtkIdList* vtkNotUsed(cells))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support FindCellsWithinBounds");
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::FindCellsAlongLine(
  const double p1[3], const double p2[3], double tolerance, vtkIdList* cells)
{
  this->IntersectWithLine(p1, p2, tolerance, nullptr, cells, nullptr);
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::FindCellsAlongPlane(const double vtkNotUsed(o)[3],
  const double vtkNotUsed(n)[3], double vtkNotUsed(tolerance), vtkIdList* vtkNotUsed(cells))
{
  vtkErrorMacro(<< "The locator " << this->GetClassName()
                << " does not yet support FindCellsAlongPlane");
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindCell(double x[3])
{
  this->UpdateInternalWeights();
  double dist2 = 0, pcoords[3];
  return this->FindCell(x, dist2, this->GenericCell, pcoords, this->Weights.data());
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindCell(
  double x[3], double tol2, vtkGenericCell* GenCell, double pcoords[3], double* weights)
{
  int subId;
  return this->FindCell(x, tol2, GenCell, subId, pcoords, weights);
}

//------------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindCell(
  double x[3], double tol2, vtkGenericCell* GenCell, int& subId, double pcoords[3], double* weights)
{
  vtkIdType returnVal = -1;
  //
  static bool warning_shown = false;
  if (!warning_shown)
  {
    vtkWarningMacro(<< this->GetClassName() << " Does not implement FindCell"
                    << " Reverting to slow DataSet implementation");
    warning_shown = true;
  }
  //
  if (this->DataSet)
  {
    returnVal = this->DataSet->FindCell(x, nullptr, GenCell, 0, tol2, subId, pcoords, weights);
  }
  return returnVal;
}

//------------------------------------------------------------------------------
bool vtkAbstractCellLocator::InsideCellBounds(double x[3], vtkIdType cell_ID)
{
  if (this->CacheCellBounds)
  {
    return vtkAbstractCellLocator::IsInBounds(&this->CellBounds[cell_ID * 6], x);
  }
  else
  {
    double cellBounds[6];
    this->DataSet->GetCellBounds(cell_ID, cellBounds);
    return vtkAbstractCellLocator::IsInBounds(cellBounds, x);
  }
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::GetCellBounds(vtkIdType cellId, double*& cellBoundsPtr)
{
  if (this->CacheCellBounds)
  {
    cellBoundsPtr = &this->CellBounds[cellId * 6];
  }
  else
  {
    this->DataSet->GetCellBounds(cellId, cellBoundsPtr);
  }
}

//------------------------------------------------------------------------------
void vtkAbstractCellLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Cache Cell Bounds: " << this->CacheCellBounds << "\n";
  os << indent << "Retain Cell Lists: " << (this->RetainCellLists ? "On\n" : "Off\n");
  os << indent << "Number of Cells Per Bucket: " << this->NumberOfCellsPerNode << "\n";
}
VTK_ABI_NAMESPACE_END
