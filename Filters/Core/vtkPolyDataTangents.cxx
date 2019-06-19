/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataTangents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkPriorityQueue.h"
#include "vtkTriangleStrip.h"

#include "vtkSMPTools.h"

struct TangentComputation
{
  TangentComputation(vtkIdType offset, vtkPoints* points, vtkCellArray* triangles,
    vtkDataArray* tcoords, vtkDataArray* tangents)
  {
    this->Points = points;
    this->Triangles = triangles;
    this->TCoords = tcoords;
    this->Tangents = tangents;
    this->Offset = offset;
  }

  void operator()(vtkIdType beginId, vtkIdType endId)
  {
    for (vtkIdType cellId = beginId; cellId < endId; cellId++)
    {
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
    }
  }

private:
  vtkPoints* Points;
  vtkCellArray* Triangles;
  vtkDataArray* TCoords;
  vtkDataArray* Tangents;
  vtkIdType Offset;
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
  vtkPointData* pd = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();

  vtkDataArray* tcoords = pd->GetTCoords();

  vtkIdType numPolys = input->GetNumberOfPolys();

  if (3 * numPolys != inPolys->GetNumberOfConnectivityIds() || input->GetNumberOfStrips() > 0)
  {
    vtkErrorMacro("This filter only supports triangles, triangulate first.");
    return 0;
  }

  vtkIdType numVerts = input->GetNumberOfVerts();
  vtkIdType numLines = input->GetNumberOfLines();

  //  Initial pass to compute polygon tangents without effects of neighbors
  vtkNew<vtkFloatArray> cellTangents;
  cellTangents->SetNumberOfComponents(3);
  cellTangents->SetName("Tangents");
  cellTangents->SetNumberOfTuples(numVerts + numLines + numPolys);

  TangentComputation functor(numVerts + numLines, inPts, inPolys, tcoords, cellTangents);

  vtkSMPTools::For(0, numVerts + numLines + numPolys, functor);

  outPD->PassData(pd);

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
  output->SetLines(input->GetLines());

  return 1;
}

//------------------------------------------------------------------------------
void vtkPolyDataTangents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Compute Point Tangents: " << (this->ComputePointTangents ? "On\n" : "Off\n");
  os << indent << "Compute Cell Tangents: " << (this->ComputeCellTangents ? "On\n" : "Off\n");
}
