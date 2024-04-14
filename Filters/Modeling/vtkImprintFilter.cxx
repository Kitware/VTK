// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImprintFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkAtomicMutex.h"
#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSignedCharArray.h"
#include "vtkStaticCellLocator.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <map>
#include <memory>
#include <mutex>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImprintFilter);

//------------------------------------------------------------------------------
// Instantiate object
vtkImprintFilter::vtkImprintFilter()
{
  this->Tolerance = 0.001;
  this->MergeTolerance = 0.025;
  this->MergeToleranceType = RELATIVE_TO_MIN_EDGE_LENGTH;
  this->ToleranceStrategy = DECOUPLED_TOLERANCES;

  this->OutputType = MERGED_IMPRINT;
  this->BoundaryEdgeInsertion = false;

  this->PassCellData = true;
  this->PassPointData = true;
  this->PointInterpolation = USE_TARGET_EDGES;

  this->TriangulateOutput = false;

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

// NOTE: The output points consist of a mixture of target points and imprint
// points, which are eventually assembled into a output vtkPoints. The target
// points are passed through, on to which some of the imprint points are
// appended. The appended imprint points consist of those imprint points that
// successfully are projected onto the target, plus those that are generated
// via edge intersection. Because some of the imprint points may be
// coincident (within a tolerance) of the target points, the imprint points
// may be a assigned a target point id.

// Imprint points are created via point projection, and edge intersection.
// The target points and edges also affect the imprinted area, those that are
// inside the imprint region are "mixed into" the imprinted area.

// Threading is performed most of the algorithm: point projection; line
// intersection; and triangulation are all threaded. Points can be projected
// in parallel, followed by a sequention process to assign them output VTK ids.
// Line intersection and triangulation proceed target cell-by-cell, the
// task decomposition is over the target cells.

// This enums assigns a classification to the points, both imprint and
// target points.
enum PointClassification
{
  TargetOutside = -4, // Target pt is outside the imprint region
  TargetInside = -3,  // Target pt is inside the imprint region
  Exclude = -2, // imprint point not to be projected (typically because it is not on the boundary)
  Unknown = -1, // initial, unknown classification
  Outside = 0,  // imprint points with classification <=0 are not inserted
  Interior = 1, // imprint pt is interior to target cell
  OnVertex = 2, // imprint pt is on a vertex of a target cell
  OnEdge = 3,   // imprint pt is on an edge of a target cell
};

// EdgeTuple is defined in vtkStaticEdgeLocatorTemplate. It represented an
// edge (V0,V1) with parametric coordinate T (i.e., Data) along the
// edge. Note that V0 < V1.
using vtkCellEdgeType = EdgeTuple<vtkIdType, double>;

// The following struct represents information relative points that are
// generated during the imprint operations. Points may be created from
// projection of the imprint onto target, or via intersection of the target
// and imprint edges.
struct vtkPointInfo
{
  char Classification; // Type of point
  vtkIdType VTKPtId;   // Which target VTK point/vertex does this map to? or is assigned?
  vtkIdType
    Cells[2]; // Which cell(s) does this point project to? <0 if misses target (e.g., Outside)
  vtkCellEdgeType TargetEdge;  // What target cell edge does this point lie on? (if applicable)
  vtkCellEdgeType ImprintEdge; // What imprint cell edge does this point lie on? (if applicable)
  double X[3];                 // Coordinates of projection

  vtkPointInfo()
    : Classification(PointClassification::Unknown)
    , VTKPtId(-1)
  {
    this->Cells[0] = this->Cells[1] = -1;
    this->TargetEdge.V0 = this->TargetEdge.V1 = -1;
    this->TargetEdge.Data = 0.0;
    this->ImprintEdge.V0 = this->ImprintEdge.V1 = -1;
    this->ImprintEdge.Data = 0.0;
  }
  vtkPointInfo(char classification, vtkIdType ptId, vtkIdType* cells, vtkIdType u0, vtkIdType u1,
    double tt, vtkIdType v0, vtkIdType v1, double ti, double x[3])
    : Classification(classification)
    , VTKPtId(ptId)
  {
    this->Cells[0] = cells[0];
    this->Cells[1] = cells[1];
    this->TargetEdge.V0 = u0;
    this->TargetEdge.V1 = u1;
    this->ImprintEdge.Data = tt;
    this->ImprintEdge.V0 = v0;
    this->ImprintEdge.V1 = v1;
    this->TargetEdge.Data = ti;
    this->X[0] = x[0];
    this->X[1] = x[1];
    this->X[2] = x[2];
  }
}; // vtkPointInfo

// Be wary of pointers to entries in the point array (i.e., vtkPointList),
// since new points may be added, meaning points may be invalidated at
// unexpected times. Hence access is necessarily via ids.  Note the
// vtkPointList keeps track of new points added due to the imprint. This
// includes projected points as well as newly created intersection points.
using vtkPointList = std::vector<vtkPointInfo>; // keep track of projected and intersected points
using vtkPointIdList =
  std::vector<vtkIdType>; // list of points (accessed via id) contained in candidate target cells

// Used to capture the intersection points along an imprint edge. Later,
// these are sorted and processed to insert edge fragments into target
// cells. Because intersection points are a mixture of thread local
// intersection points plus global (projected) imprint points, the edge
// intersection refers to (via Id) a point defined in one of potentially many
// vtkPointLists.  (This is a result of threading and the desire to generate
// intersection points within separate threads.)
struct vtkEdgeIntersection
{
  double T;                // parametric coordinate along an edge
  vtkIdType Id;            // the position that the point is located within the point list
  vtkPointList* PointList; // the list in which the point is located

  vtkEdgeIntersection()
    : T(0.0)
    , Id(-1)
    , PointList(nullptr)
  {
  }
  vtkEdgeIntersection(double t, vtkIdType id, vtkPointList* ptList)
    : T(t)
    , Id(id)
    , PointList(ptList)
  {
  }

  // Equivalence operator. See if two intersection points have the
  // same point id.
  bool operator==(vtkEdgeIntersection& edgeInt)
  {
    vtkPointInfo *p0 = nullptr, *p1 = nullptr;
    if (this->PointList != nullptr)
    {
      p0 = &(*this->PointList)[this->Id];
    }
    if (edgeInt.PointList != nullptr)
    {
      p1 = &(*edgeInt.PointList)[edgeInt.Id];
    }
    if (p0 != nullptr && p1 != nullptr && p0->VTKPtId >= 0 && p0->VTKPtId == p1->VTKPtId)
    {
      return true;
    }
    return false;
  }

  // Return the point information for a given id
  vtkPointInfo& GetPointInfo() { return (*this->PointList)[this->Id]; }

  // Sort intersections via parametric coordinate
  bool operator<(const vtkEdgeIntersection& eInt) const { return (this->T < eInt.T); }
};

// The intersection list contains a mix of points from potentially different
// point arrays.
struct vtkEdgeIntersectionList : public std::vector<vtkEdgeIntersection>
{
  // Remove duplicate points in the intersection list.
  // After sorting intersections along an edge, remove duplicate intersection
  // points.
  void CleanDuplicatePoints()
  {
    // Delete points that are duplicates (i.e., have the same VTK pt id)
    for (auto currentPt = this->begin(); currentPt != this->end(); ++currentPt)
    {
      auto nextPt = currentPt + 1;
      while (nextPt != this->end() && *nextPt == *currentPt)
      {
        this->erase(nextPt);
        nextPt = currentPt + 1;
      }
    }
  }
};

// Used to track imprint edges during the process of cell edge
// intersection.
struct vtkEmptyEdgeData
{
};
using vtkTargetEdgeType = EdgeTuple<vtkIdType, vtkEmptyEdgeData>;
using vtkTargetEdgeList = std::vector<vtkTargetEdgeType>;
using vtkTargetEdgeLocatorType = vtkStaticEdgeLocatorTemplate<vtkIdType, vtkEmptyEdgeData>;

// An edge fragment is defined by two imprint points. These points may reside
// in either global or local imprint point lists.
struct vtkEdgeFragment
{
  vtkEdgeIntersection V0; // The points that start and end the edge fragment
  vtkEdgeIntersection V1;
  vtkIdType CellId; // The target cell id in which this fragment exists

  vtkEdgeFragment(vtkEdgeIntersection v0, vtkEdgeIntersection v1, vtkIdType cellId)
    : CellId(cellId)
  {
    this->V0 = v0;
    this->V1 = v1;
  }
};
using vtkEdgeFragmentList = std::vector<vtkEdgeFragment>;

// Edge fragments are converted into edges which are triangulated. These
// edges use global VTK point ids (V0,V1).
//
struct vtkEdge
{
  vtkIdType V0;
  vtkIdType V1;

  vtkEdge(vtkIdType v0, vtkIdType v1)
    : V0(v0)
    , V1(v1)
  {
  }
};
using vtkEdgeList = std::vector<vtkEdge>;

// To build loops and triangulate, we need to keep track of which edges are
// visited. An edge can potentially be visited in two directions. Boundary
// edges are visited in only one direction; interior edges in both
// directions.  Conceptually, a vtkTriEdge is effectively a "edge use" - the
// use of an edge in a particular direction.
struct vtkTriEdge
{
  vtkIdType V0;
  vtkIdType V1;
  bool Visited;

  vtkTriEdge(vtkIdType v0, vtkIdType v1)
    : V0(v0)
    , V1(v1)
    , Visited(false)
  {
  }

  // Sort order: V0 first, then V1
  bool operator<(const vtkTriEdge& tedge) const
  {
    if (this->V0 < tedge.V0)
      return true;
    if (tedge.V0 < this->V0)
      return false;
    if (this->V1 < tedge.V1)
      return true;
    return false;
  }

  // Equivalence operator
  bool operator==(vtkTriEdge& edge) { return this->V0 == edge.V0 && this->V1 == edge.V1; }

  // Indicate whether the edge provided is a reversed edge to this one (i.e.,
  // same edge but opposite direction). This check is needed because we don't
  // want loops to travel back and forth along the same edge.
  bool IsReverseEdge(vtkTriEdge* edge) { return this->V0 == edge->V1 && this->V1 == edge->V0; }
};

// Supports the representation and construction of edge networks for the
// purposes of building loops.
struct vtkTriEdgeList : public std::vector<vtkTriEdge>
{
  vtkPoints* Points; // to support geometric queries

  vtkTriEdgeList(vtkPoints* pts)
    : Points(pts)
  {
  }

  // Internal structure for representing offsets
  struct vtkTriEdgeOffset
  {
    vtkIdType Offset;
    vtkIdType Num;
    vtkTriEdgeOffset(vtkIdType offset, vtkIdType num)
      : Offset(offset)
      , Num(num)
    {
    }
  };

  // Clear out the list and map
  void Clear()
  {
    this->clear();     // vector
    this->Map.clear(); // map
  }

  // Associate/map emanating edges from each vertex.
  std::map<vtkIdType, vtkTriEdgeOffset> Map;

  // Return the number of vertices in the edge network
  vtkIdType GetNumberOfVerts() { return this->Map.size(); }

  // Return an array of emanating edges from the specified vertex, as
  // well as a count as to the number of emanating edges.
  vtkTriEdge* GetVertexEdges(vtkIdType vId, vtkIdType& numEdges)
  {
    auto triEdgeOffset = this->Map.find(vId);
    if (triEdgeOffset != this->Map.end())
    {
      numEdges = triEdgeOffset->second.Num;
      vtkIdType offset = triEdgeOffset->second.Offset;
      return &((*this)[offset]);
    }
    else
    {
      numEdges = 0;
      return nullptr;
    }
  }

  // Clean up the edge network: this removes duplicate edges, or isolated
  // "spike" edges that fold back on themselves. Removing duplicates simply
  // means marking them "visited" so they won't be processed later. This
  // method should be invoked after BuildLinks(). It returns a flag
  // indicating whether any cleaning was performed. Long spikes segments
  // (consisting of multiple edges) require multiple passes to remove (e.g.,
  // an erosion operation). Hence CleanEdges() should be called repeatedly in
  // a loop until it returns false. Only rarely do the edge networks require
  // cleaning, but this method eliminates potential pathological cases and
  // enforces manifold loops.
  bool CleanEdges()
  {
    bool cleaned = false;

    // First mark duplicate edges as visited.
    for (auto currentEdge = this->begin(); currentEdge != this->end();)
    {
      auto nextEdge = currentEdge + 1;
      while (*currentEdge == *nextEdge && currentEdge != this->end())
      {
        cleaned = true;
        nextEdge->Visited = true;
        nextEdge++;
      }
      currentEdge = nextEdge;
    }

    // Now mark spikes as visited. A spike is defined by a vertex with
    // one incoming edge ei, and one outgoing edge e0, with ei a reverse
    // edge of e0.
    for (auto mapIter : this->Map)
    {
      vtkTriEdge *ei, *eo;
      vtkIdType numEiEdges, vId = mapIter.first;
      ei = this->GetVertexEdges(vId, numEiEdges);
      if (numEiEdges == 1)
      {
        vtkIdType numEoEdges;
        eo = this->GetVertexEdges(ei->V1, numEoEdges);
        if (numEoEdges == 1)
        {
          if (eo->V1 == ei->V0)
          {
            ei->Visited = true;
            eo->Visited = true;
            cleaned = true;
          }
        }
      }
    }

    return cleaned;
  }

  // Build offsets into the edges emanating from a particular vertex.
  void BuildLinks()
  {
    vtkTriEdgeList& triEdgeList = *this;

    // Loop over a sorted edge list, adding offsets
    vtkIdType numEdges = triEdgeList.size();
    vtkIdType offset = 0, currentOffset;
    while (offset < numEdges)
    {
      vtkTriEdge& triEdge = triEdgeList[offset];
      vtkIdType currentId = triEdge.V0;
      currentOffset = offset;
      while (++currentOffset < numEdges && triEdgeList[currentOffset].V0 == currentId)
        ; // advance
      vtkIdType num = currentOffset - offset;
      this->Map.emplace(currentId, vtkTriEdgeOffset(offset, num));
      offset += num;
    }

    // Clean up the edge network
    //    this->CleanEdges();
  }

  // Given the current vertex in the edge network, find the next edge in the
  // current loop. The method identifies the next edge in the loop, and
  // returns the end vertex of this next edge. (If no edge is found, then the
  // method returns <0.) Input to the method is an orientation normal, and
  // the current edge in the loop (which is updated on output). It returns
  // the next vertex id, and updates a pointer to the next edge.
  vtkIdType GetNextVertex(double normal[3], vtkTriEdge*& nextEdge)
  {
    // Find the edges that connect to the current edge id.
    vtkIdType numEdges, id = nextEdge->V1;
    vtkTriEdge* edge = this->GetVertexEdges(id, numEdges);

    // Choose the next, unvisited edge based on topological checks first,
    // then geometric. Firstly, if no visited edges.
    if (numEdges <= 0 || (numEdges == 1 && (edge->Visited || nextEdge->IsReverseEdge(edge))))
    {
      nextEdge = nullptr;
      return -1;
    }

    // A single non-visited edge
    else if (numEdges == 1 && !edge->Visited && !edge->IsReverseEdge(nextEdge))
    {
      nextEdge = edge;
      return edge->V1;
    }

    // The more complex cases with multiple edges to choose from.
    else
    {
      // Count the number of non-visited edges
      vtkTriEdge* ePtr = edge;
      vtkTriEdge* possibleEdge = nullptr;
      int numPossible = 0;
      // Count the number of non-visited edges
      for (auto i = 0; i < numEdges; ++i, ++ePtr)
      {
        if (!ePtr->Visited && !ePtr->IsReverseEdge(nextEdge))
        {
          ++numPossible;
          possibleEdge = ePtr;
        }
      } // for all candidate edges

      // If just one non-visited edge remains, return it
      if (numPossible == 1)
      {
        nextEdge = possibleEdge;
        return nextEdge->V1;
      }

      // If here, we need to choose the edge with the appropriate winding
      // turn. The turn should be in the direction consistent with the
      // orientation of the normal (using the right-hand-rule).
      double x[3], x0[3], x1[3], y[3], p[3], px1[3];
      double delX, delY, theta, smallestAngle = VTK_FLOAT_MAX;
      this->Points->GetPoint(nextEdge->V0, x0);
      this->Points->GetPoint(nextEdge->V1, x1);
      vtkMath::Subtract(x1, x0, x);
      vtkMath::Normalize(x);
      vtkMath::Cross(normal, x, y); // local coordinate system

      // Find the emanating edge with the smallest angle around the normal
      // (consistent with right-hand-rule)
      ePtr = edge;
      for (auto i = 0; i < numEdges; ++i, ++ePtr)
      {
        if (!ePtr->Visited && !ePtr->IsReverseEdge(nextEdge))
        {
          this->Points->GetPoint(ePtr->V1, p);
          vtkMath::Subtract(p, x1, px1);
          delX = vtkMath::Dot(px1, x);
          delY = vtkMath::Dot(px1, y);
          theta = atan2(delY, delX);
          theta = (theta >= 0 ? (vtkMath::Pi() - theta) : (vtkMath::Pi() - theta));
          if (theta < smallestAngle)
          {
            smallestAngle = theta;
            possibleEdge = ePtr;
          }
        }
      } // for all candidate edges
      nextEdge = possibleEdge;
      return (nextEdge != nullptr ? nextEdge->V1 : -1);
    }
  }

}; // vtkTriEdgeList

// Convenience typedefs for local representation of the
// output of the triangulation process.
using vtkOutCellsConn = std::vector<vtkIdType>;
using vtkOutCellsNPts = std::vector<vtkIdType>;
using vtkOutTrisClass = std::vector<char>;

// Below is the information gathered for target candidate cells that require
// tessellation. (Some target cells, initially identified through a bounding
// box overlap operation, may not require tessellation - hence candidate info
// is not gathered for them.) This information is used to tessellate a target
// candidate cell. Note that the term "tessellate" is used here, because they
// output may not be triangles.
struct vtkCandidateInfo
{
  vtkPointIdList PerimeterPoints; // All intersection points on the perimeter
  vtkPointIdList InteriorPoints;  // All points projected into the interior
  vtkEdgeList InteriorEdges;      // Edge fragments interior to the cell
  vtkOutCellsConn OutCellsConn;   // The output cells connectivity
  vtkOutCellsNPts OutCellsNPts;   // The output cells connectivity length
  vtkOutTrisClass OutCellsClass;  // The classification of the output cells
};
struct vtkCandidateList : public std::vector<vtkCandidateInfo*>
{
  vtkCandidateList(vtkIdType numCells)
    : std::vector<vtkCandidateInfo*>(numCells, nullptr)
  {
  }

  // Convenience method returns target candidate cell information. Will instantiate
  // a vtkCandidateInformation class as necessary.
  vtkCandidateInfo* GetCandidateInfo(vtkIdType cellId)
  {
    // Ensure a valid request is being made
    if (cellId < 0 || cellId >= static_cast<vtkIdType>(this->size()))
    {
      return nullptr;
    }

    // Honor the request
    vtkCandidateInfo*& cInfo = (*this)[cellId];
    if (cInfo == nullptr)
    {
      cInfo = new vtkCandidateInfo; // create new information if necessary
    }
    return cInfo;
  }
};

// Map candidate cells back to input target cells. This is used for
// debugging and for copying cell attribute data (if desired).
using vtkCellMapType = std::vector<vtkIdType>;

// A helper class used to manage point and cell attributes, and simplify the
// overall imprint code.
struct vtkAttributeManager
{
  // Support cell data processing
  vtkCellData* InCellData;
  vtkCellData* OutCellData;

  // Support point data processing
  vtkIdType NumTargetPts;                      // The number of input target points
  vtkIdType NumImprintPts;                     // The number of input imprint points
  int PointInterpolation;                      // Specify how to interpolate point data
  vtkSmartPointer<vtkPointData> TargetPtData;  // The target point data
  vtkSmartPointer<vtkPointData> ImprintPtData; // The imprint point data
  vtkPointData* OutPtData;                     // The filter output point data

  // Helper function to determine if two vtkDataSetAttributes are equivalent and may
  // be used together for copying and interpolating point data.
  bool HaveEquivalentAttributes(vtkDataSetAttributes* dsa0, vtkDataSetAttributes* dsa1)
  {
    // Check that the number of arrays is the same
    int numArrays0 = dsa0->GetNumberOfArrays();
    int numArrays1 = dsa1->GetNumberOfArrays();
    if (numArrays0 != numArrays1)
    {
      return false;
    }

    // Check that identical arrays exist in both (name, number of components, order)
    for (auto arrayNum = 0; arrayNum < numArrays0; ++arrayNum)
    {
      const char *name0, *name1;
      vtkDataArray* da0 = dsa0->GetArray(arrayNum);
      vtkDataArray* da1 = dsa1->GetArray(arrayNum);
      if (!da0 || !da1 || da0->GetDataType() != da1->GetDataType() ||
        da0->GetNumberOfComponents() != da1->GetNumberOfComponents() ||
        !(name0 = dsa0->GetArrayName(arrayNum)) || !(name1 = dsa1->GetArrayName(arrayNum)) ||
        strcmp(name0, name1) != 0)
      {
        return false;
      }
    }

    return true;
  }

  // Build point data attributes that are the set intersection of the mesh
  // point data attributes, and the loop point data attributes.  The
  // attributes must be equivalent because of the two-way interpolation/copy
  // data process that occurs.
  void IntersectAttributes(vtkPointData* targetPD, vtkPointData* imprintPD)
  {
    this->TargetPtData.TakeReference(vtkPointData::New());
    this->ImprintPtData.TakeReference(vtkPointData::New());

    // Loop over the the first attributes determining what data arrays are
    // common.
    int numTargetPDArrays = targetPD->GetNumberOfArrays();
    for (auto arrayNum = 0; arrayNum < numTargetPDArrays; ++arrayNum)
    {
      vtkDataArray *da0 = targetPD->GetArray(arrayNum), *da1;
      const char* name = targetPD->GetArrayName(arrayNum);
      if ((da1 = imprintPD->GetArray(name)) != nullptr &&
        da1->GetDataType() == da0->GetDataType() &&
        da1->GetNumberOfComponents() == da0->GetNumberOfComponents())
      {
        this->TargetPtData->AddArray(da0);
        this->ImprintPtData->AddArray(da1);
      }
    }
  }

  // Constructor: Initialize the process of attribute processing
  vtkAttributeManager(vtkPolyData* target, vtkPolyData* imprint, vtkPolyData* output, bool passCD,
    bool passPD, int ptInterpolation)
  {
    // Process cell data if requested
    this->InCellData = nullptr;
    this->OutCellData = nullptr;
    if (passCD)
    {
      this->InCellData = target->GetCellData();
      this->OutCellData = output->GetCellData();
      this->OutCellData->CopyAllocate(this->InCellData);
    }

    // Process point data if requested. Note the complication due to how the
    // new point data is interpolated from line-line intersections. The actual
    // allocation of output arrays does not occur here, rather in the method
    // ProducePointData() once the number of output points is known.
    this->NumTargetPts = target->GetNumberOfPoints();
    this->NumImprintPts = imprint->GetNumberOfPoints();
    this->PointInterpolation = ptInterpolation;
    this->TargetPtData = nullptr;
    this->ImprintPtData = nullptr;
    this->OutPtData = nullptr;
    if (passPD)
    {
      vtkPointData* targetPD = target->GetPointData();
      vtkPointData* imprintPD = imprint->GetPointData();
      // If trim edge interpoltion is specified, make sure that the trim loop point
      // attributes match the mesh point attributes. If not, point data is not passed
      // to the output.
      if (this->HaveEquivalentAttributes(targetPD, imprintPD))
      {
        this->TargetPtData = targetPD;
        this->ImprintPtData = imprintPD;
      }
      else
      {
        vtkLog(WARNING,
          "Target and imprint loop point data attributes are different, only common point data "
          "arrays "
          "will be processed");
        this->IntersectAttributes(targetPD, imprintPD);
      } // if point attributes identical

      // We now have consistent target, imprint, and filter output point data attributes.
      this->OutPtData = output->GetPointData();
      this->OutPtData->CopyAllocate(this->TargetPtData);
    } // if passing point data
  }   // constructor

  // Copy cell data from the input to the output.
  void CopyCellData(vtkIdType inCellId, vtkIdType outCellId)
  {
    if (this->InCellData)
    {
      this->OutCellData->CopyData(this->InCellData, inCellId, outCellId);
    }
  }
  void CopyCellData(vtkCellMapType* cellMap, vtkIdType inCellId, vtkIdType outCellId)
  {
    if (this->InCellData && cellMap)
    {
      this->OutCellData->CopyData(this->InCellData, (*cellMap)[inCellId], outCellId);
    }
  }

  // Processing point data is tricky because two sets of point attributes may
  // be in play. Point which are passed through, can just use the
  // conventional vtkDataSetAttributes::CopyData() approach (copy from input
  // mesh to output mesh). However, intersection points may be interpolated
  // from the target edges, or from the imprint edges. This means using the
  // correct point data to interpolate from (via InterpolateEdge()).

  // Interpolate or pass point data from either the target or imprint
  // edges, to the output mesh. There are three steps: 1) copy input target
  // point data to the output; 2) copy input imprint data to the output; and
  // 3) interpolate the point data across target or imprint edges.
  void ProducePointData(vtkPointList* pList)
  {
    // Make sure we actually want to process point data
    if (!this->OutPtData)
    {
      return;
    }

    // Gather information and initialize
    vtkIdType numTargetPts = this->NumTargetPts;
    vtkIdType numImprintedPts = pList->size();
    vtkPointData* targetPD = this->TargetPtData;
    vtkPointData* imprintPD = this->ImprintPtData;
    vtkPointData* outPD = this->OutPtData;

    // Step #1: copy the point data from the target to the filter output.
    for (vtkIdType pId = 0; pId < numTargetPts; ++pId)
    {
      outPD->CopyData(targetPD, pId, pId);
    }

    // Step #2: copy the point data from the imprint to the filter output.
    // There may be merged points which should be skipped. (Merged points
    // have the same point id as a target point.) This is combined with Step
    // #3: interpolate the point data across either the target or imprint
    // edges.
    for (vtkIdType pId = 0; pId < numImprintedPts; ++pId)
    {
      vtkPointInfo& pInfo = (*pList)[pId];
      if (pInfo.Classification == PointClassification::Interior)
      {
        outPD->CopyData(imprintPD, pId, pInfo.VTKPtId);
      }
      else if (pInfo.Classification == PointClassification::OnEdge)
      {
        if (this->PointInterpolation == vtkImprintFilter::USE_TARGET_EDGES)
        {
          outPD->InterpolateEdge(targetPD, pInfo.VTKPtId, pInfo.TargetEdge.V0, pInfo.TargetEdge.V1,
            pInfo.TargetEdge.Data);
        }
        else // if ( this->PointInterpolation == vtkImprintFilter::USE_IMPRINT_EDGES)
        {
          outPD->InterpolateEdge(imprintPD, pInfo.VTKPtId, pInfo.ImprintEdge.V0,
            pInfo.ImprintEdge.V1, pInfo.ImprintEdge.Data);
        }
      } // Edge interpolation
    }   // Copy / interpolate from target and imprint
  }     // ProducePointData()

}; // vtkAttributeManager

// Separate the kept candidate cells from the input target cells. This
// reduces the overall work/data that the algorithm must perform/process.
// Note one side effect of this functor is that the filter output may be
// partially constructed from those cells not in the imprinted region.
struct BoundsCull
{
  vtkPolyData* Target;                  // input vtkPolyData to algorithm
  vtkPolyData* Imprint;                 // imprinting vtkPolyData
  int OutputType;                       // control what is output
  double Tol;                           // tolerance
  vtkPolyData* CandidateOutput;         // kept cells
  vtkPolyData* Output;                  // initially, cells not processed by imprint
  vtkStaticCellLocator* ImprintLocator; // accelerate operation on imprint

  // Internal methods for performing the bounds culling operation.
  // The bounding box of the imprint mesh.
  vtkBoundingBox ImprintBounds;
  // CellMarks is used to mark cells to include as part of the output
  // candidate cells.
  std::vector<char> CellMarks;
  // If requested in the constructor, a CellMap is created which
  // maps the candidate cells back to their originating target cell id.
  vtkCellMapType* CellMap;
  // The attribute manager facilitates the copying and interpolation of
  // point and cell attribute data.
  vtkAttributeManager* AttributeManager;
  vtkImprintFilter* Filter;

  BoundsCull(vtkPolyData* target, vtkPolyData* imprint, vtkStaticCellLocator* impLoc,
    int outputType, double tol, vtkPolyData* candidateOutput, vtkPolyData* output,
    vtkCellMapType* cellMap, vtkAttributeManager* attrMgr, vtkImprintFilter* filter)
    : Target(target)
    , Imprint(imprint)
    , OutputType(outputType)
    , Tol(tol)
    , CandidateOutput(candidateOutput)
    , Output(output)
    , ImprintLocator(impLoc)
    , CellMap(cellMap)
    , AttributeManager(attrMgr)
    , Filter(filter)
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
    bool isFirst = vtkSMPTools::GetSingleThread();

    // Loop over target cells and mark those that should be kept.
    for (; cellId < endCellId; ++cellId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
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

  // Produce the final output: the candidate target cells that will be processed
  // by the algorithm. We also keep track of the mapping between the candidate
  // target cells and the original target cells. (This is potentially used later for
  // debugging - used when specifying DebugCellId.)
  void Reduce()
  {
    // Loop over all cell marks, and output the target candidate cells
    // accordingly. Note that if just the target candidate cells are desired,
    // or just the imprinted region is desired, the other cells are not output.
    vtkIdType npts;
    const vtkIdType* pts;
    vtkPolyData* target = this->Target;
    vtkPolyData* candidateOutput = this->CandidateOutput;
    vtkPolyData* output = this->Output;
    int outputType = this->OutputType;
    vtkIdType cellId;
    auto iter = this->CellMarks.begin();
    vtkAttributeManager* attrMgr = this->AttributeManager;

    for (cellId = 0; iter != this->CellMarks.end(); ++iter, ++cellId)
    {
      target->GetCellPoints(cellId, npts, pts);
      if (*iter > 0) // inserting target cells
      {
        candidateOutput->InsertNextCell(*iter, npts, pts);
        if (this->CellMap) // accumulate debugging information if requested
        {
          this->CellMap->push_back(cellId);
        }
      }
      else if (outputType != vtkImprintFilter::TARGET_CELLS &&
        outputType != vtkImprintFilter::IMPRINTED_REGION)
      {
        vtkIdType cId = output->InsertNextCell(-(*iter), npts, pts);
        attrMgr->CopyCellData(cellId, cId);
      }
    }
  } // Reduce()

}; // BoundsCull

// This struct manages the classification of the target points. It gathers
// information as imprint points are projected, and edges intersected, about
// the classification of points. If necessary it uses expensive geometric
// classification methods to determine the final target point classification.
struct vtkTargetPointClassifier
{
  vtkPolyData* Candidates;
  vtkPoints* CandidatePoints;
  vtkCellArray* CandidateCells;
  vtkStaticCellLocator* ImprintLocator;
  double ProjTol;
  double MergeTol;

  // Keep track of the classification of points. Because of potential simultaneous
  // accesses to point classifications, need to mutex.
  std::vector<vtkAtomicMutex> PtLocks;
  std::vector<char> PtClassification;

  // Scratch object for classifying points in parallel
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;

  vtkTargetPointClassifier(
    vtkPolyData* target, vtkStaticCellLocator* cellLoc, double projTol, double mergeTol)
    : Candidates(target)
    , ImprintLocator(cellLoc)
    , ProjTol(projTol)
    , MergeTol(mergeTol)
  {
    this->CandidatePoints = target->GetPoints();
    this->CandidateCells = target->GetPolys();
    vtkIdType numPts = target->GetNumberOfPoints();
    this->PtLocks.resize(numPts);
    this->PtClassification.resize(numPts, PointClassification::Unknown);
  }

  // Set the classification of a target point. It retains the most specialized
  // classification value.
  void SetClassification(vtkIdType ptId, char c)
  {
    char initialClass = this->PtClassification[ptId];
    if (initialClass == PointClassification::Unknown)
      this->PtClassification[ptId] = c;
  }

  // Get the classification of a target point.
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

    // Loop over cells, and classify the cell's points with an expensive
    // geometric query only if necessary. Note that this is why
    // PtLocks<vtkAtomicMutex> is used since a point may occasionally be
    // simultaneously accessed.
    for (; cellId < endCellId; cellId++)
    {
      targetIter->GetCellAtId(cellId, npts, pts);
      for (auto i = 0; i < npts; ++i)
      {
        vtkIdType pId = pts[i];
        std::lock_guard<vtkAtomicMutex> pointLockGuard(this->PtLocks[pId]);
        if (this->PtClassification[pId] == PointClassification::Unknown)
        {
          this->CandidatePoints->GetPoint(pId, x);
          int inout = this->ImprintLocator->FindClosestPointWithinRadius(
            x, this->ProjTol, closest, cell, cId, subId, dist2, inside);
          this->PtClassification[pId] =
            (inout ? PointClassification::TargetInside : PointClassification::TargetOutside);

        } // if point not previously classified
      }   // for cell points
    }     // for all cells in this batch
  }

  void Reduce() {}

}; // vtkTargetPointClassifier

// This class provides a lookup between a VTK point id, and its associated
// vtkPointInfo (which characterizes the information relative to imprint
// points). These points are produced from the imprint process, so any
// queries for vtkPointInfo with VTKId's < targetOffset returns a nullptr.
struct vtkImprintPointLookup : public std::vector<vtkPointInfo*>
{
  vtkPointList* PointList;
  vtkIdType TargetOffset; // number of target points
  vtkIdType NumNewPts;    // number of new VTK points added

  // The number of imprint points is (numVTKPts - targetOffset).
  vtkImprintPointLookup(vtkPoints* outPts, vtkPointList* pList, vtkIdType targetOffset)
    : PointList(pList)
    , TargetOffset(targetOffset)
  {
    this->NumNewPts = outPts->GetNumberOfPoints() - targetOffset;
    this->resize(this->NumNewPts);
  }

  // Get the information for an imprint point.
  vtkPointInfo* GetImprintPoint(vtkIdType VTKPtId)
  {
    if (VTKPtId < this->TargetOffset)
    {
      return nullptr;
    }
    else
    {
      return (*this)[VTKPtId - this->TargetOffset];
    }
  }

  // Build a table from VTK point id to imprint point. This needs to be
  // done after all of the imprint points have been added (because we
  // are storing pointers to std::vector - which may change is more
  // data is added to the vector).
  void BuildLookup()
  {
    // Loop over all imprint points, adding those with a VTK point id
    // that is outside of the target point id range.
    for (auto itr : *this->PointList)
    {
      if (itr.VTKPtId >= this->TargetOffset)
      {
        vtkIdType id = itr.VTKPtId - this->TargetOffset;
        (*this)[id] = &itr;
      }
    } // for all imprint points
  }   // BuildLookup

}; // vtkImprintPointLookup

// Convenience struct to provide classifications for a point (either target
// or imprint) referred to via its VTK point id. It also coordinates the
// classification of the target and imprint points.
struct vtkPointClassifier
{
  vtkTargetPointClassifier* TargetPtClassifier;
  vtkImprintPointLookup* ImprintPtLookup;
  vtkIdType TargetOffset;

  vtkPointClassifier(vtkTargetPointClassifier* tpc, vtkImprintPointLookup* ipl)
    : TargetPtClassifier(tpc)
    , ImprintPtLookup(ipl)
  {
    this->TargetOffset = ipl->TargetOffset;
    this->TargetPtClassifier->GeometricClassify();
    this->ImprintPtLookup->BuildLookup();
  }

  // Given a VTK point id, get its classification.
  char GetPointClassification(vtkIdType ptId)
  {
    if (ptId < this->TargetOffset)
    {
      return this->TargetPtClassifier->GetClassification(ptId);
    }
    else
    {
      vtkPointInfo* pInfo = (*this->ImprintPtLookup)[ptId - this->TargetOffset];
      return pInfo->Classification;
    }
  }
}; // vtkPointClassifier

// When the OutputType == ImprintBoundary, we need to determine which points are
// on the boundary of the imprint mesh.
void MarkBoundaryPoints(vtkPolyData* imprint, vtkPointList& pList)
{
  vtkIdType numPts = imprint->GetNumberOfPoints();
  vtkCellArray* cells = imprint->GetPolys();
  vtkIdType numCells = cells->GetNumberOfCells();
  auto iter = vtk::TakeSmartPointer(cells->NewIterator());
  vtkNew<vtkIdList> edgeNeighbors;
  vtkIdType npts;
  const vtkIdType* pts;

  // Initially mark all points as excluded from processing.
  for (auto ptId = 0; ptId < numPts; ++ptId)
  {
    pList[ptId].Classification = PointClassification::Exclude;
  }

  // Now mark boundary points for processing
  for (auto cellId = 0; cellId < numCells; ++cellId)
  {
    iter->GetCellAtId(cellId, npts, pts);
    for (auto i = 0; i < npts; ++i)
    {
      vtkIdType viStart = pts[i];
      vtkIdType viEnd = pts[(i + 1) % npts];
      imprint->GetCellEdgeNeighbors(cellId, viStart, viEnd, edgeNeighbors);
      if (edgeNeighbors->GetNumberOfIds() < 1) // is this a boundary edge?
      {
        // Mark these points
        pList[viStart].Classification = PointClassification::Unknown;
        pList[viEnd].Classification = PointClassification::Unknown;
      }
    }
  }
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
  vtkPointList* ImprintArray;
  double ProjTol;
  double ProjTol2;
  double MergeTol;
  double MergeTol2;
  vtkTargetPointClassifier* PtClassifier;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  vtkImprintFilter* Filter;

  ProjPoints(vtkPolyData* target, vtkStaticCellLocator* targetLoc, DataT* impPts,
    vtkPointList* pList, double projTol, double mergeTol, vtkTargetPointClassifier* tpc,
    vtkImprintFilter* filter)
    : Target(target)
    , CellLocator(targetLoc)
    , ImprintPts(impPts)
    , ImprintArray(pList)
    , ProjTol(projTol)
    , MergeTol(mergeTol)
    , PtClassifier(tpc)
    , Filter(filter)
  {
    this->TargetPts = target->GetPoints();
    this->TargetCells = target->GetPolys();
    this->NumTargetPts = target->GetNumberOfPoints();
    this->ProjTol2 = projTol * projTol;
    this->MergeTol2 = mergeTol * mergeTol;
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
    double x[3], dist2, closest[3], projTol = this->ProjTol;
    vtkIdType npts;
    const vtkIdType* pts;
    vtkNew<vtkIdList> edgeNeis;
    vtkTargetPointClassifier* tpc = this->PtClassifier;
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; ptId < endPtId; ptId++)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      vtkPointInfo& pt = (*this->ImprintArray)[ptId];

      if (pt.Classification == PointClassification::Exclude)
      {
        continue; // don't process this point
      }

      auto xTuple = imprintPts[ptId];
      x[0] = xTuple[0];
      x[1] = xTuple[1];
      x[2] = xTuple[2];

      // See if the imprint point projects onto the target
      if (!targetLoc->FindClosestPointWithinRadius(
            x, projTol, closest, cell, cellId, subId, dist2, inside))
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
          if (vtkMath::Distance2BetweenPoints(p0, pt.X) < this->MergeTol2)
          {
            pt.Classification = PointClassification::OnVertex;
            pt.VTKPtId = pts[i]; // The target point is on which the point projects
            pt.X[0] = p0[0];
            pt.X[1] = p0[1];
            pt.X[2] = p0[2];
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
            if (d2 <= this->MergeTol2)
            {
              candidateOutput->GetCellEdgeNeighbors(-1, v0, v1, edgeNeis);
              pt.Cells[0] = (edgeNeis->GetNumberOfIds() < 1 ? -1 : edgeNeis->GetId(0));
              pt.Cells[1] = (edgeNeis->GetNumberOfIds() < 2 ? -1 : edgeNeis->GetId(1));
              pt.Classification = PointClassification::OnEdge;
              pt.TargetEdge.Define(v0, v1); // this method implicitly reorders pt ids
              pt.TargetEdge.Data = (v0 < v1 ? t : (1.0 - t)); // t's origin is from smaller pt id
              // Due to potential edge interpolation, an imprint edge needs to be defined as well.
              // Use a hack to do so: edge(v,v) with t arbitrary.
              pt.ImprintEdge.V0 = pt.ImprintEdge.V1 = ptId;
              pt.ImprintEdge.Data = 0.0;
              pt.X[0] = closest[0];
              pt.X[1] = closest[1];
              pt.X[2] = closest[2];
              break;
            }
          } // if on edge
        }   // if need to check edges
      }     // Imprint point successfully projected onto target
    }       // For all imprint points
  }         // ProjPoints

  void Reduce() {}
}; // ProjPoints

// Glue between dispatch and point processing algorithm
struct ProjPointsWorker
{
  template <typename DataT>
  void operator()(DataT* impPts, vtkPolyData* candidateOutput, vtkStaticCellLocator* targetLoc,
    vtkPointList* pList, double projTol, double mergeTol, vtkTargetPointClassifier* tpc,
    vtkImprintFilter* filter)
  {
    vtkIdType numPts = impPts->GetNumberOfTuples();
    ProjPoints<DataT> pp(candidateOutput, targetLoc, impPts, pList, projTol, mergeTol, tpc, filter);
    vtkSMPTools::For(0, numPts, pp); // currently a non-thread-safe operation
  }
};

// Output projected points. If a point was not successfully projected, it retains its
// initial coordinates.
void OutputProjectedImprint(vtkPolyData* imprint, vtkPointList* pList, vtkPolyData* output)
{
  // Just output the imprint mesh.
  output->ShallowCopy(imprint);

  // However the imprint mesh point coordinates are updated.
  vtkPoints* outPts = output->GetPoints();
  vtkIdType numPts = outPts->GetNumberOfPoints();

  vtkNew<vtkPoints> newPts;
  newPts->SetDataType(outPts->GetDataType());
  newPts->SetNumberOfPoints(numPts);

  for (auto ptId = 0; ptId < numPts; ++ptId)
  {
    vtkPointInfo& pt = (*pList)[ptId];
    if (pt.Classification <= PointClassification::Outside)
    {
      continue; // point coordinates remain unchanged
    }

    // Update point coordinates
    newPts->SetPoint(ptId, pt.X);
  }

  output->SetPoints(newPts);
}

// Once point projection is completed, insert them into the output vtkPoints
// array and the candidate cells triangulation structure.  Also assign a
// global point id to the projected points - hence this method is serial.
struct ProduceProjectedPoints
{
  vtkPoints* OutPts;
  vtkPointList* PointList;
  vtkPolyData* CandidateOutput;
  vtkCandidateList* CandidateList;
  vtkIdType CurrentPtId; // used to assign VTK ids to the target points
  vtkImprintFilter* Filter;

  ProduceProjectedPoints(vtkPoints* outPts, vtkPointList* pList, vtkPolyData* candidateOutput,
    vtkCandidateList* candidateList, vtkImprintFilter* filter)
    : OutPts(outPts)
    , PointList(pList)
    , CandidateOutput(candidateOutput)
    , CandidateList(candidateList)
    , Filter(filter)
  {
  }

  void operator()()
  {
    vtkPoints* outPts = this->OutPts;
    vtkPointList* pList = this->PointList;
    vtkCandidateList* candidateList = this->CandidateList;

    vtkIdType numTargetPts = outPts->GetNumberOfPoints();
    vtkIdType numImprintPts = static_cast<vtkIdType>(pList->size());

    // Note that the projected points are converted to global id space.
    // That is, points are added at the end of the original target
    // points (all of them, not just the candidates).
    this->CurrentPtId = numTargetPts; // will be incremented

    // Traverse all these projected points, updating information in the candidate cells
    for (auto ptId = 0; ptId < numImprintPts; ++ptId)
    {
      this->Filter->CheckAbort();
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      vtkPointInfo& pt = (*pList)[ptId];
      if (pt.Classification <= PointClassification::Outside)
      {
        continue; // Nothing needs to be inserted into the vtkPoints output
      }
      else if (pt.Classification == PointClassification::OnVertex)
      {
        // The point is already created with a given VTKPtId.
        // Cell vertices will be treated later during triangulation since
        // the cell vertices are added to the triangulation process.
        continue;
      }
      else // requires creation of a new output point
      {
        vtkCandidateInfo* cInfo = candidateList->GetCandidateInfo(pt.Cells[0]);

        // Create a new output point from a successfully projected point
        outPts->InsertPoint(this->CurrentPtId, pt.X);
        pt.VTKPtId = this->CurrentPtId++;

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
          vtkCandidateInfo* cInfoE = candidateList->GetCandidateInfo(pt.Cells[1]);
          if (cInfoE != nullptr)
            cInfoE->PerimeterPoints.emplace_back(ptId + numTargetPts);
        } // on edge
      }   // requires some processing
    }     // for all imprint points
  }       // operator()
};        // ProduceProjectedPoints

// Intersect the imprint edges with the target candidate cells to produce
// intersection points on the boundary of the target cells, and edge
// fragments (which are portions of imprint edges contained in the target
// cells). Use a locator to identify potential target cells to intersect.

// Accumulate in the thread local intersection points and edge fragments.
struct vtkLocalIntData
{
  // The newly generated intersection points, on a per-thread basis,
  // are accumulated in the NewPoints thread local member.
  vtkPointList NewPoints;
  // The newly generated edge fragments are accumulated in the NewEdgee
  // thread local member.
  vtkEdgeFragmentList NewEdges;
};

struct ProduceIntersectionPoints
{
  bool BoundaryEdgeInsertion;
  vtkPoints* OutPts;
  vtkPolyData* Imprint;
  vtkCellArray* ImprintCells;
  vtkPointList* PointList;
  vtkPolyData* CandidateOutput;
  vtkCellArray* TargetCells;
  vtkStaticCellLocator* Locator;
  vtkCandidateList* CandidateList;
  vtkIdType TargetOffset;
  double ProjTol;
  double ProjTol2;
  double MergeTol;
  double MergeTol2;
  int TolStrategy;
  vtkTargetPointClassifier* PtClassifier;

  // Keep track of output points and cells
  vtkSMPThreadLocal<vtkLocalIntData> LocalIntData;
  vtkImprintFilter* Filter;

  ProduceIntersectionPoints(bool bedgeInsert, vtkPoints* outPts, vtkPolyData* imprint,
    vtkPointList* pList, vtkPolyData* candidateOutput, vtkStaticCellLocator* loc,
    vtkCandidateList* candidateList, vtkIdType offset, double projTol, double mergeTol,
    int tolStrategy, vtkTargetPointClassifier* tpc, vtkImprintFilter* filter)
    : BoundaryEdgeInsertion(bedgeInsert)
    , OutPts(outPts)
    , Imprint(imprint)
    , PointList(pList)
    , CandidateOutput(candidateOutput)
    , Locator(loc)
    , CandidateList(candidateList)
    , TargetOffset(offset)
    , ProjTol(projTol)
    , MergeTol(mergeTol)
    , TolStrategy(tolStrategy)
    , PtClassifier(tpc)
    , Filter(filter)
  {
    this->ImprintCells = this->Imprint->GetPolys();
    this->TargetCells = this->CandidateOutput->GetPolys();
    this->ProjTol2 = this->ProjTol * this->ProjTol;
    this->MergeTol2 = this->MergeTol * this->MergeTol;
  }

  // Get information about an imprint point.
  vtkPointInfo* GetPointInfo(vtkIdType ptId) { return &((*this->PointList)[ptId]); }

  // Get updated point coordinates. Since points can be projected or merged
  // due to coincidence, the coordinates of the imprint points must reflect
  // the updated coordinates.
  void GetUpdatedPoint(vtkIdType id, vtkPointInfo* vtkNotUsed(p), double x[3])
  {
    this->Imprint->GetPoint(id, x);
  }

  // Indicate whether this imprint cell is contained within a target cell.
  // Topological checks are used. Returns the id of the target candidate cell
  // if it is an interior edge; -1 otherwise.
  vtkIdType IsInteriorEdge(vtkPointInfo* pStart, vtkPointInfo* pEnd)
  {
    // Simple case: two interior points in the same target cell
    if ((pStart->Classification == PointClassification::Interior &&
          pEnd->Classification == PointClassification::Interior) &&
      (pStart->Cells[0] == pEnd->Cells[0])) // in the same target cell
    {
      return pStart->Cells[0];
    }
    else
    {
      return (-1); // By default, not an interior edge
    }
  } // IsInteriorEdge()

  // Evaluate the local topology to determine whether the intersection point
  // should be inserted. At entry into this method, it is known that the
  // intersection point is within ProjTol of an imprint edge end point.
  bool AddIntersectionPoint(
    vtkPointInfo* pStart, vtkPointInfo* pEnd, double x0[3], double x1[3], double u, double xInt[3])
  {
    // Determine which end of the imprint edge to consider.
    vtkPointInfo* pt;
    double* x;
    if (u < 0.5)
    {
      pt = pStart;
      x = x0;
    }
    else
    {
      pt = pEnd;
      x = x1;
    }

    // Determine whether the points are within MergeTol of one another. if so,
    // then they are considered the same point.
    bool coincident = (vtkMath::Distance2BetweenPoints(xInt, x) <= this->MergeTol2);

    // If the imprint edge end point is already classified on the edge, and the points are
    // coincident, then the intersection point is the same as the imprint edge end point.
    if (pt->Classification == PointClassification::OnEdge && coincident)
    {
      return false;
    }

    // Make sure the point is within parametric range (the FuzzyTolerance can result in
    // intersections outside of the edge).
    if (u < 0.0 || u > 1.0)
    {
      return false;
    }

    return true;
  }

  // Intersect the imprint edge defined by (x0,x1) with the target edge
  // (v0,v1). If there is an intersection, add the intersection point to the
  // list of intersections. This method returns non-zero if the resulting
  // edge may produce valid edge fragments (and requires further processing).
  // A return value == 1 means that all intersections were non-degenerate. A
  // return value == 2 means that a degenerate situation was encountered and
  // the edge intersections require cleaning. (A degeneracy occurs when a imprint
  // edge intersects a target cell at one of the target cell's vertices, possibly
  // producing duplicate intersection points.)
  int IntersectEdge(vtkPointInfo* pStart, vtkPointInfo* pEnd, vtkIdType u0, vtkIdType u1,
    double x0[3], double x1[3], vtkIdType v0, vtkIdType v1, vtkIdList* neighbors,
    vtkEdgeIntersectionList& eIntList)
  {
    // If the imprint edge is colinear with the target edge being intersected
    // (i.e., the imprint edge is on the boundary of the target cell), then
    // it will not produce any intersections; and furthermore cannot produce
    // any fragments.
    if (pStart->Classification == PointClassification::OnEdge &&
      pEnd->Classification == PointClassification::OnEdge &&
      (pStart->TargetEdge.V0 == pEnd->TargetEdge.V0 &&
        pStart->TargetEdge.V1 == pEnd->TargetEdge.V1))
    {
      return 0;
    }

    vtkPolyData* target = this->CandidateOutput;
    double y0[3], y1[3], u, v;

    // Note we compute parametric coordinates with the point of lowest edge
    // id at the origin. This is important later during triangulation for
    // sorting around the perimeter of the target cell.
    if (v0 > v1)
      std::swap(v0, v1);

    target->GetPoint(v0, y0);
    target->GetPoint(v1, y1);

    // Perform intersection, return if no intersection (i.e., don't add a
    // point). Recall parametric coordinate u is along the imprint edge; v is
    // along the target edge.
    if (vtkLine::Intersection(x0, x1, y0, y1, u, v, this->ProjTol, vtkLine::AbsoluteFuzzy) !=
      vtkLine::Intersect)
    {
      return 1;
    }

    // Okay we may need to add an intersection point. Check to see whether
    // the point is within tolerance of the target and imprint end points.
    double xInt[3]; // The intersection point on the target cell edge
    xInt[0] = y0[0] + v * (y1[0] - y0[0]);
    xInt[1] = y0[1] + v * (y1[1] - y0[1]);
    xInt[2] = y0[2] + v * (y1[2] - y0[2]);

    // If coincident to an imprint edge end point, depending on tolerances
    // and local topological reasons, it may or may not be necessary to add
    // the intersection point. This is because the imprint end point may
    // have already been added during point projection, and the intersection
    // point (within tolerance) is the same as the end point.
    if (vtkMath::Distance2BetweenPoints(xInt, x0) <= this->MergeTol2 ||
      vtkMath::Distance2BetweenPoints(xInt, x1) <= this->MergeTol2)
    {
      // Evaluate whether this intersection point should actually be
      // added.
      if (!this->AddIntersectionPoint(pStart, pEnd, x0, x1, u, xInt))
      {
        return 1;
      }
    }

    // If the intersection is coincident to a target edge end point, create
    // a new point labeled OnVertex. It's quite likely that a second,
    // coincident intersection point will occur as well - these duplicate
    // points along the edge are "cleaned up" and merged later (see
    // CleanDuplicatePoints).
    if (vtkMath::Distance2BetweenPoints(xInt, y0) <= this->MergeTol2 ||
      vtkMath::Distance2BetweenPoints(xInt, y1) <= this->MergeTol2)
    {
      vtkIdType vtkPtId =
        ((vtkMath::Distance2BetweenPoints(xInt, y0) <= this->MergeTol2) ? v0 : v1);

      vtkPointList& newPts = this->LocalIntData.Local().NewPoints;
      newPts.emplace_back();
      vtkPointInfo& pt = newPts.back();
      pt.Classification = PointClassification::OnVertex;
      this->PtClassifier->SetClassification(vtkPtId, PointClassification::OnVertex);
      pt.VTKPtId = vtkPtId; // The target point which the imprint edge intersects
      target->GetPoint(vtkPtId, pt.X);
      eIntList.emplace_back(u, newPts.size() - 1, &newPts);
      return 2;
    }

    // We can add a new point to the output of this thread. Later, a new
    // VTK point id will be set, and the new points composited together
    // during the Reduce() process.
    vtkPointList& newPts = this->LocalIntData.Local().NewPoints;
    vtkIdType cells[2];
    // The (-1) trick is used to get all cells using this edge.
    target->GetCellEdgeNeighbors(-1, v0, v1, neighbors);
    cells[0] = (neighbors->GetNumberOfIds() < 1 ? -1 : neighbors->GetId(0));
    cells[1] = (neighbors->GetNumberOfIds() < 2 ? -1 : neighbors->GetId(1));
    newPts.emplace_back(PointClassification::OnEdge, -1, cells, v0, v1, v, u0, u1, u, xInt);

    // For now, we are using local point ids. Later we'll update to refer to
    // global point ids. Note we have to use ids rather than pointers to
    // points because the vector is growing and may resize, invalidating
    // pointers.
    eIntList.emplace_back(vtkEdgeIntersection(u, newPts.size() - 1, &newPts));

    return 1;

  } // IntersectEdge()

  // Intersect two lists of cells to determine the common cell.
  vtkIdType IntersectCellLists(
    vtkIdType numCells0, vtkIdType* cells0, vtkIdType numCells1, vtkIdType* cells1)
  {
    vtkIdType intersectedCell = (-1);

    if (numCells0 > 0 && cells0 != nullptr && numCells1 > 0 && cells1 != nullptr)
    {
      for (auto i = 0; i < numCells0; ++i)
      {
        vtkIdType cell0 = cells0[i];
        if (cell0 >= 0)
        {
          for (auto j = 0; j < numCells1; ++j)
          {
            vtkIdType cell1 = cells1[j];
            if (cell0 == cell1)
            {
              intersectedCell = cell0;
              goto RETURN;
            } // if cells the same
          }   // for second list cells
        }     // if cell is non-negative
      }       // for first list cells
    }         // if valid lists

  RETURN:
    return intersectedCell;
  }

  // Determine whether this edge fragment is interior to a cell.
  bool IsInteriorFragment(vtkIdType cellId, vtkPointInfo& pStart, vtkPointInfo& pEnd)
  {
    // Make sure a valid cell is indicated
    if (cellId < 0)
    {
      return false;
    }

    // See if edge fragment is on an existing target edge
    if (pStart.Classification == PointClassification::OnVertex &&
      pEnd.Classification == PointClassification::OnEdge &&
      (pStart.VTKPtId == pEnd.TargetEdge.V0 || pStart.VTKPtId == pEnd.TargetEdge.V1))
    {
      return false;
    }

    else if (pEnd.Classification == PointClassification::OnVertex &&
      pStart.Classification == PointClassification::OnEdge &&
      (pEnd.VTKPtId == pStart.TargetEdge.V0 || pEnd.VTKPtId == pStart.TargetEdge.V1))
    {
      return false;
    }

    // See if edge fragment is a target edge
    else if (pStart.Classification == PointClassification::OnVertex &&
      pEnd.Classification == PointClassification::OnVertex)
    {
      if (this->CandidateOutput->IsEdge(pStart.VTKPtId, pEnd.VTKPtId) != 0)
      {
        return false;
      }
    }

    return true;
  }

  // Given sorted intersection points along an imprint edge, generate
  // portions of the edge (i.e., edge fragments) which are later
  // assembled to produce loops.
  //
  // This is a deceptively tricky classification process. Recall that
  // this algorithm inserts boundary edge fragments from intersections
  // along the boundary of the cell, in combination with cell vertices.
  // In this method, edge fragments interior to a cell are inserted.
  // Because of various forms of degeneracies (coincident vertices,
  // or overlapping, parallel edges), it is possible that the edge fragments
  // along the edge intersection list (edgeIntList) may not be interior
  // to the cell, but actually on the boundary of the cell. Hence there
  // are many topological checks in this method that insure that only
  // interior edge fragments are inserted.
  void ProduceInteriorEdgeFragments(vtkEdgeIntersectionList& edgeIntList)
  {
    vtkEdgeFragmentList& newEdges = this->LocalIntData.Local().NewEdges;

    // Now process pairs of points to define edge fragments. These fragments
    // are classified, and if interior to a target cell, added as an edge
    // fragment.
    vtkIdType outputCellId;
    vtkIdType numFrags = edgeIntList.size() - 1;
    for (auto i = 0; i < numFrags; ++i)
    {
      vtkPointInfo& pStart = edgeIntList[i].GetPointInfo();
      vtkPointInfo& pEnd = edgeIntList[i + 1].GetPointInfo();
      outputCellId = (-1);

      // Is one of the points outside? Skip if so.
      if (pStart.Classification == PointClassification::Outside ||
        pEnd.Classification == PointClassification::Outside)
      {
        // nothing output
      }

      // Is one of the points inside? Create fragment if so.
      else if (pStart.Classification == PointClassification::Interior ||
        pEnd.Classification == PointClassification::Interior)
      {
        outputCellId = (pStart.Classification == PointClassification::Interior ? pStart.Cells[0]
                                                                               : pEnd.Cells[0]);
      }

      // What's left is a combination of edge intersections, and coincident point
      // projections (i.e., OnVertex projections).
      // Maybe an edge-to-edge fragment. Find the common cell.
      else if (pStart.Classification == PointClassification::OnEdge &&
        pEnd.Classification == PointClassification::OnEdge)
      {
        if (pStart.Cells[0] >= 0 &&
          (pStart.Cells[0] == pEnd.Cells[0] || pStart.Cells[0] == pEnd.Cells[1]))
        {
          outputCellId = pStart.Cells[0];
        }
        else if (pStart.Cells[1] >= 0 &&
          (pStart.Cells[1] == pEnd.Cells[0] || pStart.Cells[1] == pEnd.Cells[1]))
        {
          outputCellId = pStart.Cells[1];
        }
        // But if a common cell is found, make sure that the points are not
        // on the *same* edge.  This can happen to due to point merging,
        // e.g., projected point is "moved" to a common edge. If on the same
        // edge, then the fragment can't be considered an interior edge
        // fragment.
        if (outputCellId >= 0 && (pStart.TargetEdge.V0 == pEnd.TargetEdge.V0) &&
          (pStart.TargetEdge.V1 == pEnd.TargetEdge.V1))
        {
          outputCellId = (-1);
        }
      }

      // The harder cases: the end points of the edge fragment are both on
      // the boundary of the cell (either classified on an edge or a
      // vertex). This gives four different cases that must be checked to
      // make sure that the edge fragment is interior to the cell (i.e., not
      // on the cell perimeter since these are taken care of through
      // different means).
      else
      {
        vtkSmartPointer<vtkIdList> startIds;
        vtkSmartPointer<vtkIdList> endIds;
        vtkIdType numStartCells = 0, numEndCells = 0;
        vtkIdType *startCells = nullptr, *endCells = nullptr;
        if (pStart.Classification == PointClassification::OnEdge)
        {
          startCells = pStart.Cells;
          numStartCells = 2;
        }
        else if (pStart.Classification == PointClassification::OnVertex && pStart.VTKPtId >= 0)
        {
          startIds = vtkSmartPointer<vtkIdList>::New();
          this->CandidateOutput->GetPointCells(pStart.VTKPtId, startIds);
          startCells = startIds->GetPointer(0);
          numStartCells = startIds->GetNumberOfIds();
        }

        if (pEnd.Classification == PointClassification::OnEdge)
        {
          endCells = pEnd.Cells;
          numEndCells = 2;
        }
        else if (pEnd.Classification == PointClassification::OnVertex && pEnd.VTKPtId >= 0)
        {
          endIds = vtkSmartPointer<vtkIdList>::New();
          this->CandidateOutput->GetPointCells(pEnd.VTKPtId, endIds);
          endCells = endIds->GetPointer(0);
          numEndCells = endIds->GetNumberOfIds();
        }

        outputCellId = this->IntersectCellLists(numStartCells, startCells, numEndCells, endCells);
        if (!this->IsInteriorFragment(outputCellId, pStart, pEnd))
        {
          outputCellId = (-1); // mark as invalid fragment
        }
      }

      // Output an edge fragment if appropriate.
      if (outputCellId >= 0)
      {
        newEdges.emplace_back(edgeIntList[i], edgeIntList[i + 1], outputCellId);
      }
    } // for all edge fragments
  }   // ProduceInteriorEdgeFragments

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
    vtkTargetEdgeList tEdges;
    vtkTargetEdgeLocatorType tLoc;

    // Keep track of intersections along the imprint edge
    vtkEdgeFragmentList& newEdges = this->LocalIntData.Local().NewEdges;
    vtkEdgeIntersectionList edgeIntList;
    bool isFirst = vtkSMPTools::GetSingleThread();

    // Loop over all imprint cells in this batch of cells and intersect the
    // appropriate cell's edges with the candidate target cells. To avoid
    // processing the same imprint edge twice, only process an imprint's cell
    // edge if the edge's cell neighbor id is larger then then the current
    // cellId; or the imprint edge is a boundary edge.
    for (; cellId < endCellId; ++cellId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      imprintIter->GetCellAtId(cellId, iNPts, iPts);
      for (auto i = 0; i < iNPts; ++i)
      {
        vtkIdType viStart = iPts[i];
        vtkIdType viEnd = iPts[(i + 1) % iNPts];
        vtkPointInfo* pStart = this->GetPointInfo(viStart);
        vtkPointInfo* pEnd = this->GetPointInfo(viEnd);

        // Gather all the potential target edges that may intersect this imprint edge.
        // In order to process each imprint edge only one time, the edge should be on
        // the boundary of the imprint region, or if it is an edge of the current cell
        // with a lesser cell id.
        vtkIdType tNPts;
        const vtkIdType* tPts;
        double xStart[3], xEnd[3];
        this->GetUpdatedPoint(viStart, pStart, xStart);
        this->GetUpdatedPoint(viEnd, pEnd, xEnd);

        // Check whether this imprint edge should be processed.
        imprint->GetCellEdgeNeighbors(cellId, viStart, viEnd, edgeNeighbors);
        if (edgeNeighbors->GetNumberOfIds() < 1 ||
          (!this->BoundaryEdgeInsertion && edgeNeighbors->GetId(0) > cellId))
        {
          // Clear data for the current imprint edge.
          tEdges.clear();

          // There are some simple cases that can avoid line-line the expense
          // of line-line intersection.  For example, if the end points of the
          // imprint edge lie on the same target cell, then the imprint edge is
          // within the (convex) target cell.
          vtkIdType candidateCell = this->IsInteriorEdge(pStart, pEnd);
          if (candidateCell >= 0)
          {
            newEdges.emplace_back(vtkEdgeIntersection(0.0, viStart, this->PointList),
              vtkEdgeIntersection(1.0, viEnd, this->PointList), candidateCell);
            continue;
          }

          // Identify the target candidate cells and consequently edges which may
          // intersect the current imprint edge.
          loc->FindCellsAlongLine(xStart, xEnd, this->ProjTol, cells);

          vtkIdType numCells = cells->GetNumberOfIds();
          for (auto j = 0; j < numCells; ++j)
          {
            vtkIdType targetCellId = cells->GetId(j);
            targetIter->GetCellAtId(targetCellId, tNPts, tPts);
            for (auto k = 0; k < tNPts; ++k)
            { // process each edge of this target cell
              vtkIdType vtStart = tPts[k];
              vtkIdType vtEnd = tPts[(k + 1) % tNPts];
              tEdges.emplace_back(vtkTargetEdgeType(vtStart, vtEnd, {}));
            }
          } // for all target cell candidates

          // To remove repeat target edges: Sort the target edges (i.e.,
          // group them together). Then process each target edge only once.
          vtkIdType numTargetEdges;
          const vtkIdType* tOffsets =
            tLoc.MergeEdges(static_cast<vtkIdType>(tEdges.size()), tEdges.data(), numTargetEdges);

          // Finally intersect the current imprint edge with the candidate
          // target edges.  This has the side affect of adding new
          // intersection points and interior edge fragments to the list of
          // intersection points and interior edges. Start by defining the
          // beginning and ending of the imprint edge in parametric space.
          edgeIntList.clear();
          edgeIntList.emplace_back(vtkEdgeIntersection(0.0, viStart, this->PointList));
          edgeIntList.emplace_back(vtkEdgeIntersection(1.0, viEnd, this->PointList));
          const vtkTargetEdgeType* tEdge;
          int produceFragments = 0;
          for (auto teNum = 0; teNum < numTargetEdges; ++teNum)
          {
            tEdge = tEdges.data() + tOffsets[teNum];
            produceFragments |= this->IntersectEdge(pStart, pEnd, viStart, viEnd, xStart, xEnd,
              tEdge->V0, tEdge->V1, edgeNeighbors, edgeIntList);
          } // for potential intersecting target edges

          // Collect edge fragments if intersections have been found. In
          // Reduce(), these will be associated with the correct target cell.
          if (produceFragments > 0)
          {
            // Sort the intersections along the imprint edge.
            std::sort(edgeIntList.begin(), edgeIntList.end());

            // If there may be duplicates let's eliminate them.
            if (produceFragments > 1)
            {
              edgeIntList.CleanDuplicatePoints();
            }

            // For this intersected target edge
            this->ProduceInteriorEdgeFragments(edgeIntList);
          } // if imprint edge intersects target edge
        }   // if should process this edge
      }     // for each imprint cell edge
    }       // for all imprint cells
  }         // operator()

  // Composite and number the generated points, add the points to the
  // candidate target cells for later triangulation. Also add edge fragments
  // to the target cells.
  void Reduce()
  {
    vtkIdType newPtId;
    vtkPointList* pList = this->PointList;
    vtkPoints* outPts = this->OutPts;

    // For each thread, copy thread's points into global arrays, and assign
    // a point id.
    vtkCandidateList* candidateList = this->CandidateList;
    vtkCandidateInfo* cInfo;

    for (auto ldItr = this->LocalIntData.begin(); ldItr != this->LocalIntData.end(); ++ldItr)
    {
      // Need to copy local points into the global imprint points array. Also need to
      // create VTK point id not already done.
      vtkPointList& newPts = (*ldItr).NewPoints;
      for (auto pIter = newPts.begin(); pIter != newPts.end(); ++pIter)
      {
        if ((newPtId = pIter->VTKPtId) < 0)
        {
          newPtId = outPts->InsertNextPoint(pIter->X);
          pIter->VTKPtId = newPtId; // Update the local VTK point id
        }
        pList->emplace_back(pIter->Classification, newPtId, pIter->Cells, pIter->TargetEdge.V0,
          pIter->TargetEdge.V1, pIter->TargetEdge.Data, pIter->ImprintEdge.V0,
          pIter->ImprintEdge.V1, pIter->ImprintEdge.Data, pIter->X);

        // Update the perimeter lists from line-line intersections.
        if (pIter->Classification == PointClassification::OnEdge)
        {
          for (auto i = 0; i < 2; ++i)
          {
            if ((cInfo = candidateList->GetCandidateInfo(pIter->Cells[i])) != nullptr)
            {
              cInfo->PerimeterPoints.emplace_back(
                static_cast<vtkIdType>(pList->size() - 1 + this->TargetOffset));
            }
          } // for cells on either side of edge
        }   // if edge intersection
      }     // for all intersection points

      // Now add edge fragments. Need to produce them using VTK ids.  (At
      // this point, all points should have an assigned VTK point id.)
      vtkEdgeFragmentList& eList = (*ldItr).NewEdges;
      for (auto eIter = eList.begin(); eIter != eList.end(); ++eIter)
      {
        if ((cInfo = candidateList->GetCandidateInfo(eIter->CellId)) != nullptr)
        {
          vtkEdgeFragment frag = *eIter;
          vtkPointInfo& v0 = frag.V0.GetPointInfo();
          vtkPointInfo& v1 = frag.V1.GetPointInfo();

          cInfo->InteriorEdges.emplace_back(v0.VTKPtId, v1.VTKPtId);
        }
      } // for each edge fragment
    }   // for all local data in threads
  }     // Reduce()

}; // ProduceIntersectionPoints

// As part of the triangulation, it's necessary to sort points around the
// boundary of each cell. This ultimately produces edges for the
// tessellation process.
struct vtkPerimeterPoint
{
  double T;
  vtkIdType Id;
  double X[3];

  vtkPerimeterPoint(double t, double x[3], vtkIdType id)
    : T(t)
    , Id(id)
  {
    this->X[0] = x[0];
    this->X[1] = x[1];
    this->X[2] = x[2];
  }

  // Used to support sorting points around the perimeter of the target candidate cell.
  bool operator<(const vtkPerimeterPoint& p) const { return (this->T < p.T); }
};
using vtkPerimeterList = std::vector<vtkPerimeterPoint>;

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
  vtkPointList* PointList;
  vtkPolyData* Candidates;
  vtkCandidateList* CandidateList;
  vtkPolyData* Output;
  vtkIdType TargetOffset;
  int OutputType;
  int DebugOption; // used for debugging
  vtkCellMapType* CellMap;
  vtkIdType DebugCellId;
  vtkPolyData* DebugOutput;
  vtkPointClassifier* PtClassifier;
  bool TriangulateOutput;
  vtkAttributeManager* AttributeManager;
  vtkSmartPointer<vtkSignedCharArray> CellLabels; // for cell labeling
  vtkImprintFilter* Filter;

  // Used for thread-based triangulation
  vtkSMPThreadLocal<vtkSmartPointer<vtkPolygon>> Polygon;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> OutTris;

  Triangulate(vtkPoints* outPts, vtkPointList* pl, vtkPolyData* candidates, vtkCandidateList* ca,
    vtkPolyData* output, vtkIdType offset, int outputType, int debugOption, vtkCellMapType* cellMap,
    vtkIdType debugCellId, vtkPolyData* debugOutput, vtkPointClassifier* pc, bool triOutput,
    vtkAttributeManager* attrMgr, vtkImprintFilter* filter)
    : OutPts(outPts)
    , PointList(pl)
    , Candidates(candidates)
    , CandidateList(ca)
    , Output(output)
    , TargetOffset(offset)
    , OutputType(outputType)
    , DebugOption(debugOption)
    , CellMap(cellMap)
    , DebugCellId(debugCellId)
    , DebugOutput(debugOutput)
    , PtClassifier(pc)
    , TriangulateOutput(triOutput)
    , AttributeManager(attrMgr)
    , Filter(filter)
  {
    // On entry into this method, all current cells are marked outside the
    // imprinted region. During triangulation, newly added cells will be
    // classified accordingly.
    vtkIdType numCells = this->Output->GetNumberOfCells();
    this->CellLabels = vtkSmartPointer<vtkSignedCharArray>::New();
    this->CellLabels->SetNumberOfTuples(numCells);
    this->CellLabels->Fill(CellClassification::TargetCell);
    this->CellLabels->SetName("ImprintedCells");
    this->Output->GetCellData()->AddArray(this->CellLabels);
  }

  // For debugging purposes: output the points provided as input to the
  // target cell triangulation process in the second output.
  void ProduceTriangulationInput(vtkIdType cellId)
  {
    vtkCandidateInfo*& cInfo = (*this->CandidateList)[cellId];

    // Setup the debugging output
    this->DebugOutput->SetPoints(this->OutPts);
    vtkNew<vtkCellArray> edges;
    this->DebugOutput->SetLines(edges);

    // Output the cell edges
    vtkIdType npts, e[2];
    const vtkIdType* pts;
    this->Candidates->GetCellPoints(cellId, npts, pts);
    for (auto i = 0; i < npts; ++i)
    {
      e[0] = pts[i];
      e[1] = pts[(i + 1) % npts];
      edges->InsertNextCell(2, e);
    }

    // Output the intersection edges
    vtkEdgeList& eList = cInfo->InteriorEdges;
    for (auto itr = eList.begin(); itr != eList.end(); ++itr)
    {
      vtkEdge& ef = *itr;
      e[0] = ef.V0;
      e[1] = ef.V1;
      edges->InsertNextCell(2, e);
    }
  }

  // For debugging purposes. Output the results of the target cell
  // triangulation in the second output.
  void ProduceTriangulationOutput(vtkIdType cellId)
  {
    vtkCandidateInfo*& cInfo = (*this->CandidateList)[cellId];
    this->DebugOutput->SetPoints(this->OutPts);
    vtkNew<vtkCellArray> polys;
    this->DebugOutput->SetPolys(polys);
    vtkIdType npts;
    const vtkIdType* pts;
    int cellType;

    vtkIdType numCells = static_cast<vtkIdType>(cInfo->OutCellsNPts.size());
    vtkIdType offset = 0;
    vtkIdType* conn = cInfo->OutCellsConn.data();
    for (auto i = 0; i < numCells; ++i)
    {
      npts = cInfo->OutCellsNPts[i];
      pts = conn + offset;
      cellType = (npts == 3 ? VTK_TRIANGLE : (npts == 4 ? VTK_QUAD : VTK_POLYGON));
      this->DebugOutput->InsertNextCell(cellType, npts, pts);
      offset += npts;
    } // for all cells in this target candidate cell
  }

  // Insert an edge intersection (perimeter) point into the cell's
  // list of perimeter points.
  void InsertPerimeterPoint(
    vtkIdType npts, const vtkIdType* pts, vtkPointInfo* pInfo, vtkPerimeterList& pList)
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
      if (v0 == pInfo->TargetEdge.V0 && v1 == pInfo->TargetEdge.V1)
      {
        double t = (swapped ? (1.0 - pInfo->TargetEdge.Data) : pInfo->TargetEdge.Data);
        t += static_cast<double>(eId);
        pList.emplace_back(t, pInfo->X, pInfo->VTKPtId);
        return;
      }
    } // for all edges
  }

  // Add edges around the perimeter of the target cell to the edge network.
  void AddPerimeterEdges(
    vtkIdType cellId, vtkIdType npts, const vtkIdType* pts, vtkTriEdgeList& triEdges)
  {
    vtkCandidateInfo*& cInfo = (*this->CandidateList)[cellId];
    vtkPoints* outPts = this->OutPts;
    vtkPointList& points = *this->PointList;
    vtkIdType targetOffset = this->TargetOffset;
    vtkPerimeterList pList;

    // Insert all of the points on the perimeter of the cell, including
    // the cell vertices. These will be sorted to create boundary edges.
    vtkIdType numPerimeterPts = static_cast<vtkIdType>(cInfo->PerimeterPoints.size());
    vtkIdType totalPerimeterPts = npts + numPerimeterPts;

    // Start by adding original cell points to the perimeter list
    double t, x[3];
    for (auto i = 0; i < npts; ++i)
    {
      outPts->GetPoint(pts[i], x);
      t = static_cast<double>(i);
      pList.emplace_back(t, x, pts[i]);
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
    std::sort(pList.begin(), pList.end());

    // Now add the perimeter edges. Recall that boundary edges are only used
    // one time, so only one edge use needs to be added.
    for (auto i = 0; i < totalPerimeterPts; ++i)
    {
      triEdges.emplace_back(vtkTriEdge(pList[i].Id, pList[(i + 1) % totalPerimeterPts].Id));
    }
  } // AddPerimeterEdges

  // Add edges interior to the target cell. There are two edge uses
  // (in opposite directions) per interior edge.
  void AddInteriorEdges(vtkIdType cellId, vtkTriEdgeList& triEdges)
  {
    vtkCandidateInfo*& cInfo = (*this->CandidateList)[cellId];
    vtkEdgeList& eList = cInfo->InteriorEdges;

    vtkIdType e[2];
    for (auto itr = eList.begin(); itr != eList.end(); ++itr)
    {
      vtkEdge& ef = *itr;
      e[0] = ef.V0;
      e[1] = ef.V1;
      triEdges.emplace_back(vtkTriEdge(e[0], e[1]));
      triEdges.emplace_back(vtkTriEdge(e[1], e[0]));
    }
  } // AddInteriorEdges

  // Classify a cell based on its vertex classifications. Basically, a
  // cell is outside it one of its points is classified as being
  // outside; otherwise it is an imprint cell.
  char ClassifyCell(vtkOutCellsConn& outCell)
  {
    vtkPointClassifier* pc = this->PtClassifier;
    for (auto itr : outCell)
    {
      if (pc->GetPointClassification(itr) == PointClassification::TargetOutside)
      {
        return CellClassification::TransitionCell;
      }
    }

    return CellClassification::ImprintCell;
  }

  // Classify a cell based on its vertex classifications. Basically, a
  // cell is outside it one of its points is classified as being
  // outside; otherwise it is an imprint cell.
  char ClassifyCell(vtkIdType npts, const vtkIdType* pts)
  {
    vtkPointClassifier* pc = this->PtClassifier;
    for (auto i = 0; i < npts; ++i)
    {
      if (pc->GetPointClassification(pts[i]) == PointClassification::TargetOutside)
      {
        return CellClassification::TransitionCell;
      }
    }

    return CellClassification::ImprintCell;
  }

  // This is a more robust triangulation method. It is adaptive in some
  // strange cases where ear-cut triangulation fails.
  int CellTriangulate(vtkOutCellsConn& outCell, vtkPolygon* poly, vtkIdList* outTris)
  {
    vtkPoints* pts = this->OutPts;
    vtkIdType npts = static_cast<vtkIdType>(outCell.size());
    double x[3];

    // Define the polygon, including determining the bounding box.
    for (vtkIdType i = 0; i < npts; ++i)
    {
      poly->PointIds->SetId(i, outCell[i]);
      pts->GetPoint(outCell[i], x);
      poly->Points->SetPoint(i, x);
    }

    // Return on successful triangulation
    if (poly->EarCutTriangulation(outTris, vtkPolygon::DOT_PRODUCT))
    {
      return 1;
    }

    return 0;
  }

  // Given a valid loop, add one or more cells to this target candidate cell
  // output. This may require triangulation of the loop, and produce more
  // than one cell. Later (in Reduce()), the output from all of these
  // candidate cells will be combined to create the final VTK output.
  void AddCell(vtkCandidateInfo* cInfo, vtkOutCellsConn& outCell)
  {
    auto npts = outCell.size();
    char cellClassification = this->ClassifyCell(outCell);

    // See if triangulation is required
    if (cellClassification == CellClassification::TransitionCell ||
      (npts > 3 && this->TriangulateOutput))
    {
      vtkPolygon* poly = this->Polygon.Local();
      vtkIdList* outTris = this->OutTris.Local();
      poly->PointIds->SetNumberOfIds(npts);
      poly->Points->SetNumberOfPoints(npts);

      // Now add a successful triangulation to this thread's output
      if (this->CellTriangulate(outCell, poly, outTris))
      {
        vtkIdType numTris = outTris->GetNumberOfIds() / 3;
        for (auto i = 0; i < numTris; ++i)
        {
          cInfo->OutCellsNPts.push_back(3);
          cInfo->OutCellsClass.push_back(cellClassification);
          cInfo->OutCellsConn.push_back(outCell[outTris->GetId(3 * i)]);
          cInfo->OutCellsConn.push_back(outCell[outTris->GetId(3 * i + 1)]);
          cInfo->OutCellsConn.push_back(outCell[outTris->GetId(3 * i + 2)]);
        }
      }
    }

    // Otherwise, add the cell as is.
    else
    {
      cInfo->OutCellsNPts.push_back(npts);
      std::move(outCell.begin(), outCell.end(), std::back_inserter(cInfo->OutCellsConn));
      cInfo->OutCellsClass.push_back(cellClassification);
    }
  }

  // Connect the edges to form loops. These loops become VTK cells. Some
  // cells may require triangulation. Note that the algorithm is based on a
  // combination of topological connectivity and geometric queries. The
  // geometric queries are used to orient the direction of the cells/loops,
  // and to help choose a "turning" direction when required. Recall that the
  // edge network consists of oriented edges (i.e., edge uses) in the
  // direction of (v0,v1).
  void BuildCells(vtkIdType cellId, vtkTriEdgeList& triEdgeList, double normal[3])
  {
    // Local arrays and variables to collect information about loop building.
    vtkCandidateInfo*& cInfo = (*this->CandidateList)[cellId];
    vtkOutCellsConn outCell;
    vtkIdType startId, nextId;

    // Traverse all the edges, marking them visited as they are incorporated
    // into loops. The edges are connected together based on topogical
    // information, and when a multiple choice for the next edge is required,
    // geometric orientation (in the same direction as the target cell) is
    // used to choose the correct turn to make.
    vtkTriEdge* edge = triEdgeList.data();
    vtkIdType numEdges = triEdgeList.size();
    for (auto i = 0; i < numEdges; ++edge, ++i)
    {
      if (!edge->Visited)
      {
        outCell.clear();
        bool success = true;
        vtkTriEdge* nextEdge = edge;
        startId = edge->V0;
        nextId = edge->V1;
        outCell.push_back(startId);

        // Build the loop
        while (nextId != startId)
        {
          if (nextId < 0) // has loop building failure occurred?
          {
            success = false;
            break;
          }
          else
          {
            outCell.push_back(nextId);
            nextEdge->Visited = true;
          }
          nextId = triEdgeList.GetNextVertex(normal, nextEdge);
        } // while building the loop
        if (success)
        {
          nextEdge->Visited = true; // close off the loop
        }

        // Make sure the loop is at least three points and a loop was
        // successfully built. If so, append it to the output cells.
        if (success && outCell.size() >= 3)
        {
          this->AddCell(cInfo, outCell);
        } // if valid loop built

      } // if edge has not yet been visited
    }   // for all edges in the edge network
  }     // BuildCells

  // SMP interface methods for triangulating the target cells in parallel.
  void Initialize()
  {
    this->Polygon.Local().TakeReference(vtkPolygon::New());
    this->Polygon.Local()->SetTolerance(0.0001);
    this->OutTris.Local().TakeReference(vtkIdList::New());
  }

  // On a target-cell by target-cell approach, tessellate each. This requires
  // building a network of edges, from which loops are extracted. These loops
  // may then be triangulated.
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkTriEdgeList triEdgeList(this->OutPts);
    double normal[3];
    bool isFirst = vtkSMPTools::GetSingleThread();

    // The cellId below is the candidate target cell id, not the target cell id,
    // so a mapping from candidate cell id to target cell id may be needed if
    // debugging information is requested (since the DebugCellId refers to
    // the target cell id, not the candidate cell id).
    for (; cellId < endCellId; cellId++)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      // Only cells requiring triangulation are processed.
      vtkCandidateInfo*& cInfo = (*this->CandidateList)[cellId];
      if (cInfo != nullptr)
      {
        // Produce requested debugging output
        if (this->DebugOption == vtkImprintFilter::TRIANGULATION_INPUT &&
          this->DebugOutput != nullptr && this->CellMap != nullptr)
        {
          const vtkCellMapType* cellMap = this->CellMap;
          if ((*cellMap)[cellId] == this->DebugCellId)
          {
            this->ProduceTriangulationInput(cellId);
          }
        }

        // Construct a network of triangulation edges. Clear out the
        // old data.
        triEdgeList.Clear();

        // Grab the current target cell information, and then add
        // edges around the target cell perimeter.
        vtkIdType npts;
        const vtkIdType* pts;
        this->Candidates->GetCellPoints(cellId, npts, pts);
        this->AddPerimeterEdges(cellId, npts, pts, triEdgeList);

        // Now add edges interior to the current cell to the edge network.
        this->AddInteriorEdges(cellId, triEdgeList);

        // Now build loops, i.e., polygons. Start by sorting edges: the sort
        // operation sorts (v0,v1) by v0 first, then by v1. The result is to
        // create groupings of all edges in the direction of (v0,vi) i.e., all
        // edges emanating from v0 (and so on).
        std::sort(triEdgeList.begin(), triEdgeList.end());

        // Now build an access structure so that it's possible to request
        // edges emanating from vertex vi. A map enables quick access into
        // the sorted edge list, along with the number of edges emanating
        // from a vertex.
        triEdgeList.BuildLinks();

        // Okay, we are ready to build the loops which eventually become VTK
        // cells.  Note that loop/cell building requires an orientation
        // normal in some cases.
        vtkPolygon::ComputeNormal(this->OutPts, npts, pts, normal);
        this->BuildCells(cellId, triEdgeList, normal);

        // Produce the requested debugging output
        if (this->DebugOption == vtkImprintFilter::TRIANGULATION_OUTPUT &&
          this->DebugOutput != nullptr && this->CellMap != nullptr)
        {
          const vtkCellMapType* cellMap = this->CellMap;
          if ((*cellMap)[cellId] == this->DebugCellId)
          {
            this->ProduceTriangulationOutput(cellId);
          }
        }

      } // if target cell needs triangulation
    }   // for all candidate cells
  }     // operator()

  // Insert the results of cell building into the filter's output.
  // This is a serial operation - it could be threaded if necessary.
  void Reduce()
  {
    vtkIdType cId;
    vtkIdType npts;
    const vtkIdType* pts;
    int cellType;
    vtkIdType numCandidates = static_cast<vtkIdType>(this->CandidateList->size());
    int outputType = this->OutputType;
    vtkAttributeManager* attrMgr = this->AttributeManager;

    for (auto cellId = 0; cellId < numCandidates; cellId++)
    {
      vtkCandidateInfo*& cInfo = (*this->CandidateList)[cellId];
      // Target cells not requiring triangulation are simply sent to the
      // output (unless only cells in the imprinted region are requested,
      // in which case make sure the cell is in the imprinted region).
      if (cInfo == nullptr)
      {
        char cellClassification = CellClassification::TargetCell;
        cellType = this->Candidates->GetCellType(cellId);
        this->Candidates->GetCellPoints(cellId, npts, pts);
        if (outputType != vtkImprintFilter::IMPRINTED_REGION ||
          (cellClassification = this->ClassifyCell(npts, pts)) == CellClassification::ImprintCell)
        {
          cId = this->Output->InsertNextCell(cellType, npts, pts);
          this->CellLabels->InsertValue(cId, cellClassification);
          attrMgr->CopyCellData(this->CellMap, cellId, cId);
        }
      }

      // Otherwise, the results of the tessellation are sent to
      // the output.
      else
      {
        vtkIdType numCells = static_cast<vtkIdType>(cInfo->OutCellsNPts.size());
        vtkIdType offset = 0;
        vtkIdType* conn = cInfo->OutCellsConn.data();
        for (auto i = 0; i < numCells; ++i)
        {
          npts = cInfo->OutCellsNPts[i];
          pts = conn + offset;
          cellType = (npts == 3 ? VTK_TRIANGLE : (npts == 4 ? VTK_QUAD : VTK_POLYGON));
          if (outputType != vtkImprintFilter::IMPRINTED_REGION ||
            cInfo->OutCellsClass[i] == ImprintCell)
          {
            cId = this->Output->InsertNextCell(cellType, npts, pts);
            this->CellLabels->InsertValue(cId, cInfo->OutCellsClass[i]);
            attrMgr->CopyCellData(this->CellMap, cellId, cId);
          }
          offset += npts;
        } // for all cells in this target candidate cell
      }   // target cell has been triangulated
    }     // for all candidate target cells
  }       // Reduce

}; // Triangulate

// Compute the minimum or average edge length of the target mesh.
struct AverageEdgeLengthType
{
  vtkIdType NumEdges;
  double Average;
  AverageEdgeLengthType()
    : NumEdges(0)
    , Average(0)
  {
  }
};

struct ComputeEdgeLength
{
  vtkPolyData* PData;
  double MinEdgeLength;
  double AverageEdgeLength;
  bool ComputeMinEdgeLen; // either edge min length or average edge length

  vtkSMPThreadLocal<double> MinLength2;
  vtkSMPThreadLocal<AverageEdgeLengthType> AveLength;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> EdgeNeighbors;

  ComputeEdgeLength(vtkPolyData* pd, bool computeMinEdgeLen)
    : PData(pd)
    , MinEdgeLength(VTK_FLOAT_MAX)
    , ComputeMinEdgeLen(computeMinEdgeLen)
  {
  }

  void Initialize()
  {
    this->MinLength2.Local() = VTK_FLOAT_MAX;
    vtkCellArray* ca = PData->GetPolys();
    this->CellIterator.Local().TakeReference(ca->NewIterator());
    this->EdgeNeighbors.Local().TakeReference(vtkIdList::New());
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkPolyData* pd = this->PData;
    double& minLength2 = this->MinLength2.Local();
    AverageEdgeLengthType& aveLength = this->AveLength.Local();
    vtkCellArrayIterator* iter = this->CellIterator.Local();
    vtkIdList* edgeNeighbors = this->EdgeNeighbors.Local();
    vtkIdType npts;
    const vtkIdType* pts;
    double x0[3], x1[3];

    // Loop over cells, and only process cell edges only when the
    // neighboring cell id is greater than the current cell id.
    for (; cellId < endCellId; cellId++)
    {
      iter->GetCellAtId(cellId, npts, pts);
      for (auto i = 0; i < npts; ++i)
      {
        vtkIdType v0 = pts[i];
        vtkIdType v1 = pts[(i + 1) % npts];
        pd->GetCellEdgeNeighbors(cellId, v0, v1, edgeNeighbors);
        // Only process edge if it's a boundary edge, or the id of the current
        // cell is less than the neighbor's id.
        if (edgeNeighbors->GetNumberOfIds() < 1 || edgeNeighbors->GetId(0) > cellId)
        {
          pd->GetPoint(v0, x0);
          pd->GetPoint(v1, x1);
          double len2 = vtkMath::Distance2BetweenPoints(x0, x1);
          if (this->ComputeMinEdgeLen)
          {
            minLength2 = (len2 < minLength2 ? len2 : minLength2);
          }
          else
          { // computing average length, accumulate running average
            aveLength.NumEdges++;
            aveLength.Average =
              aveLength.Average + ((sqrt(len2) - aveLength.Average) / aveLength.NumEdges);
          }
        }
      }
    }
  }

  void Reduce()
  {
    if (this->ComputeMinEdgeLen)
    {
      double minLength2 = VTK_FLOAT_MAX;
      auto lEnd = this->MinLength2.end();
      for (auto lItr = this->MinLength2.begin(); lItr != lEnd; ++lItr)
      {
        if (*lItr < minLength2)
        {
          minLength2 = *lItr;
        }
      }
      this->MinEdgeLength = sqrt(minLength2);
    } // computing min edge length
    else
    {
      // Determine the total number of edges processed.
      vtkIdType totalEdges = 0;
      auto aItr = this->AveLength.begin();
      auto aEnd = this->AveLength.end();
      for (; aItr != aEnd; ++aItr)
      {
        totalEdges += (*aItr).NumEdges;
      }

      // Now combine the averages computed on each thread
      double aveLength = 0.0;
      for (aItr = this->AveLength.begin(); aItr != aEnd; ++aItr)
      {
        aveLength += (static_cast<double>((*aItr).NumEdges) / totalEdges) * ((*aItr).Average);
      }
      this->AverageEdgeLength = aveLength;
    } // computing average edge length
  }

  // Cause execution of the min edge length calculation.
  static double GetMinLength(vtkPolyData* pdata)
  {
    ComputeEdgeLength compEdgeLen(pdata, true);
    vtkIdType numCells = pdata->GetNumberOfCells();
    vtkSMPTools::For(0, numCells, compEdgeLen);

    return compEdgeLen.MinEdgeLength;
  }

  // Cause execution of the average edge length calculation.
  static double GetAverageLength(vtkPolyData* pdata)
  {
    ComputeEdgeLength compEdgeLen(pdata, false);
    vtkIdType numCells = pdata->GetNumberOfCells();
    vtkSMPTools::For(0, numCells, compEdgeLen);

    return compEdgeLen.AverageEdgeLength;
  }

}; // ComputeEdgeLength

} // anonymous

//------------------------------------------------------------------------------
// Compute the tolerance used to merge near-coincident points. The polydata
// provided requires that cell links have been built.
double vtkImprintFilter::ComputeMergeTolerance(vtkPolyData* pdata)
{
  if (this->MergeToleranceType == RELATIVE_TO_PROJECTION_TOLERANCE)
  {
    return this->MergeTolerance * this->Tolerance;
  }
  else if (this->MergeToleranceType == RELATIVE_TO_MIN_EDGE_LENGTH)
  {
    return this->MergeTolerance * ComputeEdgeLength::GetMinLength(pdata);
  }
  else if (this->MergeToleranceType == RELATIVE_TO_AVERAGE_EDGE_LENGTH)
  {
    return this->MergeTolerance * ComputeEdgeLength::GetAverageLength(pdata);
  }
  else // if ( this->MergeToleranceType == ABSOLUTE )
  {
    return this->MergeTolerance;
  }
}

//------------------------------------------------------------------------------
int vtkImprintFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* targetInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* imprintInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* targetIn = vtkPolyData::SafeDownCast(targetInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* imprintIn =
    vtkPolyData::SafeDownCast(imprintInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // get the optional second output for debugging. Make sure it's empty initially.
  vtkPolyData* out2 = vtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
  out2->Initialize();

  // Initialize and check data
  vtkDebugMacro(<< "Imprinting...");

  // Make sure that the target and imprint inputs consists only of polygons.
  // Unfortunately some filters (like vtkCleanPolyData etc.) will output an
  // unexpected mix on occasion.
  vtkNew<vtkPolyData> target;
  target->ShallowCopy(targetIn);
  vtkNew<vtkPolyData> imprint;
  imprint->ShallowCopy(imprintIn);
  vtkNew<vtkCellArray> emptyCellArray;
  target->SetVerts(emptyCellArray);
  target->SetLines(emptyCellArray);
  target->SetStrips(emptyCellArray);
  imprint->SetVerts(emptyCellArray);
  imprint->SetLines(emptyCellArray);
  imprint->SetStrips(emptyCellArray);

  // Check the data
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
  impLocator->SetNumberOfCellsPerNode(5);
  impLocator->SetTolerance(this->Tolerance);
  impLocator->BuildLocator();

  // A cell map might be needed for debugging or for copying cell attribute data.
  vtkCellMapType cellMap;
  vtkCellMapType* cellMapPtr = &cellMap;
  if (this->DebugOutputType == NO_DEBUG_OUTPUT && !this->PassCellData)
  {
    cellMapPtr = nullptr; // indicate that no cell map is necessary
  }
  // Point and cell data attributes may be passed to the filter output.
  // The AttributeManager facilitates this.
  vtkAttributeManager attrMgr(
    target, imprint, output, this->PassCellData, this->PassPointData, this->PointInterpolation);

  // Here's where the kept and candidate target cells are separated. Also,
  // depending on the output, some of the target cells may be sent to the
  // filter output.
  BoundsCull bc(target, imprint, impLocator, this->OutputType, this->Tolerance, candidateOutput,
    output, cellMapPtr, &attrMgr, this);
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
  candidateCellLocator->SetNumberOfCellsPerNode(5);
  candidateCellLocator->SetTolerance(this->Tolerance);
  candidateCellLocator->BuildLocator();

  // Adaptively classify the target points wrt the imprint. We avoid classifying all
  // of the points (there may be many); use topological checks whenever possible; and
  // use geometric checks as a last resort.
  double projTol = this->Tolerance;
  double mergeTol = this->ComputeMergeTolerance(imprint);
  if (mergeTol <= 0.0)
  {
    mergeTol = 0.0;
    vtkWarningMacro("Merge tolerance <= 0.0");
  }
  if (this->ToleranceStrategy == LINKED_TOLERANCES)
  {
    projTol = mergeTol;
  }
  vtkDebugMacro(<< "(Projection) Tolerance: " << projTol << ",  Merge Tolerance: " << mergeTol);
  vtkTargetPointClassifier tpc(candidateOutput, impLocator, projTol, mergeTol);

  // Create an initial array of pointers to candidate cell information
  // structures, in which each struct contains information about the points
  // and edge fragments within each target candidate cell. This cell-by-cell
  // structure is used later for tessellation.
  vtkCandidateList candidateList(numCandidateCells);

  // The imprint points are projected and classified against the target. The
  // vtkPointList maintains information about these imprinted points.
  vtkPointList pList(numImprintPts);

  // If we are just inserting the imprint boundary points, we need to
  // identify these points. This is done by classifying them appropriately.
  if (this->BoundaryEdgeInsertion && this->OutputType != PROJECTED_IMPRINT)
  {
    MarkBoundaryPoints(imprint, pList);
  }

  // Now project all imprint points onto the target candidate cells. The
  // result is a classification of these points, typically interior but
  // sometimes on the edge or face of a target cell (or outside). Initially
  // all projected imprint points are placed in the vtkPointList; later the
  // output vtkPoints points array will grow as the edge intersection points
  // are computed and inserted.
  using ProjPointsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  ProjPointsWorker ppWorker;
  if (!ProjPointsDispatch::Execute(imprintPts->GetData(), ppWorker, candidateOutput,
        candidateCellLocator, &pList, projTol, mergeTol, &tpc, this))
  {
    ppWorker(imprintPts->GetData(), candidateOutput, candidateCellLocator, &pList, projTol,
      mergeTol, &tpc, this);
  }

  // If the desired output is a projection of the imprint onto the target,
  // the output is the imprint mesh with point coordinates modified by
  // projection onto the target.
  if (this->OutputType == PROJECTED_IMPRINT)
  {
    OutputProjectedImprint(imprint, &pList, output);
    return 1;
  }

  // With the imprint points projected, insert non-outside points into the
  // target cells for later tessellation. Also number the points (i.e., give
  // them VTK point ids) which is inherently a serial process. Finally, begin
  // building the lookup dictionary for the imprint points.
  ProduceProjectedPoints ppp(outPts, &pList, candidateOutput, &candidateList, this);
  ppp();

  // Now produce edge intersection points and edge fragments. This an
  // intersection of the imprint edges against the target edges.
  ProduceIntersectionPoints pip(this->BoundaryEdgeInsertion, outPts, imprint, &pList,
    candidateOutput, candidateCellLocator, &candidateList, numTargetPts, projTol, mergeTol,
    this->ToleranceStrategy, &tpc, this);
  vtkSMPTools::For(0, numImprintCells, pip);

  if (this->OutputType == IMPRINTED_CELLS)
  {
    // This shallow copy replaces the target cells that were passed through
    // previously.  Copy only the topology/geometry, leave the attributes
    // alone.
    output->CopyStructure(candidateOutput);
  }

  // Point classification is required as part of the triangulation process.
  // Both target and imprint points are classified.
  vtkImprintPointLookup ipl(outPts, &pList, numTargetPts);
  vtkPointClassifier pc(&tpc, &ipl);

  // Triangulate the target candidate cells, and composite them into the
  // final filter output. The intersection points and/or triangulation
  // constraint edges are associated with the candidate cells via the
  // candidate array.
  Triangulate tri(outPts, &pList, candidateOutput, &candidateList, output, numTargetPts,
    this->OutputType, this->DebugOutputType, cellMapPtr, this->DebugCellId, out2, &pc,
    this->TriangulateOutput, &attrMgr, this);
  vtkSMPTools::For(0, numCandidateCells, tri);

  // Finally produce output point data if requested. This is a combined operation
  // of copying points from the target and imprint, and interpolating point data
  // at intersecting edges.
  attrMgr.ProducePointData(&pList);

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

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  os << indent << "Merge Tolerance Type: " << this->MergeToleranceType << "\n";

  os << indent << "Tolerance Strategy: " << this->ToleranceStrategy << "\n";

  os << indent << "Output Type: " << this->OutputType << "\n";

  os << indent << "Boundary Edge Insertion: " << (this->BoundaryEdgeInsertion ? "On\n" : "Off\n");

  os << indent << "Pass Cell Data: " << (this->PassCellData ? "On\n" : "Off\n");
  os << indent << "Pass Point Data: " << (this->PassPointData ? "On\n" : "Off\n");
  os << indent << "Point Interpolation: " << this->PointInterpolation << "\n";

  os << indent << "Triangulate Output: " << (this->TriangulateOutput ? "On\n" : "Off\n");

  os << indent << "Debug Output Type: " << this->DebugOutputType << "\n";
  os << indent << "Debug Cell Id: " << this->DebugCellId << "\n";
}
VTK_ABI_NAMESPACE_END
