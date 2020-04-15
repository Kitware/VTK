/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPolyhedron.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyhedron.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkEdgeTable.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkVector.h"

#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

vtkStandardNewMacro(vtkPolyhedron);

// Special typedef
typedef vector<vtkIdType> vtkIdVectorType;
class vtkPointIdMap : public map<vtkIdType, vtkIdType>
{
};

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

// these typedefs are for the contouoring code. There the order of two edges does not matter
// so we use the specially crafted equals and hash functions defined above.

typedef vector<Edge> EdgeVector;

typedef vector<EdgeVector> FaceEdgesVector;
typedef unordered_map<Edge, set<vtkIdType>, hash_fn, equal_fn> EdgeFaceSetMap;

typedef unordered_multimap<vtkIdType, Edge> PointIndexEdgeMultiMap;
typedef unordered_map<Edge, vtkIdType, hash_fn, equal_fn> EdgePointIndexMap;

typedef unordered_set<Edge, hash_fn, equal_fn> EdgeSet;

typedef vtkIdVectorType Face;
typedef vector<Face> FaceVector;

//// Special class for iterating through polyhedron faces
////----------------------------------------------------------------------------
class vtkPolyhedronFaceIterator
{
public:
  vtkIdType CurrentPolygonSize;
  vtkIdType* Polygon;
  vtkIdType* Current;
  vtkIdType NumberOfPolygons;
  vtkIdType Id;

  vtkPolyhedronFaceIterator(vtkIdType numFaces, vtkIdType* t)
  {
    this->CurrentPolygonSize = t[0];
    this->Polygon = t;
    this->Current = t + 1;
    this->NumberOfPolygons = numFaces;
    this->Id = 0;
  }
  vtkIdType* operator++()
  {
    this->Current += this->CurrentPolygonSize + 1;
    this->Polygon = this->Current - 1;
    this->Id++;
    if (this->Id < this->NumberOfPolygons)
    {
      this->CurrentPolygonSize = this->Polygon[0];
    }
    else
    {
      this->CurrentPolygonSize = VTK_ID_MAX;
    }
    return this->Current;
  }
};

//----------------------------------------------------------------------------
// Construct the hexahedron with eight points.
vtkPolyhedron::vtkPolyhedron()
{
  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
  this->Quad = vtkQuad::New();
  this->Polygon = vtkPolygon::New();
  this->Tetra = vtkTetra::New();
  this->GlobalFaces = vtkIdTypeArray::New();
  this->FaceLocations = vtkIdTypeArray::New();
  this->PointIdMap = new vtkPointIdMap;

  this->EdgesGenerated = 0;
  this->EdgeTable = vtkEdgeTable::New();
  this->Edges = vtkIdTypeArray::New();
  this->Edges->SetNumberOfComponents(2);
  this->EdgeFaces = vtkIdTypeArray::New();
  this->EdgeFaces->SetNumberOfComponents(2);

  this->FacesGenerated = 0;
  this->Faces = vtkIdTypeArray::New();

  this->BoundsComputed = 0;

  this->PolyDataConstructed = 0;
  this->PolyData = vtkPolyData::New();
  this->Polys = vtkCellArray::New();
  this->LocatorConstructed = 0;
  this->CellLocator = vtkCellLocator::New();
  this->CellIds = vtkIdList::New();
  this->Cell = vtkGenericCell::New();
}

//----------------------------------------------------------------------------
vtkPolyhedron::~vtkPolyhedron()
{
  this->Line->Delete();
  this->Triangle->Delete();
  this->Quad->Delete();
  this->Polygon->Delete();
  this->Tetra->Delete();
  this->GlobalFaces->Delete();
  this->FaceLocations->Delete();
  delete this->PointIdMap;
  this->EdgeTable->Delete();
  this->Edges->Delete();
  this->EdgeFaces->Delete();
  this->Faces->Delete();
  this->PolyData->Delete();
  this->Polys->Delete();
  this->CellLocator->Delete();
  this->CellIds->Delete();
  this->Cell->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyhedron::ComputeBounds()
{
  if (this->BoundsComputed)
  {
    return;
  }

  this->Superclass::GetBounds(); // stored in this->Bounds
  this->BoundsComputed = 1;
}

//----------------------------------------------------------------------------
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

  if (this->Faces->GetNumberOfTuples() == 0)
  {
    return;
  }

  const vtkIdType numCells = *this->Faces->GetPointer(0);
  const vtkIdType connSize = this->Faces->GetNumberOfValues() - numCells - 1;
  this->Polys->AllocateExact(numCells, connSize);
  this->Polys->ImportLegacyFormat(this->Faces->GetPointer(1), this->Faces->GetNumberOfValues() - 1);

  // Standard setup
  this->PolyData->Initialize();
  this->PolyData->SetPoints(this->Points);
  this->PolyData->SetPolys(this->Polys);

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
//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkPolyhedron::ComputeParametricCoordinate(const double x[3], double pc[3])
{
  this->ComputeBounds();
  double* bounds = this->Bounds;

  pc[0] = (x[0] - bounds[0]) / (bounds[1] - bounds[0]);
  pc[1] = (x[1] - bounds[2]) / (bounds[3] - bounds[2]);
  pc[2] = (x[2] - bounds[4]) / (bounds[5] - bounds[4]);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::ComputePositionFromParametricCoordinate(const double pc[3], double x[3])
{
  this->ComputeBounds();
  double* bounds = this->Bounds;
  x[0] = (1 - pc[0]) * bounds[0] + pc[0] * bounds[1];
  x[1] = (1 - pc[1]) * bounds[2] + pc[1] * bounds[3];
  x[2] = (1 - pc[2]) * bounds[4] + pc[2] * bounds[5];
}

//----------------------------------------------------------------------------
// Should be called by GetCell() prior to any other method invocation and after the
// points, point ids, and faces have been loaded.
void vtkPolyhedron::Initialize()
{
  // Clear out any remaining memory.
  this->PointIdMap->clear();

  // We need to create a reverse map from the point ids to their canonical cell
  // ids. This is a fancy way of saying that we have to be able to rapidly go
  // from a PointId[i] to the location i in the cell.
  vtkIdType i, id, numPointIds = this->PointIds->GetNumberOfIds();
  for (i = 0; i < numPointIds; ++i)
  {
    id = this->PointIds->GetId(i);
    (*this->PointIdMap)[id] = i;
  }

  // Edges have to be reset
  this->EdgesGenerated = 0;
  this->EdgeTable->Reset();
  this->Edges->Reset();
  this->EdgeFaces->Reset();
  this->Faces->Reset();

  // Polys have to be reset
  this->Polys->Reset();

  // Faces may need renumbering later. This means converting the face ids from
  // global ids to local, canonical ids.
  this->FacesGenerated = 0;

  // No bounds have been computed as of yet.
  this->BoundsComputed = 0;

  // No supplemental geometric stuff created
  this->PolyDataConstructed = 0;
  this->LocatorConstructed = 0;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfEdges()
{
  // Make sure edges have been generated.
  if (!this->EdgesGenerated)
  {
    this->GenerateEdges();
  }

  return static_cast<int>(this->Edges->GetNumberOfTuples());
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkPolyhedron::GenerateEdges()
{
  if (this->EdgesGenerated)
  {
    return this->Edges->GetNumberOfTuples();
  }

  // check the number of faces and return if there aren't any
  if (this->GlobalFaces->GetNumberOfTuples() == 0 || this->GlobalFaces->GetValue(0) <= 0)
  {
    return 0;
  }

  // Loop over all faces, inserting edges into the table
  vtkIdType* faces = this->GlobalFaces->GetPointer(0);
  vtkIdType nfaces = faces[0];
  vtkIdType* face = faces + 1;
  vtkIdType fid, i, edge[2], npts, edgeFaces[2], edgeId;
  edgeFaces[1] = -1;

  this->EdgeTable->InitEdgeInsertion(this->Points->GetNumberOfPoints(), 1);
  for (fid = 0; fid < nfaces; ++fid)
  {
    npts = face[0];
    for (i = 1; i <= npts; ++i)
    {
      edge[0] = (*this->PointIdMap)[face[i]];
      edge[1] = (*this->PointIdMap)[(i != npts ? face[i + 1] : face[1])];
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
    face += face[0] + 1;
  } // for all faces

  // Okay all done
  this->EdgesGenerated = 1;
  return this->Edges->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfFaces()
{
  // Make sure faces have been generated.
  if (!this->FacesGenerated)
  {
    this->GenerateFaces();
  }

  if (this->GlobalFaces->GetNumberOfTuples() == 0)
  {
    return 0;
  }

  return static_cast<int>(this->GlobalFaces->GetValue(0));
}

//----------------------------------------------------------------------------
void vtkPolyhedron::GenerateFaces()
{
  if (this->FacesGenerated)
  {
    return;
  }

  if (this->GlobalFaces->GetNumberOfTuples() == 0)
  {
    return;
  }

  // Basically we just run through the faces and change the global ids to the
  // canonical ids using the PointIdMap.
  this->Faces->SetNumberOfTuples(this->GlobalFaces->GetNumberOfTuples());
  vtkIdType* gFaces = this->GlobalFaces->GetPointer(0);
  vtkIdType* faces = this->Faces->GetPointer(0);
  vtkIdType nfaces = gFaces[0];
  faces[0] = nfaces;
  vtkIdType* gFace = gFaces + 1;
  vtkIdType* face = faces + 1;
  vtkIdType fid, i, id, npts;

  for (fid = 0; fid < nfaces; ++fid)
  {
    npts = gFace[0];
    face[0] = npts;
    for (i = 1; i <= npts; ++i)
    {
      id = (*this->PointIdMap)[gFace[i]];
      face[i] = id;
    }
    gFace += gFace[0] + 1;
    face += face[0] + 1;
  } // for all faces

  // Okay we've done the deed
  this->FacesGenerated = 1;
}

//----------------------------------------------------------------------------
vtkCell* vtkPolyhedron::GetFace(int faceId)
{
  if (faceId < 0 || faceId >= this->GlobalFaces->GetValue(0))
  {
    return nullptr;
  }

  this->GenerateFaces();

  // Okay load up the polygon
  vtkIdType i, p, loc = this->FaceLocations->GetValue(faceId);
  vtkIdType* face = this->GlobalFaces->GetPointer(loc);

  this->Polygon->PointIds->SetNumberOfIds(face[0]);
  this->Polygon->Points->SetNumberOfPoints(face[0]);

  // grab faces in global id space
  for (i = 0; i < face[0]; ++i)
  {
    this->Polygon->PointIds->SetId(i, face[i + 1]);
    p = (*this->PointIdMap)[face[i + 1]];
    this->Polygon->Points->SetPoint(i, this->Points->GetPoint(p));
  }

  return this->Polygon;
}

//----------------------------------------------------------------------------
// Specify the faces for this cell.
void vtkPolyhedron::SetFaces(vtkIdType* faces)
{
  // Set up face structure
  this->GlobalFaces->Reset();
  this->FaceLocations->Reset();

  if (!faces)
  {
    return;
  }

  vtkIdType nfaces = faces[0];
  this->FaceLocations->SetNumberOfValues(nfaces);

  this->GlobalFaces->InsertNextValue(nfaces);
  vtkIdType* face = faces + 1;
  vtkIdType faceLoc = 1;
  vtkIdType i, fid, npts;

  for (fid = 0; fid < nfaces; ++fid)
  {
    npts = face[0];
    this->GlobalFaces->InsertNextValue(npts);
    for (i = 1; i <= npts; ++i)
    {
      this->GlobalFaces->InsertNextValue(face[i]);
    }
    this->FaceLocations->SetValue(fid, faceLoc);

    faceLoc += face[0] + 1;
    face = faces + faceLoc;
  } // for all faces
}

//----------------------------------------------------------------------------
// Return the list of faces for this cell.
vtkIdType* vtkPolyhedron::GetFaces()
{
  if (!this->GlobalFaces->GetNumberOfTuples())
  {
    return nullptr;
  }

  return this->GlobalFaces->GetPointer(0);
}

//----------------------------------------------------------------------------
int vtkPolyhedron::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& tMin, double xMin[3], double pc[3], int& subId)
{
  // It's easiest if this is done in canonical space
  this->GenerateFaces();

  // Loop over all the faces, intersecting them in turn.
  vtkIdType* face = this->Faces->GetPointer(0);
  vtkIdType nfaces = *face++;
  vtkIdType npts, i, fid, numHits = 0;
  double t = VTK_FLOAT_MAX;
  double x[3];

  tMin = VTK_FLOAT_MAX;
  for (fid = 0; fid < nfaces; ++fid)
  {
    npts = face[0];
    vtkIdType hit = 0;
    switch (npts)
    {
      case 3: // triangle
        for (i = 0; i < 3; i++)
        {
          this->Triangle->Points->SetPoint(i, this->Points->GetPoint(face[i + 1]));
          this->Triangle->PointIds->SetId(i, face[i + 1]);
        }
        hit = this->Triangle->IntersectWithLine(p1, p2, tol, t, x, pc, subId);
        break;
      case 4: // quad
        for (i = 0; i < 4; i++)
        {
          this->Quad->Points->SetPoint(i, this->Points->GetPoint(face[i + 1]));
          this->Quad->PointIds->SetId(i, face[i + 1]);
        }
        hit = this->Quad->IntersectWithLine(p1, p2, tol, t, x, pc, subId);
        break;
      default: // general polygon
        this->Polygon->GetPoints()->SetNumberOfPoints(npts);
        this->Polygon->GetPointIds()->SetNumberOfIds(npts);
        for (i = 0; i < npts; i++)
        {
          this->Polygon->Points->SetPoint(i, this->Points->GetPoint(face[i + 1]));
          this->Polygon->PointIds->SetId(i, face[i + 1]);
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

    face += face[0] + 1;
  } // for all faces

  // Compute parametric coordinates
  this->ComputeParametricCoordinate(xMin, pc);

  return (numHits > 0);
}

#define VTK_MAX_ITER 10 // Maximum iterations for ray-firing
#define VTK_VOTE_THRESHOLD 3

//----------------------------------------------------------------------------
// Shoot random rays and count the number of intersections
int vtkPolyhedron::IsInside(const double x[3], double tolerance)
{
  // do a quick bounds check
  this->ComputeBounds();
  double* bounds = this->Bounds;
  if (x[0] < bounds[0] || x[0] > bounds[1] || x[1] < bounds[2] || x[1] > bounds[3] ||
    x[2] < bounds[4] || x[2] > bounds[5])
  {
    return 0;
  }

  // It's easiest if these computations are done in canonical space
  this->GenerateFaces();

  // This algorithm is adaptive; if there are enough faces in this
  // polyhedron, a cell locator is built to accelerate intersections.
  // Otherwise brute force looping over cells is used.
  vtkIdType* faceArray = this->Faces->GetPointer(0);
  vtkIdType nfaces = *faceArray++;
  if (nfaces > 25)
  {
    this->ConstructLocator();
  }

  // We need a length to normalize the computations
  double length = sqrt(this->Superclass::GetLength2());

  //  Perform in/out by shooting random rays. Multiple rays are fired
  //  to improve accuracy of the result.
  //
  //  The variable iterNumber counts the number of rays fired and is
  //  limited by the defined variable VTK_MAX_ITER.
  //
  //  The variable deltaVotes keeps track of the number of votes for
  //  "in" versus "out" of the surface.  When deltaVotes > 0, more votes
  //  have counted for "in" than "out".  When deltaVotes < 0, more votes
  //  have counted for "out" than "in".  When the delta_vote exceeds or
  //  equals the defined variable VTK_VOTE_THRESHOLD, then the
  //  appropriate "in" or "out" status is returned.
  //
  double rayMag, ray[3], xray[3], t, pcoords[3], xint[3];
  int i, numInts, iterNumber, deltaVotes, subId;
  vtkIdType idx, numCells;
  double tol = tolerance * length;

  for (deltaVotes = 0, iterNumber = 1;
       (iterNumber < VTK_MAX_ITER) && (abs(deltaVotes) < VTK_VOTE_THRESHOLD); iterNumber++)
  {
    //  Define a random ray to fire.
    do
    {
      for (i = 0; i < 3; i++)
      {
        ray[i] = vtkMath::Random(-1.0, 1.0);
      }
      rayMag = vtkMath::Norm(ray);
    } while (rayMag == 0.0);

    // The ray must be appropriately sized wrt the bounding box. (It has to go
    // all the way through the bounding box.)
    for (i = 0; i < 3; i++)
    {
      xray[i] = x[i] + (length / rayMag) * ray[i];
    }

    // Intersect the line with each of the candidate cells
    numInts = 0;

    if (this->LocatorConstructed)
    {
      // Retrieve the candidate cells from the locator
      this->CellLocator->FindCellsAlongLine(x, xray, tol, this->CellIds);
      numCells = this->CellIds->GetNumberOfIds();

      for (idx = 0; idx < numCells; idx++)
      {
        this->PolyData->GetCell(this->CellIds->GetId(idx), this->Cell);
        if (this->Cell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId))
        {
          // Check for vertex, edge or face intersections
          // count the number of 0 or 1 pcoords
          int pcount = 0;
          for (int p = 0; p < 3; ++p)
          {
            if (pcoords[p] == 0.0 || pcoords[p] == 1.0)
            {
              pcount++;
            }
          }
          // pcount = 1, exact face intersection
          // pcount = 2, exact edge intersection
          // pcount = 3, exact vertex intersection
          if (pcount == 0)
          {
            numInts++;
          }
        }
      } // for all candidate cells
    }
    else
    {
      numCells = nfaces;
      this->ConstructPolyData();

      for (idx = 0; idx < numCells; idx++)
      {
        this->PolyData->GetCell(idx, this->Cell);
        if (this->Cell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId))
        {
          // Check for vertex, edge or face intersections
          // count the number of 0 or 1 pcoords
          int pcount = 0;
          for (int p = 0; p < 3; ++p)
          {
            if (pcoords[p] == 0.0 || pcoords[p] == 1.0)
            {
              pcount++;
            }
          }
          // pcount = 1, exact face intersection
          // pcount = 2, exact edge intersection
          // pcount = 3, exact vertex intersection
          if (pcount == 0)
          {
            numInts++;
          }
        }
      } // for all candidate cells
    }

    // Count the result
    if (numInts != 0 && (numInts % 2) == 0)
    {
      --deltaVotes;
    }
    else
    {
      ++deltaVotes;
    }
  } // try another ray

  //   If the number of votes is positive, the point is inside
  //
  return (deltaVotes < 0 ? 0 : 1);
}

#undef VTK_MAX_ITER
#undef VTK_VOTE_THRESHOLD

//----------------------------------------------------------------------------
// Determine whether or not a polyhedron is convex. This method is adapted
// from Devillers et al., "Checking the Convexity of Polytopes and the
// Planarity of Subdivisions", Computational Geometry, Volume 11, Issues 3 - 4,
// December 1998, Pages 187 - 208.
bool vtkPolyhedron::IsConvex()
{
  double x[2][3], n[3], c[3], c0[3], c1[3], c0p[3], c1p[3], n0[3], n1[3];
  double np[3], tmp0, tmp1;
  vtkIdType i, w[2], edgeId, edgeFaces[2], loc, v, *face, r = 0;
  const double eps = FLT_EPSILON;

  std::vector<double> p(this->PointIds->GetNumberOfIds());
  vtkIdVectorType d(this->PointIds->GetNumberOfIds(), 0);

  // initialization
  this->GenerateEdges();
  this->GenerateFaces();
  this->ConstructPolyData();
  this->ComputeBounds();

  // loop over all edges in the polyhedron
  this->EdgeTable->InitTraversal();
  while ((edgeId = this->EdgeTable->GetNextEdge(w[0], w[1])) >= 0)
  {
    // get the edge points
    this->Points->GetPoint(w[0], x[0]);
    this->Points->GetPoint(w[1], x[1]);

    // get the local face ids
    this->EdgeFaces->GetTypedTuple(edgeId, edgeFaces);

    // get the face vertex ids for the first face
    loc = this->FaceLocations->GetValue(edgeFaces[0]);
    face = this->Faces->GetPointer(loc);

    // compute the centroid and normal for the first face
    vtkPolygon::ComputeCentroid(this->Points, face[0], face + 1, c0);
    vtkPolygon::ComputeNormal(this->Points, face[0], face + 1, n0);

    // get the face vertex ids for the second face
    loc = this->FaceLocations->GetValue(edgeFaces[1]);
    face = this->Faces->GetPointer(loc);

    // compute the centroid and normal for the second face
    vtkPolygon::ComputeCentroid(this->Points, face[0], face + 1, c1);
    vtkPolygon::ComputeNormal(this->Points, face[0], face + 1, n1);

    // check for local convexity (the average of the two centroids must be
    // "below" both faces, as defined by their outward normals).
    for (i = 0; i < 3; i++)
    {
      c[i] = (c1[i] + c0[i]) * .5;
      c0p[i] = c[i] - c0[i];
      c1p[i] = c[i] - c1[i];
    }

    if (vtkMath::Dot(n0, c0p) > 0. || vtkMath::Dot(n1, c1p) > 0.)
    {
      return false;
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
    vtkMath::Normalize(n);
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
    tmp0 = vtkMath::Dot(np, c0p);
    tmp1 = vtkMath::Dot(np, c1p);

    if ((tmp0 < 0.) != (tmp1 < 0.))
    {
      continue;
    }

    // 3. We get the z component of the normal of the highest face
    //    If this is null, the face is in the vertical plane
    tmp0 = c0[2] > c1[2] ? n0[2] : n1[2];
    if (std::abs(tmp0) < eps)
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
        return false;
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
            return false;
          }
        }
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::CellBoundary(int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  double x[3], n[3], o[3], v[3];
  double dist, minDist = VTK_DOUBLE_MAX;
  vtkIdType numFacePts = -1;
  vtkIdType* facePts = nullptr;

  // compute coordinates
  this->ComputePositionFromParametricCoordinate(pcoords, x);

  vtkPolyhedronFaceIterator faceIter(this->GetNumberOfFaces(), this->Faces->GetPointer(1));
  while (faceIter.Id < faceIter.NumberOfPolygons)
  {
    if (faceIter.CurrentPolygonSize < 3)
    {
      vtkErrorMacro("Find a face with "
        << faceIter.CurrentPolygonSize
        << " vertices. Cannot return CellBoundary due to this degenerate case.");
      break;
    }

    vtkPolygon::ComputeNormal(this->Points, faceIter.CurrentPolygonSize, faceIter.Current, n);
    vtkMath::Normalize(n);
    this->Points->GetPoint(faceIter.Current[0], o);
    v[0] = x[0] - o[0];
    v[1] = x[1] - o[1];
    v[2] = x[2] - o[2];
    dist = fabs(vtkMath::Dot(v, n));
    if (dist < minDist)
    {
      minDist = dist;
      numFacePts = faceIter.CurrentPolygonSize;
      facePts = faceIter.Current;
    }

    ++faceIter;
  }

  pts->Reset();
  if (numFacePts > 0)
  {
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
int vtkPolyhedron::EvaluatePosition(const double x[3], double closestPoint[3],
  int& vtkNotUsed(subId), double pcoords[3], double& minDist2, double weights[])
{
  // compute parametric coordinates
  this->ComputeParametricCoordinate(x, pcoords);

  // construct polydata, the result is stored in this->PolyData,
  // the cell array is stored in this->Polys
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

//----------------------------------------------------------------------------
void vtkPolyhedron::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  this->ComputePositionFromParametricCoordinate(pcoords, x);

  this->InterpolateFunctions(x, weights);
}

//----------------------------------------------------------------------------
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

  static const double Sample_Offset_In_Parameter_Space = 0.01;

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
  double l1, l2, l3;
  // compute differences along the two axes
  for (i = 0; i < 3; i++)
  {
    v1[i] = x[1][i] - x[0][i];
    v2[i] = x[2][i] - x[0][i];
    v3[i] = x[3][i] - x[0][i];
  }
  l1 = vtkMath::Normalize(v1);
  l2 = vtkMath::Normalize(v2);
  l3 = vtkMath::Normalize(v3);

  // compute derivatives along x-y-z axes
  double ddx, ddy, ddz;
  for (j = 0; j < dim; j++)
  {
    ddx = (sample[dim + j] - sample[j]) / l1;
    ddy = (sample[2 * dim + j] - sample[j]) / l2;
    ddz = (sample[3 * dim + j] - sample[j]) / l3;

    // project onto global x-y-z axes
    derivs[3 * j] = ddx * v1[0] + ddy * v2[0] + ddz * v3[0];
    derivs[3 * j + 1] = ddx * v1[1] + ddy * v2[1] + ddz * v3[1];
    derivs[3 * j + 2] = ddx * v1[2] + ddy * v2[2] + ddz * v3[2];
  }

  delete[] weights;
  delete[] sample;
}

//----------------------------------------------------------------------------
double* vtkPolyhedron::GetParametricCoords()
{
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::InterpolateFunctions(const double x[3], double* sf)
{
  // construct polydata, the result is stored in this->PolyData,
  // the cell array is stored in this->Polys
  this->ConstructPolyData();

  // compute the weights
  if (!this->PolyData->GetPoints())
  {
    return;
  }
  vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(
    x, this->PolyData->GetPoints(), this->Polys, sf);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::InterpolateDerivs(const double x[3], double* derivs)
{
  (void)x;
  (void)derivs;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::Triangulate(int vtkNotUsed(index), vtkIdList* ptIds, vtkPoints* pts)
{
  ptIds->Reset();
  pts->Reset();

  if (!this->GetPoints() || !this->GetNumberOfPoints())
  {
    return 0;
  }

  this->ComputeBounds();

  // use ordered triangulator to triangulate the polyhedron.
  vtkSmartPointer<vtkOrderedTriangulator> triangulator =
    vtkSmartPointer<vtkOrderedTriangulator>::New();

  triangulator->InitTriangulation(this->Bounds, this->GetNumberOfPoints());
  triangulator->PreSortedOff();

  double point[3];
  for (vtkIdType i = 0; i < this->GetNumberOfPoints(); i++)
  {
    this->GetPoints()->GetPoint(i, point);
    triangulator->InsertPoint(i, point, point, 0);
  }
  triangulator->Triangulate();

  triangulator->AddTetras(0, ptIds, pts);

  // convert to global Ids
  vtkIdType* ids = ptIds->GetPointer(0);
  for (vtkIdType i = 0; i < ptIds->GetNumberOfIds(); i++)
  {
    ids[i] = this->PointIds->GetId(ids[i]);
  }

  return 1;
}

bool IntersectWithContour(vtkCell* cell, vtkDataArray* pointScalars, vtkPointIdMap* pointIdMap,
  double value, function<bool(double, double)>& compare, bool& allTrue)
{
  allTrue = true;
  bool allFalse = true;

  int nPoints = cell->GetNumberOfPoints();
  for (int i = 0; i < nPoints; ++i)
  {
    vtkIdType globalPid = cell->GetPointId(i);
    vtkIdType localPid = pointIdMap->find(globalPid)->second;

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
        set<vtkIdType> facesOfEdge;
        facesOfEdge.insert(i);
        directMap.insert(make_pair(e, facesOfEdge));
      }
      else
      {
        set<vtkIdType>& facesOfEdge = at->second;
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
    const set<vtkIdType>& facesOfEdge = entry.second;
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
  vector<vtkIdType> consistentTri1(3), consistentTri2(2);
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

void CalculateAngles(const vtkIdType* tri, vtkPoints* phPoints, const vtkPointIdMap* pointIdMap,
  double& minAngle, double& maxAngle)
{
  vtkIdType idx0 = tri[0];
  vtkIdType idx1 = tri[1];
  vtkIdType idx2 = tri[2];

  idx0 = pointIdMap->find(idx0)->second;
  idx1 = pointIdMap->find(idx1)->second;
  idx2 = pointIdMap->find(idx2)->second;

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
    dot = min(1.0, dot);
    dot = max(-1.0, dot);

    double angle = acos(dot) * 180.0 / vtkMath::Pi();

    minAngle = min(angle, minAngle);
    maxAngle = max(angle, maxAngle);
  }
}

void TriangulatePolygon(vtkCell* polygon, FaceVector& faces, vtkIdList* triIds, vtkPoints* phPoints,
  vtkPointIdMap* pointIdMap)
{
  // attempt a fan triangulation for each point on the polygon and choose the
  // fan triangulation with the lowest range in internal angles differing from 60 degrees

  int nPoints = polygon->GetNumberOfPoints();
  vector<double> minAngles(nPoints, DBL_MAX), maxAngles(nPoints, 0);

  for (int i = 0; i < nPoints; ++i)
  {
    int nTris = TriangulatePolygonAt(polygon, i, triIds);
    for (int j = 0; j < nTris; ++j)
    {
      double minAngle, maxAngle;
      CalculateAngles(triIds->GetPointer(3 * j), phPoints, pointIdMap, minAngle, maxAngle);
      minAngles[i] = min(minAngles[i], minAngle);
      maxAngles[i] = max(maxAngles[i], maxAngle);
    }
  }

  double minRange(DBL_MAX);
  int choose(-1);
  for (int i = 0; i < nPoints; ++i)
  {
    double minDiff = abs(60.0 - minAngles[i]);
    double maxDiff = abs(maxAngles[i] - 60.0);
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
  vtkPointIdMap* pointIdMap)
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

bool CheckNonManifoldTriangulation(EdgeFaceSetMap& edgeFaceMap)
{
  for (const auto& entry : edgeFaceMap)
  {
    if (entry.second.size() != 2)
    {
      return false;
    }
  }
  return true;
}

bool GetContourPoints(double value, vtkPolyhedron* cell,
  vtkPointIdMap* pointIdMap, // from global id to local cell id
  FaceEdgesVector& faceEdgesVector, EdgeFaceSetMap& edgeFaceMap, EdgeSet& originalEdges,
  vector<vector<vtkIdType> >& oririginalFaceTriFaceMap,
  PointIndexEdgeMultiMap& contourPointEdgeMultiMap, EdgePointIndexMap& edgeContourPointMap,
  vtkIncrementalPointLocator* locator, vtkDataArray* pointScalars, vtkPointData* inPd,
  vtkPointData* outPd)
{

  vtkIdType nFaces = cell->GetNumberOfFaces();

  // this will contain the (possibly triangulated) faces
  // that will be contoured.
  FaceVector faces;

  if (!CheckWatertightNonManifoldPolyhedron(cell, originalEdges))
  {
    return false;
  }

  // temporaries for triangulation
  vtkNew<vtkIdList> triIds;

  for (vtkIdType i = 0; i < nFaces; ++i)
  {
    vtkCell* face = cell->GetFace(i);
    if (!face)
    {
      return false;
    }

    size_t nTris = faces.size();
    TriangulateFace(face, faces, triIds, cell->GetPoints(), pointIdMap);
    vector<vtkIdType> trisOfFace;
    for (size_t j = nTris; j < faces.size(); ++j)
    {
      trisOfFace.push_back((vtkIdType)j);
    }
    oririginalFaceTriFaceMap.push_back(trisOfFace);
  }

  // because of the triangulation performed above,
  // the faces vector now contains only faces that give exactly 0 or 1 contour lines.
  // this enables the walking of edge-face-contourpoint tuples to give closed contour polygon(s)

  // make the edge-face map and the face edges list
  nFaces = (vtkIdType)faces.size();
  for (int i = 0; i < nFaces; ++i)
  {
    Face& face = faces[i];
    size_t nFacePoints = face.size();

    EdgeVector edges;
    for (size_t j = 0; j < nFacePoints; ++j)
    {
      // each edge is in global id space.
      Edge e(face[j], face[(j + 1) % nFacePoints]);
      edges.push_back(e);

      auto at = edgeFaceMap.find(e);
      if (at == edgeFaceMap.end())
      {
        set<vtkIdType> facesOfEdge;
        facesOfEdge.insert(i); // this edge is connected to face i
        edgeFaceMap.insert(make_pair(e, facesOfEdge));
      }
      else
      {
        set<vtkIdType>& facesOfEdge = at->second;
        facesOfEdge.insert(i);
      }
    }

    faceEdgesVector.push_back(edges);
  }

  if (!CheckNonManifoldTriangulation(edgeFaceMap))
  {
    vtkGenericWarningMacro(<< "A cell with a non-manifold triangulation has been encountered. This "
                              "cell cannot be contoured.");
    return false;
  }

  vtkPoints* cellPoints = cell->GetPoints();

  const double eps = 1e-6;

  double p0[3], p1[3], cp[3]; // left, right and contour point

  for (const auto& entry : edgeFaceMap)
  {
    const Edge& edge = entry.first;

    // here we need to convert the global ids of the edge to
    // local ids to find the points and the point scalars.
    auto at0 = pointIdMap->find(edge.first);
    auto at1 = pointIdMap->find(edge.second);
    if (at0 == pointIdMap->end() || at1 == pointIdMap->end())
    {
      vtkGenericWarningMacro(<< "Could not find global id " << edge.first << " or " << edge.second);
      continue;
    }

    vtkIdType id0 = at0->second;
    vtkIdType id1 = at1->second;

    double v0 = pointScalars->GetTuple1(id0);
    double v1 = pointScalars->GetTuple1(id1);

    // TODO: check if a face falls completely in the value being contoured.
    //       then add face DIRECTLY.

    // TODO: what when an edge is completely on a contour value?

    // TODO: what when an existing point is on a contour value?
    //       in that case the edge-face-edge walking is no longer consistent:
    //       from the point you can walk to each of the faces that border the point,
    //       which often is larger than two.

    // FOR ALL ISSUES ABOVE, FOR NOW:
    //          clamp the fraction to be in <eps, 1-eps> to
    //          resolve any difficulties that arise from a contour lying within
    //          machine tolerance on an existing mesh point, edge or face.

    if ((v0 <= value && v1 > value) || (v1 <= value && v0 > value))
    {
      cellPoints->GetPoint(id0, p0);
      cellPoints->GetPoint(id1, p1);

      // note that the predicate for the if-statement we're in prohibits v1 == v0 == value
      // that means that an edge that is exactly on the contour will never be in the contour.
      // instead, two points that lie just off two other edges branching off that edge will
      // form the contour instead. That also prevents division by zero because v1 != v0 always
      double f = (value - v0) / (v1 - v0);

      f = max(0.0 + eps, f);
      f = min(1.0 - eps, f);

      for (int i = 0; i < 3; ++i)
      {
        cp[i] = (1.0 - f) * p0[i] + f * p1[i];
      }

      vtkIdType ptId(-1);
      locator->InsertUniquePoint(cp, ptId);

      // after point addition, also add the interpolated point value
      outPd->InterpolateEdge(inPd, ptId, edge.first, edge.second, f);

      // store result in the point->edge lookup structure
      contourPointEdgeMultiMap.insert(make_pair(ptId, edge));
    }
  }

  // build the reverse lookup structure edge->point
  for (const auto& entry : contourPointEdgeMultiMap)
  {
    auto range = contourPointEdgeMultiMap.equal_range(entry.first);
    for (auto jt = range.first; jt != range.second; ++jt)
    {
      edgeContourPointMap.insert(make_pair(jt->second, entry.first));
    }
  }

  return true;
}

int CreateContours(EdgeFaceSetMap& edgeFaceMap, FaceEdgesVector& faceEdgesVector,
  EdgePointIndexMap& edgeContourPointMap, EdgeSet& originalEdges,
  function<void(vtkIdList*)> contourCallback)
{
  EdgeSet availableContourEdges;
  for (const auto& entry : edgeContourPointMap)
  {
    availableContourEdges.insert(entry.first);
  }

  vtkNew<vtkIdList> poly;
  EdgeSet visited;
  while (!availableContourEdges.empty())
  {
    Edge start = *availableContourEdges.begin();
    Edge at(start);
    vtkIdType lastFace(-1);

    do
    {
      vtkIdType cp = edgeContourPointMap.find(at)->second;
      if (originalEdges.find(at) != originalEdges.end())
      {
        poly->InsertNextId(cp);
      }

      visited.insert(at);

      const set<vtkIdType>& facesOfEdge = edgeFaceMap[at];

      vtkIdType face(lastFace);
      for (const vtkIdType& faceOfEdge : facesOfEdge)
      {
        if (lastFace != faceOfEdge)
        {
          face = faceOfEdge;
          break;
        }
      }

      if (face == lastFace)
      {
        vtkGenericWarningMacro(<< "Face navigation failed in polyhedral contouring");
        return EXIT_FAILURE;
      }

      lastFace = face;

      const EdgeVector& edgesOfFace = faceEdgesVector[face];

      for (const auto& otherEdge : edgesOfFace)
      {
        if (equal_fn()(otherEdge, at))
        {
          continue;
        }

        auto found = edgeContourPointMap.find(otherEdge);
        if (found != edgeContourPointMap.end())
        {
          at = otherEdge;
          break;
        }
      }
    } while (!equal_fn()(at, start));

    if (poly->GetNumberOfIds() > 2)
    {
      // do something with the poly
      // contour: add directly to result;
      //    clip: use poly to carve off unwanted part(s)
      contourCallback(poly);
    }

    for (const Edge& it : visited)
    {
      availableContourEdges.erase(it);
    }
    poly->Reset();
    visited.clear();
  }

  return EXIT_SUCCESS;
}

void vtkPolyhedron::Contour(double value, vtkDataArray* pointScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  EdgeFaceSetMap edgeFaceMap;
  FaceEdgesVector faceEdgesVector;
  PointIndexEdgeMultiMap contourPointEdgeMultiMap;
  EdgePointIndexMap edgeContourPointMap;
  EdgeSet originalEdges;
  vector<vector<vtkIdType> > oririginalFaceTriFaceMap;

  if (!GetContourPoints(value, this, this->PointIdMap, faceEdgesVector, edgeFaceMap, originalEdges,
        oririginalFaceTriFaceMap, contourPointEdgeMultiMap, edgeContourPointMap, locator,
        pointScalars, inPd, outPd))
  {
    return;
  }

  vtkIdType offset(0);
  if (verts)
  {
    offset += verts->GetNumberOfCells();
  }
  if (lines)
  {
    offset += lines->GetNumberOfCells();
  }

  if (contourPointEdgeMultiMap.empty())
  {
    return; // no contours made
  }

  // the callback lambda will add each polygon found polys cell array
  function<void(vtkIdList*)> cb = [=](vtkIdList* poly) {
    if (!poly)
      return;

    vtkIdType newCellId =
      offset + polys->InsertNextCell(poly->GetNumberOfIds(), poly->GetPointer(0));
    outCd->CopyData(inCd, cellId, newCellId);
  };

  CreateContours(edgeFaceMap, faceEdgesVector, edgeContourPointMap, originalEdges, cb);
}

// start new clipping code
// first some support functions, see below for the Clip(...) function

void PolygonAsEdges(vector<vtkIdType>& polygon, vector<Edge>& edges,
  unordered_map<Edge, int, hash_fn, equal_fn>& edgeCount)
{
  for (size_t i = 0; i < polygon.size(); ++i)
  {
    Edge e(polygon[i], polygon[(i + 1) % polygon.size()]);
    edges.push_back(e);

    auto at = edgeCount.find(e);
    if (at == edgeCount.end())
    {
      edgeCount.insert(make_pair(e, 1));
    }
    else
    {
      int& counter = at->second;
      counter++;
    }
  }
}

bool FindNext(
  vector<Edge>& unordered, const Edge& last, vector<Edge>::iterator& next, Edge& nextEdge)
{
  for (auto it = unordered.begin(); it != unordered.end(); ++it)
  {
    if (last.second == it->first)
    {
      next = it;
      nextEdge = *it;
      return true;
    }
    else if (last.second == it->second)
    {
      nextEdge = Edge(it->second, it->first);
      next = it;
      return true;
    }
  }

  return false;
}

bool OrderEdgePolygon(vector<Edge>& unordered, vector<vector<Edge> >& ordered)
{
  if (unordered.empty())
  {
    return true;
  }

  vector<Edge> edgePolygon;

  // ! we are NOT taking a reference here on purpose because when
  // ! the vector 'unordered' has its first element removed, a reference would
  // ! point to the *NEW* first element of the vector, or be invalid if the
  // ! vector backing store is completely re-allocated.
  // ! So, don't do this: Edge& last = *unordered.begin();

  Edge last = *unordered.begin();
  edgePolygon.push_back(last);
  unordered.erase(unordered.begin());

  while (!unordered.empty())
  {
    vector<Edge>::iterator next;
    Edge nextEdge;
    if (!FindNext(unordered, last, next, nextEdge))
    {
      if (!unordered.empty())
      {
        last = *unordered.begin();
      }
      else
      {
        break;
      }

      ordered.push_back(edgePolygon);
      edgePolygon.clear();
      continue;
    }

    edgePolygon.push_back(nextEdge);
    last = nextEdge;
    unordered.erase(next);
  }
  ordered.push_back(edgePolygon);
  return true;
}

void EdgesToPolygon(vector<Edge>& edges, vector<vtkIdType>& polygon)
{
  for (auto it = edges.begin(); it != edges.end(); ++it)
  {
    polygon.push_back(it->first);
  }
}

void EdgesToPolygons(vector<vector<Edge> >& edgePolygons, vector<vector<vtkIdType> >& polygons)
{
  for (auto it = edgePolygons.begin(); it != edgePolygons.end(); ++it)
  {
    vector<Edge>& edgePolygon = *it;
    vector<vtkIdType> polygon;
    EdgesToPolygon(edgePolygon, polygon);
    polygons.push_back(polygon);
  }
}

void PruneContourPoints(vector<vector<vtkIdType> >& merged, EdgeSet& originalEdges,
  PointIndexEdgeMultiMap& contourPointEdgeMultiMap)
{
  for (auto it = merged.begin(); it != merged.end(); ++it)
  {
    vector<vtkIdType>& polygon = *it;
    // don't use size_t because the index i will get to -1 in the loop below
    // and size_t is *UNSIGNED*
    int i = (int)polygon.size() - 1;
    for (; i >= 0; --i)
    {
      auto at = contourPointEdgeMultiMap.find(polygon[i]);
      if (at != contourPointEdgeMultiMap.end())
      {
        bool doErase(true);
        auto eq = contourPointEdgeMultiMap.equal_range(polygon[i]);
        for (auto jt = eq.first; jt != eq.second; ++jt)
        {
          const Edge& edgeOfContourPoint = jt->second;
          if (originalEdges.find(edgeOfContourPoint) != originalEdges.end())
          {
            doErase = false;
            break;
          }
        }

        if (doErase)
        {
          // the contour point is on a non-original edge: remove it from the polygon.
          polygon.erase(polygon.begin() + i);
        }
      }
    }
  }
}

void MergeTriFacePolygons(vector<vector<vtkIdType> >& toMerge, vector<vector<vtkIdType> >& merged,
  EdgeSet& originalEdges, PointIndexEdgeMultiMap& contourPointEdgeMultiMap)
{
  // this is a five-step procedure:

  // 1) convert from vector<vtkIdType> to vector<Edge>
  // 2) remove duplicate edges;
  // 3) order the remaining edges head-to-tail;
  // 4) convert back from vector<Edge> to vector<vtkIdType>
  // 5) prune contour points that are not on original edges.

  // step 1: convert from vector<vtkIdType> to vector<Edge>
  vector<vector<Edge> > polygonsAsEdges;
  unordered_map<Edge, int, hash_fn, equal_fn> edgeCount;
  for (auto it = toMerge.begin(); it != toMerge.end(); ++it)
  {
    vector<Edge> edgesPolygon;
    PolygonAsEdges(*it, edgesPolygon, edgeCount);
    polygonsAsEdges.push_back(edgesPolygon);
  }

  // step 2: remove duplicate edges.
  for (auto it = polygonsAsEdges.begin(); it != polygonsAsEdges.end(); ++it)
  {
    vector<Edge>& edgesPolygon = *it;
    // don't use size_t because the index i will get to -1 in the loop below
    // and size_t is *UNSIGNED* => overflow
    int i = (int)edgesPolygon.size() - 1;
    for (; i >= 0; --i)
    {
      int ec = edgeCount.find(edgesPolygon[i])->second;
      if (ec == 2)
      {
        edgesPolygon.erase(edgesPolygon.begin() + i);
      }
    }
  }

  // step 3: throw remaining edges together
  vector<Edge> withoutDuplicates;
  for (auto it = polygonsAsEdges.begin(); it != polygonsAsEdges.end(); ++it)
  {
    vector<Edge>& edgesPolygon = *it;
    for (auto jt = edgesPolygon.begin(); jt != edgesPolygon.end(); ++jt)
    {
      withoutDuplicates.push_back(*jt);
    }
  }

  // step 3: and merge them
  vector<vector<Edge> > result;
  OrderEdgePolygon(withoutDuplicates, result);

  // step 4: convert back to vector<vtkIdType> polygons
  EdgesToPolygons(result, merged);

  // step 5: prune contour points that are not on original edges.
  PruneContourPoints(merged, originalEdges, contourPointEdgeMultiMap);
}

void MergeTriFacePolygons(vtkPolyhedron* cell,
  unordered_map<vtkIdType, vector<vtkIdType> >& triFacePolygonMap,
  vector<vector<vtkIdType> >& oririginalFaceTriFaceMap,
  PointIndexEdgeMultiMap& contourPointEdgeMultiMap, EdgeSet& originalEdges,
  vector<vector<vtkIdType> >& polygons)
{
  // for each *original* face, find the list of triangulated faces
  // and use these to get the list of polygons on the original face
  int nFaces = cell->GetNumberOfFaces();
  for (int i = 0; i < nFaces; ++i)
  {
    const vector<vtkIdType>& triFacesOfOriginalFace = oririginalFaceTriFaceMap[i];

    vector<vector<vtkIdType> > facePolygons;
    for (auto it = triFacesOfOriginalFace.begin(); it != triFacesOfOriginalFace.end(); ++it)
    {
      vtkIdType triFace = *it;
      auto at = triFacePolygonMap.find(triFace);
      if (at != triFacePolygonMap.end())
        facePolygons.push_back(at->second);
    }

    if (!facePolygons.empty())
    {
      vector<vector<vtkIdType> > mergedPolygons;
      MergeTriFacePolygons(facePolygons, mergedPolygons, originalEdges, contourPointEdgeMultiMap);
      for (auto it = mergedPolygons.begin(); it != mergedPolygons.end(); ++it)
      {
        polygons.push_back(*it);
      }
    }
  }
}

void vtkPolyhedron::Clip(double value, vtkDataArray* pointScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* connectivity, vtkPointData* inPd,
  vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  // set the compare function
  function<bool(double, double)> c = [insideOut](double a, double b) {
    if (insideOut)
      return less_equal<double>()(a, b);

    return greater_equal<double>()(a, b);
  };

  bool all(true);

  // check if polyhedron is all in
  bool intersect = IntersectWithContour(this, pointScalars, this->PointIdMap, value, c, all);
  if (!intersect && all)
  {
    double x[3];

    vtkNew<vtkIdList> faceStream;
    int nFaces = this->GetNumberOfFaces();
    faceStream->InsertNextId(nFaces);
    for (int i = 0; i < nFaces; ++i)
    {
      vtkCell* face = this->GetFace(i);
      int nFacePoints = (int)face->GetNumberOfPoints();
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
    return;
  }

  EdgeFaceSetMap edgeFaceMap;
  FaceEdgesVector faceEdgesVector;
  PointIndexEdgeMultiMap contourPointEdgeMultiMap;
  EdgePointIndexMap edgeContourPointMap;
  EdgeSet originalEdges;
  vector<vector<vtkIdType> > oririginalFaceTriFaceMap;

  if (!GetContourPoints(value, this, this->PointIdMap, faceEdgesVector, edgeFaceMap, originalEdges,
        oririginalFaceTriFaceMap, contourPointEdgeMultiMap, edgeContourPointMap, locator,
        pointScalars, inPd, outPd))
  {
    return;
  }

  if (contourPointEdgeMultiMap.empty())
  {
    return;
  }

  unordered_map<vtkIdType, vector<vtkIdType> > triFacePolygonMap;

  vtkPoints* cellPoints = this->GetPoints();

  // for all (triangulated) faces, walk the edges and insert (+) points and contour points
  // note: the edges are oriented head-to-tail and neighbor-to-neighbor, i.e. [0-1][1-2][2-0]
  for (size_t i = 0; i < faceEdgesVector.size(); ++i)
  {
    const EdgeVector& edges = faceEdgesVector[i];

    vector<vtkIdType> polygon;
    for (auto edgeIt = edges.begin(); edgeIt != edges.end(); ++edgeIt)
    {
      const Edge& edge = *edgeIt;
      vtkIdType v0 = edge.first;
      auto localIdIt = this->PointIdMap->find(v0);
      if (localIdIt == this->PointIdMap->end())
      {
        vtkGenericWarningMacro(<< "Could not find global id " << v0);
        continue;
      }
      vtkIdType localId = localIdIt->second;

      double val0 = pointScalars->GetTuple1(localId);
      if (c(val0, value))
      {
        vtkIdType id(-1);
        locator->InsertUniquePoint(cellPoints->GetPoint(localId), id);
        // we have added a point, so add point data to the output too
        // that has to be done in global id space
        outPd->CopyData(inPd, v0, id);
        polygon.push_back(id);
      }

      // if the current edge contains a contour point, add that as well
      // note: due to the edge ordering this works.
      auto at = edgeContourPointMap.find(edge);
      if (at != edgeContourPointMap.end())
      {
        polygon.push_back(at->second);
      }
    }

    // if a polygon was identified (if all face points are all + or all -, there is no polygon)
    if (!polygon.empty())
    {
      triFacePolygonMap.insert(make_pair(static_cast<vtkIdType>(i), polygon));
    }
  }

  std::vector<std::vector<vtkIdType> > polygons;
  MergeTriFacePolygons(this, triFacePolygonMap, oririginalFaceTriFaceMap, contourPointEdgeMultiMap,
    originalEdges, polygons);

  // next, get the contour polygons.

  // inside the callback lambda function defined below, we can only use pointers to capture
  // variables
  std::vector<std::vector<vtkIdType> >* pPolygons = &polygons;

  function<void(vtkIdList*)> cb = [=](vtkIdList* poly) {
    vtkIdType nIds = poly->GetNumberOfIds();
    vector<vtkIdType> polygon;
    polygon.reserve(nIds);
    for (int i = 0; i < nIds; ++i)
    {
      polygon.push_back(poly->GetId(i));
    }
    if (!polygon.empty())
      pPolygons->push_back(polygon);
  };

  CreateContours(edgeFaceMap, faceEdgesVector, edgeContourPointMap, originalEdges, cb);

  // this next bit finds closed polyhedra by looking at disjoint sets of point ids
  // that hold the polyhedra. Note that if two closed polyhedra share one point
  // that they are identified as one closed polyhedron with two closed parts.
  while (!polygons.empty())
  {
    // the set of point ids that form a closed polyhedron
    unordered_set<vtkIdType> polyhedralIdSet;

    // this list holds the polygons by moving references
    // in the polygons list of polyhedral faces that
    // belong to the polyhedron being built.
    std::vector<std::vector<vtkIdType> > polyhedralFaceList;

    // while one face is added, keep looping all faces that
    // were not yet added. The face last added can make faces that were
    // skipped earlier be valid candidates now. At a certain point, no
    // faces can be added anymore, and the polyhedron is finished.
    bool add = true;
    while (add)
    {
      add = false;
      auto it = polygons.begin();
      while (it != polygons.end())
      {
        // If there are empty polygons, we erase them
        while (it != polygons.end() && !it->size())
        {
          it = polygons.erase(it);
        }
        if (it == polygons.end())
        {
          // All polygons were empty
          break;
        }
        if (!polyhedralIdSet.size())
        {
          // Insert seed polygon in the polyhedron
          polyhedralIdSet.insert(it->begin(), it->end());
          continue;
        }

        const vector<vtkIdType>& nextPolygon = *it;
        auto polygon_it = nextPolygon.begin();
        bool insertedNextPolygon = false;
        for (; polygon_it != nextPolygon.end(); ++polygon_it)
        {
          // Check if the next polygon has any common point with the seed polygon
          if (polyhedralIdSet.find(*polygon_it) != polyhedralIdSet.end())
          {
            polyhedralIdSet.insert(nextPolygon.begin(), nextPolygon.end());
            polyhedralFaceList.emplace_back(std::move(*it));
            it = polygons.erase(it);
            // We might have missed a polygon earlier because
            // polyhedralIdSet has new ids now
            // this flag allows to scan again the list polygons
            add = true;
            insertedNextPolygon = true;
            // We found a polygon, we can look for another one now
            break;
          }
        }
        if (it == polygons.end())
        {
          break;
        }
        if (!insertedNextPolygon)
        {
          ++it;
        }
      }
    }
    if (polyhedralFaceList.size())
    {
      // next, build the face stream for the polyhedron.
      vtkNew<vtkIdList> polyhedron;
      // first entry: # of faces:
      polyhedron->InsertNextId(static_cast<vtkIdType>(polyhedralFaceList.size()));
      for (const auto& polyFace : polyhedralFaceList)
      {
        // each face entry starts with # points in that face
        polyhedron->InsertNextId(static_cast<vtkIdType>(polyFace.size()));
        for (const auto& id : polyFace)
        {
          // then all global face point ids
          polyhedron->InsertNextId(id);
        }
      }

      vtkIdType newCellId = connectivity->InsertNextCell(polyhedron);
      // we've added a cell, so add cell data too
      outCd->CopyData(inCd, cellId, newCellId);
    }
  }
}

//----------------------------------------------------------------------------
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
