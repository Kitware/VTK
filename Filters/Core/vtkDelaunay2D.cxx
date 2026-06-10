// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDelaunay2D.h"

#include "vtkAbstractTransform.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkHilbertCurveSorter.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"
#include "vtkTriangle.h"

#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Internal helpers for the optimised RequestData path.
// The fast mesh uses a flat array of lightweight triangles with explicit
// O(1) edge-neighbour pointers, replacing the heavy vtkPolyData /
// vtkCellLinks infrastructure during construction.
namespace
{

//------------------------------------------------------------------------------
// Lightweight triangle mesh.
// Neighbors[i] is the neighbour triangle across the edge opposite PointIds[i].
struct Triangle
{
  vtkIdType PointIds[3];  // vertex point-ids
  vtkIdType Neighbors[3]; // edge-neighbour tri ids (-1 = mesh boundary)
};

// Tolerance factor for the in-circle test (slightly less than 1.0 to
// avoid treating co-circular points as inside).  Note: adding more 9s
// (towards the ~16-digit precision of double) makes the factor round to
// exactly 1.0 and removes the slack, so a deliberate ~1e-10 offset is used.
const double inCircleTol = 0.9999999999;

//------------------------------------------------------------------------------
// Inline InCircle test (matches vtkDelaunay2D::InCircle behaviour).
inline int IsInsideCircumcircle(const double x[3], const double x1[3], const double x2[3],
  const double x3[3], double boundingRadius2)
{
  double center[2];
  double radius2 = vtkTriangle::Circumcircle(x1, x2, x3, center);
  if (radius2 > boundingRadius2)
  {
    return 1;
  }
  double dist2 = (x[0] - center[0]) * (x[0] - center[0]) + (x[1] - center[1]) * (x[1] - center[1]);
  return dist2 < (inCircleTol * radius2) ? 1 : 0;
}

//------------------------------------------------------------------------------
// Walk through the triangle mesh to find the triangle containing point x.
// Returns triangle index, or -1 on failure.
// Sets onEdge/neiTri/edgeV1/edgeV2 when the point lies on a shared edge.
//
// Uses the same normalised half-plane test as the original vtkDelaunay2D::
// FindTriangle to ensure scale-independent robustness.  Without
// normalisation, locally-clustered insertions produce very small triangles
// whose raw signed-area tests suffer from floating-point cancellation.
const double halfPlaneTol = 1.0e-14;

vtkIdType FindContainingTriangle(const std::vector<Triangle>& topo, const double* coords,
  const double x[3], vtkIdType startTri, double tol, bool& onEdge, vtkIdType& neiTri,
  vtkIdType& edgeV1, vtkIdType& edgeV2, bool& isDuplicate)
{
  onEdge = false;
  neiTri = -1;
  isDuplicate = false;

  vtkIdType cur = startTri;
  vtkIdType prev = -1;

  for (int iter = 0; iter < 5000; iter++)
  {
    const Triangle& t = topo[cur];
    const double* p[3];
    for (int i = 0; i < 3; i++)
    {
      p[i] = &coords[t.PointIds[i] * 3];
    }

    // Evaluate each edge using normalised half-plane tests (same as original).
    srand(static_cast<unsigned int>(cur));
    int ir = rand() % 3;
    int inside = 1;
    double minProj = halfPlaneTol;
    int bestEdgeOpp = -1; // index of vertex opposite the most-negative edge
    vtkIdType bestP1 = -1, bestP2 = -1;

    for (int ic = 0; ic < 3; ic++)
    {
      int i = (ir + ic) % 3;
      int i2 = (i + 1) % 3;
      int i3 = (i + 2) % 3;

      // 2D edge normal
      double n[2];
      n[0] = -(p[i2][1] - p[i][1]);
      n[1] = p[i2][0] - p[i][0];
      double nLen = sqrt(n[0] * n[0] + n[1] * n[1]);
      if (nLen > 0.0)
      {
        n[0] /= nLen;
        n[1] /= nLen;
      }

      // Vectors from edge vertex to opposite vertex and to query point
      double vp[2] = { p[i3][0] - p[i][0], p[i3][1] - p[i][1] };
      double vx[2] = { x[0] - p[i][0], x[1] - p[i][1] };

      double vpLen = sqrt(vp[0] * vp[0] + vp[1] * vp[1]);
      if (vpLen > 0.0)
      {
        vp[0] /= vpLen;
        vp[1] /= vpLen;
      }
      double vxLen = sqrt(vx[0] * vx[0] + vx[1] * vx[1]);
      if (vxLen <= tol)
      {
        isDuplicate = true;
        return -1;
      }
      vx[0] /= vxLen;
      vx[1] /= vxLen;

      double dp = (n[0] * vx[0] + n[1] * vx[1]) * ((n[0] * vp[0] + n[1] * vp[1]) < 0 ? -1.0 : 1.0);

      if (dp < halfPlaneTol)
      {
        if (dp < minProj)
        {
          inside = 0;
          bestP1 = t.PointIds[i];
          bestP2 = t.PointIds[i2];
          bestEdgeOpp = i3;
          minProj = dp;
        }
      }
    } // for each edge

    if (inside) // all edges positive
    {
      return cur;
    }

    if (fabs(minProj) < halfPlaneTol) // on edge
    {
      if (t.Neighbors[bestEdgeOpp] >= 0)
      {
        neiTri = t.Neighbors[bestEdgeOpp];
        edgeV1 = bestP1;
        edgeV2 = bestP2;
        onEdge = true;
      }
      return cur;
    }

    // Walk towards the point.
    vtkIdType nei = t.Neighbors[bestEdgeOpp];
    if (nei < 0 || nei == prev)
    {
      return -1;
    }
    prev = cur;
    cur = nei;
  }
  return -1; // iteration limit
}

//------------------------------------------------------------------------------
// O(1) duplicate check: test the 3 vertices of the containing triangle and
// the 3 opposite vertices of its edge-neighbours.
bool IsDuplicatePoint(const std::vector<Triangle>& topo, const double* coords, vtkIdType tri,
  const double x[3], double tol2)
{
  const Triangle& t = topo[tri];
  for (int j = 0; j < 3; j++)
  {
    const double* vp = &coords[t.PointIds[j] * 3];
    double dx = x[0] - vp[0];
    double dy = x[1] - vp[1];
    if (dx * dx + dy * dy <= tol2)
    {
      return true;
    }
  }
  for (int j = 0; j < 3; j++)
  {
    vtkIdType nei = t.Neighbors[j];
    if (nei < 0)
    {
      continue;
    }
    const Triangle& nt = topo[nei];
    for (int k = 0; k < 3; k++)
    {
      if (nt.PointIds[k] != t.PointIds[0] && nt.PointIds[k] != t.PointIds[1] &&
        nt.PointIds[k] != t.PointIds[2])
      {
        const double* vp = &coords[nt.PointIds[k] * 3];
        double dx = x[0] - vp[0];
        double dy = x[1] - vp[1];
        if (dx * dx + dy * dy <= tol2)
        {
          return true;
        }
        break;
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------------
// Flip the shared edge between two triangles.
// Before: triA = (ptId, p1, p2), triB = (p3, ?, ?) sharing edge (p1,p2).
// After:  triA = (ptId, p3, p2), triB = (ptId, p1, p3).
void FlipEdge(std::vector<Triangle>& topo, vtkIdType triA, vtkIdType triB, vtkIdType ptId,
  vtkIdType p1, vtkIdType p2, vtkIdType p3)
{
  Triangle& A = topo[triA];
  Triangle& B = topo[triB];

  // Find local vertex indices.
  int ai[3] = { 0, 0, 0 }; // [0]=ptId, [1]=p1, [2]=p2
  for (int i = 0; i < 3; i++)
  {
    if (A.PointIds[i] == ptId)
    {
      ai[0] = i;
    }
    else if (A.PointIds[i] == p1)
    {
      ai[1] = i;
    }
    else
    {
      ai[2] = i;
    }
  }
  int bi[3] = { 0, 0, 0 }; // [0]=p3, [1]=p1, [2]=p2
  for (int i = 0; i < 3; i++)
  {
    if (B.PointIds[i] == p3)
    {
      bi[0] = i;
    }
    else if (B.PointIds[i] == p1)
    {
      bi[1] = i;
    }
    else
    {
      bi[2] = i;
    }
  }

  // Save external neighbours.
  vtkIdType extA_opp_p1 = A.Neighbors[ai[1]]; // A across (ptId,p2) opposite p1
  vtkIdType extA_opp_p2 = A.Neighbors[ai[2]]; // A across (ptId,p1) opposite p2
  vtkIdType extB_opp_p1 = B.Neighbors[bi[1]]; // B across (p3,p2) opposite p1
  vtkIdType extB_opp_p2 = B.Neighbors[bi[2]]; // B across (p3,p1) opposite p2

  // New A = (ptId, p3, p2).
  A.PointIds[0] = ptId;
  A.PointIds[1] = p3;
  A.PointIds[2] = p2;
  A.Neighbors[0] = extB_opp_p1; // across (p3,p2) opposite ptId
  A.Neighbors[1] = extA_opp_p1; // across (ptId,p2) opposite p3
  A.Neighbors[2] = triB;        // across (ptId,p3) opposite p2

  // New B = (ptId, p1, p3).
  B.PointIds[0] = ptId;
  B.PointIds[1] = p1;
  B.PointIds[2] = p3;
  B.Neighbors[0] = extB_opp_p2; // across (p1,p3) opposite ptId
  B.Neighbors[1] = triA;        // across (ptId,p3) opposite p1
  B.Neighbors[2] = extA_opp_p2; // across (ptId,p1) opposite p3

  // Update back-pointers in external neighbours.
  if (extB_opp_p1 >= 0)
  {
    Triangle& ext = topo[extB_opp_p1];
    for (int i = 0; i < 3; i++)
    {
      if (ext.Neighbors[i] == triB)
      {
        ext.Neighbors[i] = triA;
        break;
      }
    }
  }
  if (extA_opp_p2 >= 0)
  {
    Triangle& ext = topo[extA_opp_p2];
    for (int i = 0; i < 3; i++)
    {
      if (ext.Neighbors[i] == triA)
      {
        ext.Neighbors[i] = triB;
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
// Iterative Delaunay edge-flip check (replaces the recursive CheckEdge).
// Every triangle on the stack has ptId at PointIds[0]; the edge to check is
// (PointIds[1], PointIds[2]) at Neighbors[0].
void RestoreDelaunay(std::vector<Triangle>& topo, const double* coords, vtkIdType ptId,
  double boundingRadius2, std::vector<vtkIdType>& checkStack)
{
  static const unsigned int MAX_DEPTH = 2500;
  unsigned int depth = 0;
  while (!checkStack.empty() && depth < MAX_DEPTH)
  {
    vtkIdType tri = checkStack.back();
    checkStack.pop_back();
    depth++;

    Triangle& t = topo[tri];
    vtkIdType myP1 = t.PointIds[1];
    vtkIdType myP2 = t.PointIds[2];
    vtkIdType nei = t.Neighbors[0]; // neighbour across (PointIds[1], PointIds[2]) opposite ptId
    if (nei < 0)
    {
      continue;
    }

    // Find opposite vertex p3 in the neighbour.
    const Triangle& nt = topo[nei];
    vtkIdType p3 = -1;
    for (int i = 0; i < 3; i++)
    {
      if (nt.PointIds[i] != myP1 && nt.PointIds[i] != myP2)
      {
        p3 = nt.PointIds[i];
        break;
      }
    }
    if (p3 < 0)
    {
      continue;
    }

    if (IsInsideCircumcircle(&coords[p3 * 3], &coords[ptId * 3], &coords[myP1 * 3],
          &coords[myP2 * 3], boundingRadius2))
    {
      FlipEdge(topo, tri, nei, ptId, myP1, myP2, p3);
      // After flip: tri = (ptId, p3, myP2), nei = (ptId, myP1, p3).
      // Both still have ptId at PointIds[0]. Push both for further checking.
      checkStack.push_back(tri);
      checkStack.push_back(nei);
    }
  }
}

//------------------------------------------------------------------------------
// Split triangle triId (containing point ptId inside) into 3 triangles.
void SplitTriangleInside(
  std::vector<Triangle>& topo, vtkIdType triId, vtkIdType ptId, vtkIdType& t1Id, vtkIdType& t2Id)
{
  // Save original data before potential reallocation.
  vtkIdType a = topo[triId].PointIds[0];
  vtkIdType b = topo[triId].PointIds[1];
  vtkIdType c = topo[triId].PointIds[2];
  vtkIdType nA = topo[triId].Neighbors[0]; // across (b,c) opp a
  vtkIdType nB = topo[triId].Neighbors[1]; // across (a,c) opp b
  vtkIdType nC = topo[triId].Neighbors[2]; // across (a,b) opp c

  t1Id = static_cast<vtkIdType>(topo.size());
  topo.resize(topo.size() + 2);
  t2Id = t1Id + 1;

  // T0 (reuses triId) = (ptId, a, b).
  topo[triId].PointIds[0] = ptId;
  topo[triId].PointIds[1] = a;
  topo[triId].PointIds[2] = b;
  topo[triId].Neighbors[0] = nC;   // across (a,b) opp ptId
  topo[triId].Neighbors[1] = t1Id; // across (ptId,b) opp a
  topo[triId].Neighbors[2] = t2Id; // across (ptId,a) opp b

  // T1 = (ptId, b, c).
  topo[t1Id].PointIds[0] = ptId;
  topo[t1Id].PointIds[1] = b;
  topo[t1Id].PointIds[2] = c;
  topo[t1Id].Neighbors[0] = nA;    // across (b,c) opp ptId
  topo[t1Id].Neighbors[1] = t2Id;  // across (ptId,c) opp b
  topo[t1Id].Neighbors[2] = triId; // across (ptId,b) opp c

  // T2 = (ptId, c, a).
  topo[t2Id].PointIds[0] = ptId;
  topo[t2Id].PointIds[1] = c;
  topo[t2Id].PointIds[2] = a;
  topo[t2Id].Neighbors[0] = nB;    // across (c,a) opp ptId
  topo[t2Id].Neighbors[1] = triId; // across (ptId,a) opp c
  topo[t2Id].Neighbors[2] = t1Id;  // across (ptId,c) opp a

  // Update back-pointers in external neighbours.
  if (nA >= 0)
  {
    for (int i = 0; i < 3; i++)
    {
      if (topo[nA].Neighbors[i] == triId)
      {
        topo[nA].Neighbors[i] = t1Id;
        break;
      }
    }
  }
  if (nB >= 0)
  {
    for (int i = 0; i < 3; i++)
    {
      if (topo[nB].Neighbors[i] == triId)
      {
        topo[nB].Neighbors[i] = t2Id;
        break;
      }
    }
  }
  // nC still points to triId: correct.
}

//------------------------------------------------------------------------------
// Split on the shared edge between tri0Id and neiTriId.
// Creates 4 triangles, all with ptId at PointIds[0].
void SplitTrianglesOnEdge(std::vector<Triangle>& topo, vtkIdType tri0Id, vtkIdType neiTriId,
  vtkIdType ptId, vtkIdType e1, vtkIdType e2, vtkIdType& t2Id, vtkIdType& t3Id)
{
  // p2 = vertex of tri0 not on the shared edge.
  vtkIdType p2 = -1;
  for (int i = 0; i < 3; i++)
  {
    if (topo[tri0Id].PointIds[i] != e1 && topo[tri0Id].PointIds[i] != e2)
    {
      p2 = topo[tri0Id].PointIds[i];
      break;
    }
  }

  // p1 = vertex of neiTri not on the shared edge.
  vtkIdType p1 = -1;
  for (int i = 0; i < 3; i++)
  {
    if (topo[neiTriId].PointIds[i] != e1 && topo[neiTriId].PointIds[i] != e2)
    {
      p1 = topo[neiTriId].PointIds[i];
      break;
    }
  }

  // Save external neighbours from tri0.
  int t0_e1 = -1, t0_e2 = -1;
  for (int i = 0; i < 3; i++)
  {
    if (topo[tri0Id].PointIds[i] == e1)
    {
      t0_e1 = i;
    }
    else if (topo[tri0Id].PointIds[i] == e2)
    {
      t0_e2 = i;
    }
  }
  vtkIdType extA = topo[tri0Id].Neighbors[t0_e1]; // across (p2,e2) opposite e1
  vtkIdType extB = topo[tri0Id].Neighbors[t0_e2]; // across (p2,e1) opposite e2

  // Save external neighbours from neiTri.
  int n_e1 = -1, n_e2 = -1;
  for (int i = 0; i < 3; i++)
  {
    if (topo[neiTriId].PointIds[i] == e1)
    {
      n_e1 = i;
    }
    else if (topo[neiTriId].PointIds[i] == e2)
    {
      n_e2 = i;
    }
  }
  vtkIdType extC = topo[neiTriId].Neighbors[n_e1]; // across (p1,e2) opposite e1
  vtkIdType extD = topo[neiTriId].Neighbors[n_e2]; // across (p1,e1) opposite e2

  // Allocate 2 new triangles.
  t2Id = static_cast<vtkIdType>(topo.size());
  topo.resize(topo.size() + 2);
  t3Id = t2Id + 1;

  // T0 (reuses tri0Id) = (ptId, p2, e1).
  topo[tri0Id].PointIds[0] = ptId;
  topo[tri0Id].PointIds[1] = p2;
  topo[tri0Id].PointIds[2] = e1;
  topo[tri0Id].Neighbors[0] = extB;     // across (p2,e1) opp ptId
  topo[tri0Id].Neighbors[1] = neiTriId; // across (ptId,e1) opp p2, i.e. T1
  topo[tri0Id].Neighbors[2] = t2Id;     // across (ptId,p2) opp e1, i.e. T2

  // T1 (reuses neiTriId) = (ptId, p1, e1).
  topo[neiTriId].PointIds[0] = ptId;
  topo[neiTriId].PointIds[1] = p1;
  topo[neiTriId].PointIds[2] = e1;
  topo[neiTriId].Neighbors[0] = extD;   // across (p1,e1) opp ptId
  topo[neiTriId].Neighbors[1] = tri0Id; // across (ptId,e1) opp p1, i.e. T0
  topo[neiTriId].Neighbors[2] = t3Id;   // across (ptId,p1) opp e1, i.e. T3

  // T2 (new) = (ptId, p2, e2).
  topo[t2Id].PointIds[0] = ptId;
  topo[t2Id].PointIds[1] = p2;
  topo[t2Id].PointIds[2] = e2;
  topo[t2Id].Neighbors[0] = extA;   // across (p2,e2) opp ptId
  topo[t2Id].Neighbors[1] = t3Id;   // across (ptId,e2) opp p2, i.e. T3
  topo[t2Id].Neighbors[2] = tri0Id; // across (ptId,p2) opp e2, i.e. T0

  // T3 (new) = (ptId, p1, e2).
  topo[t3Id].PointIds[0] = ptId;
  topo[t3Id].PointIds[1] = p1;
  topo[t3Id].PointIds[2] = e2;
  topo[t3Id].Neighbors[0] = extC;     // across (p1,e2) opp ptId
  topo[t3Id].Neighbors[1] = t2Id;     // across (ptId,e2) opp p1, i.e. T2
  topo[t3Id].Neighbors[2] = neiTriId; // across (ptId,p1) opp e2, i.e. T1

  // Update back-pointers.
  if (extA >= 0)
  {
    for (int i = 0; i < 3; i++)
    {
      if (topo[extA].Neighbors[i] == tri0Id)
      {
        topo[extA].Neighbors[i] = t2Id;
        break;
      }
    }
  }
  // extB still points to tri0Id: correct.
  if (extC >= 0)
  {
    for (int i = 0; i < 3; i++)
    {
      if (topo[extC].Neighbors[i] == neiTriId)
      {
        topo[extC].Neighbors[i] = t3Id;
        break;
      }
    }
  }
  // extD still points to neiTriId: correct.
}

//------------------------------------------------------------------------------
// To provide a low-cost, simple, pseudo-random traversal of points, we use
// a GCD (greatest common divisor) traversal with ptId = a*idx + b, where
// idx is the index into the points list; a is a coprime factor of npts;
// and b is an initial offset. For further explanation see:
// https://lemire.me/blog/2017/09/18/visiting-all-values-in-an-array-exactly-once-in-random-order.
struct GCDTraversal
{
  vtkIdType NPts;
  vtkIdType Prime;
  vtkIdType Offset;

  // Given the number of points to iterate over, determine one coprime factor
  // a and the offset b. Note that a coprime is guaranteed between [n/2,n) which
  // means the while loop will terminate.
  GCDTraversal(vtkIdType npts)
    : NPts(npts)
  {
    this->Offset = npts / 2; // over the halfway mark, arbitrary
    this->Prime = this->Offset + 1;
    while (vtkMath::ComputeGCD(this->Prime, this->NPts) != 1)
    {
      this->Prime++;
    }
  }
  // Can be optimized to avoid the modulo %, but coded for simplicity
  // since the cost of this operation is minuscule compared to everything
  // else that is going on.
  vtkIdType GetPointId(vtkIdType idx) { return ((this->Prime * idx + this->Offset) % this->NPts); }
};
} // anonymous namespace

vtkStandardNewMacro(vtkDelaunay2D);

//------------------------------------------------------------------------------
// Construct object with Alpha = 0.0; Tolerance = 0.00001; Offset = 1.25;
// BoundingTriangulation turned off.
vtkDelaunay2D::vtkDelaunay2D()
{
  this->Alpha = 0.0;
  this->Tolerance = 0.00001;
  this->BoundingTriangulation = 0;
  this->Offset = 1.0;
  this->RandomPointInsertion = 0;
  this->UseHilbertSorter = 0;
  this->Transform = nullptr;
  this->ProjectionPlaneMode = VTK_DELAUNAY_XY_PLANE;

  // optional 2nd input
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
void vtkDelaunay2D::SetSourceData(vtkPolyData* input)
{
  this->Superclass::SetInputData(1, input);
}

//------------------------------------------------------------------------------
// Specify the input data or filter. New style.
void vtkDelaunay2D::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->Superclass::SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
vtkPolyData* vtkDelaunay2D::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
// Determine whether point x is inside of circumcircle of triangle
// defined by points (x1, x2, x3). Returns non-zero if inside circle.
// (Note that the z-component of the points is ignored.)
int vtkDelaunay2D::InCircle(double x[3], double x1[3], double x2[3], double x3[3])
{
  double radius2, center[2], dist2;

  radius2 = vtkTriangle::Circumcircle(x1, x2, x3, center);

  // Use a sanity check to determine in/out. This is needed in situations
  // where the circumcircle becomes very large due to near-degenerate
  // cases. (Near degenerate cases can emerge when an inserted point is
  // nearly on the edge of triangle.) Note that because of the way a
  // candidate point is identified (via the triangle walk / CheckEdge()) we don't
  // need to worry about which "side" the center of the circumcircle is on as
  // compared to the test point x (they will both be on the same side).
  if (radius2 > this->BoundingRadius2)
  {
    return 1;
  }

  // Check if the point is strictly inside/outside the circumcircle. Using the less than
  // operator enables ordering (and control of diagonals related to) degenerate points.
  dist2 = (x[0] - center[0]) * (x[0] - center[0]) + (x[1] - center[1]) * (x[1] - center[1]);

  // Note: at one time we experimented with std::nextafter() but it seems that it is
  // not always implemented correctly / consistently across platforms, which wreaks
  // havoc during testing (in near-degenerate situations).
  if (dist2 < (0.9999999999 * radius2))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
// Check whether the edge (p1,p2) of triangle tri satisfies the Delaunay
// criterion; if not, swap the edge diagonal.  Used by RecoverEdge during
// constrained-triangulation post-processing (non-recursive, single pass).
bool vtkDelaunay2D::CheckEdge(
  vtkIdType ptId, double x[3], vtkIdType p1, vtkIdType p2, vtkIdType tri)
{
  int i;
  const vtkIdType* pts;
  vtkIdType npts;
  vtkIdType numNei, nei, p3;
  double x1[3], x2[3], x3[3];
  vtkIdType swapTri[3];

  this->GetPoint(p1, x1);
  this->GetPoint(p2, x2);

  vtkNew<vtkIdList> neighbors;
  neighbors->Reserve(2);

  this->Mesh->GetCellEdgeNeighbors(tri, p1, p2, neighbors);
  numNei = neighbors->GetNumberOfIds();

  if (numNei > 0) // i.e., not a boundary edge
  {
    // get neighbor info including opposite point
    nei = neighbors->GetId(0);
    this->Mesh->GetCellPoints(nei, npts, pts);
    for (i = 0; i < 2; i++)
    {
      if (pts[i] != p1 && pts[i] != p2)
      {
        break;
      }
    }
    p3 = pts[i];
    this->GetPoint(p3, x3);

    // see whether point is in circumcircle
    if (this->InCircle(x3, x, x1, x2))
    {
      // swap diagonal
      this->Mesh->RemoveReferenceToCell(p1, tri);
      this->Mesh->RemoveReferenceToCell(p2, nei);
      this->Mesh->ResizeCellList(ptId, 1);
      this->Mesh->AddReferenceToCell(ptId, nei);
      this->Mesh->ResizeCellList(p3, 1);
      this->Mesh->AddReferenceToCell(p3, tri);

      swapTri[0] = ptId;
      swapTri[1] = p3;
      swapTri[2] = p2;
      this->Mesh->ReplaceCell(tri, 3, swapTri);

      swapTri[0] = ptId;
      swapTri[1] = p1;
      swapTri[2] = p3;
      this->Mesh->ReplaceCell(nei, 3, swapTri);

      return true;
    } // in circle
  }   // interior edge

  return false;
}

//------------------------------------------------------------------------------
// 2D Delaunay triangulation. Steps are as follows:
//   1. For each point
//   2. Find triangle point is in
//   3. Create 3 triangles from each edge of triangle that point is in
//   4. Recursively evaluate Delaunay criterion for each edge neighbor
//   5. If criterion not satisfied; swap diagonal
//
int vtkDelaunay2D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* source = nullptr;
  if (sourceInfo)
  {
    source = vtkPolyData::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPoints, i;
  vtkIdType numTriangles = 0;
  vtkIdType ptId;
  vtkIdType p1 = 0;
  vtkIdType p2 = 0;
  vtkIdType p3 = 0;
  vtkPoints* inPoints;
  vtkSmartPointer<vtkPoints> tPoints;
  int ncells;
  const vtkIdType* neiPts;
  const vtkIdType* triPts = nullptr;
  vtkIdType npts = 0;
  vtkIdType pts[3], swapPts[3];
  vtkIdType tri1, tri2;
  double center[3], radius, tol, x[3];
  double n1[3], n2[3];
  int* triUse = nullptr;

  vtkDebugMacro(<< "Generating 2D Delaunay triangulation");

  if (this->Transform && this->BoundingTriangulation)
  {
    vtkWarningMacro(<< "Bounding triangulation cannot be used when an input transform is "
                       "specified.  Output will not contain bounding triangulation.");
  }

  if (this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE && this->BoundingTriangulation)
  {
    vtkWarningMacro(<< "Bounding triangulation cannot be used when the best fitting plane option "
                       "is on.  Output will not contain bounding triangulation.");
  }

  // Initialize; check input
  //
  if ((inPoints = input->GetPoints()) == nullptr)
  {
    vtkDebugMacro("Cannot triangulate; no input points");
    return 1;
  }

  if ((numPoints = inPoints->GetNumberOfPoints()) <= 2)
  {
    vtkDebugMacro("Cannot triangulate; need at least 3 input points");
    return 1;
  }

  vtkNew<vtkIdList> neighbors;
  neighbors->Reserve(2);
  vtkNew<vtkIdList> cells;
  cells->Reserve(64);

  this->NumberOfDuplicatePoints = 0;
  this->NumberOfDegeneracies = 0;

  this->Mesh = vtkSmartPointer<vtkPolyData>::New();

  // If the user specified a transform, apply it to the input data.
  //
  // Only the input points are transformed.  We do not bother
  // transforming the source points (if specified).  The reason is
  // that only the topology of the Source is used during the constrain
  // operation.  The point ids in the Source topology are assumed to
  // reference points in the input. So, when an input transform is
  // used, only the input points are transformed.  We do not bother
  // with transforming the Source points since they are never
  // referenced.
  if (this->Transform)
  {
    tPoints = vtkSmartPointer<vtkPoints>::New();
    this->Transform->TransformPoints(inPoints, tPoints);
  }
  else
  {
    // If the user asked this filter to compute the best fitting plane,
    // proceed to compute the plane and generate a transform that will
    // map the input points into that plane.
    if (this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE)
    {
      this->Transform.TakeReference(vtkDelaunay2D::ComputeBestFittingPlane(input));
      tPoints = vtkSmartPointer<vtkPoints>::New();
      this->Transform->TransformPoints(inPoints, tPoints);
    }
  }

  // Create initial bounding triangulation. Have to create bounding points.
  // Initialize mesh structure.
  //
  vtkPoints* srcPoints = this->Transform ? tPoints.Get() : inPoints;

  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPoints + 8); // input points + 8 bounding points

  const double* bounds = srcPoints->GetBounds();
  center[0] = (bounds[0] + bounds[1]) / 2.0;
  center[1] = (bounds[2] + bounds[3]) / 2.0;
  center[2] = (bounds[4] + bounds[5]) / 2.0;
  tol = input->GetLength();
  radius = this->Offset * tol;
  this->BoundingRadius2 = 4 * radius * radius; // use (2*r)**2
  tol *= this->Tolerance;

  // Add the eight bounding points to the end of the points list.
  for (ptId = 0; ptId < 8; ptId++)
  {
    x[0] = center[0] + radius * cos(ptId * vtkMath::RadiansFromDegrees(45.0));
    x[1] = center[1] + radius * sin(ptId * vtkMath::RadiansFromDegrees(45.0));
    x[2] = center[2];
    points->SetPoint(numPoints + ptId, x);
  }

  // Copy the input points into the working array (InsertTuples performs the
  // type conversion when the input precision differs), then cache the flat
  // double-precision buffer for direct access in the tight insertion loop.
  points->GetData()->InsertTuples(0, numPoints, 0, srcPoints->GetData());
  this->Points = static_cast<vtkDoubleArray*>(points->GetData())->GetPointer(0);

  // ================================================================
  // FAST INSERTION PATH: lightweight topology
  // ================================================================

  // Initialise fast triangle mesh with 6 bounding triangles.
  const vtkIdType B = numPoints; // base index for bounding points
  const size_t meshReserve = static_cast<size_t>(3) * numPoints + 10;
  std::vector<Triangle> topo;
  topo.reserve(meshReserve);
  topo.resize(6);

  // tri 0: (B0, B1, B2)   n: {boundary, tri4, boundary}
  topo[0].PointIds[0] = B;
  topo[0].PointIds[1] = B + 1;
  topo[0].PointIds[2] = B + 2;
  topo[0].Neighbors[0] = -1;
  topo[0].Neighbors[1] = 4;
  topo[0].Neighbors[2] = -1;
  // tri 1: (B2, B3, B4)   n: {boundary, tri5, boundary}
  topo[1].PointIds[0] = B + 2;
  topo[1].PointIds[1] = B + 3;
  topo[1].PointIds[2] = B + 4;
  topo[1].Neighbors[0] = -1;
  topo[1].Neighbors[1] = 5;
  topo[1].Neighbors[2] = -1;
  // tri 2: (B4, B5, B6)   n: {boundary, tri5, boundary}
  topo[2].PointIds[0] = B + 4;
  topo[2].PointIds[1] = B + 5;
  topo[2].PointIds[2] = B + 6;
  topo[2].Neighbors[0] = -1;
  topo[2].Neighbors[1] = 5;
  topo[2].Neighbors[2] = -1;
  // tri 3: (B6, B7, B0)   n: {boundary, tri4, boundary}
  topo[3].PointIds[0] = B + 6;
  topo[3].PointIds[1] = B + 7;
  topo[3].PointIds[2] = B;
  topo[3].Neighbors[0] = -1;
  topo[3].Neighbors[1] = 4;
  topo[3].Neighbors[2] = -1;
  // tri 4: (B0, B2, B6)   n: {tri5, tri3, tri0}
  topo[4].PointIds[0] = B;
  topo[4].PointIds[1] = B + 2;
  topo[4].PointIds[2] = B + 6;
  topo[4].Neighbors[0] = 5;
  topo[4].Neighbors[1] = 3;
  topo[4].Neighbors[2] = 0;
  // tri 5: (B2, B4, B6)   n: {tri2, tri4, tri1}
  topo[5].PointIds[0] = B + 2;
  topo[5].PointIds[1] = B + 4;
  topo[5].PointIds[2] = B + 6;
  topo[5].Neighbors[0] = 2;
  topo[5].Neighbors[1] = 4;
  topo[5].Neighbors[2] = 1;

  const double* coords = this->Points;
  double tol2 = tol * tol;
  vtkIdType lastTri = 4; // start from a central triangle
  std::vector<vtkIdType> checkStack;
  checkStack.reserve(64);

  // Determine the point insertion order. Hilbert-curve sorting makes
  // successive insertions spatially local, which shortens the walk to
  // locate the containing triangle; the GCD pseudo-random traversal
  // improves numerics on structured inputs.
  vtkSmartPointer<vtkIdList> hilbertOrder;
  if (this->UseHilbertSorter)
  {
    vtkNew<vtkPolyData> sorterInput;
    sorterInput->SetPoints(srcPoints);
    vtkNew<vtkHilbertCurveSorter> sorter;
    sorter->SetInputData(sorterInput);
    sorter->ComputePermutationOnlyOn();
    sorter->Update();
    hilbertOrder = sorter->GetPermutation();
  }
  GCDTraversal gcdIter(numPoints);

  // For each point; find triangle containing point. Then evaluate
  // neighbouring triangles for Delaunay criterion. Triangles that do not
  // satisfy criterion have their edges swapped.
  for (vtkIdType idx = 0; idx < numPoints; idx++)
  {
    if (hilbertOrder)
    {
      ptId = hilbertOrder->GetId(idx);
    }
    else if (this->RandomPointInsertion)
    {
      ptId = gcdIter.GetPointId(idx);
    }
    else
    {
      ptId = idx;
    }
    this->GetPoint(ptId, x);

    bool onEdge = false;
    bool isDuplicate = false;
    vtkIdType neiTri = -1, edgeV1 = 0, edgeV2 = 0;

    vtkIdType containTri = ::FindContainingTriangle(
      topo, coords, x, lastTri, tol, onEdge, neiTri, edgeV1, edgeV2, isDuplicate);

    if (isDuplicate)
    {
      this->NumberOfDuplicatePoints++;
    }
    else if (containTri < 0)
    {
      this->NumberOfDegeneracies++;
      lastTri = 0;
    }
    else if (::IsDuplicatePoint(topo, coords, containTri, x, tol2))
    {
      this->NumberOfDuplicatePoints++;
    }
    else
    {
      lastTri = containTri;
      checkStack.clear();

      if (!onEdge) // point inside triangle
      {
        vtkIdType t1Id, t2Id;
        ::SplitTriangleInside(topo, containTri, ptId, t1Id, t2Id);
        checkStack.push_back(containTri);
        checkStack.push_back(t1Id);
        checkStack.push_back(t2Id);
      }
      else // point on triangle edge
      {
        vtkIdType t2Id, t3Id;
        ::SplitTrianglesOnEdge(topo, containTri, neiTri, ptId, edgeV1, edgeV2, t2Id, t3Id);
        checkStack.push_back(containTri);
        checkStack.push_back(neiTri);
        checkStack.push_back(t2Id);
        checkStack.push_back(t3Id);
      }

      ::RestoreDelaunay(topo, coords, ptId, this->BoundingRadius2, checkStack);
    }

    if (!(idx % 1000))
    {
      vtkDebugMacro(<< "point #" << ptId);
      this->UpdateProgress(static_cast<double>(idx) / numPoints);
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
    vtkDebugMacro(<< this->NumberOfDegeneracies
                  << " degenerate triangles encountered, mesh quality suspect");
  }

  // Convert fast mesh topology to vtkPolyData for post-processing
  // (boundary recovery, alpha criterion, connectivity fix).  The cell
  // array is assembled in parallel from the flat triangle array.
  {
    const vtkIdType numTris = static_cast<vtkIdType>(topo.size());
    vtkNew<vtkIdTypeArray> offsets;
    offsets->SetNumberOfValues(numTris + 1);
    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfValues(3 * numTris);
    vtkIdType* offsetsPtr = offsets->GetPointer(0);
    vtkIdType* connectivityPtr = connectivity->GetPointer(0);
    vtkSMPTools::For(0, numTris,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType triId = begin; triId < end; triId++)
        {
          offsetsPtr[triId] = 3 * triId;
          connectivityPtr[3 * triId] = topo[triId].PointIds[0];
          connectivityPtr[3 * triId + 1] = topo[triId].PointIds[1];
          connectivityPtr[3 * triId + 2] = topo[triId].PointIds[2];
        }
      });
    offsetsPtr[numTris] = 3 * numTris;

    vtkNew<vtkCellArray> triangles;
    triangles->SetData(offsets, connectivity);
    this->Mesh->SetPoints(points);
    this->Mesh->SetPolys(triangles);
    this->Mesh->EditableOn();
    this->Mesh->BuildLinks();
  }
  // Free topo memory.
  {
    std::vector<Triangle>().swap(topo);
  }

  // ================================================================
  // POST-PROCESSING (unchanged from original algorithm)
  // ================================================================

  // Finish up by recovering the boundary, or deleting all triangles connected
  // to the bounding triangulation points or not satisfying alpha criterion,
  if (!this->BoundingTriangulation || this->Alpha > 0.0 || source)
  {
    numTriangles = this->Mesh->GetNumberOfCells();
    if (source)
    {
      triUse = this->RecoverBoundary(source);
    }
    else
    {
      triUse = new int[numTriangles];
      vtkSMPTools::Fill(triUse, triUse + numTriangles, 1);
    }
  }

  // Delete triangles connected to the eight boundary points (if not desired)
  if (!this->BoundingTriangulation)
  {
    for (ptId = numPoints; ptId < (numPoints + 8); ptId++)
    {
      this->Mesh->GetPointCells(ptId, cells);
      ncells = cells->GetNumberOfIds();
      for (i = 0; i < ncells; i++)
      {
        triUse[cells->GetId(i)] = 0; // mark as deleted
      }
    }
  }

  // If non-zero alpha value, then figure out which parts of mesh are
  // contained within alpha radius.
  //
  if (this->Alpha > 0.0)
  {
    double alpha2 = this->Alpha * this->Alpha;
    double x1[3], x2[3], x3[3];
    double xx1[3], xx2[3], xx3[3];
    vtkIdType neighbor;

    vtkNew<vtkCellArray> alphaVerts;
    alphaVerts->AllocateEstimate(numPoints, 1);
    vtkNew<vtkCellArray> alphaLines;
    alphaLines->AllocateEstimate(numPoints, 2);

    std::vector<char> pointUse(numPoints + 8, 0);

    // traverse all triangles; evaluating Delaunay criterion
    for (i = 0; i < numTriangles; i++)
    {
      if (triUse[i] == 1)
      {
        this->Mesh->GetCellPoints(i, npts, triPts);

        // if any point is one of the bounding points that was added
        // at the beginning of the algorithm, then grab the points
        // from the variable "points" (this list has the boundary
        // points and the original points have been transformed by the
        // input transform).  if none of the points are bounding points,
        // then grab the points from the variable "inPoints" so the alpha
        // criterion is applied in the nontransformed space.
        if (triPts[0] < numPoints && triPts[1] < numPoints && triPts[2] < numPoints)
        {
          inPoints->GetPoint(triPts[0], x1);
          inPoints->GetPoint(triPts[1], x2);
          inPoints->GetPoint(triPts[2], x3);
        }
        else
        {
          points->GetPoint(triPts[0], x1);
          points->GetPoint(triPts[1], x2);
          points->GetPoint(triPts[2], x3);
        }

        // evaluate the alpha criterion in 3D
        vtkTriangle::ProjectTo2D(x1, x2, x3, xx1, xx2, xx3);
        if (vtkTriangle::Circumcircle(xx1, xx2, xx3, center) > alpha2)
        {
          triUse[i] = 0;
        }
        else
        {
          for (int j = 0; j < 3; j++)
          {
            pointUse[triPts[j]] = 1;
          }
        }
      } // if non-deleted triangle
    }   // for all triangles

    // traverse all edges see whether we need to create some
    vtkNew<vtkCellArray> triangles;
    triangles->DeepCopy(this->Mesh->GetPolys());
    vtkIdType cellId, numNei, ap1, ap2;
    for (cellId = 0, triangles->InitTraversal(); triangles->GetNextCell(npts, triPts); cellId++)
    {
      if (!triUse[cellId])
      {
        for (i = 0; i < npts; i++)
        {
          ap1 = triPts[i];
          ap2 = triPts[(i + 1) % npts];

          if (this->BoundingTriangulation || (ap1 < numPoints && ap2 < numPoints))
          {
            this->Mesh->GetCellEdgeNeighbors(cellId, ap1, ap2, neighbors);
            numNei = neighbors->GetNumberOfIds();

            if (numNei < 1 || ((neighbor = neighbors->GetId(0)) > cellId && !triUse[neighbor]))
            { // see whether edge is shorter than Alpha

              // same argument as above, if one is a boundary point, get
              // it using this->GetPoint() which are transformed points. if
              // neither of the points are boundary points, get the from
              // inPoints (untransformed points) so alpha comparison done
              // untransformed space
              if (ap1 < numPoints && ap2 < numPoints)
              {
                inPoints->GetPoint(ap1, x1);
                inPoints->GetPoint(ap2, x2);
              }
              else
              {
                this->GetPoint(ap1, x1);
                this->GetPoint(ap2, x2);
              }
              if ((vtkMath::Distance2BetweenPoints(x1, x2) * 0.25) <= alpha2)
              {
                pointUse[ap1] = 1;
                pointUse[ap2] = 1;
                pts[0] = ap1;
                pts[1] = ap2;
                alphaLines->InsertNextCell(2, pts);
              } // if passed test
            }   // test edge
          }     // if valid edge
        }       // for all edges of this triangle
      }         // if triangle not output
    }           // for all triangles

    // traverse all points, create vertices if none used
    for (ptId = 0; ptId < (numPoints + 8); ptId++)
    {
      if ((ptId < numPoints || this->BoundingTriangulation) && !pointUse[ptId])
      {
        pts[0] = ptId;
        alphaVerts->InsertNextCell(1, pts);
      }
    }

    // update output
    output->SetVerts(alphaVerts);
    output->SetLines(alphaLines);
  }

  // The code below fixes a bug reported by Gilles Rougeron.
  // Some input points were not connected in the output triangulation.
  // The cause was that those points were only connected to triangles
  // scheduled for removal (i.e. triangles connected to the boundary).
  //
  // We wrote the following fix: swap edges so the unconnected points
  // become connected to new triangles not scheduled for removal.
  // We only applies if:
  // - the bounding triangulation must be deleted
  //   (BoundingTriangulation == OFF)
  // - alpha spheres are not used (Alpha == 0.0)
  // - the triangulation is not constrained (source == nullptr)

  if (!this->BoundingTriangulation && this->Alpha == 0.0 && !source)
  {
    bool isConnected;
    vtkIdType numSwaps = 0;

    for (ptId = 0; ptId < numPoints; ptId++)
    {
      // check if point is only connected to triangles scheduled for
      // removal
      this->Mesh->GetPointCells(ptId, cells);
      ncells = cells->GetNumberOfIds();

      isConnected = false;

      for (i = 0; i < ncells; i++)
      {
        if (triUse[cells->GetId(i)])
        {
          isConnected = true;
          break;
        }
      }

      // this point will be connected in the output
      if (isConnected)
      {
        // point is connected: continue
        continue;
      }

      // This point is only connected to triangles scheduled for removal.
      // Therefore it will not be connected in the output triangulation.
      // Let's swap edges to create a triangle with 3 inner points.
      // - inner points have an id < numPoints
      // - boundary point ids are, numPoints <= id < numPoints+8.

      // visit every edge connected to that point.
      // check the 2 triangles touching at that edge.
      // if one triangle is connected to 2 non-boundary points

      for (i = 0; i < ncells; i++)
      {
        tri1 = cells->GetId(i);
        this->Mesh->GetCellPoints(tri1, npts, triPts);

        if (triPts[0] == ptId)
        {
          p1 = triPts[1];
          p2 = triPts[2];
        }
        else if (triPts[1] == ptId)
        {
          p1 = triPts[2];
          p2 = triPts[0];
        }
        else
        {
          p1 = triPts[0];
          p2 = triPts[1];
        }

        // if both p1 & p2 are boundary points,
        // we skip them.
        if (p1 >= numPoints && p2 >= numPoints)
        {
          continue;
        }

        vtkDebugMacro(
          "tri " << tri1 << " [" << triPts[0] << " " << triPts[1] << " " << triPts[2] << "]");

        vtkDebugMacro("edge [" << p1 << " " << p2 << "] non-boundary");

        // get the triangle sharing edge [p1 p2] with tri1
        this->Mesh->GetCellEdgeNeighbors(tri1, p1, p2, neighbors);

        // Since p1 or p2 is not on the boundary,
        // the neighbor triangle should exist.
        // If more than one neighbor triangle exist,
        // the edge is non-manifold.
        if (neighbors->GetNumberOfIds() != 1)
        {
          vtkErrorMacro("ERROR: Edge [" << p1 << " " << p2 << "] is non-manifold!!!");
          return 0;
        }

        tri2 = neighbors->GetId(0);

        // get the 3 points of the neighbor triangle
        this->Mesh->GetCellPoints(tri2, npts, neiPts);

        vtkDebugMacro(
          "triangle " << tri2 << " [" << neiPts[0] << " " << neiPts[1] << " " << neiPts[2] << "]");

        // locate the point different from p1 and p2
        if (neiPts[0] != p1 && neiPts[0] != p2)
        {
          p3 = neiPts[0];
        }
        else if (neiPts[1] != p1 && neiPts[1] != p2)
        {
          p3 = neiPts[1];
        }
        else
        {
          p3 = neiPts[2];
        }

        vtkDebugMacro("swap [" << p1 << " " << p2 << "] and [" << ptId << " " << p3 << "]");

        // create the two new triangles.
        // we just need to replace their pt ids.
        pts[0] = ptId;
        pts[1] = p1;
        pts[2] = p3;
        swapPts[0] = ptId;
        swapPts[1] = p3;
        swapPts[2] = p2;

        vtkDebugMacro("candidate tri1 " << tri1 << " [" << pts[0] << " " << pts[1] << " " << pts[2]
                                        << "]"
                                        << " triUse " << triUse[tri1]);

        vtkDebugMacro("candidate tri2 " << tri2 << " [" << swapPts[0] << " " << swapPts[1] << " "
                                        << swapPts[2] << "]"
                                        << "triUse " << triUse[tri2]);

        // compute the normal for the 2 candidate triangles
        vtkTriangle::ComputeNormal(points, 3, pts, n1);
        vtkTriangle::ComputeNormal(points, 3, swapPts, n2);

        // the normals must be along the same direction,
        // or one triangle is upside down.
        if (vtkMath::Dot(n1, n2) < 0.0)
        {
          // do not swap diagonal
          continue;
        }

        // swap edge [p1 p2] and diagonal [ptId p3]
        this->Mesh->RemoveReferenceToCell(p1, tri2);
        this->Mesh->RemoveReferenceToCell(p2, tri1);
        this->Mesh->ResizeCellList(ptId, 1);
        this->Mesh->ResizeCellList(p3, 1);
        this->Mesh->AddReferenceToCell(ptId, tri2);
        this->Mesh->AddReferenceToCell(p3, tri1);

        // it's ok to swap the diagonal
        this->Mesh->ReplaceCell(tri1, 3, pts);
        this->Mesh->ReplaceCell(tri2, 3, swapPts);

        triUse[tri1] = (p1 < numPoints && p3 < numPoints);
        triUse[tri2] = (p3 < numPoints && p2 < numPoints);

        vtkDebugMacro("replace tri1 " << tri1 << " [" << pts[0] << " " << pts[1] << " " << pts[2]
                                      << "]"
                                      << " triUse " << triUse[tri1]);

        vtkDebugMacro("replace tri2 " << tri2 << " [" << swapPts[0] << " " << swapPts[1] << " "
                                      << swapPts[2] << "]"
                                      << " triUse " << triUse[tri2]);

        // update the 'scheduled for removal' flag of the first triangle.
        // The second triangle was not scheduled for removal anyway.
        numSwaps++;
        vtkDebugMacro("numSwaps " << numSwaps);
      }
    }
    vtkDebugMacro("numSwaps " << numSwaps);
    (void)numSwaps;
  }

  // Update output; free up supporting data structures.
  //
  if (this->BoundingTriangulation && !this->Transform)
  {
    output->SetPoints(points);
  }
  else
  {
    output->SetPoints(inPoints);
    output->GetPointData()->PassData(input->GetPointData());
  }

  if (this->Alpha <= 0.0 && this->BoundingTriangulation && !source)
  {
    output->SetPolys(this->Mesh->GetPolys());
  }
  else
  {
    vtkNew<vtkCellArray> alphaTriangles;
    alphaTriangles->AllocateEstimate(numTriangles, 3);
    const vtkIdType* alphaTriPts;

    for (i = 0; i < numTriangles; i++)
    {
      if (triUse[i])
      {
        this->Mesh->GetCellPoints(i, npts, alphaTriPts);
        alphaTriangles->InsertNextCell(3, alphaTriPts);
      }
    }
    output->SetPolys(alphaTriangles);
    delete[] triUse;
  }

  // Clear out the mesh
  this->Mesh = nullptr;

  // If the best fitting option was ON, then the current transform
  // is the one that was computed internally. We must now destroy it.
  this->Transform = nullptr;

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
// Methods used to recover edges. Uses lines and polygons to determine boundary
// and inside/outside.
//
// Only the topology of the Source is used during the constrain operation.
// The point ids in the Source topology are assumed to reference points
// in the input. So, when an input transform is used, only the input points
// are transformed.  We do not bother with transforming the Source points
// since they are never referenced.
int* vtkDelaunay2D::RecoverBoundary(vtkPolyData* source)
{
  vtkCellArray* lines = source->GetLines();
  vtkCellArray* polys = source->GetPolys();
  const vtkIdType* pts = nullptr;
  vtkIdType npts = 0;
  vtkIdType i, p1, p2;
  int* triUse;

  source->BuildLinks();

  // Recover the edges of the mesh
  for (lines->InitTraversal(); lines->GetNextCell(npts, pts);)
  {
    for (i = 0; i < (npts - 1); i++)
    {
      p1 = pts[i];
      p2 = pts[i + 1];
      if (!this->Mesh->IsEdge(p1, p2))
      {
        this->RecoverEdge(source, p1, p2);
      }
    }
  }

  // Recover the enclosed regions (polygons) of the mesh
  for (polys->InitTraversal(); polys->GetNextCell(npts, pts);)
  {
    for (i = 0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i + 1) % npts];
      if (!this->Mesh->IsEdge(p1, p2))
      {
        this->RecoverEdge(source, p1, p2);
      }
    }
  }

  // Generate inside/outside marks on mesh
  int numTriangles = this->Mesh->GetNumberOfCells();
  triUse = new int[numTriangles];
  vtkSMPTools::Fill(triUse, triUse + numTriangles, 1);

  // Use any polygons to mark inside and outside. (Note that if an edge was not
  // recovered, we're going to have a problem.) The first polygon is assumed to
  // define the outside of the polygon; additional polygons carve out inside
  // holes.
  this->FillPolygons(polys, triUse);

  return triUse;
}

//------------------------------------------------------------------------------
// Method attempts to recover an edge by retriangulating mesh around the edge.
// What we do is identify a "submesh" of triangles that includes the edge to recover.
// Then we split the submesh in two with the recovered edge, and triangulate each of
// the two halves. If any part of this fails, we leave things alone.
int vtkDelaunay2D::RecoverEdge(vtkPolyData* source, vtkIdType p1, vtkIdType p2)
{
  vtkIdType cellId = 0;
  int i, j, k;
  double p1X[3], p2X[3], xyNormal[3], splitNormal[3], p21[3];
  double x1[3], x2[3], sepNormal[3], v21[3];
  int ncells, v1 = 0, v2 = 0, signX1 = 0, signX2, signP1, signP2;
  const vtkIdType* pts;
  vtkIdType *leftTris, *rightTris, npts, numRightTris, numLeftTris;
  int success = 0, nbPts;

  vtkNew<vtkIdList> cells;
  cells->Reserve(64);
  vtkNew<vtkIdList> tris;
  tris->Reserve(64);
  vtkNew<vtkPolygon> rightPoly;
  vtkNew<vtkPolygon> leftPoly;
  vtkIdList* leftChain = leftPoly->GetPointIds();
  vtkIdList* rightChain = rightPoly->GetPointIds();
  vtkPoints* leftChainX = leftPoly->GetPoints();
  vtkPoints* rightChainX = rightPoly->GetPoints();
  vtkNew<vtkIdList> neis;
  neis->Reserve(4);
  vtkSmartPointer<vtkIdList> rightPtIds = vtkSmartPointer<vtkIdList>::New();
  rightPtIds->Reserve(64);
  vtkSmartPointer<vtkIdList> leftPtIds = vtkSmartPointer<vtkIdList>::New();
  leftPtIds->Reserve(64);
  vtkNew<vtkPoints> rightTriPts;
  rightTriPts->Reserve(64);
  vtkNew<vtkPoints> leftTriPts;
  leftTriPts->Reserve(64);

  // Container for the edges (2 ids in a set, the order does not matter) we won't check
  std::set<std::set<vtkIdType>> polysEdges;
  // Container for the cells & point ids for the edge that need to be checked
  std::vector<vtkIdType> newEdges;

  // Compute a split plane along (p1,p2) and parallel to the z-axis.
  //
  this->GetPoint(p1, p1X);
  p1X[2] = 0.0; // split plane point
  this->GetPoint(p2, p2X);
  p2X[2] = 0.0; // split plane point
  xyNormal[0] = xyNormal[1] = 0.0;
  xyNormal[2] = 1.0;
  for (i = 0; i < 3; i++)
  {
    p21[i] = p2X[i] - p1X[i]; // working in x-y plane
  }

  vtkMath::Cross(p21, xyNormal, splitNormal);
  if (vtkMath::Normalize(splitNormal) == 0.0)
  { // Usually means coincident points
    goto FAILURE;
  }

  // Identify a triangle connected to the point p1 containing a portion of the edge.
  //
  this->Mesh->GetPointCells(p1, cells);
  ncells = cells->GetNumberOfIds();
  for (i = 0; i < ncells; i++)
  {
    cellId = cells->GetId(i);
    this->Mesh->GetCellPoints(cellId, npts, pts);
    for (j = 0; j < 3; j++)
    {
      if (pts[j] == p1)
      {
        break;
      }
    }
    v1 = pts[(j + 1) % 3];
    v2 = pts[(j + 2) % 3];
    this->GetPoint(v1, x1);
    x1[2] = 0.0;
    this->GetPoint(v2, x2);
    x2[2] = 0.0;
    signX1 = (vtkPlane::Evaluate(splitNormal, p1X, x1) > 0.0 ? 1 : -1);
    signX2 = (vtkPlane::Evaluate(splitNormal, p1X, x2) > 0.0 ? 1 : -1);
    if (signX1 != signX2) // points of triangle on either side of edge
    {
      // determine if edge separates p1 from p2 - then we've found triangle
      v21[0] = x2[0] - x1[0]; // working in x-y plane
      v21[1] = x2[1] - x1[1];
      v21[2] = 0.0;

      vtkMath::Cross(v21, xyNormal, sepNormal);
      if (vtkMath::Normalize(sepNormal) == 0.0)
      { // bad mesh
        goto FAILURE;
      }

      signP1 = (vtkPlane::Evaluate(sepNormal, x1, p1X) > 0.0 ? 1 : -1);
      signP2 = (vtkPlane::Evaluate(sepNormal, x1, p2X) > 0.0 ? 1 : -1);
      if (signP1 != signP2) // is a separation line
      {
        break;
      }
    }
  } // for all cells

  if (i >= ncells)
  { // something is really screwed up
    goto FAILURE;
  }

  // We found initial triangle; begin to track triangles containing edge. Also,
  // the triangle defines the beginning of two "chains" which form a boundary of
  // enclosing triangles around the edge. Create the two chains (from p1 to p2).
  // (The chains are actually defining two polygons on either side of the edge.)
  //
  tris->InsertId(0, cellId);
  rightChain->InsertId(0, p1);
  rightChainX->InsertPoint(0, p1X);
  leftChain->InsertId(0, p1);
  leftChainX->InsertPoint(0, p1X);
  if (signX1 > 0)
  {
    rightChain->InsertId(1, v1);
    rightChainX->InsertPoint(1, x1);
    leftChain->InsertId(1, v2);
    leftChainX->InsertPoint(1, x2);
  }
  else
  {
    leftChain->InsertId(1, v1);
    leftChainX->InsertPoint(1, x1);
    rightChain->InsertId(1, v2);
    rightChainX->InsertPoint(1, x2);
  }

  // Walk along triangles (edge neighbors) towards point p2.
  while (v1 != p2)
  {
    this->Mesh->GetCellEdgeNeighbors(cellId, v1, v2, neis);
    if (neis->GetNumberOfIds() != 1)
    { // Mesh is folded or degenerate
      goto FAILURE;
    }
    cellId = neis->GetId(0);
    tris->InsertNextId(cellId);
    this->Mesh->GetCellPoints(cellId, npts, pts);
    for (j = 0; j < 3; j++)
    {
      if (pts[j] != v1 && pts[j] != v2)
      { // found point opposite current edge (v1,v2)
        if (pts[j] == p2)
        {
          v1 = p2; // this will cause the walk to stop
          rightChain->InsertNextId(p2);
          rightChainX->InsertNextPoint(p2X);
          leftChain->InsertNextId(p2);
          leftChainX->InsertNextPoint(p2X);
        }
        else
        { // keep walking
          this->GetPoint(pts[j], x1);
          x1[2] = 0.0;
          if (vtkPlane::Evaluate(splitNormal, p1X, x1) > 0.0)
          {
            v1 = pts[j];
            rightChain->InsertNextId(v1);
            rightChainX->InsertNextPoint(x1);
          }
          else
          {
            v2 = pts[j];
            leftChain->InsertNextId(v2);
            leftChainX->InsertNextPoint(x1);
          }
        }
        break;
      } // else found opposite point
    }   // for all points in triangle
  }     // while walking

  // Fetch the left & right polygons edges
  nbPts = rightPoly->GetPointIds()->GetNumberOfIds();
  for (i = 0; i < nbPts; i++)
  {
    std::set<vtkIdType> edge;
    edge.insert(rightPoly->GetPointId(i));
    edge.insert(rightPoly->GetPointId((i + 1) % nbPts));
    polysEdges.insert(edge);
  }
  nbPts = leftPoly->GetPointIds()->GetNumberOfIds();
  for (i = 0; i < nbPts; i++)
  {
    std::set<vtkIdType> edge;
    edge.insert(leftPoly->GetPointId(i));
    edge.insert(leftPoly->GetPointId((i + 1) % nbPts));
    polysEdges.insert(edge);
  }

  // Now that the to chains are formed, each chain forms a polygon (along with
  // the edge (p1,p2)) that requires triangulation. If we can successfully
  // triangulate the two polygons, we will delete the triangles contained within
  // the chains and replace them with the new triangulation.
  //
  success = 1;
  success &= (rightPoly->BoundedTriangulate(rightPtIds, this->Tolerance));
  {
    vtkNew<vtkIdList> ids;
    ids->Reserve(64);
    for (i = 0; i < rightPtIds->GetNumberOfIds(); i++)
    {
      ids->InsertId(i, rightPoly->PointIds->GetId(rightPtIds->GetId(i)));
    }
    rightPtIds = ids;
  }
  numRightTris = rightPtIds->GetNumberOfIds() / 3;

  success &= (leftPoly->BoundedTriangulate(leftPtIds, this->Tolerance));
  {
    vtkNew<vtkIdList> ids;
    ids->Reserve(64);
    for (i = 0; i < leftPtIds->GetNumberOfIds(); i++)
    {
      ids->InsertId(i, leftPoly->PointIds->GetId(leftPtIds->GetId(i)));
    }
    leftPtIds = ids;
  }
  numLeftTris = leftPtIds->GetNumberOfIds() / 3;

  if (!success)
  { // polygons on either side of edge are poorly shaped
    goto FAILURE;
  }

  // Okay, delete the old triangles and replace them with new ones. There should be
  // the same number of new triangles as old ones.
  leftTris = leftPtIds->GetPointer(0);
  for (j = i = 0; i < numLeftTris; i++, j++, leftTris += 3)
  {
    cellId = tris->GetId(j);
    this->Mesh->RemoveCellReference(cellId);
    for (k = 0; k < 3; k++)
    { // allocate new space for cell lists
      this->Mesh->ResizeCellList(leftTris[k], 1);
    }
    this->Mesh->ReplaceLinkedCell(cellId, 3, leftTris);

    // Check if the added triangle contains edges which are not in the polygon edges set
    for (int e = 0; e < 3; e++)
    {
      vtkIdType ep1 = leftTris[e];
      vtkIdType ep2 = leftTris[(e + 1) % 3];
      vtkIdType ep3 = leftTris[(e + 2) % 3];
      // Make sure we won't alter a constrained edge
      if (!source->IsEdge(ep1, ep2) && !source->IsEdge(ep2, ep3) && !source->IsEdge(ep3, ep1))
      {
        std::set<vtkIdType> edge;
        edge.insert(ep1);
        edge.insert(ep2);
        if (polysEdges.find(edge) == polysEdges.end())
        {
          // Add this new edge and remember current triangle and third point ids too
          newEdges.push_back(cellId);
          newEdges.push_back(ep1);
          newEdges.push_back(ep2);
          newEdges.push_back(ep3);
        }
      }
    }
  }

  rightTris = rightPtIds->GetPointer(0);
  for (i = 0; i < numRightTris; i++, j++, rightTris += 3)
  {
    cellId = tris->GetId(j);
    this->Mesh->RemoveCellReference(cellId);
    for (k = 0; k < 3; k++)
    { // allocate new space for cell lists
      this->Mesh->ResizeCellList(rightTris[k], 1);
    }
    this->Mesh->ReplaceLinkedCell(cellId, 3, rightTris);

    // Check if the added triangle contains edges which are not in the polygon edges set
    for (int e = 0; e < 3; e++)
    {
      vtkIdType ep1 = rightTris[e];
      vtkIdType ep2 = rightTris[(e + 1) % 3];
      vtkIdType ep3 = rightTris[(e + 2) % 3];
      // Make sure we won't alter a constrained edge
      if (!source->IsEdge(ep1, ep2) && !source->IsEdge(ep2, ep3) && !source->IsEdge(ep3, ep1))
      {
        std::set<vtkIdType> edge;
        edge.insert(ep1);
        edge.insert(ep2);
        if (polysEdges.find(edge) == polysEdges.end())
        {
          // Add this new edge and remember current triangle and third point ids too
          newEdges.push_back(cellId);
          newEdges.push_back(ep1);
          newEdges.push_back(ep2);
          newEdges.push_back(ep3);
        }
      }
    }
  }

  j = static_cast<int>(newEdges.size()) / 4;

  // Now check the new suspicious edges
  for (i = 0; i < j; i++)
  {
    vtkIdType* v = &newEdges[4 * i];
    double x[3];
    this->GetPoint(v[3], x);
    if (this->CheckEdge(v[3], x, v[1], v[2], v[0]))
    {
      // Flipping an edge renders triangle and edge IDs in newEdges invalid
      break;
    }
  }

FAILURE:
  return success;
}

//------------------------------------------------------------------------------
void vtkDelaunay2D::FillPolygons(vtkCellArray* polys, int* triUse)
{
  vtkIdType p1, p2, j, kk;
  int i, k;
  const vtkIdType* pts = nullptr;
  const vtkIdType* triPts;
  vtkIdType npts = 0;
  vtkIdType numPts;
  static double xyNormal[3] = { 0.0, 0.0, 1.0 };
  double negDir[3], x21[3], x1[3], x2[3], x[3];
  vtkNew<vtkIdList> neis;
  vtkIdType cellId, numNeis;
  vtkSmartPointer<vtkIdList> tmpFront;
  vtkSmartPointer<vtkIdList> currentFront = vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkIdList> nextFront = vtkSmartPointer<vtkIdList>::New();
  vtkIdType numCellsInFront, neiId;
  vtkIdType numTriangles = this->Mesh->GetNumberOfCells();

  // Check to make sure all boundary edges were recovered. If not,
  // abandon the fill operation.
  for (polys->InitTraversal(); polys->GetNextCell(npts, pts);)
  {
    for (i = 0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i + 1) % npts];
      if (!this->Mesh->IsEdge(p1, p2))
      {
        vtkWarningMacro(<< "Edge not recovered, polygon fill not possible");
        return;
      }
    }
  }

  // Loop over edges of polygon, marking triangles on "outside" of polygon as outside.
  // Then perform a fill.
  for (polys->InitTraversal(); polys->GetNextCell(npts, pts);)
  {
    currentFront->Reset();
    for (i = 0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i + 1) % npts];
      neis->Reset();
      this->GetPoint(p1, x1);
      this->GetPoint(p2, x2);
      for (j = 0; j < 3; j++)
      {
        x21[j] = x2[j] - x1[j];
      }
      vtkMath::Cross(x21, xyNormal, negDir);
      this->Mesh->GetCellEdgeNeighbors(-1, p1, p2, neis); // get both triangles
      numNeis = neis->GetNumberOfIds();
      for (j = 0; j < numNeis; j++)
      { // find the vertex not on the edge; evaluate it (and the cell) in/out
        cellId = neis->GetId(j);
        this->Mesh->GetCellPoints(cellId, numPts, triPts);
        for (k = 0; k < 3; k++)
        {
          if (triPts[k] != p1 && triPts[k] != p2)
          {
            break;
          }
        }
        this->GetPoint(triPts[k], x);
        x[2] = 0.0;
        if (vtkPlane::Evaluate(negDir, x1, x) > 0.0)
        {
          triUse[cellId] = 0;
          currentFront->InsertNextId(cellId);
        }
        else
        {
          triUse[cellId] = -1;
        }
      }
    } // for all edges in polygon

    // Okay, now perform a fill operation (filling "outside" values).
    //
    while ((numCellsInFront = currentFront->GetNumberOfIds()) > 0)
    {
      for (j = 0; j < numCellsInFront; j++)
      {
        cellId = currentFront->GetId(j);

        this->Mesh->GetCellPoints(cellId, numPts, triPts);
        for (k = 0; k < 3; k++)
        {
          p1 = triPts[k];
          p2 = triPts[(k + 1) % 3];

          this->Mesh->GetCellEdgeNeighbors(cellId, p1, p2, neis);
          numNeis = neis->GetNumberOfIds();
          for (kk = 0; kk < numNeis; kk++)
          {
            neiId = neis->GetId(kk);
            if (triUse[neiId] == 1) // 0 is what we're filling with
            {
              triUse[neiId] = 0;
              nextFront->InsertNextId(neiId);
            }
          } // mark all neighbors
        }   // for all edges of cell
      }     // all cells in front

      tmpFront = currentFront;
      currentFront = nextFront;
      nextFront = tmpFront;
      nextFront->Reset();
    } // while still advancing

  } // for all polygons

  // convert all unvisited to inside
  for (i = 0; i < numTriangles; i++)
  {
    if (triUse[i] == -1)
    {
      triUse[i] = 1;
    }
  }
}

//------------------------------------------------------------------------------
int vtkDelaunay2D::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
vtkAbstractTransform* vtkDelaunay2D::ComputeBestFittingPlane(vtkPointSet* input)
{
  int i;
  double normal[3];
  double origin[3];

  constexpr double tolerance = 1.0e-03;

  //  This code was taken from the vtkTextureMapToPlane class
  //  and slightly modified.
  //
  for (i = 0; i < 3; i++)
  {
    normal[i] = 0.0;
  }

  //  Get minimum width of bounding box.
  const double* bounds = input->GetBounds();
  double length = input->GetLength();
  int dir = 0;
  double w;

  for (w = length, i = 0; i < 3; i++)
  {
    normal[i] = 0.0;
    if ((bounds[2 * i + 1] - bounds[2 * i]) < w)
    {
      dir = i;
      w = bounds[2 * i + 1] - bounds[2 * i];
    }
  }

  //  If the bounds is perpendicular to one of the axes, then can
  //  quickly compute normal.
  //
  normal[dir] = 1.0;
  bool normal_computed = false;
  if (w <= (length * tolerance))
  {
    normal_computed = true;
    origin[0] = 0.5 * (bounds[0] + bounds[1]);
    origin[1] = 0.5 * (bounds[2] + bounds[3]);
    origin[2] = 0.5 * (bounds[4] + bounds[5]);
  }

  //
  //   If no simple solution for the normal has been found then use the best-fitting method
  //   from vtkPlane. If that method can't find normal then it will return normal=[0,0,1] as
  //   default
  //
  if (!normal_computed)
  {
    vtkPlane::ComputeBestFittingPlane(input->GetPoints(), origin, normal);
  }

  vtkTransform* transform = vtkTransform::New();

  // Set the new Z axis as the normal to the best fitting
  // plane.
  double zaxis[3];
  zaxis[0] = 0;
  zaxis[1] = 0;
  zaxis[2] = 1;

  double rotationAxis[3];

  vtkMath::Normalize(normal);
  vtkMath::Cross(normal, zaxis, rotationAxis);
  vtkMath::Normalize(rotationAxis);

  const double rotationAngle = 180.0 * acos(vtkMath::Dot(zaxis, normal)) / vtkMath::Pi();

  transform->PreMultiply();
  transform->Identity();

  transform->RotateWXYZ(rotationAngle, rotationAxis[0], rotationAxis[1], rotationAxis[2]);

  // Set the center of mass as the origin of coordinates
  transform->Translate(-origin[0], -origin[1], -origin[2]);

  return transform;
}

//------------------------------------------------------------------------------
void vtkDelaunay2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "ProjectionPlaneMode: "
     << ((this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE) ? "Best Fitting Plane" : "XY Plane")
     << "\n";
  os << indent << "Transform: " << (this->Transform ? "specified" : "none") << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Random Point Insertion: " << (this->RandomPointInsertion ? "On" : "Off") << "\n";
  os << indent << "Use Hilbert Sorter: " << (this->UseHilbertSorter ? "On" : "Off") << "\n";
  os << indent << "Bounding Triangulation: " << (this->BoundingTriangulation ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
