// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#include "vtkFrustumSelector.h"

#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSelectionNode.h"
#include "vtkSelector.h"
#include "vtkSignedCharArray.h"
#include "vtkVector.h"

#include <bitset>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
constexpr int MAX_PLANES = 6;
//------------------------------------------------------------------------------
void ComputePlane(
  int idx, double v0[3], double v1[3], double v2[3], vtkPoints* points, vtkDoubleArray* norms)
{
  points->SetPoint(idx, v0[0], v0[1], v0[2]);

  double e0[3];
  e0[0] = v1[0] - v0[0];
  e0[1] = v1[1] - v0[1];
  e0[2] = v1[2] - v0[2];

  double e1[3];
  e1[0] = v2[0] - v0[0];
  e1[1] = v2[1] - v0[1];
  e1[2] = v2[2] - v0[2];

  double n[3];
  vtkMath::Cross(e0, e1, n);
  vtkMath::Normalize(n);

  norms->SetTuple(idx, n);
}

//------------------------------------------------------------------------------
std::array<int, MAX_PLANES * 2> ComputeNPVertexIds(vtkPlanes* frustum)
{
  std::array<int, MAX_PLANES * 2> res;
  std::array<double, 3> x;
  // find the near and far vertices to each plane for quick in/out tests
  for (int i = 0; i < MAX_PLANES; i++)
  {
    frustum->GetNormals()->GetTuple(i, x.data());
    int xside = (x[0] > 0) ? 1 : 0;
    int yside = (x[1] > 0) ? 1 : 0;
    int zside = (x[2] > 0) ? 1 : 0;
    res[2 * i] = (1 - xside) * 4 + (1 - yside) * 2 + (1 - zside);
    res[2 * i + 1] = xside * 4 + yside * 2 + zside;
  }
  return res;
}

//------------------------------------------------------------------------------
struct FrustumPlanesType : public std::array<vtkSmartPointer<vtkPlane>, MAX_PLANES>
{
  FrustumPlanesType()
  {
    for (int i = 0; i < MAX_PLANES; ++i)
    {
      this->operator[](i) = vtkSmartPointer<vtkPlane>::New();
    }
  }
  void Initialize(vtkPlanes* frustum)
  {
    for (int i = 0; i < MAX_PLANES; ++i)
    {
      frustum->GetPlane(i, this->operator[](i));
    }
  }
};

//------------------------------------------------------------------------------
class ComputeCellsInFrustumFunctor
{
private:
  vtkPlanes* Frustum;
  vtkDataSet* Input;
  vtkSignedCharArray* Array;
  std::array<int, MAX_PLANES * 2> NPVertexIds;

  vtkSMPThreadLocalObject<vtkGenericCell> TLCell;
  vtkSMPThreadLocal<FrustumPlanesType> TLFrustumPlanes;
  vtkSMPThreadLocal<std::vector<double>> TLVertexBuffer;

public:
  ComputeCellsInFrustumFunctor(vtkPlanes* f, vtkDataSet* in, vtkSignedCharArray* array)
    : Frustum(f)
    , Input(in)
    , Array(array)
  {
    // Hacky PrepareForMultithreadedAccess()
    // call everything we will call on the data object on the main thread first
    // so that it can build its caching structures
    vtkNew<vtkGenericCell> cell;
    this->Input->GetCell(0, cell);

    this->NPVertexIds = ComputeNPVertexIds(this->Frustum);
  }

  //--------------------------------------------------------------------------
  void Initialize() { this->TLFrustumPlanes.Local().Initialize(this->Frustum); }

  //--------------------------------------------------------------------------
  void operator()(vtkIdType begin, vtkIdType end)
  {
    double bounds[6];
    auto& cell = this->TLCell.Local();
    auto& frustumPlanes = this->TLFrustumPlanes.Local();
    auto& vertexBuffer = this->TLVertexBuffer.Local();

    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      this->Input->GetCellBounds(cellId, bounds);
      int isect = this->ABoxFrustumIsect(cellId, bounds, cell, frustumPlanes, vertexBuffer, false);
      this->Array->SetValue(cellId, static_cast<signed char>(isect == 1));
    }
  }

  //--------------------------------------------------------------------------
  void Reduce()
  {
    // nothing to do here.
  }

  //--------------------------------------------------------------------------
  // Intersect the cell (with its associated bounds) with the clipping frustum.
  // Return 1 if at least partially inside, 0 otherwise.
  // Also return a distance to the near plane.
  int ABoxFrustumIsect(vtkIdType cellId, double* bounds, vtkGenericCell* cell,
    FrustumPlanesType& frustumPlanes, std::vector<double>& vertexBuffer, bool cellExtracted)
  {
    if (bounds[0] > bounds[1] || bounds[2] > bounds[3] || bounds[4] > bounds[5])
    {
      if (!cellExtracted)
      {
        this->Input->GetCell(cellId, cell);
      }
      return this->IsectDegenerateCell(cell);
    }

    // convert bounds to 8 vertices
    double verts[8][3];
    verts[0][0] = bounds[0];
    verts[0][1] = bounds[2];
    verts[0][2] = bounds[4];
    verts[1][0] = bounds[0];
    verts[1][1] = bounds[2];
    verts[1][2] = bounds[5];
    verts[2][0] = bounds[0];
    verts[2][1] = bounds[3];
    verts[2][2] = bounds[4];
    verts[3][0] = bounds[0];
    verts[3][1] = bounds[3];
    verts[3][2] = bounds[5];
    verts[4][0] = bounds[1];
    verts[4][1] = bounds[2];
    verts[4][2] = bounds[4];
    verts[5][0] = bounds[1];
    verts[5][1] = bounds[2];
    verts[5][2] = bounds[5];
    verts[6][0] = bounds[1];
    verts[6][1] = bounds[3];
    verts[6][2] = bounds[4];
    verts[7][0] = bounds[1];
    verts[7][1] = bounds[3];
    verts[7][2] = bounds[5];

    int intersect = 0;

    // reject if any plane rejects the entire bbox
    for (int pid = 0; pid < MAX_PLANES; pid++)
    {
      auto& plane = frustumPlanes[pid];
      double dist;
      int nvid;
      int pvid;
      nvid = this->NPVertexIds[2 * pid];
      dist = plane->EvaluateFunction(verts[nvid]);
      if (dist > 0.0)
      {
        return 0;
      }
      pvid = this->NPVertexIds[2 * pid + 1];
      dist = plane->EvaluateFunction(verts[pvid]);
      if (dist > 0.0)
      {
        intersect = 1;
        break;
      }
    }

    // accept if entire bbox is inside all planes
    if (!intersect)
    {
      return 1;
    }

    // otherwise, we have to do clipping tests to decide if actually insects
    vtkCell* face;
    vtkCell* edge;
    vtkPoints* pts = nullptr;
    vertexBuffer.clear();
    int maxedges = 16;
    // be ready to resize if we hit a polygon with many vertices
    vertexBuffer.resize(3 * maxedges * 3);
    double* vlist = &vertexBuffer[0 * maxedges * 3];
    double* wvlist = &vertexBuffer[1 * maxedges * 3];
    double* ovlist = &vertexBuffer[2 * maxedges * 3];

    if (!cellExtracted)
    {
      this->Input->GetCell(cellId, cell);
    }

    int nfaces = cell->GetNumberOfFaces();
    if (nfaces < 1)
    {
      // some 2D cells have no faces, only edges
      int nedges = cell->GetNumberOfEdges();
      if (nedges < 1)
      {
        // VTK_LINE and VTK_POLY_LINE have no "edges" -- the cells
        // themselves are edges.  We catch them here and assemble the
        // list of vertices by hand because the code below assumes that
        // GetNumberOfEdges()==0 means a degenerate cell containing only
        // points.
        if (cell->GetCellType() == VTK_LINE)
        {
          nedges = 2;
          vtkPoints* points = cell->GetPoints();
          points->GetPoint(0, &vlist[0 * 3]);
          points->GetPoint(1, &vlist[1 * 3]);
        }
        else if (cell->GetCellType() == VTK_POLY_LINE)
        {
          nedges = cell->GetPointIds()->GetNumberOfIds();
          vtkPoints* points = cell->GetPoints();
          if (nedges + 4 > maxedges)
          {
            maxedges = (nedges + 4) * 2;
            vertexBuffer.resize(3 * maxedges * 3);
            vlist = &vertexBuffer[0 * maxedges * 3];
            wvlist = &vertexBuffer[1 * maxedges * 3];
            ovlist = &vertexBuffer[2 * maxedges * 3];
          }
          for (vtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i)
          {
            points->GetPoint(i, &vlist[i * 3]);
          }
        }
        else
        {
          return this->IsectDegenerateCell(cell);
        }
      }
      if (nedges + 4 > maxedges)
      {
        maxedges = (nedges + 4) * 2;
        vertexBuffer.resize(3 * maxedges * 3);
        vlist = &vertexBuffer[0 * maxedges * 3];
        wvlist = &vertexBuffer[1 * maxedges * 3];
        ovlist = &vertexBuffer[2 * maxedges * 3];
      }
      edge = cell->GetEdge(0);
      if (edge)
      {
        pts = edge->GetPoints();
        pts->GetPoint(0, &vlist[0 * 3]);
        pts->GetPoint(1, &vlist[1 * 3]);
      }
      switch (cell->GetCellType())
      {
        case VTK_PIXEL:
        {
          edge = cell->GetEdge(2);
          pts = edge->GetPoints();
          pts->GetPoint(0, &vlist[3 * 3]);
          pts->GetPoint(1, &vlist[2 * 3]);
          break;
        }
        case VTK_QUAD:
        {
          edge = cell->GetEdge(2);
          pts = edge->GetPoints();
          pts->GetPoint(0, &vlist[2 * 3]);
          pts->GetPoint(1, &vlist[3 * 3]);
          break;
        }
        case VTK_TRIANGLE:
        {
          edge = cell->GetEdge(1);
          pts = edge->GetPoints();
          pts->GetPoint(1, &vlist[2 * 3]);
          break;
        }
        case VTK_LINE:
        case VTK_POLY_LINE:
        {
          return this->FrustumClipPolyline(nedges, vlist, bounds);
        }
        default:
        {
          for (int e = 1; e < nedges - 1; e++)
          {
            edge = cell->GetEdge(e);
            pts = edge->GetPoints();
            pts->GetPoint(1, &vlist[(e + 1) * 3]); // get second point of the edge
          }
          break;
        }
      }
      if (this->FrustumClipPolygon(nedges, vlist, wvlist, ovlist, frustumPlanes))
      {
        return 1;
      }
    }
    else
    {
      // go around edges of each face and clip to planes
      // if nothing remains at the end, then we do not intersect and reject
      for (int f = 0; f < nfaces; f++)
      {
        face = cell->GetFace(f);

        int nedges = face->GetNumberOfEdges();
        if (nedges < 1)
        {
          if (this->IsectDegenerateCell(face))
          {
            return 1;
          }
          continue;
        }
        if (nedges + 4 > maxedges)
        {
          maxedges = (nedges + 4) * 2;
          vertexBuffer.resize(3 * maxedges * 3);
          vlist = &vertexBuffer[0 * maxedges * 3];
          wvlist = &vertexBuffer[1 * maxedges * 3];
          ovlist = &vertexBuffer[2 * maxedges * 3];
        }
        edge = face->GetEdge(0);
        pts = edge->GetPoints();
        pts->GetPoint(0, &vlist[0 * 3]);
        pts->GetPoint(1, &vlist[1 * 3]);
        switch (face->GetCellType())
        {
          case VTK_PIXEL:
            edge = face->GetEdge(2);
            pts = edge->GetPoints();
            pts->GetPoint(0, &vlist[3 * 3]);
            pts->GetPoint(1, &vlist[2 * 3]);
            break;
          case VTK_QUAD:
          {
            edge = face->GetEdge(2);
            pts = edge->GetPoints();
            pts->GetPoint(0, &vlist[2 * 3]);
            pts->GetPoint(1, &vlist[3 * 3]);
            break;
          }
          case VTK_TRIANGLE:
          {
            edge = face->GetEdge(1);
            pts = edge->GetPoints();
            pts->GetPoint(1, &vlist[2 * 3]);
            break;
          }
          case VTK_LINE:
          {
            break;
          }
          default:
          {
            for (int e = 1; e < nedges - 1; e++)
            {
              edge = cell->GetEdge(e);
              pts = edge->GetPoints();
              pts->GetPoint(1, &vlist[(e + 1) * 3]); // get second point of the edge
            }
            break;
          }
        }
        if (this->FrustumClipPolygon(nedges, vlist, wvlist, ovlist, frustumPlanes))
        {
          return 1;
        }
      }
    }

    return 0;
  }

  //--------------------------------------------------------------------------
  // handle degenerate cells by testing each point, if any in, then in
  int IsectDegenerateCell(vtkCell* cell)
  {
    vtkPoints* pts = cell->GetPoints();
    double x[3];
    for (vtkIdType i = 0, npts = cell->GetNumberOfPoints(); i < npts; i++)
    {
      pts->GetPoint(i, x);
      if (this->Frustum->EvaluateFunction(x) < 0.0)
      {
        return 1;
      }
    }
    return 0;
  }

  //--------------------------------------------------------------------------
  // clips the polygon against the frustum
  // if there is no intersection, returns 0
  // if there is an intersection, returns 1
  // update ovlist to contain the resulting clipped vertices
  int FrustumClipPolygon(
    int nverts, double* ivlist, double* wvlist, double* ovlist, FrustumPlanesType& frustumPlanes)
  {
    int nwverts = nverts;
    memcpy(wvlist, ivlist, nverts * sizeof(double) * 3);

    int noverts = 0;
    int pid;
    for (pid = 0; pid < MAX_PLANES; pid++)
    {
      noverts = 0;
      this->PlaneClipPolygon(nwverts, wvlist, pid, noverts, ovlist, frustumPlanes);
      if (noverts == 0)
      {
        return 0;
      }
      memcpy(wvlist, ovlist, noverts * sizeof(double) * 3);
      nwverts = noverts;
    }

    return 1;
  }

  //--------------------------------------------------------------------------
  // clips a polygon against the numbered plane, resulting vertices are stored
  // in ovlist, noverts
  void PlaneClipPolygon(int nverts, double* ivlist, int pid, int& noverts, double* ovlist,
    FrustumPlanesType& frustumPlanes)
  {
    int vid;
    // run around the polygon and clip to this edge
    for (vid = 0; vid < nverts - 1; vid++)
    {
      this->PlaneClipEdge(
        &ivlist[vid * 3], &ivlist[(vid + 1) * 3], pid, noverts, ovlist, frustumPlanes);
    }
    this->PlaneClipEdge(
      &ivlist[(nverts - 1) * 3], &ivlist[0 * 3], pid, noverts, ovlist, frustumPlanes);
  }

  //--------------------------------------------------------------------------
  // clips a line segment against the numbered plane.
  // intersection point and the second vertex are added to overts if on or inside
  void PlaneClipEdge(
    double* V0, double* V1, int pid, int& noverts, double* overts, FrustumPlanesType& frustumPlanes)
  {
    double t = 0.0;
    double ISECT[3];
    double normal[3], point[3];
    this->Frustum->GetNormals()->GetTuple(pid, normal);
    this->Frustum->GetPoints()->GetPoint(pid, point);
    int rc = vtkPlane::IntersectWithLine(V0, V1, normal, point, t, ISECT);

    if (rc)
    {
      overts[noverts * 3 + 0] = ISECT[0];
      overts[noverts * 3 + 1] = ISECT[1];
      overts[noverts * 3 + 2] = ISECT[2];
      noverts++;
    }

    auto& plane = frustumPlanes[pid];
    if (plane->EvaluateFunction(V1) < 0.0)
    {
      overts[noverts * 3 + 0] = V1[0];
      overts[noverts * 3 + 1] = V1[1];
      overts[noverts * 3 + 2] = V1[2];
      noverts++;
    }
  }

  //--------------------------------------------------------------------------
  // Tests edge segments against the frustum.
  // If there is no intersection, returns 0
  // If there is an intersection, returns 1
  // This is accomplished using Cyrus-Beck clipping.
  int FrustumClipPolyline(int nverts, double* ivlist, double* bounds)
  {
    if (nverts < 1)
    {
      return 0;
    }
    vtkVector3d p0(ivlist[0], ivlist[1], ivlist[2]);
    if (nverts < 2)
    {
      return this->ComputePlaneEndpointCode(p0) == 0;
    }
    // Compute the L1 "diameter" of the bounding box (used to test for degeneracy)
    // We know bounds is valid at this point, so diam >= 0
    const double diam = bounds[1] - bounds[0] + bounds[3] - bounds[2] + bounds[5] - bounds[4];
    const double epsilon = 1e-6 * diam;
    const double epsilon2 = 1e-10 * diam * diam;
    vtkVector3d normal, basePoint;
    vtkVector3d p1;
    bool in = false;
    for (int ii = 1; ii < nverts; ++ii, p0 = p1)
    {
      p1 = vtkVector3d(ivlist[3 * ii], ivlist[3 * ii + 1], ivlist[3 * ii + 2]);
      vtkVector3d lineVec = p1 - p0;
      if (lineVec.SquaredNorm() < epsilon2)
      {
        // Skip short edges; they would make denom == 0.0 and thus have no effect.
        continue;
      }
      double tmin = 0.0;
      double tmax = 1.0;
      bool mayOverlap = true;
      for (int pp = 0; mayOverlap && (pp < MAX_PLANES); ++pp)
      {
        this->Frustum->GetNormals()->GetTuple(pp, normal.GetData());
        this->Frustum->GetPoints()->GetPoint(pp, basePoint.GetData());
        vtkVector3d db = p0 - basePoint; // Vector from the plane's base point to p0 on the line.
        double numer = db.Dot(normal);
        double denom = lineVec.Dot(normal);
        double t;
        if (std::abs(denom) <= epsilon)
        {
          if (numer > 0)
          {
            mayOverlap = false;
          }
        }
        else
        {
          t = -numer / denom;
          if (denom < 0.0 && t > tmin)
          {
            tmin = t;
          }
          else if (denom > 0.0 && t < tmax)
          {
            tmax = t;
          }
        }
      }
      if (mayOverlap)
      {
        in |= (tmin <= tmax);
        if (in)
        {
          break;
        }
      }
    }
    return in ? 1 : 0;
  }

  int ComputePlaneEndpointCode(const vtkVector3d& vertex)
  {
    int code = 0;
    vtkVector3d normal, basePoint;
    for (int pp = 0; pp < MAX_PLANES; ++pp)
    {
      this->Frustum->GetNormals()->GetTuple(pp, normal.GetData());
      this->Frustum->GetPoints()->GetPoint(pp, basePoint.GetData());
      code |= ((vertex - basePoint).Dot(normal) >= 0.0) ? (1 << pp) : 0;
    }
    return code;
  }
};

struct ComputeHTGCellsInFrustumFunctor
{
public:
  ComputeHTGCellsInFrustumFunctor(
    vtkPlanes* frustum, vtkHyperTreeGrid* input, vtkSignedCharArray* insideArray)
    : Frustum(frustum)
    , HTG(input)
    , Array(insideArray)
  {
    insideArray->Fill(static_cast<signed char>(0));
  }

  void Initialize() { this->TLPlanes.Local().Initialize(this->Frustum); }

  void operator()(vtkIdType beginTree, vtkIdType endTree)
  {
    for (vtkIdType iTree = beginTree; iTree < endTree; ++iTree)
    {
      vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
      cursor->Initialize(this->HTG, iTree);
      this->RecursivelyIntersectTree(cursor);
    }
  }

  void RecursivelyIntersectTree(vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
  {
    std::array<double, 6> bounds;
    cursor->GetBounds(bounds.data());
    auto& cell = this->TLCell.Local();
    if (!this->ConstructCell(cursor, cell))
    {
      vtkErrorWithObjectMacro(nullptr, "Unable to construct cell");
      return;
    }
    vtkIdType cellId = cursor->GetGlobalNodeIndex();
    int isect = this->CheckCellFrustumHit(cell);
    this->Array->SetValue(cellId, isect);
    if (isect && !cursor->IsLeaf())
    {
      for (vtkIdType iChild = 0; iChild < cursor->GetNumberOfChildren(); ++iChild)
      {
        cursor->ToChild(iChild);
        this->RecursivelyIntersectTree(cursor);
        cursor->ToParent();
      }
    }
  }

  bool CheckCellFrustumHit(vtkGenericCell* cell)
  {
    // check every point in the cell if it is in Frustum
    vtkPoints* points = cell->GetPoints();
    std::array<double, 3> point;
    for (vtkIdType iPt = 0; iPt < cell->GetNumberOfPoints(); iPt++)
    {
      points->GetPoint(iPt, point.data());
      if (this->Frustum->EvaluateFunction(point.data()) < 0.0)
      {
        return true;
      }
    }
    // if no point is in frustum check if frustum is contained in the cell
    // do this by checking if there is a plane for which all the points in the cell
    // are a positive distance away from it
    std::vector<double> distances(cell->GetNumberOfPoints());
    auto checkAllPositive = [](std::vector<double>& vals)
    {
      for (auto val : vals)
      {
        if (val < 0)
        {
          return false;
        }
      }
      return true;
    };
    FrustumPlanesType& planes = this->TLPlanes.Local();
    for (const auto& plane : planes)
    {
      for (vtkIdType iPt = 0; iPt < cell->GetNumberOfPoints(); iPt++)
      {
        points->GetPoint(iPt, point.data());
        distances[iPt] = plane->EvaluateFunction(point.data());
      }
      if (checkAllPositive(distances))
      {
        return false;
      }
    }
    // if the cell has no points in the frustum but is not completely on one side of all the planes
    // than it must either englobe the frustum or at least one of its faces must traverse it
    return true;
  }

  bool ConstructCell(vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkGenericCell* cell) const
  {
    double* origin = cursor->GetOrigin();
    double* size = cursor->GetSize();
    if (cell == nullptr || origin == nullptr || size == nullptr)
    {
      vtkErrorWithObjectMacro(nullptr, "Cell, origin or size that was passed is nullptr");
      return false;
    }

    const unsigned int dim = this->HTG->GetDimension();
    switch (dim)
    {
      case (1):
        cell->SetCellTypeToLine();
        break;
      case (2):
        cell->SetCellTypeToPixel();
        break;
      case (3):
        cell->SetCellTypeToVoxel();
        break;
      default:
        vtkErrorWithObjectMacro(nullptr, "Wrong HyperTreeGrid dimension");
        return false;
    }

    unsigned int nPoints = std::pow(2, dim);
    for (unsigned int iP = 0; iP < nPoints; iP++)
    {
      cell->PointIds->SetId(iP, iP);
    }

    auto cubePoint = [dim, origin, size](std::bitset<3>& pos, std::vector<double>* cubePt)
    {
      for (unsigned int d = 0; d < dim; d++)
      {
        cubePt->at(d) = origin[d] + pos[d] * size[d];
      }
    };
    std::vector<double> pt(3, 0.0);
    std::vector<std::bitset<3>> positions(8);
    positions[0] = 0; // 000
    positions[1] = 1; // 001 -> +x
    positions[2] = 2; // 010 -> +y
    positions[3] = 3; // 011 -> +xy
    positions[4] = 4; // 100 -> +z
    positions[5] = 5; // 101 -> +zx
    positions[6] = 6; // 110 -> +zy
    positions[7] = 7; // 111 -> +zxy
    for (unsigned int iP = 0; iP < nPoints; iP++)
    {
      cubePoint(positions[iP], &pt);
      cell->Points->SetPoint(iP, pt.data());
    }
    return true;
  }

  void Reduce()
  { /* do nothing */
  }

protected:
  vtkPlanes* Frustum;
  vtkHyperTreeGrid* HTG;
  vtkSignedCharArray* Array;

  vtkSMPThreadLocalObject<vtkGenericCell> TLCell;
  vtkSMPThreadLocal<FrustumPlanesType> TLPlanes;
};

}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFrustumSelector);

//------------------------------------------------------------------------------
vtkFrustumSelector::vtkFrustumSelector(vtkPlanes* f)
{
  this->Frustum = f;
  if (!this->Frustum)
  {
    // an inside out unit cube - which selects nothing
    double verts[32] = {
      0.0, 0.0, 0.0, 0.0, //
      0.0, 0.0, 1.0, 0.0, //
      0.0, 1.0, 0.0, 0.0, //
      0.0, 1.0, 1.0, 0.0, //
      1.0, 0.0, 0.0, 0.0, //
      1.0, 0.0, 1.0, 0.0, //
      1.0, 1.0, 0.0, 0.0, //
      1.0, 1.0, 1.0, 0.0  //
    };
    this->Frustum = vtkSmartPointer<vtkPlanes>::New();
    this->CreateFrustum(verts);
  }
}

//------------------------------------------------------------------------------
vtkFrustumSelector::~vtkFrustumSelector() = default;

//------------------------------------------------------------------------------
vtkPlanes* vtkFrustumSelector::GetFrustum()
{
  return this->Frustum;
}

//------------------------------------------------------------------------------
void vtkFrustumSelector::SetFrustum(vtkPlanes* f)
{
  if (this->Frustum != f)
  {
    this->Frustum = f;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
vtkMTimeType vtkFrustumSelector::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType impFuncMTime;

  if (this->Frustum != nullptr)
  {
    impFuncMTime = this->Frustum->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkFrustumSelector::CreateFrustum(double verts[32])
{
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(6);

  vtkNew<vtkDoubleArray> norms;
  norms->SetNumberOfComponents(3);
  norms->SetNumberOfTuples(6);

  // left
  ComputePlane(0, &verts[0 * 4], &verts[2 * 4], &verts[3 * 4], points, norms);
  // right
  ComputePlane(1, &verts[7 * 4], &verts[6 * 4], &verts[4 * 4], points, norms);
  // bottom
  ComputePlane(2, &verts[5 * 4], &verts[4 * 4], &verts[0 * 4], points, norms);
  // top
  ComputePlane(3, &verts[2 * 4], &verts[6 * 4], &verts[7 * 4], points, norms);
  // near
  ComputePlane(4, &verts[6 * 4], &verts[2 * 4], &verts[0 * 4], points, norms);
  // far
  ComputePlane(5, &verts[1 * 4], &verts[3 * 4], &verts[7 * 4], points, norms);

  this->Frustum->SetPoints(points);
  this->Frustum->SetNormals(norms);
}

//------------------------------------------------------------------------------
void vtkFrustumSelector::Initialize(vtkSelectionNode* node)
{
  this->Superclass::Initialize(node);

  // sanity checks
  if (node && node->GetContentType() == vtkSelectionNode::FRUSTUM)
  {
    vtkDoubleArray* corners = vtkArrayDownCast<vtkDoubleArray>(node->GetSelectionList());
    this->CreateFrustum(corners->GetPointer(0));
  }
  else
  {
    vtkErrorMacro("Wrong type of selection node used to initialize vtkFrustumSelector");
  }
}

//------------------------------------------------------------------------------
bool vtkFrustumSelector::ComputeSelectedElements(
  vtkDataObject* input, vtkSignedCharArray* insidednessArray)
{
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);
  vtkHyperTreeGrid* inputHTG = vtkHyperTreeGrid::SafeDownCast(input);
  // frustum selection only supports datasets and HTGs
  // if we don't have a selection node, the frustum is uninitialized...
  if (!inputDS && !inputHTG)
  {
    vtkErrorMacro("Frustum selection only supports inputs of type vtkDataSet or vtkHypertreeGrid");
    return false;
  }
  if (!this->Node)
  {
    vtkErrorMacro("Frustum node selection is not set");
    return false;
  }
  auto fieldType = this->Node->GetProperties()->Get(vtkSelectionNode::FIELD_TYPE());
  if (fieldType == vtkSelectionNode::POINT)
  {
    if (inputHTG)
    {
      vtkErrorMacro("vtkHyperTreeGrids do not support point selection");
      return false;
    }
    this->ComputeSelectedPoints(inputDS, insidednessArray);
  }
  else if (fieldType == vtkSelectionNode::CELL)
  {
    if (inputHTG)
    {
      this->ComputeSelectedCells(inputHTG, insidednessArray);
    }
    else
    {
      this->ComputeSelectedCells(inputDS, insidednessArray);
    }
  }
  else
  {
    vtkErrorMacro("Frustum selection only supports POINT and CELL association types");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkFrustumSelector::ComputeSelectedPoints(vtkDataSet* input, vtkSignedCharArray* pointSelected)
{
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts == 0)
  {
    return;
  }

  // Hacky PrepareForMultithreadedAccess()
  // call everything we will call on the data object on the main thread first
  // so that it can build its caching structures
  double xx[3];
  input->GetPoint(0, xx);

  vtkSMPTools::For(0, numPts,
    [input, this, &pointSelected](vtkIdType begin, vtkIdType end)
    {
      double x[3];
      for (vtkIdType ptId = begin; ptId < end; ++ptId)
      {
        input->GetPoint(ptId, x);
        if ((this->Frustum->EvaluateFunction(x)) < 0.0)
        {
          pointSelected->SetValue(ptId, 1);
        }
        else
        {
          pointSelected->SetValue(ptId, 0);
        }
      }
    });
}

//------------------------------------------------------------------------------
void vtkFrustumSelector::ComputeSelectedCells(vtkDataSet* input, vtkSignedCharArray* cellSelected)
{
  vtkIdType numCells = input->GetNumberOfCells();

  if (numCells == 0)
  {
    return;
  }

  ComputeCellsInFrustumFunctor functor(this->Frustum, input, cellSelected);
  vtkSMPTools::For(0, numCells, functor);
}

//------------------------------------------------------------------------------
void vtkFrustumSelector::ComputeSelectedCells(
  vtkHyperTreeGrid* input, vtkSignedCharArray* cellSelected)
{
  vtkIdType numCells = input->GetNumberOfCells();
  if (numCells == 0)
  {
    return;
  }

  vtkIdType nTrees = input->GetMaxNumberOfTrees();
  ComputeHTGCellsInFrustumFunctor functor(this->Frustum, input, cellSelected);
  vtkSMPTools::For(0, nTrees, functor);
}

//------------------------------------------------------------------------------
void vtkFrustumSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Frustum: " << static_cast<void*>(this->Frustum) << "\n";
}

//------------------------------------------------------------------------------
int vtkFrustumSelector::OverallBoundsTest(double bounds[6])
{
  ComputeCellsInFrustumFunctor functor(this->Frustum, nullptr, nullptr);

  vtkNew<vtkGenericCell> vox;
  vox->SetCellType(VTK_VOXEL);
  vtkPoints* p = vox->GetPoints();
  p->SetPoint(0, bounds[0], bounds[2], bounds[4]);
  p->SetPoint(1, bounds[1], bounds[2], bounds[4]);
  p->SetPoint(2, bounds[0], bounds[3], bounds[4]);
  p->SetPoint(3, bounds[1], bounds[3], bounds[4]);
  p->SetPoint(4, bounds[0], bounds[2], bounds[5]);
  p->SetPoint(5, bounds[1], bounds[2], bounds[5]);
  p->SetPoint(6, bounds[0], bounds[3], bounds[5]);
  p->SetPoint(7, bounds[1], bounds[3], bounds[5]);

  FrustumPlanesType frustumPlanes;
  frustumPlanes.Initialize(this->Frustum);
  std::vector<double> vertexBuffer;
  return functor.ABoxFrustumIsect(-1, bounds, vox, frustumPlanes, vertexBuffer, true) > 0 ? 1 : 0;
}
VTK_ABI_NAMESPACE_END
