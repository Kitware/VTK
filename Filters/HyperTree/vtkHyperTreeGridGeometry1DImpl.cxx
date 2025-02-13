// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometry1DImpl.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometry1DImpl::vtkHyperTreeGridGeometry1DImpl(vtkHyperTreeGrid* input,
  vtkPoints* outPoints, vtkCellArray* outCells, vtkDataSetAttributes* inCellDataAttributes,
  vtkDataSetAttributes* outCellDataAttributes, bool passThroughCellIds,
  const std::string& originalCellIdArrayName, bool fillMaterial)
  : vtkHyperTreeGridGeometrySmallDimensionsImpl(input, outPoints, outCells, inCellDataAttributes,
      outCellDataAttributes, passThroughCellIds, originalCellIdArrayName, fillMaterial)
{
  // The orientation value indicates the axis on which the HTG 1D is oriented.
  this->Axis = this->Input->GetOrientation();

  // Cell size : 2 in 1D (segment)
  this->CellPoints->SetNumberOfPoints(2);
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry1DImpl::ProcessLeafCellWithOneInterface(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, double sign,
  const std::vector<double>& distancesToInterface)
{
  // Create storage for endpoint IDs
  std::vector<vtkIdType> outputIndexPoints;

  double xyzCrt[3];
  this->CellPoints->GetPoint(0, xyzCrt);
  double valCrt = distancesToInterface[0];

  double xyzNext[3];
  this->CellPoints->GetPoint(1, xyzNext);
  double valNext = distancesToInterface[1];

  // Retrieve vertex coordinates
  if (sign * valCrt >= 0.)
  {
    outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(xyzCrt));
  }
  if (valCrt * valNext < 0)
  {
    double nxyz[3];
    memcpy(nxyz, xyzCrt, 3 * sizeof(double));
    unsigned int iDim = this->Axis;
    nxyz[iDim] = (valNext * xyzCrt[iDim] - valCrt * xyzNext[iDim]) / (valNext - valCrt);
    outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyz));
  }
  if (sign * valNext >= 0.)
  {
    outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(xyzNext));
  }

  this->CreateNewCellAndCopyData(outputIndexPoints, cursor->GetGlobalNodeIndex());
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry1DImpl::ProcessLeafCellWithDoubleInterface(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
  const std::vector<double>& distancesToInterfaceA,
  const std::vector<double>& distancesToInterfaceB)
{
  // Create storage for endpoint IDs
  std::vector<vtkIdType> outputIndexPoints;

  double xyzCrt[3];
  this->CellPoints->GetPoint(0, xyzCrt);
  double valCrtA = distancesToInterfaceA[0];
  double valCrtB = distancesToInterfaceB[0];

  double xyzNext[3];
  this->CellPoints->GetPoint(1, xyzNext);
  double valNextA = distancesToInterfaceA[1];
  double valNextB = distancesToInterfaceB[1];

  // Retrieve vertex coordinates
  if (valCrtA >= 0 && valCrtB <= 0)
  {
    outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(xyzCrt));
  }

  bool inversionA = false;
  double nxyzA[3];
  if (valCrtA * valNextA < 0)
  {
    memcpy(nxyzA, xyzCrt, 3 * sizeof(double));
    unsigned int iDim = this->Axis;
    nxyzA[iDim] = (valNextA * xyzCrt[iDim] - valCrtA * xyzNext[iDim]) / (valNextA - valCrtA);
    inversionA = true;
  }

  bool inversionB = false;
  double nxyzB[3];
  if (valCrtB * valNextB < 0)
  {
    memcpy(nxyzB, xyzCrt, 3 * sizeof(double));
    unsigned int iDim = this->Axis;
    nxyzB[iDim] = (valNextB * xyzCrt[iDim] - valCrtB * xyzNext[iDim]) / (valNextB - valCrtB);
    inversionB = true;
  }

  if (inversionA)
  {
    if (inversionB)
    {
      if (nxyzA[this->Axis] > nxyzB[this->Axis])
      {
        outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
        outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
      }
      else if (nxyzA[this->Axis] == nxyzB[this->Axis])
      {
        outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
      }
      else if (nxyzA[this->Axis] < nxyzB[this->Axis])
      {
        outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
        outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
      }
    }
    else
    {
      outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
    }
  }
  else if (inversionB)
  {
    outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
  }

  if (valNextA >= 0 && valNextB <= 0)
  {
    outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(xyzNext));
  }

  this->CreateNewCellAndCopyData(outputIndexPoints, cursor->GetGlobalNodeIndex());
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry1DImpl::BuildCellPoints(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Case of a cell whose interface is not defined, we copy the entire surface
  // First endpoint is at origin of cursor
  double* cellOrigin = cursor->GetOrigin();
  this->CellPoints->SetPoint(0, cellOrigin);

  // Second endpoint is at origin of cursor plus its length
  double* cellSize = cursor->GetSize();
  double cellEnd[3];
  memcpy(cellEnd, cellOrigin, 3 * sizeof(double));
  cellEnd[this->Axis] += cellSize[this->Axis];
  this->CellPoints->SetPoint(1, cellEnd);
}

VTK_ABI_NAMESPACE_END
