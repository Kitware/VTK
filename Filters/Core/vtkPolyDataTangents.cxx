// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataTangents.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"

#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
struct TangentComputation
{
  TangentComputation(vtkIdType offset, vtkPoints* points, vtkCellArray* triangles,
    vtkDataArray* tcoords, vtkDataArray* tangents, vtkCellData* inCD, vtkCellData* outCD,
    vtkPolyDataTangents* filter)
  {
    this->Offset = offset;
    this->Points = points;
    this->Triangles = triangles;
    this->TCoords = tcoords;
    this->Tangents = tangents;
    this->InCD = inCD;
    this->OutCD = outCD;
    this->Filter = filter;
  }

  void operator()(vtkIdType beginId, vtkIdType endId)
  {
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endId - beginId) / 10 + 1, (vtkIdType)1000);
    for (vtkIdType cellId = beginId; cellId < endId; cellId++)
    {
      if (cellId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->Filter->CheckAbort();
        }
        if (this->Filter->GetAbortOutput())
        {
          break;
        }
      }
      double tangent[3];

      if (cellId >= this->Offset)
      {
        vtkIdType npts;
        const vtkIdType* pts;
        this->Triangles->GetCellAtId(cellId, npts, pts);

        // compute edges
        double v1[3], v2[3], v3[3];

        this->Points->GetPoint(pts[0], v1);
        this->Points->GetPoint(pts[1], v2);
        this->Points->GetPoint(pts[2], v3);

        double ax, ay, az, bx, by, bz;
        ax = v3[0] - v2[0];
        ay = v3[1] - v2[1];
        az = v3[2] - v2[2];
        bx = v1[0] - v2[0];
        by = v1[1] - v2[1];
        bz = v1[2] - v2[2];

        // compute uv direction
        double uv1[2], uv2[2], uv3[2];

        this->TCoords->GetTuple(pts[0], uv1);
        this->TCoords->GetTuple(pts[1], uv2);
        this->TCoords->GetTuple(pts[2], uv3);

        double dUV1_x = uv3[0] - uv2[0];
        double dUV1_y = uv3[1] - uv2[1];
        double dUV2_x = uv1[0] - uv2[0];
        double dUV2_y = uv1[1] - uv2[1];

        double f = 1.0 / (dUV1_x * dUV2_y - dUV2_x * dUV1_y);

        tangent[0] = f * (dUV2_y * ax - dUV1_y * bx);
        tangent[1] = f * (dUV2_y * ay - dUV1_y * by);
        tangent[2] = f * (dUV2_y * az - dUV1_y * bz);
      }
      else
      {
        tangent[0] = 1.0;
        tangent[1] = 0.0;
        tangent[2] = 0.0;
      }

      this->Tangents->SetTuple(cellId, tangent);
      this->OutCD->CopyData(this->InCD, cellId, cellId);
    }
  }

private:
  vtkPoints* Points;
  vtkCellArray* Triangles;
  vtkDataArray* TCoords;
  vtkDataArray* Tangents;
  vtkCellData *InCD, *OutCD;
  vtkIdType Offset;
  vtkPolyDataTangents* Filter;
};

vtkStandardNewMacro(vtkPolyDataTangents);

//------------------------------------------------------------------------------
int vtkPolyDataTangents::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  if (input->GetPointData()->GetTCoords() == nullptr)
  {
    vtkErrorMacro("Texture coordinates are requested to generate tangents.");
    return 0;
  }

  vtkPoints* inPts = input->GetPoints();
  vtkCellArray* inPolys = input->GetPolys();
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();

  vtkDataArray* tcoords = inPD->GetTCoords();

  vtkIdType numPolys = input->GetNumberOfPolys();

  vtkIdType largestCellSize = input->GetPolys()->GetMaxCellSize();
  if (largestCellSize != 3 || 3 * numPolys != inPolys->GetNumberOfConnectivityIds())
  {
    vtkErrorMacro("This filter only supports triangles, triangulate first.");
    return 0;
  }

  if (input->GetNumberOfStrips() > 0)
  {
    vtkErrorMacro("This filter does not support strips, use the triangulate filter first.");
    return 0;
  }

  if (input->GetNumberOfLines() > 0)
  {
    vtkErrorMacro("This filter only supports triangles, remove lines first.");
    return 0;
  }

  vtkIdType numVerts = input->GetNumberOfVerts();

  //  Initial pass to compute polygon tangents without effects of neighbors
  vtkIdType outNumCell = numVerts + numPolys;
  vtkNew<vtkFloatArray> cellTangents;
  cellTangents->SetNumberOfComponents(3);
  cellTangents->SetName("Tangents");
  cellTangents->SetNumberOfTuples(outNumCell);

  outCD->CopyAllocate(inCD, outNumCell);
  // Threads will fight over array MaxId unless we set it beforehand
  for (int i = 0; i < outCD->GetNumberOfArrays(); ++i)
  {
    outCD->GetArray(i)->SetNumberOfTuples(outNumCell);
  }

  TangentComputation functor(numVerts, inPts, inPolys, tcoords, cellTangents, inCD, outCD, this);
  vtkSMPTools::For(0, numVerts + numPolys, functor);

  outPD->PassData(inPD);

  this->UpdateProgress(0.8);

  vtkIdType numPts = input->GetNumberOfPoints();

  vtkNew<vtkFloatArray> pointTangents;
  pointTangents->SetNumberOfComponents(3);
  pointTangents->SetNumberOfTuples(numPts);
  pointTangents->SetName("Tangents");
  float* fTangents = pointTangents->GetPointer(0);
  std::fill_n(fTangents, 3 * numPts, 0);

  float* fCellTangents = cellTangents->GetPointer(0);

  if (this->ComputePointTangents)
  {
    vtkIdType cellId = 0;
    vtkIdType npts;
    const vtkIdType* pts;
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts); ++cellId)
    {
      for (vtkIdType i = 0; i < npts; ++i)
      {
        fTangents[3 * pts[i]] += fCellTangents[3 * cellId];
        fTangents[3 * pts[i] + 1] += fCellTangents[3 * cellId + 1];
        fTangents[3 * pts[i] + 2] += fCellTangents[3 * cellId + 2];
      }
    }

    for (vtkIdType i = 0; i < numPts; ++i)
    {
      vtkMath::Normalize(fTangents + 3 * i);
    }

    outPD->SetTangents(pointTangents);
  }

  output->SetPoints(inPts);

  if (this->ComputeCellTangents)
  {
    output->GetCellData()->SetTangents(cellTangents);
  }

  output->SetPolys(inPolys);

  // copy the original vertices and lines to the output
  output->SetVerts(input->GetVerts());

  return 1;
}

//------------------------------------------------------------------------------
void vtkPolyDataTangents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Compute Point Tangents: " << (this->ComputePointTangents ? "On\n" : "Off\n");
  os << indent << "Compute Cell Tangents: " << (this->ComputeCellTangents ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
