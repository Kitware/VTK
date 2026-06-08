// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConvexHull.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <unordered_map>

static_assert(sizeof(vtkVector3d) == 3 * sizeof(double),
  "vtkVector3d must be layout-compatible with double[3] for reinterpret_cast to be valid");

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkConvexHull);

//------------------------------------------------------------------------------
vtkConvexHull::vtkConvexHull() = default;

//------------------------------------------------------------------------------
void vtkConvexHull::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimension: " << this->Dimension << "\n";
  os << indent << "GeneratePolyData: " << (this->GeneratePolyData ? "On" : "Off") << "\n";
  os << indent << "HullPlanes (count): " << this->HullPlanes.size() << "\n";
}

//------------------------------------------------------------------------------
int vtkConvexHull::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkConvexHull::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  const vtkIdType numPoints = input->GetNumberOfPoints();
  if (numPoints == 0)
  {
    return 1;
  }
  vtkDataArray* pointsDA = input->GetPoints()->GetData();
  if (!pointsDA || pointsDA->GetNumberOfComponents() != 3)
  {
    vtkErrorMacro("Input points must have 3 components");
    return 0;
  }
  vtkNew<vtkAOSDataArrayTemplate<double>> doublePoints;
  doublePoints->ShallowCopy(pointsDA);
  vtkVector3d* pts = reinterpret_cast<vtkVector3d*>(doublePoints->GetPointer(0));

  this->HullPlanes.clear();
  vtkPolyData* geom = this->GeneratePolyData ? output : nullptr;
  const int n = static_cast<int>(numPoints);
  switch (this->Dimension)
  {
    case 1:
      vtkConvexHull::Compute1D(pts, n, this->HullPlanes, geom);
      break;
    case 2:
      vtkConvexHull::Compute2D(pts, n, this->HullPlanes, geom);
      break;
    default:
      vtkConvexHull::Compute3D(pts, n, this->HullPlanes, geom);
      break;
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkConvexHull::IsPointInside(
  const vtkVector3d& p, const std::vector<Plane>& planes, double tol)
{
  for (const auto& hp : planes)
  {
    if (p.Dot(hp.Normal) > hp.D + tol)
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkConvexHull::IsPointWithinConvexHull(double x, double y, double z, double tol) const
{
  return vtkConvexHull::IsPointInside(vtkVector3d(x, y, z), this->HullPlanes, tol);
}

//------------------------------------------------------------------------------
bool vtkConvexHull::IsPointWithinConvexHull(const double point[3], double tol) const
{
  return this->IsPointWithinConvexHull(point[0], point[1], point[2], tol);
}

//------------------------------------------------------------------------------
void vtkConvexHull::ComputeConvexHull(
  vtkDataArray* pointsDA, int dimension, std::vector<vtkConvexHull::Plane>& planes)
{
  if (!pointsDA || pointsDA->GetNumberOfComponents() != 3)
  {
    planes.clear();
    return;
  }
  vtkNew<vtkAOSDataArrayTemplate<double>> doublePoints;
  doublePoints->ShallowCopy(pointsDA);
  vtkVector3d* pts = reinterpret_cast<vtkVector3d*>(doublePoints->GetPointer(0));
  vtkConvexHull::ComputeConvexHull(pts, pointsDA->GetNumberOfTuples(), dimension, planes);
}

//------------------------------------------------------------------------------
void vtkConvexHull::ComputeConvexHull(
  const vtkVector3d* pts, int numPoints, int dimension, std::vector<vtkConvexHull::Plane>& planes)
{
  planes.clear();
  switch (dimension)
  {
    case 1:
      vtkConvexHull::Compute1D(pts, numPoints, planes, nullptr);
      break;
    case 2:
      vtkConvexHull::Compute2D(pts, numPoints, planes, nullptr);
      break;
    default:
      vtkConvexHull::Compute3D(pts, numPoints, planes, nullptr);
      break;
  }
}

//------------------------------------------------------------------------------
// 1-D: O(n) — two endpoint half-planes along the line direction.
void vtkConvexHull::Compute1D(
  const vtkVector3d* pts, vtkIdType n, std::vector<Plane>& planes, vtkPolyData* geom)
{
  if (n < 2)
  {
    return;
  }

  vtkVector3d dir(0, 0, 0);
  bool found = false;
  for (int i = 1; i < n && !found; ++i)
  {
    dir = pts[i] - pts[0];
    const double len = dir.Norm();
    if (len > 1e-12)
    {
      dir = (1.0 / len) * dir;
      found = true;
    }
  }
  if (!found)
  {
    return;
  }

  double dMin = std::numeric_limits<double>::max();
  double dMax = -dMin;
  int iMin = 0, iMax = 0;
  for (int i = 0; i < n; ++i)
  {
    const double d = pts[i].Dot(dir);
    if (d < dMin)
    {
      dMin = d;
      iMin = i;
    }
    if (d > dMax)
    {
      dMax = d;
      iMax = i;
    }
  }

  planes.push_back({ dir, dMax });
  planes.push_back({ vtkVector3d(-dir[0], -dir[1], -dir[2]), -dMin });

  if (geom)
  {
    vtkNew<vtkPoints> outPts;
    outPts->InsertNextPoint(pts[iMin].GetData());
    outPts->InsertNextPoint(pts[iMax].GetData());
    vtkNew<vtkCellArray> lines;
    vtkIdType lineIds[2] = { 0, 1 };
    lines->InsertNextCell(2, lineIds);
    geom->SetPoints(outPts);
    geom->SetLines(lines);
  }
}

//------------------------------------------------------------------------------
// 2-D: Andrew's monotone chain, O(n log n).
void vtkConvexHull::Compute2D(
  const vtkVector3d* pts, vtkIdType n, std::vector<Plane>& planes, vtkPolyData* geom)
{
  if (n < 2)
  {
    return;
  }

  // Find face normal: fix pts[0], scan for first non-coincident pts[ii] (ii >= 1)
  // to get a direction, then scan for first non-collinear pts[k] (k > ii) — O(n).
  vtkVector3d faceNormal;
  bool foundNormal = false;
  {
    vtkVector3d ab;
    int firstDistinct = -1;
    for (int ii = 1; ii < n; ++ii)
    {
      ab = pts[ii] - pts[0];
      if (ab.Norm() > 1e-12)
      {
        firstDistinct = ii;
        break;
      }
    }
    if (firstDistinct != -1)
    {
      for (int k = firstDistinct + 1; k < n && !foundNormal; ++k)
      {
        const vtkVector3d ac = pts[k] - pts[0];
        faceNormal = ab.Cross(ac);
        if (faceNormal.Norm() > 1e-12)
        {
          faceNormal.Normalize();
          foundNormal = true;
        }
      }
    }
  }
  if (!foundNormal)
  {
    vtkConvexHull::Compute1D(pts, n, planes, geom);
    return;
  }

  // Build orthonormal basis {u, v} in the face plane.
  vtkVector3d u =
    (std::abs(faceNormal[0]) < 0.9) ? vtkVector3d(1.0, 0.0, 0.0) : vtkVector3d(0.0, 1.0, 0.0);
  {
    const double proj = u.Dot(faceNormal);
    u = u - proj * faceNormal;
    u.Normalize();
  }
  const vtkVector3d v = faceNormal.Cross(u);

  // Project all points to 2-D (u, v) coordinates.
  using Pt2 = std::pair<double, double>;
  std::vector<Pt2> pts2(n);
  for (int i = 0; i < n; ++i)
  {
    pts2[i] = { pts[i].Dot(u), pts[i].Dot(v) };
  }

  // Sort indices lexicographically for the monotone-chain sweep.
  std::vector<int> idx(n);
  std::iota(idx.begin(), idx.end(), 0);
  std::sort(idx.begin(), idx.end(), [&](int a, int b) { return pts2[a] < pts2[b]; });

  // 2-D signed-area cross product: >0 iff the turn O->A->B is CCW.
  auto cross2D = [&](int O, int A, int B) -> double
  {
    return (pts2[A].first - pts2[O].first) * (pts2[B].second - pts2[O].second) -
      (pts2[A].second - pts2[O].second) * (pts2[B].first - pts2[O].first);
  };

  // Andrew's monotone chain — builds a CCW polygon.
  std::vector<int> hull;
  hull.reserve(2 * n);
  for (int i = 0; i < n; ++i) // lower hull
  {
    while (hull.size() >= 2 && cross2D(hull[hull.size() - 2], hull[hull.size() - 1], idx[i]) <= 0)
    {
      hull.pop_back();
    }
    hull.push_back(idx[i]);
  }
  const int lowerSize = static_cast<int>(hull.size());
  for (int i = n - 2; i >= 0; --i) // upper hull
  {
    while (static_cast<int>(hull.size()) > lowerSize &&
      cross2D(hull[hull.size() - 2], hull[hull.size() - 1], idx[i]) <= 0)
    {
      hull.pop_back();
    }
    hull.push_back(idx[i]);
  }
  hull.pop_back(); // last vertex repeats first

  // For a CCW hull seen from +faceNormal, outward edge normal is edgeDir x faceNormal.
  const int h = static_cast<int>(hull.size());
  for (int i = 0; i < h; ++i)
  {
    const int a = hull[i], b = hull[(i + 1) % h];
    const vtkVector3d edgeDir = pts[b] - pts[a];
    vtkVector3d edgeNormal = edgeDir.Cross(faceNormal);
    const double len = edgeNormal.Norm();
    if (len < 1e-12)
    {
      continue;
    }
    edgeNormal = (1.0 / len) * edgeNormal;
    planes.push_back({ edgeNormal, pts[a].Dot(edgeNormal) });
  }

  // Face-normal slab: pin to the plane using the centroid for robustness
  // instead of relying on a single point.
  vtkVector3d centroid(0.0, 0.0, 0.0);
  for (int i = 0; i < n; ++i)
  {
    centroid += pts[i];
  }
  centroid = (1.0 / n) * centroid;
  const double faceD = centroid.Dot(faceNormal);
  planes.push_back({ faceNormal, faceD });
  planes.push_back({ -1.0 * faceNormal, -faceD });

  if (geom)
  {
    vtkNew<vtkPoints> outPts;
    for (const int i : hull)
    {
      outPts->InsertNextPoint(pts[i].GetData());
    }
    vtkNew<vtkCellArray> polys;
    std::vector<vtkIdType> ids(h);
    std::iota(ids.begin(), ids.end(), 0);
    polys->InsertNextCell(h, ids.data());
    geom->SetPoints(outPts);
    geom->SetPolys(polys);
  }
}

//------------------------------------------------------------------------------
// 3-D: Quickhull, O(n log n) average.
//------------------------------------------------------------------------------
// 3-D: Quickhull, O(n log n) average.
void vtkConvexHull::Compute3D(
  const vtkVector3d* pts, vtkIdType n, std::vector<Plane>& planes, vtkPolyData* geom)
{
  if (n < 4)
  {
    vtkConvexHull::Compute2D(pts, n, planes, geom);
    return;
  }

  // --- Step 1: find initial tetrahedron ---

  // Two most distant points: O(n) via per-axis extremes instead of O(n^2).
  int p0 = 0, p1 = 1;
  {
    int minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
    for (int i = 1; i < n; ++i)
    {
      if (pts[i][0] < pts[minX][0])
        minX = i;
      if (pts[i][0] > pts[maxX][0])
        maxX = i;
      if (pts[i][1] < pts[minY][1])
        minY = i;
      if (pts[i][1] > pts[maxY][1])
        maxY = i;
      if (pts[i][2] < pts[minZ][2])
        minZ = i;
      if (pts[i][2] > pts[maxZ][2])
        maxZ = i;
    }
    const int candidates[6] = { minX, maxX, minY, maxY, minZ, maxZ };
    double maxD2 = -1.0;
    for (int a = 0; a < 6; ++a)
    {
      for (int b = a + 1; b < 6; ++b)
      {
        const double d2 = (pts[candidates[b]] - pts[candidates[a]]).SquaredNorm();
        if (d2 > maxD2)
        {
          maxD2 = d2;
          p0 = candidates[a];
          p1 = candidates[b];
        }
      }
    }
    if (maxD2 < 1e-24)
    {
      return;
    }
  }

  // Point farthest from line p0-p1.
  int p2 = -1;
  {
    vtkVector3d ab = pts[p1] - pts[p0];
    const double abLen = ab.Norm();
    ab = (1.0 / abLen) * ab;
    double maxD2 = -1.0;
    for (int i = 0; i < n; ++i)
    {
      if (i == p0 || i == p1)
      {
        continue;
      }
      const vtkVector3d ap = pts[i] - pts[p0];
      const vtkVector3d perp = ap - ap.Dot(ab) * ab;
      const double d2 = perp.SquaredNorm();
      if (d2 > maxD2)
      {
        maxD2 = d2;
        p2 = i;
      }
    }
    if (p2 == -1 || maxD2 < 1e-24)
    {
      vtkConvexHull::Compute2D(pts, n, planes, geom);
      return;
    }
  }

  // Point farthest from plane p0-p1-p2.
  int p3 = -1;
  {
    vtkVector3d tn = (pts[p1] - pts[p0]).Cross(pts[p2] - pts[p0]);
    const double tnLen = tn.Norm();
    tn = (1.0 / tnLen) * tn;
    const double td = pts[p0].Dot(tn);
    double maxDist = -1.0;
    for (int i = 0; i < n; ++i)
    {
      if (i == p0 || i == p1 || i == p2)
      {
        continue;
      }
      const double dist = std::abs(pts[i].Dot(tn) - td);
      if (dist > maxDist)
      {
        maxDist = dist;
        p3 = i;
      }
    }
    if (p3 == -1 || maxDist < 1e-12)
    {
      vtkConvexHull::Compute2D(pts, n, planes, geom);
      return;
    }
  }

  // Fixed interior reference: centroid of the initial tetrahedron.
  const vtkVector3d interior = 0.25 * (pts[p0] + pts[p1] + pts[p2] + pts[p3]);

  // --- Step 2: face representation ---

  struct QHFace
  {
    std::array<int, 3> v;
    vtkVector3d n;
    double d;
    std::vector<int> pts;
    bool alive{ true };
  };

  auto makeFace = [&](int v0, int v1, int v2) -> QHFace
  {
    vtkVector3d fn = (pts[v1] - pts[v0]).Cross(pts[v2] - pts[v0]);
    const double len = fn.Norm();
    if (len < 1e-15)
    {
      return { { v0, v1, v2 }, {}, 0.0, {}, false };
    }
    fn = (1.0 / len) * fn;
    double d = pts[v0].Dot(fn);
    if (interior.Dot(fn) > d)
    {
      fn = -1.0 * fn;
      d = -d;
    }
    return { { v0, v1, v2 }, fn, d, {}, true };
  };

  // --- Step 3: build initial tetrahedron and assign outside points ---

  std::vector<QHFace> faces;
  faces.reserve(32);
  for (auto [a, b, c] : std::initializer_list<std::array<int, 3>>{
         { p0, p1, p2 }, { p0, p1, p3 }, { p0, p2, p3 }, { p1, p2, p3 } })
  {
    QHFace f = makeFace(a, b, c);
    if (f.alive)
    {
      faces.push_back(std::move(f));
    }
  }

  // alive[] mirrors faces[] so visible-face scan never touches dead entries.
  // We keep a separate compact list of alive indices to avoid O(total-ever-created) scans.
  // faceToAliveIdx[fi] = position of fi in aliveFaces; enables O(1) swap-and-pop deletion.
  std::vector<int> aliveFaces;
  std::vector<int> faceToAliveIdx;
  aliveFaces.reserve(32);
  faceToAliveIdx.reserve(32);
  for (int i = 0; i < static_cast<int>(faces.size()); ++i)
  {
    faceToAliveIdx.push_back(static_cast<int>(aliveFaces.size()));
    aliveFaces.push_back(i);
  }

  for (int i = 0; i < n; ++i)
  {
    if (i == p0 || i == p1 || i == p2 || i == p3)
    {
      continue;
    }
    int bestFace = -1;
    double bestDist = 1e-10;
    for (const int fi : aliveFaces)
    {
      const double dist = pts[i].Dot(faces[fi].n) - faces[fi].d;
      if (dist > bestDist)
      {
        bestDist = dist;
        bestFace = fi;
      }
    }
    if (bestFace != -1)
    {
      faces[bestFace].pts.push_back(i);
    }
  }

  // --- Step 4: iterative hull expansion ---

  // Queue holds alive face indices that have outside points.
  std::vector<int> queue;
  queue.reserve(32);
  for (const int fi : aliveFaces)
  {
    if (!faces[fi].pts.empty())
    {
      queue.push_back(fi);
    }
  }

  // Reusable scratch buffers to avoid per-iteration heap churn.
  std::vector<int> visible;
  std::vector<int> orphans;
  // Horizon encoded as packed int64: (u << 32 | w). Flat vector + sort beats std::set.
  std::vector<int64_t> visEdges;
  std::vector<std::pair<int, int>> horizon;

  int qHead = 0;
  while (qHead < static_cast<int>(queue.size()))
  {
    const int fIdx = queue[qHead++];
    if (!faces[fIdx].alive || faces[fIdx].pts.empty())
    {
      continue;
    }

    // Apex: point farthest outside this face.
    int apex = -1;
    double maxDist = -1.0;
    for (const int idx : faces[fIdx].pts)
    {
      const double dist = pts[idx].Dot(faces[fIdx].n) - faces[fIdx].d;
      if (dist > maxDist)
      {
        maxDist = dist;
        apex = idx;
      }
    }
    if (apex == -1)
    {
      continue;
    }

    // Collect all alive faces visible from apex.
    visible.clear();
    for (const int fi : aliveFaces)
    {
      if (faces[fi].alive && pts[apex].Dot(faces[fi].n) > faces[fi].d + 1e-10)
      {
        visible.push_back(fi);
      }
    }

    // Collect orphaned outside points from visible faces.
    orphans.clear();
    for (const int vi : visible)
    {
      for (const int idx : faces[vi].pts)
      {
        if (idx != apex)
        {
          orphans.push_back(idx);
        }
      }
    }
    // Horizon: directed edges of visible faces whose reverse is absent.
    // Encode each directed edge as a single int64 for O(1) lookup via sort.
    visEdges.clear();
    visEdges.reserve(visible.size() * 3);
    for (const int vi : visible)
    {
      for (int e = 0; e < 3; ++e)
      {
        const int u = faces[vi].v[e];
        const int w = faces[vi].v[(e + 1) % 3];
        visEdges.push_back((static_cast<int64_t>(u) << 32) | static_cast<uint32_t>(w));
      }
    }
    std::sort(visEdges.begin(), visEdges.end());

    horizon.clear();
    for (const int vi : visible)
    {
      for (int e = 0; e < 3; ++e)
      {
        const int u = faces[vi].v[e];
        const int w = faces[vi].v[(e + 1) % 3];
        // Edge (u,w) is on the horizon iff reverse (w,u) is NOT in visEdges.
        const int64_t rev = (static_cast<int64_t>(w) << 32) | static_cast<uint32_t>(u);
        if (!std::binary_search(visEdges.begin(), visEdges.end(), rev))
        {
          horizon.emplace_back(u, w);
        }
      }
    }

    // Kill visible faces; O(1) swap-and-pop per face using faceToAliveIdx.
    for (const int vi : visible)
    {
      faces[vi].alive = false;
      faces[vi].pts.clear();
      const int alivePos = faceToAliveIdx[vi];
      const int lastFi = aliveFaces.back();
      aliveFaces[alivePos] = lastFi;
      faceToAliveIdx[lastFi] = alivePos;
      aliveFaces.pop_back();
    }

    // Create new faces from horizon edges to apex.
    const int newStart = static_cast<int>(faces.size());
    for (const auto& [u, w] : horizon)
    {
      QHFace nf = makeFace(u, w, apex);
      if (nf.alive)
      {
        faces.push_back(std::move(nf));
        faceToAliveIdx.push_back(static_cast<int>(aliveFaces.size()));
        aliveFaces.push_back(static_cast<int>(faces.size()) - 1);
      }
    }

    // Reassign orphans to new faces only.
    for (const int idx : orphans)
    {
      int bestFace = -1;
      double bestDist = 1e-10;
      for (int i = newStart; i < static_cast<int>(faces.size()); ++i)
      {
        if (!faces[i].alive)
        {
          continue;
        }
        const double dist = pts[idx].Dot(faces[i].n) - faces[i].d;
        if (dist > bestDist)
        {
          bestDist = dist;
          bestFace = i;
        }
      }
      if (bestFace != -1)
      {
        faces[bestFace].pts.push_back(idx);
      }
    }

    // Enqueue new faces that have outside points.
    for (int i = newStart; i < static_cast<int>(faces.size()); ++i)
    {
      if (faces[i].alive && !faces[i].pts.empty())
      {
        queue.push_back(i);
      }
    }
  }

  // --- Step 5: collect half-planes from surviving faces ---

  planes.reserve(planes.size() + aliveFaces.size());
  for (const int fi : aliveFaces)
  {
    planes.push_back({ faces[fi].n, faces[fi].d });
  }

  if (geom)
  {
    std::unordered_map<int, vtkIdType> vertMap;
    vtkNew<vtkPoints> outPts;
    for (const int fi : aliveFaces)
    {
      for (const int vi : faces[fi].v)
      {
        if (vertMap.find(vi) == vertMap.end())
        {
          vertMap[vi] = outPts->InsertNextPoint(pts[vi].GetData());
        }
      }
    }
    vtkNew<vtkCellArray> tris;
    for (const int fi : aliveFaces)
    {
      vtkIdType tri[3] = { vertMap[faces[fi].v[0]], vertMap[faces[fi].v[1]],
        vertMap[faces[fi].v[2]] };
      tris->InsertNextCell(3, tri);
    }
    geom->SetPoints(outPts);
    geom->SetPolys(tris);
  }
}

VTK_ABI_NAMESPACE_END
