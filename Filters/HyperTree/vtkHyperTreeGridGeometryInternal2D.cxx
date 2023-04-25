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
#include "vtkHyperTreeGridGeometryInternal2D.h"

VTK_ABI_NAMESPACE_BEGIN

#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkInternal2D::vtkInternal2D(std::string _trace, bool _merging_points,
  vtkHyperTreeGrid* _input, vtkPoints* _outputPoints, vtkCellArray* _outputCells,
  vtkDataSetAttributes* _inputCellDataAttributes, vtkDataSetAttributes* _outputCellDataAttributes,
  bool _passThroughCellIds, const std::string& _originalCellIdArrayName)
  : vtkInternal(_trace, _merging_points, _input, _outputPoints, _outputCells,
      _inputCellDataAttributes, _outputCellDataAttributes, _passThroughCellIds,
      _originalCellIdArrayName)
{
  TRACE("::" << this->m_trace)
  switch (this->m_input->GetOrientation())
  {
    case 0:               // plane YZ
      this->m_axis_1 = 1; // Y
      this->m_axis_2 = 2; // Z
      break;
    case 1:               // plane XZ
      this->m_axis_1 = 0; // X
      this->m_axis_2 = 2; // Z
      break;
    case 2:               // plane XY
      this->m_axis_1 = 0; // X
      this->m_axis_2 = 1; // Y
      break;
  }
  //
  this->m_cell_points = vtkPoints::New();
  this->m_cell_points->SetNumberOfPoints(4);
  // initialize iterator on HypterTrees (HT) of an HyperTreeGrid (HTG)
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  this->m_input->InitializeTreeIterator(it);
  // index of current HT
  vtkIdType HT_index;
  // non oriented geometry cursor describe one cell on HT
  this->m_cursor = vtkHyperTreeGridNonOrientedGeometryCursor::New();

  if (this->m_hasInterface)
  {
    TRACE("::" << this->m_trace << " HASINTERFACE TRUE")
  }
  else
  {
    TRACE("::" << this->m_trace << " HASINTERFACE FALSE")
  }

  // traversal on HTG for describe a current HT
  while (it.GetNextTree(HT_index))
  {
    TRACE("::" << this->m_trace << " HT_index:" << HT_index)

    // initialize cursor on first cell (root)  of current HT
    this->m_input->InitializeNonOrientedGeometryCursor(this->m_cursor, HT_index);

    this->m_number_of_children = this->m_cursor->GetNumberOfChildren();

    // traversal recursively
    this->recursivelyProcessTree();
  } // it
  TRACE("::" << this->m_trace << " Finish")
  this->m_cell_points->Delete();
  this->m_cell_points = nullptr;
  this->m_cursor->Delete();
  this->m_cursor = nullptr;
  // finish trace
  this->finish();
}

//----------------------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkInternal2D::~vtkInternal2D() {}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal2D::recursivelyProcessTree()
{
  vtkIdType _inputCellIndex = this->m_cursor->GetGlobalNodeIndex();
  TRACE("::" << this->m_trace << " ::recursivelyProcessTree #" << _inputCellIndex << " Level#"
             << this->m_cursor->GetLevel())
  if (this->isMaskedOrGhosted(_inputCellIndex))
  {
    TRACE(
      "::" << this->m_trace << "::recursivelyProcessTree isMaskedOrGhosted #" << _inputCellIndex)
    return;
  }
  // case leaf cell
  if (this->m_cursor->IsLeaf())
  {
    TRACE("::" << this->m_trace << "::recursivelyProcessTree leaf #" << _inputCellIndex)
    if (this->m_hasInterface)
    {
      this->processLeafCellWithInterface(_inputCellIndex);
    }
    else
    {
      this->processLeafCellWithoutInterface(_inputCellIndex);
    }
    return;
  }
  // case coarse cell
  for (unsigned int ichild = 0; ichild < this->m_number_of_children; ++ichild)
  {
    TRACE("::" << this->m_trace << "::recursivelyProcessTree coarse #" << _inputCellIndex << " #"
               << ichild)
    this->m_cursor->ToChild(ichild);
    this->recursivelyProcessTree();
    this->m_cursor->ToParent();
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal2D::processLeafCellWithoutInterface(
  vtkIdType _inputCellIndex)
{
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface BEGIN")
  // case of a cell whose interface is not defined, we copy the entire surface
  double* cell_origin = this->m_cursor->GetOrigin();
  double* cell_size = this->m_cursor->GetSize();
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface Origin [" << cell_origin[0]
             << " ; " << cell_origin[1] << " ; " << cell_origin[2] << "]")
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface Size [" << cell_size[0] << " ; "
             << cell_size[1] << " ; " << cell_size[2] << "]")
  std::vector<double> xyz(3);
  memcpy(xyz.data(), cell_origin, 3 * sizeof(double));
  std::vector<vtkIdType> outputIndexPoints(4);

  outputIndexPoints[0] = this->m_outputPoints->InsertNextPoint(xyz.data());
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface Add [" << xyz[0] << " ; "
             << xyz[1] << " ; " << xyz[2] << "] #" << outputIndexPoints[0])
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface m_outputPoints#"
             << this->m_outputPoints->GetNumberOfPoints())

  TRACE("     axis_1:" << this->m_axis_1)
  xyz[this->m_axis_1] += cell_size[this->m_axis_1];
  outputIndexPoints[1] = this->m_outputPoints->InsertNextPoint(xyz.data());
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface Add [" << xyz[0] << " ; "
             << xyz[1] << " ; " << xyz[2] << "] #" << outputIndexPoints[1])
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface m_outputPoints#"
             << this->m_outputPoints->GetNumberOfPoints())

  TRACE("     axis_2:" << this->m_axis_2)
  xyz[this->m_axis_2] += cell_size[this->m_axis_2];
  outputIndexPoints[2] = this->m_outputPoints->InsertNextPoint(xyz.data());
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface Add [" << xyz[0] << " ; "
             << xyz[1] << " ; " << xyz[2] << "] #" << outputIndexPoints[2])
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface m_outputPoints#"
             << this->m_outputPoints->GetNumberOfPoints())

  TRACE("     axis_1:" << this->m_axis_1)
  xyz[this->m_axis_1] = cell_origin[this->m_axis_1];
  outputIndexPoints[3] = this->m_outputPoints->InsertNextPoint(xyz.data());
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface Add [" << xyz[0] << " ; "
             << xyz[1] << " ; " << xyz[2] << "] #" << outputIndexPoints[3])
  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface m_outputPoints#"
             << this->m_outputPoints->GetNumberOfPoints())

  this->createNewCellAndCopyData(outputIndexPoints, _inputCellIndex);

  TRACE("::" << this->m_trace << "::processLeafCellWithoutInterface END")
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal2D::processLeafCellWithOneInterface(
  vtkIdType _inputCellIndex, double signe, const std::vector<double>& _scalarsInterface)
{
  TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface BEGIN")
  TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface Level#"
             << this->m_cursor->GetLevel())
  std::vector<vtkIdType> outputIndexPoints;
  double xyz_crt[3], xyz_next[3];
  double val_crt, val_next = _scalarsInterface[0];
  for (vtkIdType iPt = 0; iPt < 4; ++iPt)
  {
    // Retrieve vertex coordinates
    this->m_cell_points->GetPoint(iPt, xyz_crt);
    TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface GetPoint iPt#" << iPt << " ["
               << xyz_crt[0] << " ; " << xyz_crt[1] << " ; " << xyz_crt[2] << "]")
    val_crt = val_next;
    vtkIdType niPt = (iPt + 1) % 4;
    val_next = _scalarsInterface[niPt];
    if (signe * val_crt >= 0.)
    {
      outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(xyz_crt));
      TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface Add Crt [" << xyz_crt[0]
                 << " ; " << xyz_crt[1] << " ; " << xyz_crt[2] << "] #"
                 << outputIndexPoints[outputIndexPoints.size() - 1])
    }
    if (val_crt * val_next < 0)
    {
      this->m_cell_points->GetPoint(niPt, xyz_next);
      TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface GetPoint niPt#" << iPt
                 << " [" << xyz_next[0] << " ; " << xyz_next[1] << " ; " << xyz_next[2] << "]")
      double nxyz[3];
      for (int iDim = 0; iDim < 3; ++iDim)
      {
        nxyz[iDim] = (val_next * xyz_crt[iDim] - val_crt * xyz_next[iDim]) / (val_next - val_crt);
        TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface Compute iDim: " << iDim)
        TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface   xyz crt: "
                   << xyz_crt[iDim] << " next: " << xyz_next[iDim])
        TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface   déno: "
                   << (val_next * xyz_crt[iDim] - val_crt * xyz_next[iDim]))
        TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface   val crt: " << val_crt
                   << " next: " << val_next)
        TRACE(
          "::processLeafCellWithOneInterface   val val_next - val_crt: " << (val_next - val_crt))
        TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface   nxyz: " << nxyz[iDim])
      }
      outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz));
      TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface Add New [" << nxyz[0]
                 << " ; " << nxyz[1] << " ; " << nxyz[2] << "] #"
                 << outputIndexPoints[outputIndexPoints.size() - 1])
    }
  }

  this->createNewCellAndCopyData(outputIndexPoints, _inputCellIndex);

  TRACE("::" << this->m_trace << "::processLeafCellWithOneInterface END")
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal2D::processLeafCellWithDoubleInterface(
  vtkIdType _inputCellIndex, const std::vector<double>& _scalarsInterfaceA,
  const std::vector<double>& _scalarsInterfaceB)
{
  TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface")
  TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Level#"
             << this->m_cursor->GetLevel())
  std::vector<vtkIdType> outputIndexPoints;
  double xyz_crt[3], xyz_next[3];
  double val_crt_A, val_next_A = _scalarsInterfaceA[0];
  double val_crt_B, val_next_B = _scalarsInterfaceB[0];
  for (vtkIdType iPt = 0; iPt < 4; ++iPt)
  {
    // Retrieve vertex coordinates
    vtkIdType niPt = (iPt + 1) % 4;
    this->m_cell_points->GetPoint(iPt, xyz_crt);
    this->m_cell_points->GetPoint(niPt, xyz_next);
    val_crt_A = val_next_A;
    val_next_A = _scalarsInterfaceA[niPt];
    val_crt_B = val_next_B;
    val_next_B = _scalarsInterfaceB[niPt];
    bool inside = false;
    if (val_crt_A >= 0 && val_crt_B <= 0)
    {
      outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(xyz_crt));
      TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add Crt [" << xyz_crt[0]
                 << " ; " << xyz_crt[1] << " ; " << xyz_crt[2] << "] #"
                 << outputIndexPoints[outputIndexPoints.size() - 1])
      inside = true;
    }
    //
    bool inversion_A = false;
    double nxyz_A[3];
    if (val_crt_A * val_next_A < 0)
    {
      TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface inversion A")
      inversion_A = true;
      for (int iDim = 0; iDim < 3; ++iDim)
      {
        nxyz_A[iDim] =
          (val_next_A * xyz_crt[iDim] - val_crt_A * xyz_next[iDim]) / (val_next_A - val_crt_A);
      }
      TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface - inter A [" << nxyz_A[0]
                 << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "]")
    }
    //
    bool inversion_B = false;
    double nxyz_B[3];
    if (val_crt_B * val_next_B < 0)
    {
      TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface inversion B")
      inversion_B = true;
      for (int iDim = 0; iDim < 3; ++iDim)
      {
        nxyz_B[iDim] =
          (val_next_B * xyz_crt[iDim] - val_crt_B * xyz_next[iDim]) / (val_next_B - val_crt_B);
      }
      TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface - inter B [" << nxyz_B[0]
                 << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "]")
    }
    //
    if (inversion_A)
    {
      if (inversion_B)
      {
        switch (iPt)
        {
          case 0:
            if (nxyz_A[this->m_axis_1] > nxyz_B[this->m_axis_1])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B ["
                         << nxyz_B[0] << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            else if (nxyz_A[this->m_axis_1] == nxyz_B[this->m_axis_1])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A/B ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            else if (nxyz_A[this->m_axis_1] < nxyz_B[this->m_axis_1])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B ["
                         << nxyz_B[0] << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            break;
          case 1:
            if (nxyz_A[this->m_axis_2] > nxyz_B[this->m_axis_2])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B ["
                         << nxyz_B[0] << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            else if (nxyz_A[this->m_axis_2] == nxyz_B[this->m_axis_2])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A/B ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            else if (nxyz_A[this->m_axis_2] < nxyz_B[this->m_axis_2])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B ["
                         << nxyz_B[0] << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            break;
          case 2:
            if (nxyz_A[this->m_axis_1] < nxyz_B[this->m_axis_1])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B ["
                         << nxyz_B[0] << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            else if (nxyz_A[this->m_axis_1] == nxyz_B[this->m_axis_1])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A/B ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            else if (nxyz_A[this->m_axis_1] > nxyz_B[this->m_axis_1])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B ["
                         << nxyz_B[0] << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            break;
          case 3:
            if (nxyz_A[this->m_axis_2] < nxyz_B[this->m_axis_2])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B ["
                         << nxyz_B[0] << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            else if (nxyz_A[this->m_axis_2] == nxyz_B[this->m_axis_2])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A/B ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            else if (nxyz_A[this->m_axis_2] > nxyz_B[this->m_axis_2])
            {
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                         << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
              outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
              TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B ["
                         << nxyz_B[0] << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                         << outputIndexPoints[outputIndexPoints.size() - 1])
            }
            break;
        }
      }
      else
      {
        outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_A));
        TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New A ["
                   << nxyz_A[0] << " ; " << nxyz_A[1] << " ; " << nxyz_A[2] << "] #"
                   << outputIndexPoints[outputIndexPoints.size() - 1])
      }
    }
    else if (inversion_B)
    {
      outputIndexPoints.emplace_back(this->m_outputPoints->InsertNextPoint(nxyz_B));
      TRACE("::" << this->m_trace << "::processLeafCellWithDoubleInterface Add New B [" << nxyz_B[0]
                 << " ; " << nxyz_B[1] << " ; " << nxyz_B[2] << "] #"
                 << outputIndexPoints[outputIndexPoints.size() - 1])
    }
  }

  this->createNewCellAndCopyData(outputIndexPoints, _inputCellIndex);
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal2D::buildCellPoints()
{
  // Retrieve intercept tuple and type
  double* cell_origin = this->m_cursor->GetOrigin();
  double* cell_size = this->m_cursor->GetSize();
  TRACE("::" << this->m_trace << "::processLeafCellWithInterface Origin [" << cell_origin[0]
             << " ; " << cell_origin[1] << " ; " << cell_origin[2] << "]")
  TRACE("::" << this->m_trace << "::processLeafCellWithInterface Size [" << cell_size[0] << " ; "
             << cell_size[1] << " ; " << cell_size[2] << "]")
  std::vector<double> xyz(3);
  memcpy(xyz.data(), cell_origin, 3 * sizeof(double));
  TRACE("::" << this->m_trace << "::processLeafCellWithInterface xyz[0] [" << xyz[0] << " ; "
             << xyz[1] << " ; " << xyz[2] << "]")
  this->m_cell_points->SetPoint(0, xyz.data());
  xyz[this->m_axis_1] += cell_size[this->m_axis_1];
  TRACE("::" << this->m_trace << "::processLeafCellWithInterface xyz[1] [" << xyz[0] << " ; "
             << xyz[1] << " ; " << xyz[2] << "]")
  this->m_cell_points->SetPoint(1, xyz.data());
  xyz[this->m_axis_2] += cell_size[this->m_axis_2];
  TRACE("::" << this->m_trace << "::processLeafCellWithInterface xyz[2] [" << xyz[0] << " ; "
             << xyz[1] << " ; " << xyz[2] << "]")
  this->m_cell_points->SetPoint(2, xyz.data());
  xyz[this->m_axis_1] = cell_origin[this->m_axis_1];
  TRACE("::" << this->m_trace << "::processLeafCellWithInterface xyz[3] [" << xyz[0] << " ; "
             << xyz[1] << " ; " << xyz[2] << "]")
  this->m_cell_points->SetPoint(3, xyz.data());
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal2D::processLeafCellWithInterface(
  vtkIdType _inputCellIndex)
{
  if (!this->extractCellInterface(_inputCellIndex, false))
  { // case type >= 2, pure cell
    this->processLeafCellWithoutInterface(_inputCellIndex);
    return;
  }

  this->buildCellPoints();
  unsigned int nbPts = this->m_cell_points->GetNumberOfPoints();

  // compute distance point to interface
  std::vector<double> scalarsInterfaceA(nbPts);
  std::vector<double> scalarsInterfaceB(nbPts);
  for (vtkIdType iPt = 0; iPt < nbPts; ++iPt)
  {
    // Retrieve vertex coordinates
    double* xyz = this->m_cell_points->GetPoint(iPt);

    // Set face scalars
    TRACE("this->m_cell_interface_type: " << this->m_cell_interface_type << " =?= 1. "
                                          << (this->m_cell_interface_type != 1.))
    if (this->m_cell_interface_type != 1.)
    {
      scalarsInterfaceA[iPt] = this->computeInterfaceA(xyz);
    }
    TRACE("this->m_cell_interface_type: " << this->m_cell_interface_type << " =?= -1. "
                                          << (this->m_cell_interface_type != -1.))
    if (this->m_cell_interface_type != -1.)
    {
      scalarsInterfaceB[iPt] = this->computeInterfaceB(xyz);
    }
  }
  //
  if (this->m_cell_interface_type == 1)
  { // case juste "rigth" interface with A
    TRACE("1 == this->m_cell_interface_type: " << this->m_cell_interface_type)
    processLeafCellWithOneInterface(_inputCellIndex, -1.0, scalarsInterfaceB);
  }
  else if (!this->m_cell_interface_type)
  { // case intermediate interface with A and B
    TRACE("0 == this->m_cell_interface_type: " << this->m_cell_interface_type)
    processLeafCellWithDoubleInterface(_inputCellIndex, scalarsInterfaceA, scalarsInterfaceB);
  }
  else
  { // case type == -1, case juste "left" interface with B
    TRACE("-1 == this->m_cell_interface_type: " << this->m_cell_interface_type)
    processLeafCellWithOneInterface(_inputCellIndex, 1.0, scalarsInterfaceA);
  }
}

VTK_ABI_NAMESPACE_END
