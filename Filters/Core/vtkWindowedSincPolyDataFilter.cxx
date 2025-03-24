// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWindowedSincPolyDataFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTriangle.h"
#include "vtkTriangleFilter.h"

#include <atomic>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWindowedSincPolyDataFilter);

//------------------------------------------------------------------------------
// Internal classes and methods for smoothing.
namespace
{ // anonymous

enum PointType
{
  FIXED = 0,   // never moves
  SMOOTHED = 1 // any point >0 smoothing connections is smoothed
};

// forward declare
template <typename TIds>
struct PointConnectivity;

// Compute normal vectors for mesh polygons. Only called if feature
// edge smoothing is enabled.
vtkSmartPointer<vtkDoubleArray> ComputeNormals(vtkPolyData* mesh)
{
  vtkPoints* pts = mesh->GetPoints();
  vtkCellArray* polys = mesh->GetPolys();
  vtkIdType numCells = polys->GetNumberOfCells();
  vtkNew<vtkDoubleArray> normals;
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(numCells);
  double* n = normals->GetPointer(0);

  vtkSMPTools::For(0, numCells,
    [&, pts, polys, n](vtkIdType cellId, vtkIdType endCellId)
    {
      vtkSmartPointer<vtkCellArrayIterator> cellIter;
      cellIter.TakeReference(polys->NewIterator());
      vtkIdType npts;
      const vtkIdType* points;
      double* normal = n + 3 * cellId;

      for (; cellId < endCellId; ++cellId, normal += 3)
      {
        cellIter->GetCellAtId(cellId, npts, points);
        vtkPolygon::ComputeNormal(pts, npts, points, normal);
      }
    }); // end lambda

  return normals;
} // ComputeNormals

// Process line edges. There are two "modes" in which this functor is
// called. In the first mode (Insertion==false), it's simply counting the
// number of incident edges. This information is later used to configure the
// output for threading (building offsets and such). Then, the functor is
// called again this time with (Insertion==true) which directs it to populate
// the edges in the point connectivity.
template <typename TIds>
struct LineConnectivity
{
  vtkCellArray* Lines;
  PointConnectivity<TIds>* PtConn;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> LineIterator;
  vtkWindowedSincPolyDataFilter* Filter;

  LineConnectivity(
    vtkCellArray* lines, PointConnectivity<TIds>* ptConn, vtkWindowedSincPolyDataFilter* filter)
    : Lines(lines)
    , PtConn(ptConn)
    , Filter(filter)
  {
  }

  void ProcessCell(bool closedLoop, vtkIdType npts, const vtkIdType* pts)
  {
    PointConnectivity<TIds>* ptConn = this->PtConn;

    // For all points in this line. In a closed loop, the first point==last point,
    // adjust to make the code saner.
    npts = (closedLoop ? (npts - 1) : npts);

    for (auto i = 0; i < npts; i++)
    {
      vtkIdType ptId = pts[i];

      // First point
      if (i == 0)
      {
        ptConn->AddEdge(ptId, pts[1]);
        if (closedLoop)
        {
          ptConn->AddEdge(ptId, pts[npts - 1]);
        }
      }

      // Last point
      else if (i == (npts - 1))
      {
        ptConn->AddEdge(ptId, pts[i - 1]);
        if (closedLoop)
        {
          ptConn->AddEdge(ptId, pts[0]);
        }
      }

      // In between point
      else
      {
        ptConn->AddEdge(ptId, pts[i + 1]);
        ptConn->AddEdge(ptId, pts[i - 1]);
      }

    } // for all points in this line
  }

  // SMP methods
  void Initialize() { this->LineIterator.Local().TakeReference(this->Lines->NewIterator()); }
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkCellArrayIterator* cellIter = this->LineIterator.Local();
    vtkIdType npts;
    const vtkIdType* pts;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endCellId - cellId) / 10 + 1, (vtkIdType)1000);

    for (; cellId < endCellId; ++cellId)
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

      cellIter->GetCellAtId(cellId, npts, pts);
      bool closedLoop = (pts[0] == pts[npts - 1] && npts > 3);
      this->ProcessCell(closedLoop, npts, pts);
    }
  }
  void Reduce() {}

  void Execute()
  {
    vtkIdType numLines = this->Lines->GetNumberOfCells();
    if (numLines > 0)
    {
      vtkSMPTools::For(0, numLines, *this);
    }
  }
}; // LineConnectivity

// Process mesh edges. There are two "modes" in which this functor is
// called. In the first mode (Insertion==false), it's simply counting the
// number of incident edges. This information is later used to configure the
// output for threading (building offsets and such). Then, the functor is
// called again this time with (Insertion==true) which directs it to populate
// the edges in the point connectivity.
// An important note: realize that the same edge (p1,p2) may be added more
// than once. This implicitly represents the counts of edge uses, and later
// on a sort will be used to combine the duplicates (in order to build a
// proper point smoothing network/stencil).
template <typename TIds>
struct MeshConnectivity
{
  vtkPolyData* Mesh;
  vtkCellArray* Polys;
  vtkPoints* Points;
  PointConnectivity<TIds>* PtConn;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> MeshIterator;
  vtkWindowedSincPolyDataFilter* Filter;

  MeshConnectivity(
    vtkPolyData* mesh, PointConnectivity<TIds>* ptConn, vtkWindowedSincPolyDataFilter* filter)
    : Mesh(mesh)
    , PtConn(ptConn)
    , Filter(filter)
  {
    this->Polys = this->Mesh->GetPolys();
    this->Points = this->Mesh->GetPoints();
  }

  void ProcessCell(vtkIdType npts, const vtkIdType* pts)
  {
    PointConnectivity<TIds>* ptConn = this->PtConn;

    for (auto i = 0; i < npts; i++)
    {
      vtkIdType ptId = pts[i];

      // First point
      if (i == 0)
      {
        ptConn->AddEdge(ptId, pts[npts - 1]);
        ptConn->AddEdge(ptId, pts[1]);
      }

      // Last point
      else if (i == (npts - 1))
      {
        ptConn->AddEdge(ptId, pts[i - 1]);
        ptConn->AddEdge(ptId, pts[0]);
      }

      // In between point (simple)
      else
      {
        ptConn->AddEdge(ptId, pts[i - 1]);
        ptConn->AddEdge(ptId, pts[i + 1]);
      }
    }
  } // Evaluate

  // SMP methods
  void Initialize() { this->MeshIterator.Local().TakeReference(this->Polys->NewIterator()); }
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkCellArrayIterator* cellIter = this->MeshIterator.Local();
    vtkIdType npts;
    const vtkIdType* pts;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endCellId - cellId) / 10 + 1, (vtkIdType)1000);

    for (; cellId < endCellId; ++cellId)
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
      cellIter->GetCellAtId(cellId, npts, pts);
      this->ProcessCell(npts, pts);
    }
  }
  void Reduce() {}

  void Execute()
  {
    vtkIdType numCells = this->Mesh->GetNumberOfCells();
    if (numCells > 0)
    {
      vtkSMPTools::For(0, numCells, *this);
    }
  }
}; // MeshConnectivity

// Class for representing and constructing the point connectivity. Templated
// on id type so that smaller ids can be used for memory reduction and speed
// improvements. Designed for threaded construction and access, and for
// efficient memory construction.
struct PointConnectivityBase
{
  vtkPolyData* Input;                      // input polydata we are working on
  vtkIdType NumPts;                        // total points in polydata
  vtkWindowedSincPolyDataFilter* Self;     // used to grab info from VTK class
  vtkSmartPointer<vtkPolyData> Mesh;       // optional 2D mesh derived from input
  vtkSmartPointer<vtkDoubleArray> Normals; // optional mesh polygon normals

  bool EdgeInsertion; // whether in edge counting mode, or edge insertion mode
  int OptLevel;       // level of optimization, 2 requires less analysis, 0 the most

  bool NonManifoldSmoothing;   // Control how smoothing is performed
  bool WeightNonManifoldEdges; // Control how smoothing is performed
  bool BoundarySmoothing;
  bool FeatureEdgeSmoothing;
  double CosFeatureAngle; // Cosine of angle between edge-adjacent polys
  double CosEdgeAngle;    // Cosine of angle between adjacent edges
  int NumberOfIterations;
  double PassBand;

  vtkIdType NumSimple; // Some statistics: the number of simple points
  vtkIdType NumFixed;  // The number of fixed points
  vtkIdType NumEdges;  // The number of edge points (feature, boundary, or non-manifold)

  PointConnectivityBase(vtkPolyData* input, vtkWindowedSincPolyDataFilter* self)
    : Input(input)
    , Self(self)
  {
    this->NumPts = input->GetNumberOfPoints();
    this->NonManifoldSmoothing = self->GetNonManifoldSmoothing();
    this->WeightNonManifoldEdges = self->GetWeightNonManifoldEdges();
    this->BoundarySmoothing = self->GetBoundarySmoothing();
    this->FeatureEdgeSmoothing = self->GetFeatureEdgeSmoothing();
    this->CosFeatureAngle = cos(vtkMath::RadiansFromDegrees(self->GetFeatureAngle()));
    this->CosEdgeAngle = cos(vtkMath::RadiansFromDegrees(self->GetEdgeAngle()));
    this->NumberOfIterations = self->GetNumberOfIterations();
    this->PassBand = self->GetPassBand();

    // Set the optimization level as appropriate to key options
    if (this->FeatureEdgeSmoothing)
    {
      // Requires topological and geometric analysis, normal generation, and
      // BuildLinks() (for edge neighbor information).
      this->OptLevel = 0;
    }
    else if (this->BoundarySmoothing || this->NonManifoldSmoothing)
    {
      // Requires topological analysis.
      this->OptLevel = 1;
    }
    else
    {
      // Requires less analysis. Either fixed (non-smoothed point), or simple
      // smoothed.
      this->OptLevel = 2;
    }
  }
  virtual ~PointConnectivityBase() = default;

  // Supports configuring connectivity (the counting and insertion
  // processes).
  void EdgeInsertionOn() { this->EdgeInsertion = true; }
  void EdgeInsertionOff() { this->EdgeInsertion = false; }
}; // PointConnectivityBase

// Limit the number of incident smoothing edges. Any unsigned or signed
// integer is okay, at the cost of additional memory and performance. If
// a point has more than this number of incident edges, it is classified
// as a fixed point (i.e., never moves).
typedef unsigned char EDGE_COUNT_TYPE;
#define MAX_EDGE_COUNT VTK_UNSIGNED_CHAR_MAX

template <typename TIds>
struct PointConnectivity : PointConnectivityBase
{
  // Offsets initially: Counts of edges per each point. After offsets are built:
  // Offsets into the incident edges (Edges) array. Needs to be atomic
  // because of potential simultaneous writes.
  std::atomic<TIds>* Offsets;
  TIds* Edges;                 // the connected points (which form incident edges)
  EDGE_COUNT_TYPE* EdgeCounts; // the number of smoothing edges

  PointConnectivity(vtkPolyData* input, vtkWindowedSincPolyDataFilter* self)
    : PointConnectivityBase(input, self)
  {
    // Offsets point into the list of incident edges for a particular point
    // id.  The edges represent all the incident edges to the
    // points. Initially there are duplicates edges; later on they are
    // rearranged. The EdgeCounts indicates the actual number of smoothing
    // edges. Note the type of EdgeCounts: this may limit the total number of
    // smoothing edges. This type can readily be changed (at compile time) to
    // accommodate more smoothing edges (although after a certain point,
    // additional edges make little difference, especially at the cost of
    // memory and speed).
    this->Offsets = new std::atomic<TIds>[this->NumPts + 1](); // Initialized to zero
    this->Edges = nullptr;                                     // initially until constructed
    this->EdgeCounts = new EDGE_COUNT_TYPE[this->NumPts];      // values set later
  }

  ~PointConnectivity() override
  {
    delete[] this->Offsets;
    delete[] this->Edges;
    delete[] this->EdgeCounts;
  }

  // The edge count indicates both a point's number of smoothing edges (not
  // the same as the point's number of incident edges), and its type. The
  // types are basically fixed (count==0), or smoothed (count>0), with a
  // count==2 meaning that the point is smoothed on an edge.  (Note that the
  // difference (Offsets[ptId+1]-Offsets[ptId]) indicates the number of
  // incident edges to a point.)
  EDGE_COUNT_TYPE GetEdgeCount(TIds ptId) { return this->EdgeCounts[ptId]; }
  void SetEdgeCount(TIds ptId, EDGE_COUNT_TYPE type) { this->EdgeCounts[ptId] = type; }

  // Supports populating the offsets and connectivity structure
  void AddEdge(TIds ptId, TIds id)
  {
    if (!this->EdgeInsertion) // we are counting
    {
      this->Offsets[ptId]++;
    }
    else
    {
      // Add an incident edge (ptId,id) to the connectivity array. Offset must have been built.
      // We are counting down from the initial offset.
      TIds offset = (--this->Offsets[ptId]);
      this->Edges[offset] = id;
    }
  }
  TIds GetOffset(TIds ptId)
  {
    // Offsets must have been built.
    return this->Offsets[ptId];
  }
  TIds GetNumberOfIncidentEdges(TIds ptId)
  {
    // Offsets must have been built.
    return (this->Offsets[ptId + 1] - this->Offsets[ptId]);
  }
  EDGE_COUNT_TYPE GetNumberOfSmoothingEdges(TIds ptId)
  {
    // Offsets must have been built.
    return this->EdgeCounts[ptId];
  }
  TIds* GetEdges(TIds ptId)
  {
    // Offsets must have been built.
    return (this->Edges + this->Offsets[ptId]);
  }
  void BuildOffsets()
  {
    // Prefix sum over the offsets. Someday (e.g., C++17) could use system
    // call to perform the prefix sum. The Offsets are initially setup at the
    // end of list of edges, and decremented until eventually they point at
    // the beginning of the list.
    TIds offset = 0;
    for (auto ptId = 0; ptId < this->NumPts; ++ptId)
    {
      offset += this->Offsets[ptId];
      this->Offsets[ptId] = offset;
    }
    this->Offsets[this->NumPts] = offset;

    // Now create space for edges to be written
    this->Edges = new TIds[offset];
  }
  void ConfigureOutput()
  {
    this->EdgeInsertionOff(); // edges will be counted

    LineConnectivity<TIds> lineConn(this->Input->GetLines(), this, this->Self);
    lineConn.Execute();

    // The mesh may need special treatment (e.g., triangulation
    // of triangle strips).
    vtkIdType numStrips = this->Input->GetStrips()->GetNumberOfCells();
    this->Mesh.TakeReference(vtkPolyData::New());
    this->Mesh->SetPoints(this->Input->GetPoints());
    if (numStrips > 0)
    {
      vtkNew<vtkPolyData> tmpMesh;
      tmpMesh->SetPoints(this->Input->GetPoints());
      tmpMesh->SetPolys(this->Input->GetPolys());
      tmpMesh->SetStrips(this->Input->GetStrips());
      vtkNew<vtkTriangleFilter> toTris;
      toTris->SetInputData(tmpMesh);
      toTris->SetContainerAlgorithm(this->Self);
      toTris->Update();
      this->Mesh->SetPolys(toTris->GetOutput()->GetPolys());
    }
    else
    {
      this->Mesh->SetPolys(this->Input->GetPolys());
    }

    // If possible, avoid building links. This is only necessary
    // when feature edge smoothing is enabled. This saves a lot
    // of time.
    if (this->OptLevel == 0)
    {
      this->Mesh->BuildLinks(); // for neighbor information
      //      this->Normals.TakeReference(ComputeNormals(this->Mesh)); // for feature edges
      this->Normals = ComputeNormals(this->Mesh); // for feature edges
    }

    MeshConnectivity<TIds> meshConn(this->Mesh, this, this->Self);
    meshConn.Execute();
  }
  void InsertEdges()
  {
    this->EdgeInsertionOn(); // incident edges will now be inserted

    LineConnectivity<TIds> lineConn(this->Input->GetLines(), this, this->Self);
    lineConn.Execute();

    MeshConnectivity<TIds> meshConn(this->Mesh, this, this->Self);
    meshConn.Execute();
  }
}; // PointConnectivity

// Various methods for performing local analysis of the region around each
// point to determine the smoothing stencil.  OptLevel==2: simple topological
// analysis. Points are either smoothed, or fixed. This is the fastest
// analysis.
template <typename TIds>
EDGE_COUNT_TYPE inline BuildO2Stencil(
  vtkIdType vtkNotUsed(ptId), TIds* edges, TIds nedges, PointConnectivity<TIds>* vtkNotUsed(ptConn))
{
  // Check the necessary condition that there is an even number of incident
  // edges (required if all edges are manifold). This is because all edges
  // come in pairs (if a point is interior to a manifold mesh).
  if ((nedges % 2))
  {
    return PointType::FIXED;
  }

  // Okay now see if we can group edges into pairs. If so, we have a manifold
  // situation. If not, the point may be on the boundary or in some unusual
  // nonmanifold state.
  TIds curEdge = (-1);
  int numPairs = nedges / 2;
  TIds* e = edges;

  for (auto i = 0; i < numPairs; ++i)
  {
    if (edges[2 * i] == curEdge ||      // if the id is the same as the previous pair
      edges[2 * i] != edges[2 * i + 1]) // or if the id is not the same for this pair
    {
      return PointType::FIXED; // the point is fixed
    }
    curEdge = edges[2 * i];
    *e++ = curEdge; // rearrange edges
  }

  return numPairs;
} // BuildO2Stencil

// Helper function compares the dot product between successive edges to the
// cosine of the angle between the edges.
template <typename TIds, typename TPts>
bool inline ExceedsEdgeAngle(vtkIdType ptId, TIds pt0, TIds pt1, double cosEdgeAngle, TPts* pts)
{
  const auto inPts = vtk::DataArrayTupleRange<3>(pts);
  const auto p0 = inPts[pt0];
  const auto p1 = inPts[ptId];
  const auto p2 = inPts[pt1];
  double l1[3], l2[3];

  for (auto k = 0; k < 3; k++)
  {
    l1[k] = p1[k] - p0[k];
    l2[k] = p2[k] - p1[k];
  }
  return vtkMath::Normalize(l1) >= 0.0 && vtkMath::Normalize(l2) >= 0.0 &&
    vtkMath::Dot(l1, l2) < cosEdgeAngle;
} // ExceedsEdgeAngle

// Various methods for performing local analysis of the region around a point
// to determine the smoothing stencil.  OptLevel==1: more complex topological
// analysis, plus geometric query for edge angle (if needed). Points may be
// fixed, or constrained to smooth along boundary or non-manifold edges. Feature
// edges are not considered.
template <typename TIds, typename TPts>
EDGE_COUNT_TYPE inline BuildO1Stencil(
  vtkIdType ptId, TIds* edges, TIds nedges, PointConnectivity<TIds>* ptConn, TPts* pts)
{
  // Likely this is the end of a polyline, the point shouldn't move, so marked "fixed".
  if (nedges == 1)
  {
    return PointType::FIXED;
  }

  TIds totalEdges = 0, numBEdges = 0, numNMEdges = 0;
  TIds bEdges[2];
  TIds eStart = 0, eEnd = 1, num;
  bool nmSmoothing = ptConn->NonManifoldSmoothing;
  bool weightNMEdges = ptConn->WeightNonManifoldEdges;

  // For the current point id ptId, group edges connected to ptId, count the
  // number of duplicates to determine the edge type, and reorder the edges
  // into the final smoothing stencil for ptId.
  while (true)
  {
    // Find a group of identical edges.
    while (eEnd < nedges && edges[eEnd] == edges[eStart])
    {
      ++eEnd;
    }

    // Now classify the edges, and move them into position for later
    // smoothing. Simple manifold edges are given no special treatment.
    // We keep track of boundary edges and possibly nonmanifold edges.
    num = (eEnd - eStart);
    if (num == 1) // boundary edge
    {
      // If more than two boundary edges are incident on ptId, then the
      // point is fixed.
      if (numBEdges == 2)
      {
        return PointType::FIXED;
      }
      // Keep track of boundary edges
      bEdges[numBEdges++] = edges[eStart];
    }

    // Nonmanifold edge might be treated as a boundary edge if nonmanifold
    // smoothing is off and number of nonmanifold edges == 2.
    else if (num > 2)
    {
      numNMEdges++;
    }

    // Copy the edge into a new position in the list of edges. If edges are
    // nonmanifold, and nonmanifold weighting is on, all instances of the
    // nonmanifold edges are copied into the smoothing stencil.
    edges[totalEdges++] = edges[eStart];
    if (nmSmoothing && weightNMEdges)
    {
      for (auto i = 0; i < (num - 1); ++i)
      {
        edges[totalEdges++] = edges[eStart];
      }
    }

    // Advance to the next group of edges, or break out if all edges have
    // been processed.
    if (eEnd >= nedges)
    {
      break;
    }
    eStart = eEnd++;
  } // while in list of edges

  // Let's see what the analysis reveals. If all simple edges, we have the
  // smoothing stencil. Also if nonmanifold smoothing and no boundary edges
  // also consider the nonmanifold edges as simple edges.
  if (numBEdges == 0)
  {
    if (nmSmoothing || numNMEdges == 0)
    {
      return totalEdges;
    }
  }

  // For point along boundary edges, we have two edges to smooth along. Check
  // that the angle between the two edges is less than the edge angle.
  else if (numBEdges == 2 && numNMEdges == 0)
  {
    if (ExceedsEdgeAngle(ptId, bEdges[0], bEdges[1], ptConn->CosEdgeAngle, pts))
    {
      return PointType::FIXED;
    }
    edges[0] = bEdges[0]; // smoothing on pair of boundary edges
    edges[1] = bEdges[1];
    return 2; // a pair of boundary edges which can be smoothed
  }

  // A complex collection of edges, don't smooth (i.e., fix the point).
  return PointType::FIXED;
} // BuildO1Stencil

// Various methods for performing local analysis of the region around a point
// to determine the smoothing stencil.  OptLevel==0: requires both geometric
// and topological analysis. Points may be fixed, or constrained to smooth
// along feature, boundary, or non-manifold edges.
template <typename TIds, typename TPts>
EDGE_COUNT_TYPE inline BuildO0Stencil(vtkIdType ptId, TIds* edges, TIds nedges,
  PointConnectivity<TIds>* ptConn, TPts* pts, vtkIdList* neighbors)
{
  // Likely this is the end of a polyline, the point shouldn't move
  if (nedges == 1)
  {
    return PointType::FIXED;
  }

  TIds totalEdges = 0, numFEdges = 0, numBEdges = 0, numNMEdges = 0;
  TIds fEdges[2], bEdges[2];
  TIds eStart = 0, eEnd = 1, num;
  bool nmSmoothing = ptConn->NonManifoldSmoothing;
  bool weightNMEdges = ptConn->WeightNonManifoldEdges;
  vtkPolyData* mesh = ptConn->Mesh;
  double* normals = ptConn->Normals->GetPointer(0);

  // For the current point id ptId, group edges connected to ptId, count the
  // number of duplicates to determine the edge type, and reorder the edges
  // into the final smoothing stencil for ptId.
  while (true)
  {
    // Find group of identical edges
    while (eEnd < nedges && edges[eEnd] == edges[eStart])
    {
      ++eEnd;
    }

    // Now classify the edges, and move them into position for later
    // smoothing. Simple manifold edges are given no special treatment.
    // We keep track of boundary edges and possibly nonmanifold edges.
    num = (eEnd - eStart);
    if (num == 1) // boundary edge
    {
      if (numBEdges == 2) // already have two edges, one more makes this fixed
      {
        return PointType::FIXED;
      }
      bEdges[numBEdges++] = edges[eStart];
    }

    // Simple manifold edge, but could be a feature edge. If more than two
    // feature edges are incident on ptId, then the point is fixed/
    else if (num == 2)
    {
      mesh->GetCellEdgeNeighbors((-1), ptId, edges[eStart], neighbors);
      double* n0 = normals + 3 * neighbors->GetId(0);
      double* n1 = normals + 3 * neighbors->GetId(1);
      if (vtkMath::Dot(n0, n1) <= ptConn->CosFeatureAngle)
      {
        // See if we already have two feature edges; if so it is fixed.
        if (numFEdges == 2)
        {
          return PointType::FIXED;
        }
        fEdges[numFEdges++] = edges[eStart];
      }
    }

    // Count the number of nonmanifold edges.
    else if (num > 2)
    {
      numNMEdges++;
    }

    // Copy the edge into a new position in the list of edges. If edges are
    // nonmanifold, and nonmanifold weighting is on, *all* instances of the
    // nonmanifold edges are copied into the smoothing stencil.
    edges[totalEdges++] = edges[eStart];
    if (nmSmoothing && weightNMEdges)
    {
      for (auto i = 0; i < (num - 1); ++i)
      {
        edges[totalEdges++] = edges[eStart];
      }
    }

    // Advance to the next group of edges, or break out if all edges have
    // been processed.
    if (eEnd >= nedges)
    {
      break;
    }
    eStart = eEnd++;
  } // while in list of edges

  // Let's see what the analysis reveals. If all simple edges, we have the
  // smoothing stencil. Also if nonmanifold smoothing and no boundary edges nor
  // feature edges consider the nonmanifold edges as simple edges.
  if (numBEdges == 0 && numFEdges == 0)
  {
    if (nmSmoothing || numNMEdges == 0)
    {
      return totalEdges;
    }
  }

  // See if ptId can be smoothed along a boundary edge
  else if (numBEdges == 2 && numFEdges == 0 && numNMEdges == 0)
  {
    if (ExceedsEdgeAngle(ptId, bEdges[0], bEdges[1], ptConn->CosEdgeAngle, pts))
    {
      return PointType::FIXED;
    }
    edges[0] = bEdges[0]; // smoothing on pair of boundary edges
    edges[1] = bEdges[1];
    return 2; // a pair of boundary edges which can be smoothed
  }

  // See if ptId can be smoothed along a feature edge
  else if (numBEdges == 0 && numFEdges == 2 && numNMEdges == 0)
  {
    if (ExceedsEdgeAngle(ptId, fEdges[0], fEdges[1], ptConn->CosEdgeAngle, pts))
    {
      return PointType::FIXED;
    }
    edges[0] = fEdges[0]; // smoothing on pair of feature edges
    edges[1] = fEdges[1];
    return 2; // a pair of boundary edges which can be smoothed
  }

  // a complex mess, don't smooth
  return PointType::FIXED;
} // BuildO0Stencil

// Perform point classification by examining local topology and/or geometry
// around each point. Update the count of the edges around the point that
// make up the smoothing stencil.
template <typename TIds, typename TPts>
struct AnalyzePoints
{
  TPts* Points;
  PointConnectivity<TIds>* PtConn;
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> Neighbors;
  vtkWindowedSincPolyDataFilter* Filter;

  AnalyzePoints(TPts* pts, PointConnectivity<TIds>* ptConn, vtkWindowedSincPolyDataFilter* filter)
    : Points(pts)
    , PtConn(ptConn)
    , Filter(filter)
  {
  }

  void Initialize() { this->Neighbors.Local().TakeReference(vtkIdList::New()); }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    PointConnectivity<TIds>* ptConn = this->PtConn;
    vtkIdList* neighbors = this->Neighbors.Local();
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

    for (; ptId < endPtId; ++ptId)
    {
      if (ptId % checkAbortInterval == 0)
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
      // First sort the local list of edges (i.e., the edges incident to
      // ptId). This will group duplicate edges (if any). Manifold edges
      // come in groups of two, boundary edges just a single edge, and
      // non-manifold have groups of more than two edges.
      TIds* edges = ptConn->Edges + ptConn->GetOffset(ptId);
      TIds nedges = ptConn->GetNumberOfIncidentEdges(ptId);
      std::sort(edges, edges + nedges);

      // Process the trivial cases. Note that if the number of incident edges
      // is really large, we assume that the point is fixed (because it is
      // not going to move anyway - it is likely overconstrained).
      if (nedges <= 0 || nedges >= 2 * MAX_EDGE_COUNT)
      {
        ptConn->SetEdgeCount(ptId, PointType::FIXED);
      }

      else
      {
        // Now rearrange the edges so that the first few are the ones used to
        // represent the smoothing stencil. Update the edge counts (indicating
        // number of edges in the stencil). As long as we modify just the local
        // list of edges, we are not going to collide with other threads.
        if (ptConn->OptLevel == 2) // simple or fixed
        {
          ptConn->SetEdgeCount(ptId, BuildO2Stencil(ptId, edges, nedges, ptConn));
        }
        else if (ptConn->OptLevel == 1) // simple, fixed, boundary, non-manifold edge smoothing
        {
          ptConn->SetEdgeCount(ptId, BuildO1Stencil(ptId, edges, nedges, ptConn, this->Points));
        }
        else // simple, fixed, or feature/boundary/non-manifold edge smoothing
        {
          ptConn->SetEdgeCount(
            ptId, BuildO0Stencil(ptId, edges, nedges, ptConn, this->Points, neighbors));
        }
      } // non trivial point
    }
  }

  void Reduce() {}

  void Execute()
  {
    vtkIdType numPts = this->Points->GetNumberOfTuples();
    if (numPts > 0)
    {
      vtkSMPTools::For(0, numPts, *this);
    }
  }
}; // AnalyzePoints

// Analyze points to develop smoothing stencil
struct AnalyzeWorker
{
  template <typename DataT1, typename TIds>
  void operator()(
    DataT1* pts, PointConnectivity<TIds>* ptConn, vtkWindowedSincPolyDataFilter* filter)
  {
    // This analyzes the surface mesh and polylines
    AnalyzePoints<TIds, DataT1> pppoints(pts, ptConn, filter);
    pppoints.Execute();
  }
}; // AnalyzeWorker

// Dispatch to the local point analysis.
template <typename TIds>
void AnalyzePointTopology(PointConnectivityBase* ptConnBase, vtkWindowedSincPolyDataFilter* filter)
{
  PointConnectivity<TIds>* ptConn = static_cast<PointConnectivity<TIds>*>(ptConnBase);
  vtkPoints* pts = ptConn->Input->GetPoints();

  // Need to dispatch on the type of points
  using vtkArrayDispatch::Reals;
  using AnalyzeDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  AnalyzeWorker ppWorker;
  if (!AnalyzeDispatch::Execute(pts->GetData(), ppWorker, ptConn, filter))
  { // Fallback to slowpath for other point types
    ppWorker(pts->GetData(), ptConn, filter);
  }

  // The last word comes from the vertices which will mark points fixed.
  vtkCellArray* verts = ptConn->Input->GetVerts();
  vtkIdType numVerts = (verts != nullptr ? verts->GetNumberOfCells() : 0);
  if (numVerts > 0 && !filter->CheckAbort())
  {
    vtkSMPTools::For(0, numVerts,
      [&, verts, ptConn](vtkIdType cellId, vtkIdType endCellId)
      {
        vtkSmartPointer<vtkCellArrayIterator> vIter;
        vIter.TakeReference(verts->NewIterator());
        vtkIdType npts;
        const vtkIdType* p;

        for (; cellId < endCellId; ++cellId)
        {
          vIter->GetCellAtId(cellId, npts, p);
          for (auto j = 0; j < npts; ++j)
          {
            ptConn->SetEdgeCount(p[j], PointType::FIXED);
          }
        }
      }); // end lambda
  }       // if any verts
} // AnalyzePointTopology

// Initialize points prior to applying smoothing operations.
struct InitializePointsWorker
{
  template <typename DataT1, typename DataT2>
  void operator()(DataT1* inPts, DataT2* outPts, vtkIdType numPts, int normalize, double length,
    double center[3], vtkWindowedSincPolyDataFilter* filter)
  {
    vtkSMPTools::For(0, numPts,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        const auto inTuples = vtk::DataArrayTupleRange<3>(inPts);
        auto outTuples = vtk::DataArrayTupleRange<3>(outPts);
        double x[3];
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

        for (; ptId < endPtId; ++ptId)
        {
          if (ptId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              filter->CheckAbort();
            }
            if (filter->GetAbortOutput())
            {
              break;
            }
          }
          const auto inTuple = inTuples[ptId];
          auto outTuple = outTuples[ptId];
          x[0] = static_cast<double>(inTuple[0]);
          x[1] = static_cast<double>(inTuple[1]);
          x[2] = static_cast<double>(inTuple[2]);

          if (normalize)
          {
            x[0] = (x[0] - center[0]) / length;
            x[1] = (x[1] - center[1]) / length;
            x[2] = (x[2] - center[2]) / length;
          }

          // Now set the value of the new points
          outTuple[0] = x[0];
          outTuple[1] = x[1];
          outTuple[2] = x[2];
        } // for all points
      }); // end lambda
  }
}; // InitializePointsWorker

// Initialize points including possibly normalizing them.
// Currently the output points are the same type as the
// input points - could be user specified.
vtkSmartPointer<vtkPoints> InitializePoints(int normalize, vtkPolyData* input, double& length,
  double center[3], vtkWindowedSincPolyDataFilter* filter)
{
  vtkPoints* inPts = input->GetPoints();
  vtkIdType numPts = inPts->GetNumberOfPoints();
  vtkNew<vtkPoints> newPts;
  newPts->SetDataType(inPts->GetDataType());
  newPts->SetNumberOfPoints(numPts);

  // May need to grab normalization info which can be expensive
  if (normalize)
  {
    length = input->GetLength();
    input->GetCenter(center);
  }

  using vtkArrayDispatch::Reals;
  using InitializePointsDispatch =
    vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::Reals>;
  InitializePointsWorker initPtsWorker;
  if (!InitializePointsDispatch::Execute(inPts->GetData(), newPts->GetData(), initPtsWorker, numPts,
        normalize, length, center, filter))
  { // Fallback to slowpath for other point types
    initPtsWorker(inPts->GetData(), newPts->GetData(), numPts, normalize, length, center, filter);
  }

  return newPts;
} // InitializePoints

// Driver function builds smoothing connectivity (i.e., the stencil of
// smoothing edges). The connectivity that it allocates must be deleted later
// by the caller.
template <typename TIds>
PointConnectivityBase* BuildConnectivity(vtkPolyData* input, vtkWindowedSincPolyDataFilter* self)
{
  PointConnectivity<TIds>* ptConn = new PointConnectivity<TIds>(input, self);

  // First we have to configure / count the output
  ptConn->ConfigureOutput();

  // Now build the data structure e.g. offsets. This requires
  // a prefix sum over the initial counts to build the offsets.
  ptConn->BuildOffsets();

  // Now insert the edges into the vertex connectivity.
  ptConn->InsertEdges();

  return ptConn;
} // PointConnectivityBase

// Calculation of the Chebychev coefficients c.  Currently this process is
// not threaded: since it ends on convergence, typically after say maybe
// 20-40 iterations, it is probably not worth it. The functor is dependent
// on the PassBand, and number of iterations.
struct CoefficientsWorker
{
  template <typename TIds>
  void operator()(PointConnectivity<TIds>* ptConn, int numIters, double* c)
  {
    int i, j;
    double theta_pb, k_pb, sigma;

    // Allocate scratch arrays
    std::vector<double> w(numIters + 1);
    std::vector<double> cprime(numIters + 1);

    // calculate weights and filter coefficients
    k_pb = ptConn->PassBand;           // reasonable default for k_pb in [0, 2] is 0.1
    theta_pb = acos(1.0 - 0.5 * k_pb); // theta_pb in [0, M_PI/2]

    // Windowed sinc function weights. This is for a Hamming window. Other
    // window function could be implemented here. Probably not worth threading
    // since typically there are so few iterations.
    switch (ptConn->Self->GetWindowFunction())
    {
      case vtkWindowedSincPolyDataFilter::NUTTALL:
        for (i = 0; i <= numIters; i++)
        {
          w[i] = 0.355768 + 0.487396 * cos(((double)i) * vtkMath::Pi() / (numIters + 1)) +
            0.144232 * cos(2.0 * ((double)i) * vtkMath::Pi() / (numIters + 1)) +
            0.012604 * cos(3.0 * ((double)i) * vtkMath::Pi() / (numIters + 1));
        }
        break;
      case vtkWindowedSincPolyDataFilter::BLACKMAN:
        for (i = 0; i <= numIters; i++)
        {
          w[i] = 0.42 + 0.5 * cos(((double)i) * vtkMath::Pi() / (numIters + 1)) +
            0.08 * cos(2.0 * ((double)i) * vtkMath::Pi() / (numIters + 1));
        }
        break;
      case vtkWindowedSincPolyDataFilter::HANNING:
        for (i = 0; i <= numIters; i++)
        {
          w[i] = 0.5 + 0.5 * cos(((double)i) * vtkMath::Pi() / (double)(numIters + 1));
        }
        break;
      case vtkWindowedSincPolyDataFilter::HAMMING:
        for (i = 0; i <= numIters; i++)
        {
          w[i] = 0.54 + 0.46 * cos(((double)i) * vtkMath::Pi() / (double)(numIters + 1));
        }
        break;
    }

    // Calculate the optimal sigma (offset or fudge factor for the filter).
    // This is a Newton-Raphson search.
    double f_kpb = 0.0, fprime_kpb;
    int done = 0;
    sigma = 0.0;

    const double errorTolerance = 1e-3;

    // Although this loop can run up to 500 times, in practice 20-40 iterations
    // is typical.
    for (j = 0; !done && (j < 500); j++)
    {
      // Chebyshev coefficients
      c[0] = w[0] * (theta_pb + sigma) / vtkMath::Pi();
      for (i = 1; i <= numIters; i++)
      {
        c[i] = 2.0 * w[i] * sin(((double)i) * (theta_pb + sigma)) / (((double)i) * vtkMath::Pi());
      }

      // calculate the Chebyshev coefficients for the derivative of the filter
      cprime[numIters] = 0.0;
      cprime[numIters - 1] = 0.0;
      if (numIters > 1)
      {
        cprime[numIters - 2] = 2.0 * (numIters - 1) * c[numIters - 1];
      }
      for (i = numIters - 3; i >= 0; i--)
      {
        cprime[i] = cprime[i + 2] + 2.0 * (i + 1) * c[i + 1];
      }
      // Evaluate the filter and its derivative at k_pb (note the discrepancy
      // of calculating the c's based on theta_pb + sigma and evaluating the
      // filter at k_pb (which is equivalent to theta_pb)
      f_kpb = 0.0;
      fprime_kpb = 0.0;
      f_kpb += c[0];
      fprime_kpb += cprime[0];
      for (i = 1; i <= numIters; i++)
      {
        if (i == 1)
        {
          f_kpb += c[i] * (1.0 - 0.5 * k_pb);
          fprime_kpb += cprime[i] * (1.0 - 0.5 * k_pb);
        }
        else
        {
          f_kpb += c[i] * cos(((double)i) * acos(1.0 - 0.5 * k_pb));
          fprime_kpb += cprime[i] * cos(((double)i) * acos(1.0 - 0.5 * k_pb));
        }
      }
      // if f_kpb is not close enough to 1.0, then adjust sigma
      if (numIters > 1)
      {
        if (fabs(f_kpb - 1.0) >= errorTolerance)
        {
          sigma -= (f_kpb - 1.0) / fprime_kpb; // Newton-Rhapson (want f=1)
        }
        else
        {
          done = 1;
        }
      }
      else
      {
        // Order of Chebyshev is 1. Can't use Newton-Raphson to find an
        // optimal sigma. Object will most likely shrink.
        done = 1;
        sigma = 0.0;
      }
    }
    if (fabs(f_kpb - 1.0) >= errorTolerance)
    {
      cout << "An optimal offset for the smoothing filter could not be found.\n";
    }
  }
}; // CoefficientsWorker

// Threaded point smoothing (initial iteration to set things up)
struct InitSmoothingWorker
{
  template <typename DataT, typename TIds>
  void operator()(DataT* vtkNotUsed(pts), vtkIdType numPts, vtkDataArray* da[4],
    PointConnectivity<TIds>* ptConn, double* c, int ptSelect[4],
    vtkWindowedSincPolyDataFilter* filter)
  {
    vtkSMPTools::For(0, numPts,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        EDGE_COUNT_TYPE numEdges;
        double deltaX[3];
        auto tuples0 = vtk::DataArrayTupleRange<3>(vtkArrayDownCast<DataT>(da[ptSelect[0]]));
        auto tuples1 = vtk::DataArrayTupleRange<3>(vtkArrayDownCast<DataT>(da[ptSelect[1]]));
        auto tuples3 = vtk::DataArrayTupleRange<3>(vtkArrayDownCast<DataT>(da[ptSelect[3]]));
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

        for (; ptId < endPtId; ++ptId)
        {
          if (ptId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              filter->CheckAbort();
            }
            if (filter->GetAbortOutput())
            {
              break;
            }
          }
          // Grab the edges
          TIds* edges = ptConn->GetEdges(ptId);
          numEdges = ptConn->GetEdgeCount(ptId);

          // calculate the negative of the laplacian
          auto x = tuples0[ptId];
          deltaX[0] = deltaX[1] = deltaX[2] = 0.0;
          for (auto j = 0; j < numEdges; j++)
          {
            auto y = tuples0[edges[j]];
            for (auto k = 0; k < 3; k++)
            {
              deltaX[k] += (x[k] - y[k]) / static_cast<double>(numEdges);
            }
          } // for all connected points

          for (auto k = 0; k < 3; k++)
          {
            deltaX[k] = x[k] - 0.5 * deltaX[k];
          }
          auto dX = tuples1[ptId];
          dX[0] = deltaX[0];
          dX[1] = deltaX[1];
          dX[2] = deltaX[2];

          for (auto k = 0; k < 3; k++)
          {
            deltaX[k] = c[0] * x[k] + c[1] * deltaX[k];
          }

          auto XN = tuples3[ptId];
          XN[0] = deltaX[0];
          XN[1] = deltaX[1];
          XN[2] = deltaX[2];
        } // for all points
      }); // end lambda
  }
}; // InitSmoothingWorker

// Threaded point smoothing (latter iterations)
struct SmoothingWorker
{
  template <typename DataT, typename TIds>
  void operator()(DataT* vtkNotUsed(pts), vtkIdType numPts, vtkDataArray* da[4],
    PointConnectivity<TIds>* ptConn, int iterNum, double* c, int ptSelect[4],
    vtkWindowedSincPolyDataFilter* filter)
  {
    vtkSMPTools::For(0, numPts,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        EDGE_COUNT_TYPE numEdges;
        double deltaX[3], xNew[3];
        auto tuples0 = vtk::DataArrayTupleRange<3>(vtkArrayDownCast<DataT>(da[ptSelect[0]]));
        auto tuples1 = vtk::DataArrayTupleRange<3>(vtkArrayDownCast<DataT>(da[ptSelect[1]]));
        auto tuples2 = vtk::DataArrayTupleRange<3>(vtkArrayDownCast<DataT>(da[ptSelect[2]]));
        auto tuples3 = vtk::DataArrayTupleRange<3>(vtkArrayDownCast<DataT>(da[ptSelect[3]]));
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

        for (; ptId < endPtId; ++ptId)
        {
          if (ptId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              filter->CheckAbort();
            }
            if (filter->GetAbortOutput())
            {
              break;
            }
          }
          // Grab the edges
          TIds* edges = ptConn->GetEdges(ptId);
          numEdges = ptConn->GetEdgeCount(ptId);

          // The point is allowed to move
          auto p_x0 = tuples0[ptId];
          auto p_x1 = tuples1[ptId];

          // Calculate the negative laplacian of x1
          deltaX[0] = deltaX[1] = deltaX[2] = 0.0;
          for (auto j = 0; j < numEdges; j++)
          {
            auto y = tuples1[edges[j]];
            for (auto k = 0; k < 3; k++)
            {
              deltaX[k] += (p_x1[k] - y[k]) / static_cast<double>(numEdges);
            }
          } // for all connected points

          // Taubin:  x2 = (x1 - x0) + (x1 - x2)
          for (auto k = 0; k < 3; k++)
          {
            deltaX[k] = p_x1[k] - p_x0[k] + p_x1[k] - deltaX[k];
          }
          auto dX = tuples2[ptId];
          dX[0] = deltaX[0];
          dX[1] = deltaX[1];
          dX[2] = deltaX[2];

          // smooth the vertex (x3 = x3 + cj x2)
          auto p_x3 = tuples3[ptId];
          for (auto k = 0; k < 3; k++)
          {
            xNew[k] = p_x3[k] + c[iterNum] * deltaX[k];
          }

          auto XN = tuples3[ptId];
          XN[0] = xNew[0];
          XN[1] = xNew[1];
          XN[2] = xNew[2];
        } // for all points
      }); // end lambda
  }
}; // SmoothingWorker

// Driver function to perform windowed sinc smoothing
template <typename TIds>
vtkSmartPointer<vtkPoints> SmoothMesh(
  PointConnectivity<TIds>* ptConn, vtkPoints* pts, vtkWindowedSincPolyDataFilter* filter)
{
  vtkIdType numPts = ptConn->NumPts;
  int numIters = ptConn->NumberOfIterations;

  using vtkArrayDispatch::Reals;
  using SmoothingDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;

  // COMPUTE SMOOTHING COEFFICIENTS============================================

  // Allocate coefficient array
  std::vector<double> c(numIters + 1);

  // Compute the smoothing coefficients
  CoefficientsWorker cWorker;
  cWorker(ptConn, numIters, c.data());

  // BEGIN SMOOTHING PASSES==============================================

  // Need 4 point arrays for smoothing. The point arrays are all of the
  // same type, and of the same value type. Dispatching requires the
  // underlying data arrays.
  vtkSmartPointer<vtkPoints> newPts[4];
  vtkDataArray* newDA[4];
  int ptSelect[4] = { 0, 1, 2, 3 };

  newPts[0] = pts;
  newDA[0] = pts->GetData();
  newPts[1].TakeReference(vtkPoints::New());
  newPts[1]->SetDataType(pts->GetDataType());
  newPts[1]->SetNumberOfPoints(numPts);
  newDA[1] = newPts[1]->GetData();
  newPts[2].TakeReference(vtkPoints::New());
  newPts[2]->SetDataType(pts->GetDataType());
  newPts[2]->SetNumberOfPoints(numPts);
  newDA[2] = newPts[2]->GetData();
  newPts[3].TakeReference(vtkPoints::New());
  newPts[3]->SetDataType(pts->GetDataType());
  newPts[3]->SetNumberOfPoints(numPts);
  newDA[3] = newPts[3]->GetData();

  // Prepare for smoothing dispatches
  InitSmoothingWorker isWorker;
  SmoothingWorker sWorker;

  // Threaded execute smoothing initialization pass
  if (!SmoothingDispatch::Execute(
        newDA[0], isWorker, numPts, newDA, ptConn, c.data(), ptSelect, filter))
  { // Fallback to slowpath for other point types
    isWorker(newDA[0], numPts, newDA, ptConn, c.data(), ptSelect, filter);
  }

  // for the rest of the iterations
  for (auto iterNum = 2; iterNum <= numIters; iterNum++)
  {
    // Threaded execute smoothing pass
    if (!SmoothingDispatch::Execute(
          newDA[0], sWorker, numPts, newDA, ptConn, iterNum, c.data(), ptSelect, filter))
    { // Fallback to slowpath for other point types
      sWorker(newDA[0], numPts, newDA, ptConn, iterNum, c.data(), ptSelect, filter);
    }

    // Update the point arrays. ptSelect[3] is always three. All other pointers
    // shift by one and wrap.
    ptSelect[0] = (1 + ptSelect[0]) % 3;
    ptSelect[1] = (1 + ptSelect[1]) % 3;
    ptSelect[2] = (1 + ptSelect[2]) % 3;
  } // for all iterations or until convergence

  // Return the appropriate points
  return newPts[ptSelect[3]];
} // SmoothMesh

// If points were initially normalized, inverse transform them
// into original coordinate system.
struct UnnormalizePointsWorker
{
  template <typename DataT>
  void operator()(DataT* pts, vtkIdType numPts, double length, double center[3],
    vtkWindowedSincPolyDataFilter* filter)
  {
    vtkSMPTools::For(0, numPts,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        auto inTuples = vtk::DataArrayTupleRange<3>(pts, ptId, endPtId);
        double x[3];
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

        for (auto tuple : inTuples)
        {
          if (ptId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              filter->CheckAbort();
            }
            if (filter->GetAbortOutput())
            {
              break;
            }
          }
          ptId++;
          x[0] = (static_cast<double>(tuple[0]) * length) + center[0];
          x[1] = (static_cast<double>(tuple[1]) * length) + center[1];
          x[2] = (static_cast<double>(tuple[2]) * length) + center[2];

          tuple[0] = x[0];
          tuple[1] = x[1];
          tuple[2] = x[2];
        } // for all points
      }); // end lambda
  }
}; // UnnormalizePointsWorker

// If points have been normalized, restore them to normal space
void UnnormalizePoints(
  vtkPoints* inPts, double& length, double center[3], vtkWindowedSincPolyDataFilter* filter)
{
  vtkIdType numPts = inPts->GetNumberOfPoints();

  using vtkArrayDispatch::Reals;
  using UnnormalizePointsDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
  UnnormalizePointsWorker unnWorker;
  if (!UnnormalizePointsDispatch::Execute(
        inPts->GetData(), unnWorker, numPts, length, center, filter))
  { // Fallback to slowpath for other point types
    unnWorker(inPts->GetData(), numPts, length, center, filter);
  }
} // UnnormalizePoints

// If requested, generate scalars indicating error magnitude
struct ErrorScalarsWorker
{
  template <typename DataT1, typename DataT2>
  void operator()(DataT1* inPts, DataT2* outPts, vtkIdType numPts, vtkFloatArray* es,
    vtkWindowedSincPolyDataFilter* filter)
  {
    vtkSMPTools::For(0, numPts,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        const auto inTuples = vtk::DataArrayTupleRange<3>(inPts);
        const auto outTuples = vtk::DataArrayTupleRange<3>(outPts);
        float* esPtr = es->GetPointer(0) + ptId;
        double x[3];
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

        for (; ptId < endPtId; ++ptId)
        {
          if (ptId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              filter->CheckAbort();
            }
            if (filter->GetAbortOutput())
            {
              break;
            }
          }
          const auto inTuple = inTuples[ptId];
          const auto outTuple = outTuples[ptId];
          x[0] = outTuple[0] - inTuple[0];
          x[1] = outTuple[1] - inTuple[1];
          x[2] = outTuple[2] - inTuple[2];
          *esPtr++ = sqrt(vtkMath::Norm(x));
        } // for all points
      }); // end lambda
  }
}; // ErrorScalarsWorker

// Dispatch computation of error scalars. Caller takes the
// reference to the created error scalars.
vtkSmartPointer<vtkFloatArray> ProduceErrorScalars(
  vtkPoints* inPts, vtkPoints* outPts, vtkWindowedSincPolyDataFilter* filter)
{
  vtkIdType numPts = inPts->GetNumberOfPoints();
  vtkNew<vtkFloatArray> errorScalars;
  errorScalars->SetNumberOfComponents(1);
  errorScalars->SetNumberOfTuples(numPts);

  using vtkArrayDispatch::Reals;
  using ErrorScalarsDispatch = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::Reals>;
  ErrorScalarsWorker esWorker;
  if (!ErrorScalarsDispatch::Execute(
        inPts->GetData(), outPts->GetData(), esWorker, numPts, errorScalars, filter))
  { // Fallback to slowpath for other point types
    esWorker(inPts->GetData(), outPts->GetData(), numPts, errorScalars, filter);
  }

  return errorScalars;
} // ProduceErrorScalars

// If requested, produce vectors indicating vector difference in position
struct ErrorVectorsWorker
{
  template <typename DataT1, typename DataT2>
  void operator()(DataT1* inPts, DataT2* outPts, vtkIdType numPts, vtkFloatArray* ev,
    vtkWindowedSincPolyDataFilter* filter)
  {
    vtkSMPTools::For(0, numPts,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        const auto inTuples = vtk::DataArrayTupleRange<3>(inPts);
        const auto outTuples = vtk::DataArrayTupleRange<3>(outPts);
        float* evPtr = ev->GetPointer(0) + 3 * ptId;
        bool isFirst = vtkSMPTools::GetSingleThread();
        vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

        for (; ptId < endPtId; ++ptId)
        {
          if (ptId % checkAbortInterval == 0)
          {
            if (isFirst)
            {
              filter->CheckAbort();
            }
            if (filter->GetAbortOutput())
            {
              break;
            }
          }
          const auto inTuple = inTuples[ptId];
          const auto outTuple = outTuples[ptId];
          *evPtr++ = outTuple[0] - inTuple[0];
          *evPtr++ = outTuple[1] - inTuple[1];
          *evPtr++ = outTuple[2] - inTuple[2];
        } // for all points
      }); // end lambda
  }
}; // ErrorVectorsWorker

// Dispatch computation of error vectors. Caller takes the
// reference to the created error vectors.
vtkSmartPointer<vtkFloatArray> ProduceErrorVectors(
  vtkPoints* inPts, vtkPoints* outPts, vtkWindowedSincPolyDataFilter* filter)
{
  vtkIdType numPts = inPts->GetNumberOfPoints();
  vtkNew<vtkFloatArray> errorVectors;
  errorVectors->SetNumberOfComponents(3);
  errorVectors->SetNumberOfTuples(numPts);

  using vtkArrayDispatch::Reals;
  using ErrorVectorsDispatch = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::Reals>;
  ErrorVectorsWorker evWorker;
  if (!ErrorVectorsDispatch::Execute(
        inPts->GetData(), outPts->GetData(), evWorker, numPts, errorVectors, filter))
  { // Fallback to slowpath for other point types
    evWorker(inPts->GetData(), outPts->GetData(), numPts, errorVectors, filter);
  }

  return errorVectors;
} // ProduceErrorVectors

} // anonymous namespace

//------------------------------------------------------------------------------
// Construct object with number of iterations 20; passband 0.1; feature edge
// smoothing turned off; feature angle 45 degrees; edge angle 15 degrees; and
// boundary smoothing turned on. Error scalars and vectors are not generated
// (by default).
vtkWindowedSincPolyDataFilter::vtkWindowedSincPolyDataFilter()
{
  this->NumberOfIterations = 20;
  this->PassBand = 0.1;

  this->NormalizeCoordinates = false;

  this->WindowFunction = NUTTALL;

  this->FeatureAngle = 45.0;
  this->EdgeAngle = 15.0;

  this->FeatureEdgeSmoothing = false;
  this->BoundarySmoothing = true;
  this->NonManifoldSmoothing = false;
  this->WeightNonManifoldEdges = true;

  this->GenerateErrorScalars = false;
  this->GenerateErrorVectors = false;
}

//------------------------------------------------------------------------------
int vtkWindowedSincPolyDataFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check input
  //
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
  {
    vtkWarningMacro(<< "No data to smooth!");
    return 1;
  }

  vtkDebugMacro(<< "Smoothing " << numPts << " vertices, " << numCells << " cells with:\n"
                << "\tIterations= " << this->NumberOfIterations << "\n"
                << "\tPassBand= " << this->PassBand << "\n"
                << "\tEdge Angle= " << this->EdgeAngle << "\n"
                << "\tBoundary Smoothing " << (this->BoundarySmoothing ? "On\n" : "Off\n")
                << "\tFeature Edge Smoothing " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n")
                << "\tNonmanifold Smoothing " << (this->NonManifoldSmoothing ? "On\n" : "Off\n")
                << "\tWeight NonManifold Edges "
                << (this->WeightNonManifoldEdges ? "On\n" : "Off\n") << "\tError Scalars "
                << (this->GenerateErrorScalars ? "On\n" : "Off\n") << "\tError Vectors "
                << (this->GenerateErrorVectors ? "On\n" : "Off\n"));

  // We will replace the smoothed points later with newPts
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  if (this->NumberOfIterations <= 0) // don't do anything!
  {
    vtkWarningMacro(<< "Number of iterations == 0: passing data through unchanged");
    return 1;
  }

  // Build the smoothing connectivity. This is a function of the id type
  // (i.e., size of the point and cell ids). Smaller id types reduce memory
  // and improve performance. It is done in two steps: first the topological
  // edge connectivity is constructed (i.e., incident edges to each point are
  // identified), then the local topology around each point is analyzed to
  // create a local smoothing stencil.
  bool largeIds = numPts > VTK_INT_MAX || numCells > VTK_INT_MAX;
  PointConnectivityBase* ptConn;
  if (largeIds)
  {
    ptConn = BuildConnectivity<vtkIdType>(input, this);
    AnalyzePointTopology<vtkIdType>(ptConn, this);
  }
  else
  {
    ptConn = BuildConnectivity<int>(input, this);
    AnalyzePointTopology<int>(ptConn, this);
  }

  vtkDebugMacro(<< "Found\n\t" << ptConn->NumSimple << " simple vertices\n\t" << ptConn->NumEdges
                << " edge vertices\n\t" << ptConn->NumFixed << " fixed vertices\n\t");

  // Copy the input points to the output; normalize the output points if
  // requested.
  double length = 1.0, center[3];
  vtkSmartPointer<vtkPoints> newPts =
    InitializePoints(this->NormalizeCoordinates, input, length, center, this);

  // Now smooth the mesh. Basically what is happening is that the input point
  // positions are adjusted to remove high-frequency information / noise.
  vtkSmartPointer<vtkPoints> outPts;
  if (largeIds)
  {
    outPts =
      SmoothMesh<vtkIdType>(static_cast<PointConnectivity<vtkIdType>*>(ptConn), newPts, this);
  }
  else
  {
    outPts = SmoothMesh<int>(static_cast<PointConnectivity<int>*>(ptConn), newPts, this);
  }

  // If the points were normalized, reverse the normalization process.
  if (this->NormalizeCoordinates)
  {
    UnnormalizePoints(outPts, length, center, this);
  }

  // If error scalars are requested, create them.
  if (this->GenerateErrorScalars)
  {
    vtkSmartPointer<vtkFloatArray> errorScalars =
      ProduceErrorScalars(input->GetPoints(), outPts, this);
    int idx = output->GetPointData()->AddArray(errorScalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  }

  // If error vector are requested, create them.
  if (this->GenerateErrorVectors)
  {
    vtkSmartPointer<vtkFloatArray> errorVectors =
      ProduceErrorVectors(input->GetPoints(), outPts, this);
    output->GetPointData()->AddArray(errorVectors);
  }

  // Set the new points
  output->SetPoints(outPts);

  // Clean up
  delete ptConn;

  return 1;
}

//------------------------------------------------------------------------------
void vtkWindowedSincPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Iterations: " << this->NumberOfIterations << "\n";
  os << indent << "Passband: " << this->PassBand << "\n";
  os << indent << "Normalize Coordinates: " << (this->NormalizeCoordinates ? "On\n" : "Off\n");
  os << indent << "Feature Edge Smoothing: " << (this->FeatureEdgeSmoothing ? "On\n" : "Off\n");
  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Edge Angle: " << this->EdgeAngle << "\n";
  os << indent << "Boundary Smoothing: " << (this->BoundarySmoothing ? "On\n" : "Off\n");
  os << indent << "NonManifold Smoothing: " << (this->NonManifoldSmoothing ? "On\n" : "Off\n");
  os << indent << "Weight NonManifold Edges: " << (this->WeightNonManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Generate Error Scalars: " << (this->GenerateErrorScalars ? "On\n" : "Off\n");
  os << indent << "Generate Error Vectors: " << (this->GenerateErrorVectors ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
