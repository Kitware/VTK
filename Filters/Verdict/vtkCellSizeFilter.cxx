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
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
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
vtkCellSizeFilter::vtkCellSizeFilter()
  : ComputeVertexCount(true)
  , ComputeLength(true)
  , ComputeArea(true)
  , ComputeVolume(true)
  , ComputeSum(false)
  , VertexCountArrayName(nullptr)
  , LengthArrayName(nullptr)
  , AreaArrayName(nullptr)
  , VolumeArrayName(nullptr)
{
  this->SetVertexCountArrayName("VertexCount");
  this->SetLengthArrayName("Length");
  this->SetAreaArrayName("Area");
  this->SetVolumeArrayName("Volume");
}

//-----------------------------------------------------------------------------
vtkCellSizeFilter::~vtkCellSizeFilter()
{
  this->SetVertexCountArrayName(nullptr);
  this->SetLengthArrayName(nullptr);
  this->SetAreaArrayName(nullptr);
  this->SetVolumeArrayName(nullptr);
}

//----------------------------------------------------------------------------
void vtkCellSizeFilter::ExecuteBlock(vtkDataSet* input, vtkDataSet* output, double sum[4])
{
  vtkSmartPointer<vtkIdList> cellPtIds = vtkSmartPointer<vtkIdList>::New();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkPoints> cellPoints = vtkSmartPointer<vtkPoints>::New();
  int cellType;
  vtkDoubleArray* arrays[4] = { nullptr, nullptr, nullptr, nullptr };
  if (this->ComputeVertexCount)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(this->VertexCountArrayName);
    array->SetNumberOfTuples(numCells);
    array->Fill(0);
    output->GetCellData()->AddArray(array);
    arrays[0] = array;
  }
  if (this->ComputeLength)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(this->LengthArrayName);
    array->SetNumberOfTuples(numCells);
    array->Fill(0);
    output->GetCellData()->AddArray(array);
    arrays[1] = array;
  }
  if (this->ComputeArea)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(this->AreaArrayName);
    array->SetNumberOfTuples(numCells);
    array->Fill(0);
    output->GetCellData()->AddArray(array);
    arrays[2] = array;
  }
  if (this->ComputeVolume)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(this->VolumeArrayName);
    array->SetNumberOfTuples(numCells);
    array->Fill(0);
    output->GetCellData()->AddArray(array);
    arrays[3] = array;
  }

  vtkNew<vtkGenericCell> cell;
  vtkPointSet* inputPS = vtkPointSet::SafeDownCast(input);

  vtkUnsignedCharArray* ghostArray = nullptr;
  if (sum)
  {
    ghostArray = input->GetCellGhostArray();
  }
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    double value = 0;
    int cellDimension = -1;
    cellType = input->GetCellType(cellId);
    value = -1;
    switch (cellType)
    {
      case VTK_EMPTY_CELL:
        value = 0;
        break;
      case VTK_VERTEX:
        if (this->ComputeVertexCount)
        {
          value = 1;
          cellDimension = 0;
        }
        else
        {
          value = 0;
        }
        break;
      case VTK_POLY_VERTEX:
        if (this->ComputeVertexCount)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = static_cast<double>(cellPtIds->GetNumberOfIds());
          cellDimension = 0;
        }
        else
        {
          value = 0;
        }
        break;
      case VTK_POLY_LINE:
      case VTK_LINE:
      {
        if (this->ComputeLength)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegratePolyLine(input, cellPtIds);
          cellDimension = 1;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case VTK_TRIANGLE:
      {
        if (this->ComputeArea)
        {
          input->GetCell(cellId, cell);
          value = vtkMeshQuality::TriangleArea(cell);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case VTK_TRIANGLE_STRIP:
      {
        if (this->ComputeArea)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegrateTriangleStrip(inputPS, cellPtIds);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case VTK_POLYGON:
      {
        if (this->ComputeArea)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegratePolygon(inputPS, cellPtIds);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case VTK_PIXEL:
      {
        if (this->ComputeArea)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegratePixel(input, cellPtIds);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case VTK_QUAD:
      {
        if (this->ComputeArea)
        {
          input->GetCell(cellId, cell);
          value = vtkMeshQuality::QuadArea(cell);
          cellDimension = 2;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case VTK_VOXEL:
      {
        if (this->ComputeVolume)
        {
          input->GetCellPoints(cellId, cellPtIds);
          value = this->IntegrateVoxel(input, cellPtIds);
          cellDimension = 3;
        }
        else
        {
          value = 0;
        }
      }
      break;

      case VTK_TETRA:
      {
        if (this->ComputeVolume)
        {
          input->GetCell(cellId, cell);
          value = vtkMeshQuality::TetVolume(cell);
          cellDimension = 3;
        }
        else
        {
          value = 0;
        }
      }
      break;

      default:
      {
        // We need to explicitly get the cell
        input->GetCell(cellId, cell);
        cellDimension = cell->GetCellDimension();
        switch (cellDimension)
        {
          case 0:
            if (this->ComputeVertexCount)
            {
              input->GetCellPoints(cellId, cellPtIds);
              value = static_cast<double>(cellPtIds->GetNumberOfIds());
            }
            else
            {
              value = 0;
              cellDimension = -1;
            }
            break;
          case 1:
            if (this->ComputeLength)
            {
              cell->Triangulate(1, cellPtIds, cellPoints);
              value = this->IntegrateGeneral1DCell(input, cellPtIds);
            }
            else
            {
              value = 0;
              cellDimension = -1;
            }
            break;
          case 2:
            if (this->ComputeArea)
            {
              cell->Triangulate(1, cellPtIds, cellPoints);
              value = this->IntegrateGeneral2DCell(inputPS, cellPtIds);
            }
            else
            {
              value = 0;
              cellDimension = -1;
            }
            break;
          case 3:
            if (this->ComputeVolume)
            {
              cell->Triangulate(1, cellPtIds, cellPoints);
              value = this->IntegrateGeneral3DCell(inputPS, cellPtIds);
            }
            else
            {
              value = 0;
              cellDimension = -1;
            }
            break;
          default:
            vtkWarningMacro("Unsupported Cell Dimension = " << cellDimension);
            cellDimension = -1;
        }
      }
    } // end switch (cellType)
    if (cellDimension != -1)
    { // a valid cell that we want to compute the size of
      arrays[cellDimension]->SetValue(cellId, value);
      if (sum && (!ghostArray || !ghostArray->GetValue(cellId)))
      {
        sum[cellDimension] += value;
      }
    }
  } // end cell iteration
}

//-----------------------------------------------------------------------------
int vtkCellSizeFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  bool retVal = true;
  if (vtkDataSet* inputDataSet =
        vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT())))
  {
    vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
    double sum[4] = { 0, 0, 0, 0 };
    retVal = this->ComputeDataSet(inputDataSet, output, sum);
    if (this->ComputeSum)
    {
      this->ComputeGlobalSum(sum);
      this->AddSumFieldData(output, sum);
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
    double sumComposite[4] = { 0, 0, 0, 0 };
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      double sum[4] = { 0, 0, 0, 0 };
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
        for (int i = 0; i < 4; i++)
        {
          sumComposite[i] += sum[i];
        }
      }
    }
    iter->Delete();
    if (this->ComputeSum)
    {
      this->AddSumFieldData(output, sumComposite);
    }
  }
  else
  {
    retVal = false;
    vtkWarningMacro(
      "Cannot handle input of type " << inInfo->Get(vtkDataObject::DATA_OBJECT())->GetClassName());
  }

  return retVal;
}

//-----------------------------------------------------------------------------
bool vtkCellSizeFilter::ComputeDataSet(vtkDataSet* input, vtkDataSet* output, double sum[4])
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
  if (this->ComputeSum)
  {
    this->AddSumFieldData(output, sum);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkCellSizeFilter::IntegrateImageData(vtkImageData* input, vtkImageData* output, double sum[4])
{
  int extent[6];
  input->GetExtent(extent);
  double spacing[3];
  input->GetSpacing(spacing);
  double val = 1;
  int dimension = 0;
  for (int i = 0; i < 3; i++)
  {
    if (extent[2 * i + 1] > extent[2 * i])
    {
      val *= spacing[i];
      dimension++;
    }
  }
  if (this->ComputeVertexCount)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(this->VertexCountArrayName);
    array->SetNumberOfTuples(output->GetNumberOfCells());
    if (dimension == 0)
    {
      array->SetValue(0, 1);
    }
    else
    {
      array->Fill(0);
    }
    output->GetCellData()->AddArray(array);
  }
  if (this->ComputeLength)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(this->LengthArrayName);
    array->SetNumberOfTuples(output->GetNumberOfCells());
    if (dimension == 1)
    {
      array->Fill(val);
    }
    else
    {
      array->Fill(0);
    }
    output->GetCellData()->AddArray(array);
  }
  if (this->ComputeArea)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(this->AreaArrayName);
    array->SetNumberOfTuples(output->GetNumberOfCells());
    if (dimension == 2)
    {
      array->Fill(val);
    }
    else
    {
      array->Fill(0);
    }
    output->GetCellData()->AddArray(array);
  }
  if (this->ComputeVolume)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(this->VolumeArrayName);
    array->SetNumberOfTuples(output->GetNumberOfCells());
    if (dimension == 3)
    {
      array->Fill(val);
    }
    else
    {
      array->Fill(0);
    }
    output->GetCellData()->AddArray(array);
  }
  if (this->ComputeSum)
  {
    if (vtkUnsignedCharArray* ghosts = input->GetCellGhostArray())
    {
      for (vtkIdType i = 0; i < output->GetNumberOfCells(); i++)
      {
        if (!ghosts->GetValue(i))
        {
          sum[dimension] += val;
        }
      }
    }
    else
    {
      sum[dimension] = input->GetNumberOfCells() * val;
    }
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
double vtkCellSizeFilter::IntegrateGeneral1DCell(vtkDataSet* input, vtkIdList* ptIds)
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
double vtkCellSizeFilter::IntegrateTriangleStrip(vtkPointSet* input, vtkIdList* ptIds)
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
  vtkIdType trianglePtIds[3] = { ptIds->GetId(0), 0, 0 };
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
// For axis aligned rectangular cells
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
double vtkCellSizeFilter::IntegrateGeneral2DCell(vtkPointSet* input, vtkIdList* ptIds)
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
// For axis aligned hexahedral cells
double vtkCellSizeFilter::IntegrateVoxel(vtkDataSet* input, vtkIdList* cellPtIds)
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
double vtkCellSizeFilter::IntegrateGeneral3DCell(vtkPointSet* input, vtkIdList* ptIds)
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
    sum += vtkMeshQuality::TetVolume(tet);
  }
  return sum;
}

//-----------------------------------------------------------------------------
void vtkCellSizeFilter::AddSumFieldData(vtkDataObject* output, double sum[4])
{
  if (this->ComputeVertexCount)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetNumberOfTuples(1);
    array->SetValue(0, sum[0]);
    array->SetName(this->VertexCountArrayName);
    output->GetFieldData()->AddArray(array);
  }
  if (this->ComputeLength)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetNumberOfTuples(1);
    array->SetValue(0, sum[1]);
    array->SetName(this->LengthArrayName);
    output->GetFieldData()->AddArray(array);
  }
  if (this->ComputeArea)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetNumberOfTuples(1);
    array->SetValue(0, sum[2]);
    array->SetName(this->AreaArrayName);
    output->GetFieldData()->AddArray(array);
  }
  if (this->ComputeVolume)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetNumberOfTuples(1);
    array->SetValue(0, sum[3]);
    array->SetName(this->VolumeArrayName);
    output->GetFieldData()->AddArray(array);
  }
}

//-----------------------------------------------------------------------------
void vtkCellSizeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ComputeVertexCount: " << this->ComputeVertexCount << endl;
  os << indent << "ComputeLength: " << this->ComputeLength << endl;
  os << indent << "ComputeArea: " << this->ComputeArea << endl;
  os << indent << "ComputeVolume: " << this->ComputeVolume << endl;
  if (this->VertexCountArrayName)
  {
    os << indent << "VertexCountArrayName:" << this->VertexCountArrayName << endl;
  }
  else
  {
    os << indent << "VertexCountArrayName: (null)\n";
  }
  if (this->LengthArrayName)
  {
    os << indent << "LengthArrayName:" << this->LengthArrayName << endl;
  }
  else
  {
    os << indent << "LengthArrayName: (null)\n";
  }
  if (this->AreaArrayName)
  {
    os << indent << "AreaArrayName:" << this->AreaArrayName << endl;
  }
  else
  {
    os << indent << "AreaArrayName: (null)\n";
  }
  if (this->VolumeArrayName)
  {
    os << indent << "VolumeArrayName:" << this->VolumeArrayName << endl;
  }
  else
  {
    os << indent << "VolumeArrayName: (null)\n";
  }
  os << indent << "ComputeSum: " << this->ComputeSum << endl;
}
