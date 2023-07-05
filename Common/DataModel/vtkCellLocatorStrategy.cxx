// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellLocatorStrategy.h"

#include "vtkAbstractCellLocator.h"
#include "vtkCell.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellLocatorStrategy);

//------------------------------------------------------------------------------
vtkCellLocatorStrategy::vtkCellLocatorStrategy()
{
  this->CellLocator = nullptr;
}

//------------------------------------------------------------------------------
vtkCellLocatorStrategy::~vtkCellLocatorStrategy()
{
  if (this->OwnsLocator && this->CellLocator != nullptr)
  {
    this->CellLocator->Delete();
    this->CellLocator = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkCellLocatorStrategy::SetCellLocator(vtkAbstractCellLocator* cL)
{
  if (cL != this->CellLocator)
  {
    if (this->CellLocator != nullptr && this->OwnsLocator)
    {
      this->CellLocator->Delete();
    }

    this->CellLocator = cL;

    if (cL != nullptr)
    {
      cL->Register(this);
    }

    this->OwnsLocator = true;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkCellLocatorStrategy::Initialize(vtkPointSet* ps)
{
  // See whether anything has changed. If not, just return.
  if (this->PointSet != nullptr && ps == this->PointSet && this->MTime < this->InitializeTime)
  {
    return 1;
  }

  // Set up the point set; return on failure.
  if (this->Superclass::Initialize(ps) == 0)
  {
    return 0;
  }

  // Use the point set's cell locator preferentially. If no cell locator,
  // then we need to create one. If one is specified here in the strategy,
  // use that. If not, then used the point set's default build cell locator
  // method.
  vtkAbstractCellLocator* psCL = ps->GetCellLocator();
  if (psCL == nullptr)
  {
    if (this->CellLocator != nullptr)
    {
      // only the owner of the locator can change it
      if (this->OwnsLocator)
      {
        this->CellLocator->SetDataSet(ps);
        this->CellLocator->BuildLocator();
      }
    }
    else
    {
      ps->BuildCellLocator();
      psCL = ps->GetCellLocator();
      this->CellLocator = psCL;
      this->OwnsLocator = false;
    }
  }
  else
  {
    if (psCL != this->CellLocator)
    {
      this->CellLocator = psCL;
      this->OwnsLocator = false;
    }
    // ensure that the point-set's locator is up-to-date
    // this should be done only by one thread
    if (!this->IsACopy)
    {
      this->CellLocator->BuildLocator();
    }
  }

  this->InitializeTime.Modified();

  return 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkCellLocatorStrategy::FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell,
  vtkIdType cellId, double tol2, int& subId, double pcoords[3], double* weights)
{
  // If we are given a starting cell, try that.
  if (cell && (cellId >= 0))
  {
    double closestPoint[3];
    double dist2;
    if ((cell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) == 1) &&
      (dist2 <= tol2))
    {
      return cellId;
    }
  }

  // Okay cache miss, try the cell locator
  return this->CellLocator->FindCell(x, tol2, gencell, subId, pcoords, weights);
}

//------------------------------------------------------------------------------
vtkIdType vtkCellLocatorStrategy::FindClosestPointWithinRadius(double x[3], double radius,
  double closestPoint[3], vtkGenericCell* cell, vtkIdType& cellId, int& subId, double& dist2,
  int& inside)
{
  return this->CellLocator->FindClosestPointWithinRadius(
    x, radius, closestPoint, cell, cellId, subId, dist2, inside);
}

//------------------------------------------------------------------------------
bool vtkCellLocatorStrategy::InsideCellBounds(double x[3], vtkIdType cellId)
{
  return this->CellLocator->InsideCellBounds(x, cellId);
}

//------------------------------------------------------------------------------
void vtkCellLocatorStrategy::CopyParameters(vtkFindCellStrategy* from)
{
  this->Superclass::CopyParameters(from);

  if (auto strategy = vtkCellLocatorStrategy::SafeDownCast(from))
  {
    if (strategy->CellLocator)
    {
      this->CellLocator = strategy->CellLocator;
      this->OwnsLocator = false;
    }
  }
}

//------------------------------------------------------------------------------
void vtkCellLocatorStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CellLocator: " << this->CellLocator << "\n";
}
VTK_ABI_NAMESPACE_END
