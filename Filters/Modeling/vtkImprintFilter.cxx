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
#include "vtkCellData.h"
#include "vtkCharArray.h"
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

  this->DebugOutputType = NO_DEBUG_OUTPUT;
  this->DebugCellId = (-1);

  this->SetNumberOfInputPorts(2);

  this->SetNumberOfOutputPorts(2);
  vtkNew<vtkPolyData> output2;
  this->GetExecutive()->SetOutputData(1, output2);
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
// Target points also affect the imprinted area, those that are inside
// the imprint region are "mixed into" the imprinted area.
// This enums assigns a classification to the points.
enum PointClassification
{
  TargetOutside = -3, // Target pt is outside the imprint region
  TargetInside = -2,  // Target pt is inside the imprint region
  Unknown = -1,       // initial, unknown classification
  Outside = 0,        // imprint points with classification <=0 are not inserted
  Interior = 1,       // imprint pt is interior to target cell
  OnVertex = 2,       // imprint pt is on a vertex of a target cell
  OnEdge = 3,         // imprint pt is on an edge of a target cell
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
  double T;            // Perimeter coordinate (if applicable, eg. point on edge)
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

// Convenience typedefs
using vtkOutTrisArray = std::vector<vtkIdType>;
using vtkOutTrisClass = std::vector<char>;

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
  vtkOutTrisClass OutTrisClass; // The classification of the output triangles
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

// Separate the kept candidate cells from the input target. This reduces
// the overall work.
struct BoundsCull
{
  vtkPolyData* Target;                  // input vtkPolyData to algorithm
  vtkPolyData* Imprint;                 // imprinting vtkPolyData
  int OutputType;                       // control what is output
  double Tol;                           // tolerance
  vtkPolyData* CandidateOutput;         // kept cells
  vtkPolyData* Output;                  // initially, cells not processed by imprint
  vtkStaticCellLocator* ImprintLocator; // accelerate operation on imprint

  // Internal methods for computing
  vtkBoundingBox ImprintBounds;
  std::vector<char> CellMarks;

  BoundsCull(vtkPolyData* target, vtkPolyData* imprint, vtkStaticCellLocator* impLoc,
    int outputType, double tol, vtkPolyData* candidateOutput, vtkPolyData* output)
    : Target(target)
    , Imprint(imprint)
    , OutputType(outputType)
    , Tol(tol)
    , CandidateOutput(candidateOutput)
    , Output(output)
    , ImprintLocator(impLoc)
  {
    vtkIdType numCells = target->GetNumberOfCells();
    this->CellMarks.resize(numCells);
    this->Target->BuildCells(); // to avoid thead collision in GetCellType()
    double imprintBds[6];
    this->Imprint->GetBounds(imprintBds);
    this->ImprintBounds.SetBounds(imprintBds);
    this->ImprintBounds.Inflate(this->Tol);
  }

  // Needed for Reduce() to run
  void Initialize() {}

  // Mark the cells
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkBoundingBox targetBounds;
    double targetCellBounds[6];
    vtkNew<vtkIdList> cells;
    vtkPolyData* target = this->Target;
    vtkStaticCellLocator* impLocator = this->ImprintLocator;

    // Loop over target cells and mark those that should be kept.
    for (; cellId < endCellId; ++cellId)
    {
      int cellType = target->GetCellType(cellId);
      if (cellType == VTK_TRIANGLE || cellType == VTK_QUAD || cellType == VTK_POLYGON)
      {
        target->GetCellBounds(cellId, targetCellBounds);
        targetBounds.SetBounds(targetCellBounds);
        targetBounds.GetBounds(targetCellBounds);
        if (this->ImprintBounds.Intersects(targetBounds))
        {
          impLocator->FindCellsWithinBounds(targetCellBounds, cells);
          // Negative mark means it's not kept but may be part of the output
          this->CellMarks[cellId] = (cells->GetNumberOfIds() > 0 ? cellType : -cellType);
        }
        else
        {
          this->CellMarks[cellId] = -cellType;
        }
      }
    }
  } // operator()

  // Produce final output
  void Reduce()
  {
    // Loop over all cell marks, and output the target candidate cells
    // accordingly. Note that if just the target candidate cells are desired,
    // the other cells are not output.
    vtkIdType npts;
    const vtkIdType* pts;
    vtkPolyData* target = this->Target;
    vtkPolyData* candidateOutput = this->CandidateOutput;
    vtkPolyData* output = this->Output;
    int outputType = this->OutputType;
    vtkIdType cellId;
    auto iter = this->CellMarks.begin();

    for (cellId = 0; iter != this->CellMarks.end(); ++iter, ++cellId)
    {
      target->GetCellPoints(cellId, npts, pts);
      if (*iter > 0) // inserting target cells
      {
        candidateOutput->InsertNextCell(*iter, npts, pts);
      }
      else if (outputType != vtkImprintFilter::TARGET_CELLS)
      {
        output->InsertNextCell(-(*iter), npts, pts);
      }
    }
  } // Reduce()

}; // BoundsCull

struct TargetPointClassifier
{
  vtkPolyData* Candidates;
  vtkPoints* CandidatePoints;
  vtkCellArray* CandidateCells;
  vtkStaticCellLocator* ImprintLocator;
  double Tol;
  std::vector<char> PtClassification;
  // Scratch object for classifying points in parallel
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  TargetPointClassifier(vtkPolyData* target, vtkStaticCellLocator* cellLoc, double tol)
    : Candidates(target)
    , ImprintLocator(cellLoc)
    , Tol(tol)
  {
    this->CandidatePoints = target->GetPoints();
    this->CandidateCells = target->GetPolys();
    vtkIdType numPts = target->GetNumberOfPoints();
    this->PtClassification.insert(
      this->PtClassification.begin(), numPts, PointClassification::Unknown);
  }

  // Set the classification of a point. It retains the most specialized
  // classification value.
  void SetClassification(vtkIdType ptId, char c)
  {
    char initialClass = this->PtClassification[ptId];
    if (initialClass == PointClassification::Unknown)
      this->PtClassification[ptId] = c;
  }

  // Get the classification of a point.
  char GetClassification(vtkIdType ptId) { return this->PtClassification[ptId]; }

  // Classify remaining unclassified candidate target points
  // using geometric operations.
  void GeometricClassify()
  {
    vtkIdType numCells = this->CandidateCells->GetNumberOfCells();
    vtkSMPTools::For(0, numCells, *this);
  }

  void Initialize()
  {
    this->Cell.Local().TakeReference(vtkGenericCell::New());
    this->CellIterator.Local().TakeReference(this->CandidateCells->NewIterator());
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkGenericCell* cell = this->Cell.Local();
    vtkCellArrayIterator* targetIter = this->CellIterator.Local();
    vtkIdType npts, cId;
    const vtkIdType* pts;
    double x[3], closest[3], dist2;
    int subId, inside;

    // Loop over cells, and just evaluate points if necessary.
    for (; cellId < endCellId; cellId++)
    {
      targetIter->GetCellAtId(cellId, npts, pts);
      for (auto i = 0; i < npts; ++i)
      {
        vtkIdType pId = pts[i];
        if (this->PtClassification[pId] == PointClassification::Unknown)
        {
          this->CandidatePoints->GetPoint(pId, x);
          int inout = this->ImprintLocator->FindClosestPointWithinRadius(
            x, this->Tol, closest, cell, cId, subId, dist2, inside);
          this->PtClassification[pId] =
            (inout ? PointClassification::TargetInside : PointClassification::TargetOutside);

        } // if point not previously classified
      }   // for cell points
    }     // for all cells in this batch
  }

  void Reduce() {}

}; // TargetPointClassifier

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
  TargetPointClassifier* PtClassifier;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  ProjPoints(vtkPolyData* target, vtkStaticCellLocator* targetLoc, DataT* impPts,
    vtkPointArray* pArray, double tol, TargetPointClassifier* tpc)
    : Target(target)
    , CellLocator(targetLoc)
    , ImprintPts(impPts)
    , ImprintArray(pArray)
    , Tol(tol)
    , PtClassifier(tpc)
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
    vtkStaticCellLocator* targetLoc = this->CellLocator;
    vtkIdType cellId;
    int subId, inside;
    double x[3], dist2, closest[3], tol = this->Tol;
    vtkIdType npts;
    const vtkIdType* pts;
    vtkNew<vtkIdList> edgeNeis;
    TargetPointClassifier* tpc = this->PtClassifier;

    for (; ptId < endPtId; ptId++)
    {
      vtkPointInfo& pt = (*this->ImprintArray)[ptId];

      auto xTuple = imprintPts[ptId];
      x[0] = xTuple[0];
      x[1] = xTuple[1];
      x[2] = xTuple[2];

      // See if the point projects onto the target
      if (!targetLoc->FindClosestPointWithinRadius(
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

        // Let's see if the projected point is on a cell vertex or on a cell
        // edge (within tolerance).
        targetIter->GetCellAtId(cellId, npts, pts);
        double p0[3], p1[3], t, d2;

        // Check the target cell vertices first
        vtkIdType i;
        for (i = 0; i < npts; ++i)
        {
          targetPts->GetPoint(pts[i], p0);
          if (vtkMath::Distance2BetweenPoints(p0, pt.X) < this->Tol2)
          {
            pt.Classification = PointClassification::OnVertex;
            pt.PtId = pts[i]; // The target point is on which the point projects
            tpc->SetClassification(pts[i], PointClassification::TargetInside);
            break;
          }
        }
        // If a vertex is found it always take precedence. Check the edges
        // only if a coincident vertex is not found.
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
              pt.T = (v0 < v1 ? t : (1.0 - t)); // t's origin is from smaller pt id
              pt.CellEdge.Define(v0, v1);       // this method implicitly reorders pt ids
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
  void operator()(DataT* impPts, vtkPolyData* candidateOutput, vtkStaticCellLocator* targetLoc,
    vtkPointArray* pArray, double tol, TargetPointClassifier* tpc)
  {
    vtkIdType numPts = impPts->GetNumberOfTuples();
    ProjPoints<DataT> pp(candidateOutput, targetLoc, impPts, pArray, tol, tpc);
    vtkSMPTools::For(0, numPts, pp); // currently a non-thread-safe operation
  }
};

// Once point projection is completed, insert them into the output vtkPoints
// array and the candidate cells triangulation structure.  Also assign a
// global point id to the projected points - hence this method is serial.
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
        continue; // Nothing needs to be inserted into the vtkPoints output
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
        // logic, the points are either in a cell interior, or on a cell edge.
        if (pt.Classification == PointClassification::Interior)
        {
          // An interior point is associated with just a single cell
          cInfo->InteriorPoints.emplace_back(ptId + numTargetPts);
        }
        else // pt.Classification == PointClassification::OnEdge
        {
          // The point has to be associated on the perimeter of all candidate
          // cells which use this edge.
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
// intersection points on the boundary of the target cells. Use a locator to
// identify potential target cells to intersect.
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

  // Indicate which candidate target edges should be intersected. This simply uses
  // topological checks to avoid edge-edge intersections if possible.
  bool ShouldImprintEdgeBeProcessed(vtkPointInfo* pStart, vtkPointInfo* pEnd)
  {
    // Simple case: two interior points
    if (pStart->Classification == PointClassification::Interior &&
      pEnd->Classification == PointClassification::Interior)
    {
      // If an imprint edge is interior to a target cell, because the cell
      // is convex it will produce no intersections
      if (pStart->Cells[0] == pEnd->Cells[0]) // in the same target cell
        return false;
      else
        return true;
    }

    // Maybe there is a point classified as an edge intersection that is involved.
    else
    {
      if (pStart->Cells[1] >= 0) // not -1 (undefined)
      {
        if ((pStart->Cells[1] == pEnd->Cells[0]) ||
          (pEnd->Cells[1] >= 0 && pEnd->Cells[1] == pStart->Cells[1]))
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
          (pStart->Cells[1] >= 0 && pStart->Cells[1] == pEnd->Cells[1]))
        {
          return false;
        }
        else
        {
          return true;
        }
      }

      return true; // By default, process the edge
    }
  } // ShouldImprintEdgeBeProcessed()

  // Intersect the imprint edge defined by (x0,x1) with the
  // target edge (v0,v1). If there is an intersection, add the
  // intersection point to the list of intersections.
  void IntersectEdge(double x0[3], double x1[3], vtkIdType v0, vtkIdType v1, vtkIdList* neighbors)
  {
    vtkPolyData* target = this->CandidateOutput;
    double y0[3], y1[3], u, v;

    // Note we compute parametric coordinates with the point of lowest edge
    // id at the origin. This is important later during triangulation for
    // sorting around the perimeter of the target cell.
    if (v0 > v1)
      std::swap(v0, v1);

    target->GetPoint(v0, y0);
    target->GetPoint(v1, y1);

    // Perform intersection, return if none
    if (vtkLine::Intersection(x0, x1, y0, y1, u, v, this->Tol, vtkLine::Absolute) !=
      vtkLine::Intersect)
    {
      return;
    }

    // Okay we may need to add an intersection point. Check to see whether
    // the point is within tolerance of the target and imprint end points. If
    // so, we discard it (i.e., it is merged with an existing target cell
    // vertex, or projecte imprint vertex).
    double yU[3];
    yU[0] = y0[0] + v * (y1[0] - y0[0]);
    yU[1] = y0[1] + v * (y1[1] - y0[1]);
    yU[2] = y0[2] + v * (y1[2] - y0[2]);

    if (vtkMath::Distance2BetweenPoints(yU, x0) <= this->Tol2 ||
      vtkMath::Distance2BetweenPoints(yU, x1) <= this->Tol2 ||
      vtkMath::Distance2BetweenPoints(yU, y0) <= this->Tol2 ||
      vtkMath::Distance2BetweenPoints(yU, y1) <= this->Tol2)
    {
      return;
    }

    // We can add a new point to the output of this thread. Later, a new
    // VTK point id will be set, and the new points composited together
    // during the Reduce() process.
    vtkPointArray& newPts = this->NewPoints.Local();
    vtkIdType cells[2];
    // The (-1) trick is used to get all cells using this edge.
    target->GetCellEdgeNeighbors(-1, v0, v1, neighbors);
    cells[0] = (neighbors->GetNumberOfIds() < 1 ? -1 : neighbors->GetId(0));
    cells[1] = (neighbors->GetNumberOfIds() < 2 ? -1 : neighbors->GetId(1));
    newPts.emplace_back(vtkPointInfo(PointClassification::OnEdge, -1, cells, v0, v1, yU, v));

  } // IntersectEdge()

  // The following methods support SMPTools integration.
  void Initialize() {}

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkPolyData* imprint = this->Imprint;
    vtkStaticCellLocator* loc = this->Locator;
    vtkIdType iNPts;
    const vtkIdType* iPts;

    // Some scratch objects to support computation (and avoid lots of new/delete)
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
            this->IntersectEdge(xStart, xEnd, tEdge->V0, tEdge->V1, edgeNeighbors);
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
            cInfo->PerimeterPoints.emplace_back(
              static_cast<vtkIdType>(pArray->size() - 1 + this->TargetOffset));
          }
        } // for cells on either side of edge
      }   // for all intersection points
    }     // for all threads
  }       // Reduce()

}; // ProduceIntersectionPoints

// As part of the triangulation, it's necessary to sort points around the
// boundary of each cell. This ultimately produces constraint edges for the
// triangulation process.
struct PerimeterPoint
{
  double T;
  double X[3];
  vtkIdType Id;
  char Classification;

  PerimeterPoint(double t, double x[3], vtkIdType id, char classification)
  {
    this->T = t;
    this->X[0] = x[0];
    this->X[1] = x[1];
    this->X[2] = x[2];
    this->Id = id;
    this->Classification = classification;
  }
};

using PerimeterList = std::vector<PerimeterPoint>;

// Used to support sorting points around the perimeter of the target candidate cell.
bool PerimeterSort(const PerimeterPoint& p0, const PerimeterPoint& p1)
{
  return (p0.T < p1.T);
}

// Support classification / labeling of output triangles. A TargetCell is a cell
// that was initially part of the target and was not imprinted. An ImprintCell is
// cell that is within the imprinted region. A TransitionCell is not within the
// imprinted region, but it is a cell that transitions the target cells to the
// imprinted cells.
enum CellClassification
{
  TargetCell = 0,
  TransitionCell = 1,
  ImprintCell = 2,
};

// Threaded triangulation of target candidate cells. Only the candidate cells
// which contain projected points, edge intersection points, and/or edge
// fragments, are processed. After triangulation, the output is sent to
// the final output (during Reduce()).
struct Triangulate
{
  vtkPoints* OutPts;
  vtkPointArray* PointArray;
  vtkPolyData* Candidates;
  vtkCandidateArray* CandidateArray;
  vtkPolyData* Output;
  vtkIdType TargetOffset;
  vtkIdType DebugOption; // used for debugging
  vtkIdType DebugCellId;
  vtkPolyData* DebugOutput;
  TargetPointClassifier* PtClassifier;
  vtkSmartPointer<vtkCharArray> CellLabels; // for cell labeling

  Triangulate(vtkPoints* outPts, vtkPointArray* pa, vtkPolyData* candidates, vtkCandidateArray* ca,
    vtkPolyData* output, vtkIdType offset, vtkIdType debugOption, vtkIdType debugCellId,
    vtkPolyData* debugOutput, TargetPointClassifier* ptClassifier)
    : OutPts(outPts)
    , PointArray(pa)
    , Candidates(candidates)
    , CandidateArray(ca)
    , Output(output)
    , TargetOffset(offset)
    , DebugOption(debugOption)
    , DebugCellId(debugCellId)
    , DebugOutput(debugOutput)
    , PtClassifier(ptClassifier)
  {
    // On entry into this method, all current cells are marked outside the
    // imprinted region. During triangulation, newly added cells will be
    // classified accordingly.
    vtkIdType numCells = this->Output->GetNumberOfCells();
    this->CellLabels = vtkSmartPointer<vtkCharArray>::New();
    this->CellLabels->SetNumberOfTuples(numCells);
    this->CellLabels->Fill(CellClassification::TargetCell);
    this->CellLabels->SetName("ImprintedCells");
    this->Output->GetCellData()->AddArray(this->CellLabels);

    // The target points have been partially classified based on topological
    // information. Now geometric information is used to fill in any
    // missing point classifications.
    this->PtClassifier->GeometricClassify();
  }

  // For debugging purposes: output the points provided as input to the
  // target cell triangulation process in the second output.
  void ProduceTriangulationInput(vtkPolyData* pd) { this->DebugOutput->DeepCopy(pd); }

  // For debugging purposes. Output the results of the target cell
  // triangulation in the second output.
  void ProduceTriangulationOutput(vtkPolyData* pd) { this->DebugOutput->DeepCopy(pd); }

  // Insert an edge intersection (perimeter) point into the cell's
  // list of perimeter points.
  void InsertPerimeterPoint(
    vtkIdType npts, const vtkIdType* pts, vtkPointInfo* pInfo, PerimeterList& pList)
  {
    // Find on which edge this point is located
    for (vtkIdType eId = 0; eId < npts; ++eId)
    {
      bool swapped = false;
      vtkIdType v0 = pts[eId];
      vtkIdType v1 = pts[(eId + 1) % npts];
      if (v0 > v1)
      {
        std::swap(v0, v1);
        swapped = true;
      }
      if (v0 == pInfo->CellEdge.V0 && v1 == pInfo->CellEdge.V1)
      {
        double t = (swapped ? (1.0 - pInfo->T) : pInfo->T);
        t += static_cast<double>(eId);
        pList.emplace_back(t, pInfo->X, pInfo->PtId, pInfo->Classification);
        return;
      }
    } // for all edges
  }

  // Classify a triangle based on topological and/or geometric queries. Use
  // topological measures first to reduce computation, resort to geometric
  // queries only when necessary.
  int ClassifyTriangle(
    const vtkIdType* ptIds, std::vector<char>& ptClass, vtkPoints* vtkNotUsed(triPts))
  {
    // Topological check. If any vertex of the triangle is connected
    // to the interior of the imprint region, then the triangle is
    // classified accordingly.
    char c[3]; // point classifications
    c[0] = ptClass[ptIds[0]];
    c[1] = ptClass[ptIds[1]];
    c[2] = ptClass[ptIds[2]];

    if (c[0] == PointClassification::TargetOutside || c[1] == PointClassification::TargetOutside ||
      c[2] == PointClassification::TargetOutside)
    {
      return CellClassification::TransitionCell;
    }

    return CellClassification::ImprintCell;
  }

  // SMP interface methods
  void Initialize() {}

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkPoints* outPts = this->OutPts;
    vtkPointArray& points = *this->PointArray;
    vtkIdType npts;
    const vtkIdType* pts;
    vtkIdType targetOffset = this->TargetOffset;
    TargetPointClassifier* tpc = this->PtClassifier;

    // These were initially made thread local to improve
    // performance. However, this caused weird VTK pipeline errors. TODO:
    // see if thread local can be made to work.
    vtkNew<vtkPoints> triPts;
    triPts->SetDataTypeToDouble();
    vtkNew<vtkIdTypeArray> ptMap;
    std::vector<char> ptClass;
    vtkNew<vtkPolyData> polyData;
    vtkNew<vtkDelaunay2D> triangulator;

    polyData->SetPoints(triPts);
    polyData->GetPointData()->AddArray(ptMap);

    vtkNew<vtkPolyData> constraints;
    vtkNew<vtkCellArray> constraintEdges;
    constraints->SetPoints(triPts);
    constraints->SetLines(constraintEdges);

    triangulator->SetInputData(polyData);
    triangulator->SetSourceData(constraints);
    triangulator->SetOffset(5.0);
    triangulator->SetTolerance(0.001);
    triangulator->SetProjectionPlaneMode(VTK_BEST_FITTING_PLANE);
    PerimeterList pList;

    for (; cellId < endCellId; cellId++)
    {
      // Only cells requiring triangulation are processed.
      vtkCandidateInfo*& cInfo = (*this->CandidateArray)[cellId];
      if (cInfo != nullptr)
      {
        // Insert all of the points on the perimeter of the cell, including
        // the cell vertices. These will be sorted to create constraint edges.
        this->Candidates->GetCellPoints(cellId, npts, pts);
        vtkIdType numPerimeterPts = static_cast<vtkIdType>(cInfo->PerimeterPoints.size());
        vtkIdType totalPerimeterPts = npts + numPerimeterPts;
        pList.clear();
        triPts->SetNumberOfPoints(totalPerimeterPts);
        ptMap->SetNumberOfTuples(totalPerimeterPts);
        constraintEdges->Reset();
        ptClass.clear();

        // Start by adding original cell points to the perimeter list
        double t, x[3];
        for (auto i = 0; i < npts; ++i)
        {
          outPts->GetPoint(pts[i], x);
          t = static_cast<double>(i);
          pList.emplace_back(PerimeterPoint(t, x, pts[i], tpc->GetClassification(pts[i])));
        }

        // Now insert edge points around the perimeter with the appropriate
        // parametric coordinate.
        vtkIdType pId;
        vtkPointInfo* pInfo;
        for (auto i = 0; i < numPerimeterPts; ++i)
        {
          pId = cInfo->PerimeterPoints[i] - targetOffset;
          pInfo = &points[pId];
          this->InsertPerimeterPoint(npts, pts, pInfo, pList);
        }

        // Sort the perimeter points
        std::sort(pList.begin(), pList.end(), PerimeterSort);

        // Add the sorted perimeter points and constraints to the
        // triangulator.
        for (auto i = 0; i < totalPerimeterPts; ++i)
        {
          PerimeterPoint& ppt0 = pList[i];
          triPts->SetPoint(i, ppt0.X);
          ptMap->SetValue(i, ppt0.Id);
          ptClass.emplace_back(ppt0.Classification);
          vtkIdType cEdge[2];
          cEdge[0] = i;
          cEdge[1] = (i + 1) % totalPerimeterPts;
          constraintEdges->InsertNextCell(2, cEdge); // constraint edge on perimeter
        }

        // Now, add additional interior points (if any)
        vtkIdType numInteriorPts = static_cast<vtkIdType>(cInfo->InteriorPoints.size());
        for (auto i = 0; i < numInteriorPts; ++i)
        {
          pId = cInfo->InteriorPoints[i] - targetOffset;
          pInfo = &points[pId];
          pId = triPts->InsertNextPoint(pInfo->X);
          ptMap->InsertNextValue(pInfo->PtId);
          ptClass.emplace_back(pInfo->Classification);
        }

        // TODO: Interior constraint edges (if any). These would come from
        // the imprint cell edges.

        // Perform the constrained triangulation. Make sure the filter
        // reexecutes.
        polyData->Modified();
        constraints->Modified();

        // Triangulate and produce requested debugging output
        if (this->DebugOption == vtkImprintFilter::INPUT_POINTS && this->DebugCellId == cellId &&
          this->DebugOutput != nullptr)
        {
          this->ProduceTriangulationInput(polyData);
        }

        triangulator->Update();

        if (this->DebugOption == vtkImprintFilter::OUTPUT_TRIANGULATION &&
          this->DebugCellId == cellId && this->DebugOutput != nullptr)
        {
          this->ProduceTriangulationOutput(triangulator->GetOutput());
        }

        // Clean up, need to remove cell links etc. in preparation for next
        // cell triangulation. (This is a bug in cell / link building, the
        // call below should not be necessary if mtime time stamps were
        // used.)
        constraints->DeleteCells();

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
          char triClass = this->ClassifyTriangle(pts, ptClass, triPts);
          cInfo->OutTrisClass.emplace_back(triClass);
        }
      }

    } // for all candidate cells
  }   // operator()

  // Insert the triangulation into the filter's output.
  // For now this is a serial operation.
  void Reduce()
  {
    vtkIdType cId;
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
        cId = this->Output->InsertNextCell(cellType, npts, pts);
        this->CellLabels->InsertValue(cId, CellClassification::TargetCell);
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
          cId = this->Output->InsertNextCell(VTK_TRIANGLE, 3, tri);
          this->CellLabels->InsertValue(cId, cInfo->OutTrisClass[i]);
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

  // get the optional second output for debugging. Make sure it's empty initially.
  vtkPolyData* out2 = vtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
  out2->Initialize();

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
  // the target is performed to improve performance since only a subset of data
  // needs to be worked on.
  vtkNew<vtkPoints> outPts;
  outPts->DeepCopy(targetPts); // points will be appended to later
  output->SetPoints(outPts);
  vtkNew<vtkPolyData> candidateOutput;
  candidateOutput->SetPoints(outPts);

  // Now separate out the kept cells from the candidate cells. We are throwing
  // out any non-polygon cells. Kept cells are determined using bounding box
  // overlap tests.
  output->AllocateEstimate(numTargetCells, 3);
  candidateOutput->AllocateEstimate(numImprintCells, 3);

  // Use a threaded bounding-box intersection operation to separate the kept
  // candidate cells from the target cells. A locator for the imprint is used
  // to accelerate some geometric operations.
  vtkNew<vtkStaticCellLocator> impLocator;
  impLocator->SetDataSet(imprint);
  impLocator->BuildLocator();

  BoundsCull bc(
    target, imprint, impLocator, this->OutputType, this->Tolerance, candidateOutput, output);
  vtkSMPTools::For(0, numTargetCells, bc);

  // The desired output type may just be the target candidate cells.
  if (this->OutputType == TARGET_CELLS)
  {
    output->ShallowCopy(candidateOutput);
    return 1;
  }

  // Topological links are required for neighborhood information. This
  // information is needed for both the imprint and target candidate cells.
  candidateOutput->BuildLinks();
  imprint->BuildLinks();

  // Make sure candidate cells are available to imprint. Build a locator to
  // project imprint points onto the candidate target cells, where the target
  // is now a subset (i.e., the candidates) of the original input target.
  vtkIdType numCandidateCells = candidateOutput->GetNumberOfCells();
  if (numCandidateCells < 1)
  {
    vtkWarningMacro("Imprint not in contact with target");
    output->ShallowCopy(target);
    return 1;
  }
  vtkNew<vtkStaticCellLocator> candidateCellLocator;
  candidateCellLocator->SetDataSet(candidateOutput);
  candidateCellLocator->BuildLocator();

  // Adaptively classify the target points wrt the imprint. We avoid classifying all
  // of the points (there may be many); use topological checks whenever possible; and
  // use geometric checks as a last resort.
  TargetPointClassifier tpc(candidateOutput, impLocator, this->Tolerance);

  // Create an initial array of pointers to candidate cell information
  // structures, in which each struct contains information about the points
  // and edge fragments within each target candidate cell. This cell-by-cell
  // structure is used later for triangulation.
  vtkCandidateArray candidateArray(numCandidateCells, nullptr);

  // Now project all imprint points onto the target candidate cells. The
  // result is a classification of these points, typically interior but
  // sometimes on the edge or face of a target cell. Initially all imprint
  // points are placed in the vtkPointArray; however the output vtkPoints
  // points array will grow later when the edge intersection points are
  // computed.
  vtkPointArray pArray(numImprintPts);
  using ProjPointsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  ProjPointsWorker ppWorker;
  if (!ProjPointsDispatch::Execute(imprintPts->GetData(), ppWorker, candidateOutput,
        candidateCellLocator, &pArray, this->Tolerance, &tpc))
  {
    ppWorker(
      imprintPts->GetData(), candidateOutput, candidateCellLocator, &pArray, this->Tolerance, &tpc);
  }

  // With the points projected, add points classified as not-outside to the
  // candidate target cells for later triangulation.
  ProduceProjectedPoints ppp(outPts, &pArray, candidateOutput, &candidateArray);
  ppp();

  // Now produce edge intersection points and optionally constraint
  // edges. This an intersection of the imprint edges against the target
  // edges.
  ProduceIntersectionPoints pip(outPts, imprint, &pArray, candidateOutput, candidateCellLocator,
    &candidateArray, numTargetPts, this->Tolerance);
  vtkSMPTools::For(0, numImprintCells, pip);

  if (this->OutputType == IMPRINTED_CELLS)
  {
    // This eliminates the target cells that were passed through previously.
    output->ShallowCopy(candidateOutput);
  }

  // Triangulate the target candidate cells, and composite them into the
  // final filter output. The intersection points and/or triangulation
  // constraint edges are associated with the candidate cells via the
  // candidate array.
  Triangulate tri(outPts, &pArray, candidateOutput, &candidateArray, output, numTargetPts,
    this->DebugOutputType, this->DebugCellId, out2, &tpc);
  vtkSMPTools::For(0, numCandidateCells, tri);

  return 1;
}

//------------------------------------------------------------------------------
vtkPolyData* vtkImprintFilter::GetDebugOutput()
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
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

  os << indent << "Debug Output Type: " << this->DebugOutputType << "\n";
  os << indent << "Debug Cell Id: " << this->DebugCellId << "\n";
}
