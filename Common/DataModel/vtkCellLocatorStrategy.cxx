/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocatorStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellLocatorStrategy.h"

#include "vtkAbstractCellLocator.h"
#include "vtkCell.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCellLocatorStrategy);

//----------------------------------------------------------------------------
vtkCellLocatorStrategy::vtkCellLocatorStrategy()
{
  // You may ask, why this OwnsLocator rigamarole. The reason is because the
  // reference counting garbage collecter gets confused when the locator,
  // point set, and strategy are all mixed together; resulting in memory
  // leaks etc.
  this->OwnsLocator = false;
  this->CellLocator = nullptr;
}

//----------------------------------------------------------------------------
vtkCellLocatorStrategy::~vtkCellLocatorStrategy()
{
  if (this->OwnsLocator && this->CellLocator != nullptr)
  {
    this->CellLocator->Delete();
    this->CellLocator = nullptr;
  }
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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
    if (this->CellLocator != nullptr && this->OwnsLocator)
    {
      this->CellLocator->SetDataSet(ps);
      this->CellLocator->BuildLocator();
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
    this->CellLocator = psCL;
    this->OwnsLocator = false;
  }

  this->InitializeTime.Modified();

  return 1;
}

//-----------------------------------------------------------------------------
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
  subId = 0; // The cell locator FindCell API should return subId.
  return this->CellLocator->FindCell(x, tol2, gencell, pcoords, weights);
}

//----------------------------------------------------------------------------
void vtkCellLocatorStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CellLocator: " << this->CellLocator << "\n";
}
