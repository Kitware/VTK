// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVoronoiTile.h"

#include "vtkCellArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSpheres.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkVoronoiTile::Initialize(vtkIdType genPtId, const double genPt[3], double bds[4])
{
  // The generating tile point.
  this->PtId = genPtId;

  // The generating point coordinates for the Voronoi tile.
  this->X[0] = genPt[0];
  this->X[1] = genPt[1];

  // Initialize the number of clips
  this->NumClips = 0;

  // Make sure that the tile is reset (if used multiple times as for
  // example in multiple threads).
  this->Points.clear();

  // Now for each of the corners of the bounding box, add a tile
  // vertex. Note this is done in counterclockwise ordering. The initial
  // generating point id (<0, [-4,-1]) means that this point is on the
  // boundary. The numbering (-1,-2,-3,-4) corresponds to the top, lhs,
  // bottom, and rhs edges of the bounding box - useful for debugging
  // and trimming the Voronoi Flower when on the boundary.
  double v[2];
  v[0] = bds[1];
  v[1] = bds[3];
  this->Points.emplace_back(this->X, v, (-1));

  v[0] = bds[0];
  v[1] = bds[3];
  this->Points.emplace_back(this->X, v, (-2));

  v[0] = bds[0];
  v[1] = bds[2];
  this->Points.emplace_back(this->X, v, (-3));

  v[0] = bds[1];
  v[1] = bds[2];
  this->Points.emplace_back(this->X, v, (-4));

  // This is used to prevent recomputing the circumflower unless
  // necessary.
  this->RecomputeCircumFlower = true;
  this->CircumFlower2 = VTK_FLOAT_MAX;
}

//------------------------------------------------------------------------------
void vtkVoronoiTile::Initialize(
  vtkIdType genPtId, const double x[3], vtkPoints* pts, vtkIdType nPts, const vtkIdType* p)
{
  // The generating tile point
  this->PtId = genPtId;

  // The generating point for the Voronoi tile
  this->X[0] = x[0];
  this->X[1] = x[1];

  // Initialize the number of clips
  this->NumClips = 0;

  // Make sure that the tile is reset.
  this->Points.clear();

  // Now for each of the points of the polygon, insert a vertex. The initial point
  // id <0 corresponds to the N points of the polygon.
  double v[3];
  for (vtkIdType i = 0; i < nPts; ++i)
  {
    pts->GetPoint(p[i], v);
    this->Points.emplace_back(this->X, v, ((i + 1) * (-1)));
  }

  // Control circumflower recomputation.
  this->RecomputeCircumFlower = true;
  this->CircumFlower2 = VTK_FLOAT_MAX;
}

/**
 * Insert the next point neighboring point p_j. The method will return
 * Intersect if the v_i is modified as a result of inserting the point. The
 * return value Pruned is returned when the resulting clip is numerically
 * small. Otherwise the v_i is not modified. Make sure that Initialize() has
 * been invoked prior to calling this method.
 */
ClipIntersectionStatus vtkVoronoiTile::Clip(vtkIdType neiPtId, const double neiPt[2])
{
  // Make sure the neighboring point is not topologically coincident.
  if (neiPtId == this->PtId)
  {
    return ClipIntersectionStatus::NoIntersection;
  }

  // Order the calculations to obtain the same result.
  double origin[2], normal[2];
  bool negate = false;
  if (neiPtId < this->PtId)
  {
    origin[0] = (neiPt[0] + this->X[0]) / 2.0;
    origin[1] = (neiPt[1] + this->X[1]) / 2.0;
    normal[0] = neiPt[0] - this->X[0];
    normal[1] = neiPt[1] - this->X[1];
  }
  else
  {
    origin[0] = (this->X[0] + neiPt[0]) / 2.0;
    origin[1] = (this->X[1] + neiPt[1]) / 2.0;
    normal[0] = this->X[0] - neiPt[0];
    normal[1] = this->X[1] - neiPt[1];
    negate = true;
  }

  // Make sure the neighboring point is not geometrically
  // coincident.
  double n = vtkMath::Normalize2D(normal);
  if (n <= 0)
  {
    return ClipIntersectionStatus::NoIntersection;
  }

  // Flip the normal if necessary.
  if (negate)
  {
    normal[0] = (-normal[0]);
    normal[1] = (-normal[1]);
  }

  // Now perform the plane clipping / intersection operation.
  ClipIntersectionStatus retStatus = this->IntersectWithLine(origin, normal, neiPtId);
  if (retStatus == ClipIntersectionStatus::Intersection)
  {
    // Update the number of successful clips
    this->NumClips++;
    return ClipIntersectionStatus::Intersection;
  }
  else if (retStatus == ClipIntersectionStatus::Pruned)
  {
    return ClipIntersectionStatus::Pruned;
  }
  else
  {
    return ClipIntersectionStatus::NoIntersection;
  }
} // Clip

//------------------------------------------------------------------------------
ClipIntersectionStatus vtkVoronoiTile::IntersectWithLine(
  double origin[2], double normal[2], vtkIdType neiPtId)
{
  // Evaluate all the points of the convex polygon. Positive valued points
  // are eventually clipped away from the tile.
  double val, minVal = 0, maxVal = 0;
  for (auto& p : this->Points)
  {
    val = vtkLine::Evaluate(normal, origin, p.X);
    minVal = std::min(val, minVal);
    maxVal = std::max(val, maxVal);
    p.Val = val;
  }

  // Test the trivial case for no intersection. Note that if using
  // InFlower() tests, this return should not be called.
  if (maxVal <= 0)
  {
    return ClipIntersectionStatus::NoIntersection;
  }

  // Make sure the intersection is numerically sound. Recall that the
  // evaluated values are the distance away from the clipping line.
  // This is useful in that it provides a measure of the "length" of
  // the tile, so tolerances relative to this length can be used.
  // Based on the prune tolerance, clips that just nick the tile can
  // be discarded. This significantly improves numerical stability of
  // the tile generation. Later, during the validation process, prunes
  // can be corrected by eliminating hanging spokes.
  double len = (maxVal - minVal);
  if (len <= 0 || ((maxVal / len) <= this->PruneTolerance))
  {
    return ClipIntersectionStatus::Pruned;
  }

  // An intersection has occurred.  The tile intersects the half-space
  // line. Add the remaining tile vertices and new intersection points to
  // modify the tile. Care is taken to preserve the counterclockwise vertex
  // ordering.
  this->NewPoints.clear();
  for (auto p = this->Points.begin(); p != this->Points.end(); ++p)
  {
    // If the vertex is inside the clip, just add it. Otherwise, see
    // how it affects the circumflower.
    if (p->Val <= 0.0)
    {
      this->NewPoints.emplace_back(*p);
    }
    else if ((4 * p->R2) >= this->CircumFlower2)
    {
      this->RecomputeCircumFlower = true;
    }

    // Now see if the edge requires clipping. If so, create a new tile
    // vertex. Note that depending on the order of edge, the new vertex
    // has to be treated differently (i.e., the neigboring tile id).
    double t, x[2];
    PointRingIterator pNext = this->Next(p);
    if ((p->Val <= 0.0 && pNext->Val > 0.0) || (p->Val > 0.0 && pNext->Val <= 0.0))
    {
      t = (-p->Val) / (pNext->Val - p->Val);
      x[0] = p->X[0] + t * (pNext->X[0] - p->X[0]);
      x[1] = p->X[1] + t * (pNext->X[1] - p->X[1]);
      vtkIdType pId = (p->Val < 0.0 ? neiPtId : p->NeiId);
      this->NewPoints.emplace_back(this->X, x, pId);
    } // check for intersecting edge
  }   // clip verts & edges

  // Now just swap the newly added vertices to update the tile.
  this->Points.swap(this->NewPoints);

  return ClipIntersectionStatus::Intersection;
} // IntersectWithLine

//------------------------------------------------------------------------------
void vtkVoronoiTile::ProducePolyData(vtkPolyData* pd, vtkSpheres* spheres)
{
  vtkIdType numPts = this->GetNumberOfPoints();

  // Produce the points
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPts);

  // Produce a single tile
  vtkNew<vtkCellArray> tile;
  tile->InsertNextCell(numPts);

  // Produce radii attribute data
  vtkNew<vtkDoubleArray> radii;
  radii->SetNumberOfTuples(numPts);
  radii->SetName("Voronoi Flower Radii");

  // Populate the data
  vtkIdType ptId = 0;
  for (const auto& v : this->Points)
  {
    points->SetPoint(ptId, v.X[0], v.X[1], this->X[2]);
    tile->InsertCellPoint(ptId);
    double r = sqrt(vtkMath::Distance2BetweenPoints2D(v.X, this->X));
    radii->SetTuple1(ptId++, r);
  }

  pd->SetPoints(points);
  pd->SetPolys(tile);
  pd->GetPointData()->SetScalars(radii);

  // Optional implicit function.
  if (spheres)
  {
    spheres->SetCenters(points);
    spheres->SetRadii(radii);
  }
} // ProducePolyData

//------------------------------------------------------------------------------
// Update the flower petals which are passed off to the locator.
// Only petals which exend past the minimal radius of the annular
// request are added to the list of petals. It is presumed that
// UpdateCircumFlower() has been invoked previously.
void vtkVoronoiTile::UpdatePetals(double cf2)
{
  // If the radii of the flower circles (petals) is highly variable (which
  // occurs when the spacing of points is highly variable), then there is
  // likely a lot of empty search space. Only add flower petals which extend
  // past the outer shell request boundary. These petals are used to further
  // limit the point search space.
  this->Petals->Reset();
  this->RecomputePetals = false; // petals will be updated in the following

  constexpr double CircleRatio = 2.5, CircleRatio2 = CircleRatio * CircleRatio;
  if (this->MinRadius2 > 0 && ((this->MaxRadius2 / this->MinRadius2) < CircleRatio2))
  {
    return; // it's not worth using the petals
  }

  // Empirically determined
  constexpr double LargeCircleRatio = 0.4;
  int maxLargeCircles = LargeCircleRatio * this->GetNumberOfPoints();

  this->SortP.clear();
  double minR2 = VTK_FLOAT_MAX, maxR2 = VTK_FLOAT_MIN;
  for (auto& v : this->Points)
  {
    // (2*R)**2 >= shell request radius**2
    if ((4 * v.R2) >= cf2)
    {
      minR2 = std::min(v.R2, minR2);
      maxR2 = std::max(v.R2, maxR2);
      this->SortP.emplace_back(&v);
    }
  }

  if (static_cast<int>(this->SortP.size()) > maxLargeCircles || (maxR2 / minR2) < CircleRatio2)
  {
    return; // it's not worth using the petals
  }
  else // sort from large circles to small
  {
    std::sort(this->SortP.begin(), this->SortP.end(),
      [](const vtkTilePoint* p0, const vtkTilePoint* p1) { return (p0->R2 > p1->R2); });
    for (auto& pt : this->SortP)
    {
      this->Petals->InsertNextTuple3(pt->X[0], pt->X[1], pt->R2);
    }
  } // it's worth using large circle culling
} // UpdatePetals

VTK_ABI_NAMESPACE_END
