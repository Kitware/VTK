/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeoemtricLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometricLocator.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <limits>
#include <vector>

#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridGeometricLocator);

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometricLocator::vtkHyperTreeGridGeometricLocator()
  : vtkHyperTreeGridLocator()
  , Bins1D(0)
{
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometricLocator::SetHTG(vtkHyperTreeGrid* cand)
{
  this->Superclass::SetHTG(cand); // classic Set method
  const unsigned int bf = this->HTG->GetBranchFactor();
  this->Bins1D.resize(bf - 1);
  for (unsigned int b = 0; b < bf - 1; b++)
  {
    this->Bins1D[b] = static_cast<double>(b + 1) / static_cast<double>(bf);
  }
} // SetHTG

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::Search(const double point[3])
{
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  return this->Search(point, cursor);
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::Search(
  const double point[3], vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Check bounds
  std::array<unsigned int, 3> bin;
  std::fill(bin.begin(), bin.end(), 0);
  const int dim = this->HTG->GetDimension();
  bin[0] =
    this->HTG->FindDichotomicX(point[0]); // I expect this and subsequent calls to be thread safe
  if (dim > 1)
  {
    bin[1] = this->HTG->FindDichotomicY(point[1]);
  }
  if (dim > 2)
  {
    bin[2] = this->HTG->FindDichotomicZ(point[2]);
  }
  const unsigned int* cellDims = this->HTG->GetCellDims();
  for (int i = 0; i < dim; i++)
  {
    if (bin[i] >= cellDims[i])
    {
      return -1;
    }
  }
  // Get the index of the tree it's in
  vtkIdType treeId;
  this->HTG->GetIndexFromLevelZeroCoordinates(treeId, bin[0], bin[1], bin[2]);
  // Create cursor for looking for the point
  this->HTG->InitializeNonOrientedGeometryCursor(cursor, treeId, false);
  // recurse
  return this->RecursiveSearch(cursor, point);
} // Search

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::RecursiveSearch(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, const double pt[3])
{
  if (cursor->IsMasked())
  {
    return -1;
  }
  if (this->CheckLeafOrChildrenMasked(cursor))
  {
    return cursor->GetGlobalNodeIndex();
  }
  const unsigned int dim = this->HTG->GetDimension();
  const unsigned int bf = this->HTG->GetBranchFactor();
  const double* origin = cursor->GetOrigin();
  if (origin == nullptr)
  {
    vtkErrorMacro("Cursor has no origin");
    return false;
  }
  const double* size = cursor->GetSize();
  if (size == nullptr)
  {
    vtkErrorMacro("Cursor has no size");
    return false;
  }
  double normalizedPt[3];
  std::copy(pt, pt + 3, normalizedPt);
  for (unsigned int d = 0; d < dim; d++)
  {
    normalizedPt[d] -= origin[d];
    normalizedPt[d] /= size[d];
  }
  vtkIdType childIndex = vtkHyperTreeGridGeometricLocator::FindChildIndex(dim, bf, normalizedPt);
  cursor->ToChild(childIndex);
  return this->RecursiveSearch(cursor, pt);
} // RecursiveSearch

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::FindCell(const double point[3],
  const double vtkNotUsed(tol), vtkGenericCell* cell, int& subId, double pcoords[3],
  double* weights)
{
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  vtkIdType globId = this->Search(point, cursor);
  if (!this->ConstructCell(cursor, cell))
  {
    vtkErrorMacro("Failed to construct cell");
    return -1;
  }
  double dist2;
  if (cell->EvaluatePosition(point, nullptr, subId, pcoords, dist2, weights) == 1)
  {
    return globId;
  }
  return -1;
} // FindCell

//------------------------------------------------------------------------------
int vtkHyperTreeGridGeometricLocator::IntersectWithLine(const double p0[3], const double p[2],
  const double tol, double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId,
  vtkGenericCell* cell)
{
  return 0;
} // IntersectWithLine

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometricLocator::FindChildIndex(
  const unsigned int dim, const unsigned int bf, const double normalizedPt[3]) const
{
  std::vector<int> binPt(dim, -1);
  for (unsigned int d = 0; d < dim; d++)
  {
    binPt[d] = std::distance(
      Bins1D.begin(), std::upper_bound(Bins1D.begin(), Bins1D.end(), normalizedPt[d]));
  }
  // convert tuple index to single index
  vtkIdType childIndex = 0;
  for (unsigned int d = 0; d < dim; d++)
  {
    childIndex *= bf;
    childIndex += binPt[dim - d - 1];
  }
  return childIndex;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometricLocator::CheckLeafOrChildrenMasked(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor) const
{
  if (cursor->IsLeaf())
  {
    return true;
  }
  bool allMasked = false;
  // could optimize here by using a smaller cursor for checking masks of children
  for (unsigned int iChild = 0; iChild < cursor->GetNumberOfChildren(); iChild++)
  {
    cursor->ToChild(iChild);
    allMasked = (cursor->IsMasked());
    cursor->ToParent();
    if (!allMasked)
    {
      break;
    }
  }
  return (allMasked);
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometricLocator::ConstructCell(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkGenericCell* cell) const
{
  if (cell == nullptr)
  {
    vtkErrorMacro("Cell that was passed is nullptr");
    return false;
  }
  const unsigned int dim = this->HTG->GetDimension();
  switch (dim)
  {
    case (1):
      cell->SetCellTypeToLine();
      break;
    case (2):
      cell->SetCellTypeToQuad();
      break;
    case (3):
      cell->SetCellTypeToHexahedron();
      break;
    default:
      vtkErrorMacro("Wrong HyperTreeGrid dimension");
      return false;
  }

  const double* origin = cursor->GetOrigin();
  if (origin == nullptr)
  {
    vtkErrorMacro("Cursor has no origin");
    return false;
  }
  const double* size = cursor->GetSize();
  if (size == nullptr)
  {
    vtkErrorMacro("Cursor has no size");
    return false;
  }

  unsigned int nPoints = std::pow(2, dim);
  for (unsigned int iP = 0; iP < nPoints; iP++)
  {
    cell->PointIds->SetId(iP, iP);
  }

  auto cubePoint = [dim, origin, size](std::bitset<3>& pos, std::vector<double>* cubePt) {
    for (unsigned int d = 0; d < dim; d++)
    {
      cubePt->at(d) = origin[d] + pos[d] * size[d];
    }
  };
  std::vector<double> pt(3, 0.0);
  std::vector<std::bitset<3>> positions(8);
  positions[0] = 000;
  positions[1] = 001;
  positions[2] = 011;
  positions[3] = 010;
  positions[4] = 100;
  positions[5] = 101;
  positions[6] = 111;
  positions[7] = 110;
  for (unsigned int iP = 0; iP < nPoints; iP++)
  {
    cubePoint(positions[iP], &pt);
    cell->Points->SetPoint(iP, pt.data());
  }
  return true;
}
