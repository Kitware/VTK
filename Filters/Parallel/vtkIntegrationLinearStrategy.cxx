// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIntegrationLinearStrategy.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIntegrateAttributes.h"
#include "vtkIntegrateAttributesFieldList.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkIntegrationLinearStrategy);

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Integration Strategy: Linear" << std::endl;
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegratePolyLine(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  double length;
  double pt1[3], pt2[3], mid[3];
  vtkIdType numLines, lineIdx;
  vtkIdType pt1Id, pt2Id;

  numLines = numPts - 1;
  for (lineIdx = 0; lineIdx < numLines; ++lineIdx)
  {
    pt1Id = cellPtIds[lineIdx];
    pt2Id = cellPtIds[lineIdx + 1];
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    length = std::sqrt(vtkMath::Distance2BetweenPoints(pt1, pt2));
    sum += length;

    // Compute the middle, which is really just another attribute.
    mid[0] = (pt1[0] + pt2[0]) * 0.5;
    mid[1] = (pt1[1] + pt2[1]) * 0.5;
    mid[2] = (pt1[2] + pt2[2]) * 0.5;
    // Add weighted to sumCenter.
    sumCenter[0] += mid[0] * length;
    sumCenter[1] += mid[1] * length;
    sumCenter[2] += mid[2] * length;

    // Now integrate the rest of the attributes.
    this->IntegrateData2(
      input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, length, pointFieldList, index);
    this->IntegrateData1(
      input->GetCellData(), output->GetCellData(), cellId, length, cellFieldList, index);
  }
}

//------------------------------------------------------------------------------
// Works for convex polygons, and interpolation is not correct.
void vtkIntegrationLinearStrategy::IntegratePolygon(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  vtkIdType numTris, triIdx;
  vtkIdType pt1Id, pt2Id, pt3Id;

  numTris = numPts - 2;
  pt1Id = cellPtIds[0];
  for (triIdx = 0; triIdx < numTris; ++triIdx)
  {
    pt2Id = cellPtIds[triIdx + 1];
    pt3Id = cellPtIds[triIdx + 2];
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id, sum, sumCenter,
      cellFieldList, pointFieldList, index);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateTriangleStrip(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  vtkIdType numTris, triIdx;
  vtkIdType pt1Id, pt2Id, pt3Id;

  numTris = numPts - 2;
  for (triIdx = 0; triIdx < numTris; ++triIdx)
  {
    pt1Id = cellPtIds[triIdx];
    pt2Id = cellPtIds[triIdx + 1];
    pt3Id = cellPtIds[triIdx + 2];
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id, sum, sumCenter,
      cellFieldList, pointFieldList, index);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateTriangle(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double& sum,
  double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  double pt1[3], pt2[3], pt3[3];
  double mid[3], v1[3], v2[3];
  double cross[3];
  double area;

  input->GetPoint(pt1Id, pt1);
  input->GetPoint(pt2Id, pt2);
  input->GetPoint(pt3Id, pt3);

  // Compute two legs.
  v1[0] = pt2[0] - pt1[0];
  v1[1] = pt2[1] - pt1[1];
  v1[2] = pt2[2] - pt1[2];
  v2[0] = pt3[0] - pt1[0];
  v2[1] = pt3[1] - pt1[1];
  v2[2] = pt3[2] - pt1[2];

  // Use the cross product to compute the area of the parallelogram.
  vtkMath::Cross(v1, v2, cross);
  area = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]) * 0.5;

  if (area == 0.0)
  {
    return;
  }
  sum += area;

  // Compute the middle, which is really just another attribute.
  mid[0] = (pt1[0] + pt2[0] + pt3[0]) / 3.0;
  mid[1] = (pt1[1] + pt2[1] + pt3[1]) / 3.0;
  mid[2] = (pt1[2] + pt2[2] + pt3[2]) / 3.0;
  // Add weighted to sumCenter.
  sumCenter[0] += mid[0] * area;
  sumCenter[1] += mid[1] * area;
  sumCenter[2] += mid[2] * area;

  // Now integrate the rest of the attributes.
  this->IntegrateData3(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, area,
    pointFieldList, index);
  this->IntegrateData1(
    input->GetCellData(), output->GetCellData(), cellId, area, cellFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateQuad(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double& sum,
  double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id, sum, sumCenter, cellFieldList,
    pointFieldList, index);
  this->IntegrateTriangle(input, output, cellId, pt1Id, pt4Id, pt3Id, sum, sumCenter, cellFieldList,
    pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateTetrahedron(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id,
  vtkIdType pt4Id, double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  double pts[4][3];
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);

  double edge0[3], edge1[3], edge2[3], normal[3], volume, mid[3];
  // Compute the principle vectors around pt0 and the
  // centroid
  for (int i = 0; i < 3; i++)
  {
    edge0[i] = pts[1][i] - pts[0][i];
    edge1[i] = pts[2][i] - pts[0][i];
    edge2[i] = pts[3][i] - pts[0][i];
    mid[i] = (pts[0][i] + pts[1][i] + pts[2][i] + pts[3][i]) * 0.25;
  }

  // Calculate the volume of the tet which is 1/6 * the box product
  vtkMath::Cross(edge0, edge1, normal);
  volume = vtkMath::Dot(edge2, normal) / 6.0;
  sum += volume;

  // Add weighted to sumCenter.
  sumCenter[0] += mid[0] * volume;
  sumCenter[1] += mid[1] * volume;
  sumCenter[2] += mid[2] * volume;

  // Integrate the attributes on the cell itself
  this->IntegrateData1(
    input->GetCellData(), output->GetCellData(), cellId, volume, cellFieldList, index);

  // Integrate the attributes associated with the points
  this->IntegrateData4(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, pt4Id,
    volume, pointFieldList, index);
}

//------------------------------------------------------------------------------
// For axis aligned rectangular cells
void vtkIntegrationLinearStrategy::IntegratePixel(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType vtkNotUsed(numPts), const vtkIdType* cellPtIds, double& sum,
  double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  double pts[4][3];
  input->GetPoint(cellPtIds[0], pts[0]);
  input->GetPoint(cellPtIds[1], pts[1]);
  input->GetPoint(cellPtIds[2], pts[2]);
  input->GetPoint(cellPtIds[3], pts[3]);

  double length, width, area, mid[3];

  // get the lengths of its 2 orthogonal sides.  Since only 1 coordinate
  // can be different we can add the differences in all 3 directions
  length = (pts[0][0] - pts[1][0]) + (pts[0][1] - pts[1][1]) + (pts[0][2] - pts[1][2]);

  width = (pts[0][0] - pts[2][0]) + (pts[0][1] - pts[2][1]) + (pts[0][2] - pts[2][2]);

  area = std::abs(length * width);
  sum += area;
  // Compute the middle, which is really just another attribute.
  mid[0] = (pts[0][0] + pts[1][0] + pts[2][0] + pts[3][0]) * 0.25;
  mid[1] = (pts[0][1] + pts[1][1] + pts[2][1] + pts[3][1]) * 0.25;
  mid[2] = (pts[0][2] + pts[1][2] + pts[2][2] + pts[3][2]) * 0.25;
  // Add weighted to sumCenter.
  sumCenter[0] += mid[0] * area;
  sumCenter[1] += mid[1] * area;
  sumCenter[2] += mid[2] * area;

  // Now integrate the rest of the attributes.
  this->IntegrateData4(input->GetPointData(), output->GetPointData(), cellPtIds[0], cellPtIds[1],
    cellPtIds[2], cellPtIds[3], area, pointFieldList, index);
  this->IntegrateData1(
    input->GetCellData(), output->GetCellData(), cellId, area, cellFieldList, index);
}

//------------------------------------------------------------------------------
// For axis aligned hexahedral cells
void vtkIntegrationLinearStrategy::IntegrateVoxel(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkIdType cellId, vtkIdType vtkNotUsed(numPts), const vtkIdType* cellPtIds, double& sum,
  double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id, pt5Id;
  double pts[5][3];
  pt1Id = cellPtIds[0];
  pt2Id = cellPtIds[1];
  pt3Id = cellPtIds[2];
  pt4Id = cellPtIds[3];
  pt5Id = cellPtIds[4];
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);
  input->GetPoint(pt5Id, pts[4]);

  double length, width, height, volume, mid[3];

  // Calculate the volume of the voxel
  length = pts[1][0] - pts[0][0];
  width = pts[2][1] - pts[0][1];
  height = pts[4][2] - pts[0][2];
  volume = std::abs(length * width * height);
  sum += volume;

  // Partially Compute the middle, which is really just another attribute.
  mid[0] = (pts[0][0] + pts[1][0] + pts[2][0] + pts[3][0]) * 0.125;
  mid[1] = (pts[0][1] + pts[1][1] + pts[2][1] + pts[3][1]) * 0.125;
  mid[2] = (pts[0][2] + pts[1][2] + pts[2][2] + pts[3][2]) * 0.125;

  // Integrate the attributes on the cell itself
  this->IntegrateData1(
    input->GetCellData(), output->GetCellData(), cellId, volume, cellFieldList, index);

  // Integrate the attributes associated with the points on the bottom face
  // note that since IntegrateData4 is going to weigh everything by 1/4
  // we need to pass down 1/2 the volume so they will be weighted by 1/8

  this->IntegrateData4(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, pt4Id,
    volume * 0.5, pointFieldList, index);

  // Now process the top face points
  pt1Id = cellPtIds[5];
  pt2Id = cellPtIds[6];
  pt3Id = cellPtIds[7];
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  // Finish Computing the middle, which is really just another attribute.
  mid[0] += (pts[0][0] + pts[1][0] + pts[2][0] + pts[4][0]) * 0.125;
  mid[1] += (pts[0][1] + pts[1][1] + pts[2][1] + pts[4][1]) * 0.125;
  mid[2] += (pts[0][2] + pts[1][2] + pts[2][2] + pts[4][2]) * 0.125;

  // Add weighted to sumCenter.
  sumCenter[0] += mid[0] * volume;
  sumCenter[1] += mid[1] * volume;
  sumCenter[2] += mid[2] * volume;

  // Integrate the attributes associated with the points on the top face
  // note that since IntegrateData4 is going to weigh everything by 1/4
  // we need to pass down 1/2 the volume so they will be weighted by 1/8
  this->IntegrateData4(input->GetPointData(), output->GetPointData(), pt1Id, pt2Id, pt3Id, pt5Id,
    volume * 0.5, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateHexahedron(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkGenericCell* cell, vtkIdType cellId, vtkIdType numPts,
  const vtkIdType* vtkNotUsed(cellPtIds), vtkIdList* cellPtIdsList, double& sum,
  double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  this->IntegrateDefault(input, output, cell, cellId, numPts, cellPtIdsList, sum, sumCenter,
    cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateWedge(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkGenericCell* cell, vtkIdType cellId, vtkIdType numPts, const vtkIdType* vtkNotUsed(cellPtIds),
  vtkIdList* cellPtIdsList, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  this->IntegrateDefault(input, output, cell, cellId, numPts, cellPtIdsList, sum, sumCenter,
    cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegratePyramid(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkGenericCell* cell, vtkIdType cellId, vtkIdType numPts, const vtkIdType* vtkNotUsed(cellPtIds),
  vtkIdList* cellPtIdsList, double& sum, double sumCenter[3],
  vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
  int index)
{
  this->IntegrateDefault(input, output, cell, cellId, numPts, cellPtIdsList, sum, sumCenter,
    cellFieldList, pointFieldList, index);
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateGeneral1DCell(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  if (numPts % 2)
  {
    vtkWarningMacro("Odd number of points(" << numPts << ")  encountered - skipping "
                                            << " 1D Cell: " << cellId);
    return;
  }

  double length;
  double pt1[3], pt2[3], mid[3];

  for (vtkIdType pid = 0; pid < numPts; pid += 2)
  {
    input->GetPoint(cellPtIds[pid], pt1);
    input->GetPoint(cellPtIds[pid + 1], pt2);

    // Compute the length of the line.
    length = std::sqrt(vtkMath::Distance2BetweenPoints(pt1, pt2));
    sum += length;

    // Compute the middle, which is really just another attribute.
    mid[0] = (pt1[0] + pt2[0]) * 0.5;
    mid[1] = (pt1[1] + pt2[1]) * 0.5;
    mid[2] = (pt1[2] + pt2[2]) * 0.5;
    // Add weighted to sumCenter.
    sumCenter[0] += mid[0] * length;
    sumCenter[1] += mid[1] * length;
    sumCenter[2] += mid[2] * length;

    // Now integrate the rest of the attributes.
    this->IntegrateData2(input->GetPointData(), output->GetPointData(), cellPtIds[pid],
      cellPtIds[pid + 1], length, pointFieldList, index);
    this->IntegrateData1(
      input->GetCellData(), output->GetCellData(), cellId, length, cellFieldList, index);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateGeneral2DCell(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  if (numPts % 3)
  {
    vtkWarningMacro("Number of points (" << numPts << ") is not divisible by 3 - skipping "
                                         << " 2D Cell: " << cellId);
    return;
  }

  vtkIdType pt1Id, pt2Id, pt3Id;
  for (vtkIdType triIdx = 0; triIdx < numPts;)
  {
    pt1Id = cellPtIds[triIdx++];
    pt2Id = cellPtIds[triIdx++];
    pt3Id = cellPtIds[triIdx++];
    this->IntegrateTriangle(input, output, cellId, pt1Id, pt2Id, pt3Id, sum, sumCenter,
      cellFieldList, pointFieldList, index);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateGeneral3DCell(vtkDataSet* input,
  vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
  double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  if (numPts % 4)
  {
    vtkWarningMacro("Number of points (" << numPts << ") is not divisiable by 4 - skipping "
                                         << " 3D Cell: " << cellId);
    return;
  }

  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id;
  for (vtkIdType tetIdx = 0; tetIdx < numPts;)
  {
    pt1Id = cellPtIds[tetIdx++];
    pt2Id = cellPtIds[tetIdx++];
    pt3Id = cellPtIds[tetIdx++];
    pt4Id = cellPtIds[tetIdx++];
    this->IntegrateTetrahedron(input, output, cellId, pt1Id, pt2Id, pt3Id, pt4Id, sum, sumCenter,
      cellFieldList, pointFieldList, index);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateDefault(vtkDataSet* input, vtkUnstructuredGrid* output,
  vtkGenericCell* cell, vtkIdType cellId, vtkIdType numPts, vtkIdList* cellPtIds, double& sum,
  double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
  vtkIntegrateAttributesFieldList& pointFieldList, int index)
{
  // We need to explicitly get the cell
  input->GetCell(cellId, cell);

  cell->TriangulateIds(1, cellPtIds);

  int cellType = input->GetCellType(cellId);
  int cellDim = vtkCellTypes::GetDimension(cellType);
  switch (cellDim)
  {
    case 1:
      this->IntegrateGeneral1DCell(input, output, cellId, numPts, cellPtIds->GetPointer(0), sum,
        sumCenter, cellFieldList, pointFieldList, index);
      break;
    case 2:
      this->IntegrateGeneral2DCell(input, output, cellId, cellPtIds->GetNumberOfIds(),
        cellPtIds->GetPointer(0), sum, sumCenter, cellFieldList, pointFieldList, index);
      break;
    case 3:
      this->IntegrateGeneral3DCell(input, output, cellId, cellPtIds->GetNumberOfIds(),
        cellPtIds->GetPointer(0), sum, sumCenter, cellFieldList, pointFieldList, index);
      break;
    default:
      vtkWarningMacro("Unsupported Cell Dimension = " << cellDim);
  }
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateData1(vtkDataSetAttributes* inda,
  vtkDataSetAttributes* outda, vtkIdType pt1Id, double volume,
  vtkIntegrateAttributesFieldList& fieldList, int index)
{
  auto f = [pt1Id, volume](vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray)
  {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double dv = vIn1;
        const double vOut = (dv * volume) + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
}

//------------------------------------------------------------------------------
void vtkIntegrationLinearStrategy::IntegrateData2(vtkDataSetAttributes* inda,
  vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id, double volume,
  vtkIntegrateAttributesFieldList& fieldList, int index)
{
  auto f = [pt1Id, pt2Id, volume](vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray)
  {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double dv = 0.5 * (vIn1 + vIn2);
        const double vOut = (dv * volume) + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
}

//------------------------------------------------------------------------------
// Is the extra performance worth duplicating this code with IntergrateData2.
void vtkIntegrationLinearStrategy::IntegrateData3(vtkDataSetAttributes* inda,
  vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double volume,
  vtkIntegrateAttributesFieldList& fieldList, int index)
{
  auto f = [pt1Id, pt2Id, pt3Id, volume](vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray)
  {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double vIn3 = inArray->GetComponent(pt3Id, j);
        const double dv = (vIn1 + vIn2 + vIn3) / 3.0;
        const double vOut = (dv * volume) + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };
  fieldList.TransformData(index, inda, outda, f);
}

//------------------------------------------------------------------------------
// Is the extra performance worth duplicating this code with IntergrateData2.
void vtkIntegrationLinearStrategy::IntegrateData4(vtkDataSetAttributes* inda,
  vtkDataSetAttributes* outda, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id,
  double volume, vtkIntegrateAttributesFieldList& fieldList, int index)
{
  auto f = [pt1Id, pt2Id, pt3Id, pt4Id, volume](
             vtkAbstractArray* ainArray, vtkAbstractArray* aoutArray)
  {
    vtkDataArray* inArray = vtkDataArray::FastDownCast(ainArray);
    vtkDoubleArray* outArray = vtkDoubleArray::FastDownCast(aoutArray);
    if (inArray && outArray)
    {
      // We could template for speed.
      const int numComponents = inArray->GetNumberOfComponents();
      for (int j = 0; j < numComponents; ++j)
      {
        const double vIn1 = inArray->GetComponent(pt1Id, j);
        const double vIn2 = inArray->GetComponent(pt2Id, j);
        const double vIn3 = inArray->GetComponent(pt3Id, j);
        const double vIn4 = inArray->GetComponent(pt4Id, j);
        const double dv = (vIn1 + vIn2 + vIn3 + vIn4) * 0.25;
        const double vOut = (dv * volume) + outArray->GetTypedComponent(0, j);
        outArray->SetTypedComponent(0, j, vOut);
      }
    }
  };

  fieldList.TransformData(index, inda, outda, f);
}

VTK_ABI_NAMESPACE_END
