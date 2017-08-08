/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellSizeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkCellSizeFilter.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMeshQuality.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPolygon.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkCellSizeFilter);

//-----------------------------------------------------------------------------
vtkCellSizeFilter::vtkCellSizeFilter() :
  ComputePoint(true), ComputeLength(true), ComputeArea(true), ComputeVolume(true),
  ComputeHighestDimension(false), ComputeSum(false), ArrayName(nullptr)
{
  this->SetArrayName("size");
}

//-----------------------------------------------------------------------------
vtkCellSizeFilter::~vtkCellSizeFilter()
{
  this->SetArrayName(nullptr);
}

//----------------------------------------------------------------------------
void vtkCellSizeFilter::ExecuteBlock(vtkDataSet* input, vtkDataSet* output, vtkDoubleArray* sum)
{
  vtkSmartPointer<vtkIdList> cellPtIds = vtkSmartPointer<vtkIdList>::New();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkPoints> cellPoints = vtkSmartPointer<vtkPoints>::New();
  int cellType;
  vtkNew<vtkDoubleArray> values;
  values->SetName(this->ArrayName);
  values->SetNumberOfTuples(numCells);
  double value;
  output->GetCellData()->AddArray(values.GetPointer());
  vtkNew<vtkGenericCell> cell;
  vtkPointSet* inputPS = vtkPointSet::SafeDownCast(input);
  int highestDimension = -1;
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    input->GetCell(cellId, cell.GetPointer());
    int cellDimension = cell->GetCellDimension();
    highestDimension = std::max(cellDimension, highestDimension);
    if (highestDimension == 3)
    {
      break;
    }
  }

  vtkUnsignedCharArray* ghostArray = nullptr;
  if (sum)
  {
    ghostArray = input->GetCellGhostArray();
  }
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    int cellDimension = -1;
    cellType = input->GetCellType(cellId);
    value = -1;
    switch (cellType)
    {
    case VTK_EMPTY_CELL:
      value = 0;
      break;
    case VTK_VERTEX:
      if ( (this->ComputeHighestDimension && highestDimension == 0) ||
           (!this->ComputeHighestDimension && this->ComputePoint) )
      {
        value = 1;
      }
      else
      {
        value = 0;
      }
      cellDimension = 0;
      break;
    case VTK_POLY_VERTEX:
      if ( (this->ComputeHighestDimension && highestDimension == 0) ||
           (!this->ComputeHighestDimension && this->ComputePoint) )
      {
        input->GetCellPoints(cellId, cellPtIds);
        value = static_cast<double>(cellPtIds->GetNumberOfIds());
      }
      else
      {
        value = 0;
      }
      cellDimension = 0;
      break;
    case VTK_POLY_LINE:
    case VTK_LINE:
    {
      if ( (this->ComputeHighestDimension && highestDimension == 1) ||
           (!this->ComputeHighestDimension && this->ComputeLength) )
      {
        input->GetCellPoints(cellId, cellPtIds);
        value = this->IntegratePolyLine(input, cellPtIds);
      }
      else
      {
        value = 0;
      }
    }
    cellDimension = 1;
    break;

    case VTK_TRIANGLE:
    {
      if ( (this->ComputeHighestDimension && highestDimension == 2) ||
           (!this->ComputeHighestDimension && this->ComputeArea) )
      {
        input->GetCell(cellId, cell.GetPointer());
        value = vtkMeshQuality::TriangleArea(cell.GetPointer());
      }
      else
      {
        value = 0;
      }
    }
    cellDimension = 2;
    break;

    case VTK_TRIANGLE_STRIP:
    {
      if ( (this->ComputeHighestDimension && highestDimension == 2) ||
           (!this->ComputeHighestDimension && this->ComputeArea) )
      {
        input->GetCellPoints(cellId, cellPtIds);
        value = this->IntegrateTriangleStrip(inputPS, cellPtIds);
      }
      else
      {
        value = 0;
      }
    }
    cellDimension = 2;
    break;

    case VTK_POLYGON:
    {
      if ( (this->ComputeHighestDimension && highestDimension == 2) ||
           (!this->ComputeHighestDimension && this->ComputeArea) )
      {
        input->GetCellPoints(cellId, cellPtIds);
        value = this->IntegratePolygon(inputPS, cellPtIds);
      }
      else
      {
        value = 0;
      }
    }
    cellDimension = 2;
    break;

    case VTK_PIXEL:
    {
      if ( (this->ComputeHighestDimension && highestDimension == 2) ||
           (!this->ComputeHighestDimension && this->ComputeArea) )
      {
        input->GetCellPoints(cellId, cellPtIds);
        value = this->IntegratePixel(input, cellPtIds);
      }
      else
      {
        value = 0;
      }
    }
    cellDimension = 2;
    break;

    case VTK_QUAD:
    {
      if ( (this->ComputeHighestDimension && highestDimension == 2) ||
           (!this->ComputeHighestDimension && this->ComputeArea) )
      {
        input->GetCell(cellId, cell.GetPointer());
        value = vtkMeshQuality::QuadArea(cell.GetPointer());
      }
      else
      {
        value = 0;
      }
    }
    cellDimension = 2;
    break;

    case VTK_VOXEL:
    {
      if (this->ComputeVolume || this->ComputeHighestDimension)
      {
        input->GetCellPoints(cellId, cellPtIds);
        value = this->IntegrateVoxel(input, cellPtIds);
      }
      else
      {
        value = 0;
      }
    }
    cellDimension = 3;
    break;

    case VTK_TETRA:
    {
      if (this->ComputeVolume || this->ComputeHighestDimension)
      {
        input->GetCell(cellId, cell.GetPointer());
        value = vtkMeshQuality::TetVolume(cell.GetPointer());
      }
      else
      {
        value = 0;
      }
    }
    cellDimension = 3;
    break;

    default:
    {
      // We need to explicitly get the cell
      input->GetCell(cellId, cell.GetPointer());
      cellDimension = cell->GetCellDimension();
      switch (cellDimension)
      {
      case 0:
        if ( (this->ComputeHighestDimension && highestDimension == 0) ||
             (!this->ComputeHighestDimension && this->ComputePoint) )
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = static_cast<double>(cellPtIds->GetNumberOfIds());
        }
        else
        {
          value = 0;
        }
        break;
      case 1:
      if ( (this->ComputeHighestDimension && highestDimension == 1) ||
           (!this->ComputeHighestDimension && this->ComputeLength) )
        {
          cell->Triangulate(1, cellPtIds, cellPoints);
          value = this->IntegrateGeneral1DCell(input, cellPtIds);
        }
        else
        {
          value = 0;
        }
        break;
      case 2:
      if ( (this->ComputeHighestDimension && highestDimension == 2) ||
           (!this->ComputeHighestDimension && this->ComputeArea) )
        {
          cell->Triangulate(1, cellPtIds, cellPoints);
          value = this->IntegrateGeneral2DCell(inputPS, cellPtIds);
        }
        else
        {
          value = 0;
        }
        break;
      case 3:
        if (this->ComputeVolume || this->ComputeHighestDimension)
        {
          cell->Triangulate(1, cellPtIds, cellPoints);
          value = this->IntegrateGeneral3DCell(inputPS, cellPtIds);
        }
        else
        {
          value = 0;
        }
        break;
      default:
        vtkWarningMacro("Unsupported Cell Dimension = " << cellDimension);
      }
    }
    }
    values->SetValue(cellId, value);
    if ( sum && cellDimension != -1 && (!ghostArray || !ghostArray->GetValue(cellId) ) )
    {
      sum->SetValue(cellDimension, sum->GetValue(cellDimension)+value);
    }
  }
}

//-----------------------------------------------------------------------------
int vtkCellSizeFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  bool retVal = true;
  if (vtkDataSet* inputDataSet = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())))
  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
    vtkSmartPointer<vtkDoubleArray> sum;
    if (this->ComputeSum)
    {
      sum = vtkSmartPointer<vtkDoubleArray>::New();
      sum->SetNumberOfTuples(4);
      sum->SetName(this->ArrayName);
      sum->Fill(0.);
    }
    retVal = this->ComputeDataSet(inputDataSet, output, sum);
    if (this->ComputeSum)
    {
      this->ComputeGlobalSum(sum);
    }
  }
  else if (vtkCompositeDataSet* input =
           vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())))
  {
    vtkCompositeDataSet* output =
      vtkCompositeDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
    output->CopyStructure(input);
    vtkCompositeDataIterator* iter = input->NewIterator();
    iter->SkipEmptyNodesOff();
    vtkSmartPointer<vtkDoubleArray> sumComposite;
    if (this->ComputeSum)
    {
      sumComposite = vtkSmartPointer<vtkDoubleArray>::New();
      sumComposite->SetNumberOfTuples(4);
      sumComposite->SetName(this->ArrayName);
      sumComposite->Fill(0.);
    }
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkSmartPointer<vtkDoubleArray> sum;
      if (this->ComputeSum)
      {
        sum = vtkSmartPointer<vtkDoubleArray>::New();
        sum->SetNumberOfTuples(4);
        sum->SetName(this->ArrayName);
        sum->Fill(0.);
      }
      if (vtkDataSet* inputDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
      {
        vtkDataSet* outputDS = inputDS->NewInstance();
        output->SetDataSet(iter, outputDS);
        outputDS->Delete();
        retVal = retVal && this->ComputeDataSet(inputDS, outputDS, sum);
        if (this->ComputeSum)
        {
          this->ComputeGlobalSum(sum);
        }
      }
      if (this->ComputeSum)
      {
        for (int i=0;i<4;i++)
        {
          double v = sumComposite->GetValue(i)+sum->GetValue(i);
          sumComposite->SetValue(i, v);
        }
      }
    }
    iter->Delete();
    if (this->ComputeSum)
    {
      output->GetFieldData()->AddArray(sumComposite);
    }
  }
  else
  {
    retVal = false;
    vtkWarningMacro("Cannot handle input of type " <<
                    inInfo->Get(vtkDataObject::DATA_OBJECT())->GetClassName());
  }

  return retVal;
}

//-----------------------------------------------------------------------------
bool vtkCellSizeFilter::ComputeDataSet(
  vtkDataSet* input, vtkDataSet* output, vtkDoubleArray* sum)
{
  output->ShallowCopy(input);

  // fast path for image data since all the cells have the same volume
  if (vtkImageData* imageData = vtkImageData::SafeDownCast(input))
  {
    this->IntegrateImageData(imageData, vtkImageData::SafeDownCast(output), sum);
  }
  else
  {
    this->ExecuteBlock(input, output, sum);
  }
  if (sum)
  {
    output->GetFieldData()->AddArray(sum);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkCellSizeFilter::IntegrateImageData(
  vtkImageData* input, vtkImageData* output, vtkDoubleArray* sum)
{
  int extent[6];
  input->GetExtent(extent);
  double spacing[3];
  input->GetSpacing(spacing);
  double val = 1;
  int dimension = 0;
  for (int i=0;i<3;i++)
  {
    if (extent[2*i+1] > extent[2*i])
    {
      val *= spacing[i];
      dimension++;
    }
  }
  switch (dimension)
  {
  case 0:
    val = this->ComputePoint || this->ComputeHighestDimension ? 1 : 0;
    break;
  case 1:
    val = this->ComputeLength || this->ComputeHighestDimension ? val : 0;
    break;
  case 2:
    val = this->ComputeArea || this->ComputeHighestDimension ? val : 0;
    break;
  case 3:
    val = this->ComputeVolume || this->ComputeHighestDimension ? val : 0;
  }
  vtkNew<vtkDoubleArray> outArray;
  outArray->SetName(this->ArrayName);
  outArray->SetNumberOfTuples(input->GetNumberOfCells());
  outArray->Fill(val);
  output->GetCellData()->AddArray(outArray.GetPointer());
  if (sum)
  {
    if (vtkUnsignedCharArray* ghosts = input->GetCellGhostArray())
    {
      double summation = 0;
      for (vtkIdType i=0;i<output->GetNumberOfCells();i++)
      {
        if (!ghosts->GetValue(i))
        {
          summation += val;
        }
      }
      sum->SetValue(dimension, summation);
    }
    else
    {
      sum->SetValue(dimension, input->GetNumberOfCells()*val);
    }
    output->GetFieldData()->AddArray(sum);
  }
}

//-----------------------------------------------------------------------------
double vtkCellSizeFilter::IntegratePolyLine(vtkDataSet* input, vtkIdList* ptIds)
{
  double sum = 0;
  double pt1[3], pt2[3];

  vtkIdType numLines = ptIds->GetNumberOfIds() - 1;
  for (vtkIdType lineIdx = 0; lineIdx < numLines; ++lineIdx)
  {
    vtkIdType pt1Id = ptIds->GetId(lineIdx);
    vtkIdType pt2Id = ptIds->GetId(lineIdx + 1);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    double length = sqrt(vtkMath::Distance2BetweenPoints(pt1, pt2));
    sum += length;
  }
  return sum;
}

//-----------------------------------------------------------------------------
double vtkCellSizeFilter::IntegrateGeneral1DCell(
  vtkDataSet* input, vtkIdList* ptIds)
{
  // Determine the number of lines
  vtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be an even number of points from the triangulation
  if (nPnts % 2)
  {
    vtkWarningMacro("Odd number of points(" << nPnts << ")  encountered - skipping ");
    return 0;
  }

  double pt1[3], pt2[3];
  vtkIdType pid = 0;
  vtkIdType pt1Id, pt2Id;
  double sum = 0;
  while (pid < nPnts)
  {
    pt1Id = ptIds->GetId(pid++);
    pt2Id = ptIds->GetId(pid++);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    double length = sqrt(vtkMath::Distance2BetweenPoints(pt1, pt2));
    sum += length;
  }
  return sum;
}

//-----------------------------------------------------------------------------
double vtkCellSizeFilter::IntegrateTriangleStrip(
  vtkPointSet* input, vtkIdList* ptIds)
{
  vtkIdType trianglePtIds[3];
  vtkIdType numTris = ptIds->GetNumberOfIds() - 2;
  double sum = 0;
  for (vtkIdType triIdx = 0; triIdx < numTris; ++triIdx)
  {
    trianglePtIds[0] = ptIds->GetId(triIdx);
    trianglePtIds[1] = ptIds->GetId(triIdx + 1);
    trianglePtIds[2] = ptIds->GetId(triIdx + 2);
    vtkNew<vtkTriangle> triangle;
    triangle->Initialize(3, trianglePtIds, input->GetPoints());
    sum += triangle->ComputeArea();
  }
  return sum;
}

//-----------------------------------------------------------------------------
// Works for convex polygons, and interpolation is not correct.
double vtkCellSizeFilter::IntegratePolygon(vtkPointSet* input, vtkIdList* ptIds)
{
  vtkIdType numTris = ptIds->GetNumberOfIds() - 2;
  vtkIdType trianglePtIds[3] = {ptIds->GetId(0), 0, 0};
  double sum = 0;
  for (vtkIdType triIdx = 0; triIdx < numTris; ++triIdx)
  {
    trianglePtIds[1] = ptIds->GetId(triIdx + 1);
    trianglePtIds[2] = ptIds->GetId(triIdx + 2);
    vtkNew<vtkTriangle> triangle;
    triangle->Initialize(3, trianglePtIds, input->GetPoints());
    sum += triangle->ComputeArea();
  }
  return sum;
}

//-----------------------------------------------------------------------------
// For axis alligned rectangular cells
double vtkCellSizeFilter::IntegratePixel(vtkDataSet* input, vtkIdList* cellPtIds)
{
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id;
  double pts[4][3];
  pt1Id = cellPtIds->GetId(0);
  pt2Id = cellPtIds->GetId(1);
  pt3Id = cellPtIds->GetId(2);
  pt4Id = cellPtIds->GetId(3);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);

  // get the lengths of its 2 orthogonal sides.  Since only 1 coordinate
  // can be different we can add the differences in all 3 directions
  double l = (pts[0][0] - pts[1][0]) + (pts[0][1] - pts[1][1]) + (pts[0][2] - pts[1][2]);
  double w = (pts[0][0] - pts[2][0]) + (pts[0][1] - pts[2][1]) + (pts[0][2] - pts[2][2]);

  return fabs(l * w);
}

//-----------------------------------------------------------------------------
double vtkCellSizeFilter::IntegrateGeneral2DCell(
  vtkPointSet* input, vtkIdList* ptIds)
{
  vtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 3
  // from the triangulation
  if (nPnts % 3)
  {
    vtkWarningMacro("Number of points (" << nPnts << ") is not divisible by 3 - skipping ");
    return 0;
  }

  vtkIdType triIdx = 0;
  vtkIdType trianglePtIds[3];
  double sum = 0;
  while (triIdx < nPnts)
  {
    trianglePtIds[0] = ptIds->GetId(triIdx++);
    trianglePtIds[1] = ptIds->GetId(triIdx++);
    trianglePtIds[2] = ptIds->GetId(triIdx++);
    vtkNew<vtkTriangle> triangle;
    triangle->Initialize(3, trianglePtIds, input->GetPoints());
    sum += triangle->ComputeArea();
  }
  return sum;
}

//-----------------------------------------------------------------------------
// For axis alligned hexahedral cells
double vtkCellSizeFilter::IntegrateVoxel(
  vtkDataSet* input, vtkIdList* cellPtIds)
{
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id, pt5Id;
  double pts[5][3];
  pt1Id = cellPtIds->GetId(0);
  pt2Id = cellPtIds->GetId(1);
  pt3Id = cellPtIds->GetId(2);
  pt4Id = cellPtIds->GetId(3);
  pt5Id = cellPtIds->GetId(4);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);
  input->GetPoint(pt5Id, pts[4]);

  // Calculate the volume of the voxel
  double l = pts[1][0] - pts[0][0];
  double w = pts[2][1] - pts[0][1];
  double h = pts[4][2] - pts[0][2];
  return fabs(l * w * h);
}

//-----------------------------------------------------------------------------
double vtkCellSizeFilter::IntegrateGeneral3DCell(
  vtkPointSet* input, vtkIdList* ptIds)
{
  vtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 4
  // from the triangulation
  if (nPnts % 4)
  {
    vtkWarningMacro("Number of points (" << nPnts << ") is not divisible by 4 - skipping ");
    return 0;
  }

  vtkIdType tetIdx = 0;
  vtkIdType tetPtIds[4];
  double sum = 0;

  while (tetIdx < nPnts)
  {
    tetPtIds[0] = ptIds->GetId(tetIdx++);
    tetPtIds[1] = ptIds->GetId(tetIdx++);
    tetPtIds[2] = ptIds->GetId(tetIdx++);
    tetPtIds[3] = ptIds->GetId(tetIdx++);
    vtkNew<vtkTetra> tet;
    tet->Initialize(4, tetPtIds, input->GetPoints());
    sum += vtkMeshQuality::TetVolume(tet.GetPointer());
  }
  return sum;
}

//-----------------------------------------------------------------------------
void vtkCellSizeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ComputePoint:" << this->ComputePoint << endl;
  os << indent << "ComputeLength:" << this->ComputeLength << endl;
  os << indent << "ComputeArea:" << this->ComputeArea << endl;
  os << indent << "ComputeVolume:" << this->ComputeVolume << endl;
  os << indent << "ComputeHighestDimension:" << this->ComputeHighestDimension << endl;
  os << indent << "ComputeSum:" << this->ComputeSum << endl;
}
