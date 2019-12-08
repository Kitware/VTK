/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractCellLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAbstractCellLocator.h"

#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
vtkAbstractCellLocator::vtkAbstractCellLocator()
{
  this->CacheCellBounds = 0;
  this->CellBounds = nullptr;
  this->MaxLevel = 8;
  this->Level = 0;
  this->RetainCellLists = 1;
  this->NumberOfCellsPerNode = 32;
  this->UseExistingSearchStructure = 0;
  this->LazyEvaluation = 0;
  this->GenericCell = vtkGenericCell::New();
}
//----------------------------------------------------------------------------
vtkAbstractCellLocator::~vtkAbstractCellLocator()
{
  this->GenericCell->Delete();
}
//----------------------------------------------------------------------------
bool vtkAbstractCellLocator::StoreCellBounds()
{
  if (this->CellBounds)
    return false;
  if (!this->DataSet)
    return false;
  // Allocate space for cell bounds storage, then fill
  vtkIdType numCells = this->DataSet->GetNumberOfCells();
  this->CellBounds = new double[numCells][6];
  for (vtkIdType j = 0; j < numCells; j++)
  {
    this->DataSet->GetCellBounds(j, CellBounds[j]);
  }
  return true;
}
//----------------------------------------------------------------------------
void vtkAbstractCellLocator::FreeCellBounds()
{
  delete[] this->CellBounds;
  this->CellBounds = nullptr;
}
//----------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId)
{
  vtkIdType cellId = -1;
  return this->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId);
}
//----------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId)
{
  int returnVal;
  returnVal = this->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId, this->GenericCell);
  return returnVal;
}
//----------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double vtkNotUsed(p1)[3],
  const double vtkNotUsed(p2)[3], double vtkNotUsed(tol), double& vtkNotUsed(t),
  double vtkNotUsed(x)[3], double vtkNotUsed(pcoords)[3], int& vtkNotUsed(subId),
  vtkIdType& vtkNotUsed(cellId), vtkGenericCell* vtkNotUsed(cell))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support IntersectWithLine");
  return 0;
}
//----------------------------------------------------------------------------
int vtkAbstractCellLocator::IntersectWithLine(const double vtkNotUsed(p1)[3],
  const double vtkNotUsed(p2)[3], vtkPoints* vtkNotUsed(points), vtkIdList* vtkNotUsed(cellIds))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support this IntersectWithLine interface");
  return 0;
}
//----------------------------------------------------------------------------
void vtkAbstractCellLocator::FindClosestPoint(
  const double x[3], double closestPoint[3], vtkIdType& cellId, int& subId, double& dist2)
{
  this->FindClosestPoint(x, closestPoint, this->GenericCell, cellId, subId, dist2);
}
//----------------------------------------------------------------------------
void vtkAbstractCellLocator::FindClosestPoint(const double vtkNotUsed(x)[3],
  double vtkNotUsed(closestPoint)[3], vtkGenericCell* vtkNotUsed(cell),
  vtkIdType& vtkNotUsed(cellId), int& vtkNotUsed(subId), double& vtkNotUsed(dist2))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support FindClosestPoint");
}
//----------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindClosestPointWithinRadius(double x[3], double radius,
  double closestPoint[3], vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2)
{
  int inside;
  return this->FindClosestPointWithinRadius(
    x, radius, closestPoint, cell, cellId, subId, dist2, inside);
}
//----------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindClosestPointWithinRadius(
  double x[3], double radius, double closestPoint[3], vtkIdType& cellId, int& subId, double& dist2)
{
  int inside;
  return this->FindClosestPointWithinRadius(
    x, radius, closestPoint, this->GenericCell, cellId, subId, dist2, inside);
}
//----------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindClosestPointWithinRadius(double vtkNotUsed(x)[3],
  double vtkNotUsed(radius), double vtkNotUsed(closestPoint)[3], vtkGenericCell* vtkNotUsed(cell),
  vtkIdType& vtkNotUsed(cellId), int& vtkNotUsed(subId), double& vtkNotUsed(dist2),
  int& vtkNotUsed(inside))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support FindClosestPoint");
  return 0;
}
//----------------------------------------------------------------------------
void vtkAbstractCellLocator::FindCellsWithinBounds(
  double* vtkNotUsed(bbox), vtkIdList* vtkNotUsed(cells))
{
  vtkErrorMacro(<< "The locator class - " << this->GetClassName()
                << " does not yet support FindCellsWithinBounds");
}
//----------------------------------------------------------------------------
void vtkAbstractCellLocator::FindCellsAlongLine(const double vtkNotUsed(p1)[3],
  const double vtkNotUsed(p2)[3], double vtkNotUsed(tolerance), vtkIdList* vtkNotUsed(cells))
{
  vtkErrorMacro(<< "The locator " << this->GetClassName()
                << " does not yet support FindCellsAlongLine");
}
//---------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindCell(double x[3])
{
  //
  double dist2 = 0, pcoords[3], weights[32];
  return this->FindCell(x, dist2, this->GenericCell, pcoords, weights);
}
//----------------------------------------------------------------------------
vtkIdType vtkAbstractCellLocator::FindCell(
  double x[3], double tol2, vtkGenericCell* GenCell, double pcoords[3], double* weights)
{
  vtkIdType returnVal = -1;
  int subId;
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
//----------------------------------------------------------------------------
bool vtkAbstractCellLocator::InsideCellBounds(double x[3], vtkIdType cell_ID)
{
  double cellBounds[6], delta[3] = { 0.0, 0.0, 0.0 };
  if (this->DataSet)
  {
    this->DataSet->GetCellBounds(cell_ID, cellBounds);
    return vtkMath::PointIsWithinBounds(x, cellBounds, delta) != 0;
  }
  return 0;
}
//----------------------------------------------------------------------------
void vtkAbstractCellLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Cache Cell Bounds: " << this->CacheCellBounds << "\n";
  os << indent << "Retain Cell Lists: " << (this->RetainCellLists ? "On\n" : "Off\n");
  os << indent << "Number of Cells Per Bucket: " << this->NumberOfCellsPerNode << "\n";
  os << indent << "UseExistingSearchStructure: " << this->UseExistingSearchStructure << "\n";
  os << indent << "LazyEvaluation: " << this->LazyEvaluation << "\n";
}
//----------------------------------------------------------------------------
