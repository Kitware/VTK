/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGeometryInternal1D.h"

VTK_ABI_NAMESPACE_BEGIN

#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkInternal1D::vtkInternal1D(std::string _trace, bool _merging_points,
  vtkHyperTreeGrid* _input, vtkPoints* _outputPoints, vtkCellArray* _outputCells,
  vtkDataSetAttributes* _inputCellDataAttributes, vtkDataSetAttributes* _outputCellDataAttributes,
  bool _passThroughCellIds, const std::string& _originalCellIdArrayName)
  : vtkInternal2D(_trace, _merging_points, _input, _outputPoints, _outputCells,
      _inputCellDataAttributes, _outputCellDataAttributes, _passThroughCellIds,
      _originalCellIdArrayName)
{
}

//----------------------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkInternal1D::~vtkInternal1D() {}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal1D::processLeafCellWithoutInterface(
  vtkIdType _inputCellIndex)
{
  // Create storage for endpoint IDs
  std::vector<vtkIdType> outputIndexPoints;

  // Case of a cell whose interface is not defined, we copy the entire surface
  // First endpoint is at origin of cursor
  double* cell_origin = this->m_cursor->GetOrigin();
  outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(cell_origin));

  // Second endpoint is at origin of cursor plus its length
  double* cell_size = this->m_cursor->GetSize();
  double cell_end[3];
  memcpy(cell_end, cell_origin, 3 * sizeof(double));
  cell_end[this->m_orientation] += cell_size[this->m_orientation];
  outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(cell_end));

  this->createNewCellAndCopyData(outputIndexPoints, _inputCellIndex);
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal1D::processLeafCellWithOneInterface(
  vtkIdType _inputCellIndex, double signe, const std::vector<double>& _scalarsInterface)
{
  // Create storage for endpoint IDs
  std::vector<vtkIdType> outputIndexPoints;

  double xyz_crt[3];
  this->m_cell_points->GetPoint(0, xyz_crt);
  double val_crt = _scalarsInterface[0];

  double xyz_next[3];
  this->m_cell_points->GetPoint(1, xyz_next);
  double val_next = _scalarsInterface[1];

  // Retrieve vertex coordinates
  if (signe * val_crt >= 0.)
  {
    outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(xyz_crt));
  }
  if (val_crt * val_next < 0)
  {
    double nxyz[3];
    memcpy(nxyz, xyz_crt, 3 * sizeof(double));
    unsigned int iDim = this->m_orientation;
    nxyz[iDim] = (val_next * xyz_crt[iDim] - val_crt * xyz_next[iDim]) / (val_next - val_crt);
    outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz));
  }
  if (signe * val_next >= 0.)
  {
    outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(xyz_next));
  }

  this->createNewCellAndCopyData(outputIndexPoints, _inputCellIndex);
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal1D::processLeafCellWithDoubleInterface(
  vtkIdType _inputCellIndex, const std::vector<double>& _scalarsInterfaceA,
  const std::vector<double>& _scalarsInterfaceB)
{
  // Create storage for endpoint IDs
  std::vector<vtkIdType> outputIndexPoints;

  double xyz_crt[3];
  this->m_cell_points->GetPoint(0, xyz_crt);
  double val_crt_A = _scalarsInterfaceA[0];
  double val_crt_B = _scalarsInterfaceB[0];

  double xyz_next[3];
  this->m_cell_points->GetPoint(1, xyz_next);
  double val_next_A = _scalarsInterfaceA[1];
  double val_next_B = _scalarsInterfaceB[1];

  // Retrieve vertex coordinates
  if (val_crt_A >= 0 && val_crt_B <= 0)
  {
    outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(xyz_crt));
  }

  bool inversion_A = false;
  double nxyz_A[3];
  if (val_crt_A * val_next_A < 0)
  {
    memcpy(nxyz_A, xyz_crt, 3 * sizeof(double));
    unsigned int iDim = this->m_orientation;
    nxyz_A[iDim] =
      (val_next_A * xyz_crt[iDim] - val_crt_A * xyz_next[iDim]) / (val_next_A - val_crt_A);
    inversion_A = true;
  }

  bool inversion_B = false;
  double nxyz_B[3];
  if (val_crt_B * val_next_B < 0)
  {
    memcpy(nxyz_B, xyz_crt, 3 * sizeof(double));
    unsigned int iDim = this->m_orientation;
    nxyz_B[iDim] =
      (val_next_B * xyz_crt[iDim] - val_crt_B * xyz_next[iDim]) / (val_next_B - val_crt_B);
    inversion_B = true;
  }

  if (inversion_A)
  {
    if (inversion_B)
    {
      if (nxyz_A[this->m_orientation] > nxyz_B[this->m_orientation])
      {
        outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
        outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
      }
      else if (nxyz_A[this->m_orientation] == nxyz_B[this->m_orientation])
      {
        outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
      }
      else if (nxyz_A[this->m_orientation] < nxyz_B[this->m_orientation])
      {
        outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
        outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
      }
    }
    else
    {
      outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
    }
  }
  else if (inversion_B)
  {
    outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
  }

  if (val_next_A >= 0 && val_next_B <= 0)
  {
    outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(xyz_next));
  }

  this->createNewCellAndCopyData(outputIndexPoints, _inputCellIndex);
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal1D::buildCellPoints()
{
  // Case of a cell whose interface is not defined, we copy the entire surface
  // First endpoint is at origin of cursor
  double* cell_origin = this->m_cursor->GetOrigin();
  this->m_cell_points->SetPoint(0, cell_origin);

  // Second endpoint is at origin of cursor plus its length
  double* cell_size = this->m_cursor->GetSize();
  double cell_end[3];
  memcpy(cell_end, cell_origin, 3 * sizeof(double));
  cell_end[this->m_orientation] += cell_size[this->m_orientation];
  this->m_cell_points->SetPoint(1, cell_origin);
}

VTK_ABI_NAMESPACE_END
