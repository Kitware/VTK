// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDelaunay3D.h"

#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Internal helpers for the optimised RequestData path.
// The fast mesh uses a flat array of lightweight tetrahedra with explicit
// O(1) face-neighbour pointers, replacing the heavy vtkUnstructuredGrid /
// vtkCellLinks infrastructure during construction, yielding a large
// speed-up.
namespace
{

//------------------------------------------------------------------------------
// Lightweight tetrahedron mesh.
// Hot/cold split for cache efficiency.  During the walk only the
// topology (PointIds/Neighbors) is touched; the circumsphere data
// (Radius2/Center) is only read during cavity expansion.  Keeping them
// in separate arrays avoids polluting the cache with unused circumsphere
// data during the walk, and vice-versa.
struct Tetra
{
  vtkIdType PointIds[4];  // vertex point-ids
  vtkIdType Neighbors[4]; // face-neighbour tet ids (-1 = mesh boundary)
};

struct TetraSphere
{
  double Radius2; // circumsphere radius squared
  double Center[3];
};

struct CavityFace
{
  vtkIdType PointIds[3];   // outward-oriented face vertices (from the deleted tet)
  vtkIdType NeighborTetra; // external neighbour tet id (-1 = mesh boundary)
  int NeighborFace;        // face index in external neighbour pointing back
};

// Tolerance factor for the in-sphere test (slightly less than 1.0 to
// avoid treating co-spherical points as inside).  Note: adding more 9s
// (towards the ~16-digit precision of double) makes the factor round to
// exactly 1.0 and removes the slack, so a deliberate ~1e-10 offset is used.
const double inSphereTol = 0.9999999999;

// Face f is opposite vertex v[f].  The three vertices of face f are
// v[faceVerts[f][0..2]], ordered so that the outward normal points
// away from v[f] (assuming positive-orientation tet).
const int faceVerts[4][3] = {
  { 1, 2, 3 }, // face 0
  { 0, 3, 2 }, // face 1
  { 0, 1, 3 }, // face 2
  { 0, 2, 1 }  // face 3
};

//------------------------------------------------------------------------------
// Walk through the mesh to find the tetrahedron containing point x,
// starting from startTet.  Returns tet index, or -1 on failure.
//
// At each iteration, advance to the face neighbour opposite the vertex
// with the most-negative barycentric coordinate of x (i.e. the face
// that x lies on the outside of, viewed from the current tet).
// Delaunay walks typically terminate in a small number of steps, but
// floating-point imprecision near degenerate configurations can in
// principle drive the walk into a cycle.  The hard iteration cap below
// turns such a hypothetical cycle into a graceful failure: the function
// returns -1, and the caller counts the point as a degeneracy and skips
// it, instead of looping forever.
vtkIdType FindContainingTetra(
  const std::vector<Tetra>& topo, const double* coords, const double x[3], vtkIdType startTet)
{
  vtkIdType cur = startTet;
  for (int iter = 0; iter < 400; iter++)
  {
    const Tetra& t = topo[cur];
    double* p0 = const_cast<double*>(&coords[t.PointIds[0] * 3]);
    double* p1 = const_cast<double*>(&coords[t.PointIds[1] * 3]);
    double* p2 = const_cast<double*>(&coords[t.PointIds[2] * 3]);
    double* p3 = const_cast<double*>(&coords[t.PointIds[3] * 3]);

    double b[4] = { 0.25, 0.25, 0.25, 0.25 }; // fallback for degenerate
    vtkTetra::BarycentricCoords(const_cast<double*>(x), p0, p1, p2, p3, b);

    int worst = -1;
    double worstVal = 0.0;
    for (int j = 0; j < 4; j++)
    {
      if (b[j] < worstVal)
      {
        worstVal = b[j];
        worst = j;
      }
    }
    if (worst < 0)
    {
      return cur; // point is inside this tet
    }
    vtkIdType nei = t.Neighbors[worst];
    if (nei < 0)
    {
      return -1; // walked off the mesh boundary
    }
    cur = nei;
  }
  return -1; // iteration limit reached
}

//------------------------------------------------------------------------------
// Expand the Delaunay cavity around a containing tetrahedron.
// On return cavityTets holds the IDs of all tets whose circumsphere
// contains x, and cavityFaces holds the boundary faces of the cavity
// together with their external-neighbour information.
// inCavity[] is a scratch marker array; entries touched here are reset
// by the caller after each insertion.
void ExpandCavity(const std::vector<Tetra>& topo, const std::vector<TetraSphere>& sph,
  const double x[3], vtkIdType startTet, std::vector<char>& inCavity,
  std::vector<vtkIdType>& cavityTets, std::vector<CavityFace>& cavityFaces)
{
  cavityTets.clear();
  cavityFaces.clear();

  cavityTets.push_back(startTet);
  inCavity[startTet] = 1;

  for (size_t ci = 0; ci < cavityTets.size(); ci++)
  {
    vtkIdType tid = cavityTets[ci];
    const Tetra& t = topo[tid];

    for (int f = 0; f < 4; f++)
    {
      vtkIdType nei = t.Neighbors[f];
      if (nei < 0)
      {
        CavityFace cf;
        cf.PointIds[0] = t.PointIds[faceVerts[f][0]];
        cf.PointIds[1] = t.PointIds[faceVerts[f][1]];
        cf.PointIds[2] = t.PointIds[faceVerts[f][2]];
        cf.NeighborTetra = -1;
        cf.NeighborFace = -1;
        cavityFaces.push_back(cf);
        continue;
      }
      if (inCavity[nei])
      {
        continue;
      }
      const TetraSphere& neiSph = sph[nei];
      double dx = x[0] - neiSph.Center[0];
      double dy = x[1] - neiSph.Center[1];
      double dz = x[2] - neiSph.Center[2];
      if (dx * dx + dy * dy + dz * dz < inSphereTol * neiSph.Radius2)
      {
        inCavity[nei] = 1;
        cavityTets.push_back(nei);
      }
      else
      {
        CavityFace cf;
        cf.PointIds[0] = t.PointIds[faceVerts[f][0]];
        cf.PointIds[1] = t.PointIds[faceVerts[f][1]];
        cf.PointIds[2] = t.PointIds[faceVerts[f][2]];
        cf.NeighborTetra = nei;
        cf.NeighborFace = -1;
        const Tetra& neiTopo = topo[nei];
        for (int ff = 0; ff < 4; ff++)
        {
          if (neiTopo.Neighbors[ff] == tid)
          {
            cf.NeighborFace = ff;
            break;
          }
        }
        cavityFaces.push_back(cf);
      }
    }
  }
}

struct PendingEdge
{
  vtkIdType Point0, Point1;
  vtkIdType TetraId;
  int Face;
};

struct EdgeFacePair
{
  vtkIdType Point0, Point1;
  int Face;
};

} // anonymous namespace
vtkStandardNewMacro(vtkDelaunay3D);

//------------------------------------------------------------------------------
// Structure used to represent sphere around tetrahedron
//
typedef struct
{
  double r2;
  double center[3];
} vtkDelaunayTetra;

// Special classes for manipulating tetra array
//
class vtkTetraArray
{ //;prevent man page generation
public:
  vtkTetraArray(vtkIdType sz, vtkIdType extend);
  ~vtkTetraArray() { delete[] this->Array; }
  vtkDelaunayTetra* GetTetra(vtkIdType tetraId) { return this->Array + tetraId; }
  void InsertTetra(vtkIdType tetraId, double r2, double center[3]);
  vtkDelaunayTetra* Resize(vtkIdType sz); // reallocates data

protected:
  vtkDelaunayTetra* Array; // pointer to data
  vtkIdType MaxId;         // maximum index inserted thus far
  vtkIdType Size;          // allocated size of data
  vtkIdType Extend;        // grow array by this amount
};

//------------------------------------------------------------------------------
vtkTetraArray::vtkTetraArray(vtkIdType sz, vtkIdType extend)
{
  this->MaxId = -1;
  this->Array = new vtkDelaunayTetra[sz];
  this->Size = sz;
  this->Extend = extend;
}

//------------------------------------------------------------------------------
void vtkTetraArray::InsertTetra(vtkIdType id, double r2, double center[3])
{
  if (id >= this->Size)
  {
    this->Resize(id + 1);
  }
  this->Array[id].r2 = r2;
  this->Array[id].center[0] = center[0];
  this->Array[id].center[1] = center[1];
  this->Array[id].center[2] = center[2];
  this->MaxId = std::max(id, this->MaxId);
}

//------------------------------------------------------------------------------
vtkDelaunayTetra* vtkTetraArray::Resize(vtkIdType sz)
{
  vtkDelaunayTetra* newArray;
  vtkIdType newSize;

  if (sz > this->Size)
  {
    newSize = this->Size + this->Extend * (((sz - this->Size) / this->Extend) + 1);
  }
  else if (sz == this->Size)
  {
    return this->Array;
  }
  else
  {
    newSize = sz;
  }

  if ((newArray = new vtkDelaunayTetra[newSize]) == nullptr)
  {
    vtkGenericWarningMacro(<< "Cannot allocate memory\n");
    return nullptr;
  }

  if (this->Array)
  {
    memcpy(newArray, this->Array, (sz < this->Size ? sz : this->Size) * sizeof(vtkDelaunayTetra));
    delete[] this->Array;
  }

  this->Size = newSize;
  this->Array = newArray;

  return this->Array;
}

// vtkDelaunay3D methods
//

//------------------------------------------------------------------------------
// Construct object with Alpha = 0.0; Tolerance = 0.001; Offset = 2.5;
// BoundingTriangulation turned off.
vtkDelaunay3D::vtkDelaunay3D()
{
  this->Alpha = 0.0;
  this->AlphaTets = 1;
  this->AlphaTris = 1;
  this->AlphaLines = 1;
  this->AlphaVerts = 1;
  this->Tolerance = 0.001;
  this->BoundingTriangulation = 0;
  this->Offset = 2.5;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->Locator = nullptr;
  this->TetraArray = nullptr;
  this->References = nullptr;

  // added for performance
  this->Tetras = vtkIdList::New();
  this->Tetras->Reserve(5);
  this->Faces = vtkIdList::New();
  this->Faces->Reserve(15);
  this->CheckedTetras = vtkIdList::New();
  this->CheckedTetras->Reserve(25);
}

//------------------------------------------------------------------------------
vtkDelaunay3D::~vtkDelaunay3D()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  delete this->TetraArray;

  this->Tetras->Delete();
  this->Faces->Delete();
  this->CheckedTetras->Delete();
}

//------------------------------------------------------------------------------
// special method for performance
static int GetTetraFaceNeighbor(vtkUnstructuredGrid* Mesh, vtkIdType tetraId, vtkIdType p1,
  vtkIdType p2, vtkIdType p3, vtkIdType& nei);

//------------------------------------------------------------------------------
// Find all faces that enclose a point. (Enclosure means not satisfying
// Delaunay criterion.) This method works in two distinct parts. First, the
// tetrahedra containing the point are found (there may be more than one if
// the point falls on an edge or face). Next, face neighbors of these points
// are visited to see whether they satisfy the Delaunay criterion. Face
// neighbors are visited repeatedly until no more tetrahedron are found.
// Enclosing tetras are returned in the tetras list; the enclosing faces
// are returned in the faces list.
vtkIdType vtkDelaunay3D::FindEnclosingFaces(double x[3], vtkUnstructuredGrid* Mesh,
  vtkIdList* tetras, vtkIdList* faces, vtkIncrementalPointLocator* locator)
{
  vtkIdType tetraId, i, numTetras;
  int j, insertFace;
  vtkIdType p1, p2, p3, nei;
  int hasNei;
  const vtkIdType* tetraPts;
  vtkIdType npts;
  vtkIdType closestPoint;
  double xd[3];
  xd[0] = x[0];
  xd[1] = x[1];
  xd[2] = x[2];

  // Start off by finding closest point and tetras that use the point.
  // This will serve as the starting point to determine an enclosing
  // tetrahedron. (We just need a starting point
  if (locator->IsInsertedPoint(x) >= 0)
  {
    this->NumberOfDuplicatePoints++;
    return 0;
  }

  closestPoint = locator->FindClosestInsertedPoint(x);
  vtkCellLinks* links = static_cast<vtkCellLinks*>(Mesh->GetLinks());
  int numCells = links->GetNcells(closestPoint);
  vtkIdType* cells = links->GetCells(closestPoint);
  if (numCells <= 0) // shouldn't happen
  {
    this->NumberOfDegeneracies++;
    return 0;
  }
  else
  {
    tetraId = cells[0];
  }

  // Okay, walk towards the containing tetrahedron
  tetraId = this->FindTetra(Mesh, xd, tetraId, 0);
  if (tetraId < 0)
  {
    this->NumberOfDegeneracies++;
    return 0;
  }

  // Initialize the list of tetras who contain the point according
  // to the Delaunay criterion.
  tetras->InsertNextId(tetraId); // means that point is in this tetra

  // Okay, check neighbors for Delaunay criterion. Purpose is to find
  // list of enclosing faces and deleted tetras.
  numTetras = tetras->GetNumberOfIds();
  for (this->CheckedTetras->Reset(), i = 0; i < numTetras; i++)
  {
    this->CheckedTetras->InsertId(i, tetras->GetId(i));
  }

  p1 = 0;
  p2 = 0;
  p3 = 0;
  for (i = 0; i < numTetras; i++)
  {
    tetraId = tetras->GetId(i);
    Mesh->GetCellPoints(tetraId, npts, tetraPts);
    for (j = 0; j < 4; j++)
    {
      insertFace = 0;
      // Make sure to arrange these points so that they're in
      // counterclockwise order when viewed from the center of the
      // cell
      switch (j)
      {
        case 0: // face 0: points 0, 1, 2
          p1 = tetraPts[0];
          p2 = tetraPts[1];
          p3 = tetraPts[2];
          break;
        case 1: // face 1: points 1, 2, 3 (must flip order!)
          p1 = tetraPts[1];
          p2 = tetraPts[3];
          p3 = tetraPts[2];
          break;
        case 2: // face 2: points 2, 3, 0
          p1 = tetraPts[2];
          p2 = tetraPts[3];
          p3 = tetraPts[0];
          break;
        case 3: // face 3: points 3, 0, 1 (must flip order!)
          p1 = tetraPts[3];
          p2 = tetraPts[1];
          p3 = tetraPts[0];
          break;
      }

      hasNei = GetTetraFaceNeighbor(Mesh, tetraId, p1, p2, p3, nei);

      // if a boundary face or an enclosing face
      if (!hasNei) // a boundary face
      {
        insertFace = 1;
      }
      else
      {
        if (this->CheckedTetras->IsId(nei) == -1) // if not checked
        {
          if (this->InSphere(xd, nei)) // if point inside circumsphere
          {
            numTetras++;
            tetras->InsertNextId(nei); // delete this tetra
          }
          else
          {
            insertFace = 1; // this is a boundary face
          }
          this->CheckedTetras->InsertNextId(nei); // okay, we've checked it
        }
        else
        {
          if (tetras->IsId(nei) == -1) // if checked but not deleted
          {
            insertFace = 1; // a boundary face
          }
        }
      }

      if (insertFace)
      {
        faces->InsertNextId(p1);
        faces->InsertNextId(p2);
        faces->InsertNextId(p3);
      }

    } // for each tetra face
  } // for all deleted tetras

  // Okay, let's delete the tetras and prepare the data structure
  for (i = 0; i < tetras->GetNumberOfIds(); i++)
  {
    tetraId = tetras->GetId(i);
    Mesh->GetCellPoints(tetraId, npts, tetraPts);
    for (j = 0; j < 4; j++)
    {
      this->References[tetraPts[j]]--;
      Mesh->RemoveReferenceToCell(tetraPts[j], tetraId);
    }
  }

  return (faces->GetNumberOfIds() / 3);
}

//------------------------------------------------------------------------------
int vtkDelaunay3D::FindTetra(vtkUnstructuredGrid* Mesh, double x[3], vtkIdType tetraId, int depth)
{
  double p[4][3];
  double b[4];
  vtkTetra* tetra;
  int neg = 0;
  int j, numNeg;
  double negValue;

  // prevent aimless wandering and death by recursion
  if (depth > 200)
  {
    return -1;
  }

  tetra = static_cast<vtkTetra*>(Mesh->GetCell(tetraId));
  for (j = 0; j < 4; j++) // load the points
  {
    tetra->Points->GetPoint(j, p[j]);
  }

  vtkTetra::BarycentricCoords(x, p[0], p[1], p[2], p[3], b);

  // find the most negative face
  for (negValue = VTK_DOUBLE_MAX, numNeg = j = 0; j < 4; j++)
  {
    if (b[j] < 0.0)
    {
      numNeg++;
      if (b[j] < negValue)
      {
        negValue = b[j];
        neg = j;
      }
    }
  }

  // if no negatives, then inside this tetra
  if (numNeg <= 0)
  {
    return tetraId;
  }

  // okay, march towards the most negative direction
  int p1 = 0, p2 = 0, p3 = 0;
  switch (neg)
  {
    case 0:
      p1 = tetra->PointIds->GetId(1);
      p2 = tetra->PointIds->GetId(2);
      p3 = tetra->PointIds->GetId(3);
      break;
    case 1:
      p1 = tetra->PointIds->GetId(0);
      p2 = tetra->PointIds->GetId(2);
      p3 = tetra->PointIds->GetId(3);
      break;
    case 2:
      p1 = tetra->PointIds->GetId(0);
      p2 = tetra->PointIds->GetId(1);
      p3 = tetra->PointIds->GetId(3);
      break;
    case 3:
      p1 = tetra->PointIds->GetId(0);
      p2 = tetra->PointIds->GetId(1);
      p3 = tetra->PointIds->GetId(2);
      break;
  }
  vtkIdType nei;
  if (GetTetraFaceNeighbor(Mesh, tetraId, p1, p2, p3, nei))
  {
    return this->FindTetra(Mesh, x, nei, ++depth);
  }
  else
  {
    return -1;
  }
}

//------------------------------------------------------------------------------
// 3D Delaunay triangulation. Steps are as follows:
//   1. For each point
//   2. Find tetrahedron point is in
//   3. Repeatedly visit face neighbors and evaluate Delaunay criterion
//   4. Gather list of faces forming boundary of insertion polyhedron
//   5. Make sure that faces/point combination forms good tetrahedron
//   6. Create tetrahedron from each point/face combination
//
// This implementation uses a lightweight internal mesh with explicit O(1)
// face-neighbour pointers, instead of the heavier vtkUnstructuredGrid /
// vtkCellLinks infrastructure.
//
int vtkDelaunay3D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Generating 3D Delaunay triangulation");

  // Initialize; check input
  //
  vtkPoints* inPoints = input->GetPoints();
  if (inPoints == nullptr)
  {
    vtkWarningMacro("Cannot triangulate; no input points");
    return 1;
  }
  vtkIdType numPoints = inPoints->GetNumberOfPoints();
  if (numPoints == 0)
  {
    vtkWarningMacro("Cannot triangulate; no input points");
    return 1;
  }

  // Create initial bounding triangulation. Have to create bounding points.
  // Initialize mesh structure.
  double center[3];
  input->GetCenter(center);
  double length = this->Offset * input->GetLength();
  if (length <= 0.0)
  {
    length = 1.0;
  }

  // Set the desired precision for the points in the output.
  vtkNew<vtkPoints> allPoints;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    allPoints->SetDataType(inPoints->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    allPoints->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    allPoints->SetDataType(VTK_DOUBLE);
  }
  allPoints->SetNumberOfPoints(numPoints + 6);

  const vtkIdType np = numPoints; // shorthand for bounding-point offsets
  double bp[6][3] = { { center[0] - length, center[1], center[2] },
    { center[0] + length, center[1], center[2] }, { center[0], center[1] - length, center[2] },
    { center[0], center[1] + length, center[2] }, { center[0], center[1], center[2] - length },
    { center[0], center[1], center[2] + length } };

  // Copy input points into allPoints (InsertTuples performs the type
  // conversion when allPoints and inPoints differ in precision), then
  // append the 6 bounding-octahedron points.
  allPoints->GetData()->InsertTuples(0, numPoints, 0, inPoints->GetData());
  for (int i = 0; i < 6; i++)
  {
    allPoints->SetPoint(np + i, bp[i]);
  }

  // Build a flat double-precision coordinate cache for direct access in the
  // tight insertion loop.  ShallowCopy shares the buffer when allPoints is
  // already double, and performs a converting deep copy when it is float.
  vtkNew<vtkDoubleArray> coordsArray;
  coordsArray->ShallowCopy(allPoints->GetData());
  const double* coords = coordsArray->GetPointer(0);

  // Squared tolerance for duplicate-point detection.  Duplicates are
  // detected by checking the vertices of the containing tetrahedron
  // after the walk – an O(1) test that replaces the expensive
  // locator-based scan used by the legacy path.
  double tol = this->Tolerance * input->GetLength();
  double tol2 = tol * tol;

  // create bounding octahedron: 6 points & 4 tetra
  //   tet 0: (P4, P5, P0, P2)   tet 1: (P4, P5, P2, P1)
  //   tet 2: (P4, P5, P1, P3)   tet 3: (P4, P5, P3, P0)
  // Neighbour table (face i is opposite vertex v[i]):
  //   tet k: n = {-1, -1, (k+1)%4, (k+3)%4}

  const size_t meshReserve = static_cast<size_t>(5) * numPoints + 4;
  std::vector<Tetra> topo;
  std::vector<TetraSphere> sph;
  topo.reserve(meshReserve);
  sph.reserve(meshReserve);
  topo.resize(4);
  sph.resize(4);

  const vtkIdType bndV[4][4] = { { np + 4, np + 5, np + 0, np + 2 },
    { np + 4, np + 5, np + 2, np + 1 }, { np + 4, np + 5, np + 1, np + 3 },
    { np + 4, np + 5, np + 3, np + 0 } };
  for (int k = 0; k < 4; k++)
  {
    for (int j = 0; j < 4; j++)
    {
      topo[k].PointIds[j] = bndV[k][j];
    }
    topo[k].Neighbors[0] = -1;
    topo[k].Neighbors[1] = -1;
    topo[k].Neighbors[2] = (k + 1) % 4;
    topo[k].Neighbors[3] = (k + 3) % 4;

    sph[k].Radius2 = vtkTetra::Circumsphere(const_cast<double*>(&coords[topo[k].PointIds[0] * 3]),
      const_cast<double*>(&coords[topo[k].PointIds[1] * 3]),
      const_cast<double*>(&coords[topo[k].PointIds[2] * 3]),
      const_cast<double*>(&coords[topo[k].PointIds[3] * 3]), sph[k].Center);
  }

  std::vector<char> active(4, 1);
  active.reserve(meshReserve);

  std::vector<vtkIdType> freelist;

  // Insert each point into triangulation. Points laying "inside"
  // of tetra cause tetra to be deleted, leaving a void with bounding
  // faces. Combination of point and each face is used to form new
  // tetrahedra.

  this->NumberOfDuplicatePoints = 0;
  this->NumberOfDegeneracies = 0;

  vtkIdType lastTet = 0;

  // Reusable per-insertion scratch buffers.
  std::vector<vtkIdType> cavityTets;
  cavityTets.reserve(32);
  std::vector<CavityFace> cavityFaces;
  cavityFaces.reserve(64);
  std::vector<char> inCavity(4, 0);

  std::vector<PendingEdge> pendingEdges;
  pendingEdges.reserve(192);

  for (vtkIdType ptId = 0; ptId < numPoints; ptId++)
  {
    double x[3];
    inPoints->GetPoint(ptId, x);

    // --- grow scratch marker if the mesh grew ---
    if (inCavity.size() < topo.size())
    {
      inCavity.resize(topo.size(), 0);
    }

    // --- walk to the containing tet ---
    vtkIdType containTet = FindContainingTetra(topo, coords, x, lastTet);
    if (containTet < 0)
    {
      this->NumberOfDegeneracies++;
      continue;
    }

    // --- duplicate check against nearby vertices ---
    // Check the four vertices of the containing tet plus the opposite
    // vertex of each face-neighbour.  This covers all vertices within
    // the immediate neighbourhood of x, giving equivalent coverage to
    // the old locator-based scan while remaining O(1).
    {
      const Tetra& ct = topo[containTet];
      bool isDuplicate = false;
      // Check containing tet vertices
      for (int j = 0; j < 4 && !isDuplicate; j++)
      {
        const double* vp = &coords[ct.PointIds[j] * 3];
        double dx = x[0] - vp[0];
        double dy = x[1] - vp[1];
        double dz = x[2] - vp[2];
        if (dx * dx + dy * dy + dz * dz < tol2)
        {
          isDuplicate = true;
        }
      }
      // Check opposite vertex of each face neighbour
      for (int j = 0; j < 4 && !isDuplicate; j++)
      {
        vtkIdType neiId = ct.Neighbors[j];
        if (neiId < 0)
        {
          continue;
        }
        // Face j of the containing tet is opposite vertex ct.PointIds[j].
        // The neighbour shares the other 3 vertices; its unique
        // ("opposite") vertex is the one not shared with ct.
        const Tetra& neiT = topo[neiId];
        for (int k = 0; k < 4; k++)
        {
          vtkIdType nv = neiT.PointIds[k];
          if (nv != ct.PointIds[0] && nv != ct.PointIds[1] && nv != ct.PointIds[2] &&
            nv != ct.PointIds[3])
          {
            const double* vp = &coords[nv * 3];
            double dx = x[0] - vp[0];
            double dy = x[1] - vp[1];
            double dz = x[2] - vp[2];
            if (dx * dx + dy * dy + dz * dz < tol2)
            {
              isDuplicate = true;
            }
            break;
          }
        }
      }
      if (isDuplicate)
      {
        this->NumberOfDuplicatePoints++;
        continue;
      }
    }

    // --- expand Delaunay cavity ---
    ExpandCavity(topo, sph, x, containTet, inCavity, cavityTets, cavityFaces);

    if (cavityFaces.empty())
    {
      for (auto id : cavityTets)
      {
        inCavity[id] = 0;
      }
      this->NumberOfDegeneracies++;
      continue;
    }

    // --- delete cavity tets, reclaim their slots ---
    for (auto id : cavityTets)
    {
      active[id] = false;
      freelist.push_back(id);
      inCavity[id] = 0;
    }

    // --- create one new tet per boundary face ---
    // The boundary face is in outward orientation (away from the cavity).
    // The new point is inside the cavity, so we swap v[1]↔v[2] of the
    // face to obtain a positively-oriented tetrahedron.

    vtkIdType numNewFaces = static_cast<vtkIdType>(cavityFaces.size());
    pendingEdges.clear();
    vtkIdType firstNewTet = -1;

    for (vtkIdType fi = 0; fi < numNewFaces; fi++)
    {
      const CavityFace& cf = cavityFaces[fi];

      // allocate slot
      vtkIdType newId;
      if (!freelist.empty())
      {
        newId = freelist.back();
        freelist.pop_back();
        active[newId] = true;
      }
      else
      {
        newId = static_cast<vtkIdType>(topo.size());
        topo.emplace_back();
        sph.emplace_back();
        active.push_back(true);
        if (inCavity.size() < topo.size())
        {
          inCavity.resize(topo.size(), 0);
        }
      }
      if (firstNewTet < 0)
      {
        firstNewTet = newId;
      }

      Tetra& nt = topo[newId];
      nt.PointIds[0] = cf.PointIds[0];
      nt.PointIds[1] = cf.PointIds[2]; // swap for positive orientation
      nt.PointIds[2] = cf.PointIds[1]; // swap
      nt.PointIds[3] = ptId;
      nt.Neighbors[0] = -1;
      nt.Neighbors[1] = -1;
      nt.Neighbors[2] = -1;
      nt.Neighbors[3] = cf.NeighborTetra;

      // update external neighbour to point back at the new tet
      if (cf.NeighborTetra >= 0 && cf.NeighborFace >= 0)
      {
        topo[cf.NeighborTetra].Neighbors[cf.NeighborFace] = newId;
      }

      // circumsphere
      sph[newId].Radius2 = vtkTetra::Circumsphere(const_cast<double*>(&coords[nt.PointIds[0] * 3]),
        const_cast<double*>(&coords[nt.PointIds[1] * 3]),
        const_cast<double*>(&coords[nt.PointIds[2] * 3]),
        const_cast<double*>(&coords[nt.PointIds[3] * 3]), sph[newId].Center);

      // --- match internal faces (those containing the new point) ---
      // After the swap the boundary-edge keys for each internal face are:
      //   face 0 → sort(cf.PointIds[1], cf.PointIds[2])
      //   face 1 → sort(cf.PointIds[0], cf.PointIds[1])
      //   face 2 → sort(cf.PointIds[0], cf.PointIds[2])
      EdgeFacePair edges[3] = { { std::min(cf.PointIds[1], cf.PointIds[2]),
                                  std::max(cf.PointIds[1], cf.PointIds[2]), 0 },
        { std::min(cf.PointIds[0], cf.PointIds[1]), std::max(cf.PointIds[0], cf.PointIds[1]), 1 },
        { std::min(cf.PointIds[0], cf.PointIds[2]), std::max(cf.PointIds[0], cf.PointIds[2]), 2 } };

      for (int ei = 0; ei < 3; ei++)
      {
        bool matched = false;
        for (auto& pe : pendingEdges)
        {
          if (pe.Point0 == edges[ei].Point0 && pe.Point1 == edges[ei].Point1)
          {
            topo[newId].Neighbors[edges[ei].Face] = pe.TetraId;
            topo[pe.TetraId].Neighbors[pe.Face] = newId;
            pe.Point0 = -1; // invalidate
            matched = true;
            break;
          }
        }
        if (!matched)
        {
          pendingEdges.push_back({ edges[ei].Point0, edges[ei].Point1, newId, edges[ei].Face });
        }
      }
    } // for each boundary face

    if (firstNewTet >= 0)
    {
      lastTet = firstNewTet;
    }

    if (!(ptId % 250))
    {
      vtkDebugMacro(<< "point #" << ptId);
      this->UpdateProgress(static_cast<double>(ptId) / numPoints);
      if (this->CheckAbort())
      {
        break;
      }
    }
  } // for all points

  vtkDebugMacro(<< "Triangulated " << numPoints << " points, " << this->NumberOfDuplicatePoints
                << " of which were duplicates");

  if (this->NumberOfDegeneracies > 0)
  {
    vtkWarningMacro(<< this->NumberOfDegeneracies
                    << " degenerate triangles encountered, mesh quality suspect");
  }

  // Send appropriate portions of triangulation to output
  //

  vtkIdType numTetras = static_cast<vtkIdType>(topo.size());
  output->Allocate(5 * numPoints);

  // tetraUse: 0 = deleted, 1 = discarded by alpha, 2 = output
  std::vector<char> tetraUse(numTetras);
  vtkSMPTools::For(0, numTetras,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; i++)
      {
        tetraUse[i] = active[i] ? 2 : 0;
      }
    });

  // if boundary triangulation not desired, delete tetras connected to
  // boundary points
  if (!this->BoundingTriangulation)
  {
    vtkSMPTools::For(0, numTetras,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; i++)
        {
          if (tetraUse[i] == 2)
          {
            for (int j = 0; j < 4; j++)
            {
              if (topo[i].PointIds[j] >= numPoints)
              {
                tetraUse[i] = 0;
                break;
              }
            }
          }
        }
      });
  }

  // If non-zero alpha value, then figure out which parts of mesh are
  // contained within alpha radius.
  //
  if (this->Alpha > 0.0)
  {
    using EdgeTupleType = EdgeTuple<vtkIdType, vtkIdType>;

    double alpha2 = this->Alpha * this->Alpha;
    std::vector<char> pointUse(numPoints + 6, 0);

    // Edges of the output tetrahedra and triangles, collected up front and
    // merged in parallel (rather than incrementally), so the line phase can
    // skip edges already covered by a higher-dimensional output primitive.
    std::vector<EdgeTupleType> coveredEdges;

    static const int edgeVerts[6][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 }, { 0, 3 }, { 1, 3 },
      { 2, 3 } };

    // Output tetrahedra if requested
    if (this->AlphaTets)
    {
      for (vtkIdType i = 0; i < numTetras; i++)
      {
        if (tetraUse[i] == 2)
        {
          if (sph[i].Radius2 > alpha2)
          {
            tetraUse[i] = 1;
          }
          else
          {
            for (int j = 0; j < 4; j++)
            {
              pointUse[topo[i].PointIds[j]] = 1;
            }
            for (int j = 0; j < 6; j++)
            {
              coveredEdges.emplace_back(
                topo[i].PointIds[edgeVerts[j][0]], topo[i].PointIds[edgeVerts[j][1]], 0);
            }
          }
        }
      }
    }

    // traverse tetras again, this time examining faces
    // used tetras have already been output, so we look at those that haven't
    if (this->AlphaTris)
    {
      for (vtkIdType i = 0; i < numTetras; i++)
      {
        if (tetraUse[i] == 1)
        {
          for (int f = 0; f < 4; f++)
          {
            vtkIdType fp1 = topo[i].PointIds[faceVerts[f][0]];
            vtkIdType fp2 = topo[i].PointIds[faceVerts[f][1]];
            vtkIdType fp3 = topo[i].PointIds[faceVerts[f][2]];

            if (this->BoundingTriangulation ||
              (fp1 < numPoints && fp2 < numPoints && fp3 < numPoints))
            {
              vtkIdType nei = topo[i].Neighbors[f];
              bool neiOk = (nei < 0) || (nei > i && tetraUse[nei] != 2);
              if (neiOk)
              {
                double dv1[3], dv2[3], dv3[3], dcenter[3];
                double tx1[3] = { coords[fp1 * 3], coords[fp1 * 3 + 1], coords[fp1 * 3 + 2] };
                double tx2[3] = { coords[fp2 * 3], coords[fp2 * 3 + 1], coords[fp2 * 3 + 2] };
                double tx3[3] = { coords[fp3 * 3], coords[fp3 * 3 + 1], coords[fp3 * 3 + 2] };
                vtkTriangle::ProjectTo2D(tx1, tx2, tx3, dv1, dv2, dv3);
                if (vtkTriangle::Circumcircle(dv1, dv2, dv3, dcenter) <= alpha2)
                {
                  vtkIdType tri[3] = { fp1, fp2, fp3 };
                  output->InsertNextCell(VTK_TRIANGLE, 3, tri);
                  coveredEdges.emplace_back(fp1, fp2, 0);
                  coveredEdges.emplace_back(fp2, fp3, 0);
                  coveredEdges.emplace_back(fp3, fp1, 0);
                  pointUse[fp1] = 1;
                  pointUse[fp2] = 1;
                  pointUse[fp3] = 1;
                }
              }
            }
          }
        }
      }
    }

    // Build a static locator over the covered edges for O(1) membership tests.
    vtkStaticEdgeLocatorTemplate<vtkIdType, vtkIdType> coveredLoc;
    if (!coveredEdges.empty())
    {
      coveredLoc.BuildLocator(static_cast<vtkIdType>(coveredEdges.size()), coveredEdges.data());
    }

    // traverse tetras again, this time examining edges.  Candidate line edges
    // (passing the alpha test and not covered by a tet/triangle) are gathered
    // and then merged in parallel to remove duplicates before output.
    if (this->AlphaLines)
    {
      std::vector<EdgeTupleType> lineEdges;
      for (vtkIdType i = 0; i < numTetras; i++)
      {
        if (tetraUse[i] == 1)
        {
          for (int j = 0; j < 6; j++)
          {
            vtkIdType ep1 = topo[i].PointIds[edgeVerts[j][0]];
            vtkIdType ep2 = topo[i].PointIds[edgeVerts[j][1]];
            if ((this->BoundingTriangulation || (ep1 < numPoints && ep2 < numPoints)) &&
              (coveredEdges.empty() || coveredLoc.IsInsertedEdge(ep1, ep2) < 0))
            {
              double ex1[3] = { coords[ep1 * 3], coords[ep1 * 3 + 1], coords[ep1 * 3 + 2] };
              double ex2[3] = { coords[ep2 * 3], coords[ep2 * 3 + 1], coords[ep2 * 3 + 2] };
              if (vtkMath::Distance2BetweenPoints(ex1, ex2) * 0.25 <= alpha2)
              {
                lineEdges.emplace_back(ep1, ep2, 0);
              }
            }
          }
        }
      }

      if (!lineEdges.empty())
      {
        vtkStaticEdgeLocatorTemplate<vtkIdType, vtkIdType> lineLoc;
        vtkIdType numUniqueEdges = 0;
        const vtkIdType* edgeOffsets = lineLoc.MergeEdges(
          static_cast<vtkIdType>(lineEdges.size()), lineEdges.data(), numUniqueEdges);
        for (vtkIdType e = 0; e < numUniqueEdges; e++)
        {
          const EdgeTupleType& edge = lineEdges[edgeOffsets[e]];
          vtkIdType ln[2] = { edge.V0, edge.V1 };
          output->InsertNextCell(VTK_LINE, 2, ln);
          pointUse[edge.V0] = 1;
          pointUse[edge.V1] = 1;
        }
      }
    }

    if (this->AlphaVerts)
    {
      for (vtkIdType pid = 0; pid < numPoints + 6; pid++)
      {
        if ((pid < numPoints || this->BoundingTriangulation) && !pointUse[pid])
        {
          vtkIdType vt[1] = { pid };
          output->InsertNextCell(VTK_VERTEX, 1, vt);
        }
      }
    }
  } // if output alpha shapes

  // Update output; free up supporting data structures.
  //
  if (this->BoundingTriangulation)
  {
    output->SetPoints(allPoints);
  }
  else
  {
    if (inPoints->GetDataType() != allPoints->GetDataType())
    {
      allPoints->DeepCopy(inPoints);
      output->SetPoints(allPoints);
    }
    else
    {
      output->SetPoints(inPoints);
    }
    output->GetPointData()->PassData(input->GetPointData());
  }

  for (vtkIdType i = 0; i < numTetras; i++)
  {
    if (tetraUse[i] == 2)
    {
      output->InsertNextCell(VTK_TETRA, 4, topo[i].PointIds);
    }
  }

  vtkDebugMacro(<< "Generated " << output->GetNumberOfPoints() << " points and "
                << output->GetNumberOfCells() << " tetrahedra");

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
// This is a helper method used with InsertPoint() to create
// tetrahedronalizations of points. Its purpose is construct an initial
// Delaunay triangulation into which to inject other points. You must
// specify the center of a cubical bounding box and its length, as well
// as the number of points to insert. The method returns a pointer to
// an unstructured grid. Use this pointer to manipulate the mesh as
// necessary. You must delete (with Delete()) the mesh when done.
// Note: This initialization method places points forming bounding octahedron
// at the end of the Mesh's point list. That is, InsertPoint() assumes that
// you will be inserting points between (0,numPtsToInsert-1).
vtkUnstructuredGrid* vtkDelaunay3D::InitPointInsertion(
  double center[3], double length, vtkIdType numPtsToInsert, vtkPoints*& points)
{
  double x[3], bounds[6];
  vtkIdType tetraId;
  vtkIdType pts[4];
  vtkUnstructuredGrid* Mesh = vtkUnstructuredGrid::New();
  Mesh->EditableOn();

  if (numPtsToInsert == 0)
  {
    return Mesh;
  }

  this->NumberOfDuplicatePoints = 0;
  this->NumberOfDegeneracies = 0;

  if (length <= 0.0)
  {
    length = 1.0;
  }
  bounds[0] = center[0] - length;
  bounds[1] = center[0] + length;
  bounds[2] = center[1] - length;
  bounds[3] = center[1] + length;
  bounds[4] = center[2] - length;
  bounds[5] = center[2] + length;

  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(points, bounds);

  // create bounding octahedron: 6 points & 4 tetra
  x[0] = center[0] - length;
  x[1] = center[1];
  x[2] = center[2];
  this->Locator->InsertPoint(numPtsToInsert, x);

  x[0] = center[0] + length;
  x[1] = center[1];
  x[2] = center[2];
  this->Locator->InsertPoint(numPtsToInsert + 1, x);

  x[0] = center[0];
  x[1] = center[1] - length;
  x[2] = center[2];
  this->Locator->InsertPoint(numPtsToInsert + 2, x);

  x[0] = center[0];
  x[1] = center[1] + length;
  x[2] = center[2];
  this->Locator->InsertPoint(numPtsToInsert + 3, x);

  x[0] = center[0];
  x[1] = center[1];
  x[2] = center[2] - length;
  this->Locator->InsertPoint(numPtsToInsert + 4, x);

  x[0] = center[0];
  x[1] = center[1];
  x[2] = center[2] + length;
  this->Locator->InsertPoint(numPtsToInsert + 5, x);

  Mesh->Allocate(5 * numPtsToInsert);

  delete this->TetraArray;

  this->TetraArray = new vtkTetraArray(5 * numPtsToInsert, numPtsToInsert);

  // create bounding tetras (there are four)
  pts[0] = numPtsToInsert + 4;
  pts[1] = numPtsToInsert + 5;
  pts[2] = numPtsToInsert;
  pts[3] = numPtsToInsert + 2;
  tetraId = Mesh->InsertNextCell(VTK_TETRA, 4, pts);
  this->InsertTetra(Mesh, points, tetraId);

  pts[0] = numPtsToInsert + 4;
  pts[1] = numPtsToInsert + 5;
  pts[2] = numPtsToInsert + 2;
  pts[3] = numPtsToInsert + 1;
  tetraId = Mesh->InsertNextCell(VTK_TETRA, 4, pts);
  this->InsertTetra(Mesh, points, tetraId);

  pts[0] = numPtsToInsert + 4;
  pts[1] = numPtsToInsert + 5;
  pts[2] = numPtsToInsert + 1;
  pts[3] = numPtsToInsert + 3;
  tetraId = Mesh->InsertNextCell(VTK_TETRA, 4, pts);
  this->InsertTetra(Mesh, points, tetraId);

  pts[0] = numPtsToInsert + 4;
  pts[1] = numPtsToInsert + 5;
  pts[2] = numPtsToInsert + 3;
  pts[3] = numPtsToInsert;
  tetraId = Mesh->InsertNextCell(VTK_TETRA, 4, pts);
  this->InsertTetra(Mesh, points, tetraId);

  Mesh->SetPoints(points);
  points->Delete();
  Mesh->BuildLinks();

  // Keep track of change in references to points
  this->References = new int[numPtsToInsert + 6];
  memset(this->References, 0, (numPtsToInsert + 6) * sizeof(int));

  return Mesh;
}

//------------------------------------------------------------------------------
// This is a helper method used with InitPointInsertion() to create
// tetrahedronalizations of points. Its purpose is to inject point at
// coordinates specified into tetrahedronalization. The point id is an index
// into the list of points in the mesh structure.  (See
// vtkDelaunay3D::InitPointInsertion() for more information.)  When you have
// completed inserting points, traverse the mesh structure to extract desired
// tetrahedra (or tetra faces and edges). The holeTetras id list lists all the
// tetrahedra that are deleted (invalid) in the mesh structure.
void vtkDelaunay3D::InsertPoint(
  vtkUnstructuredGrid* Mesh, vtkPoints* points, vtkIdType ptId, double x[3], vtkIdList* holeTetras)
{
  vtkIdType tetraId, numFaces;
  int i;
  vtkIdType nodes[4];
  vtkIdType tetraNum, numTetras;

  this->Tetras->Reset();
  this->Faces->Reset();

  // Find faces containing point. (Faces are found by deleting
  // one or more tetrahedra "containing" point.) Tetrahedron contain point
  // when they satisfy Delaunay criterion. (More than one tetra may contain
  // a point if the point is on or near an edge or face.) For each face,
  // create a tetrahedron. (The locator helps speed search of points
  // in tetras.)
  if ((numFaces = this->FindEnclosingFaces(x, Mesh, this->Tetras, this->Faces, this->Locator)) > 0)
  {
    this->Locator->InsertPoint(ptId, x); // point is part of mesh now
    numTetras = this->Tetras->GetNumberOfIds();

    // create new tetra for each face
    for (tetraNum = 0; tetraNum < numFaces; tetraNum++)
    {
      // Define tetrahedron.  The order of the points matters: points
      // 0, 1, and 2 must appear in counterclockwise order when seen
      // from point 3.  When we get here, point ptId is inside the
      // tetrahedron whose faces we're considering and we've
      // guaranteed that the 3 points in this face are
      // counterclockwise wrt the new point.  That lets us create a
      // new tetrahedron with the right ordering.
      nodes[0] = this->Faces->GetId(3 * tetraNum);
      nodes[1] = this->Faces->GetId(3 * tetraNum + 1);
      nodes[2] = this->Faces->GetId(3 * tetraNum + 2);
      nodes[3] = ptId;

      // either replace previously deleted tetra or create new one
      if (tetraNum < numTetras)
      {
        tetraId = this->Tetras->GetId(tetraNum);
        Mesh->ReplaceCell(tetraId, 4, nodes);
      }
      else
      {
        tetraId = Mesh->InsertNextCell(VTK_TETRA, 4, nodes);
      }

      // Update data structures
      for (i = 0; i < 4; i++)
      {
        if (this->References[nodes[i]] >= 0)
        {
          Mesh->ResizeCellList(nodes[i], 5);
          this->References[nodes[i]] -= 5;
        }
        this->References[nodes[i]]++;
        Mesh->AddReferenceToCell(nodes[i], tetraId);
      }

      this->InsertTetra(Mesh, points, tetraId);

    } // for each face

    // Sometimes there are more tetras deleted than created. These
    // have to be accounted for because they leave a "hole" in the
    // data structure. Keep track of them here...mark them deleted later.
    for (tetraNum = numFaces; tetraNum < numTetras; tetraNum++)
    {
      holeTetras->InsertNextId(this->Tetras->GetId(tetraNum));
    }
  } // if enclosing faces found
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void vtkDelaunay3D::SetLocator(vtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkDelaunay3D::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = vtkPointLocator::New();
    vtkPointLocator::SafeDownCast(this->Locator)->SetDivisions(25, 25, 25);
  }
}

//------------------------------------------------------------------------------
// See whether point is in sphere of tetrahedron
int vtkDelaunay3D::InSphere(double x[3], vtkIdType tetraId)
{
  double dist2;
  vtkDelaunayTetra* tetra = this->TetraArray->GetTetra(tetraId);

  // check if inside/outside circumcircle
  dist2 = (x[0] - tetra->center[0]) * (x[0] - tetra->center[0]) +
    (x[1] - tetra->center[1]) * (x[1] - tetra->center[1]) +
    (x[2] - tetra->center[2]) * (x[2] - tetra->center[2]);

  if (dist2 < (0.9999999999L * tetra->r2))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
// Compute circumsphere and place into array of tetras
void vtkDelaunay3D::InsertTetra(vtkUnstructuredGrid* Mesh, vtkPoints* points, vtkIdType tetraId)
{
  double dx1[3], dx2[3], dx3[3], dx4[3], radius2, center[3];
  vtkIdType npts;
  const vtkIdType* pts;

  Mesh->GetCellPoints(tetraId, npts, pts);
  points->GetPoint(pts[0], dx1);
  points->GetPoint(pts[1], dx2);
  points->GetPoint(pts[2], dx3);
  points->GetPoint(pts[3], dx4);

  radius2 = vtkTetra::Circumsphere(dx1, dx2, dx3, dx4, center);
  this->TetraArray->InsertTetra(tetraId, radius2, center);
}

//------------------------------------------------------------------------------
void vtkDelaunay3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "Alpha Tets: " << (this->AlphaTets ? "On\n" : "Off\n");
  os << indent << "Alpha Tris: " << (this->AlphaTris ? "On\n" : "Off\n");
  os << indent << "Alpha Lines: " << (this->AlphaLines ? "On\n" : "Off\n");
  os << indent << "Alpha Verts: " << (this->AlphaVerts ? "On\n" : "Off\n");
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Bounding Triangulation: " << (this->BoundingTriangulation ? "On\n" : "Off\n");

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}

//------------------------------------------------------------------------------
void vtkDelaunay3D::EndPointInsertion()
{
  delete[] this->References;
  this->References = nullptr;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkDelaunay3D::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//------------------------------------------------------------------------------
static int GetTetraFaceNeighbor(vtkUnstructuredGrid* Mesh, vtkIdType tetraId, vtkIdType p1,
  vtkIdType p2, vtkIdType p3, vtkIdType& nei)
{
  // gather necessary information
  vtkCellLinks* links = static_cast<vtkCellLinks*>(Mesh->GetLinks());
  int numCells = links->GetNcells(p1);
  vtkIdType* cells = links->GetCells(p1);
  int i;
  vtkIdType npts;
  const vtkIdType* pts;

  // perform set operation
  for (i = 0; i < numCells; i++)
  {
    if (cells[i] != tetraId)
    {
      Mesh->GetCellPoints(cells[i], npts, pts);
      if ((p2 == pts[0] || p2 == pts[1] || p2 == pts[2] || p2 == pts[3]) &&
        (p3 == pts[0] || p3 == pts[1] || p3 == pts[2] || p3 == pts[3]))
      {
        nei = cells[i];
        break;
      }
    } // if not referring tetra
  } // for all candidate cells

  if (i < numCells)
  {
    return 1;
  }
  else
  {
    return 0; // there is no neighbor
  }
}

//------------------------------------------------------------------------------
int vtkDelaunay3D::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}
VTK_ABI_NAMESPACE_END
