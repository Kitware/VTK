// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometry2DImpl.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkPoints.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometry2DImpl::vtkHyperTreeGridGeometry2DImpl(vtkHyperTreeGrid* input,
  vtkPoints* outPoints, vtkCellArray* outCells, vtkDataSetAttributes* inCellDataAttributes,
  vtkDataSetAttributes* outCellDataAttributes, bool passThroughCellIds,
  const std::string& originalCellIdArrayName, bool fillMaterial)
  : vtkHyperTreeGridGeometrySmallDimensionsImpl(input, outPoints, outCells, inCellDataAttributes,
      outCellDataAttributes, passThroughCellIds, originalCellIdArrayName, fillMaterial)
{
  /**
   * The Orientation value indicates the plane on which the HTG 2D is oriented:
   * - 0 describe a plane YZ (axis1=1 as Y, axis2=2 as Z);
   * - 1 describe a plane XZ (axis1=0 as X, axis2=2 as Z);
   * - 2 describe a plane XY (axis1=0 as X, axis2=1 as Y).
   */
  switch (this->Input->GetOrientation())
  {
    case 0:
      this->Axis1 = 1; // Y
      this->Axis2 = 2; // Z
      break;
    case 1:
      this->Axis1 = 0; // X
      this->Axis2 = 2; // Z
      break;
    case 2:
      this->Axis1 = 0; // X
      this->Axis2 = 1; // Y
      break;
    default:
      vtkErrorWithObjectMacro(
        nullptr, << "Input HTG orientation should be comprised between 0 and 2 !");
      break;
  }

  // Cell size : 4 in 2D (quad)
  this->CellPoints->SetNumberOfPoints(4);
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry2DImpl::ProcessLeafCellWithOneInterface(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor, double sign,
  const std::vector<double>& distancesToInterface)
{
  std::vector<vtkIdType> outputIndexPoints;
  double xyzCrt[3], xyzNext[3];
  double valCrt, valNext = distancesToInterface[0];
  for (vtkIdType iPt = 0; iPt < 4; ++iPt)
  {
    // Retrieve vertex coordinates
    this->CellPoints->GetPoint(iPt, xyzCrt);
    valCrt = valNext;
    vtkIdType niPt = (iPt + 1) % 4;
    valNext = distancesToInterface[niPt];
    if (this->FillMaterial && sign * valCrt >= 0.)
    {
      outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(xyzCrt));
    }
    if (valCrt * valNext < 0)
    {
      this->CellPoints->GetPoint(niPt, xyzNext);
      double nxyz[3];
      for (int iDim = 0; iDim < 3; ++iDim)
      {
        nxyz[iDim] = (valNext * xyzCrt[iDim] - valCrt * xyzNext[iDim]) / (valNext - valCrt);
      }
      outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyz));
    }
  }

  /**
   * XXX: In practice, outputIndexPoints can be empty.
   * This is probably caused by the fact that the interface passes exactly through
   * one of the "corner" points of the cell, but it must be verified.
   * Maximum number of points is 5, if one interface cuts 2 neighbouring edges
   * of the cell.
   */
  if (!outputIndexPoints.empty())
  {
    this->CreateNewCellAndCopyData(outputIndexPoints, cursor->GetGlobalNodeIndex());
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry2DImpl::ProcessLeafCellWithDoubleInterface(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor,
  const std::vector<double>& distancesToInterfaceA,
  const std::vector<double>& distancesToInterfaceB)
{
  std::vector<vtkIdType> outputIndexPoints;
  double xyzCrt[3], xyzNext[3];
  double valCrtA, valNextA = distancesToInterfaceA[0];
  double valCrtB, valNextB = distancesToInterfaceB[0];
  for (vtkIdType iPt = 0; iPt < 4; ++iPt)
  {
    // Retrieve vertex coordinates
    vtkIdType niPt = (iPt + 1) % 4;
    this->CellPoints->GetPoint(iPt, xyzCrt);
    this->CellPoints->GetPoint(niPt, xyzNext);
    valCrtA = valNextA;
    valNextA = distancesToInterfaceA[niPt];
    valCrtB = valNextB;
    valNextB = distancesToInterfaceB[niPt];
    if (valCrtA >= 0 && valCrtB <= 0)
    {
      outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(xyzCrt));
    }
    bool invertA = false;
    double nxyzA[3];
    if (valCrtA * valNextA < 0)
    {
      invertA = true;
      for (int iDim = 0; iDim < 3; ++iDim)
      {
        nxyzA[iDim] = (valNextA * xyzCrt[iDim] - valCrtA * xyzNext[iDim]) / (valNextA - valCrtA);
      }
    }
    bool invertB = false;
    double nxyzB[3];
    if (valCrtB * valNextB < 0)
    {
      invertB = true;
      for (int iDim = 0; iDim < 3; ++iDim)
      {
        nxyzB[iDim] = (valNextB * xyzCrt[iDim] - valCrtB * xyzNext[iDim]) / (valNextB - valCrtB);
      }
    }
    if (invertA)
    {
      if (invertB)
      {
        switch (iPt)
        {
          case 0:
            if (nxyzA[this->Axis1] > nxyzB[this->Axis1])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
            }
            else if (nxyzA[this->Axis1] == nxyzB[this->Axis1])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
            }
            else if (nxyzA[this->Axis1] < nxyzB[this->Axis1])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
            }
            break;
          case 1:
            if (nxyzA[this->Axis2] > nxyzB[this->Axis2])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
            }
            else if (nxyzA[this->Axis2] == nxyzB[this->Axis2])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
            }
            else if (nxyzA[this->Axis2] < nxyzB[this->Axis2])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
            }
            break;
          case 2:
            if (nxyzA[this->Axis1] < nxyzB[this->Axis1])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
            }
            else if (nxyzA[this->Axis1] == nxyzB[this->Axis1])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
            }
            else if (nxyzA[this->Axis1] > nxyzB[this->Axis1])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
            }
            break;
          case 3:
            if (nxyzA[this->Axis2] < nxyzB[this->Axis2])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
            }
            else if (nxyzA[this->Axis2] == nxyzB[this->Axis2])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
            }
            else if (nxyzA[this->Axis2] > nxyzB[this->Axis2])
            {
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
              outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
            }
            break;
        }
      }
      else
      {
        outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzA));
      }
    }
    else if (invertB)
    {
      outputIndexPoints.emplace_back(this->OutPoints->InsertNextPoint(nxyzB));
    }
  }

  /**
   * XXX: In practice, outputIndexPoints can be empty.
   * This is probably caused by the fact that interfaces passes exactly through
   * the "corner" points of the cell, but it must be verified.
   * Maximum number of points is 6, if 2 interfaces cuts 2 neighbouring edges
   * of the cell.
   */
  if (!outputIndexPoints.empty())
  {
    this->CreateNewCellAndCopyData(outputIndexPoints, cursor->GetGlobalNodeIndex());
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry2DImpl::BuildCellPoints(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Retrieve intercept tuple and type
  double* cellOrigin = cursor->GetOrigin();
  double* cellSize = cursor->GetSize();
  std::vector<double> xyz(3);
  memcpy(xyz.data(), cellOrigin, 3 * sizeof(double));
  this->CellPoints->SetPoint(0, xyz.data());
  xyz[this->Axis1] += cellSize[this->Axis1];
  this->CellPoints->SetPoint(1, xyz.data());
  xyz[this->Axis2] += cellSize[this->Axis2];
  this->CellPoints->SetPoint(2, xyz.data());
  xyz[this->Axis1] = cellOrigin[this->Axis1];
  this->CellPoints->SetPoint(3, xyz.data());
}

VTK_ABI_NAMESPACE_END
