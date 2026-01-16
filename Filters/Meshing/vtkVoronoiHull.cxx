// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVoronoiHull.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTriangle.h"

#include <iomanip>
#include <iostream>

VTK_ABI_NAMESPACE_BEGIN

namespace // anonymous
{
//======= Some convenience methods.
// Evaluate the 3D plane equation for a given point x. Normal n is expected
// to be a unit normal to the plane; o is a plane origin (i.e., point on
// the plane).
double EvaluatePlane(double x[3], double o[3], double n[3])
{
  return (((x[0] - o[0]) * n[0]) + ((x[1] - o[1]) * n[1]) + ((x[2] - o[2]) * n[2]));
}

} // anonymous namespace

//------------------------------------------------------------------------------
void vtkVoronoiHull::Initialize(vtkIdType genPtId, const double genPt[3], double bds[6])
{
  // Update the generating point position and id, and the bounds. This
  // information may be needed later so record it.
  this->PtId = genPtId;
  this->X[0] = genPt[0];
  this->X[1] = genPt[1];
  this->X[2] = genPt[2];

  // Empty out the points and faces
  this->Clear();

  // Add the eight points of the initial bounding polyhedron (box).
  this->AddNewPoint(bds[0], bds[2], bds[4]); // point 0
  this->AddNewPoint(bds[1], bds[2], bds[4]); // point 1
  this->AddNewPoint(bds[0], bds[3], bds[4]); // point 2
  this->AddNewPoint(bds[1], bds[3], bds[4]); // point 3
  this->AddNewPoint(bds[0], bds[2], bds[5]); // point 4
  this->AddNewPoint(bds[1], bds[2], bds[5]); // point 5
  this->AddNewPoint(bds[0], bds[3], bds[5]); // point 6
  this->AddNewPoint(bds[1], bds[3], bds[5]); // point 7

  // Add the six outside faces of the initial boundng polyhedron
  // (box). Note that the "neighboring" points are outside of the bounding
  // box (indicated by <0 values), representing the infinite space bounding
  // the Voroni cell.
  int faceId = this->AddNewFace(4, -1); // face 0 of the polyhedron, marked as -1
  vtkHullFace* face = this->GetFace(faceId);
  this->AddFacePoint(face, 0);
  this->AddFacePoint(face, 4);
  this->AddFacePoint(face, 6);
  this->AddFacePoint(face, 2);

  faceId = this->AddNewFace(4, -2); // face 1
  face = this->GetFace(faceId);
  this->AddFacePoint(face, 1);
  this->AddFacePoint(face, 3);
  this->AddFacePoint(face, 7);
  this->AddFacePoint(face, 5);

  faceId = this->AddNewFace(4, -3); // face 2
  face = this->GetFace(faceId);
  this->AddFacePoint(face, 0);
  this->AddFacePoint(face, 1);
  this->AddFacePoint(face, 5);
  this->AddFacePoint(face, 4);

  faceId = this->AddNewFace(4, -4); // face 3
  face = this->GetFace(faceId);
  this->AddFacePoint(face, 2);
  this->AddFacePoint(face, 6);
  this->AddFacePoint(face, 7);
  this->AddFacePoint(face, 3);

  faceId = this->AddNewFace(4, -5); // face 4
  face = this->GetFace(faceId);
  this->AddFacePoint(face, 0);
  this->AddFacePoint(face, 2);
  this->AddFacePoint(face, 3);
  this->AddFacePoint(face, 1);

  faceId = this->AddNewFace(4, -6); // face 5
  face = this->GetFace(faceId);
  this->AddFacePoint(face, 4);
  this->AddFacePoint(face, 5);
  this->AddFacePoint(face, 7);
  this->AddFacePoint(face, 6);

  // Now update the faces connected to the the eight initial points.
  this->SetPointFaces(0, 0, 2, 4);
  this->SetPointFaces(1, 1, 2, 4);
  this->SetPointFaces(2, 0, 3, 4);
  this->SetPointFaces(3, 1, 3, 4);
  this->SetPointFaces(4, 0, 2, 5);
  this->SetPointFaces(5, 1, 2, 5);
  this->SetPointFaces(6, 0, 3, 5);
  this->SetPointFaces(7, 1, 3, 5);

  // Keep track of the number of Clip() operations.
  this->NumClips = 0;

  // This is used to prevent recomputing the circumflower unless
  // absolutely necessary.
  this->RecomputeCircumFlower = true;
  this->CircumFlower2 = VTK_FLOAT_MAX;
} // Initialize

/**
 * Insert the next point neighboring point p_j. The method will return
 * Intersection if the v_i is modified as a result of inserting the
 * point. The return value Pruned is returned when the resulting clip is
 * numerically small. Otherwise the v_i is not modified. Make sure that
 * Initialize() has been invoked prior to calling this method.
 */
ClipIntersectionStatus vtkVoronoiHull::Clip(vtkIdType neiPtId, const double neiPt[3])
{
  // Make sure the neighboring point is not topologically coincident.
  if (neiPtId == this->PtId)
  {
    return ClipIntersectionStatus::NoIntersection;
  }

  // Order the calculations to obtain the same result (i.e., face neighbors
  // compute the same result).
  double origin[3], normal[3];
  bool negate = false;
  if (neiPtId < this->PtId)
  {
    origin[0] = (neiPt[0] + this->X[0]) / 2.0;
    origin[1] = (neiPt[1] + this->X[1]) / 2.0;
    origin[2] = (neiPt[2] + this->X[2]) / 2.0;
    normal[0] = neiPt[0] - this->X[0];
    normal[1] = neiPt[1] - this->X[1];
    normal[2] = neiPt[2] - this->X[2];
  }
  else
  {
    origin[0] = (this->X[0] + neiPt[0]) / 2.0;
    origin[1] = (this->X[1] + neiPt[1]) / 2.0;
    origin[2] = (this->X[2] + neiPt[2]) / 2.0;
    normal[0] = this->X[0] - neiPt[0];
    normal[1] = this->X[1] - neiPt[1];
    normal[2] = this->X[2] - neiPt[2];
    negate = true;
  }

  // Make sure the neighboring point is not geometrically
  // coincident.
  double n = vtkMath::Normalize(normal);
  if (n <= 0)
  {
    return ClipIntersectionStatus::NoIntersection;
  }

  // Flip the normal if necessary.
  if (negate)
  {
    normal[0] = (-normal[0]);
    normal[1] = (-normal[1]);
    normal[2] = (-normal[2]);
  }

  // Now perform the plane clipping / intersection operation.
  ClipIntersectionStatus retStatus = this->IntersectWithPlane(origin, normal, neiPtId);

  // In the rare case of numeric issues, jitter the normal to compute different
  // approximations to the Voronoi tessellation.
  int numBumps = 0;
  while (retStatus == ClipIntersectionStatus::Numeric && numBumps < 12)
  {
    double bmpNormal[3];
    this->BumpNormal(numBumps, normal, bmpNormal);
    retStatus = this->IntersectWithPlane(origin, bmpNormal, neiPtId);
    numBumps++;
  }

  // Return the appropriate result.
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
} // Clip()

//------------------------------------------------------------------------------
void vtkVoronoiHull::BumpNormal(int bumpNum, double normal[3], double bumpNormal[3])
{
  // Make sure this operation is reproducible.
  if (bumpNum == 0)
  {
    this->Bumper.Seed(this->PtId);
  }

  // std::cout << "Normal  In: " << "(" << std::setprecision(16) << normal[0]
  //           << "," << std::setprecision(16) << normal[1]
  //           << "," << std::setprecision(16) << normal[2] << ")\n";

  // Find the maximum component of the current normal. Then randomly set the
  // other two components. Note that because the random range of the vector
  // components is [-1,1] with a mean of zero, we have to increase the bump
  // amount with an empirical factor.
  int iMax = (normal[0] > normal[1] ? 0 : 1);
  iMax = (normal[iMax] > normal[2] ? iMax : 2);
  double sum = 0.0;
  for (auto i = 0; i < 3; ++i)
  {
    if (i != iMax)
    {
      bumpNormal[i] = (2.0 * this->Bumper.Next() - 1.0) * this->PruneTolerance * 1e+4;
      sum += bumpNormal[i] * normal[i];
    }
  }
  bumpNormal[iMax] = sum / normal[iMax];
  bumpNormal[0] += normal[0];
  bumpNormal[1] += normal[1];
  bumpNormal[2] += normal[2];
  vtkMath::Normalize(bumpNormal);

  // std::cout << "Normal Out: " << "(" << std::setprecision(16) << bumpNormal[0]
  //           << "," << std::setprecision(16) << bumpNormal[1]
  //           << "," << std::setprecision(16) << bumpNormal[2] << ")\n";
}

//------------------------------------------------------------------------------
ClipIntersectionStatus vtkVoronoiHull::IntersectWithPlane(
  double origin[3], double normal[3], vtkIdType neiPtId)
{
  // Begin by evaluating all the polyhedron vertices against the clipping
  // plane. We need to determine the "length" of the polyhedron being clipped
  // to determine an adaptive tolerance.
  double val, minVal = 0, maxVal = 0;
  for (int ptId = 0; ptId < static_cast<int>(this->Points.size()); ++ptId)
  {
    vtkHullPoint& point = this->Points[ptId];
    if (point.Status == ProcessingStatus::Valid)
    {
      val = EvaluatePlane(point.X, origin, normal);
      minVal = std::min(val, minVal);
      maxVal = std::max(val, maxVal);
      point.Val = val;
    } // only process valid points
  }

  // Test the trivial case for no intersection. Note that if using InFlower()
  // tests, this return will not be invoked.
  if (maxVal <= 0)
  {
    return ClipIntersectionStatus::NoIntersection;
  }

  // Make sure the intersection is numerically sound. Recall that the
  // evaluated value (val) is the distance away from the clipping plane.
  // This is useful in that it provides a measure of the "length" of the
  // hull, so tolerances relative to this length can be used.  Based on the
  // prune tolerance, clips that just nick the hull can be discarded. This
  // significantly improves numerical stability of the hull
  // generation. However, it is possible that neighboring hulls don't
  // properly match up with one another, so that the necessary condition
  // of a symmetric adjacency graph are violated. This will be corrected
  // later during the validation process, where prunes producing hanging
  // spokes can be corrected by eliminating any hanging spokes.
  double len = (maxVal - minVal);
  if (len <= 0 || ((maxVal / len) <= this->PruneTolerance))
  {
    return ClipIntersectionStatus::Pruned;
  }

  // Prepare for the processing of intersected points and faces.
  this->InProcessPoints.clear();
  this->InProcessFaces.clear();
  this->InsertedEdgePoints.clear();
  this->RecomputePetals = true;

  // Revisit the evaluated points to assess whether there is a potential
  // intersection of faces connected to clipped points. Be wary of degenerate
  // points: while not common, degeneracies must be treated more carefully,
  // while also providing a fast path for non-degenerate situations. At the
  // same time, evaluate the connected faces to determine what operations
  // must be performed on the faces to perform the clip.
  double tol = len * this->PruneTolerance;
  for (int ptId = 0; ptId < static_cast<int>(this->Points.size()); ++ptId)
  {
    vtkHullPoint& point = this->Points[ptId];
    if (point.Status == ProcessingStatus::Valid)
    {
      val = point.Val;
      // Points inside the clip are kept.
      if (val < -tol)
      {
        ;
      }
      // If a point is outside the clip, it will be discarded. The faces
      // attached to the point require further processing.
      else if (val > tol)
      {
        if (!this->InProcessPoints.AddPoint(this, point, ptId))
        {
          return ClipIntersectionStatus::Numeric;
        }
      }
      // We avoid degenerate situations.
      else
      {
        return ClipIntersectionStatus::Numeric;
      }
    } // only process valid points
  }

  // Process those faces which are connected to a clipped point. Since we've
  // already determined what geometric operations need to be performed on
  // each face, and avoided degeneracies, we can now modify the hull (i.e.,
  // perform face operations). This ensures that the hull remains in a valid
  // state.
  for (auto& faceOp : this->InProcessFaces)
  {
    faceOp.Function(*this, faceOp.FaceId, faceOp.StartIdx, faceOp.NumKeptPts);
  }

  // Now build the new capping polygon from the edge / clipping plane
  // intersections.  All the intersection points must be circumferentially
  // ordered to create a final capping polygon. Fortunately, the polygon is
  // convex, so the ordering (i.e., assigning of loop index/order) can be
  // performed by counting the number of points on one side of the polygon
  // fan diagonals. (This cute method works well for small number of
  // vertices in the capping polygon--no sqrt nor atan functions, nor angle
  // sort. However, for large numbers of points, which is rare, the method
  // will not scale well and may require an alternative method.)

  // The number of vertices forming the capping polygon. A numeric situation
  // should never arise.
  int npts = static_cast<int>(this->InsertedEdgePoints.size());
  if (npts < 3)
  {
    return ClipIntersectionStatus::Numeric;
  }

  // The fan pivot point, assigned loop index 0. All diagonals across the
  // capping polygon start from the pivot point.
  this->InsertedEdgePoints[0].LoopIdx = 0;
  double* xO = this->Points[this->InsertedEdgePoints[0].Id].X;

  // Now create lines between the pivot point and other points in the
  // capping polygon. These lines define separation planes orthogonal to
  // the clipped polygon. By counting the number of points on one side of
  // the separation plane, we can assign a loop index value. Exit early
  // if we only have 3 points (a triangle), as the order is trivial (0,1,2).
  if (npts == 3)
  {
    this->InsertedEdgePoints[1].LoopIdx = 1;
    this->InsertedEdgePoints[2].LoopIdx = 2;
  }
  else
  {
    double *x, xxO[3], sepN[3];
    for (vtkIdType i = 1; i < npts; ++i)
    {
      // Compute the separation plane.
      x = this->Points[this->InsertedEdgePoints[i].Id].X;
      xxO[0] = x[0] - xO[0];
      xxO[1] = x[1] - xO[1];
      xxO[2] = x[2] - xO[2];
      vtkMath::Cross(xxO, normal, sepN);

      // Now evaluate all other points against the separation plane. We
      // don't worry about normalizing the separation plane normal because
      // we just counting values >0.
      int idx = 1; // the index is one to begin with.
      for (int j = 1; j < npts; ++j)
      {
        if (i == j)
        {
          continue;
        }
        x = this->Points[this->InsertedEdgePoints[j].Id].X;
        if (EvaluatePlane(x, xO, sepN) > 0)
        {
          idx++;
        }
      } // point evaluation

      // Assign loop index
      this->InsertedEdgePoints[i].LoopIdx = idx;
    } // for all non-origin points forming the capping polygon.
  }   // more than 3 points in the face

  // Finally, create a new face (the capping polygon) with the points
  // inserted in the correct order (using the loop index).
  int newFaceId = this->AddNewFace(npts, neiPtId);
  vtkHullFace* newFace = this->GetFace(newFaceId);
  for (int i = 0; i < npts; ++i)
  {
    this->AddNthFacePoint(
      newFace, this->InsertedEdgePoints[i].LoopIdx, this->InsertedEdgePoints[i].Id);
    this->SetPointFaces(this->InsertedEdgePoints[i].Id, this->InsertedEdgePoints[i].Faces[0],
      this->InsertedEdgePoints[i].Faces[1], newFaceId);
  }

  // Clean up, and delete the clipped points. The deletion process is deferred to the
  // end of the clipping process so we don't replace a point (memory recovery) while
  // in the middle of processing.
  for (auto& ptId : this->InProcessPoints)
  {
    this->DeletePoint(ptId);
  }

  // Successful intersection has been performed.
  return ClipIntersectionStatus::Intersection;
} // IntersectWithPlane()

//------------------------------------------------------------------------------
int vtkVoronoiHull::IntersectFaceEdge(int faceId, int p0, int p1)
{
  // See if a previous edge intersection has been found when processing
  // another face. Use direct linear search for small numbers of edge
  // points.
  int pid = 0;
  bool found = false;
  vtkHullEdgeTuple et(p0, p1);
  if (this->InsertedEdgePoints.size() < 12)
  {
    for (auto& ep : this->InsertedEdgePoints)
    {
      if (ep == et)
      {
        pid = ep.Id;
        ep.Faces[1] = faceId; // this is the second face using the edge
        found = true;
        break;
      }
    }
  } // small set search
  else
  {
    // Use std::find for larger sets
    InsertedEdgePointsArray::iterator insItr =
      std::find(this->InsertedEdgePoints.begin(), this->InsertedEdgePoints.end(), et);
    if (insItr != this->InsertedEdgePoints.end())
    {
      pid = insItr->Id;
      insItr->Faces[1] = faceId; // this is the second face using the edge
      found = true;
    }
  } // large edge intersection set search

  if (!found) // Intersect the edge and create a new point.
  {
    double x[3];
    double val0 = this->Points[p0].Val;
    double val1 = this->Points[p1].Val;
    double t = -val0 / (val1 - val0);
    x[0] = this->Points[p0].X[0] + t * (this->Points[p1].X[0] - this->Points[p0].X[0]);
    x[1] = this->Points[p0].X[1] + t * (this->Points[p1].X[1] - this->Points[p0].X[1]);
    x[2] = this->Points[p0].X[2] + t * (this->Points[p1].X[2] - this->Points[p0].X[2]);
    pid = this->AddNewPoint(x);
    this->InsertedEdgePoints.emplace_back(p0, p1);
    this->InsertedEdgePoints.back().Id = pid;
    this->InsertedEdgePoints.back().Faces[0] = faceId; // first face using this edge
  }

  return pid;
} // IntersectFaceEdge()

//------------------------------------------------------------------------------
void vtkVoronoiHull::RebuildFace(int faceId, int startIdx, int numKeptPts)
{
  // Modify this face's connectivity list (point ids) to include the two
  // new clipping intersection points, and include all interior
  // (non-clipped) points.
  vtkHullFace* face = this->GetFace(faceId);
  int npts = face->NumPts;
  this->FaceIdsBuffer.clear();

  // Intersect the first edge to create a new point.
  int p0 = this->GetFacePoint(face, startIdx);
  int ip = ((startIdx + 1) == npts ? 0 : (startIdx + 1));
  int p1 = this->GetFacePoint(face, ip);
  int pid = this->IntersectFaceEdge(faceId, p0, p1);
  this->FaceIdsBuffer.emplace_back(pid);

  // Add the existing interior points
  this->FaceIdsBuffer.emplace_back(p1);
  for (int i = 2; i <= numKeptPts; ++i)
  {
    ip = (startIdx + i) % npts;
    p1 = this->GetFacePoint(face, ip);
    this->FaceIdsBuffer.emplace_back(p1);
  }

  // Intersect the second edge to create a new point.
  p0 = p1;
  ip = (startIdx + numKeptPts + 1) % npts;
  p1 = this->GetFacePoint(face, ip);
  pid = this->IntersectFaceEdge(faceId, p0, p1);
  this->FaceIdsBuffer.emplace_back(pid);

  // Copy the list of point ids from the face ids buffer into the current
  // face points ids.
  this->RebuildFacePoints(face, this->FaceIdsBuffer);
  face->Status = ProcessingStatus::Valid;
} // RebuildFace()

//------------------------------------------------------------------------------
void vtkVoronoiHull::ProduceFacePolyData(vtkPolyData* pd, vtkHullFace* face)
{
  // Make sure the valid points are numbered.
  this->MapPoints();

  // Grab some face information
  int npts = face->NumPts;

  // We'll produce a single (face) polygon, points, and scalars from
  // the current evaluation of the clip plane.

  // Produce the points
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(npts);
  for (int i = 0; i < npts; ++i)
  {
    int ptId = this->GetFacePoint(face, i);
    auto& pitr = this->Points[ptId];
    points->SetPoint(i, pitr.X);
  }

  // Produce the scalars
  vtkNew<vtkDoubleArray> faceVals;
  faceVals->SetNumberOfTuples(npts);
  for (int i = 0; i < npts; ++i)
  {
    int ptId = this->GetFacePoint(face, i);
    auto& pitr = this->Points[ptId];
    faceVals->SetTuple1(i, pitr.Val);
  }

  // Produce the face
  vtkNew<vtkCellArray> faces;
  faces->InsertNextCell(npts);
  for (auto i = 0; i < npts; ++i)
  {
    faces->InsertCellPoint(i);
  }

  pd->SetPoints(points);
  pd->SetPolys(faces);
  pd->GetPointData()->SetScalars(faceVals);
} // ProduceFacePolyData

//------------------------------------------------------------------------------
void vtkVoronoiHull::ProducePolyData(vtkPolyData* pd)
{
  // Make sure the valid points are numbered.
  this->MapPoints();

  // Produce the points
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(this->NumPts);
  for (auto& pitr : this->Points)
  {
    if (pitr.Status == ProcessingStatus::Valid)
    {
      points->SetPoint(pitr.PtMap, pitr.X);
    }
  }

  // Produce the faces
  vtkNew<vtkCellArray> faces;
  for (auto& fitr : this->Faces)
  {
    if (fitr.Status == ProcessingStatus::Valid)
    {
      faces->InsertNextCell(fitr.NumPts);
      for (auto i = 0; i < fitr.NumPts; ++i)
      {
        vtkIdType pid = this->GetFacePoint(&fitr, i);
        faces->InsertCellPoint(this->Points[pid].PtMap);
      }
    }
  }

  pd->SetPoints(points);
  pd->SetPolys(faces);
} // ProducePolyData()

VTK_ABI_NAMESPACE_END
