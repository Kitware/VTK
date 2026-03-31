// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPolyhedron.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeTable.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkPolyhedronContour.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"

#include <cmath>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define VTK_DEFAULT_PLANARITY_TOLERANCE 0.1

VTK_ABI_NAMESPACE_BEGIN

namespace
{

/// Called in IsConvex() when a face is flipped so its normal points outward.
///
/// This modifies \a unevenCoedges for all the edges of \a face so that successive
/// attempts to flip those edges will pass; if \a unevenCoedges is not updated, flips
/// will not be allowed and double-flips may be.
void updateFlippedEdges(
  std::map<vtkIdType, int>& unevenCoedges, int face, vtkCellArray* faces, vtkEdgeTable* edgeTable)
{
  vtkNew<vtkIdList> face_tmp;
  const vtkIdType* conn;
  vtkIdType numPts;
  faces->GetCellAtId(face, numPts, conn, face_tmp);
  if (conn)
  {
    vtkIdType pp = conn[numPts - 1];
    vtkIdType qq;
    for (int ii = 0; ii < numPts; ++ii, pp = qq)
    {
      qq = conn[ii];
      // The direction has been flipped, so \a dir is opposite from elsewhere:
      // NOLINTNEXTLINE(readability-avoid-nested-conditional-operator)
      int dir = (pp < qq ? -1 : (pp == qq ? 0 : +1));
      auto edgeId = edgeTable->IsEdge(pp, qq);
      unevenCoedges[edgeId] += dir;
    }
  }
}

//------------------------------------------------------------------------------
} // anonymous namespace

vtkStandardNewMacro(vtkPolyhedron);

// Special typedef
typedef std::vector<vtkIdType> vtkIdVectorType;

// an edge consists of two id's and their order
// is *not* important. To that end special hash and
// equals functions have been made
// typedef std::pair<vtkIdType, vtkIdType> Edge;

struct Edge : public std::pair<vtkIdType, vtkIdType>
{
public:
  Edge() = default;
  Edge(vtkIdType a, vtkIdType b)
    : std::pair<vtkIdType, vtkIdType>(a, b)
  {
  }
  Edge(vtkCell* edge)
    : std::pair<vtkIdType, vtkIdType>(edge->GetPointId(0), edge->GetPointId(1))
  {
  }
  friend ostream& operator<<(ostream& stream, const Edge& e)
  {
    stream << e.first << " - " << e.second;
    return stream;
  }
};

struct hash_fn
{
  size_t operator()(Edge const& p) const
  {
    size_t i = (size_t)p.first;
    size_t j = (size_t)p.second;

    // first make order-independent, i.e. hash(i,j) == hash(j,i)
    if (i < j)
    {
      size_t tmp = i;
      i = j;
      j = tmp;
    }

    // then XOR both together , multiplied by two primes to try to prevent collisions
    return (17 * i) ^ (31 * j);
  }
};

struct equal_fn
{
  bool operator()(Edge const& e1, Edge const& e2) const
  {
    return (e1.first == e2.first && e1.second == e2.second) ||
      (e1.second == e2.first && e1.first == e2.second);
  }
};

struct point
{
public:
  point()
    : x(0)
    , y(0)
    , z(0)
  {
    // empty
  }

  point(const double _x, const double _y, const double _z)
    : x(_x)
    , y(_y)
    , z(_z)
  {
    // empty
  }

  double x;
  double y;
  double z;
};

// these typedefs are for the contouoring code. There the order of two edges does not matter
// so we use the specially crafted equals and hash functions defined above.

typedef std::vector<Edge> EdgeVector;

typedef std::vector<EdgeVector> FaceEdgesVector;
typedef std::unordered_map<Edge, std::set<vtkIdType>, hash_fn, equal_fn> EdgeFaceSetMap;

typedef std::unordered_map<vtkIdType, point> PointIndexLocationMap;

typedef std::unordered_multimap<vtkIdType, Edge> PointIndexEdgeMultiMap;
typedef std::unordered_map<Edge, vtkIdType, hash_fn, equal_fn> EdgePointIndexMap;

typedef std::unordered_set<Edge, hash_fn, equal_fn> EdgeSet;

typedef vtkIdVectorType Face;
typedef std::vector<Face> FaceVector;

//------------------------------------------------------------------------------
// Construct the hexahedron with eight points.
vtkPolyhedron::vtkPolyhedron()
{
  this->Edges->SetNumberOfComponents(2);
  this->EdgeFaces->SetNumberOfComponents(2);
}

//------------------------------------------------------------------------------
vtkPolyhedron::~vtkPolyhedron() = default;

//------------------------------------------------------------------------------
void vtkPolyhedron::ComputeBounds()
{
  if (this->BoundsComputed)
  {
    return;
  }

  this->Superclass::GetBounds(); // stored in this->Bounds
  this->BoundsComputed = 1;
}

//------------------------------------------------------------------------------
void vtkPolyhedron::ConstructPolyData()
{
  if (this->PolyDataConstructed)
  {
    return;
  }

  // Here's a trick, we're going to use the Faces array as the connectivity
  // array. Note that the Faces have an added nfaces value at the beginning
  // of the array. Other than that,it's a vtkCellArray. So we play games
  // with the pointers.
  this->GenerateFaces();

  if (this->Faces->GetNumberOfCells() == 0)
  {
    return;
  }

  // Standard setup
  this->PolyData->Initialize();
  this->PolyData->SetPoints(this->Points);
  this->PolyData->SetPolys(this->Faces);

  this->PolyDataConstructed = 1;
}

vtkPolyData* vtkPolyhedron::GetPolyData()
{
  if (!this->PolyDataConstructed)
  {
    this->ConstructPolyData();
  }

  return this->PolyData;
}
//------------------------------------------------------------------------------
void vtkPolyhedron::ConstructLocator()
{
  if (this->LocatorConstructed)
  {
    return;
  }

  this->ConstructPolyData();

  // With the polydata set up, we can assign it to the locator
  this->CellLocator->Initialize();
  this->CellLocator->SetDataSet(this->PolyData);
  this->CellLocator->BuildLocator();

  this->LocatorConstructed = 1;
}

//------------------------------------------------------------------------------
void vtkPolyhedron::ComputeParametricCoordinate(const double x[3], double pc[3])
{
  this->ComputeBounds();
  double* bounds = this->Bounds;

  pc[0] = (x[0] - bounds[0]) / (bounds[1] - bounds[0]);
  pc[1] = (x[1] - bounds[2]) / (bounds[3] - bounds[2]);
  pc[2] = (x[2] - bounds[4]) / (bounds[5] - bounds[4]);
}

//------------------------------------------------------------------------------
void vtkPolyhedron::ComputePositionFromParametricCoordinate(const double pc[3], double x[3])
{
  this->ComputeBounds();
  double* bounds = this->Bounds;
  x[0] = (1 - pc[0]) * bounds[0] + pc[0] * bounds[1];
  x[1] = (1 - pc[1]) * bounds[2] + pc[1] * bounds[3];
  x[2] = (1 - pc[2]) * bounds[4] + pc[2] * bounds[5];
}

//------------------------------------------------------------------------------
// Should be called by GetCell() prior to any other method invocation and after the
// points, point ids, and faces have been loaded.
void vtkPolyhedron::Initialize()
{
  // Clear out any remaining memory.
  this->PointIdMap.clear();

  // Clear out any remaining memory.
  this->PointToIncidentFaces.clear();

  // We need to create a reverse map from the point ids to their canonical cell
  // ids. This is a fancy way of saying that we have to be able to rapidly go
  // from a PointId[i] to the location i in the cell.
  vtkIdType i, id, numPointIds = this->PointIds->GetNumberOfIds();
  for (i = 0; i < numPointIds; ++i)
  {
    id = this->PointIds->GetId(i);
    this->PointIdMap[id] = i;
  }

  // Edges have to be reset
  this->EdgesGenerated = 0;
  this->EdgeTable->Reset();
  this->Edges->Reset();
  this->EdgeFaces->Reset();

  // Polys have to be reset
  this->Faces->Reset();

  // Faces may need renumbering later. This means converting the face ids from
  // global ids to local, canonical ids.
  this->FacesGenerated = 0;

  // No bounds have been computed as of yet.
  this->BoundsComputed = 0;

  // No supplemental geometric stuff created
  this->PolyDataConstructed = 0;
  this->LocatorConstructed = 0;
}

//------------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfEdges()
{
  // Make sure edges have been generated.
  if (!this->EdgesGenerated)
  {
    this->GenerateEdges();
  }

  return static_cast<int>(this->Edges->GetNumberOfTuples());
}

//------------------------------------------------------------------------------
// This method requires that GenerateEdges() is invoked beforehand.
vtkCell* vtkPolyhedron::GetEdge(int edgeId)
{
  // Make sure edges have been generated.
  if (!this->EdgesGenerated)
  {
    this->GenerateEdges();
  }

  // Make sure requested edge is within range
  vtkIdType numEdges = this->Edges->GetNumberOfTuples();

  if (edgeId < 0 || edgeId >= numEdges)
  {
    return nullptr;
  }

  // Return the requested edge
  vtkIdType edge[2];
  this->Edges->GetTypedTuple(edgeId, edge);

  // Recall that edge tuples are stored in canonical numbering
  for (int i = 0; i < 2; i++)
  {
    this->Line->PointIds->SetId(i, this->PointIds->GetId(edge[i]));
    this->Line->Points->SetPoint(i, this->Points->GetPoint(edge[i]));
  }

  return this->Line;
}

//------------------------------------------------------------------------------
int vtkPolyhedron::GenerateEdges()
{
  if (this->EdgesGenerated)
  {
    return this->Edges->GetNumberOfTuples();
  }

  // check the number of faces and return if there aren't any
  if (this->GlobalFaces->GetNumberOfCells() <= 0)
  {
    return 0;
  }

  vtkNew<vtkIdList> tmpface;
  vtkIdType nfaces = 0;
  const vtkIdType* face;
  vtkIdType fid, i, edge[2], npts, edgeFaces[2], edgeId;
  edgeFaces[1] = -1;

  // Loop over all faces, inserting edges into the table
  this->EdgeTable->InitEdgeInsertion(this->Points->GetNumberOfPoints(), 1);
  nfaces = this->GlobalFaces->GetNumberOfCells();
  for (fid = 0; fid < nfaces; ++fid)
  {
    this->GlobalFaces->GetCellAtId(fid, npts, face, tmpface);
    for (i = 0; i < npts; ++i)
    {
      edge[0] = this->PointIdMap[face[i]];
      edge[1] = this->PointIdMap[((i + 1) != npts ? face[i + 1] : face[0])];
      edgeFaces[0] = fid;
      if ((edgeId = this->EdgeTable->IsEdge(edge[0], edge[1])) == (-1))
      {
        edgeId = this->EdgeTable->InsertEdge(edge[0], edge[1]);
        this->Edges->InsertNextTypedTuple(edge);
        this->EdgeFaces->InsertTypedTuple(edgeId, edgeFaces);
      }
      else
      {
        this->EdgeFaces->SetComponent(edgeId, 1, fid);
      }
    }
  } // for all faces

  // Okay all done
  this->EdgesGenerated = 1;
  return this->Edges->GetNumberOfTuples();
}

//------------------------------------------------------------------------------
int vtkPolyhedron::GenerateEdges(std::map<vtkIdType, int>& unevenCoedges)
{
  // check the number of faces and return if there aren't any
  if (this->GlobalFaces->GetNumberOfCells() <= 0)
  {
    return 0;
  }

  // This method will always regenerate edges so that \a flips can be populated.
  // So instead of exiting early here, we reset the edge list and table.
  if (this->EdgesGenerated)
  {
    this->Edges->Initialize();
    this->EdgeTable->Initialize();
  }

  unevenCoedges.clear();
  vtkNew<vtkIdList> tmpface;
  vtkIdType nfaces = 0;
  const vtkIdType* face;
  vtkIdType fid, i, edge[2], npts, edgeFaces[2], edgeId;
  edgeFaces[1] = -1;

  // Loop over all faces, inserting edges into the table
  this->EdgeTable->InitEdgeInsertion(this->Points->GetNumberOfPoints(), 1);
  nfaces = this->GlobalFaces->GetNumberOfCells();
  for (fid = 0; fid < nfaces; ++fid)
  {
    this->GlobalFaces->GetCellAtId(fid, npts, face, tmpface);
    for (i = 0; i < npts; ++i)
    {
      edge[0] = this->PointIdMap[face[i]];
      edge[1] = this->PointIdMap[((i + 1) != npts ? face[i + 1] : face[0])];
      // NOLINTNEXTLINE(readability-avoid-nested-conditional-operator)
      int dir = (edge[0] < edge[1] ? +1 : (edge[0] == edge[1] ? 0 : -1));
      edgeFaces[0] = fid;
      if ((edgeId = this->EdgeTable->IsEdge(edge[0], edge[1])) == (-1))
      {
        edgeId = this->EdgeTable->InsertEdge(edge[0], edge[1]);
        this->Edges->InsertNextTypedTuple(edge);
        this->EdgeFaces->InsertTypedTuple(edgeId, edgeFaces);
        unevenCoedges[edgeId] = dir;
      }
      else
      {
        this->EdgeFaces->SetComponent(edgeId, 1, fid);
        auto it = unevenCoedges.find(edgeId);
        if (it == unevenCoedges.end())
        {
          unevenCoedges[edgeId] = dir;
        }
        else if (it->second + dir == 0)
        {
          unevenCoedges.erase(it);
        }
        else
        {
          it->second += dir;
        }
      }
    }
  } // for all faces

  // Okay all done
  this->EdgesGenerated = 1;
  return this->Edges->GetNumberOfTuples();
}

//------------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfFaces()
{
  // Make sure faces have been generated.
  if (!this->FacesGenerated)
  {
    this->GenerateFaces();
  }

  if (this->GlobalFaces->GetNumberOfCells() == 0)
  {
    return 0;
  }

  return static_cast<int>(this->GlobalFaces->GetNumberOfCells());
}

//------------------------------------------------------------------------------
void vtkPolyhedron::GenerateFaces()
{
  if (this->FacesGenerated)
  {
    return;
  }

  if (this->GlobalFaces->GetNumberOfCells() == 0)
  {
    return;
  }

  // Basically we just run through the faces and change the global ids to the
  // canonical ids using the PointIdMap.
  this->Faces->DeepCopy(this->GlobalFaces);
  vtkIdType numConn = this->Faces->GetNumberOfConnectivityIds();

  switch (this->Faces->GetStorageType())
  {
    case vtkCellArray::Int64:
    case vtkCellArray::FixedSizeInt64:
    {
      vtkTypeInt64* c = this->Faces->GetConnectivityAOSArray64()->GetPointer(0);
      for (vtkIdType id = 0; id < numConn; ++id)
      {
        c[id] = this->PointIdMap[c[id]];
      }
      break;
    }
    case vtkCellArray::Int32:
    case vtkCellArray::FixedSizeInt32:
    {
      vtkTypeInt32* c = this->Faces->GetConnectivityAOSArray32()->GetPointer(0);
      for (vtkIdType id = 0; id < numConn; ++id)
      {
        c[id] = this->PointIdMap[c[id]];
      }
      break;
    }
    case vtkCellArray::Generic:
    default:
    {
      auto c = vtk::DataArrayValueRange<1, vtkIdType>(this->Faces->GetConnectivityArray());
      for (vtkIdType id = 0; id < numConn; ++id)
      {
        c[id] = this->PointIdMap[c[id]];
      }
      break;
    }
  }
  // Okay we've done the deed
  this->FacesGenerated = 1;
}

//------------------------------------------------------------------------------
vtkCell* vtkPolyhedron::GetFace(int faceId)
{
  if (faceId < 0 || faceId >= this->GlobalFaces->GetNumberOfCells())
  {
    return nullptr;
  }

  this->GenerateFaces();

  // Okay load up the polygon
  vtkIdType i, p, numPts = 0;

  numPts = this->GlobalFaces->GetCellSize(faceId);
  this->GlobalFaces->GetCellAtId(faceId, this->Polygon->PointIds);
  this->Polygon->Points->SetNumberOfPoints(numPts);

  // grab faces in global id space
  for (i = 0; i < numPts; ++i)
  {
    vtkIdType pid = this->Polygon->PointIds->GetId(i);
    p = this->PointIdMap[pid];
    this->Polygon->Points->SetPoint(i, this->Points->GetPoint(p));
  }

  return this->Polygon;
}

// VTK_DEPRECATED_IN_9_6_0()
//------------------------------------------------------------------------------
// Specify the faces for this cell.
void vtkPolyhedron::SetFaces(vtkIdType* faces)
{
  // Set up face structure
  this->GlobalFaces->Reset();

  if (!faces)
  {
    return;
  }

  vtkIdType nfaces = faces[0];
  this->GlobalFaces->AllocateEstimate(nfaces, nfaces);
  vtkIdType* face = faces + 1;
  vtkIdType faceLoc = 1;

  for (vtkIdType fid = 0; fid < nfaces; ++fid)
  {
    this->GlobalFaces->InsertNextCell(face[0], &face[1]);
    faceLoc += face[0] + 1;
    face = faces + faceLoc;
  } // for all faces
}

//------------------------------------------------------------------------------
// Specify the faces for this cell from a vtkCellArray definition.
int vtkPolyhedron::SetCellFaces(vtkCellArray* faces)
{
  // Set up face structure
  this->GlobalFaces->Reset();

  if (!faces)
  {
    return 0;
  }
  if (faces->GetNumberOfCells() < 1)
  {
    return 0;
  }
  this->GlobalFaces->DeepCopy(faces);

  return 1;
}

// VTK_DEPRECATED_IN_9_6_0()
//------------------------------------------------------------------------------
// Return the list of faces for this cell.
vtkIdType* vtkPolyhedron::GetFaces()
{
  if (!this->GlobalFaces->GetNumberOfCells())
  {
    return nullptr;
  }

  vtkNew<vtkIdTypeArray> tmpFaces;
  this->GlobalFaces->ExportLegacyFormat(tmpFaces);

  this->LegacyGlobalFaces->Reset();
  this->LegacyGlobalFaces->InsertNextValue(this->GlobalFaces->GetNumberOfCells());
  this->LegacyGlobalFaces->InsertTuples(1, tmpFaces->GetNumberOfValues(), 0, tmpFaces);

  return this->LegacyGlobalFaces->GetPointer(0);
}

vtkCellArray* vtkPolyhedron::GetCellFaces()
{
  return this->GlobalFaces;
}

void vtkPolyhedron::GetCellFaces(vtkCellArray* faces)
{
  if (!faces)
  {
    vtkGenericWarningMacro(<< "Unexpected nullptr provided to GetCellFaces.");
    return;
  }
  if (!this->GlobalFaces->GetNumberOfCells())
  {
    faces->Reset();
    return;
  }

  faces->DeepCopy(this->GlobalFaces);
}

//------------------------------------------------------------------------------
int vtkPolyhedron::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& tMin, double xMin[3], double pc[3], int& subId)
{
  // It's easiest if this is done in canonical space
  this->GenerateFaces();

  // Loop over all the faces, intersecting them in turn.
  vtkIdType nfaces = this->Faces->GetNumberOfCells();
  vtkIdType npts, i, fid, numHits = 0;
  double t = VTK_FLOAT_MAX;
  double x[3];

  tMin = VTK_FLOAT_MAX;
  for (fid = 0; fid < nfaces; ++fid)
  {
    npts = this->Faces->GetCellSize(fid);
    vtkIdType hit = 0;
    switch (npts)
    {
      case 3: // triangle
        this->Faces->GetCellAtId(fid, this->Triangle->PointIds);
        for (i = 0; i < 3; i++)
        {
          this->Triangle->Points->SetPoint(
            i, this->Points->GetPoint(this->Triangle->PointIds->GetId(i)));
        }
        hit = this->Triangle->IntersectWithLine(p1, p2, tol, t, x, pc, subId);
        break;
      case 4: // quad
        this->Faces->GetCellAtId(fid, this->Quad->PointIds);
        for (i = 0; i < 4; i++)
        {
          this->Quad->Points->SetPoint(i, this->Points->GetPoint(this->Quad->PointIds->GetId(i)));
        }
        hit = this->Quad->IntersectWithLine(p1, p2, tol, t, x, pc, subId);
        break;
      default: // general polygon
        this->Faces->GetCellAtId(fid, this->Polygon->PointIds);
        this->Polygon->GetPoints()->SetNumberOfPoints(npts);
        for (i = 0; i < npts; i++)
        {
          this->Polygon->Points->SetPoint(
            i, this->Points->GetPoint(this->Polygon->PointIds->GetId(i)));
        }
        hit = this->Polygon->IntersectWithLine(p1, p2, tol, t, x, pc, subId);
        break;
    }

    // Update minimum hit
    if (hit)
    {
      numHits++;
      if (t < tMin)
      {
        tMin = t;
        xMin[0] = x[0];
        xMin[1] = x[1];
        xMin[2] = x[2];
      }
    }
  } // for all faces

  // Compute parametric coordinates
  this->ComputeParametricCoordinate(xMin, pc);

  return (numHits > 0);
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
vtkIdType vtkPolyhedron::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  vtkIdType npts;
  this->GlobalFaces->GetCellAtId(faceId, npts, pts);
  return npts;
}

// New IsInside using winding number / solid angle + tolerance-based near-surface early-out.
int vtkPolyhedron::IsInside(const double x[3], double tolerance)
{
  // Quick bounds check
  this->ComputeBounds();
  const double* bounds = this->Bounds;
  if (x[0] < bounds[0] || x[0] > bounds[1] || x[1] < bounds[2] || x[1] > bounds[3] ||
    x[2] < bounds[4] || x[2] > bounds[5])
  {
    return 0;
  }

  // Match legacy behavior: scale tolerance by cell length
  const double length = std::sqrt(this->Superclass::GetLength2());
  const double tol = tolerance * length;
  const double tol2 = tol * tol;

  // --- Primary method: solid angle (fast, deterministic, O(n)) ---
  // Sum the signed solid angle subtended by each face triangle as seen from x.
  // For a closed surface with consistent outward normals, |total| = 4*pi inside, 0 outside.
  constexpr double FourPi = 4.0 * vtkMath::Pi();
  constexpr double angleTol = 1e-3;

  double total = 0.0;

  // Direct access to the points array avoids per-point virtual dispatch.
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro("Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  // Use GlobalFaces directly with PointIdMap to convert global->local indices,
  // avoiding the GenerateFaces() side effect.
  const auto toLocal = [&](vtkIdType globalId) -> vtkIdType
  {
    const auto it = this->PointIdMap.find(globalId);
    return it != this->PointIdMap.end() ? it->second : -1;
  };

  // Reusable polygon + id list for faces with > 4 vertices.
  vtkNew<vtkPolygon> poly;
  vtkNew<vtkIdList> triPtIds;
  std::vector<vtkIdType> localIds;

  vtkIdType npts = 0;
  const vtkIdType* fpts = nullptr;

  for (this->GlobalFaces->InitTraversal(); this->GlobalFaces->GetNextCell(npts, fpts);)
  {
    if (npts < 3)
    {
      continue;
    }

    // --- Triangle: no triangulation needed ---
    if (npts == 3)
    {
      const double* a = pts + 3 * toLocal(fpts[0]);
      const double* b = pts + 3 * toLocal(fpts[1]);
      const double* c = pts + 3 * toLocal(fpts[2]);

      if (tol > 0.0 && vtkTriangle::DistanceToTriangle(x, a, b, c) <= tol2)
      {
        return 1;
      }
      total += vtkTriangle::SolidAngle(x, a, b, c);
    }
    // --- Quad: simple fan split (0-1-2, 0-2-3) ---
    else if (npts == 4)
    {
      const double* p0 = pts + 3 * toLocal(fpts[0]);
      const double* p1 = pts + 3 * toLocal(fpts[1]);
      const double* p2 = pts + 3 * toLocal(fpts[2]);
      const double* p3 = pts + 3 * toLocal(fpts[3]);

      if (tol > 0.0)
      {
        if (vtkTriangle::DistanceToTriangle(x, p0, p1, p2) <= tol2 ||
          vtkTriangle::DistanceToTriangle(x, p0, p2, p3) <= tol2)
        {
          return 1;
        }
      }
      total += vtkTriangle::SolidAngle(x, p0, p1, p2);
      total += vtkTriangle::SolidAngle(x, p0, p2, p3);
    }
    // --- General polygon: triangulate and sum ---
    else
    {
      localIds.resize(npts);
      for (vtkIdType i = 0; i < npts; ++i)
      {
        localIds[i] = toLocal(fpts[i]);
      }
      poly->PointIds->SetNumberOfIds(npts);
      poly->Points->SetNumberOfPoints(npts);
      for (vtkIdType i = 0; i < npts; ++i)
      {
        poly->PointIds->SetId(i, i); // local ids 0..npts-1
        poly->Points->SetPoint(i, pts + 3 * localIds[i]);
      }

      triPtIds->Reset();
      if (!poly->TriangulateLocalIds(0, triPtIds))
      {
        return 0; // can't triangulate: conservative
      }

      for (vtkIdType i = 0; i < triPtIds->GetNumberOfIds(); i += 3)
      {
        const double* a = pts + 3 * localIds[triPtIds->GetId(i)];
        const double* b = pts + 3 * localIds[triPtIds->GetId(i + 1)];
        const double* c = pts + 3 * localIds[triPtIds->GetId(i + 2)];

        if (tol > 0.0 && vtkTriangle::DistanceToTriangle(x, a, b, c) <= tol2)
        {
          return 1;
        }
        total += vtkTriangle::SolidAngle(x, a, b, c);
      }
    }
  }

  const double absTotal = std::abs(total);

  if (std::abs(absTotal - FourPi) <= angleTol)
  {
    return 1; // clearly inside
  }
  if (absTotal <= angleTol)
  {
    return 0; // clearly outside
  }

  // Ambiguous: choose closer class
  return (std::abs(absTotal - FourPi) < absTotal) ? 1 : 0;
}

//------------------------------------------------------------------------------
// Determine whether or not a polyhedron is convex. This method is adapted
// from Devillers et al., "Checking the Convexity of Polytopes and the
// Planarity of Subdivisions", Computational Geometry, Volume 11, Issues 3 - 4,
// December 1998, Pages 187 - 208.
bool vtkPolyhedron::IsConvex()
{
  auto status = this->IsConvex(VTK_DEFAULT_PLANARITY_TOLERANCE);
  return status == Status::Valid;
}

vtkPolyhedron::Status vtkPolyhedron::IsConvex(double planarThreshold)
{
  double x[2][3];
  vtkVector3d n;
  vtkVector3d c, c0, c1;
  vtkVector3d c0p, c1p;
  vtkVector3d n0, n1;
  vtkVector3d np;
  vtkIdType i, w[2], edgeId, edgeFaces[2], v, r = 0;
  vtkIdType numPts;
  vtkNew<vtkIdList> face_tmp;
  const vtkIdType* face;
  const double eps = FLT_EPSILON;
  enum FaceNormalStatus
  {
    Unvisited,
    Degenerate,
    Visited
  };
  std::vector<std::pair<vtkVector3d, FaceNormalStatus>> faceNormalsAndStatus;

  std::vector<double> p(this->PointIds->GetNumberOfIds());
  vtkIdVectorType d(this->PointIds->GetNumberOfIds(), 0);

  // initialization
  std::map<vtkIdType, int> unevenCoedges;
  this->GenerateEdges(unevenCoedges);
  this->GenerateFaces();
  this->ConstructPolyData();
  this->ComputeBounds();

  // Compute face normals as best we can. This eliminates work below
  // when vtkPolygon::ComputeNormal is called for each edge (N times per polygon).
  // Also, we can determine whether normals need to be flipped as we visit edges
  // below. Each face is initially marked as unvisited. As we loop over edges of
  // the polyhedron, we may flip any unvisited normal if both polygons orient the
  // edge the same way.
  vtkIdType numFaces = this->Faces->GetNumberOfCells();
  faceNormalsAndStatus.reserve(numFaces);
  for (int ff = 0; ff < numFaces; ++ff)
  {
    vtkVector3d norm;
    this->Faces->GetCellAtId(ff, numPts, face, face_tmp);
    auto stat = vtkPolygon::ComputeNormal(this->Points, numPts, face, norm.GetData());
    faceNormalsAndStatus.emplace_back(norm,
      stat == vtkCellStatus::DegenerateFaces ? FaceNormalStatus::Degenerate
                                             : FaceNormalStatus::Unvisited);
  }

  // loop over all edges in the polyhedron
  this->EdgeTable->InitTraversal();
  while ((edgeId = this->EdgeTable->GetNextEdge(w[0], w[1])) >= 0)
  {
    // Are we allowed to flip a face-normal attached to this edge?
    // This is true when coedge pairs do not cancel one another out.
    // A flip will occur if allowed and if the local convexity test
    // would fail otherwise and if one of the faces has not been
    // visited (i.e., used to pass a convexity test elsewhere).
    bool flipAllowed = (unevenCoedges.find(edgeId) != unevenCoedges.end());

    // get the edge points
    this->Points->GetPoint(w[0], x[0]);
    this->Points->GetPoint(w[1], x[1]);

    // get the local face ids
    this->EdgeFaces->GetTypedTuple(edgeId, edgeFaces);

    // get the face vertex ids for the first face
    this->Faces->GetCellAtId(edgeFaces[0], numPts, face, face_tmp);

    // compute the centroid and normal for the first face
    auto status =
      vtkPolygon::ComputeCentroid(this->Points, numPts, face, c0.GetData(), planarThreshold);
    if (!status)
    {
      return status;
    }
    // vtkPolygon::ComputeNormal(this->Points, numPts, face, n0);
    n0 = faceNormalsAndStatus[edgeFaces[0]].first;

    // get the face vertex ids for the second face
    this->Faces->GetCellAtId(edgeFaces[1], numPts, face, face_tmp);

    // compute the centroid and normal for the second face
    status = vtkPolygon::ComputeCentroid(this->Points, numPts, face, c1.GetData(), planarThreshold);
    if (!status)
    {
      return status;
    }
    // vtkPolygon::ComputeNormal(this->Points, numPts, face, n1);
    n1 = faceNormalsAndStatus[edgeFaces[1]].first;

    // check for local convexity (the average of the two centroids must be
    // "below" both faces, as defined by their outward normals).
    c = 0.5 * (c0 + c1);
    c0p = c - c0;
    c1p = c - c1;

    bool out0 = n0.Dot(c0p) > 0.; // vtkMath::Dot(n0, c0p) > 0.;
    bool out1 = n1.Dot(c1p) > 0.; // vtkMath::Dot(n1, c1p) > 0.;
    if (out0 && out1 && faceNormalsAndStatus[edgeFaces[0]].second != Visited &&
      faceNormalsAndStatus[edgeFaces[1]].second != Visited && !flipAllowed)
    {
      // 1 flip is not allowed, but two are.
      out0 = false;
      out1 = false;
      faceNormalsAndStatus[edgeFaces[0]].first = -faceNormalsAndStatus[edgeFaces[0]].first;
      faceNormalsAndStatus[edgeFaces[1]].first = -faceNormalsAndStatus[edgeFaces[1]].first;
      n0 = -n0;
      n1 = -n1;
      updateFlippedEdges(unevenCoedges, edgeFaces[0], this->Faces, this->EdgeTable);
      updateFlippedEdges(unevenCoedges, edgeFaces[1], this->Faces, this->EdgeTable);
    }
    else if (flipAllowed)
    {
      // Flip at most one normal, and only if required and allowed.
      if (out0 && faceNormalsAndStatus[edgeFaces[0]].second != Visited)
      {
        out0 = false;
        n0 = -n0;
        faceNormalsAndStatus[edgeFaces[0]].first = -faceNormalsAndStatus[edgeFaces[0]].first;
        updateFlippedEdges(unevenCoedges, edgeFaces[0], this->Faces, this->EdgeTable);
      }
      else if (out1 && faceNormalsAndStatus[edgeFaces[1]].second != Visited)
      {
        out1 = false;
        n1 = -n1;
        faceNormalsAndStatus[edgeFaces[1]].first = -faceNormalsAndStatus[edgeFaces[1]].first;
        updateFlippedEdges(unevenCoedges, edgeFaces[1], this->Faces, this->EdgeTable);
      }
    }
    faceNormalsAndStatus[edgeFaces[0]].second = FaceNormalStatus::Visited;
    faceNormalsAndStatus[edgeFaces[1]].second = FaceNormalStatus::Visited;
    if (out0 || out1)
    {
      return Status::Nonconvex;
    }

    // check if the edge is a seam edge
    // 1. the edge must not be vertical
    // 2. the two faces must lie on the same side of a vertical plane
    // 3. the upper face must not be vertical

    // 1. simply check that the unit normal along the seam has x or y
    //    components
    for (i = 0; i < 3; i++)
    {
      n[i] = x[1][i] - x[0][i];
    }
    n.Normalize();
    if (std::abs(n[0]) < eps && std::abs(n[1]) < eps)
    {
      continue;
    }

    // 2. we need a plane through the seam and through a vector parallel to the
    //    z axis (or, more accurately, we need a vector perpendicular to this
    //    plane). This vector can be computed using the cross product between
    //    the a vector along the edge, and the vertical axis.
    np[0] = +n[1];
    np[1] = -n[0];
    np[2] = 0;

    for (i = 0; i < 3; i++)
    {
      c[i] = (x[1][i] + x[0][i]) * .5;
      c0p[i] = c0[i] - c[i];
      c1p[i] = c1[i] - c[i];
    }

    // if the vectors from the seam centroid to the face centroid are in the
    // same direction relative to the plane, then condition 2 is satisfied.
    double tmp0 = np.Dot(c0p);
    double tmp1 = np.Dot(c1p);

    if ((tmp0 < 0.) != (tmp1 < 0.))
    {
      continue;
    }

    // 3. We get the z component of the normal of the highest face
    //    If this is null, the face is in the vertical plane
    double tmp0z = ((c0p[2] - c0p.Dot(n) * n[2]) > (c1p[2] - c1p.Dot(n) * n[2]) ? n0[2] : n1[2]);
    if (std::abs(tmp0z) < eps)
    {
      continue;
    }

    // at this point, we know we have a seam edge. We now look at each vertex
    // in the seam and determine whether or not it is a right-2-seam vertex. A
    // convex polytope has exactly one right-2-seam vertex.
    for (i = 0; i < 2; i++)
    {
      v = w[i];

      // are there already 2 seams associated with this vertex? If so, then the
      // projection of the polytope onto the x-y plane would have multiple seams
      // emanating from the vertex => non-convex.
      if (d[v] == 2)
      {
        return Status::Nonconvex;
      }

      // is this the first time that this vertex has been associated with a
      // seam? If so, increment its seam count and record the x-coordinate of
      // the adjacent edge vertex.
      if (d[v] == 0)
      {
        d[v]++;
        p[v] = x[(i + 1) % 2][0];
      }
      else
      {
        d[v]++;
        // is v a right-2-seam vertex (i.e. is the x-value of v larger than the
        // x-values of both u and p[v])?
        if (x[i][0] > x[(i + 1) % 2][0] && x[i][0] > p[v])
        {
          // is this the first right-2-seam vertex?
          if (r == 0)
          {
            r++;
          }
          else
          {
            return Status::Nonconvex;
          }
        }
      }
    }
  }

  return Status::Valid;
}

//------------------------------------------------------------------------------
int vtkPolyhedron::CellBoundary(int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  double x[3], n[3], o[3], v[3];
  double minDist = VTK_DOUBLE_MAX;
  vtkIdType numFacePts = -1;
  vtkIdType minFaceId = 0;
  const vtkIdType* facePts = nullptr;
  vtkNew<vtkIdList> pts_tmp;
  vtkIdType nfaces = this->Faces->GetNumberOfCells();

  // compute coordinates
  this->ComputePositionFromParametricCoordinate(pcoords, x);

  for (vtkIdType fid = 0; fid < nfaces; ++fid)
  {
    vtkIdType npts;
    const vtkIdType* ptsIds;
    this->Faces->GetCellAtId(fid, npts, ptsIds, pts_tmp);

    if (npts < 3)
    {
      vtkErrorMacro("Find a face with "
        << npts << " vertices. Cannot return CellBoundary due to this degenerate case.");
      break;
    }

    vtkPolygon::ComputeNormal(this->Points, npts, ptsIds, n);
    vtkMath::Normalize(n);
    this->Points->GetPoint(ptsIds[0], o);
    v[0] = x[0] - o[0];
    v[1] = x[1] - o[1];
    v[2] = x[2] - o[2];
    double dist = std::abs(vtkMath::Dot(v, n));
    if (dist < minDist)
    {
      minDist = dist;
      numFacePts = npts;
      minFaceId = fid;
    }
  }

  pts->Reset();
  if (numFacePts > 0)
  {
    this->Faces->GetCellAtId(minFaceId, numFacePts, facePts, pts_tmp);
    for (vtkIdType i = 0; i < numFacePts; i++)
    {
      pts->InsertNextId(this->PointIds->GetId(facePts[i]));
    }
  }

  // determine whether point is inside of polygon
  if (pcoords[0] >= 0.0 && pcoords[0] <= 1.0 && pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
    pcoords[2] >= 0.0 && pcoords[2] <= 1.0 &&
    (this->IsInside(x, std::numeric_limits<double>::infinity())))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
double vtkPolyhedron::ComputeVolume()
{
  this->GenerateFaces();
  assert(this->Faces != nullptr && "Faces must be set before calling ComputeVolume.");

  // Divergence theorem: V = (1/6) sum over face triangles of v0 . (v1 x v2).
  // Fan-triangulate each face from its first vertex.
  double totalVolume6 = 0.0;

  const vtkIdType numFaces = this->Faces->GetNumberOfCells();
  const vtkIdType* facePts = nullptr;
  vtkIdType numFacePts;
  for (vtkIdType i = 0; i < numFaces; ++i)
  {
    this->Faces->GetCellAtId(i, numFacePts, facePts);
    if (numFacePts < 3)
    {
      continue;
    }

    double v0[3];
    this->Points->GetPoint(facePts[0], v0);

    for (vtkIdType j = 1; j < numFacePts - 1; ++j)
    {
      double v1[3], v2[3];
      this->Points->GetPoint(facePts[j], v1);
      this->Points->GetPoint(facePts[j + 1], v2);

      double cross[3];
      vtkMath::Cross(v1, v2, cross);
      totalVolume6 += vtkMath::Dot(v0, cross);
    }
  }

  return totalVolume6 / 6.0;
}

//----------------------------------------------------------------------------
bool vtkPolyhedron::GetCentroid(double centroid[3]) const
{
  assert(this->Faces != nullptr && "Faces must be set before calling GetCentroid.");

  // Compute volume and centroid using the divergence theorem.
  // Fan-triangulate each face from its first vertex. Each triangle (v0, v1, v2)
  // forms a signed tetrahedron with the origin. The signed volume of this tet is
  // det(v0, v1, v2) / 6 = v0 . (v1 x v2) / 6. Summing over all face triangles
  // gives the total signed volume. The centroid is the volume-weighted average
  // of the tet centroids (v0 + v1 + v2) / 4.
  double totalVolume6 = 0.0; // 6 x volume (defer division)
  std::fill_n(centroid, 3, 0.0);

  const vtkIdType numFaces = this->Faces->GetNumberOfCells();
  const vtkIdType* facePts = nullptr;
  vtkIdType numFacePts;
  for (vtkIdType i = 0; i < numFaces; ++i)
  {
    this->Faces->GetCellAtId(i, numFacePts, facePts);
    if (numFacePts < 3)
    {
      continue;
    }

    double v0[3];
    this->Points->GetPoint(facePts[0], v0);

    for (vtkIdType j = 1; j < numFacePts - 1; ++j)
    {
      double v1[3], v2[3];
      this->Points->GetPoint(facePts[j], v1);
      this->Points->GetPoint(facePts[j + 1], v2);

      // Signed tet volume x 6 = v0 . (v1 x v2)
      double cross[3];
      vtkMath::Cross(v1, v2, cross);
      const double det = vtkMath::Dot(v0, cross);

      totalVolume6 += det;
      // Accumulate centroid weighted by signed tet volume.
      // Tet centroid = (v0 + v1 + v2) / 4, weight = det / 6.
      // Combined: det * (v0 + v1 + v2) / 24. Defer the 1/24 division.
      centroid[0] += det * (v0[0] + v1[0] + v2[0]);
      centroid[1] += det * (v0[1] + v1[1] + v2[1]);
      centroid[2] += det * (v0[2] + v1[2] + v2[2]);
    }
  }

  if (std::abs(totalVolume6) > 1e-12)
  {
    // centroid = accumulated / (24 * (totalVolume6 / 6)) = accumulated / (4 * totalVolume6)
    const double invW = 1.0 / (4.0 * totalVolume6);
    centroid[0] *= invW;
    centroid[1] *= invW;
    centroid[2] *= invW;
    return true;
  }
  else
  {
    // Degenerate volume -- fall back to point average
    const vtkIdType numPts = this->Points->GetNumberOfPoints();
    std::fill_n(centroid, 3, 0.0);
    double x[3];
    for (vtkIdType i = 0; i < numPts; i++)
    {
      this->Points->GetPoint(i, x);
      vtkMath::Add(centroid, x, centroid);
    }
    vtkMath::MultiplyScalar(centroid, 1.0 / numPts);
    return false;
  }
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetParametricCenter(double pcoords[3])
{
  this->GenerateFaces();
  double centroid[3];
  this->GetCentroid(centroid);
  this->ComputeParametricCoordinate(centroid, pcoords);
  return 0; // The subId is always 0 for vtkPolyhedron
}

//------------------------------------------------------------------------------
int vtkPolyhedron::EvaluatePosition(const double x[3], double closestPoint[3],
  int& vtkNotUsed(subId), double pcoords[3], double& minDist2, double weights[])
{
  // compute parametric coordinates
  this->ComputeParametricCoordinate(x, pcoords);

  // construct polydata, the result is stored in this->PolyData,
  // the cell array is stored in this->Faces
  this->ConstructPolyData();

  // Construct cell locator
  this->ConstructLocator();

  // find closest point and store the squared distance
  vtkIdType cellId;
  int id;
  double cp[3];
  this->Cell->Initialize();
  this->CellLocator->FindClosestPoint(x, cp, this->Cell, cellId, id, minDist2);

  if (closestPoint)
  {
    closestPoint[0] = cp[0];
    closestPoint[1] = cp[1];
    closestPoint[2] = cp[2];
  }

  // get the MVC weights
  this->InterpolateFunctions(x, weights);

  // set distance to be zero, if point is inside
  int isInside = this->IsInside(x, std::numeric_limits<double>::infinity());
  if (isInside)
  {
    minDist2 = 0.0;
  }

  return isInside;
}

//------------------------------------------------------------------------------
void vtkPolyhedron::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  this->ComputePositionFromParametricCoordinate(pcoords, x);

  this->InterpolateFunctions(x, weights);
}

//------------------------------------------------------------------------------
void vtkPolyhedron::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  int i, j, k, idx;
  for (j = 0; j < dim; j++)
  {
    for (i = 0; i < 3; i++)
    {
      derivs[j * dim + i] = 0.0;
    }
  }

  static constexpr double Sample_Offset_In_Parameter_Space = 0.01;

  double x[4][3];
  double coord[3];

  // compute positions of point and three offset sample points
  coord[0] = pcoords[0];
  coord[1] = pcoords[1];
  coord[2] = pcoords[2];
  this->ComputePositionFromParametricCoordinate(coord, x[0]);

  coord[0] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[1]);
  coord[0] = pcoords[0];

  coord[1] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[2]);
  coord[1] = pcoords[1];

  coord[2] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[3]);
  coord[2] = pcoords[2];

  this->ConstructPolyData();
  int numVerts = this->PolyData->GetNumberOfPoints();

  double* weights = new double[numVerts];
  double* sample = new double[dim * 4];
  // for each sample point, sample data values
  for (idx = 0, k = 0; k < 4; k++) // loop over three sample points
  {
    this->InterpolateFunctions(x[k], weights);
    for (j = 0; j < dim; j++, idx++) // over number of derivates requested
    {
      sample[idx] = 0.0;
      for (i = 0; i < numVerts; i++)
      {
        sample[idx] += weights[i] * values[j + i * dim];
      }
    }
  }

  double v1[3], v2[3], v3[3];
  // compute differences along the two axes
  for (i = 0; i < 3; i++)
  {
    v1[i] = x[1][i] - x[0][i];
    v2[i] = x[2][i] - x[0][i];
    v3[i] = x[3][i] - x[0][i];
  }
  double l1 = vtkMath::Normalize(v1);
  double l2 = vtkMath::Normalize(v2);
  double l3 = vtkMath::Normalize(v3);

  // compute derivatives along x-y-z axes
  for (j = 0; j < dim; j++)
  {
    double ddx = (sample[dim + j] - sample[j]) / l1;
    double ddy = (sample[2 * dim + j] - sample[j]) / l2;
    double ddz = (sample[3 * dim + j] - sample[j]) / l3;

    // project onto global x-y-z axes
    derivs[3 * j] = ddx * v1[0] + ddy * v2[0] + ddz * v3[0];
    derivs[3 * j + 1] = ddx * v1[1] + ddy * v2[1] + ddz * v3[1];
    derivs[3 * j + 2] = ddx * v1[2] + ddy * v2[2] + ddz * v3[2];
  }

  delete[] weights;
  delete[] sample;
}

//------------------------------------------------------------------------------
void vtkPolyhedron::InterpolateFunctions(const double x[3], double* sf)
{
  // construct polydata, the result is stored in this->PolyData,
  // the cell array is stored in this>Faces
  this->ConstructPolyData();

  // compute the weights
  if (!this->PolyData->GetPoints())
  {
    return;
  }
  vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(
    x, this->PolyData->GetPoints(), this->Faces, sf);
}

//------------------------------------------------------------------------------
void vtkPolyhedron::InterpolateDerivs(const double x[3], double* derivs)
{
  (void)x;
  (void)derivs;
}

//------------------------------------------------------------------------------
int vtkPolyhedron::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  ptIds->Reset();

  if (!this->GetPoints() || !this->GetNumberOfPoints())
  {
    return 0;
  }

  this->ComputeBounds();

  // use ordered triangulator to triangulate the polyhedron.
  if (!this->Triangulator)
  {
    this->Triangulator = vtkSmartPointer<vtkOrderedTriangulator>::New();
  }

  this->Triangulator->InitTriangulation(this->Bounds, this->GetNumberOfPoints());
  this->Triangulator->PreSortedOff();

  double point[3];
  for (vtkIdType i = 0; i < this->GetNumberOfPoints(); i++)
  {
    this->GetPoints()->GetPoint(i, point);
    this->Triangulator->InsertPoint(i, point, point, 0);
  }
  this->Triangulator->Triangulate();
  this->Triangulator->AddTetras(0, ptIds);
  return 1;
}

//------------------------------------------------------------------------------
int vtkPolyhedron::TriangulateFaces(vtkIdList* newFaces)
{
  newFaces->Initialize();
  newFaces->InsertNextId(0); // Keep room for the total nb of faces
  vtkIdType totalNbOfFaces = 0;

  for (vtkIdType faceId = 0; faceId < this->GetNumberOfFaces(); ++faceId)
  {
    vtkCell* face = this->GetFace(faceId);
    if (!face)
    {
      vtkErrorMacro("Unable to retrieve the face !");
      return 0;
    }

    vtkNew<vtkIdList> ptIds;

    // Triangulate the face
    // - Triangle : returns the triangle
    // - Quad : adds the "shortest" diagonal
    // - Polygon : uses "EarCut" triangulation
    face->TriangulateIds(0, ptIds);

    // Insert triangles from triangulation
    const auto nbOfTriangles = ptIds->GetNumberOfIds() / 3;
    for (vtkIdType i = 0; i < nbOfTriangles; i++)
    {
      newFaces->InsertNextId(3); // Number of points
      for (vtkIdType j = 0; j < 3; j++)
      {
        newFaces->InsertNextId(ptIds->GetId(3 * i + j));
      }
    }

    totalNbOfFaces += nbOfTriangles;
  }

  // Insert the total number of faces (triangles) at the beginning
  newFaces->InsertId(0, totalNbOfFaces);

  return 1;
}

//------------------------------------------------------------------------------
int vtkPolyhedron::TriangulateFaces(vtkCellArray* newFaces)
{
  newFaces->Initialize();

  for (vtkIdType faceId = 0; faceId < this->GetNumberOfFaces(); ++faceId)
  {
    vtkCell* face = this->GetFace(faceId);
    if (!face)
    {
      vtkErrorMacro("Unable to retrieve the face !");
      return 0;
    }

    vtkNew<vtkIdList> ptIds;

    // Triangulate the face
    // - Triangle : returns the triangle
    // - Quad : adds the "shortest" diagonal
    // - Polygon : uses "EarCut" triangulation
    face->TriangulateIds(0, ptIds);

    // Insert triangles from triangulation
    const auto nbOfTriangles = ptIds->GetNumberOfIds() / 3;
    for (vtkIdType i = 0; i < nbOfTriangles; i++)
    {
      newFaces->InsertNextCell(3); // Number of points
      for (vtkIdType j = 0; j < 3; j++)
      {
        newFaces->InsertCellPoint(ptIds->GetId(3 * i + j));
      }
    }
    newFaces->GetConnectivityArray()->Squeeze();
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkPolyhedron::GeneratePointToIncidentFaces()
{
  // Allocate memory
  this->PointToIncidentFaces.clear();
  this->PointToIncidentFaces.resize(this->GetNumberOfPoints());
  // Add the faces that hold each cell local point id
  std::vector<std::set<vtkIdType>> setFacesOfPoint(this->GetNumberOfPoints());
  for (int faceIndex = 0; faceIndex < this->GetNumberOfFaces(); faceIndex++)
  {
    auto face = this->GetFace(faceIndex);
    // For each point of the face
    for (int pointIndexFace = 0; pointIndexFace < face->GetNumberOfPoints(); pointIndexFace++)
    {
      // Get the global id of the point of the face
      auto pointId = face->GetPointId(pointIndexFace);
      // Transform the global id of the point of the face to the local id of the point in the cell
      auto pointCellLocalId = this->PointIdMap[pointId];
      // Insert this face in the set
      setFacesOfPoint[pointCellLocalId].insert(faceIndex);
    }
  }
  // Fill in PointToIncidentFaces using the set data
  for (int pointIndex = 0; pointIndex < this->GetNumberOfPoints(); pointIndex++)
  {
    this->PointToIncidentFaces[pointIndex].resize(setFacesOfPoint[pointIndex].size());
    int indexInsert = 0;
    for (auto faceId : setFacesOfPoint[pointIndex])
    {
      this->PointToIncidentFaces[pointIndex][indexInsert++] = faceId;
    }
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkPolyhedron::GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < this->GetNumberOfPoints() && "pointId too large");
  if (this->PointToIncidentFaces.empty())
  {
    this->GeneratePointToIncidentFaces();
  }
  const auto& pointFaces = this->PointToIncidentFaces[pointId];
  faceIds = pointFaces.data();
  return static_cast<vtkIdType>(pointFaces.size());
}

bool IntersectWithContour(vtkCell* cell, vtkDataArray* pointScalars,
  vtkPolyhedron::vtkPointIdMap& pointIdMap, double value,
  std::function<bool(double, double)>& compare, bool& allTrue)
{
  allTrue = true;
  bool allFalse = true;

  int nPoints = cell->GetNumberOfPoints();
  for (int i = 0; i < nPoints; ++i)
  {
    vtkIdType globalPid = cell->GetPointId(i);
    vtkIdType localPid = pointIdMap.find(globalPid)->second;

    double pointValue = pointScalars->GetTuple1(localPid);

    if (compare(pointValue, value))
    {
      allFalse = false;
    }
    else
    {
      allTrue = false;
    }
  }

  return !(allTrue || allFalse);
}

// start new contouring code //

/*

This code contains a new way of polyhedral contouring. The approach is as follows:
each of the polyhedron faces is triangulated (independent on normal orientation).

After triangulation, the contouring will give exactly 0 or 1 lines across the
(tri-)faces. This allows for a straightforward face-edge-contourpoint walking to
create one or more closed contour polygons.

The face-edge walking starts a given contour point. Using a lookup structure,
the edge of the contour point is used to find an unvisited face in the list of
the two faces that border the edge. The edges of that face are then searched
to find the other edge with a contour point. These two contour points then define
one contour line. The walking procedure stops when the starting contour point
is reached again. The collection of lines forms a closed polyhedron.

*/

bool CheckWatertightNonManifoldPolyhedron(vtkPolyhedron* cell, EdgeSet& originalEdges)
{
  EdgeFaceSetMap directMap;
  int nFaces = cell->GetNumberOfFaces();
  for (int i = 0; i < nFaces; ++i)
  {
    vtkCell* face = cell->GetFace(i);
    for (int j = 0; j < face->GetNumberOfEdges(); ++j)
    {
      Edge e(face->GetEdge(j));
      originalEdges.insert(e);

      auto at = directMap.find(e);
      if (at == directMap.end())
      {
        std::set<vtkIdType> facesOfEdge;
        facesOfEdge.insert(i);
        directMap.insert(make_pair(e, facesOfEdge));
      }
      else
      {
        std::set<vtkIdType>& facesOfEdge = at->second;
        facesOfEdge.insert(i);
      }
    }
  }

  size_t nEdges = cell->GetNumberOfEdges();
  size_t sizeMap = directMap.size();
  if (sizeMap != nEdges)
  {
    vtkGenericWarningMacro(
      << "The number of edges in the edge>face map does not match the number of edges of the cell");
    return false;
  }

  bool ok = true;

  for (const auto& entry : directMap)
  {
    const std::set<vtkIdType>& facesOfEdge = entry.second;
    if (facesOfEdge.size() != 2)
    {
      vtkGenericWarningMacro(
        << "The polyhedron is not watertight or non-manifold because the number of faces of edge "
        << entry.first.first << "-" << entry.first.second << " is not 2 but "
        << facesOfEdge.size());
      ok = false;
    }
  }

  return ok;
}

/*

When directly triangulating the polyhedron faces that are not simple triangles or quads (i.e.
they're polygons), a problem can occur which gives the resulting triangulated polyhedron
non-manifold triangle faces

For example:

0 ----- 1 ----- 2
|       |       |
|       |       |
|       6       |
|       |       |
|       |       |
3 ----- 4 ----- 5

this can be triangulated as (0,1,6), (0,6,3), (3,6,4) and (1,2,6), (6,2,5), (6,5,4) (that would be
OK) OR triangulated as          (0,1,4), (0,4,3), (1,6,4) and (1,2,5), (1,5,4), (1,6,4) (that would
be NOT OK because of the duplicate (1,6,4) triangle)

In fact, the ear-clipping polygon triangulation can produce, depending on the geometry,
the *unwanted* triangulation instead of the desired one because it prioritizes triangles with
inner angles close to 60 degrees, even though it then ends with a triangle with a very large
internal angle (up to 180 degrees).

Therefore the preferred approach is to triangulate a polygon using a fan triangulation that gives
the smallest range of internal angles. This approach will always choose to triangulate starting at
(6) in the example given above. If (6) is moved out-of-plane as it were (see TestPolyhedron5.cxx)
then the tetrahedralization gives a face triangulation that includes the edge (1)-(4), but
triangulates the face as (1-4-2)-(2-4-5). The now preferred method triangulates it as
(6-2-1)-(6-2-5)-(6-5-4), thereby preserving the original shape of the polygon, even if it is
slightly concave. Note that extremely concave polygons will give completely incorrect triangulations
with this method, but that would also be problematic for the tetrahedralization approach.

*/
// by using an *ordered* set, the triangles are consistently ordered, independent of face normal

int FindLowestIndex(vtkIdType n, vtkIdType* arr)
{
  int lowest(-1);
  vtkIdType min(VTK_ID_MAX);
  for (int i = 0; i < n; ++i)
  {
    if (arr[i] < min)
    {
      lowest = i;
      min = arr[i];
    }
  }
  return lowest;
}

void FindLowestNeighbor(vtkIdType n, vtkIdType* arr, int idx, bool& mustReverse)
{
  idx += n; // add n to prevent negative remainders
  vtkIdType left = arr[(idx - 1) % n];
  vtkIdType right = arr[(idx + 1) % n];
  if (left < right)
  {
    mustReverse = true;
  }
  else if (left > right)
  {
    mustReverse = false;
  }
}

// independent of direction of the quad, return the same triangle(s). If a quad is organized
// [0,1,2,3] or [1,2,3,0] or whatever it should return the same two triangles so that two adjacent
// cells that have opposite normals on a quad will have the same consistent face triangulation and
// therefore the same polygonized border.
void TriangulateQuad(vtkCell* quad, FaceVector& faces)
{
  std::vector<vtkIdType> consistentTri1(3), consistentTri2(3);
  int l = FindLowestIndex(4, quad->GetPointIds()->GetPointer(0));
  bool mustReverse(false);
  FindLowestNeighbor(4, quad->GetPointIds()->GetPointer(0), l, mustReverse);

  if (mustReverse)
  {
    int m = l + 4; // add four to prevent negative remainders: ' (0-1)%4 => -1 '
    consistentTri1[0] = quad->GetPointIds()->GetId(l);
    consistentTri1[1] = quad->GetPointIds()->GetId((m - 1) % 4);
    consistentTri1[2] = quad->GetPointIds()->GetId((m - 2) % 4);

    consistentTri2[0] = quad->GetPointIds()->GetId(l);
    consistentTri2[1] = quad->GetPointIds()->GetId((m - 2) % 4);
    consistentTri2[2] = quad->GetPointIds()->GetId((m - 3) % 4);
  }
  else
  {
    consistentTri1[0] = quad->GetPointIds()->GetId(l);
    consistentTri1[1] = quad->GetPointIds()->GetId((l + 1) % 4);
    consistentTri1[2] = quad->GetPointIds()->GetId((l + 2) % 4);

    consistentTri2[0] = quad->GetPointIds()->GetId(l);
    consistentTri2[1] = quad->GetPointIds()->GetId((l + 2) % 4);
    consistentTri2[2] = quad->GetPointIds()->GetId((l + 3) % 4);
  }

  faces.push_back(consistentTri1);
  faces.push_back(consistentTri2);
}

int TriangulatePolygonAt(vtkCell* polygon, int offset, vtkIdList* triIds)
{
  triIds->Reset();
  int nPoints = polygon->GetNumberOfPoints();

  for (int i = 0; i < nPoints - 2; ++i)
  {
    int idx0 = offset;
    int idx1 = (i + offset + 1) % nPoints;
    int idx2 = (i + offset + 2) % nPoints;
    triIds->InsertNextId(polygon->GetPointId(idx0));
    triIds->InsertNextId(polygon->GetPointId(idx1));
    triIds->InsertNextId(polygon->GetPointId(idx2));
  }
  return nPoints - 2;
}

void CalculateAngles(const vtkIdType* tri, vtkPoints* phPoints,
  const vtkPolyhedron::vtkPointIdMap& pointIdMap, double& minAngle, double& maxAngle)
{
  vtkIdType idx0 = tri[0];
  vtkIdType idx1 = tri[1];
  vtkIdType idx2 = tri[2];

  idx0 = pointIdMap.find(idx0)->second;
  idx1 = pointIdMap.find(idx1)->second;
  idx2 = pointIdMap.find(idx2)->second;

  double p[9];
  phPoints->GetPoint(idx0, p + 0);
  phPoints->GetPoint(idx1, p + 3);
  phPoints->GetPoint(idx2, p + 6);

  minAngle = DBL_MAX;
  maxAngle = 0;

  vtkVector3d left, right;
  for (int i = 0; i < 3; ++i)
  {
    int a = 3 * i;
    int b = 3 * ((i + 1) % 3);
    int c = 3 * ((i + 2) % 3);

    double* p0 = p + a;
    double* p1 = p + b;
    double* p2 = p + c;

    left.Set(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]);
    right.Set(p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]);
    left.Normalize();
    right.Normalize();

    double dot = left.Dot(right);
    // rounding errors can occur in the vtkVector3d::Dot function,
    // clamp to [-1, 1] (i.e. the input range for the acos function)
    dot = std::min(1.0, dot);
    dot = std::max(-1.0, dot);

    double angle = acos(dot) * 180.0 / vtkMath::Pi();

    minAngle = std::min(angle, minAngle);
    maxAngle = std::max(angle, maxAngle);
  }
}

void TriangulatePolygon(vtkCell* polygon, FaceVector& faces, vtkIdList* triIds, vtkPoints* phPoints,
  const vtkPolyhedron::vtkPointIdMap& pointIdMap)
{
  // attempt a fan triangulation for each point on the polygon and choose the
  // fan triangulation with the lowest range in internal angles differing from 60 degrees

  int nPoints = polygon->GetNumberOfPoints();
  std::vector<double> minAngles(nPoints, DBL_MAX), maxAngles(nPoints, 0);

  for (int i = 0; i < nPoints; ++i)
  {
    int nTris = TriangulatePolygonAt(polygon, i, triIds);
    for (int j = 0; j < nTris; ++j)
    {
      double minAngle, maxAngle;
      CalculateAngles(triIds->GetPointer(3 * j), phPoints, pointIdMap, minAngle, maxAngle);
      minAngles[i] = std::min(minAngles[i], minAngle);
      maxAngles[i] = std::max(maxAngles[i], maxAngle);
    }
  }

  double minRange(DBL_MAX);
  int choose(-1);
  for (int i = 0; i < nPoints; ++i)
  {
    double minDiff = std::abs(60.0 - minAngles[i]);
    double maxDiff = std::abs(maxAngles[i] - 60.0);
    double range = minDiff + maxDiff;
    if (range < minRange)
    {
      choose = i;
      minRange = range;
    }
  }

  int nTris = TriangulatePolygonAt(polygon, choose, triIds);
  for (int i = 0; i < nTris; ++i)
  {
    Face tri;
    tri.push_back(triIds->GetId(3 * i + 0));
    tri.push_back(triIds->GetId(3 * i + 1));
    tri.push_back(triIds->GetId(3 * i + 2));
    faces.push_back(tri);
  }
}

void TriangulateFace(vtkCell* face, FaceVector& faces, vtkIdList* triIds, vtkPoints* phPoints,
  const vtkPolyhedron::vtkPointIdMap& pointIdMap)
{
  switch (face->GetCellType())
  {
    case VTK_TRIANGLE:
    {
      Face tri;
      for (int i = 0; i < 3; ++i)
      {
        tri.push_back(face->GetPointIds()->GetId(i));
      }
      faces.push_back(tri);
      break;
    }
    case VTK_QUAD:
    {
      TriangulateQuad(face, faces);
      break;
    }
    case VTK_POLYGON:
    {
      TriangulatePolygon(face, faces, triIds, phPoints, pointIdMap);
      break;
    }
    default:
    {
      vtkGenericWarningMacro(<< "Unable to triangulate face cell type " << face->GetCellType());
    }
  }
}

void vtkPolyhedron::Contour(double value, vtkDataArray* pointScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* vtkNotUsed(verts),
  vtkCellArray* vtkNotUsed(lines), vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd)
{
  // Use López polygon-tracing algorithm for isosurface extraction
  // Reference: López et al., "A new isosurface extraction method on arbitrary grids",
  // Journal of Computational Physics 444 (2021) 110579.
  vtkPolyhedronContour contour;

  auto result = contour.Execute(this, pointScalars, value, locator, inPd, outPd);

  if (result == vtkPolyhedronContour::CellClassification::Intersected)
  {
    // Output iso-polygons directly. The calling filter (e.g., vtkContourHelper)
    // handles triangulation if GenerateTriangles=ON.
    contour.OutputContours(polys, inCd, cellId, outCd);
  }
}

void vtkPolyhedron::Clip(double value, vtkDataArray* pointScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* connectivity, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  // Use López polygon-tracing algorithm for clipping
  // Reference: López et al., "A new isosurface extraction method on arbitrary grids",
  // Journal of Computational Physics 444 (2021) 110579.
  vtkPolyhedronContour contour;

  auto result = contour.Execute(this, pointScalars, value, locator, inPd, outPd);

  // Handle classification results:
  // AllInside: all vertices have phi > isoValue
  // AllOutside: all vertices have phi <= isoValue
  // With insideOut=0: keep phi >= value (inside)
  // With insideOut=1: keep phi <= value (outside)

  // Determine if the entire cell should be output unchanged:
  // - AllInside without insideOut: keep the whole cell
  // - AllOutside with insideOut: keep the whole cell
  // Otherwise, if not Intersected, nothing is output.
  bool outputEntireCell =
    (result == vtkPolyhedronContour::CellClassification::AllInside && !insideOut) ||
    (result == vtkPolyhedronContour::CellClassification::AllOutside && insideOut);

  if (outputEntireCell)
  {
    // Output entire cell unchanged
    double x[3];
    vtkNew<vtkIdList> faceStream;
    int nFaces = this->GetNumberOfFaces();
    faceStream->InsertNextId(nFaces);
    for (int i = 0; i < nFaces; ++i)
    {
      vtkCell* face = this->GetFace(i);
      int nFacePoints = static_cast<int>(face->GetNumberOfPoints());
      faceStream->InsertNextId(nFacePoints);
      for (int j = 0; j < nFacePoints; ++j)
      {
        face->GetPoints()->GetPoint(j, x);
        vtkIdType id(-1);
        locator->InsertUniquePoint(x, id);
        faceStream->InsertNextId(id);
        outPd->CopyData(inPd, face->GetPointId(j), id);
      }
    }
    if (nFaces > 0)
    {
      vtkIdType newCellId = connectivity->InsertNextCell(faceStream);
      outCd->CopyData(inCd, cellId, newCellId);
    }
  }
  else if (result == vtkPolyhedronContour::CellClassification::Intersected)
  {
    contour.OutputClip(this, connectivity, locator, inPd, outPd, inCd, cellId, outCd, insideOut);
  }
  // else: nothing to output
}

//------------------------------------------------------------------------------
void vtkPolyhedron::ClipWithContext(double value, vtkDataArray* pointScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* connectivity, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut,
  vtkCellArray* outFaces, vtkCellArray* outFaceLocs)
{
  vtkPolyhedronContour contour;
  auto result = contour.Execute(this, pointScalars, value, locator, inPd, outPd);

  bool outputEntireCell =
    (result == vtkPolyhedronContour::CellClassification::AllInside && !insideOut) ||
    (result == vtkPolyhedronContour::CellClassification::AllOutside && insideOut);

  // Note: vtkTableBasedClipDataSet Phase 1 pre-filters all-inside/all-outside
  // cells before Phase 4, so outputEntireCell is never true when called from TBC.
  // It is retained here for correctness when ClipWithContext is called directly
  // by other consumers that may not pre-classify.
  if (outputEntireCell)
  {
    if (outFaces && outFaceLocs)
    {
      // Direct path: write faces into outFaces/outFaceLocs.
      double x[3];
      int nFaces = this->GetNumberOfFaces();
      if (nFaces > 0)
      {
        std::vector<vtkIdType> faceIds;
        faceIds.reserve(nFaces);
        for (int i = 0; i < nFaces; ++i)
        {
          vtkCell* face = this->GetFace(i);
          int nFacePoints = static_cast<int>(face->GetNumberOfPoints());
          std::vector<vtkIdType> facePts(nFacePoints);
          for (int j = 0; j < nFacePoints; ++j)
          {
            face->GetPoints()->GetPoint(j, x);
            vtkIdType id(-1);
            locator->InsertUniquePoint(x, id);
            facePts[j] = id;
            outPd->CopyData(inPd, face->GetPointId(j), id);
          }
          faceIds.push_back(outFaces->InsertNextCell(nFacePoints, facePts.data()));
        }
        outFaceLocs->InsertNextCell(static_cast<vtkIdType>(faceIds.size()), faceIds.data());
        vtkIdType nPts = this->GetNumberOfPoints();
        vtkIdType newCellId =
          connectivity->InsertNextCell(nPts, this->GetPointIds()->GetPointer(0));
        if (outCd)
        {
          outCd->CopyData(inCd, cellId, newCellId);
        }
      }
    }
    else
    {
      // Fallback to embedded format.
      this->Clip(
        value, pointScalars, locator, connectivity, inPd, outPd, inCd, cellId, outCd, insideOut);
    }
  }
  else if (result == vtkPolyhedronContour::CellClassification::Intersected)
  {
    contour.OutputClip(this, connectivity, locator, inPd, outPd, inCd, cellId, outCd, insideOut,
      outFaces, outFaceLocs);
  }
  // else: nothing to output
}

//------------------------------------------------------------------------------
void vtkPolyhedron::ClipWithContext(double value, vtkDataArray* pointScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* connectivity, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut,
  vtkUnstructuredGrid* outUG)
{
  this->ClipWithContext(value, pointScalars, locator, connectivity, inPd, outPd, inCd, cellId,
    outCd, insideOut, outUG ? outUG->GetPolyhedronFaces() : nullptr,
    outUG ? outUG->GetPolyhedronFaceLocations() : nullptr);
}

//------------------------------------------------------------------------------
void vtkPolyhedron::ShallowCopy(vtkCell* c)
{
  this->Superclass::ShallowCopy(c);

  vtkPolyhedron* cell = vtkPolyhedron::SafeDownCast(c);
  if (cell)
  {
    this->GlobalFaces->ShallowCopy(cell->GlobalFaces);
    this->Initialize();
  }
}

//------------------------------------------------------------------------------
void vtkPolyhedron::DeepCopy(vtkCell* c)
{
  this->Superclass::DeepCopy(c);

  vtkPolyhedron* cell = vtkPolyhedron::SafeDownCast(c);
  if (cell)
  {
    this->GlobalFaces->DeepCopy(cell->GlobalFaces);
    this->Initialize();
  }
}

//------------------------------------------------------------------------------
void vtkPolyhedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Polygon:\n";
  this->Polygon->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Tetra:\n";
  this->Tetra->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Faces:\n";
  this->GlobalFaces->PrintSelf(os, indent.GetNextIndent());
}

VTK_ABI_NAMESPACE_END
