/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTypeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkCellTypeSource.h"

#include "vtkBezierCurve.h"
#include "vtkBezierHexahedron.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkBezierTetra.h"
#include "vtkBezierTriangle.h"
#include "vtkBezierWedge.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeHexahedron.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTetra.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <map>
typedef std::map<std::pair<vtkIdType, vtkIdType>, vtkIdType> EdgeToPointMap;

vtkStandardNewMacro(vtkCellTypeSource);

namespace
{
const int NumberOf1DCellTypes = 5;
const int OneDCellTypes[NumberOf1DCellTypes] = { VTK_LINE, VTK_QUADRATIC_EDGE, VTK_CUBIC_LINE,
  VTK_LAGRANGE_CURVE, VTK_BEZIER_CURVE };
const int NumberOf2DCellTypes = 8;
const int TwoDCellTypes[NumberOf2DCellTypes] = { VTK_TRIANGLE, VTK_QUAD, VTK_QUADRATIC_TRIANGLE,
  VTK_QUADRATIC_QUAD, VTK_LAGRANGE_TRIANGLE, VTK_LAGRANGE_QUADRILATERAL, VTK_BEZIER_TRIANGLE,
  VTK_BEZIER_QUADRILATERAL };
const int NumberOf3DCellTypes = 16;
const int ThreeDCellTypes[NumberOf3DCellTypes] = { VTK_TETRA, VTK_HEXAHEDRON, VTK_WEDGE,
  VTK_PYRAMID, VTK_PENTAGONAL_PRISM, VTK_HEXAGONAL_PRISM, VTK_QUADRATIC_TETRA,
  VTK_QUADRATIC_HEXAHEDRON, VTK_QUADRATIC_WEDGE, VTK_QUADRATIC_PYRAMID, VTK_LAGRANGE_TETRAHEDRON,
  VTK_LAGRANGE_HEXAHEDRON, VTK_LAGRANGE_WEDGE, VTK_BEZIER_TETRAHEDRON, VTK_BEZIER_HEXAHEDRON,
  VTK_BEZIER_WEDGE };
}

// ----------------------------------------------------------------------------
vtkCellTypeSource::vtkCellTypeSource()
  : CellType(VTK_HEXAHEDRON)
  , CellOrder(3)
  , CompleteQuadraticSimplicialElements(false)
  , OutputPrecision(SINGLE_PRECISION)
  , PolynomialFieldOrder(1)
{
  for (int i = 0; i < 3; i++)
  {
    this->BlocksDimensions[i] = 1;
  }
  this->SetNumberOfInputPorts(0);
}

// ----------------------------------------------------------------------------
void vtkCellTypeSource::SetCellType(int cellType)
{
  if (cellType == this->CellType)
  {
    return;
  }
  for (int i = 0; i < NumberOf1DCellTypes; i++)
  {
    if (cellType == OneDCellTypes[i])
    {
      this->CellType = cellType;
      this->Modified();
      return;
    }
  }
  for (int i = 0; i < NumberOf2DCellTypes; i++)
  {
    if (cellType == TwoDCellTypes[i])
    {
      this->CellType = cellType;
      this->Modified();
      return;
    }
  }
  for (int i = 0; i < NumberOf3DCellTypes; i++)
  {
    if (cellType == ThreeDCellTypes[i])
    {
      this->CellType = cellType;
      this->Modified();
      return;
    }
  }
  vtkWarningMacro("Cell type " << cellType << " not supported");
}

// ----------------------------------------------------------------------------
int vtkCellTypeSource::GetCellDimension()
{
  for (int i = 0; i < NumberOf1DCellTypes; i++)
  {
    if (this->CellType == OneDCellTypes[i])
    {
      return 1;
    }
  }
  for (int i = 0; i < NumberOf2DCellTypes; i++)
  {
    if (this->CellType == TwoDCellTypes[i])
    {
      return 2;
    }
  }
  for (int i = 0; i < NumberOf3DCellTypes; i++)
  {
    if (this->CellType == ThreeDCellTypes[i])
    {
      return 3;
    }
  }
  return -1;
}

// ----------------------------------------------------------------------------
void vtkCellTypeSource::SetBlocksDimensions(int* dims)
{
  for (int i = 0; i < 3; i++)
  {
    if (dims[i] != this->BlocksDimensions[i] && dims[i] > 0)
    {
      this->BlocksDimensions[i] = dims[i];
      this->Modified();
    }
  }
}

// ----------------------------------------------------------------------------
void vtkCellTypeSource::SetBlocksDimensions(int iDim, int jDim, int kDim)
{
  int dims[3] = { iDim, jDim, kDim };
  this->SetBlocksDimensions(dims);
}

// ----------------------------------------------------------------------------
int vtkCellTypeSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  vtkNew<vtkExtentTranslator> extentTranslator;
  int dimension = this->GetCellDimension();
  int wholeExtent[6] = { 0, this->BlocksDimensions[0], 0, 0, 0, 0 };
  if (dimension > 1)
  {
    wholeExtent[3] = this->BlocksDimensions[1];
  }
  if (dimension > 2)
  {
    wholeExtent[5] = this->BlocksDimensions[2];
  }
  int extent[6];
  double bounds[6];
  extentTranslator->PieceToExtentThreadSafe(
    piece, numPieces, 0, wholeExtent, extent, extentTranslator->GetSplitMode(), 0);
  int numberOfPoints = 1;
  for (int i = 0; i < 3; i++)
  {
    if (extent[i * 2 + 1] != extent[i * 2])
    {
      numberOfPoints *= extent[i * 2 + 1] - extent[i * 2] + 1;
    }
    bounds[i * 2] = static_cast<double>(extent[i * 2]);
    bounds[i * 2 + 1] = static_cast<double>(extent[i * 2 + 1]);
  }

  vtkNew<vtkPoints> points;
  vtkNew<vtkMergePoints> locator;
  this->Locator = locator.GetPointer();
  this->Locator->InitPointInsertion(points.GetPointer(), bounds);
  // Set the desired precision for the points in the output.
  if (this->OutputPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    points->SetDataType(VTK_DOUBLE);
  }
  else
  {
    points->SetDataType(VTK_FLOAT);
  }

  points->Allocate(numberOfPoints);
  double coord[3];
  for (int k = extent[4]; k < extent[5] + 1; k++)
  {
    coord[2] = static_cast<double>(k);
    for (int j = extent[2]; j < extent[3] + 1; j++)
    {
      coord[1] = static_cast<double>(j);
      for (int i = extent[0]; i < extent[1] + 1; i++)
      {
        coord[0] = static_cast<double>(i);
        this->Locator->InsertNextPoint(coord);
      }
    }
  }
  output->SetPoints(points);

  switch (this->CellType)
  {
    case VTK_LINE:
    {
      output->Allocate(numberOfPoints - 1);
      for (int i = 0; i < numberOfPoints - 1; i++)
      {
        vtkIdType ids[2] = { i, i + 1 };
        output->InsertNextCell(VTK_LINE, 2, ids);
      }
      break;
    }
    case VTK_QUADRATIC_EDGE:
    {
      output->Allocate(numberOfPoints - 1);
      for (int i = 0; i < numberOfPoints - 1; i++)
      {
        double point1[3], point2[3];
        output->GetPoint(i, point1);
        output->GetPoint(i + 1, point2);
        for (int j = 0; j < 3; j++)
        {
          point1[j] = (point1[j] + point2[j]) * .5;
        }
        vtkIdType midPointId = points->InsertNextPoint(point1);
        vtkIdType ids[3] = { i, i + 1, midPointId };
        output->InsertNextCell(VTK_QUADRATIC_EDGE, 3, ids);
      }
      break;
    }
    case VTK_CUBIC_LINE:
    {
      output->Allocate(numberOfPoints - 1);
      for (int i = 0; i < numberOfPoints - 1; i++)
      {
        double point1[3], point2[3], newPoint1[3], newPoint2[3];
        output->GetPoint(i, point1);
        output->GetPoint(i + 1, point2);
        for (int j = 0; j < 3; j++)
        {
          newPoint1[j] = point1[j] * 2. / 3. + point2[j] / 3.;
          newPoint2[j] = point1[j] / 3. + point2[j] * 2. / 3.;
        }
        vtkIdType newPointId1 = points->InsertNextPoint(newPoint1);
        vtkIdType newPointId2 = points->InsertNextPoint(newPoint2);
        vtkIdType ids[4] = { i, i + 1, newPointId1, newPointId2 };
        output->InsertNextCell(VTK_CUBIC_LINE, 4, ids);
      }
      break;
    }
    case VTK_TRIANGLE:
    {
      this->GenerateTriangles(output, extent);
      break;
    }
    case VTK_QUAD:
    {
      this->GenerateQuads(output, extent);
      break;
    }
    case VTK_QUADRATIC_TRIANGLE:
    {
      this->GenerateQuadraticTriangles(output, extent);
      break;
    }
    case VTK_QUADRATIC_QUAD:
    {
      this->GenerateQuadraticQuads(output, extent);
      break;
    }
    case VTK_TETRA:
    {
      this->GenerateTetras(output, extent);
      break;
    }
    case VTK_HEXAHEDRON:
    {
      this->GenerateHexahedron(output, extent);
      break;
    }
    case VTK_WEDGE:
    {
      this->GenerateWedges(output, extent);
      break;
    }
    case VTK_PYRAMID:
    {
      this->GeneratePyramids(output, extent);
      break;
    }
    case VTK_PENTAGONAL_PRISM:
    {
      this->GeneratePentagonalPrism(output, extent);
      break;
    }
    case VTK_HEXAGONAL_PRISM:
    {
      this->GenerateHexagonalPrism(output, extent);
      break;
    }
    case VTK_QUADRATIC_TETRA:
    {
      this->GenerateQuadraticTetras(output, extent);
      break;
    }
    case VTK_QUADRATIC_HEXAHEDRON:
    {
      this->GenerateQuadraticHexahedron(output, extent);
      break;
    }
    case VTK_QUADRATIC_WEDGE:
    {
      this->GenerateQuadraticWedges(output, extent);
      break;
    }
    case VTK_QUADRATIC_PYRAMID:
    {
      this->GenerateQuadraticPyramids(output, extent);
      break;
    }
    case VTK_LAGRANGE_CURVE:
    {
      this->GenerateLagrangeCurves(output, extent);
      break;
    }
    case VTK_LAGRANGE_TRIANGLE:
    {
      this->GenerateLagrangeTris(output, extent);
      break;
    }
    case VTK_LAGRANGE_QUADRILATERAL:
    {
      this->GenerateLagrangeQuads(output, extent);
      break;
    }
    case VTK_LAGRANGE_TETRAHEDRON:
    {
      this->GenerateLagrangeTets(output, extent);
      break;
    }
    case VTK_LAGRANGE_HEXAHEDRON:
    {
      this->GenerateLagrangeHexes(output, extent);
      break;
    }
    case VTK_LAGRANGE_WEDGE:
    {
      this->GenerateLagrangeWedges(output, extent);
      break;
    }
    case VTK_BEZIER_CURVE:
    {
      this->GenerateBezierCurves(output, extent);
      break;
    }
    case VTK_BEZIER_TRIANGLE:
    {
      this->GenerateBezierTris(output, extent);
      break;
    }
    case VTK_BEZIER_QUADRILATERAL:
    {
      this->GenerateBezierQuads(output, extent);
      break;
    }
    case VTK_BEZIER_TETRAHEDRON:
    {
      this->GenerateBezierTets(output, extent);
      break;
    }
    case VTK_BEZIER_HEXAHEDRON:
    {
      this->GenerateBezierHexes(output, extent);
      break;
    }
    case VTK_BEZIER_WEDGE:
    {
      this->GenerateBezierWedges(output, extent);
      break;
    }
    default:
    {
      vtkWarningMacro("Cell type " << this->CellType << " not supported");
    }
  }

  this->ComputeFields(output);

  this->Locator = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int vtkCellTypeSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateTriangles(vtkUnstructuredGrid* output, int extent[6])
{
  int firstDim = extent[1] - extent[0];
  int secondDim = extent[3] - extent[2];
  output->Allocate(firstDim * secondDim * 2);
  for (int j = 0; j < secondDim; j++)
  {
    for (int i = 0; i < firstDim; i++)
    {
      vtkIdType ids[3] = { i + j * (firstDim + 1), i + 1 + j * (firstDim + 1),
        i + (j + 1) * (firstDim + 1) };
      output->InsertNextCell(VTK_TRIANGLE, 3, ids);
      ids[0] = ids[1];
      ids[1] = i + 1 + (j + 1) * (firstDim + 1);
      output->InsertNextCell(VTK_TRIANGLE, 3, ids);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuads(vtkUnstructuredGrid* output, int extent[6])
{
  int firstDim = extent[1] - extent[0];
  int secondDim = extent[3] - extent[2];
  output->Allocate(firstDim * secondDim);
  for (int j = 0; j < secondDim; j++)
  {
    for (int i = 0; i < firstDim; i++)
    {
      vtkIdType ids[4] = { i + j * (firstDim + 1), i + 1 + j * (firstDim + 1),
        i + 1 + (j + 1) * (firstDim + 1), i + (j + 1) * (firstDim + 1) };
      output->InsertNextCell(VTK_QUAD, 4, ids);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticTriangles(vtkUnstructuredGrid* output, int extent[6])
{
  int firstDim = extent[1] - extent[0];
  int secondDim = extent[3] - extent[2];
  output->Allocate(firstDim * secondDim * 2);
  EdgeToPointMap edgeToPointId;
  for (int j = 0; j < secondDim; j++)
  {
    for (int i = 0; i < firstDim; i++)
    {
      vtkIdType mids[3];
      std::pair<vtkIdType, vtkIdType> horizontalEdge =
        std::make_pair(static_cast<vtkIdType>(i + j * (firstDim + 1)),
          static_cast<vtkIdType>(i + 1 + j * (firstDim + 1)));
      EdgeToPointMap::iterator it = edgeToPointId.find(horizontalEdge);
      if (it == edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(horizontalEdge.first, point1);
        output->GetPoint(horizontalEdge.second, point2);
        for (int k = 0; k < 3; k++)
        {
          point1[k] = (point1[k] + point2[k]) * .5;
        }
        vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[horizontalEdge] = mid;
        mids[0] = mid;
      }
      else
      {
        mids[0] = it->second;
      }
      std::pair<vtkIdType, vtkIdType> verticalEdge =
        std::make_pair(static_cast<vtkIdType>(i + j * (firstDim + 1)),
          static_cast<vtkIdType>(i + (j + 1) * (firstDim + 1)));
      it = edgeToPointId.find(verticalEdge);
      if (it == edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(verticalEdge.first, point1);
        output->GetPoint(verticalEdge.second, point2);
        for (int k = 0; k < 3; k++)
        {
          point1[k] = (point1[k] + point2[k]) * .5;
        }
        vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[verticalEdge] = mid;
        mids[2] = mid;
      }
      else
      {
        mids[2] = it->second;
      }
      // always need to create the point on the diagonal
      double point1[3], point2[3];
      output->GetPoint(i + 1 + j * (firstDim + 1), point1);
      output->GetPoint(i + (j + 1) * (firstDim + 1), point2);
      for (int k = 0; k < 3; k++)
      {
        point1[k] = (point1[k] + point2[k]) * .5;
      }
      vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
      mids[1] = mid;
      vtkIdType cellPoints[6] = { i + j * (firstDim + 1), i + 1 + j * (firstDim + 1),
        i + (j + 1) * (firstDim + 1), mids[0], mids[1], mids[2] };
      output->InsertNextCell(VTK_QUADRATIC_TRIANGLE, 6, cellPoints);
      horizontalEdge = std::make_pair(static_cast<vtkIdType>(i + (j + 1) * (firstDim + 1)),
        static_cast<vtkIdType>(i + 1 + (j + 1) * (firstDim + 1)));
      it = edgeToPointId.find(horizontalEdge);
      if (it == edgeToPointId.end())
      {
        output->GetPoint(horizontalEdge.first, point1);
        output->GetPoint(horizontalEdge.second, point2);
        for (int k = 0; k < 3; k++)
        {
          point1[k] = (point1[k] + point2[k]) * .5;
        }
        mid = output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[horizontalEdge] = mid;
        mids[0] = mid;
      }
      else
      {
        mids[0] = it->second;
      }
      verticalEdge = std::make_pair(static_cast<vtkIdType>(i + 1 + j * (firstDim + 1)),
        static_cast<vtkIdType>(i + 1 + (j + 1) * (firstDim + 1)));
      it = edgeToPointId.find(verticalEdge);
      if (it == edgeToPointId.end())
      {
        output->GetPoint(verticalEdge.first, point1);
        output->GetPoint(verticalEdge.second, point2);
        for (int k = 0; k < 3; k++)
        {
          point1[k] = (point1[k] + point2[k]) * .5;
        }
        mid = output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[verticalEdge] = mid;
        mids[2] = mid;
      }
      else
      {
        mids[2] = it->second;
      }
      vtkIdType cellPoints2[6] = { i + 1 + j * (firstDim + 1), i + 1 + (j + 1) * (firstDim + 1),
        i + (j + 1) * (firstDim + 1), mids[2], mids[0], mids[1] };
      output->InsertNextCell(VTK_QUADRATIC_TRIANGLE, 6, cellPoints2);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticQuads(vtkUnstructuredGrid* output, int extent[6])
{
  int firstDim = extent[1] - extent[0];
  int secondDim = extent[3] - extent[2];
  output->Allocate(firstDim * secondDim);
  EdgeToPointMap edgeToPointId;
  for (int j = 0; j < secondDim; j++)
  {
    for (int i = 0; i < firstDim; i++)
    {
      vtkIdType pointIds[8] = { i + j * (firstDim + 1), i + 1 + j * (firstDim + 1),
        i + 1 + (j + 1) * (firstDim + 1), i + (j + 1) * (firstDim + 1), -1, -1, -1, -1 };
      std::pair<vtkIdType, vtkIdType> bottomEdge = std::make_pair(pointIds[0], pointIds[1]);
      EdgeToPointMap::iterator it = edgeToPointId.find(bottomEdge);
      if (it == edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(bottomEdge.first, point1);
        output->GetPoint(bottomEdge.second, point2);
        for (int k = 0; k < 3; k++)
        {
          point1[k] = (point1[k] + point2[k]) * .5;
        }
        vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[bottomEdge] = mid;
        pointIds[4] = mid;
      }
      else
      {
        pointIds[4] = it->second;
      }
      std::pair<vtkIdType, vtkIdType> rightEdge = std::make_pair(pointIds[1], pointIds[2]);
      it = edgeToPointId.find(rightEdge);
      if (it == edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(rightEdge.first, point1);
        output->GetPoint(rightEdge.second, point2);
        for (int k = 0; k < 3; k++)
        {
          point1[k] = (point1[k] + point2[k]) * .5;
        }
        vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[rightEdge] = mid;
        pointIds[5] = mid;
      }
      else
      {
        pointIds[5] = it->second;
      }
      std::pair<vtkIdType, vtkIdType> topEdge = std::make_pair(pointIds[3], pointIds[2]);
      it = edgeToPointId.find(topEdge);
      if (it == edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(topEdge.first, point1);
        output->GetPoint(topEdge.second, point2);
        for (int k = 0; k < 3; k++)
        {
          point1[k] = (point1[k] + point2[k]) * .5;
        }
        vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[topEdge] = mid;
        pointIds[6] = mid;
      }
      else
      {
        pointIds[6] = it->second;
      }
      std::pair<vtkIdType, vtkIdType> leftEdge = std::make_pair(pointIds[0], pointIds[3]);
      it = edgeToPointId.find(leftEdge);
      if (it == edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(leftEdge.first, point1);
        output->GetPoint(leftEdge.second, point2);
        for (int k = 0; k < 3; k++)
        {
          point1[k] = (point1[k] + point2[k]) * .5;
        }
        vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[leftEdge] = mid;
        pointIds[7] = mid;
      }
      else
      {
        pointIds[7] = it->second;
      }
      output->InsertNextCell(VTK_QUADRATIC_QUAD, 8, pointIds);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateTetras(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim * 5);
  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType hexIds[8] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
        };
        // add in center point
        double point1[3], point2[3];
        output->GetPoint(hexIds[0], point1);
        output->GetPoint(hexIds[6], point2);
        for (int l = 0; l < 3; l++)
        {
          point1[l] = .5 * (point1[l] + point2[l]);
        }
        vtkIdType middlePoint = output->GetPoints()->InsertNextPoint(point1);

        vtkIdType pointIds1[4] = { hexIds[0], hexIds[1], hexIds[2], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds1);
        vtkIdType pointIds2[4] = { hexIds[0], hexIds[2], hexIds[3], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds2);

        vtkIdType pointIds3[4] = { hexIds[6], hexIds[5], hexIds[4], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds3);
        vtkIdType pointIds4[4] = { hexIds[6], hexIds[4], hexIds[7], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds4);

        vtkIdType pointIds5[4] = { hexIds[1], hexIds[5], hexIds[6], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds5);
        vtkIdType pointIds6[4] = { hexIds[1], hexIds[6], hexIds[2], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds6);

        vtkIdType pointIds7[4] = { hexIds[0], hexIds[4], hexIds[5], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds7);
        vtkIdType pointIds8[4] = { hexIds[0], hexIds[5], hexIds[1], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds8);

        vtkIdType pointIds9[4] = { hexIds[0], hexIds[3], hexIds[7], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds9);
        vtkIdType pointIds10[4] = { hexIds[0], hexIds[7], hexIds[4], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds10);

        vtkIdType pointIds11[4] = { hexIds[6], hexIds[7], hexIds[3], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds11);
        vtkIdType pointIds12[4] = { hexIds[6], hexIds[3], hexIds[2], middlePoint };
        output->InsertNextCell(VTK_TETRA, 4, pointIds12);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateHexahedron(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim);

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType hexIds[8] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
        };
        output->InsertNextCell(VTK_HEXAHEDRON, 8, hexIds);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateWedges(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  int xDim = extent[1] - extent[0];
  int yDim = extent[3] - extent[2];
  int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim * 2);

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType wedgeIds[6] = { i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1) };
        output->InsertNextCell(VTK_WEDGE, 6, wedgeIds);
        vtkIdType wedgeIds2[6] = { i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1) };
        output->InsertNextCell(VTK_WEDGE, 6, wedgeIds2);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GeneratePyramids(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  int xDim = extent[1] - extent[0];
  int yDim = extent[3] - extent[2];
  int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim * 6);

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType hexIds[8] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
        };
        // add in center point
        double point1[3], point2[3];
        output->GetPoint(hexIds[0], point1);
        output->GetPoint(hexIds[6], point2);
        for (int l = 0; l < 3; l++)
        {
          point1[l] = .5 * (point1[l] + point2[l]);
        }
        vtkIdType middlePoint = output->GetPoints()->InsertNextPoint(point1);
        vtkIdType pointIds1[5] = { hexIds[0], hexIds[1], hexIds[2], hexIds[3], middlePoint };
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds1);
        vtkIdType pointIds2[5] = { hexIds[6], hexIds[5], hexIds[4], hexIds[7], middlePoint };
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds2);
        vtkIdType pointIds3[5] = { hexIds[1], hexIds[5], hexIds[6], hexIds[2], middlePoint };
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds3);
        vtkIdType pointIds4[5] = { hexIds[0], hexIds[4], hexIds[5], hexIds[1], middlePoint };
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds4);
        vtkIdType pointIds5[5] = { hexIds[0], hexIds[3], hexIds[7], hexIds[4], middlePoint };
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds5);
        vtkIdType pointIds6[5] = { hexIds[6], hexIds[7], hexIds[3], hexIds[2], middlePoint };
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds6);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GeneratePentagonalPrism(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[2][2] = { { 0, 2 }, { 5, 7 } };

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType hexIds[10] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          -1,
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          -1,
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
        };
        int cpt = 0;
        for (int e = 0; e < 10; e++)
        {
          if (hexIds[e] == -1)
          {
            double point1[3], point2[3];
            output->GetPoint(hexIds[edgePairs[cpt][0]], point1);
            output->GetPoint(hexIds[edgePairs[cpt][1]], point2);
            for (int l = 0; l < 3; l++)
            {
              point1[l] = (point1[l] + point2[l]) * .5;
            }
            vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
            hexIds[e] = mid;
            cpt++;
          }
        }
        output->InsertNextCell(VTK_PENTAGONAL_PRISM, 10, hexIds);
      }
    }
  }
}

void vtkCellTypeSource::GenerateHexagonalPrism(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[4][2] = { { 0, 2 }, { 3, 5 }, { 6, 8 }, { 9, 11 } };

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType hexIds[12] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          -1,
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          -1,
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          -1,
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          -1,
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
        };
        int cpt = 0;
        for (int e = 0; e < 12; e++)
        {
          if (hexIds[e] == -1)
          {
            double point1[3], point2[3];
            output->GetPoint(hexIds[edgePairs[cpt][0]], point1);
            output->GetPoint(hexIds[edgePairs[cpt][1]], point2);
            for (int l = 0; l < 3; l++)
            {
              point1[l] = (point1[l] + point2[l]) * .5;
            }
            vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
            hexIds[e] = mid;
            cpt++;
          }
        }
        output->InsertNextCell(VTK_HEXAGONAL_PRISM, 12, hexIds);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticTetras(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim * 5);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[12][6][2] = {
    { { 0, 1 }, { 1, 2 }, { 0, 2 }, { 0, 8 }, { 1, 8 }, { 2, 8 } },
    { { 0, 2 }, { 3, 2 }, { 0, 3 }, { 0, 8 }, { 2, 8 }, { 3, 8 } },
    { { 5, 6 }, { 4, 5 }, { 4, 6 }, { 6, 8 }, { 5, 8 }, { 4, 8 } },
    { { 4, 6 }, { 4, 7 }, { 7, 6 }, { 6, 8 }, { 4, 8 }, { 7, 8 } },
    { { 1, 5 }, { 5, 6 }, { 1, 6 }, { 1, 8 }, { 5, 8 }, { 6, 8 } },
    { { 1, 6 }, { 2, 6 }, { 1, 2 }, { 1, 8 }, { 6, 8 }, { 2, 8 } },
    { { 0, 4 }, { 4, 5 }, { 0, 5 }, { 0, 8 }, { 4, 8 }, { 5, 8 } },
    { { 0, 5 }, { 1, 5 }, { 0, 1 }, { 0, 8 }, { 5, 8 }, { 1, 8 } },
    { { 0, 3 }, { 3, 7 }, { 0, 7 }, { 0, 8 }, { 3, 8 }, { 7, 8 } },
    { { 0, 7 }, { 4, 7 }, { 0, 4 }, { 0, 8 }, { 7, 8 }, { 4, 8 } },
    { { 7, 6 }, { 3, 7 }, { 3, 6 }, { 6, 8 }, { 7, 8 }, { 3, 8 } },
    { { 3, 6 }, { 3, 2 }, { 2, 6 }, { 6, 8 }, { 3, 8 }, { 2, 8 } },
  };

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType hexIds[9] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          -1,
        };

        // add in center point
        double point1[3], point2[3];
        output->GetPoint(hexIds[0], point1);
        output->GetPoint(hexIds[6], point2);
        for (int l = 0; l < 3; l++)
        {
          point1[l] = .5 * (point1[l] + point2[l]);
        }
        hexIds[8] = output->GetPoints()->InsertNextPoint(point1);

        vtkIdType tetraIds[12][10] = { { hexIds[0], hexIds[1], hexIds[2], hexIds[8], -1, -1, -1, -1,
                                         -1, -1 },
          { hexIds[0], hexIds[2], hexIds[3], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[6], hexIds[5], hexIds[4], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[6], hexIds[4], hexIds[7], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[1], hexIds[5], hexIds[6], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[1], hexIds[6], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[0], hexIds[4], hexIds[5], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[0], hexIds[5], hexIds[1], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[0], hexIds[3], hexIds[7], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[0], hexIds[7], hexIds[4], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[6], hexIds[7], hexIds[3], hexIds[8], -1, -1, -1, -1, -1, -1 },
          { hexIds[6], hexIds[3], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1 } };
        for (int c = 0; c < 12; c++)
        {
          for (int e = 0; e < 6; e++)
          {
            std::pair<vtkIdType, vtkIdType> edge =
              std::make_pair(hexIds[edgePairs[c][e][0]], hexIds[edgePairs[c][e][1]]);
            EdgeToPointMap::iterator it = edgeToPointId.find(edge);
            if (it == edgeToPointId.end())
            {
              output->GetPoint(edge.first, point1);
              output->GetPoint(edge.second, point2);
              for (int l = 0; l < 3; l++)
              {
                point1[l] = (point1[l] + point2[l]) * .5;
              }
              vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
              edgeToPointId[edge] = mid;
              tetraIds[c][4 + e] = mid;
            }
            else
            {
              tetraIds[c][4 + e] = it->second;
            }
          }
          output->InsertNextCell(VTK_QUADRATIC_TETRA, 10, tetraIds[c]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticHexahedron(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[12][2] = {
    { 0, 1 },
    { 1, 2 },
    { 3, 2 },
    { 0, 3 },
    { 4, 5 },
    { 5, 6 },
    { 7, 6 },
    { 4, 7 },
    { 0, 4 },
    { 1, 5 },
    { 2, 6 },
    { 3, 7 },
  };

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType hexIds[20] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          -1,
          -1,
          -1,
          -1,
          -1,
          -1,
          -1,
          -1,
          -1,
          -1,
          -1,
          -1,
        };
        for (int e = 0; e < 12; e++)
        {
          std::pair<vtkIdType, vtkIdType> edge =
            std::make_pair(hexIds[edgePairs[e][0]], hexIds[edgePairs[e][1]]);
          EdgeToPointMap::iterator it = edgeToPointId.find(edge);
          if (it == edgeToPointId.end())
          {
            double point1[3], point2[3];
            output->GetPoint(edge.first, point1);
            output->GetPoint(edge.second, point2);
            for (int l = 0; l < 3; l++)
            {
              point1[l] = (point1[l] + point2[l]) * .5;
            }
            vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
            edgeToPointId[edge] = mid;
            hexIds[8 + e] = mid;
          }
          else
          {
            hexIds[8 + e] = it->second;
          }
        }
        output->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, hexIds);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticWedges(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim * 2);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[2][9][2] = {
    {
      { 0, 3 }, { 1, 3 }, { 0, 1 }, //
      { 4, 7 }, { 5, 7 }, { 4, 5 }, //
      { 0, 4 }, { 3, 7 }, { 1, 5 }  //
    },
    {
      { 1, 3 }, { 3, 2 }, { 1, 2 }, //
      { 5, 7 }, { 7, 6 }, { 5, 6 }, //
      { 1, 5 }, { 3, 7 }, { 2, 6 }  //
    },
  };
  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        vtkIdType hexIds[8] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
        };

        vtkIdType wedgeIds[2][15] = { { hexIds[0], hexIds[3], hexIds[1], hexIds[4], hexIds[7],
                                        hexIds[5], -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          { hexIds[1], hexIds[3], hexIds[2], hexIds[5], hexIds[7], hexIds[6], -1, -1, -1, -1, -1,
            -1, -1, -1, -1 } };
        for (int c = 0; c < 2; c++)
        {
          for (int e = 0; e < 9; e++)
          {
            std::pair<vtkIdType, vtkIdType> edge =
              std::make_pair(hexIds[edgePairs[c][e][0]], hexIds[edgePairs[c][e][1]]);
            EdgeToPointMap::iterator it = edgeToPointId.find(edge);
            if (it == edgeToPointId.end())
            {
              double point1[3], point2[3];
              output->GetPoint(edge.first, point1);
              output->GetPoint(edge.second, point2);
              for (int l = 0; l < 3; l++)
              {
                point1[l] = (point1[l] + point2[l]) * .5;
              }
              vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
              edgeToPointId[edge] = mid;
              wedgeIds[c][6 + e] = mid;
            }
            else
            {
              wedgeIds[c][6 + e] = it->second;
            }
          }
          output->InsertNextCell(VTK_QUADRATIC_WEDGE, 15, wedgeIds[c]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticPyramids(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  output->Allocate(xDim * yDim * zDim * 6);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[6][8][2] = {
    {
      { 0, 1 }, { 1, 2 }, { 3, 2 }, { 0, 3 }, //
      { 0, 8 }, { 1, 8 }, { 2, 8 }, { 3, 8 }  //
    },
    {
      { 5, 6 }, { 4, 5 }, { 4, 7 }, { 7, 6 }, //
      { 6, 8 }, { 5, 8 }, { 4, 8 }, { 7, 8 }  //
    },
    {
      { 1, 5 }, { 5, 6 }, { 2, 6 }, { 1, 2 }, //
      { 1, 8 }, { 5, 8 }, { 6, 8 }, { 2, 8 }  //
    },
    {
      { 0, 4 }, { 4, 5 }, { 1, 5 }, { 0, 1 }, //
      { 0, 8 }, { 4, 8 }, { 5, 8 }, { 1, 8 }  //
    },
    {
      { 0, 3 }, { 3, 7 }, { 4, 7 }, { 0, 4 }, //
      { 0, 8 }, { 3, 8 }, { 7, 8 }, { 4, 8 }  //
    },
    {
      { 7, 6 }, { 3, 7 }, { 3, 2 }, { 2, 6 }, //
      { 6, 8 }, { 7, 8 }, { 3, 8 }, { 2, 8 }  //
    },
  };

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        // also add in the middle point id
        vtkIdType hexIds[9] = {
          i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1),
          i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1),
          -1,
        };
        // add in center point
        double point1[3], point2[3];
        output->GetPoint(hexIds[0], point1);
        output->GetPoint(hexIds[6], point2);
        for (int l = 0; l < 3; l++)
        {
          point1[l] = .5 * (point1[l] + point2[l]);
        }
        hexIds[8] = output->GetPoints()->InsertNextPoint(point1);

        vtkIdType pyramidIds[6][13] = {
          { hexIds[0], hexIds[1], hexIds[2], hexIds[3], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1 },
          { hexIds[6], hexIds[5], hexIds[4], hexIds[7], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1 },
          { hexIds[1], hexIds[5], hexIds[6], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1 },
          { hexIds[0], hexIds[4], hexIds[5], hexIds[1], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1 },
          { hexIds[0], hexIds[3], hexIds[7], hexIds[4], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1 },
          { hexIds[6], hexIds[7], hexIds[3], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1 }
        };

        for (int c = 0; c < 6; c++)
        {
          for (int e = 0; e < 8; e++)
          {
            std::pair<vtkIdType, vtkIdType> edge =
              std::make_pair(hexIds[edgePairs[c][e][0]], hexIds[edgePairs[c][e][1]]);
            EdgeToPointMap::iterator it = edgeToPointId.find(edge);
            if (it == edgeToPointId.end())
            {
              output->GetPoint(edge.first, point1);
              output->GetPoint(edge.second, point2);
              for (int l = 0; l < 3; l++)
              {
                point1[l] = (point1[l] + point2[l]) * .5;
              }
              vtkIdType mid = output->GetPoints()->InsertNextPoint(point1);
              edgeToPointId[edge] = mid;
              pyramidIds[c][5 + e] = mid;
            }
            else
            {
              pyramidIds[c][5 + e] = it->second;
            }
          }
          output->InsertNextCell(VTK_QUADRATIC_PYRAMID, 13, pyramidIds[c]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateLagrangeCurves(vtkUnstructuredGrid* output, int extent[6])
{
  vtkPoints* points = output->GetPoints();
  vtkIdType numberOfPoints = points->GetNumberOfPoints();
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  // const int yDim = extent[3]-extent[2];
  // const int zDim = extent[5]-extent[4];
  // Connectivity size = (numCells = xDim * (numPtsPerCell = (order + 1) + /* conn size */ 1))
  output->Allocate(xDim * (this->CellOrder + 2));
  // output->Allocate(numberOfPoints-1);
  std::vector<vtkIdType> conn;
  conn.resize(this->CellOrder + 1);
  for (int i = 0; i < numberOfPoints - 1; ++i)
  {
    vtkVector3d p0, p1, dp, pm;
    output->GetPoint(i, p0.GetData());
    output->GetPoint(i + 1, p1.GetData());
    dp = p1 - p0;
    conn[0] = i;
    conn[1] = i + 1;
    double denom = static_cast<double>(this->CellOrder);
    for (int j = 1; j < this->CellOrder; ++j)
    {
      pm = p0 + (static_cast<double>(j) / denom) * dp;
      vtkIdType innerPointId = points->InsertNextPoint(pm.GetData());
      conn[j + 1] = innerPointId;
    }
    output->InsertNextCell(VTK_LAGRANGE_CURVE, this->CellOrder + 1, &conn[0]);
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateLagrangeTris(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int numCells = (xDim - 1) * (yDim - 1) * 2; // 2 tris per quad
  const int order = this->CellOrder;
  const int numPtsPerCell = ((order + 1) * (order + 2) / 2) +
    ((order == 2 && this->CompleteQuadraticSimplicialElements) ? 1 : 0);
  vtkIdType bary[3]; // barycentric indices
  output->Allocate(numCells * (numPtsPerCell + 1));
  std::vector<vtkIdType> cta;
  std::vector<vtkIdType> ctb;
  cta.resize(numPtsPerCell);
  ctb.resize(numPtsPerCell);
  for (int j = 0; j < yDim; ++j)
  {
    for (int i = 0; i < xDim; ++i)
    {
      cta[0] = i + j * (xDim + 1);       // 0
      cta[1] = i + 1 + j * (xDim + 1);   // 1
      cta[2] = i + (j + 1) * (xDim + 1); // 3

      ctb[0] = i + 1 + (j + 1) * (xDim + 1); // 2
      ctb[1] = i + (j + 1) * (xDim + 1);     // 3
      ctb[2] = i + 1 + j * (xDim + 1);       // 1

      vtkVector3d p0, p1, p2, p3, pm;
      output->GetPoint(cta[0], p0.GetData());
      output->GetPoint(cta[1], p1.GetData());
      output->GetPoint(ctb[0], p2.GetData());
      output->GetPoint(ctb[1], p3.GetData());

      for (int n = 0; n <= order; ++n)
      {
        for (int m = 0; m <= order; ++m)
        {
          if ((m == 0 || m == order) && (n == 0 || n == order))
          { // skip corner points
            continue;
          }
          double r = static_cast<double>(m) / order;
          double s = static_cast<double>(n) / order;
          pm = (1.0 - r) * (p3 * s + p0 * (1.0 - s)) + r * (p2 * s + p1 * (1.0 - s));
          vtkIdType innerPointId;
          this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);

          if (m + n <= order)
          {
            bary[0] = m;
            bary[1] = n;
            bary[2] = order - m - n;
            int ctaidx = vtkLagrangeTriangle::Index(bary, order);
            cta[ctaidx] = innerPointId;
          }
          if (m + n >= order)
          {
            bary[0] = order - m;
            bary[1] = order - n;
            bary[2] = order - bary[0] - bary[1];
            int ctbidx = vtkLagrangeTriangle::Index(bary, order);
            ctb[ctbidx] = innerPointId;
          }
        }
      }
      // Add mid-face point if serendipity elements were requested:
      if (order == 2 && this->CompleteQuadraticSimplicialElements)
      {
        double r, s;
        r = 1. / 3.;
        s = 1. / 3.;
        pm = (1.0 - r) * (p3 * s + p0 * (1.0 - s)) + r * (p2 * s + p1 * (1.0 - s));
        this->Locator->InsertUniquePoint(pm.GetData(), cta[numPtsPerCell - 1]);
        r = 2. / 3.;
        s = 2. / 3.;
        pm = (1.0 - r) * (p3 * s + p0 * (1.0 - s)) + r * (p2 * s + p1 * (1.0 - s));
        this->Locator->InsertUniquePoint(pm.GetData(), ctb[numPtsPerCell - 1]);
      }
      output->InsertNextCell(VTK_LAGRANGE_TRIANGLE, numPtsPerCell, &cta[0]);
      output->InsertNextCell(VTK_LAGRANGE_TRIANGLE, numPtsPerCell, &ctb[0]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateLagrangeQuads(vtkUnstructuredGrid* output, int extent[6])
{
  vtkPoints* points = output->GetPoints();
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int numCells = (xDim - 1) * (yDim - 1);
  const int numPtsPerCell = (this->CellOrder + 1) * (this->CellOrder + 1);
  // Connectivity size = numCells * (numPtsPerCell + 1))
  // numPtsPerCell + 1 because conn doesn't hold number of pts per cell, but output cell array does.
  output->Allocate(numCells * (numPtsPerCell + 1));
  std::vector<vtkIdType> conn;
  conn.resize(numPtsPerCell);
  const int order[2] = { this->CellOrder, this->CellOrder };
  for (int j = 0; j < yDim; ++j)
  {
    for (int i = 0; i < xDim; ++i)
    {
      conn[0] = i + j * (xDim + 1);
      conn[1] = i + 1 + j * (xDim + 1);
      conn[2] = i + 1 + (j + 1) * (xDim + 1);
      conn[3] = i + (j + 1) * (xDim + 1);
      vtkVector3d p0, p1, p2, p3, pm;
      output->GetPoint(conn[0], p0.GetData());
      output->GetPoint(conn[1], p1.GetData());
      output->GetPoint(conn[2], p2.GetData());
      output->GetPoint(conn[3], p3.GetData());

      for (int n = 0; n <= order[1]; ++n)
      {
        for (int m = 0; m <= order[0]; ++m)
        {
          if ((m == 0 || m == order[0]) && (n == 0 || n == order[1]))
          { // skip corner points
            continue;
          }
          int connidx = vtkLagrangeQuadrilateral::PointIndexFromIJK(m, n, order);
          double r = static_cast<double>(m) / order[0];
          double s = static_cast<double>(n) / order[1];
          pm = (1.0 - r) * (p3 * s + p0 * (1.0 - s)) + r * (p2 * s + p1 * (1.0 - s));
          vtkIdType innerPointId = points->InsertNextPoint(pm.GetData());
          conn[connidx] = innerPointId;
        }
      }
      output->InsertNextCell(VTK_LAGRANGE_QUADRILATERAL, numPtsPerCell, &conn[0]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateLagrangeTets(vtkUnstructuredGrid* output, int extent[6])
{
  static const int tetsOfHex[12][4] = {
    { 0, 1, 2, 8 },
    { 0, 2, 3, 8 },
    { 6, 5, 4, 8 },
    { 6, 4, 7, 8 },
    { 1, 5, 6, 8 },
    { 1, 6, 2, 8 },
    { 0, 4, 5, 8 },
    { 0, 5, 1, 8 },
    { 0, 3, 7, 8 },
    { 0, 7, 4, 8 },
    { 6, 7, 3, 8 },
    { 6, 3, 2, 8 },
  };

  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  const int numCells = (xDim - 1) * (yDim - 1) * (zDim - 1);
  const int numPtsPerCell = (this->CellOrder == 2 && this->CompleteQuadraticSimplicialElements)
    ? 15
    : (this->CellOrder + 1) * (this->CellOrder + 2) * (this->CellOrder + 3) / 6;
  const int order[3] = { this->CellOrder, this->CellOrder, this->CellOrder };

  vtkIdType hexIds[9];
  std::vector<vtkIdType> conn;
  conn.resize(numPtsPerCell);

  // Allocate numCells * (numPtsPerCell + 1) because connectivity array doesn't
  // hold number of pts per cell, but output cell array does:
  output->Allocate(numCells * (numPtsPerCell + 1));

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        hexIds[0] = i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1);
        hexIds[1] = i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1);
        hexIds[2] = i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1);
        hexIds[3] = i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1);
        hexIds[4] = i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1);
        hexIds[5] = i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1);
        hexIds[6] = i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1);
        hexIds[7] = i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1);

        vtkVector3d pt[9], pm;
        output->GetPoint(hexIds[0], pt[0].GetData());
        output->GetPoint(hexIds[1], pt[1].GetData());
        output->GetPoint(hexIds[2], pt[2].GetData());
        output->GetPoint(hexIds[3], pt[3].GetData());
        output->GetPoint(hexIds[4], pt[4].GetData());
        output->GetPoint(hexIds[5], pt[5].GetData());
        output->GetPoint(hexIds[6], pt[6].GetData());
        output->GetPoint(hexIds[7], pt[7].GetData());
        // add in center point
        for (int l = 0; l < 3; l++)
        {
          pt[8][l] = .5 * (pt[0][l] + pt[6][l]);
        }
        this->Locator->InsertUniquePoint(pt[8].GetData(), hexIds[8]);

        for (int te = 0; te < 12; ++te)
        {
          vtkVector3d tpts[4];
          vtkIdType innerPointId;

          // Get corners
          for (int ii = 0; ii < 4; ++ii)
          {
            conn[ii] = hexIds[tetsOfHex[te][ii]];
            tpts[ii] = pt[tetsOfHex[te][ii]];
          }
          for (int kk = 0; kk <= order[2]; ++kk)
          {
            double tt = static_cast<double>(kk) / order[2];
            for (int jj = 0; jj <= order[1] - kk; ++jj)
            {
              double ss = static_cast<double>(jj) / order[1];
              for (int ii = 0; ii <= order[0] - jj - kk; ++ii)
              {
                double rr = static_cast<double>(ii) / order[0];
                pm = rr * tpts[1] + ss * tpts[2] + tt * tpts[3] + (1. - rr - ss - tt) * tpts[0];
                vtkIdType ijkl[4] = { ii, jj, kk, order[0] - ii - jj - kk };
                vtkIdType connidx = vtkLagrangeTetra::Index(ijkl, order[0]);
                this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);
                conn[connidx] = innerPointId;
              }
            }
          }
          if (this->CompleteQuadraticSimplicialElements && order[0] == 2)
          { // Add 5 new mid-face+mid-body points
            static const int facePts[4][3] = {
              { 0, 1, 2 },
              { 0, 1, 3 },
              { 1, 2, 3 },
              { 0, 2, 3 },
            };
            for (int extra = 0; extra < 4; ++extra)
            {
              pm = (tpts[facePts[extra][0]] + tpts[facePts[extra][1]] + tpts[facePts[extra][2]]) *
                (1.0 / 3.0);
              this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);
              conn[10 + extra] = innerPointId;
            }
            pm = (tpts[0] + tpts[1] + tpts[2] + tpts[3]) * 0.25;
            this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);
            conn[14] = innerPointId;
          }
          output->InsertNextCell(VTK_LAGRANGE_TETRAHEDRON, numPtsPerCell, &conn[0]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateLagrangeHexes(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  const int numCells = (xDim - 1) * (yDim - 1) * (zDim - 1);
  const int numPtsPerCell = (this->CellOrder + 1) * (this->CellOrder + 1) * (this->CellOrder + 1);
  // Connectivity size = numCells * (numPtsPerCell + 1))
  // numPtsPerCell + 1 because conn doesn't hold number of pts per cell, but output cell array does.
  output->Allocate(numCells * (numPtsPerCell + 1));
  std::vector<vtkIdType> conn;
  conn.resize(numPtsPerCell);
  const int order[3] = { this->CellOrder, this->CellOrder, this->CellOrder };
  for (int k = 0; k < zDim; ++k)
  {
    for (int j = 0; j < yDim; ++j)
    {
      for (int i = 0; i < xDim; ++i)
      {
        conn[0] = i + (j + k * (yDim + 1)) * (xDim + 1);
        conn[1] = i + 1 + (j + k * (yDim + 1)) * (xDim + 1);
        conn[2] = i + 1 + ((j + 1) + k * (yDim + 1)) * (xDim + 1);
        conn[3] = i + ((j + 1) + k * (yDim + 1)) * (xDim + 1);
        conn[4] = i + (j + (k + 1) * (yDim + 1)) * (xDim + 1);
        conn[5] = i + 1 + (j + (k + 1) * (yDim + 1)) * (xDim + 1);
        conn[6] = i + 1 + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1);
        conn[7] = i + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1);

        vtkVector3d p0, p1, p2, p3, p4, p5, p6, p7, pm;
        output->GetPoint(conn[0], p0.GetData());
        output->GetPoint(conn[1], p1.GetData());
        output->GetPoint(conn[2], p2.GetData());
        output->GetPoint(conn[3], p3.GetData());
        output->GetPoint(conn[4], p4.GetData());
        output->GetPoint(conn[5], p5.GetData());
        output->GetPoint(conn[6], p6.GetData());
        output->GetPoint(conn[7], p7.GetData());

        for (int p = 0; p <= order[2]; ++p)
        {
          for (int n = 0; n <= order[1]; ++n)
          {
            for (int m = 0; m <= order[0]; ++m)
            {
              if ((m == 0 || m == order[0]) && (n == 0 || n == order[1]) &&
                (p == 0 || p == order[2]))
              { // skip corner points
                continue;
              }
              int connidx = vtkLagrangeHexahedron::PointIndexFromIJK(m, n, p, order);
              double r = static_cast<double>(m) / order[0];
              double s = static_cast<double>(n) / order[1];
              double t = static_cast<double>(p) / order[2];
              pm = (1.0 - r) *
                  ((p3 * (1.0 - t) + p7 * t) * s + (p0 * (1.0 - t) + p4 * t) * (1.0 - s)) +
                r * ((p2 * (1.0 - t) + p6 * t) * s + (p1 * (1.0 - t) + p5 * t) * (1.0 - s));
              vtkIdType innerPointId;
              this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);
              conn[connidx] = innerPointId;
            }
          }
        }
        output->InsertNextCell(VTK_LAGRANGE_HEXAHEDRON, numPtsPerCell, &conn[0]);
      } // i
    }   // j
  }     // k
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateLagrangeWedges(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  const int numCells = (xDim - 1) * (yDim - 1) * (zDim - 1) * 2; // 2 wedges per hex
  const int numPtsPerCell = (this->CompleteQuadraticSimplicialElements && this->CellOrder == 2)
    ? 21
    : (this->CellOrder + 1) * (this->CellOrder + 1) * (this->CellOrder + 2) / 2;

  // There is some ambiguity about whether or not <order> should be a 3-array
  // containing the order in each cardinal direction or a 4-array that
  // additionally holds the number of points. Since
  // vtkLagrangeWedge::PointIndexFromIJK expects the order to be a 4-array, we
  // use this convention here.
  const int order[4] = { this->CellOrder, this->CellOrder, this->CellOrder, numPtsPerCell };

  output->Allocate(numCells * (numPtsPerCell + 1));
  std::vector<vtkIdType> cta;
  std::vector<vtkIdType> ctb;
  cta.resize(numPtsPerCell);
  ctb.resize(numPtsPerCell);
  for (int k = 0; k < zDim; ++k)
  {
    for (int j = 0; j < yDim; ++j)
    {
      for (int i = 0; i < xDim; ++i)
      {
        cta[0] = i + (j + k * (yDim + 1)) * (xDim + 1);       // 0
        cta[1] = i + 1 + (j + k * (yDim + 1)) * (xDim + 1);   // 1
        cta[2] = i + ((j + 1) + k * (yDim + 1)) * (xDim + 1); // 3

        cta[3] = i + (j + (k + 1) * (yDim + 1)) * (xDim + 1);       // 0
        cta[4] = i + 1 + (j + (k + 1) * (yDim + 1)) * (xDim + 1);   // 1
        cta[5] = i + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1); // 3

        ctb[0] = i + 1 + ((j + 1) + k * (yDim + 1)) * (xDim + 1); // 2
        ctb[1] = i + ((j + 1) + k * (yDim + 1)) * (xDim + 1);     // 3
        ctb[2] = i + 1 + (j + k * (yDim + 1)) * (xDim + 1);       // 1

        ctb[3] = i + 1 + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1); // 2
        ctb[4] = i + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1);     // 3
        ctb[5] = i + 1 + (j + (k + 1) * (yDim + 1)) * (xDim + 1);       // 1

        vtkVector3d pt[8], pm;
        output->GetPoint(cta[0], pt[0].GetData());
        output->GetPoint(cta[1], pt[1].GetData());
        output->GetPoint(ctb[0], pt[2].GetData());
        output->GetPoint(ctb[1], pt[3].GetData());
        output->GetPoint(cta[3], pt[4].GetData());
        output->GetPoint(cta[4], pt[5].GetData());
        output->GetPoint(ctb[3], pt[6].GetData());
        output->GetPoint(ctb[4], pt[7].GetData());

        for (int p = 0; p <= order[2]; ++p)
        {
          for (int n = 0; n <= order[0]; ++n)
          {
            for (int m = 0; m <= order[0]; ++m)
            {
              if ((m == 0 || m == order[0]) && (n == 0 || n == order[0]) &&
                (p == 0 || p == order[2]))
              { // skip corner points
                continue;
              }
              double r = static_cast<double>(m) / order[0];
              double s = static_cast<double>(n) / order[0];
              double t = static_cast<double>(p) / order[2];
              pm = (1.0 - r) *
                  ((pt[3] * (1.0 - t) + pt[7] * t) * s +
                    (pt[0] * (1.0 - t) + pt[4] * t) * (1.0 - s)) +
                r *
                  ((pt[2] * (1.0 - t) + pt[6] * t) * s +
                    (pt[1] * (1.0 - t) + pt[5] * t) * (1.0 - s));
              vtkIdType innerPointId;
              this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);

              if (m + n <= order[0])
              {
                int ctaidx = vtkLagrangeWedge::PointIndexFromIJK(m, n, p, order);
                cta[ctaidx] = innerPointId;
              }
              if (m + n >= order[0])
              {
                int ctbidx =
                  vtkLagrangeWedge::PointIndexFromIJK(order[0] - m, order[0] - n, p, order);
                ctb[ctbidx] = innerPointId;
              }
            }
          }
        }
        if (this->CompleteQuadraticSimplicialElements && this->CellOrder == 2)
        {
          // When present, triangle mid-face nodes should appear before
          // the quadrilateral mid-face nodes. So, shift the 3 quad-face
          // nodes by 2 entries in the connectivity array:
          for (int ii = 0; ii < 3; ++ii)
          {
            cta[19 - ii] = cta[17 - ii];
            ctb[19 - ii] = ctb[17 - ii];
          }
          // Now fill in the "holes" at ct{a,b}[15,16] with tri-face nodes:
          static const int facePts[2][2][3] = {
            { { 0, 1, 3 }, { 4, 5, 7 } }, // cta
            { { 1, 2, 3 }, { 5, 6, 7 } }  // ctb
          };
          vtkVector3d bodyA(0., 0., 0.);
          vtkVector3d bodyB(0., 0., 0.);
          vtkIdType innerA, innerB;
          for (int ii = 0; ii < 2; ++ii)
          {
            vtkVector3d pA =
              (pt[facePts[0][ii][0]] + pt[facePts[0][ii][1]] + pt[facePts[0][ii][2]]) * (1. / 3.);
            vtkVector3d pB =
              (pt[facePts[1][ii][0]] + pt[facePts[1][ii][1]] + pt[facePts[1][ii][2]]) * (1. / 3.);
            bodyA = bodyA + 0.5 * pA;
            bodyB = bodyB + 0.5 * pB;
            this->Locator->InsertUniquePoint(pA.GetData(), innerA);
            this->Locator->InsertUniquePoint(pB.GetData(), innerB);
            cta[15 + ii] = innerA;
            ctb[15 + ii] = innerB;
          }
          // Finally, add a body-centered node to cta and ctb:
          this->Locator->InsertUniquePoint(bodyA.GetData(), innerA);
          this->Locator->InsertUniquePoint(bodyB.GetData(), innerB);
          cta[20] = innerA;
          ctb[20] = innerB;
        }
        output->InsertNextCell(VTK_LAGRANGE_WEDGE, numPtsPerCell, &cta[0]);
        output->InsertNextCell(VTK_LAGRANGE_WEDGE, numPtsPerCell, &ctb[0]);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateBezierCurves(vtkUnstructuredGrid* output, int extent[6])
{
  vtkPoints* points = output->GetPoints();
  vtkIdType numberOfPoints = points->GetNumberOfPoints();
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  // const int yDim = extent[3]-extent[2];
  // const int zDim = extent[5]-extent[4];
  // Connectivity size = (numCells = xDim * (numPtsPerCell = (order + 1) + /* conn size */ 1))
  output->Allocate(xDim * (this->CellOrder + 2));
  // output->Allocate(numberOfPoints-1);
  std::vector<vtkIdType> conn;
  conn.resize(this->CellOrder + 1);
  for (int i = 0; i < numberOfPoints - 1; ++i)
  {
    vtkVector3d p0, p1, dp, pm;
    output->GetPoint(i, p0.GetData());
    output->GetPoint(i + 1, p1.GetData());
    dp = p1 - p0;
    conn[0] = i;
    conn[1] = i + 1;
    double denom = static_cast<double>(this->CellOrder);
    for (int j = 1; j < this->CellOrder; ++j)
    {
      pm = p0 + (static_cast<double>(j) / denom) * dp;
      vtkIdType innerPointId = points->InsertNextPoint(pm.GetData());
      conn[j + 1] = innerPointId;
    }
    output->InsertNextCell(VTK_BEZIER_CURVE, this->CellOrder + 1, &conn[0]);
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateBezierTris(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int numCells = (xDim - 1) * (yDim - 1) * 2; // 2 tris per quad
  const int order = this->CellOrder;
  const int numPtsPerCell = ((order + 1) * (order + 2) / 2) +
    ((order == 2 && this->CompleteQuadraticSimplicialElements) ? 1 : 0);
  vtkIdType bary[3]; // barycentric indices
  output->Allocate(numCells * (numPtsPerCell + 1));
  std::vector<vtkIdType> cta;
  std::vector<vtkIdType> ctb;
  cta.resize(numPtsPerCell);
  ctb.resize(numPtsPerCell);
  for (int j = 0; j < yDim; ++j)
  {
    for (int i = 0; i < xDim; ++i)
    {
      cta[0] = i + j * (xDim + 1);       // 0
      cta[1] = i + 1 + j * (xDim + 1);   // 1
      cta[2] = i + (j + 1) * (xDim + 1); // 3

      ctb[0] = i + 1 + (j + 1) * (xDim + 1); // 2
      ctb[1] = i + (j + 1) * (xDim + 1);     // 3
      ctb[2] = i + 1 + j * (xDim + 1);       // 1

      vtkVector3d p0, p1, p2, p3, pm;
      output->GetPoint(cta[0], p0.GetData());
      output->GetPoint(cta[1], p1.GetData());
      output->GetPoint(ctb[0], p2.GetData());
      output->GetPoint(ctb[1], p3.GetData());

      for (int n = 0; n <= order; ++n)
      {
        for (int m = 0; m <= order; ++m)
        {
          if ((m == 0 || m == order) && (n == 0 || n == order))
          { // skip corner points
            continue;
          }
          double r = static_cast<double>(m) / order;
          double s = static_cast<double>(n) / order;
          pm = (1.0 - r) * (p3 * s + p0 * (1.0 - s)) + r * (p2 * s + p1 * (1.0 - s));
          vtkIdType innerPointId;
          this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);

          if (m + n <= order)
          {
            bary[0] = m;
            bary[1] = n;
            bary[2] = order - m - n;
            int ctaidx = vtkBezierTriangle::Index(bary, order);
            cta[ctaidx] = innerPointId;
          }
          if (m + n >= order)
          {
            bary[0] = order - m;
            bary[1] = order - n;
            bary[2] = order - bary[0] - bary[1];
            int ctbidx = vtkBezierTriangle::Index(bary, order);
            ctb[ctbidx] = innerPointId;
          }
        }
      }
      // Add mid-face point if serendipity elements were requested:
      if (order == 2 && this->CompleteQuadraticSimplicialElements)
      {
        double r, s;
        r = 1. / 3.;
        s = 1. / 3.;
        pm = (1.0 - r) * (p3 * s + p0 * (1.0 - s)) + r * (p2 * s + p1 * (1.0 - s));
        this->Locator->InsertUniquePoint(pm.GetData(), cta[numPtsPerCell - 1]);
        r = 2. / 3.;
        s = 2. / 3.;
        pm = (1.0 - r) * (p3 * s + p0 * (1.0 - s)) + r * (p2 * s + p1 * (1.0 - s));
        this->Locator->InsertUniquePoint(pm.GetData(), ctb[numPtsPerCell - 1]);
      }
      output->InsertNextCell(VTK_BEZIER_TRIANGLE, numPtsPerCell, &cta[0]);
      output->InsertNextCell(VTK_BEZIER_TRIANGLE, numPtsPerCell, &ctb[0]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateBezierQuads(vtkUnstructuredGrid* output, int extent[6])
{
  vtkPoints* points = output->GetPoints();
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int numCells = (xDim - 1) * (yDim - 1);
  const int numPtsPerCell = (this->CellOrder + 1) * (this->CellOrder + 1);
  // Connectivity size = numCells * (numPtsPerCell + 1))
  // numPtsPerCell + 1 because conn doesn't hold number of pts per cell, but output cell array does.
  output->Allocate(numCells * (numPtsPerCell + 1));
  std::vector<vtkIdType> conn;
  conn.resize(numPtsPerCell);
  const int order[2] = { this->CellOrder, this->CellOrder };
  for (int j = 0; j < yDim; ++j)
  {
    for (int i = 0; i < xDim; ++i)
    {
      conn[0] = i + j * (xDim + 1);
      conn[1] = i + 1 + j * (xDim + 1);
      conn[2] = i + 1 + (j + 1) * (xDim + 1);
      conn[3] = i + (j + 1) * (xDim + 1);
      vtkVector3d p0, p1, p2, p3, pm;
      output->GetPoint(conn[0], p0.GetData());
      output->GetPoint(conn[1], p1.GetData());
      output->GetPoint(conn[2], p2.GetData());
      output->GetPoint(conn[3], p3.GetData());

      for (int n = 0; n <= order[1]; ++n)
      {
        for (int m = 0; m <= order[0]; ++m)
        {
          if ((m == 0 || m == order[0]) && (n == 0 || n == order[1]))
          { // skip corner points
            continue;
          }
          int connidx = vtkBezierQuadrilateral::PointIndexFromIJK(m, n, order);
          double r = static_cast<double>(m) / order[0];
          double s = static_cast<double>(n) / order[1];
          pm = (1.0 - r) * (p3 * s + p0 * (1.0 - s)) + r * (p2 * s + p1 * (1.0 - s));
          vtkIdType innerPointId = points->InsertNextPoint(pm.GetData());
          conn[connidx] = innerPointId;
        }
      }
      output->InsertNextCell(VTK_BEZIER_QUADRILATERAL, numPtsPerCell, &conn[0]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateBezierTets(vtkUnstructuredGrid* output, int extent[6])
{
  static const int tetsOfHex[12][4] = {
    { 0, 1, 2, 8 },
    { 0, 2, 3, 8 },
    { 6, 5, 4, 8 },
    { 6, 4, 7, 8 },
    { 1, 5, 6, 8 },
    { 1, 6, 2, 8 },
    { 0, 4, 5, 8 },
    { 0, 5, 1, 8 },
    { 0, 3, 7, 8 },
    { 0, 7, 4, 8 },
    { 6, 7, 3, 8 },
    { 6, 3, 2, 8 },
  };

  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  const int numCells = (xDim - 1) * (yDim - 1) * (zDim - 1);
  const int numPtsPerCell = (this->CellOrder == 2 && this->CompleteQuadraticSimplicialElements)
    ? 15
    : (this->CellOrder + 1) * (this->CellOrder + 2) * (this->CellOrder + 3) / 6;
  const int order[3] = { this->CellOrder, this->CellOrder, this->CellOrder };

  vtkIdType hexIds[9];
  std::vector<vtkIdType> conn;
  conn.resize(numPtsPerCell);

  // Allocate numCells * (numPtsPerCell + 1) because connectivity array doesn't
  // hold number of pts per cell, but output cell array does:
  output->Allocate(numCells * (numPtsPerCell + 1));

  for (int k = 0; k < zDim; k++)
  {
    for (int j = 0; j < yDim; j++)
    {
      for (int i = 0; i < xDim; i++)
      {
        hexIds[0] = i + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1);
        hexIds[1] = i + 1 + j * (xDim + 1) + k * (xDim + 1) * (yDim + 1);
        hexIds[2] = i + 1 + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1);
        hexIds[3] = i + (j + 1) * (xDim + 1) + k * (xDim + 1) * (yDim + 1);
        hexIds[4] = i + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1);
        hexIds[5] = i + 1 + j * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1);
        hexIds[6] = i + 1 + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1);
        hexIds[7] = i + (j + 1) * (xDim + 1) + (k + 1) * (xDim + 1) * (yDim + 1);

        vtkVector3d pt[9], pm;
        output->GetPoint(hexIds[0], pt[0].GetData());
        output->GetPoint(hexIds[1], pt[1].GetData());
        output->GetPoint(hexIds[2], pt[2].GetData());
        output->GetPoint(hexIds[3], pt[3].GetData());
        output->GetPoint(hexIds[4], pt[4].GetData());
        output->GetPoint(hexIds[5], pt[5].GetData());
        output->GetPoint(hexIds[6], pt[6].GetData());
        output->GetPoint(hexIds[7], pt[7].GetData());
        // add in center point
        for (int l = 0; l < 3; l++)
        {
          pt[8][l] = .5 * (pt[0][l] + pt[6][l]);
        }
        this->Locator->InsertUniquePoint(pt[8].GetData(), hexIds[8]);

        for (int te = 0; te < 12; ++te)
        {
          vtkVector3d tpts[4];
          vtkIdType innerPointId;

          // Get corners
          for (int ii = 0; ii < 4; ++ii)
          {
            conn[ii] = hexIds[tetsOfHex[te][ii]];
            tpts[ii] = pt[tetsOfHex[te][ii]];
          }
          for (int kk = 0; kk <= order[2]; ++kk)
          {
            double tt = static_cast<double>(kk) / order[2];
            for (int jj = 0; jj <= order[1] - kk; ++jj)
            {
              double ss = static_cast<double>(jj) / order[1];
              for (int ii = 0; ii <= order[0] - jj - kk; ++ii)
              {
                double rr = static_cast<double>(ii) / order[0];
                pm = rr * tpts[1] + ss * tpts[2] + tt * tpts[3] + (1. - rr - ss - tt) * tpts[0];
                vtkIdType ijkl[4] = { ii, jj, kk, order[0] - ii - jj - kk };
                vtkIdType connidx = vtkBezierTetra::Index(ijkl, order[0]);
                this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);
                conn[connidx] = innerPointId;
              }
            }
          }
          if (this->CompleteQuadraticSimplicialElements && order[0] == 2)
          { // Add 5 new mid-face+mid-body points
            static const int facePts[4][3] = {
              { 0, 1, 2 },
              { 0, 1, 3 },
              { 1, 2, 3 },
              { 0, 2, 3 },
            };
            for (int extra = 0; extra < 4; ++extra)
            {
              pm = (tpts[facePts[extra][0]] + tpts[facePts[extra][1]] + tpts[facePts[extra][2]]) *
                (1.0 / 3.0);
              this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);
              conn[10 + extra] = innerPointId;
            }
            pm = (tpts[0] + tpts[1] + tpts[2] + tpts[3]) * 0.25;
            this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);
            conn[14] = innerPointId;
          }
          output->InsertNextCell(VTK_BEZIER_TETRAHEDRON, numPtsPerCell, &conn[0]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateBezierHexes(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  const int numCells = (xDim - 1) * (yDim - 1) * (zDim - 1);
  const int numPtsPerCell = (this->CellOrder + 1) * (this->CellOrder + 1) * (this->CellOrder + 1);
  // Connectivity size = numCells * (numPtsPerCell + 1))
  // numPtsPerCell + 1 because conn doesn't hold number of pts per cell, but output cell array does.
  output->Allocate(numCells * (numPtsPerCell + 1));
  std::vector<vtkIdType> conn;
  conn.resize(numPtsPerCell);
  const int order[3] = { this->CellOrder, this->CellOrder, this->CellOrder };
  for (int k = 0; k < zDim; ++k)
  {
    for (int j = 0; j < yDim; ++j)
    {
      for (int i = 0; i < xDim; ++i)
      {
        conn[0] = i + (j + k * (yDim + 1)) * (xDim + 1);
        conn[1] = i + 1 + (j + k * (yDim + 1)) * (xDim + 1);
        conn[2] = i + 1 + ((j + 1) + k * (yDim + 1)) * (xDim + 1);
        conn[3] = i + ((j + 1) + k * (yDim + 1)) * (xDim + 1);
        conn[4] = i + (j + (k + 1) * (yDim + 1)) * (xDim + 1);
        conn[5] = i + 1 + (j + (k + 1) * (yDim + 1)) * (xDim + 1);
        conn[6] = i + 1 + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1);
        conn[7] = i + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1);

        vtkVector3d p0, p1, p2, p3, p4, p5, p6, p7, pm;
        output->GetPoint(conn[0], p0.GetData());
        output->GetPoint(conn[1], p1.GetData());
        output->GetPoint(conn[2], p2.GetData());
        output->GetPoint(conn[3], p3.GetData());
        output->GetPoint(conn[4], p4.GetData());
        output->GetPoint(conn[5], p5.GetData());
        output->GetPoint(conn[6], p6.GetData());
        output->GetPoint(conn[7], p7.GetData());

        for (int p = 0; p <= order[2]; ++p)
        {
          for (int n = 0; n <= order[1]; ++n)
          {
            for (int m = 0; m <= order[0]; ++m)
            {
              if ((m == 0 || m == order[0]) && (n == 0 || n == order[1]) &&
                (p == 0 || p == order[2]))
              { // skip corner points
                continue;
              }
              int connidx = vtkBezierHexahedron::PointIndexFromIJK(m, n, p, order);
              double r = static_cast<double>(m) / order[0];
              double s = static_cast<double>(n) / order[1];
              double t = static_cast<double>(p) / order[2];
              pm = (1.0 - r) *
                  ((p3 * (1.0 - t) + p7 * t) * s + (p0 * (1.0 - t) + p4 * t) * (1.0 - s)) +
                r * ((p2 * (1.0 - t) + p6 * t) * s + (p1 * (1.0 - t) + p5 * t) * (1.0 - s));
              vtkIdType innerPointId;
              this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);
              conn[connidx] = innerPointId;
            }
          }
        }
        output->InsertNextCell(VTK_BEZIER_HEXAHEDRON, numPtsPerCell, &conn[0]);
      } // i
    }   // j
  }     // k
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateBezierWedges(vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1] - extent[0];
  const int yDim = extent[3] - extent[2];
  const int zDim = extent[5] - extent[4];
  const int numCells = (xDim - 1) * (yDim - 1) * (zDim - 1) * 2; // 2 wedges per hex
  const int numPtsPerCell = (this->CompleteQuadraticSimplicialElements && this->CellOrder == 2)
    ? 21
    : (this->CellOrder + 1) * (this->CellOrder + 1) * (this->CellOrder + 2) / 2;

  // There is some ambiguity about whether or not <order> should be a 3-array
  // containing the order in each cardinal direction or a 4-array that
  // additionally holds the number of points. Since
  // vtkBezierWedge::PointIndexFromIJK expects the order to be a 4-array, we
  // use this convention here.
  const int order[4] = { this->CellOrder, this->CellOrder, this->CellOrder, numPtsPerCell };

  output->Allocate(numCells * (numPtsPerCell + 1));
  std::vector<vtkIdType> cta;
  std::vector<vtkIdType> ctb;
  cta.resize(numPtsPerCell);
  ctb.resize(numPtsPerCell);
  for (int k = 0; k < zDim; ++k)
  {
    for (int j = 0; j < yDim; ++j)
    {
      for (int i = 0; i < xDim; ++i)
      {
        cta[0] = i + (j + k * (yDim + 1)) * (xDim + 1);       // 0
        cta[1] = i + 1 + (j + k * (yDim + 1)) * (xDim + 1);   // 1
        cta[2] = i + ((j + 1) + k * (yDim + 1)) * (xDim + 1); // 3

        cta[3] = i + (j + (k + 1) * (yDim + 1)) * (xDim + 1);       // 0
        cta[4] = i + 1 + (j + (k + 1) * (yDim + 1)) * (xDim + 1);   // 1
        cta[5] = i + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1); // 3

        ctb[0] = i + 1 + ((j + 1) + k * (yDim + 1)) * (xDim + 1); // 2
        ctb[1] = i + ((j + 1) + k * (yDim + 1)) * (xDim + 1);     // 3
        ctb[2] = i + 1 + (j + k * (yDim + 1)) * (xDim + 1);       // 1

        ctb[3] = i + 1 + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1); // 2
        ctb[4] = i + ((j + 1) + (k + 1) * (yDim + 1)) * (xDim + 1);     // 3
        ctb[5] = i + 1 + (j + (k + 1) * (yDim + 1)) * (xDim + 1);       // 1

        vtkVector3d pt[8], pm;
        output->GetPoint(cta[0], pt[0].GetData());
        output->GetPoint(cta[1], pt[1].GetData());
        output->GetPoint(ctb[0], pt[2].GetData());
        output->GetPoint(ctb[1], pt[3].GetData());
        output->GetPoint(cta[3], pt[4].GetData());
        output->GetPoint(cta[4], pt[5].GetData());
        output->GetPoint(ctb[3], pt[6].GetData());
        output->GetPoint(ctb[4], pt[7].GetData());

        for (int p = 0; p <= order[2]; ++p)
        {
          for (int n = 0; n <= order[0]; ++n)
          {
            for (int m = 0; m <= order[0]; ++m)
            {
              if ((m == 0 || m == order[0]) && (n == 0 || n == order[0]) &&
                (p == 0 || p == order[2]))
              { // skip corner points
                continue;
              }
              double r = static_cast<double>(m) / order[0];
              double s = static_cast<double>(n) / order[0];
              double t = static_cast<double>(p) / order[2];
              pm = (1.0 - r) *
                  ((pt[3] * (1.0 - t) + pt[7] * t) * s +
                    (pt[0] * (1.0 - t) + pt[4] * t) * (1.0 - s)) +
                r *
                  ((pt[2] * (1.0 - t) + pt[6] * t) * s +
                    (pt[1] * (1.0 - t) + pt[5] * t) * (1.0 - s));
              vtkIdType innerPointId;
              this->Locator->InsertUniquePoint(pm.GetData(), innerPointId);

              if (m + n <= order[0])
              {
                int ctaidx = vtkBezierWedge::PointIndexFromIJK(m, n, p, order);
                cta[ctaidx] = innerPointId;
              }
              if (m + n >= order[0])
              {
                int ctbidx =
                  vtkBezierWedge::PointIndexFromIJK(order[0] - m, order[0] - n, p, order);
                ctb[ctbidx] = innerPointId;
              }
            }
          }
        }
        if (this->CompleteQuadraticSimplicialElements && this->CellOrder == 2)
        {
          // When present, triangle mid-face nodes should appear before
          // the quadrilateral mid-face nodes. So, shift the 3 quad-face
          // nodes by 2 entries in the connectivity array:
          for (int ii = 0; ii < 3; ++ii)
          {
            cta[19 - ii] = cta[17 - ii];
            ctb[19 - ii] = ctb[17 - ii];
          }
          // Now fill in the "holes" at ct{a,b}[15,16] with tri-face nodes:
          static const int facePts[2][2][3] = {
            { { 0, 1, 3 }, { 4, 5, 7 } }, // cta
            { { 1, 2, 3 }, { 5, 6, 7 } }  // ctb
          };
          vtkVector3d bodyA(0., 0., 0.);
          vtkVector3d bodyB(0., 0., 0.);
          vtkIdType innerA, innerB;
          for (int ii = 0; ii < 2; ++ii)
          {
            vtkVector3d pA =
              (pt[facePts[0][ii][0]] + pt[facePts[0][ii][1]] + pt[facePts[0][ii][2]]) * (1. / 3.);
            vtkVector3d pB =
              (pt[facePts[1][ii][0]] + pt[facePts[1][ii][1]] + pt[facePts[1][ii][2]]) * (1. / 3.);
            bodyA = bodyA + 0.5 * pA;
            bodyB = bodyB + 0.5 * pB;
            this->Locator->InsertUniquePoint(pA.GetData(), innerA);
            this->Locator->InsertUniquePoint(pB.GetData(), innerB);
            cta[15 + ii] = innerA;
            ctb[15 + ii] = innerB;
          }
          // Finally, add a body-centered node to cta and ctb:
          this->Locator->InsertUniquePoint(bodyA.GetData(), innerA);
          this->Locator->InsertUniquePoint(bodyB.GetData(), innerB);
          cta[20] = innerA;
          ctb[20] = innerB;
        }
        output->InsertNextCell(VTK_BEZIER_WEDGE, numPtsPerCell, &cta[0]);
        output->InsertNextCell(VTK_BEZIER_WEDGE, numPtsPerCell, &ctb[0]);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::ComputeFields(vtkUnstructuredGrid* output)
{
  double center[3] = { this->BlocksDimensions[0] * .5, this->BlocksDimensions[1] * .5,
    this->BlocksDimensions[2] * .5 };
  int cellDimension = this->GetCellDimension();
  if (cellDimension < 3)
  {
    center[2] = 0;
  }
  if (cellDimension < 2)
  {
    center[1] = 0;
  }
  const vtkIdType numberOfPoints = output->GetNumberOfPoints();
  double coords[3];
  vtkDataArray* distanceToCenter = output->GetPoints()->GetData()->NewInstance();
  distanceToCenter->SetNumberOfTuples(numberOfPoints);
  distanceToCenter->SetName("DistanceToCenter");
  output->GetPointData()->AddArray(distanceToCenter);
  distanceToCenter->FastDelete();
  vtkDataArray* polynomialField = distanceToCenter->NewInstance();
  polynomialField->SetNumberOfTuples(numberOfPoints);
  polynomialField->SetName("Polynomial");
  output->GetPointData()->AddArray(polynomialField);
  polynomialField->FastDelete();
  for (vtkIdType i = 0; i < numberOfPoints; i++)
  {
    output->GetPoint(i, coords);
    double d = sqrt(vtkMath::Distance2BetweenPoints(coords, center));
    distanceToCenter->SetComponent(i, 0, d);
    double p = 1;
    for (int pi = 1; pi <= this->PolynomialFieldOrder; pi++)
    {
      p += this->GetValueOfOrder(pi, coords);
    }
    polynomialField->SetComponent(i, 0, p);
  }
}

//----------------------------------------------------------------------------
double vtkCellTypeSource::GetValueOfOrder(int order, double coords[3])
{
  int v = 0;
  for (int i = 0; i <= order; i++)
  {
    for (int j = 0; j <= order - i; j++)
    {
      int k = order - i - j;
      v += pow(coords[0], i) * pow(coords[1], j) * pow(coords[2], k);
    }
  }
  return v;
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BlocksDimensions: ( " << this->BlocksDimensions[0] << ", "
     << this->BlocksDimensions[1] << ", " << this->BlocksDimensions[2] << " )\n";
  os << indent << "CellType: " << this->CellType << "\n";
  os << indent << "CellOrder: " << this->CellOrder << "\n";
  os << indent << "CompleteQuadraticSimplicialElements: "
     << (this->CompleteQuadraticSimplicialElements ? "TRUE" : "FALSE") << "\n";
  os << indent << "OutputPrecision: " << this->OutputPrecision << "\n";
  os << indent << "PolynomialFieldOrder: " << this->PolynomialFieldOrder << "\n";
}
