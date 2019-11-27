/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataPointSampler.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataPointSampler.h"

#include "vtkCellArray.h"
#include "vtkEdgeTable.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkPolyDataPointSampler);

//------------------------------------------------------------------------
vtkPolyDataPointSampler::vtkPolyDataPointSampler()
{
  this->Distance = 0.01;
  this->Distance2 = this->Distance * this->Distance;

  this->GenerateVertexPoints = true;
  this->GenerateEdgePoints = true;
  this->GenerateInteriorPoints = true;
  this->GenerateVertices = true;

  this->InterpolatePointData = false;

  this->TriIds->SetNumberOfIds(3);
  this->QuadIds->SetNumberOfIds(4);
}

//------------------------------------------------------------------------
int vtkPolyDataPointSampler::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Initialize
  vtkDebugMacro(<< "Resampling polygonal data");

  if (this->Distance <= 0.0)
  {
    vtkWarningMacro("Cannot resample to zero distance\n");
    return 1;
  }
  if (!input || !input->GetPoints() ||
    (!this->GenerateVertexPoints && !this->GenerateEdgePoints && !this->GenerateInteriorPoints))
  {
    return 1;
  }
  vtkPoints* inPts = input->GetPoints();
  vtkIdType numInputPts = input->GetNumberOfPoints();

  // If requested, interpolate point data
  vtkPointData *inPD = nullptr, *outPD = nullptr;
  if (this->InterpolatePointData)
  {
    inPD = input->GetPointData();
    outPD = output->GetPointData();
    outPD->CopyAllocate(inPD);
  }

  // Prepare output
  vtkIdType i;
  const vtkIdType* pts;
  vtkIdType npts;
  vtkPoints* newPts = input->GetPoints()->NewInstance();
  this->Distance2 = this->Distance * this->Distance;
  if (this->GenerateEdgePoints)
  {
    this->EdgeTable->InitEdgeInsertion(numInputPts);
  }

  // First the vertex points
  if (this->GenerateVertexPoints)
  {
    newPts->DeepCopy(inPts);
    if (inPD)
    {
      for (i = 0; i < numInputPts; ++i)
      {
        outPD->CopyData(inPD, i, i);
      }
    }
  }
  this->UpdateProgress(0.1);
  int abort = this->GetAbortExecute();

  // Now the edge points
  vtkCellArray* inPolys = input->GetPolys();
  vtkCellArray* inStrips = input->GetStrips();
  if (this->GenerateEdgePoints && !abort)
  {
    vtkCellArray* inLines = input->GetLines();

    for (inLines->InitTraversal(); inLines->GetNextCell(npts, pts);)
    {
      for (i = 0; i < (npts - 1); i++)
      {
        if (this->EdgeTable->IsEdge(pts[i], pts[i + 1]) == -1)
        {
          this->EdgeTable->InsertEdge(pts[i], pts[i + 1]);
          this->SampleEdge(newPts, pts[i], pts[i + 1], inPD, outPD);
        }
      }
    }

    this->UpdateProgress(0.2);
    abort = this->GetAbortExecute();

    vtkIdType p0, p1;
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
    {
      for (i = 0; i < npts; i++)
      {
        p0 = pts[i];
        p1 = pts[(i + 1) % npts];
        if (this->EdgeTable->IsEdge(p0, p1) == -1)
        {
          this->EdgeTable->InsertEdge(p0, p1);
          this->SampleEdge(newPts, p0, p1, inPD, outPD);
        }
      }
    }

    this->UpdateProgress(0.3);
    abort = this->GetAbortExecute();

    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
    {
      // The first triangle
      for (i = 0; i < 3; i++)
      {
        p0 = pts[i];
        p1 = pts[(i + 1) % 3];
        if (this->EdgeTable->IsEdge(p0, p1) == -1)
        {
          this->EdgeTable->InsertEdge(p0, p1);
          this->SampleEdge(newPts, p0, p1, inPD, outPD);
        }
      }
      // Now the other triangles
      for (i = 3; i < npts; i++)
      {
        p0 = pts[i - 2];
        p1 = pts[i];
        if (this->EdgeTable->IsEdge(p0, p1) == -1)
        {
          this->EdgeTable->InsertEdge(p0, p1);
          this->SampleEdge(newPts, p0, p1, inPD, outPD);
        }
        p0 = pts[i - 1];
        p1 = pts[i];
        if (this->EdgeTable->IsEdge(p0, p1) == -1)
        {
          this->EdgeTable->InsertEdge(p0, p1);
          this->SampleEdge(newPts, p0, p1, inPD, outPD);
        }
      }
    }
  }

  this->UpdateProgress(0.5);
  abort = this->GetAbortExecute();

  // Finally the interior points on polygons and triangle strips
  if (this->GenerateInteriorPoints && !abort)
  {
    // First the polygons
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
    {
      if (npts == 3)
      {
        this->SampleTriangle(newPts, inPts, pts, inPD, outPD);
      }
      else
      {
        this->SamplePolygon(newPts, inPts, npts, pts, inPD, outPD);
      }
    }

    this->UpdateProgress(0.75);
    abort = this->GetAbortExecute();

    // Next the triangle strips
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts) && !abort;)
    {
      vtkIdType stripPts[3];
      for (i = 0; i < (npts - 2); i++)
      {
        stripPts[0] = pts[i];
        stripPts[1] = pts[i + 1];
        stripPts[2] = pts[i + 2];
        this->SampleTriangle(newPts, inPts, stripPts, inPD, outPD);
      }
    } // for all strips
  }   // for interior points

  this->UpdateProgress(0.90);
  abort = this->GetAbortExecute();

  // Generate vertex cells if requested
  if (this->GenerateVertices && !abort)
  {
    vtkIdType id, numPts = newPts->GetNumberOfPoints();
    vtkCellArray* verts = vtkCellArray::New();
    verts->AllocateEstimate(numPts + 1, 1);
    verts->InsertNextCell(numPts);
    for (id = 0; id < numPts; id++)
    {
      verts->InsertCellPoint(id);
    }
    output->SetVerts(verts);
    verts->Delete();
  }

  // Clean up and get out
  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

//------------------------------------------------------------------------
void vtkPolyDataPointSampler::SampleEdge(
  vtkPoints* pts, vtkIdType p0, vtkIdType p1, vtkPointData* inPD, vtkPointData* outPD)
{
  double x0[3], x1[3];
  pts->GetPoint(p0, x0);
  pts->GetPoint(p1, x1);
  double len = vtkMath::Distance2BetweenPoints(x0, x1);
  if (len > this->Distance2)
  {
    vtkIdType pId;
    double t, x[3];
    len = sqrt(len);
    int npts = static_cast<int>(len / this->Distance) + 2;
    for (vtkIdType id = 1; id < (npts - 1); id++)
    {
      t = static_cast<double>(id) / (npts - 1);
      x[0] = x0[0] + t * (x1[0] - x0[0]);
      x[1] = x0[1] + t * (x1[1] - x0[1]);
      x[2] = x0[2] + t * (x1[2] - x0[2]);
      pId = pts->InsertNextPoint(x);
      if (inPD)
      {
        outPD->InterpolateEdge(inPD, pId, p0, p1, t);
      }
    }
  }
}

//------------------------------------------------------------------------
void vtkPolyDataPointSampler::SampleTriangle(vtkPoints* newPts, vtkPoints* inPts,
  const vtkIdType* pts, vtkPointData* inPD, vtkPointData* outPD)
{
  double x0[3], x1[3], x2[3];
  inPts->GetPoint(pts[0], x0);
  inPts->GetPoint(pts[1], x1);
  inPts->GetPoint(pts[2], x2);

  double l1 = vtkMath::Distance2BetweenPoints(x0, x1);
  double l2 = vtkMath::Distance2BetweenPoints(x0, x2);
  if (l1 > this->Distance2 || l2 > this->Distance2)
  {
    vtkIdType pId;
    double s, t, x[3];
    if (inPD)
    {
      this->TriIds->SetId(0, pts[0]);
      this->TriIds->SetId(1, pts[1]);
      this->TriIds->SetId(2, pts[2]);
    }

    l1 = sqrt(l1);
    l2 = sqrt(l2);
    int n1 = static_cast<int>(l1 / this->Distance) + 2;
    int n2 = static_cast<int>(l2 / this->Distance) + 2;
    n1 = (n1 < 3 ? 3 : n1); // make sure there is at least one point
    n2 = (n2 < 3 ? 3 : n2);
    for (vtkIdType j = 1; j < (n2 - 1); j++)
    {
      t = static_cast<double>(j) / (n2 - 1);
      for (vtkIdType i = 1; i < (n1 - 1); i++)
      {
        s = static_cast<double>(i) / (n1 - 1);
        if ((1.0 - s - t) > 0.0)
        {
          x[0] = x0[0] + s * (x1[0] - x0[0]) + t * (x2[0] - x0[0]);
          x[1] = x0[1] + s * (x1[1] - x0[1]) + t * (x2[1] - x0[1]);
          x[2] = x0[2] + s * (x1[2] - x0[2]) + t * (x2[2] - x0[2]);
          pId = newPts->InsertNextPoint(x);
          if (inPD)
          {
            this->TriWeights[0] = 1.0 - s - t;
            this->TriWeights[1] = s;
            this->TriWeights[2] = t;
            outPD->InterpolatePoint(inPD, pId, this->TriIds, this->TriWeights);
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------
void vtkPolyDataPointSampler::SamplePolygon(vtkPoints* newPts, vtkPoints* inPts, vtkIdType npts,
  const vtkIdType* pts, vtkPointData* inPD, vtkPointData* outPD)
{
  // Specialize for quads
  if (npts == 4)
  {
    double x0[3], x1[3], x2[3], x3[4];
    inPts->GetPoint(pts[0], x0);
    inPts->GetPoint(pts[1], x1);
    inPts->GetPoint(pts[2], x2);
    inPts->GetPoint(pts[3], x3);

    double l1 = vtkMath::Distance2BetweenPoints(x0, x1);
    double l2 = vtkMath::Distance2BetweenPoints(x0, x3);
    if (l1 > this->Distance2 || l2 > this->Distance2)
    {
      vtkIdType pId;
      double s, t, x[3];
      if (inPD)
      {
        this->QuadIds->SetId(0, pts[0]);
        this->QuadIds->SetId(1, pts[1]);
        this->QuadIds->SetId(2, pts[2]);
        this->QuadIds->SetId(3, pts[3]);
      }

      l1 = sqrt(l1);
      l2 = sqrt(l2);
      int n1 = static_cast<int>(l1 / this->Distance) + 2;
      int n2 = static_cast<int>(l2 / this->Distance) + 2;
      n1 = (n1 < 3 ? 3 : n1); // make sure there is at least one point
      n2 = (n2 < 3 ? 3 : n2);
      for (vtkIdType j = 1; j < (n2 - 1); j++)
      {
        t = static_cast<double>(j) / (n2 - 1);
        for (vtkIdType i = 1; i < (n1 - 1); i++)
        {
          s = static_cast<double>(i) / (n1 - 1);
          x[0] = x0[0] + s * (x1[0] - x0[0]) + t * (x3[0] - x0[0]);
          x[1] = x0[1] + s * (x1[1] - x0[1]) + t * (x3[1] - x0[1]);
          x[2] = x0[2] + s * (x1[2] - x0[2]) + t * (x3[2] - x0[2]);
          pId = newPts->InsertNextPoint(x);
          if (inPD)
          {
            this->QuadWeights[0] = (1.0 - s) * (1.0 - t);
            this->QuadWeights[1] = s * (1.0 - t);
            this->QuadWeights[2] = s * t;
            this->QuadWeights[3] = (1.0 - s) * t;
            outPD->InterpolatePoint(inPD, pId, this->QuadIds, this->QuadWeights);
          }
        }
      }
    }
  } // if quad

  // Otherwise perform simple fan triangulation, and process each triangle
  // for polygons with more than 4 sides. Also have to sample fan edges if
  // GenerateEdgePoints is enabled.
  else
  {
    vtkIdType triPts[3];
    for (vtkIdType i = 0; i < (npts - 2); ++i)
    {
      triPts[0] = pts[0];
      triPts[1] = pts[i + 1];
      triPts[2] = pts[i + 2];
      if (this->GenerateEdgePoints && this->EdgeTable->IsEdge(triPts[0], triPts[2]) == -1)
      {
        this->EdgeTable->InsertEdge(triPts[0], triPts[2]);
        this->SampleEdge(newPts, triPts[0], triPts[2], inPD, outPD);
      }
      this->SampleTriangle(newPts, inPts, triPts, inPD, outPD);
    }
  } // if polygon npts > 4
}

//------------------------------------------------------------------------
void vtkPolyDataPointSampler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Distance: " << this->Distance << "\n";

  os << indent << "Generate Vertex Points: " << (this->GenerateVertexPoints ? "On\n" : "Off\n");
  os << indent << "Generate Edge Points: " << (this->GenerateEdgePoints ? "On\n" : "Off\n");
  os << indent << "Generate Interior Points: " << (this->GenerateInteriorPoints ? "On\n" : "Off\n");

  os << indent << "Generate Vertices: " << (this->GenerateVertices ? "On\n" : "Off\n");

  os << indent << "Interpolate Point Data: " << (this->GenerateVertices ? "On\n" : "Off\n");
}
