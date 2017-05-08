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

#include "vtkCellType.h"
#include "vtkExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <map>
typedef std::map<std::pair<vtkIdType, vtkIdType>, vtkIdType> EdgeToPointMap;

vtkStandardNewMacro(vtkCellTypeSource);

namespace
{
  const int NumberOf1DCellTypes = 3;
  const int OneDCellTypes[NumberOf1DCellTypes] =
  {VTK_LINE, VTK_QUADRATIC_EDGE, VTK_CUBIC_LINE};
  const int NumberOf2DCellTypes = 4;
  const int TwoDCellTypes[NumberOf2DCellTypes] =
  {VTK_TRIANGLE, VTK_QUAD, VTK_QUADRATIC_TRIANGLE, VTK_QUADRATIC_QUAD};
  const int NumberOf3DCellTypes = 8;
  const int ThreeDCellTypes[NumberOf3DCellTypes] =
  {VTK_TETRA, VTK_HEXAHEDRON, VTK_WEDGE, VTK_PYRAMID,
   VTK_QUADRATIC_TETRA, VTK_QUADRATIC_HEXAHEDRON,
   VTK_QUADRATIC_WEDGE, VTK_QUADRATIC_PYRAMID};
}

// ----------------------------------------------------------------------------
vtkCellTypeSource::vtkCellTypeSource()
{
  this->CellType = VTK_HEXAHEDRON;
  this->OutputPointsPrecision = SINGLE_PRECISION;
  for(int i=0;i<3;i++)
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
  for(int i=0;i<NumberOf1DCellTypes;i++)
  {
    if(cellType == OneDCellTypes[i])
    {
      this->CellType = cellType;
      this->Modified();
      return;
    }
  }
  for(int i=0;i<NumberOf2DCellTypes;i++)
  {
    if(cellType == TwoDCellTypes[i])
    {
      this->CellType = cellType;
      this->Modified();
      return;
    }
  }
  for(int i=0;i<NumberOf3DCellTypes;i++)
  {
    if(cellType == ThreeDCellTypes[i])
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
  for(int i=0;i<NumberOf1DCellTypes;i++)
  {
    if(this->CellType == OneDCellTypes[i])
    {
      return 1;
    }
  }
  for(int i=0;i<NumberOf2DCellTypes;i++)
  {
    if(this->CellType == TwoDCellTypes[i])
    {
      return 2;
    }
  }
  for(int i=0;i<NumberOf3DCellTypes;i++)
  {
    if(this->CellType == ThreeDCellTypes[i])
    {
      return 3;
    }
  }
  return -1;
}

// ----------------------------------------------------------------------------
void vtkCellTypeSource::SetBlocksDimensions(int* dims)
{
  for(int i=0;i<3;i++)
  {
    if(dims[i] != this->BlocksDimensions[i] && dims[i] > 0)
    {
      this->BlocksDimensions[i] = dims[i];
      this->Modified();
    }
  }
}

// ----------------------------------------------------------------------------
void vtkCellTypeSource::SetBlocksDimensions(int iDim, int jDim, int kDim)
{
  int dims[3] = {iDim, jDim, kDim};
  this->SetBlocksDimensions(dims);
}

// ----------------------------------------------------------------------------
int vtkCellTypeSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // Get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  vtkNew<vtkExtentTranslator> extentTranslator;
  int dimension = this->GetCellDimension();
  int wholeExtent[6] = {0, this->BlocksDimensions[0], 0, 0, 0, 0};
  if(dimension>1)
  {
    wholeExtent[3] = this->BlocksDimensions[1];
  }
  if(dimension>2)
  {
    wholeExtent[5] = this->BlocksDimensions[2];
  }
  int extent[6];
  extentTranslator->PieceToExtentThreadSafe(
    piece, numPieces, 0, wholeExtent, extent,
    extentTranslator->GetSplitMode(), 0);
  int numberOfPoints = 1;
  for(int i=0;i<3;i++)
  {
    if(extent[i*2+1]!=extent[i*2])
    {
      numberOfPoints *= extent[i*2+1]-extent[i*2]+1;
    }
  }

  vtkNew<vtkPoints> points;
  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    points->SetDataType(VTK_DOUBLE);
  }
  else
  {
    points->SetDataType(VTK_FLOAT);
  }

  points->Allocate(numberOfPoints);
  for(int k=extent[4];k<extent[5]+1;k++)
  {
    for(int j=extent[2];j<extent[3]+1;j++)
    {
      for(int i=extent[0];i<extent[1]+1;i++)
      {
        points->InsertNextPoint(i, j, k);
      }
    }
  }
  output->SetPoints(points.GetPointer());
  switch(this->CellType)
  {
  case VTK_LINE:
  {
    output->Allocate(numberOfPoints-1);
    for(int i=0;i<numberOfPoints-1;i++)
    {
      vtkIdType ids[2] = {i, i+1};
      output->InsertNextCell(VTK_LINE, 2, ids);
    }
    break;
  }
  case VTK_QUADRATIC_EDGE:
  {
    output->Allocate(numberOfPoints-1);
    for(int i=0;i<numberOfPoints-1;i++)
    {
      double point1[3], point2[3];
      output->GetPoint(i, point1);
      output->GetPoint(i+1, point2);
      for(int j=0;j<3;j++)
      {
        point1[j] = (point1[j]+point2[j])*.5;
      }
      vtkIdType midPointId = points->InsertNextPoint(point1);
      vtkIdType ids[3] = {i, i+1, midPointId};
      output->InsertNextCell(VTK_QUADRATIC_EDGE, 3, ids);
    }
    break;
  }
  case VTK_CUBIC_LINE:
  {
    output->Allocate(numberOfPoints-1);
    for(int i=0;i<numberOfPoints-1;i++)
    {
      double point1[3], point2[3], newPoint1[3], newPoint2[3];
      output->GetPoint(i, point1);
      output->GetPoint(i+1, point2);
      for(int j=0;j<3;j++)
      {
        newPoint1[j] = point1[j]*2./3.+point2[j]/3.;
        newPoint2[j] = point1[j]/3.+point2[j]*2./3.;
      }
      vtkIdType newPointId1 = points->InsertNextPoint(newPoint1);
      vtkIdType newPointId2 = points->InsertNextPoint(newPoint2);
      vtkIdType ids[4] = {i, i+1, newPointId1, newPointId2};
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
  default:
  {
    vtkWarningMacro("Cell type " << this->CellType << " not supported");
  }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCellTypeSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateTriangles(
  vtkUnstructuredGrid* output, int extent[6])
{
  int firstDim = extent[1]-extent[0];
  int secondDim = extent[3]-extent[2];
  output->Allocate(firstDim*secondDim*2);
  for(int j=0;j<secondDim;j++)
  {
    for(int i=0;i<firstDim;i++)
    {
      vtkIdType ids[3] = {i+j*(firstDim+1), i+1+j*(firstDim+1),
                          i+(j+1)*(firstDim+1)};
      output->InsertNextCell(VTK_TRIANGLE, 3, ids);
      ids[0] = ids[1];
      ids[1] = i+1+(j+1)*(firstDim+1);
      output->InsertNextCell(VTK_TRIANGLE, 3, ids);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuads(
  vtkUnstructuredGrid* output, int extent[6])
{
  int firstDim = extent[1]-extent[0];
  int secondDim = extent[3]-extent[2];
  output->Allocate(firstDim*secondDim);
  for(int j=0;j<secondDim;j++)
  {
    for(int i=0;i<firstDim;i++)
    {
      vtkIdType ids[4] = {
        i+j*(firstDim+1), i+1+j*(firstDim+1),
        i+1+(j+1)*(firstDim+1), i+(j+1)*(firstDim+1)};
      output->InsertNextCell(VTK_QUAD, 4, ids);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticTriangles(
  vtkUnstructuredGrid* output, int extent[6])
{
  int firstDim = extent[1]-extent[0];
  int secondDim = extent[3]-extent[2];
  output->Allocate(firstDim*secondDim*2);
  EdgeToPointMap edgeToPointId;
  for(int j=0;j<secondDim;j++)
  {
    for(int i=0;i<firstDim;i++)
    {
      vtkIdType mids[3];
      std::pair<vtkIdType, vtkIdType> horizontalEdge =
        std::make_pair(static_cast<vtkIdType>(i+j*(firstDim+1)),
                       static_cast<vtkIdType>(i+1+j*(firstDim+1)));
      EdgeToPointMap::iterator it=edgeToPointId.find(horizontalEdge);
      if(it==edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(horizontalEdge.first, point1);
        output->GetPoint(horizontalEdge.second, point2);
        for(int k=0;k<3;k++)
        {
          point1[k]=(point1[k]+point2[k])*.5;
        }
        vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[horizontalEdge]=mid;
        mids[0] = mid;
      }
      else
      {
        mids[0] = it->second;
      }
      std::pair<vtkIdType, vtkIdType> verticalEdge =
        std::make_pair(static_cast<vtkIdType>(i+j*(firstDim+1)),
                       static_cast<vtkIdType>(i+(j+1)*(firstDim+1)));
      it=edgeToPointId.find(verticalEdge);
      if(it==edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(verticalEdge.first, point1);
        output->GetPoint(verticalEdge.second, point2);
        for(int k=0;k<3;k++)
        {
          point1[k]=(point1[k]+point2[k])*.5;
        }
        vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[verticalEdge]=mid;
        mids[2] = mid;
      }
      else
      {
        mids[2] = it->second;
      }
      // always need to create the point on the diagonal
      double point1[3], point2[3];
      output->GetPoint(i+1+j*(firstDim+1), point1);
      output->GetPoint(i+(j+1)*(firstDim+1), point2);
      for(int k=0;k<3;k++)
      {
        point1[k]=(point1[k]+point2[k])*.5;
      }
      vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
      mids[1] = mid;
      vtkIdType cellPoints[6] = {i+j*(firstDim+1), i+1+j*(firstDim+1),
                                 i+(j+1)*(firstDim+1), mids[0],
                                 mids[1], mids[2]};
      output->InsertNextCell(VTK_QUADRATIC_TRIANGLE, 6, cellPoints);
      horizontalEdge =
        std::make_pair(static_cast<vtkIdType>(i+(j+1)*(firstDim+1)),
                       static_cast<vtkIdType>(i+1+(j+1)*(firstDim+1)));
      it=edgeToPointId.find(horizontalEdge);
      if(it==edgeToPointId.end())
      {
        output->GetPoint(horizontalEdge.first, point1);
        output->GetPoint(horizontalEdge.second, point2);
        for(int k=0;k<3;k++)
        {
          point1[k]=(point1[k]+point2[k])*.5;
        }
        mid=output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[horizontalEdge]=mid;
        mids[0] = mid;
      }
      else
      {
        mids[0] = it->second;
      }
      verticalEdge =
        std::make_pair(static_cast<vtkIdType>(i+1+j*(firstDim+1)),
                       static_cast<vtkIdType>(i+1+(j+1)*(firstDim+1)));
      it=edgeToPointId.find(verticalEdge);
      if(it==edgeToPointId.end())
      {
        output->GetPoint(verticalEdge.first, point1);
        output->GetPoint(verticalEdge.second, point2);
        for(int k=0;k<3;k++)
        {
          point1[k]=(point1[k]+point2[k])*.5;
        }
        mid=output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[verticalEdge]=mid;
        mids[2] = mid;
      }
      else
      {
        mids[2] = it->second;
      }
      vtkIdType cellPoints2[6] = {i+1+j*(firstDim+1), i+1+(j+1)*(firstDim+1),
                                  i+(j+1)*(firstDim+1), mids[2], mids[0], mids[1]};
      output->InsertNextCell(VTK_QUADRATIC_TRIANGLE, 6, cellPoints2);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticQuads(
  vtkUnstructuredGrid* output, int extent[6])
{
  int firstDim = extent[1]-extent[0];
  int secondDim = extent[3]-extent[2];
  output->Allocate(firstDim*secondDim);
  EdgeToPointMap edgeToPointId;
  for(int j=0;j<secondDim;j++)
  {
    for(int i=0;i<firstDim;i++)
    {
      vtkIdType pointIds[8] = {
        i+j*(firstDim+1), i+1+j*(firstDim+1), i+1+(j+1)*(firstDim+1),
        i+(j+1)*(firstDim+1), -1, -1, -1, -1};
      std::pair<vtkIdType, vtkIdType> bottomEdge =
        std::make_pair(pointIds[0], pointIds[1]);
      EdgeToPointMap::iterator it=edgeToPointId.find(bottomEdge);
      if(it==edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(bottomEdge.first, point1);
        output->GetPoint(bottomEdge.second, point2);
        for(int k=0;k<3;k++)
        {
          point1[k]=(point1[k]+point2[k])*.5;
        }
        vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[bottomEdge]=mid;
        pointIds[4] = mid;
      }
      else
      {
        pointIds[4] = it->second;
      }
      std::pair<vtkIdType, vtkIdType> rightEdge =
        std::make_pair(pointIds[1], pointIds[2]);
      it=edgeToPointId.find(rightEdge);
      if(it==edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(rightEdge.first, point1);
        output->GetPoint(rightEdge.second, point2);
        for(int k=0;k<3;k++)
        {
          point1[k]=(point1[k]+point2[k])*.5;
        }
        vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[rightEdge]=mid;
        pointIds[5] = mid;
      }
      else
      {
        pointIds[5] = it->second;
      }
      std::pair<vtkIdType, vtkIdType> topEdge =
        std::make_pair(pointIds[3], pointIds[2]);
      it=edgeToPointId.find(topEdge);
      if(it==edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(topEdge.first, point1);
        output->GetPoint(topEdge.second, point2);
        for(int k=0;k<3;k++)
        {
          point1[k]=(point1[k]+point2[k])*.5;
        }
        vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[topEdge]=mid;
        pointIds[6] = mid;
      }
      else
      {
        pointIds[6] = it->second;
      }
      std::pair<vtkIdType, vtkIdType> leftEdge =
        std::make_pair(pointIds[0], pointIds[3]);
      it=edgeToPointId.find(leftEdge);
      if(it==edgeToPointId.end())
      {
        double point1[3], point2[3];
        output->GetPoint(leftEdge.first, point1);
        output->GetPoint(leftEdge.second, point2);
        for(int k=0;k<3;k++)
        {
          point1[k]=(point1[k]+point2[k])*.5;
        }
        vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
        edgeToPointId[leftEdge]=mid;
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
void vtkCellTypeSource::GenerateTetras(
  vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1]-extent[0];
  const int yDim = extent[3]-extent[2];
  const int zDim = extent[5]-extent[4];
  output->Allocate(xDim*yDim*zDim*5);
  for(int k=0;k<zDim;k++)
  {
    for(int j=0;j<yDim;j++)
    {
      for(int i=0;i<xDim;i++)
      {
        vtkIdType hexIds[8] = {
          i+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1)};
        // add in center point
        double point1[3], point2[3];
        output->GetPoint(hexIds[0], point1);
        output->GetPoint(hexIds[6], point2);
        for(int l=0;l<3;l++)
        {
          point1[l] = .5*(point1[l]+point2[l]);
        }
        vtkIdType middlePoint =
          output->GetPoints()->InsertNextPoint(point1);

        vtkIdType pointIds1[4] = {hexIds[0], hexIds[1], hexIds[2], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds1);
        vtkIdType pointIds2[4] = {hexIds[0], hexIds[2], hexIds[3], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds2);

        vtkIdType pointIds3[4] = {hexIds[6], hexIds[5], hexIds[4], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds3);
        vtkIdType pointIds4[4] = {hexIds[6], hexIds[4], hexIds[7], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds4);

        vtkIdType pointIds5[4] = {hexIds[1], hexIds[5], hexIds[6], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds5);
        vtkIdType pointIds6[4] = {hexIds[1], hexIds[6], hexIds[2], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds6);

        vtkIdType pointIds7[4] = {hexIds[0], hexIds[4], hexIds[5], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds7);
        vtkIdType pointIds8[4] = {hexIds[0], hexIds[5], hexIds[1], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds8);

        vtkIdType pointIds9[4] = {hexIds[0], hexIds[3], hexIds[7], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds9);
        vtkIdType pointIds10[4] = {hexIds[0], hexIds[7], hexIds[4], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds10);

        vtkIdType pointIds11[4] = {hexIds[6], hexIds[7], hexIds[3], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds11);
        vtkIdType pointIds12[4] = {hexIds[6], hexIds[3], hexIds[2], middlePoint};
        output->InsertNextCell(VTK_TETRA, 4, pointIds12);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateHexahedron(
  vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1]-extent[0];
  const int yDim = extent[3]-extent[2];
  const int zDim = extent[5]-extent[4];
  output->Allocate(xDim*yDim*zDim);

  for(int k=0;k<zDim;k++)
  {
    for(int j=0;j<yDim;j++)
    {
      for(int i=0;i<xDim;i++)
      {
        vtkIdType hexIds[8] = {
          i+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1)};
        output->InsertNextCell(VTK_HEXAHEDRON, 8, hexIds);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateWedges(
  vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  int xDim = extent[1]-extent[0];
  int yDim = extent[3]-extent[2];
  int zDim = extent[5]-extent[4];
  output->Allocate(xDim*yDim*zDim*2);

  for(int k=0;k<zDim;k++)
  {
    for(int j=0;j<yDim;j++)
    {
      for(int i=0;i<xDim;i++)
      {
        vtkIdType wedgeIds[6] = {
          i+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1)};
        output->InsertNextCell(VTK_WEDGE, 6, wedgeIds);
        vtkIdType wedgeIds2[6] = {
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1)};
        output->InsertNextCell(VTK_WEDGE, 6, wedgeIds2);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GeneratePyramids(
  vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  int xDim = extent[1]-extent[0];
  int yDim = extent[3]-extent[2];
  int zDim = extent[5]-extent[4];
  output->Allocate(xDim*yDim*zDim*6);

  for(int k=0;k<zDim;k++)
  {
    for(int j=0;j<yDim;j++)
    {
      for(int i=0;i<xDim;i++)
      {
        vtkIdType hexIds[8] = {
          i+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1)};
        // add in center point
        double point1[3], point2[3];
        output->GetPoint(hexIds[0], point1);
        output->GetPoint(hexIds[6], point2);
        for(int l=0;l<3;l++)
        {
          point1[l] = .5*(point1[l]+point2[l]);
        }
        vtkIdType middlePoint =
          output->GetPoints()->InsertNextPoint(point1);
        vtkIdType pointIds1[5] = {hexIds[0], hexIds[1], hexIds[2],
                                  hexIds[3], middlePoint};
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds1);
        vtkIdType pointIds2[5] = {hexIds[6], hexIds[5], hexIds[4],
                                  hexIds[7], middlePoint};
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds2);
        vtkIdType pointIds3[5] = {hexIds[1], hexIds[5], hexIds[6],
                                  hexIds[2], middlePoint};
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds3);
        vtkIdType pointIds4[5] = {hexIds[0], hexIds[4], hexIds[5],
                                  hexIds[1], middlePoint};
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds4);
        vtkIdType pointIds5[5] = {hexIds[0], hexIds[3], hexIds[7],
                                  hexIds[4], middlePoint};
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds5);
        vtkIdType pointIds6[5] = {hexIds[6], hexIds[7], hexIds[3],
                                  hexIds[2], middlePoint};
        output->InsertNextCell(VTK_PYRAMID, 5, pointIds6);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticTetras(
  vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1]-extent[0];
  const int yDim = extent[3]-extent[2];
  const int zDim = extent[5]-extent[4];
  output->Allocate(xDim*yDim*zDim*5);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[12][6][2] = {
    {{0, 1}, {1, 2}, {0, 2}, {0, 8}, {1, 8}, {2, 8}},
    {{0, 2}, {3, 2}, {0, 3}, {0, 8}, {2, 8}, {3, 8}},
    {{5, 6}, {4, 5}, {4, 6}, {6, 8}, {5, 8}, {4, 8}},
    {{4, 6}, {4, 7}, {7, 6}, {6, 8}, {4, 8}, {7, 8}},
    {{1, 5}, {5, 6}, {1, 6}, {1, 8}, {5, 8}, {6, 8}},
    {{1, 6}, {2, 6}, {1, 2}, {1, 8}, {6, 8}, {2, 8}},
    {{0, 4}, {4, 5}, {0, 5}, {0, 8}, {4, 8}, {5, 8}},
    {{0, 5}, {1, 5}, {0, 1}, {0, 8}, {5, 8}, {1, 8}},
    {{0, 3}, {3, 7}, {0, 7}, {0, 8}, {3, 8}, {7, 8}},
    {{0, 7}, {4, 7}, {0, 4}, {0, 8}, {7, 8}, {4, 8}},
    {{7, 6}, {3, 7}, {3, 6}, {6, 8}, {7, 8}, {3, 8}},
    {{3, 6}, {3, 2}, {2, 6}, {6, 8}, {3, 8}, {2, 8}}
  };

  for(int k=0;k<zDim;k++)
  {
    for(int j=0;j<yDim;j++)
    {
      for(int i=0;i<xDim;i++)
      {
        vtkIdType hexIds[9] = {
          i+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          -1};

        // add in center point
        double point1[3], point2[3];
        output->GetPoint(hexIds[0], point1);
        output->GetPoint(hexIds[6], point2);
        for(int l=0;l<3;l++)
        {
          point1[l] = .5*(point1[l]+point2[l]);
        }
        hexIds[8] = output->GetPoints()->InsertNextPoint(point1);

        vtkIdType tetraIds[12][10] = {
          {hexIds[0], hexIds[1], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[0], hexIds[2], hexIds[3], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[6], hexIds[5], hexIds[4], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[6], hexIds[4], hexIds[7], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[1], hexIds[5], hexIds[6], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[1], hexIds[6], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[0], hexIds[4], hexIds[5], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[0], hexIds[5], hexIds[1], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[0], hexIds[3], hexIds[7], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[0], hexIds[7], hexIds[4], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[6], hexIds[7], hexIds[3], hexIds[8], -1, -1, -1, -1, -1, -1},
          {hexIds[6], hexIds[3], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1}};
        for(int c=0;c<12;c++)
        {
          for(int e=0;e<6;e++)
          {
            std::pair<vtkIdType, vtkIdType> edge =
              std::make_pair(hexIds[edgePairs[c][e][0]], hexIds[edgePairs[c][e][1]]);
            EdgeToPointMap::iterator it=edgeToPointId.find(edge);
            if(it==edgeToPointId.end())
            {
              output->GetPoint(edge.first, point1);
              output->GetPoint(edge.second, point2);
              for(int l=0;l<3;l++)
              {
                point1[l]=(point1[l]+point2[l])*.5;
              }
              vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
              edgeToPointId[edge]=mid;
              tetraIds[c][4+e] = mid;
            }
            else
            {
              tetraIds[c][4+e] = it->second;
            }
          }
          output->InsertNextCell(VTK_QUADRATIC_TETRA, 10, tetraIds[c]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticHexahedron(
  vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1]-extent[0];
  const int yDim = extent[3]-extent[2];
  const int zDim = extent[5]-extent[4];
  output->Allocate(xDim*yDim*zDim);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[12][2] = {
    {0, 1}, {1, 2}, {3, 2}, {0, 3},
    {4, 5}, {5, 6}, {7, 6}, {4, 7},
    {0, 4}, {1, 5}, {2, 6}, {3, 7} };

  for(int k=0;k<zDim;k++)
  {
    for(int j=0;j<yDim;j++)
    {
      for(int i=0;i<xDim;i++)
      {
        vtkIdType hexIds[20] = {
          i+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
        for(int e=0;e<12;e++)
        {
          std::pair<vtkIdType, vtkIdType> edge =
            std::make_pair(hexIds[edgePairs[e][0]], hexIds[edgePairs[e][1]]);
          EdgeToPointMap::iterator it=edgeToPointId.find(edge);
          if(it==edgeToPointId.end())
          {
            double point1[3], point2[3];
            output->GetPoint(edge.first, point1);
            output->GetPoint(edge.second, point2);
            for(int l=0;l<3;l++)
            {
              point1[l]=(point1[l]+point2[l])*.5;
            }
            vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
            edgeToPointId[edge]=mid;
            hexIds[8+e] = mid;
          }
          else
          {
            hexIds[8+e] = it->second;
          }
        }
        output->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, hexIds);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticWedges(
  vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1]-extent[0];
  const int yDim = extent[3]-extent[2];
  const int zDim = extent[5]-extent[4];
  output->Allocate(xDim*yDim*zDim*2);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[2][9][2] = {
    { {0, 3}, {1, 3}, {0, 1},
      {4, 7}, {5, 7}, {4, 5},
      {0, 4}, {3, 7}, {1, 5}},
    { {1, 3}, {3, 2}, {1, 2},
      {5, 7}, {7, 6}, {5, 6},
      {1, 5}, {3, 7}, {2, 6}
    }};
  for(int k=0;k<zDim;k++)
  {
    for(int j=0;j<yDim;j++)
    {
      for(int i=0;i<xDim;i++)
      {
        vtkIdType hexIds[8] = {
          i+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1)};

        vtkIdType wedgeIds[2][15] = {
          {hexIds[0], hexIds[3], hexIds[1],
           hexIds[4], hexIds[7], hexIds[5],
           -1, -1, -1, -1, -1, -1, -1, -1, -1},
          {hexIds[1], hexIds[3], hexIds[2],
           hexIds[5], hexIds[7], hexIds[6],
           -1, -1, -1, -1, -1, -1, -1, -1, -1} };
        for(int c=0;c<2;c++)
        {
          for(int e=0;e<9;e++)
          {
            std::pair<vtkIdType, vtkIdType> edge =
              std::make_pair(hexIds[edgePairs[c][e][0]], hexIds[edgePairs[c][e][1]]);
            EdgeToPointMap::iterator it=edgeToPointId.find(edge);
            if(it==edgeToPointId.end())
            {
              double point1[3], point2[3];
              output->GetPoint(edge.first, point1);
              output->GetPoint(edge.second, point2);
              for(int l=0;l<3;l++)
              {
                point1[l]=(point1[l]+point2[l])*.5;
              }
              vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
              edgeToPointId[edge]=mid;
              wedgeIds[c][6+e] = mid;
            }
            else
            {
              wedgeIds[c][6+e] = it->second;
            }
          }
          output->InsertNextCell(VTK_QUADRATIC_WEDGE, 15, wedgeIds[c]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::GenerateQuadraticPyramids(
  vtkUnstructuredGrid* output, int extent[6])
{
  // cell dimensions
  const int xDim = extent[1]-extent[0];
  const int yDim = extent[3]-extent[2];
  const int zDim = extent[5]-extent[4];
  output->Allocate(xDim*yDim*zDim*6);

  EdgeToPointMap edgeToPointId;
  // pairs go from lower to higher point id
  const vtkIdType edgePairs[6][8][2] = {
    { {0, 1}, {1, 2}, {3, 2}, {0, 3},
      {0, 8}, {1, 8}, {2, 8}, {3, 8} },
    { {5, 6}, {4, 5}, {4, 7}, {7, 6},
      {6, 8}, {5, 8}, {4, 8}, {7, 8} },
    { {1, 5}, {5, 6}, {2, 6}, {1, 2},
      {1, 8}, {5, 8}, {6, 8}, {2, 8} },
    { {0, 4}, {4, 5}, {1, 5}, {0, 1},
      {0, 8}, {4, 8}, {5, 8}, {1, 8} },
    { {0, 3}, {3, 7}, {4, 7}, {0, 4},
      {0, 8}, {3, 8}, {7, 8}, {4, 8} },
    { {7, 6}, {3, 7}, {3, 2}, {2, 6},
      {6, 8}, {7, 8}, {3, 8}, {2, 8} }
  };

  for(int k=0;k<zDim;k++)
  {
    for(int j=0;j<yDim;j++)
    {
      for(int i=0;i<xDim;i++)
      {
        // also add in the middle point id
        vtkIdType hexIds[9] = {
          i+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+k*(xDim+1)*(yDim+1),
          i+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+j*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+1+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1),
          i+(j+1)*(xDim+1)+(k+1)*(xDim+1)*(yDim+1), -1};
        // add in center point
        double point1[3], point2[3];
        output->GetPoint(hexIds[0], point1);
        output->GetPoint(hexIds[6], point2);
        for(int l=0;l<3;l++)
        {
          point1[l] = .5*(point1[l]+point2[l]);
        }
        hexIds[8] =
          output->GetPoints()->InsertNextPoint(point1);

        vtkIdType pyramidIds[6][13] = {
          {hexIds[0], hexIds[1], hexIds[2], hexIds[3], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1},
          {hexIds[6], hexIds[5], hexIds[4], hexIds[7], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1},
          {hexIds[1], hexIds[5], hexIds[6], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1},
          {hexIds[0], hexIds[4], hexIds[5], hexIds[1], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1},
          {hexIds[0], hexIds[3], hexIds[7], hexIds[4], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1},
          {hexIds[6], hexIds[7], hexIds[3], hexIds[2], hexIds[8], -1, -1, -1, -1, -1, -1, -1, -1} };

        for(int c=0;c<6;c++)
        {
          for(int e=0;e<8;e++)
          {
            std::pair<vtkIdType, vtkIdType> edge =
              std::make_pair(hexIds[edgePairs[c][e][0]], hexIds[edgePairs[c][e][1]]);
            EdgeToPointMap::iterator it=edgeToPointId.find(edge);
            if(it==edgeToPointId.end())
            {
              output->GetPoint(edge.first, point1);
              output->GetPoint(edge.second, point2);
              for(int l=0;l<3;l++)
              {
                point1[l]=(point1[l]+point2[l])*.5;
              }
              vtkIdType mid=output->GetPoints()->InsertNextPoint(point1);
              edgeToPointId[edge]=mid;
              pyramidIds[c][5+e] = mid;
            }
            else
            {
              pyramidIds[c][5+e] = it->second;
            }
          }
          output->InsertNextCell(VTK_QUADRATIC_PYRAMID, 13, pyramidIds[c]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCellTypeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BlocksDimensions: ( "
     << this->BlocksDimensions[0] << ", "
     << this->BlocksDimensions[1] << ", "
     << this->BlocksDimensions[2] << " )\n";
  os << indent << "CellType: " << this->CellType << "\n";
  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
}
