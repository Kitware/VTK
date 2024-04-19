// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataPointSampler.h"

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkEdgeTable.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTriangle.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyDataPointSampler);

namespace
{
// Superclass for generating points
struct GeneratePoints
{
  vtkPolyDataPointSampler* Self;
  double Distance;
  vtkIdType NumPts;
  vtkPoints* InPts;
  vtkPointData* InPD;
  vtkCellArray* InLines;
  vtkCellArray* InPolys;
  vtkCellArray* InStrips;
  vtkPoints* OutPts;
  vtkPointData* OutPD;

  // Internal scratch arrays supporting point data interpolation, and
  // sampling edges.
  bool GenerateEdgePoints;
  double Distance2;
  vtkSmartPointer<vtkEdgeTable> EdgeTable;
  double TriWeights[3];
  vtkNew<vtkIdList> TriIds;
  double QuadWeights[4];
  vtkNew<vtkIdList> QuadIds;

  GeneratePoints(vtkPolyDataPointSampler* self, double d, vtkIdType numPts, vtkPoints* inPts,
    vtkPoints* newPts, vtkPointData* inPD, vtkPointData* outPD, vtkCellArray* inLines,
    vtkCellArray* inPolys, vtkCellArray* inStrips)
    : Self(self)
    , Distance(d)
    , NumPts(numPts)
    , InPts(inPts)
    , InPD(inPD)
    , InLines(inLines)
    , InPolys(inPolys)
    , InStrips(inStrips)
    , OutPts(newPts)
    , OutPD(outPD)
  {
    this->Distance2 = this->Distance * this->Distance;
    this->TriIds->SetNumberOfIds(3);
    this->QuadIds->SetNumberOfIds(4);
    if (Self->GetGenerateEdgePoints())
    {
      this->EdgeTable.TakeReference(vtkEdgeTable::New());
      this->EdgeTable->InitEdgeInsertion(this->NumPts);
    }
  }
  virtual ~GeneratePoints() = default;

  // Driver function
  int operator()()
  {
    vtkIdType npts;
    const vtkIdType* pts;

    // Vertices if requested
    if (Self->GetGenerateVertexPoints())
    {
      this->SamplePoints();
    }
    Self->UpdateProgress(0.1);
    bool abort = Self->CheckAbort();

    // Now the edge points
    if (Self->GetGenerateEdgePoints() && !abort)
    {
      auto iter = vtk::TakeSmartPointer(this->InLines->NewIterator());
      for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
      {
        if (Self->CheckAbort())
        {
          break;
        }
        iter->GetCurrentCell(npts, pts);
        for (auto i = 0; i < (npts - 1); i++)
        {
          if (this->EdgeTable->IsEdge(pts[i], pts[i + 1]) == -1)
          {
            this->EdgeTable->InsertEdge(pts[i], pts[i + 1]);
            this->SampleEdge(pts[i], pts[i + 1]);
          }
        }
      }
      Self->UpdateProgress(0.2);
      abort = Self->CheckAbort();

      vtkIdType p0, p1;
      iter = vtk::TakeSmartPointer(this->InPolys->NewIterator());
      for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
      {
        if (Self->CheckAbort())
        {
          break;
        }
        iter->GetCurrentCell(npts, pts);
        for (auto i = 0; i < npts; i++)
        {
          p0 = pts[i];
          p1 = pts[(i + 1) % npts];
          if (this->EdgeTable->IsEdge(p0, p1) == -1)
          {
            this->EdgeTable->InsertEdge(p0, p1);
            this->SampleEdge(p0, p1);
          }
        }
      }
      Self->UpdateProgress(0.3);
      abort = Self->CheckAbort();

      iter = vtk::TakeSmartPointer(this->InStrips->NewIterator());
      for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
      {
        if (Self->CheckAbort())
        {
          break;
        }
        iter->GetCurrentCell(npts, pts);
        // The first triangle
        for (auto i = 0; i < 3; i++)
        {
          p0 = pts[i];
          p1 = pts[(i + 1) % 3];
          if (this->EdgeTable->IsEdge(p0, p1) == -1)
          {
            this->EdgeTable->InsertEdge(p0, p1);
            this->SampleEdge(p0, p1);
          }
        }
        // Now the other triangles
        for (auto i = 3; i < npts; i++)
        {
          p0 = pts[i - 2];
          p1 = pts[i];
          if (this->EdgeTable->IsEdge(p0, p1) == -1)
          {
            this->EdgeTable->InsertEdge(p0, p1);
            this->SampleEdge(p0, p1);
          }
          p0 = pts[i - 1];
          p1 = pts[i];
          if (this->EdgeTable->IsEdge(p0, p1) == -1)
          {
            this->EdgeTable->InsertEdge(p0, p1);
            this->SampleEdge(p0, p1);
          }
        }
      }
    }
    Self->UpdateProgress(0.5);
    abort = Self->CheckAbort();

    // Finally the interior points on polygons and triangle strips
    if (Self->GetGenerateInteriorPoints() && !abort)
    {
      // First the polygons
      auto iter = vtk::TakeSmartPointer(this->InPolys->NewIterator());
      for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
      {
        iter->GetCurrentCell(npts, pts);
        if (npts == 3)
        {
          this->SampleTriangle(pts);
        }
        else
        {
          this->SamplePolygon(npts, pts);
        }
      }
      Self->UpdateProgress(0.75);
      abort = Self->CheckAbort();

      // Next the triangle strips
      iter = vtk::TakeSmartPointer(this->InStrips->NewIterator());
      for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
      {
        iter->GetCurrentCell(npts, pts);
        vtkIdType stripPts[3];
        for (auto i = 0; i < (npts - 2); i++)
        {
          stripPts[0] = pts[i];
          stripPts[1] = pts[i + 1];
          stripPts[2] = pts[i + 2];
          this->SampleTriangle(stripPts);
        }
      } // for all strips
    }   // for interior points
    return abort;
  }

  // Internal methods for sampling edges, triangles, and polygons
  // Subclasses must implement these.
  virtual void SamplePoints() = 0;
  virtual void SampleEdge(vtkIdType p0, vtkIdType p1) = 0;
  virtual void SampleTriangle(const vtkIdType* pts) = 0;
  virtual void SamplePolygon(vtkIdType npts, const vtkIdType* pts) = 0;
};

// Generate regular points
struct RegularGeneration : public GeneratePoints
{
  RegularGeneration(vtkPolyDataPointSampler* self, double d, vtkIdType numPts, vtkPoints* inPts,
    vtkPoints* newPts, vtkPointData* inPD, vtkPointData* outPD, vtkCellArray* inLines,
    vtkCellArray* inPolys, vtkCellArray* inStrips)
    : GeneratePoints(self, d, numPts, inPts, newPts, inPD, outPD, inLines, inPolys, inStrips)
  {
  }

  // Internal methods for sampling points, edges, triangles, and polygons
  void SamplePoints() override
  {
    // First the vertex points
    this->OutPts->DeepCopy(this->InPts);
    if (this->InPD)
    {
      for (auto i = 0; i < this->NumPts; ++i)
      {
        this->OutPD->CopyData(this->InPD, i, i);
      }
    }
  }

  void SampleEdge(vtkIdType p0, vtkIdType p1) override
  {
    double x0[3], x1[3];
    this->InPts->GetPoint(p0, x0);
    this->InPts->GetPoint(p1, x1);
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
        pId = this->OutPts->InsertNextPoint(x);
        if (this->InPD)
        {
          this->OutPD->InterpolateEdge(this->InPD, pId, p0, p1, t);
        }
      }
    }
  }

  void SampleTriangle(const vtkIdType* pts) override
  {
    double x0[3], x1[3], x2[3];
    this->InPts->GetPoint(pts[0], x0);
    this->InPts->GetPoint(pts[1], x1);
    this->InPts->GetPoint(pts[2], x2);

    double l1 = vtkMath::Distance2BetweenPoints(x0, x1);
    double l2 = vtkMath::Distance2BetweenPoints(x0, x2);
    if (l1 > this->Distance2 || l2 > this->Distance2)
    {
      vtkIdType pId;
      double s, t, x[3];
      if (this->InPD)
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
            pId = this->OutPts->InsertNextPoint(x);
            if (this->InPD)
            {
              this->TriWeights[0] = 1.0 - s - t;
              this->TriWeights[1] = s;
              this->TriWeights[2] = t;
              this->OutPD->InterpolatePoint(this->InPD, pId, this->TriIds, this->TriWeights);
            }
          }
        }
      }
    }
  }

  void SamplePolygon(vtkIdType npts, const vtkIdType* pts) override
  {
    // Specialize for quads
    if (npts == 4)
    {
      double x0[3], x1[3], x2[3], x3[4];
      this->InPts->GetPoint(pts[0], x0);
      this->InPts->GetPoint(pts[1], x1);
      this->InPts->GetPoint(pts[2], x2);
      this->InPts->GetPoint(pts[3], x3);

      double l1 = vtkMath::Distance2BetweenPoints(x0, x1);
      double l2 = vtkMath::Distance2BetweenPoints(x0, x3);
      if (l1 > this->Distance2 || l2 > this->Distance2)
      {
        vtkIdType pId;
        double s, t, x[3];
        if (this->InPD)
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
            pId = this->OutPts->InsertNextPoint(x);
            if (this->InPD)
            {
              this->QuadWeights[0] = (1.0 - s) * (1.0 - t);
              this->QuadWeights[1] = s * (1.0 - t);
              this->QuadWeights[2] = s * t;
              this->QuadWeights[3] = (1.0 - s) * t;
              this->OutPD->InterpolatePoint(this->InPD, pId, this->QuadIds, this->QuadWeights);
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
        if (Self->GetGenerateEdgePoints() && this->EdgeTable->IsEdge(triPts[0], triPts[2]) == -1)
        {
          this->EdgeTable->InsertEdge(triPts[0], triPts[2]);
          this->SampleEdge(triPts[0], triPts[2]);
        }
        this->SampleTriangle(triPts);
      }
    } // if polygon npts > 4
  }
};

// Generate random points
struct RandomGeneration : public GeneratePoints
{
  double Length;
  vtkNew<vtkMinimalStandardRandomSequence> RandomSeq;

  RandomGeneration(vtkPolyDataPointSampler* self, double d, vtkIdType numPts, vtkPoints* inPts,
    vtkPoints* newPts, vtkPointData* inPD, vtkPointData* outPD, vtkCellArray* inLines,
    vtkCellArray* inPolys, vtkCellArray* inStrips)
    : GeneratePoints(self, d, numPts, inPts, newPts, inPD, outPD, inLines, inPolys, inStrips)
  {
    double bounds[6];
    this->InPts->GetBounds(bounds);
    this->Length = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
      (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
      (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
    this->Length = (this->Length <= 0.0 ? 1.0 : this->Length);
    this->RandomSeq->Initialize(1177);
  }

  // Internal methods for sampling edges, triangles, and polygons
  void SamplePoints() override
  {
    // First the vertex points
    double ranVal, frac = (this->Distance / std::pow(this->Length, 0.3333));
    for (auto i = 0; i < this->NumPts; ++i)
    {
      ranVal = this->RandomSeq->GetValue();
      this->RandomSeq->Next();
      if (ranVal <= frac)
      { // insert point
        this->OutPts->InsertNextPoint(this->InPts->GetPoint(i));
        if (this->InPD)
        {
          this->OutPD->CopyData(this->InPD, i, i);
        }
      }
    }
  }

  void SampleEdge(vtkIdType p0, vtkIdType p1) override
  {
    double x0[3], x1[3];
    this->InPts->GetPoint(p0, x0);
    this->InPts->GetPoint(p1, x1);
    double len = vtkMath::Distance2BetweenPoints(x0, x1);
    if (len > this->Distance2)
    {
      len = sqrt(len);
      vtkIdType npts = std::ceil(len / this->Distance);
      vtkIdType pId;
      double t, x[3];
      for (auto i = 0; i < npts; ++i)
      {
        t = this->RandomSeq->GetValue();
        this->RandomSeq->Next();
        x[0] = x0[0] + t * (x1[0] - x0[0]);
        x[1] = x0[1] + t * (x1[1] - x0[1]);
        x[2] = x0[2] + t * (x1[2] - x0[2]);
        pId = this->OutPts->InsertNextPoint(x);
        if (this->InPD)
        {
          this->OutPD->InterpolateEdge(this->InPD, pId, p0, p1, t);
        }
      }
    }
  }

  void SampleTriangle(const vtkIdType* pts) override
  {
    double x0[3], x1[3], x2[3];
    this->InPts->GetPoint(pts[0], x0);
    this->InPts->GetPoint(pts[1], x1);
    this->InPts->GetPoint(pts[2], x2);

    double area = vtkTriangle::TriangleArea(x0, x1, x2);
    vtkIdType npts = static_cast<vtkIdType>(std::ceil(2.0 * area / this->Distance2));
    npts *= 2; // due to triangular parametric space

    if (npts > 0)
    {
      vtkIdType pId;
      double s, t, x[3];
      if (this->InPD)
      {
        this->TriIds->SetId(0, pts[0]);
        this->TriIds->SetId(1, pts[1]);
        this->TriIds->SetId(2, pts[2]);
      }

      for (auto i = 0; i < npts; ++i)
      {
        s = this->RandomSeq->GetValue();
        this->RandomSeq->Next();
        t = this->RandomSeq->GetValue();
        this->RandomSeq->Next();
        if ((1.0 - s - t) >= 0.0)
        { // in triangle parametric space
          x[0] = x0[0] + s * (x1[0] - x0[0]) + t * (x2[0] - x0[0]);
          x[1] = x0[1] + s * (x1[1] - x0[1]) + t * (x2[1] - x0[1]);
          x[2] = x0[2] + s * (x1[2] - x0[2]) + t * (x2[2] - x0[2]);

          pId = this->OutPts->InsertNextPoint(x);
          if (this->InPD)
          {
            this->TriWeights[0] = 1.0 - s - t;
            this->TriWeights[1] = s;
            this->TriWeights[2] = t;
            this->OutPD->InterpolatePoint(this->InPD, pId, this->TriIds, this->TriWeights);
          }
        }
      }
    }
  }

  void SamplePolygon(vtkIdType npts, const vtkIdType* pts) override
  {
    vtkIdType triPts[3];
    for (vtkIdType i = 0; i < (npts - 2); ++i)
    {
      triPts[0] = pts[0];
      triPts[1] = pts[i + 1];
      triPts[2] = pts[i + 2];
      if (Self->GetGenerateEdgePoints() && this->EdgeTable->IsEdge(triPts[0], triPts[2]) == -1)
      {
        this->EdgeTable->InsertEdge(triPts[0], triPts[2]);
        this->SampleEdge(triPts[0], triPts[2]);
      }
      this->SampleTriangle(triPts);
    }
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
vtkPolyDataPointSampler::vtkPolyDataPointSampler()
{
  this->Distance = 0.01;

  this->PointGenerationMode = REGULAR_GENERATION;

  this->GenerateVertexPoints = true;
  this->GenerateEdgePoints = true;
  this->GenerateInteriorPoints = true;
  this->GenerateVertices = true;

  this->InterpolatePointData = false;
}

//------------------------------------------------------------------------------
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
  vtkIdType numPts = input->GetNumberOfPoints();

  // If requested, interpolate point data
  vtkPointData *inPD = nullptr, *outPD = nullptr;
  if (this->InterpolatePointData)
  {
    inPD = input->GetPointData();
    outPD = output->GetPointData();
    outPD->CopyAllocate(inPD);
  }

  // Prepare to generate output. Gather the input topology.
  vtkPoints* newPts = input->GetPoints()->NewInstance();
  vtkCellArray* inPolys = input->GetPolys();
  vtkCellArray* inStrips = input->GetStrips();
  vtkCellArray* inLines = input->GetLines();

  // Depending on mode, generate points
  int abort = 0;
  if (this->PointGenerationMode == REGULAR_GENERATION)
  {
    RegularGeneration gen(
      this, this->Distance, numPts, inPts, newPts, inPD, outPD, inLines, inPolys, inStrips);
    abort = gen();
  }
  else // RANDOM_GENERATION
  {
    RandomGeneration gen(
      this, this->Distance, numPts, inPts, newPts, inPD, outPD, inLines, inPolys, inStrips);
    abort = gen();
  }
  this->UpdateProgress(0.90);
  abort = this->CheckAbort();

  // Generate vertex cells if requested
  if (this->GenerateVertices && !abort)
  {
    vtkCellArray* verts = vtkCellArray::New();
    numPts = newPts->GetNumberOfPoints();
    verts->AllocateEstimate(numPts + 1, 1);
    verts->InsertNextCell(numPts);
    for (auto id = 0; id < numPts; id++)
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

//------------------------------------------------------------------------------
void vtkPolyDataPointSampler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Distance: " << this->Distance << "\n";

  os << indent << "Point Generation Mode: " << this->PointGenerationMode << "\n";

  os << indent << "Generate Vertex Points: " << (this->GenerateVertexPoints ? "On\n" : "Off\n");
  os << indent << "Generate Edge Points: " << (this->GenerateEdgePoints ? "On\n" : "Off\n");
  os << indent << "Generate Interior Points: " << (this->GenerateInteriorPoints ? "On\n" : "Off\n");

  os << indent << "Generate Vertices: " << (this->GenerateVertices ? "On\n" : "Off\n");

  os << indent << "Interpolate Point Data: " << (this->GenerateVertices ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
