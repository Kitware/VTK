/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImprintFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImprintFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkDelaunay2D.h"
#include "vtkEdgeTable.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLocator.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImprintFilter);

//------------------------------------------------------------------------------
// Instantiate object
vtkImprintFilter::vtkImprintFilter()
{
  this->Tolerance = 0.001;
  this->OutputType = MERGED_IMPRINT;

  this->CellLocator = vtkSmartPointer<vtkStaticCellLocator>::New();

  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
vtkImprintFilter::~vtkImprintFilter() = default;

//------------------------------------------------------------------------------
void vtkImprintFilter::SetTargetConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(0, algOutput);
}

//------------------------------------------------------------------------------
vtkAlgorithmOutput* vtkImprintFilter::GetTargetConnection()
{
  return this->GetInputConnection(0, 0);
}

//------------------------------------------------------------------------------
void vtkImprintFilter::SetTargetData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkImprintFilter::GetTarget()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(0, 0);
}

//------------------------------------------------------------------------------
void vtkImprintFilter::SetImprintConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
vtkAlgorithmOutput* vtkImprintFilter::GetImprintConnection()
{
  return this->GetInputConnection(1, 0);
}

//------------------------------------------------------------------------------
void vtkImprintFilter::SetImprintData(vtkDataObject* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkImprintFilter::GetImprint()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return this->GetExecutive()->GetInputData(1, 0);
}

//------------------------------------------------------------------------------
namespace
{

// Imprint points are created via point projection, and edge intersection.
// This enums assigns a classification to the points.
enum PointClassification
{
  Unknown = -1,
  Outside = 0, // points with classification <=0 are not inserted
  Interior = 1,
  OnVertex = 2,
  OnEdge = 3,
};

// Used to capture edge fragments represented by a pair of points. These
// may become constraint edges during triangulation.
using vtkEdgeType = EdgeTuple<vtkIdType, vtkIdType>; // templated on two point ids
using vtkEdgeList = std::vector<vtkEdgeType>;

// Used to track imprint edges during the process of cell edge
// intersection.
struct EmptyEdgeData
{
};
using TargetEdgeType = EdgeTuple<vtkIdType, EmptyEdgeData>;
using TargetEdgeList = std::vector<TargetEdgeType>;
using TargetEdgeLocatorType = vtkStaticEdgeLocatorTemplate<vtkIdType, EmptyEdgeData>;

// The following struct represents information relative to new points that
// are generated during the imprint operation. Points may be created from
// projection of the imprint onto target, or via intersection of the target
// and imprint edges.
struct vtkPointInfo
{
  char Classification; // Type of point
  vtkIdType PtId;      // Which target point/vertex does this map to?
  double T;            // Perimeter coordinate (if applicable)
  vtkIdType
    Cells[2]; // Which cell(s) does this point project to? <0 if misses target (e.g., Outside)
  vtkEdgeType CellEdge; // What cell edge does this point lie on? (if applicable)
  double X[3];          // Coordinates of projection

  vtkPointInfo()
    : Classification(PointClassification::Unknown)
    , PtId(-1)
    , T(0.0)
  {
    this->Cells[0] = this->Cells[1] = -1;
    this->CellEdge.V0 = this->CellEdge.V1 = -1;
  }
  vtkPointInfo(int classification, vtkIdType ptId, vtkIdType* cells, vtkIdType v0, vtkIdType v1,
    double x[3], double t)
    : Classification(classification)
    , PtId(ptId)
    , T(t)
  {
    this->Cells[0] = cells[0];
    this->Cells[1] = cells[1];
    this->CellEdge.V0 = v0;
    this->CellEdge.V1 = v1;
    this->X[0] = x[0];
    this->X[1] = x[1];
    this->X[2] = x[2];
  }
};
// Be wary of pointers to entries in the point array, since new points may be added,
// meaning points may be invalidated at unexpected times. Hence access should be via ids.
// Note the vtkPointArray keeps track of new points due to the imprint.
using vtkPointArray = std::vector<vtkPointInfo>; // keep track of projected and intersected points
using vtkPointList =
  std::vector<vtkIdType>; // list of points (accessed via id) contained in candidate target cells

// Used to support sorting points around the perimeter of the target candidate cell.
// bool PerimeterSort(const vtkPointInfo& p0, const vtkPointInfo& p1)
// {
//   return ( p0.T < p1.T );
// }

// Convenience typedefs
using vtkOutTrisArray = std::vector<vtkIdType>;

// Information gathered for target candidate cells that require
// triangulation. (Some target cells, initially identified through a
// bounding box overlap operation, may not require triangulation - hence
// candiate info is not gathered for them.) This information is used to
// triangulate a target candidate cell.
struct vtkCandidateInfo
{
  vtkPointList PerimeterPoints; // All intersection points on the perimeter
  vtkPointList InteriorPoints;  // All points projected into the interior
  vtkEdgeList ConstraintEdges;  // Edge fragments uses to control the triangulation
  vtkOutTrisArray OutTris;      // The output from the triangulation process.
};
using vtkCandidateArray = std::vector<vtkCandidateInfo*>;

// Convenience method returns target candidate cell information. Will instantiate
// a vtkCandidateInformation class as necessary.
vtkCandidateInfo* GetCandidateInfo(vtkCandidateArray* candidateArray, vtkIdType cellId)
{
  // Ensure a valid request is being made
  if (cellId < 0 || cellId >= static_cast<vtkIdType>(candidateArray->size()))
  {
    return nullptr;
  }

  // Honor the request
  vtkCandidateInfo*& cInfo = (*candidateArray)[cellId];
  if (cInfo == nullptr)
  {
    cInfo = new vtkCandidateInfo; // create new information if necessary
  }
  return cInfo;
}

// Project imprint points onto the target and gather information about the
// projection.
template <typename DataT>
struct ProjPoints
{
  vtkPolyData* Target;
  vtkPoints* TargetPts;
  vtkCellArray* TargetCells;
  vtkIdType NumTargetPts;
  vtkStaticCellLocator* CellLocator;
  DataT* ImprintPts;
  vtkPointArray* ImprintArray;
  double Tol;
  double Tol2;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  ProjPoints(vtkPolyData* target, vtkStaticCellLocator* cellLoc, DataT* impPts,
    vtkPointArray* pArray, double tol)
    : Target(target)
    , CellLocator(cellLoc)
    , ImprintPts(impPts)
    , ImprintArray(pArray)
    , Tol(tol)
  {
    this->TargetPts = target->GetPoints();
    this->TargetCells = target->GetPolys();
    this->NumTargetPts = target->GetNumberOfPoints();
    this->Tol2 = tol * tol;
  }

  void Initialize()
  {
    this->Cell.Local().TakeReference(vtkGenericCell::New());
    this->CellIterator.Local().TakeReference(this->TargetCells->NewIterator());
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    vtkPolyData* candidateOutput = this->Target;
    vtkPoints* targetPts = this->TargetPts;
    vtkGenericCell* cell = this->Cell.Local();
    vtkCellArrayIterator* targetIter = this->CellIterator.Local();
    const auto imprintPts = vtk::DataArrayTupleRange<3>(this->ImprintPts);
    vtkStaticCellLocator* cellLoc = this->CellLocator;
    vtkIdType cellId;
    int subId, inside;
    double x[3], dist2, closest[3], tol = this->Tol;
    vtkIdType npts;
    const vtkIdType* pts;
    vtkNew<vtkIdList> edgeNeis;

    for (; ptId < endPtId; ptId++)
    {
      vtkPointInfo& pt = (*this->ImprintArray)[ptId];

      auto xTuple = imprintPts[ptId];
      x[0] = xTuple[0];
      x[1] = xTuple[1];
      x[2] = xTuple[2];

      // See if the point projects onto the target
      if (!cellLoc->FindClosestPointWithinRadius(
            x, tol, closest, cell, cellId, subId, dist2, inside))
      {
        pt.Classification = PointClassification::Outside;
      }
      else // The point projects onto the target. See if it hits a vertex or edge.
      {
        // At a minimum it's an interior point
        pt.Classification = PointClassification::Interior;
        pt.Cells[0] = cellId;
        pt.X[0] = closest[0];
        pt.X[1] = closest[1];
        pt.X[2] = closest[2];

        // Let's see if it's on a cell vertex or on a cell edge
        targetIter->GetCellAtId(cellId, npts, pts);
        double p0[3], p1[3], t, d2;

        // Check the vertices first
        vtkIdType i;
        for (i = 0; i < npts; ++i)
        {
          targetPts->GetPoint(pts[i], p0);
          if (vtkMath::Distance2BetweenPoints(p0, pt.X) < this->Tol2)
          {
            pt.Classification = PointClassification::OnVertex;
            pt.PtId = pts[i]; // Need to specify this here
            break;
          }
        }
        // If a vertex is found it always take precedence. Check the edges
        // only if necessary.
        if (i >= npts)
        {
          // Okay need to check the edges
          for (i = 0; i < npts; ++i)
          {
            vtkIdType v0 = pts[i];
            vtkIdType v1 = pts[(i + 1) % npts];
            targetPts->GetPoint(v0, p0);
            targetPts->GetPoint(v1, p1);
            d2 = vtkLine::DistanceToLine(pt.X, p0, p1, t, closest);
            if (d2 <= this->Tol2)
            {
              candidateOutput->GetCellEdgeNeighbors(cellId, v0, v1, edgeNeis);
              if (edgeNeis->GetNumberOfIds() > 0)
                pt.Cells[1] = edgeNeis->GetId(0);
              pt.Classification = PointClassification::OnEdge;
              pt.CellEdge.Define(v0, v1);
              break;
            }
          } // if on edge
        }   // if need to check edges
      }     // Imprint point successfully projected onto target
    }       // For all imprint points
  }

  void Reduce() {}
};

// Glue between dispatch and point processing algorithm
struct ProjPointsWorker
{
  template <typename DataT>
  void operator()(DataT* impPts, vtkPolyData* candidateOutput, vtkStaticCellLocator* cellLoc,
    vtkPointArray* pArray, double tol)
  {
    vtkIdType numPts = impPts->GetNumberOfTuples();

    ProjPoints<DataT> pp(candidateOutput, cellLoc, impPts, pArray, tol);

    vtkSMPTools::For(0, numPts, pp); // currently a non-thread-safe operation
    // pp.Initialize();
    // pp(0,numPts);
  }
};

// Once point projection is completed, generate additional points and
// insert them into the points array and the candidate cells structure.
// Also assign a point id to the projected points.
struct ProduceProjectedPoints
{
  vtkPoints* OutPts;
  vtkPointArray* PointArray;
  vtkPolyData* CandidateOutput;
  vtkCandidateArray* CandidateArray;
  vtkIdType CurrentPtId;

  ProduceProjectedPoints(vtkPoints* outPts, vtkPointArray* pArray, vtkPolyData* candidateOutput,
    vtkCandidateArray* candidateArray)
    : OutPts(outPts)
    , PointArray(pArray)
    , CandidateOutput(candidateOutput)
    , CandidateArray(candidateArray)
  {
  }

  void operator()()
  {
    vtkPoints* outPts = this->OutPts;
    vtkPointArray* pArray = this->PointArray;
    vtkCandidateArray* candidateArray = this->CandidateArray;

    vtkIdType numTargetPts = outPts->GetNumberOfPoints();
    vtkIdType numImprintPts = static_cast<vtkIdType>(pArray->size());

    // Note that the projected points are converted to global id space.
    // That is, points are added at the end of the original target
    // points (all of them, not just the candidates).
    this->CurrentPtId = numTargetPts; // will be incremented

    // Traverse all these projected points, updating information in the candidate cells
    for (auto ptId = 0; ptId < numImprintPts; ++ptId)
    {
      vtkPointInfo& pt = (*pArray)[ptId];
      if (pt.Classification <= PointClassification::Outside)
      {
        continue; // Skip this point
      }
      else if (pt.Classification == PointClassification::OnVertex)
      {
        // The point is already created with a given PtId.
        // Cell vertices will be treated later during triangulation since
        // the cell vertices are added to the triangulation process.
        continue;
      }
      else // requires creation of a new output point
      {
        vtkCandidateInfo* cInfo = GetCandidateInfo(candidateArray, pt.Cells[0]);

        // Create a new output point from a successfully projected point
        outPts->InsertPoint(this->CurrentPtId, pt.X);
        pt.PtId = this->CurrentPtId++;

        // Associate this point with the proper cell(s). At this point in the
        // logic, the points are either in the interior, or on a cell edge.
        if (pt.Classification == PointClassification::Interior)
        {
          // An interior point is associated with just a single cell
          cInfo->InteriorPoints.emplace_back(ptId + numTargetPts);
        }
        else // pt.Classification == PointClassification::OnEdge
        {
          // The point has to be associated with all candidate cells
          // which use this edge.
          cInfo->PerimeterPoints.emplace_back(ptId + numTargetPts);
          vtkCandidateInfo* cInfoE = GetCandidateInfo(candidateArray, pt.Cells[1]);
          if (cInfoE != nullptr)
            cInfoE->PerimeterPoints.emplace_back(ptId + numTargetPts);
        } // on edge
      }   // requires some processing
    }     // for all imprint points
  }       // operator()
};        // ProduceProjectedPoints

// Intersect the imprint edges with the target candidate cells to produce
// intersection points. Use a locator to identify potential
// target cells to intersect.
struct ProduceIntersectionPoints
{
  vtkPoints* OutPts;
  vtkPolyData* Imprint;
  vtkCellArray* ImprintCells;
  vtkPointArray* PointArray;
  vtkPolyData* CandidateOutput;
  vtkCellArray* TargetCells;
  vtkStaticCellLocator* Locator;
  vtkCandidateArray* CandidateArray;
  vtkIdType TargetOffset;
  double Tol;
  double Tol2;
  // The newly generated intersection points, on a per-thread basis,
  // are accumulated in the NewPoints thread local member.
  vtkSMPThreadLocal<vtkPointArray> NewPoints;

  ProduceIntersectionPoints(vtkPoints* outPts, vtkPolyData* imprint, vtkPointArray* pArray,
    vtkPolyData* candidateOutput, vtkStaticCellLocator* loc, vtkCandidateArray* candidateArray,
    vtkIdType offset, double tol)
    : OutPts(outPts)
    , Imprint(imprint)
    , PointArray(pArray)
    , CandidateOutput(candidateOutput)
    , Locator(loc)
    , CandidateArray(candidateArray)
    , TargetOffset(offset)
    , Tol(tol)
  {
    this->ImprintCells = this->Imprint->GetPolys();
    this->TargetCells = this->CandidateOutput->GetPolys();
    this->Tol2 = this->Tol * this->Tol;
  }

  // Get information about an imprint point.
  vtkPointInfo* GetPointInfo(vtkIdType ptId) { return &((*this->PointArray)[ptId]); }

  // Control which candidate target edges should be intersected.
  bool ShouldImprintEdgeBeProcessed(vtkPointInfo* pStart, vtkPointInfo* pEnd)
  {
    // Simple case: two interior points
    if (pStart->Classification == PointClassification::Interior &&
      pEnd->Classification == PointClassification::Interior)
    {
      if (pStart->Cells[0] == pEnd->Cells[0]) // in the same target cell
        return false;
      else
        return true;
    }

    // Maybe there is an edge intersection point involved.
    else
    {
      if (pStart->Cells[1] >= 0) // not -1 (undefined)
      {
        if ((pStart->Cells[1] == pEnd->Cells[0]) ||
          (pEnd->Cells[1] >= 0 && pStart->Cells[1] == pEnd->Cells[1]))
        {
          return false;
        }
        else
        {
          return true;
        }
      }
      else if (pEnd->Cells[1] >= 0) // not -1 (undefined)
      {
        if ((pEnd->Cells[1] == pStart->Cells[0]) ||
          (pStart->Cells[1] >= 0 && pEnd->Cells[1] == pStart->Cells[1]))
        {
          return false;
        }
        else
        {
          return true;
        }
      }

      return true;
    }
  } // ShouldImprintEdgeBeProcessed()

  // Intersect the imprint edge defined by (x0,x1) with the
  // target edge (v0,v1). If there is an intersection, add the
  // intersection point to the list of intersections.
  void IntersectEdges(double x0[3], double x1[3], vtkIdType v0, vtkIdType v1, vtkIdList* neighbors)
  {
    vtkPolyData* target = this->CandidateOutput;
    double y0[3], y1[3], u, v;
    target->GetPoint(v0, y0);
    target->GetPoint(v1, y1);

    if (vtkLine::Intersection(x0, x1, y0, y1, u, v, this->Tol) != vtkLine::Intersect)
    {
      return;
    }

    // Okay we may need to add an intersection point. Check to see whether the point is
    // within tolerance of the target end points. If so, we discard it.
    double xU[3], xV[3];
    for (auto i = 0; i < 3; ++i)
    {
      xU[i] = x0[i] + u * (x1[i] - x0[i]);
      xV[i] = y0[i] + v * (y1[i] - y0[i]);
    }

    if (vtkMath::Distance2BetweenPoints(xU, xV) > this->Tol2 ||
      vtkMath::Distance2BetweenPoints(xV, y0) <= this->Tol2 ||
      vtkMath::Distance2BetweenPoints(xV, y1) <= this->Tol2)
    {
      return;
    }

    // We can add a new point to the output of this thread. Later, a new
    // VTK point id will be set, and the new points composited together
    // during the Reduce() process.
    vtkPointArray& newPts = this->NewPoints.Local();
    vtkIdType cells[2];
    target->GetCellEdgeNeighbors(-1, v0, v1, neighbors);
    cells[0] = (neighbors->GetNumberOfIds() < 1 ? -1 : neighbors->GetId(0));
    cells[1] = (neighbors->GetNumberOfIds() < 2 ? -1 : neighbors->GetId(1));
    newPts.emplace_back(vtkPointInfo(PointClassification::OnEdge, -1, cells, v0, v1, xV, v));

  } // IntersectEdges()

  // The following methods to support SMPTools integration.
  void Initialize() {}

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkPolyData* imprint = this->Imprint;
    vtkStaticCellLocator* loc = this->Locator;
    vtkIdType iNPts;
    const vtkIdType* iPts;

    // Some scratch objects to support computation
    vtkNew<vtkIdList> cells;
    vtkNew<vtkIdList> edgeNeighbors;

    // Support threaded random access across the imprint and target
    vtkSmartPointer<vtkCellArrayIterator> imprintIter =
      vtk::TakeSmartPointer(this->ImprintCells->NewIterator());
    vtkSmartPointer<vtkCellArrayIterator> targetIter =
      vtk::TakeSmartPointer(this->TargetCells->NewIterator());

    // Keep track of imprint edges to process
    TargetEdgeList tEdges;
    TargetEdgeLocatorType tLoc;

    // Loop over all imprint cells in this batch of cells and intersect the
    // appropriate cell's edges with the candidate target cells. To avoid
    // processing the same edge twice, only process an imprint's cell edge
    // if the edge's cell neighbor id is larger then then the current
    // cellId; or the imprint edge is a boundary edge.
    for (; cellId < endCellId; ++cellId)
    {
      imprintIter->GetCellAtId(cellId, iNPts, iPts);
      for (auto i = 0; i < iNPts; ++i)
      {
        vtkIdType viStart = iPts[i];
        vtkIdType viEnd = iPts[(i + 1) % iNPts];
        vtkPointInfo* pStart = this->GetPointInfo(viStart);
        vtkPointInfo* pEnd = this->GetPointInfo(viEnd);

        // There are some simple cases that can avoid line-line intersection,
        // or where processing of the imprint edges is not needed. For example,
        // if the end points of the imprint edge lie on the same target cell,
        // then the imprint edge is within the (convex) target cell.
        if (!this->ShouldImprintEdgeBeProcessed(pStart, pEnd))
        {
          continue;
        }

        // See whether to process the current imprint edge. If so, gather
        // the target edges to intersect.
        vtkIdType tNPts;
        const vtkIdType* tPts;
        double xStart[3], xEnd[3];
        imprint->GetPoint(viStart, xStart);
        imprint->GetPoint(viEnd, xEnd);

        imprint->GetCellEdgeNeighbors(cellId, viStart, viEnd, edgeNeighbors);
        if (edgeNeighbors->GetNumberOfIds() < 1 || edgeNeighbors->GetId(0) > cellId)
        {
          // Clear data for the current imprint edge.
          tEdges.clear();

          // Identify the target candidate cells and consequently edges which may
          // intersect the current imprint edge.
          loc->FindCellsAlongLine(xStart, xEnd, this->Tol, cells);

          vtkIdType numCells = cells->GetNumberOfIds();
          for (auto j = 0; j < numCells; ++j)
          {
            targetIter->GetCellAtId(cells->GetId(j), tNPts, tPts);
            for (auto k = 0; k < tNPts; ++k)
            { // process each edge of this target cell
              vtkIdType vtStart = tPts[k];
              vtkIdType vtEnd = tPts[(k + 1) % tNPts];
              tEdges.emplace_back(TargetEdgeType(vtStart, vtEnd, {}));
            }
          } // for all target cell candidates

          // Sort the target edges (group them together). Then process each
          // target edge only once.
          vtkIdType numTargetEdges;
          const vtkIdType* tOffsets =
            tLoc.MergeEdges(static_cast<vtkIdType>(tEdges.size()), tEdges.data(), numTargetEdges);

          // Finally intersect the current imprint edge with the candidate target edges.
          // This has the side affect of adding new intersection points to the list
          // of intersection points.
          const TargetEdgeType* tEdge;
          for (auto teNum = 0; teNum < numTargetEdges; ++teNum)
          {
            tEdge = tEdges.data() + tOffsets[teNum];
            this->IntersectEdges(xStart, xEnd, tEdge->V0, tEdge->V1, edgeNeighbors);
          }
        } // if should process this edge
      }   // for each imprint cell edge
    }     // for all cells
  }       // operator()

  // Composite and number the generated points, add the points to the
  // candidate target cells for later triangulation.
  void Reduce()
  {
    vtkIdType newPtId;
    vtkPointArray* pArray = this->PointArray;
    vtkPoints* outPts = this->OutPts;

    // For each thread, copy thread's points into global arrays, and assign
    // a point id.
    vtkCandidateArray* candidateArray = this->CandidateArray;
    vtkCandidateInfo* cInfo;
    auto npEnd = this->NewPoints.end();
    for (auto npItr = this->NewPoints.begin(); npItr != npEnd; ++npItr)
    {
      vtkPointArray& newPts = *npItr;
      for (auto pIter = newPts.begin(); pIter != newPts.end(); ++pIter)
      {
        newPtId = outPts->InsertNextPoint(pIter->X);
        pArray->emplace_back(vtkPointInfo(PointClassification::OnEdge, newPtId, pIter->Cells,
          pIter->CellEdge.V0, pIter->CellEdge.V1, pIter->X, pIter->T));
        for (auto i = 0; i < 2; ++i)
        {
          if ((cInfo = GetCandidateInfo(candidateArray, pIter->Cells[i])) != nullptr)
          {
            cInfo->PerimeterPoints.emplace_back((pArray->size() - 1 + this->TargetOffset));
          }
        } // for cells on either side of edge
      }   // for all intersection points
    }     // for all threads
  }       // Reduce()

}; // ProduceIntersectionPoints

// Threaded triangulation of target candidate cells. Only the candidates
// which contain projected points, edge intersection points, or edge
// fragments, are processed. After triangulation, the output is sent to
// the final output.
struct Triangulate
{
  vtkPoints* OutPts;
  vtkPointArray* PointArray;
  vtkPolyData* Candidates;
  vtkCandidateArray* CandidateArray;
  vtkPolyData* Output;
  vtkIdType TargetOffset;

  Triangulate(vtkPoints* outPts, vtkPointArray* pa, vtkPolyData* candidates, vtkCandidateArray* ca,
    vtkPolyData* output, vtkIdType offset)
    : OutPts(outPts)
    , PointArray(pa)
    , Candidates(candidates)
    , CandidateArray(ca)
    , Output(output)
    , TargetOffset(offset)
  {
  }

  // Setup the triangulation pipeline
  void Initialize() {}

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkPoints* outPts = this->OutPts;
    vtkPointArray& points = *this->PointArray;
    vtkIdType npts;
    const vtkIdType* pts;
    vtkIdType targetOffset = this->TargetOffset;

    // These were initially made thread local to improve
    // performance. However, this caused weird VTK pipeline errors. TODO:
    // see if thread local can be made to work.
    vtkNew<vtkPoints> triPts;
    triPts->SetDataTypeToDouble();
    vtkNew<vtkIdTypeArray> ptMap;
    vtkNew<vtkPolyData> polyData;
    vtkNew<vtkDelaunay2D> triangulator;
    polyData->SetPoints(triPts);
    polyData->GetPointData()->AddArray(ptMap);
    triangulator->SetInputData(polyData);
    triangulator->SetOffset(10.0);

    for (; cellId < endCellId; cellId++)
    {
      // Only cells requiring triangulation are processed.
      vtkCandidateInfo*& cInfo = (*this->CandidateArray)[cellId];
      if (cInfo != nullptr)
      {
        this->Candidates->GetCellPoints(cellId, npts, pts);

        triPts->SetNumberOfPoints(npts);
        ptMap->SetNumberOfTuples(npts);

        // Start by adding original cell points
        double x[3];
        for (auto i = 0; i < npts; ++i)
        {
          outPts->GetPoint(pts[i], x);
          triPts->SetPoint(i, x);
          ptMap->SetTypedComponent(i, 0, pts[i]);
        }

        // Next, any perimeter points
        vtkIdType pId;
        vtkPointInfo* pInfo;
        vtkIdType numPerimeterPts = static_cast<vtkIdType>(cInfo->PerimeterPoints.size());
        for (auto i = 0; i < numPerimeterPts; ++i)
        {
          pId = cInfo->PerimeterPoints[i] - targetOffset;
          pInfo = &points[pId];
          triPts->InsertNextPoint(pInfo->X);
          ptMap->InsertNextValue(pInfo->PtId);
        }

        // Now, interior points
        vtkIdType numInteriorPts = static_cast<vtkIdType>(cInfo->InteriorPoints.size());
        for (auto i = 0; i < numInteriorPts; ++i)
        {
          pId = cInfo->InteriorPoints[i] - targetOffset;
          ;
          pInfo = &points[pId];
          pId = triPts->InsertNextPoint(pInfo->X);
          ptMap->InsertNextValue(pInfo->PtId);
        }

        // TODO: Constraint edges (if any)

        // Triangulate
        polyData->Modified();
        triangulator->Update();

        // Copy output of the triangulation filter into local candidate info.
        // Make sure to use the point map to obtain the correct point ids.
        vtkCellArray* ca = triangulator->GetOutput()->GetPolys();
        cInfo->OutTris.resize(0);
        for (ca->InitTraversal(); ca->GetNextCell(npts, pts);)
        {
          vtkIdType v0 = ptMap->GetTypedComponent(pts[0], 0);
          vtkIdType v1 = ptMap->GetTypedComponent(pts[1], 0);
          vtkIdType v2 = ptMap->GetTypedComponent(pts[2], 0);
          cInfo->OutTris.emplace_back(v0);
          cInfo->OutTris.emplace_back(v1);
          cInfo->OutTris.emplace_back(v2);
        }
      }

    } // for all candidate cells
  }   // operator()

  // Insert the triangulation into the filter's output.
  // For now this is a serial operation.
  void Reduce()
  {
    vtkIdType numCandidates = static_cast<vtkIdType>(this->CandidateArray->size());
    for (auto cellId = 0; cellId < numCandidates; cellId++)
    {
      vtkCandidateInfo*& cInfo = (*this->CandidateArray)[cellId];
      // Cells not requiring triangulation are simply sent to
      // the output.
      if (cInfo == nullptr)
      {
        vtkIdType npts;
        const vtkIdType* pts;
        int cellType = this->Candidates->GetCellType(cellId);
        this->Candidates->GetCellPoints(cellId, npts, pts);
        this->Output->InsertNextCell(cellType, npts, pts);
      }

      // Otherwise, the results of the triangulation are sent to
      // the output.
      else
      {
        vtkIdType numTris = static_cast<vtkIdType>(cInfo->OutTris.size()) / 3;
        vtkIdType tri[3];
        for (auto i = 0; i < numTris; ++i)
        {
          tri[0] = cInfo->OutTris[3 * i];
          tri[1] = cInfo->OutTris[3 * i + 1];
          tri[2] = cInfo->OutTris[3 * i + 2];
          this->Output->InsertNextCell(VTK_TRIANGLE, 3, tri);
        }
      }

    } // for all candidate target cells
  }   // Reduce

}; // Triangulate

} // anonymous

//------------------------------------------------------------------------------
int vtkImprintFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* targetInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* imprintInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* target = vtkPolyData::SafeDownCast(targetInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* imprint = vtkPolyData::SafeDownCast(imprintInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Initialize and check data
  vtkDebugMacro(<< "Imprinting...");

  vtkIdType numTargetPts = target->GetNumberOfPoints();
  vtkIdType numTargetCells = target->GetPolys()->GetNumberOfCells();
  if (numTargetPts < 1 || numTargetCells < 1)
  {
    vtkErrorMacro("Target is empty");
    return 1;
  }
  vtkPoints* targetPts = target->GetPoints();

  if (!imprint)
  {
    vtkErrorMacro("Imprint is empty");
    return 1;
  }
  vtkIdType numImprintPts = imprint->GetNumberOfPoints();
  vtkIdType numImprintCells = imprint->GetNumberOfCells();
  if (numImprintPts < 1 || numImprintCells < 1)
  {
    vtkErrorMacro("Please define a non-empty imprint");
    return 1;
  }
  vtkPoints* imprintPts = imprint->GetPoints();

  // Begin by segrating out the target cells that may be imprinted (the
  // target "candidate" cells), from those that won't be (the "kept" cells). Also
  // copy the target points.  This creates two outputs: 1) the actual filter
  // output - initially it contains the input target points and the kept
  // cells; and 2) the candidate cells that are operated on. Eventually, the
  // candidate cells (and their triangulation) and any newly generated points
  // are appended to the output #1. This separation of the candidates from
  // the target is performed to improve performance since only a subset needs
  // to be worked on.
  vtkNew<vtkPoints> outPts;
  outPts->DeepCopy(targetPts);
  output->SetPoints(outPts);
  vtkNew<vtkPolyData> candidateOutput;
  candidateOutput->SetPoints(outPts);

  // Now separate out the kept cells from the candidate cells. We are throwing
  // out any non-polygon cells. Kept cells are determined using bounding box
  // overlap tests.
  output->AllocateEstimate(numTargetCells, 3);
  candidateOutput->AllocateEstimate(numImprintCells, 3);
  vtkBoundingBox targetBounds;
  vtkBoundingBox imprintBounds;
  double imprintBds[6], targetCellBounds[6];
  imprint->GetCellsBounds(imprintBds);
  imprintBounds.SetBounds(imprintBds);
  imprintBounds.Inflate(this->Tolerance);
  vtkIdType npts;
  const vtkIdType* pts;
  for (auto i = 0; i < numTargetCells; ++i)
  {
    int cellType = target->GetCellType(i);
    if (cellType == VTK_TRIANGLE || cellType == VTK_QUAD || cellType == VTK_POLYGON)
    {
      target->GetCellBounds(i, targetCellBounds);
      targetBounds.SetBounds(targetCellBounds);
      target->GetCellPoints(i, npts, pts);
      if (!targetBounds.Intersects(imprintBounds))
      {
        // This cell is kept
        output->InsertNextCell(cellType, npts, pts);
      }
      else
      {
        // Otherwise this gets shunted to the candidate output for further
        // processing.
        candidateOutput->InsertNextCell(cellType, npts, pts);
      }
    }
  }

  // The output type may just be the target candidate cells.
  if (this->OutputType == TARGET_CELLS)
  {
    output->ShallowCopy(candidateOutput);
    return 1;
  }

  // Topological links are required for neighborhood information. This
  // information is needed for both the imprint and target candidate cells.
  candidateOutput->BuildLinks();
  imprint->BuildLinks();

  // Make sure candiate cells are available to imprint. Build a locator to
  // project imprint points onto the candidate target cells, where the target
  // is now a subset of the original input target.
  vtkIdType numCandidateCells = candidateOutput->GetNumberOfCells();
  if (numCandidateCells < 1)
  {
    vtkWarningMacro("Imprint not in contact with target");
    output->ShallowCopy(target);
    return 1;
  }
  this->CellLocator->SetDataSet(candidateOutput);
  this->CellLocator->BuildLocator();

  // Create an initial array of pointers to information structures, in which
  // each struct contains information about the points and edge fragments
  // within each target candidate cell.
  vtkCandidateArray candidateArray(numCandidateCells, nullptr);

  // Now project all imprint points onto the candidate target. This points
  // array will grow over time as edge intersection points are computed.
  vtkPointArray pArray(numImprintPts);
  using ProjPointsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  ProjPointsWorker ppWorker;
  if (!ProjPointsDispatch::Execute(imprintPts->GetData(), ppWorker, candidateOutput,
        this->CellLocator, &pArray, this->Tolerance))
  {
    ppWorker(imprintPts->GetData(), candidateOutput, this->CellLocator, &pArray, this->Tolerance);
  }

  // With the points projected, add points classified as not-outside to the
  // candidate target cells for later triangulation.
  ProduceProjectedPoints ppp(outPts, &pArray, candidateOutput, &candidateArray);
  ppp();

  // Now produce edge intersection points and optionally constraint
  // edges. This an intersection of the imprint edges against the target
  // edges.
  ProduceIntersectionPoints pip(outPts, imprint, &pArray, candidateOutput, this->CellLocator,
    &candidateArray, numTargetPts, this->Tolerance);
  vtkSMPTools::For(0, numImprintCells, pip);

  // Generate triangulated target candidate cells. The points and/or
  // constraint edges are associated with the candidate cells via
  // the candidate array.
  if (this->OutputType == IMPRINTED_CELLS)
  {
    // This eliminates the target cells that were passed through previously.
    output->ShallowCopy(candidateOutput);
  }
  Triangulate tri(outPts, &pArray, candidateOutput, &candidateArray, output, numTargetPts);
  vtkSMPTools::For(0, numCandidateCells, tri);

  return 1;
}

//------------------------------------------------------------------------------
int vtkImprintFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* targetInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* imprintInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (imprintInfo)
  {
    imprintInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    imprintInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    imprintInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  }
  targetInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  targetInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  targetInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  targetInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//------------------------------------------------------------------------------
void vtkImprintFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Output Type: " << this->OutputType << "\n";
}
