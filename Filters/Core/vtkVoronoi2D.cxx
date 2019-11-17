/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoronoi2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVoronoi2D.h"

#include "vtkAbstractTransform.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDelaunay2D.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSpheres.h"
#include "vtkStaticPointLocator2D.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"

#include <vector>

vtkStandardNewMacro(vtkVoronoi2D);
vtkCxxSetObjectMacro(vtkVoronoi2D, Transform, vtkAbstractTransform);

//----------------------------------------------------------------------------
namespace
{

// Evaluate 2D line equation. Normal n is expected to be a unit normal. The
// point o is a point on the line (typically midpoint between two Voronoi
// points).
double EvaluateLine(double x[2], double o[2], double n[2])
{
  return ((x[0] - o[0]) * n[0] + (x[1] - o[1]) * n[1]);
}

// Determine the angle (in radians) of the point x around tile generating
// point tileX.
double EvaluateTheta(double x[2], double tileX[2])
{
  double t = atan2((x[1] - tileX[1]), (x[0] - tileX[0]));
  return (t >= 0 ? t : (2.0 * vtkMath::Pi() + t));
}

// The data structure for representing a Voronoi tile vertex. This implicitly defines
// a ray from the tile generating point to the vertex. It also carries
// information about how the half-space was produced. Namely, the point id
// that when combined with the tile's point id, produced the convex edge to the
// left of this vertex ray (i.e., in the counterclockwise direction).
struct VVertex
{
  vtkIdType PointId; // generating point id (across from tile edge)
  double Theta;      // angle around generating point
  double X[2];       // position of this vertex
  double Val;        // later used to evaluate half-space function

  VVertex(vtkIdType ptId, double tileX[2], double x[2])
    : PointId(ptId)
    , Val(0.0)
  {
    this->Theta = EvaluateTheta(x, tileX);
    this->X[0] = x[0];
    this->X[1] = x[1];
  }
};

// Types defined for convenience.
typedef std::vector<VVertex> VertexRingType;
typedef std::vector<VVertex>::iterator VertexRingIterator;

// Method supports sorting points around the tile ring using the angle theta.
bool VVertexCompare(const VVertex& a, const VVertex& b)
{
  return (a.Theta < b.Theta);
}

// Convex Voronoi tile represented by an ordered (counterclockwise) ring of
// vertices.
struct VTile
{
  vtkIdType NPts;                   // total number of points in dataset
  vtkIdType PointId;                // generating tile point id (in tile)
  double TileX[2];                  // generating tile point - x-y coordinates
  VertexRingType Verts;             // ordered loop of vertices (ordered in theta)
  vtkStaticPointLocator2D* Locator; // locator
  double PaddedBounds[4];           // the domain over which Voronoi is calculated
  double Bounds[4];                 // locator bounds
  int Divisions[2];                 // locator binning dimensions
  double H[2];                      // locator spacing
  double BucketRadius;              // diagonal length of any bucket
  int SpiralOrigin[2];              // beginning of clipping spiral iterator
  int SpiralX, SpiralY;             // current spiral location
  int SpiralDelX, SpiralDelY;       // current spiral delta
  int FMinIJ[2], FMaxIJ[2];         // Voronoi Flower rectangular footprint

  // Instantiate with initial values. Typically tiles consist of 5 to 6
  // vertices. Preallocate for performance.
  VTile()
    : PointId(-1)
    , Locator(nullptr)
  {
    this->TileX[0] = 0.0;
    this->TileX[1] = 0.0;
    this->Verts.reserve(24);
    // Suppress paternalistic compiler warnings
    this->PaddedBounds[0] = this->PaddedBounds[1] = 0.0;
    this->PaddedBounds[2] = this->PaddedBounds[3] = 0.0;
    this->Bounds[0] = this->Bounds[1] = 0.0;
    this->Bounds[2] = this->Bounds[3] = 0.0;
    this->Divisions[0] = this->Divisions[1] = 0;
    this->H[0] = this->H[1] = this->BucketRadius = 0.0;
    this->SpiralOrigin[0] = this->SpiralOrigin[1] = 0;
    this->SpiralX = this->SpiralY = this->SpiralDelX = this->SpiralDelY = 0;
    this->FMinIJ[0] = this->FMinIJ[1] = 0;
    this->FMaxIJ[0] = this->FMaxIJ[1] = 0;
  }

  // Initialize with a generating point - the resulting tile is just the
  // bounds rectangle, i.e., the four corners of the padded bounds defining
  // the tile. The points are added in increasing theta in counterclockwise
  // order.
  void Initialize(vtkIdType ptId, const double x[2])
  {
    // The generating tile point
    this->PointId = ptId;

    // The generating point for the Voronoi tile.
    this->TileX[0] = x[0];
    this->TileX[1] = x[1];

    // Make sure that the tile is reset (if used multiple times as for
    // example in multiple threads)
    this->Verts.clear();

    // Now for each of the corners of the bounding box, add a tile
    // vertex. Note this is done in increasing (counterclockwise) theta
    // ordering. The initial (-1) generating point id means that this point
    // is on the boundary.
    double v[2], *bds = this->PaddedBounds;
    v[0] = bds[1];
    v[1] = bds[3];
    this->Verts.emplace_back(VVertex((-1), this->TileX, v));

    v[0] = bds[0];
    v[1] = bds[3];
    this->Verts.emplace_back(VVertex((-1), this->TileX, v));

    v[0] = bds[0];
    v[1] = bds[2];
    this->Verts.emplace_back(VVertex((-1), this->TileX, v));

    v[0] = bds[1];
    v[1] = bds[2];
    this->Verts.emplace_back(VVertex((-1), this->TileX, v));
  }

  // Initialize with a convex polygon. The points are in counterclockwise order
  // (normal in the z-direction).
  void Initialize(
    vtkIdType ptId, const double x[2], vtkPoints* pts, vtkIdType nPts, const vtkIdType* p)
  {
    // The generating tile point
    this->PointId = ptId;

    // The generating point for the Voronoi tile
    this->TileX[0] = x[0];
    this->TileX[1] = x[1];

    // Make sure that the tile is reset.
    this->Verts.clear();

    // Now for each of the points of the polygon, insert a vertex.
    double v[3];
    for (vtkIdType i = 0; i < nPts; ++i)
    {
      pts->GetPoint(p[i], v);
      this->Verts.emplace_back(VVertex((-1), this->TileX, v));
    }
  }

  // Convenience methods for moving around the modulo ring of the vertices.
  VertexRingIterator Previous(VertexRingIterator itr)
  {
    if (itr == this->Verts.begin())
    {
      return this->Verts.end() - 1;
    }
    return (itr - 1);
  }
  VertexRingIterator Next(VertexRingIterator itr)
  {
    if (itr == (this->Verts.end() - 1))
    {
      return this->Verts.begin();
    }
    return (itr + 1);
  }

  // Indicate whether the point provided would produce a half-space that
  // would intersect the tile. See also InFlower(x) which is an equivalent
  // computation.
  bool IntersectTile(double x[2])
  {
    // Produce the half-space
    double o[2], normal[2];
    o[0] = (x[0] + this->TileX[0]) / 2.0;
    o[1] = (x[1] + this->TileX[1]) / 2.0;
    normal[0] = x[0] - this->TileX[0];
    normal[1] = x[1] - this->TileX[1];
    vtkMath::Normalize2D(normal);

    // Evaluate all the points of the convex polygon. Positive values indicate
    // an intersection occurs.
    VertexRingIterator tPtr;
    for (tPtr = this->Verts.begin(); tPtr != this->Verts.end(); ++tPtr)
    {
      if (EvaluateLine(tPtr->X, o, normal) >= 0.0)
      {
        return true;
      }
    }
    return false;
  }

  // Populate a polydata with the tile. Used to produce output / for
  // debugging.
  void PopulatePolyData(vtkPoints* centers, vtkCellArray* tile, vtkDoubleArray* radii)
  {
    vtkIdType nPts = static_cast<vtkIdType>(this->Verts.size());
    centers->SetNumberOfPoints(nPts);
    radii->SetNumberOfTuples(nPts);
    tile->InsertNextCell(static_cast<int>(nPts));

    vtkIdType i;
    double r;
    VertexRingIterator tPtr;
    for (i = 0, tPtr = this->Verts.begin(); tPtr != this->Verts.end(); ++tPtr, ++i)
    {
      centers->SetPoint(i, tPtr->X[0], tPtr->X[1], 0.0);
      r = sqrt((tPtr->X[0] - this->TileX[0]) * (tPtr->X[0] - this->TileX[0]) +
        (tPtr->X[1] - this->TileX[1]) * (tPtr->X[1] - this->TileX[1]));
      radii->SetTuple1(i, r);
      tile->InsertCellPoint(i);
    }
  }

  // This error measure is based on whether the spiral iterator has "covered"
  // the rectangular footprint of the Voronoi flower. Returns true if the
  // Voronoi flower has been covered; otherwise false. Assumed that the flower
  // footprint has been updated with UpdateFlowerFootprint().
  bool IsFlowerCovered()
  {
    // The coverage (rectangular footprint) of the spiral iterator is related
    // to the current level (distance from origin). We use a conservative
    // footprint, in other words the spiral iterator may have covered more
    // than a complete level (a level is effectivelt a rotation around the
    // spiral origin).
    int iDist = abs(this->SpiralX);
    int jDist = abs(this->SpiralY);
    int level = (iDist >= jDist ? iDist : jDist) - 1;
    level = (level < 0 ? 0 : level);
    int sMinIJ[2], sMaxIJ[2];
    sMinIJ[0] = this->SpiralOrigin[0] - level;
    sMinIJ[1] = this->SpiralOrigin[1] - level;
    sMaxIJ[0] = this->SpiralOrigin[0] + level;
    sMaxIJ[1] = this->SpiralOrigin[1] + level;

    // If the spiral iterator covers the flower footprint then we are done.
    if (sMinIJ[0] <= this->FMinIJ[0] && this->FMaxIJ[0] <= sMaxIJ[0] &&
      sMinIJ[1] <= this->FMinIJ[1] && this->FMaxIJ[1] <= sMaxIJ[1])
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  // Call to update the Voronoi flower footprint.
  void UpdateFlowerFootprint()
  {
    // For the footprint of the flower we take the union of the circles
    // composing the Voronoi flower. Then determine the minimum and maximum,
    // eventually converting this to locator space.
    double r, min, max, xMin[2], xMax[2];
    xMin[0] = xMin[1] = VTK_FLOAT_MAX;
    xMax[0] = xMax[1] = VTK_FLOAT_MIN;
    VertexRingIterator tPtr;
    for (tPtr = this->Verts.begin(); tPtr != this->Verts.end(); ++tPtr)
    {
      r = sqrt((tPtr->X[0] - this->TileX[0]) * (tPtr->X[0] - this->TileX[0]) +
        (tPtr->X[1] - this->TileX[1]) * (tPtr->X[1] - this->TileX[1]));

      min = tPtr->X[0] - r;
      max = tPtr->X[0] + r;
      xMin[0] = (min < xMin[0] ? min : xMin[0]);
      xMax[0] = (max > xMax[0] ? max : xMax[0]);
      min = tPtr->X[1] - r;
      max = tPtr->X[1] + r;
      xMin[1] = (min < xMin[1] ? min : xMin[1]);
      xMax[1] = (max > xMax[1] ? max : xMax[1]);
    }

    // Define the rectangular footprint
    this->Locator->GetBucketIndices(xMin, this->FMinIJ);
    this->Locator->GetBucketIndices(xMax, this->FMaxIJ);
  }

  // Clip the convex tile with a half-space line. Return whether there was
  // successful clip or not (1 or 0).  The line is represented by an origin
  // and unit normal.
  int ClipTile(vtkIdType ptId, const double p[3], double vtkNotUsed(tol))
  {
    // Create half-space
    double origin[2], normal[2];
    origin[0] = (p[0] + this->TileX[0]) / 2.0;
    origin[1] = (p[1] + this->TileX[1]) / 2.0;
    normal[0] = p[0] - this->TileX[0];
    normal[1] = p[1] - this->TileX[1];
    vtkMath::Normalize2D(normal);

    // Evaluate all the points of the convex polygon. Positive values are
    // eventually clipped away from the tile.
    bool intersection;
    VertexRingIterator tPtr;
    std::size_t i, nSectors = this->Verts.size();
    for (intersection = false, tPtr = this->Verts.begin(); tPtr != this->Verts.end(); ++tPtr)
    {
      tPtr->Val = EvaluateLine(tPtr->X, origin, normal);
      intersection = (tPtr->Val >= 0.0 ? true : intersection);
    }
    if (!intersection)
    {
      return 0;
    }

    // Now march around tile identifying any point in the clipped
    // region (half-space value >= 0.0).
    //
    for (i = 0, tPtr = this->Verts.begin(); tPtr->Val < 0.0 && i < nSectors; ++tPtr, ++i)
    {
    }
    if (i >= nSectors) // sanity check
    {
      return 0; // something really bad has happened
    }

    // Get the segments to perform the clipping on. The clipped segments
    // are bracketed by (tPLeft,tLeft) and (tRight,tMRight)
    VertexRingIterator tRight, tMRight, tLeft, tPLeft;
    tMRight = tPLeft = tPtr;
    for (; tPLeft->Val >= 0.0; tPLeft = this->Next(tPLeft))
    {
    }
    tLeft = this->Previous(tPLeft);

    for (; tMRight->Val >= 0.0; tMRight = this->Previous(tMRight))
    {
    }
    tRight = this->Next(tMRight);

    // Now intersect the segments bracketing the sign change and prepare the
    // data needed to insert two new points.
    double tL, tR, xL[2], xR[2];
    tL = (-tLeft->Val) / (tPLeft->Val - tLeft->Val);
    tR = (-tMRight->Val) / (tRight->Val - tMRight->Val);
    xL[0] = tLeft->X[0] + tL * (tPLeft->X[0] - tLeft->X[0]);
    xL[1] = tLeft->X[1] + tL * (tPLeft->X[1] - tLeft->X[1]);
    xR[0] = tMRight->X[0] + tR * (tRight->X[0] - tMRight->X[0]);
    xR[1] = tMRight->X[1] + tR * (tRight->X[1] - tMRight->X[1]);

    // Clipping must be done carefully to ensure that the sorted theta
    // ordering is preserved. Here we use a very simple but slightly
    // inefficient way to produce the final clipped polygon. Clipped points
    // are marked with large theta values, and the new intersection points
    // (with proper theta values) are inserted at the end of the vector. Then
    // the points are sorted around the polygon into counterclockwise
    // order. Finally, the clipped points are easily removed from the ring
    // (as they have been sorted into the last position(s)).
    std::size_t numToDelete;
    for (numToDelete = 0, tPtr = this->Next(tMRight); tPtr != tPLeft; tPtr = this->Next(tPtr))
    {
      numToDelete++;
      tPtr->Theta = VTK_FLOAT_MAX;
    }
    this->Verts.emplace_back(VVertex(ptId, this->TileX, xR));
    this->Verts.emplace_back(VVertex(tLeft->PointId, this->TileX, xL));
    std::sort(this->Verts.begin(), this->Verts.end(), VVertexCompare);
    for (i = 0; i < numToDelete; ++i)
    {
      this->Verts.pop_back();
    }

    return 1;
  }

  // Initialize spiraling traversal of the locator. Retain the starting
  // position and initial state.
  void InitSpiral(int ij[2])
  {
    this->SpiralOrigin[0] = ij[0];
    this->SpiralOrigin[1] = ij[1];
    this->SpiralX = 0;
    this->SpiralY = 0;
    this->SpiralDelX = 0;
    this->SpiralDelY = -1;
  }

  // Support spiraling traversal of the locator. Note that portions of the
  // spiral may extend past the boundaries of the locator. These buckets are
  // marked as "invalid" i.e., bucketId=(-1) so late no attempt will be made
  // to process the bucket.
  vtkIdType NextSpiralBucket(int ij[2])
  {
    if (this->SpiralX == this->SpiralY || (this->SpiralX < 0 && this->SpiralX == -this->SpiralY) ||
      (this->SpiralX > 0 && this->SpiralX == (1 - this->SpiralY)))
    {
      int temp = this->SpiralDelX;
      this->SpiralDelX = -this->SpiralDelY;
      this->SpiralDelY = temp;
    }

    this->SpiralX += this->SpiralDelX;
    this->SpiralY += this->SpiralDelY;

    ij[0] = this->SpiralOrigin[0] + this->SpiralX;
    ij[1] = this->SpiralOrigin[1] + this->SpiralY;

    if (0 <= ij[0] && ij[0] < this->Divisions[0] && 0 <= ij[1] && ij[1] < this->Divisions[1])
    {
      return (ij[0] + ij[1] * this->Divisions[0]);
    }
    else
    {
      return (-1);
    }
  }

  // Indicate whether the specified ij bucket is inside the current Voronoi flower.
  bool InFlower(int ij[2])
  {
    double center[2], r, R;
    center[0] = this->Bounds[0] + (static_cast<double>(ij[0]) + 0.5) * this->H[0];
    center[1] = this->Bounds[2] + (static_cast<double>(ij[1]) + 0.5) * this->H[1];

    // Check all contributions to the Voronoi flower
    VertexRingIterator tPtr;
    for (tPtr = this->Verts.begin(); tPtr != this->Verts.end(); ++tPtr)
    {
      R = sqrt((tPtr->X[0] - this->TileX[0]) * (tPtr->X[0] - this->TileX[0]) +
        (tPtr->X[1] - this->TileX[1]) * (tPtr->X[1] - this->TileX[1]));
      r = sqrt((tPtr->X[0] - center[0]) * (tPtr->X[0] - center[0]) +
        (tPtr->X[1] - center[1]) * (tPtr->X[1] - center[1]));
      if ((r - this->BucketRadius) <= R)
      {
        return true;
      }
    }
    return false;
  }

// Dead code. Kept because this method seems to be here for instructional purposes,
// and is mentioned in comments elsewhere.
#if 0
  // Indicate whether the specified point x is inside the Voronoi flower. See
  // also IntersectTile(x) which is an equivalent computation.
  bool InFlower(const double x[2])
  {
    double r2, R2;

    // Check all contributions to the Voronoi flower
    VertexRingIterator tPtr;
    for ( tPtr=this->Verts.begin(); tPtr != this->Verts.end(); ++tPtr )
    {
      R2 = (tPtr->X[0]-this->TileX[0])*(tPtr->X[0]-this->TileX[0]) +
        (tPtr->X[1]-this->TileX[1])*(tPtr->X[1]-this->TileX[1]);
      r2 = (tPtr->X[0]-x[0])*(tPtr->X[0]-x[0]) +
        (tPtr->X[1]-x[1])*(tPtr->X[1]-x[1]);
      if ( r2 <= R2 )
      {
        return true;
      }
    }
    return false;
  }
#endif

  // Generate a Voronoi tile by iterative clipping of the tile with nearby points.
  // Termination of the clipping process occurs when the neighboring points become
  // "far enough" away from the generating point (i.e., error metric is satisfied).
  bool BuildTile(vtkIdList* pIds, const double* pts, double tol, vtkIdType maxClips)
  {
    const double* v;
    int ij[2];
    vtkIdType bucket, i, ptId, nPts, numClips = 0, numClipAttempts = 0;
    vtkIdType prevNumClips, numPts = this->NPts;
    ;

    // Use a spiral iterator to visit locator buckets starting at the bucket
    // containing the tile generating point, and then spiraling out and
    // around the tile generating point. Points are inserted along the way;
    // the process terminates when the error metric is satisfied.
    this->Locator->GetBucketIndices(this->TileX, ij);
    bucket = ij[0] + ij[1] * this->Divisions[0];

    // The first bunch of points is from the locator bucket containing the
    // generating tile point. Insert points from this bucket to get things
    // started.
    if ((nPts = this->Locator->GetNumberOfPointsInBucket(bucket)) > 0)
    {
      this->Locator->GetBucketIds(bucket, pIds);
      for (i = 0; i < nPts && numClips < maxClips; ++i)
      {
        ptId = pIds->GetId(i);
        if (ptId != this->PointId)
        {
          v = pts + 3 * ptId;
          numClips += this->ClipTile(ptId, v, tol);
          numClipAttempts++;
        } // if not generating tile point
      }
    }

    // Now spiral around the locator in (approximately) increasing radius
    // injecting points until the error measure is satisfied.
    this->InitSpiral(ij);
    this->UpdateFlowerFootprint();
    while (!this->IsFlowerCovered() && numClips < maxClips && numClipAttempts < numPts)
    {
      if ((bucket = this->NextSpiralBucket(ij)) >= 0 &&
        (nPts = this->Locator->GetNumberOfPointsInBucket(bucket)) > 0 && this->InFlower(ij))
      {
        this->Locator->GetBucketIds(bucket, pIds);
        prevNumClips = numClips;
        for (i = 0; i < nPts && numClips < maxClips; ++i)
        {
          ptId = pIds->GetId(i);
          v = pts + 3 * ptId;
          numClips += this->ClipTile(ptId, v, tol);
          numClipAttempts++;
        }
        if (prevNumClips != numClips)
        {
          this->UpdateFlowerFootprint();
        }
      } // if bucket should be processed
    }   // while error metric not satisfied

    return true;
  }
};

// Used to accumulate the points within a thread from each tile. Later in
// Reduce() we composite the output from all of the threads.
struct TileVertex
{
  double X;
  double Y;
  TileVertex(double x, double y)
    : X(x)
    , Y(y)
  {
  }
};

// Track local data on a per-thread basis. In the Reduce() method this
// information will be used to composite the data from each thread into a
// single vtkPolyData output.
struct LocalDataType
{
  vtkIdType NumberOfTiles;
  vtkIdType NumberOfPoints;
  std::vector<vtkIdType> LocalTiles;
  std::vector<TileVertex> LocalPoints;
  std::vector<vtkIdType> LocalScalars; // cell scalars
  VTile Tile;
  vtkIdType Offset;

  LocalDataType()
    : NumberOfTiles(0)
    , NumberOfPoints(0)
    , Offset(0)
  {
    this->LocalTiles.reserve(2048);
    this->LocalPoints.reserve(2048);
    this->LocalScalars.reserve(2048);
  }
};

// The threaded core of the algorithm. This could be templated over point
// type, but due to numerical sensitivity we'll just do double for now.
struct VoronoiTiles
{
  const double* Points;
  vtkIdType NPts;
  vtkStaticPointLocator2D* Locator;
  double PaddedBounds[4]; // the domain over which Voronoi is calculated
  double Bounds[4];       // locator bounds
  int Divisions[2];
  double H[2];
  double BucketRadius;
  double Tol;
  vtkPoints* NewPoints;
  vtkCellArray* Tiles;
  int ScalarMode;
  vtkIdTypeArray* Scalars;
  vtkIdType MaxClips;
  int NumThreadsUsed;

  // Storage local to each thread. We don't want to allocate working arrays
  // on every thread invocation. Thread local storage saves lots of
  // new/delete (e.g. the PIds).
  vtkSMPThreadLocalObject<vtkIdList> PIds;
  vtkSMPThreadLocal<LocalDataType> LocalData;

  VoronoiTiles(vtkIdType npts, double* points, vtkStaticPointLocator2D* loc, double padding,
    double tol, vtkPolyData* output, int scalarMode, vtkIdType maxClips)
    : Points(points)
    , NPts(npts)
    , Locator(loc)
    , Tol(tol)
    , ScalarMode(scalarMode)
    , MaxClips(maxClips)
    , NumThreadsUsed(0)
  {
    // Tiles and associated points are filled in later in Reduce()
    this->NewPoints = output->GetPoints();
    this->Tiles = output->GetPolys();

    // Output scalars may be produced if desired
    this->Scalars = static_cast<vtkIdTypeArray*>(output->GetCellData()->GetScalars());

    // Compute some local data for speed. Just need 2D info because
    // everything is happening in 2D.
    loc->GetBounds(this->Bounds);
    loc->GetDivisions(this->Divisions);
    this->H[0] = (this->Bounds[1] - this->Bounds[0]) / static_cast<double>(this->Divisions[0]);
    this->H[1] = (this->Bounds[3] - this->Bounds[2]) / static_cast<double>(this->Divisions[1]);
    this->BucketRadius = 0.5 * sqrt(this->H[0] * this->H[0] + this->H[1] * this->H[1]);

    // Define the Voronoi domain by padding out from bounds.
    for (int i = 0; i < 2; ++i)
    {
      this->PaddedBounds[2 * i] = this->Bounds[2 * i] - padding;
      this->PaddedBounds[2 * i + 1] = this->Bounds[2 * i + 1] + padding;
    }
  }

  // Allocate a little bit of memory to get started. Set some initial values
  // for each thread for accelerating computation.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory

    LocalDataType& localData = this->LocalData.Local();
    localData.Tile.NPts = this->NPts;
    localData.Tile.Locator = this->Locator;
    localData.Tile.Divisions[0] = this->Divisions[0];
    localData.Tile.Divisions[1] = this->Divisions[1];
    localData.Tile.PaddedBounds[0] = this->PaddedBounds[0];
    localData.Tile.PaddedBounds[1] = this->PaddedBounds[1];
    localData.Tile.PaddedBounds[2] = this->PaddedBounds[2];
    localData.Tile.PaddedBounds[3] = this->PaddedBounds[3];
    localData.Tile.Bounds[0] = this->Bounds[0];
    localData.Tile.Bounds[1] = this->Bounds[1];
    localData.Tile.Bounds[2] = this->Bounds[2];
    localData.Tile.Bounds[3] = this->Bounds[3];
    localData.Tile.H[0] = this->H[0];
    localData.Tile.H[1] = this->H[1];
    localData.Tile.BucketRadius = this->BucketRadius;
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    vtkIdList*& pIds = this->PIds.Local();
    LocalDataType& localData = this->LocalData.Local();
    vtkIdType& numTiles = localData.NumberOfTiles;
    vtkIdType& numPoints = localData.NumberOfPoints;
    std::vector<vtkIdType>& lTiles = localData.LocalTiles;
    std::vector<TileVertex>& lPoints = localData.LocalPoints;
    std::vector<vtkIdType>& lScalars = localData.LocalScalars;
    VTile& tile = localData.Tile;
    const double* x = this->Points + 3 * ptId;

    for (; ptId < endPtId; ++ptId, x += 3)
    {
      // Initialize the Voronoi tile
      tile.Initialize(ptId, x);

      // If tile is successfully built, copy the convex tile polygon and
      // points it to thread local storage.
      if (tile.BuildTile(pIds, this->Points, this->Tol, this->MaxClips))
      {
        // Now accumulate the tile / convex polygon in this thread
        vtkIdType i, nPts = static_cast<vtkIdType>(tile.Verts.size());
        lTiles.push_back(nPts);
        for (i = 0; i < nPts; ++i)
        {
          lTiles.push_back((i + numPoints));
          VVertex& tileVertex = tile.Verts.at(i);
          lPoints.emplace_back(TileVertex(tileVertex.X[0], tileVertex.X[1]));
        }

        // Accumulate scalars if requested
        if (this->ScalarMode == vtkVoronoi2D::POINT_IDS)
        {
          lScalars.push_back(ptId);
        }

        numTiles++;
        numPoints += nPts;
      } // if tile built
    }   // for all points
  }

  void Reduce()
  {
    // Count the total number of cells and connectivity storage required,
    // plus the number of points. Keep track of the point id offset to
    // update the cell connectivity list.
    vtkIdType totalTiles = 0;
    vtkIdType connSize = 0;
    vtkIdType totalPoints = 0;
    vtkIdType offset = totalPoints;
    this->NumThreadsUsed = 0;
    vtkSMPThreadLocal<LocalDataType>::iterator ldItr;
    vtkSMPThreadLocal<LocalDataType>::iterator ldEnd = this->LocalData.end();
    for (ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      totalTiles += (*ldItr).NumberOfTiles;
      connSize += static_cast<vtkIdType>((*ldItr).LocalTiles.size());
      totalPoints += (*ldItr).NumberOfPoints;
      (*ldItr).Offset = offset;
      offset = totalPoints;
      this->NumThreadsUsed++;
    }

    // Now copy the data: points and cell connectivity. Points are placed in
    // the x-y plane. Cell connectivities must be updated with new point
    // offsets to reference the correct global point id.
    double z = this->Points[2];
    this->NewPoints->SetNumberOfPoints(totalPoints);
    double* pts = static_cast<double*>(this->NewPoints->GetVoidPointer(0));
    std::vector<TileVertex>::iterator tvItr, tvEnd;
    this->Tiles->AllocateExact(totalTiles, connSize - totalTiles);
    std::vector<vtkIdType>::iterator sItr, sEnd;
    vtkIdType* scalars = nullptr;
    if (this->Scalars)
    {
      this->Scalars->SetNumberOfTuples(totalTiles);
      scalars = static_cast<vtkIdType*>(this->Scalars->GetVoidPointer(0));
    }

    vtkIdType threadId = 0;
    for (ldItr = this->LocalData.begin(); ldItr != ldEnd; ++ldItr)
    {
      // Points
      tvEnd = (*ldItr).LocalPoints.end();
      for (tvItr = (*ldItr).LocalPoints.begin(); tvItr != tvEnd; ++tvItr)
      {
        *pts++ = (*tvItr).X;
        *pts++ = (*tvItr).Y;
        *pts++ = z;
      }

      // Cells
      this->Tiles->AppendLegacyFormat((*ldItr).LocalTiles.data(),
        static_cast<vtkIdType>((*ldItr).LocalTiles.size()), (*ldItr).Offset);

      // Scalars if requested
      if (scalars != nullptr)
      {
        if (this->ScalarMode == vtkVoronoi2D::THREAD_IDS)
        {
          std::size_t j, nCells = (*ldItr).NumberOfTiles;
          for (j = 0; j < nCells; ++j)
          {
            *scalars++ = threadId;
          }
        }
        else // if ( this->ScalarMode == vtkVoronoi2D::POINT_IDS )
        {
          sEnd = (*ldItr).LocalScalars.end();
          for (sItr = (*ldItr).LocalScalars.begin(); sItr != sEnd; ++sItr)
          {
            *scalars++ = *sItr;
          }
        }
      }

      threadId++;
    } // for each thread
  }

  // A little factory method to conveniently instantiate the tiles etc.
  static int Execute(vtkStaticPointLocator2D* loc, vtkIdType numPts, double* points, double padding,
    double tol, vtkPolyData* output, int sMode, vtkIdType pointOfInterest, vtkIdType maxClips)
  {
    VoronoiTiles vt(numPts, points, loc, padding, tol, output, sMode, maxClips);
    if (pointOfInterest < 0 || pointOfInterest >= numPts)
    {
      vtkSMPTools::For(0, numPts, vt);
    }
    else
    {
      vtkSMPTools::For(pointOfInterest, pointOfInterest + 1, vt);
    }
    return vt.NumThreadsUsed;
  }
}; // VoronoiTiles

} // anonymous namespace

//================= Begin VTK class proper =====================================
//------------------------------------------------------------------------------
// Construct object
vtkVoronoi2D::vtkVoronoi2D()
{
  this->Padding = 0.01;
  this->Tolerance = 0.00001; // currently hidden
  this->Locator = vtkStaticPointLocator2D::New();
  this->Locator->SetNumberOfPointsPerBucket(2);
  this->Transform = nullptr;
  this->GenerateScalars = NONE;
  this->ProjectionPlaneMode = XY_PLANE;
  this->PointOfInterest = (-1);
  this->MaximumNumberOfTileClips = VTK_ID_MAX;
  this->GenerateVoronoiFlower = false;
  this->NumberOfThreadsUsed = 0;
  this->Spheres = vtkSpheres::New();

  // Optional second and third outputs for Voroni flower
  this->SetNumberOfOutputPorts(3);
}

//------------------------------------------------------------------------------
vtkVoronoi2D::~vtkVoronoi2D()
{
  this->Locator->Delete();
  this->Locator = nullptr;
  this->Spheres->Delete();
  this->Spheres = nullptr;
  this->SetTransform(nullptr);
}

//------------------------------------------------------------------------------
int vtkVoronoi2D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Generating 2D Voronoi Tessellation");

  // Initialize; check input
  vtkIdType pId, numPts;
  vtkPoints* inPoints;
  if ((inPoints = input->GetPoints()) == nullptr || (numPts = inPoints->GetNumberOfPoints()) < 1)
  {
    vtkDebugMacro("Cannot tessellate; need at least 1 input point");
    return 1;
  }

  // If the user specified a transform, apply it to the input data.
  // Only the input points are transformed. Note points are always
  // converted to double.
  vtkPoints* tPoints = nullptr;
  if (this->Transform)
  {
    tPoints = vtkPoints::New();
    tPoints->SetDataTypeToDouble();
    this->Transform->TransformPoints(inPoints, tPoints);
  }
  else if (this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE)
  {
    // If the user asked this filter to compute the best fitting plane,
    // proceed to compute the plane and generate a transform that will
    // map the input points into that plane.
    this->SetTransform(vtkDelaunay2D::ComputeBestFittingPlane(input));
    tPoints = vtkPoints::New();
    tPoints->SetDataTypeToDouble();
    this->Transform->TransformPoints(inPoints, tPoints);
  }
  else if (inPoints->GetDataType() == VTK_DOUBLE)
  { // fast path no conversion
    tPoints = inPoints;
    tPoints->Register(this);
  }
  else
  { // convert points to double
    tPoints = vtkPoints::New();
    tPoints->SetDataTypeToDouble();
    tPoints->SetNumberOfPoints(numPts);
    for (pId = 0; pId < numPts; ++pId)
    {
      tPoints->SetPoint(pId, inPoints->GetPoint(pId));
    }
  }

  // Temporary data object holds points to be tessellated
  vtkPolyData* tInput = vtkPolyData::New();
  tInput->SetPoints(tPoints);
  tPoints->Delete();

  // Construct the output
  vtkPoints* newPts = vtkPoints::New();
  newPts->SetDataTypeToDouble();
  vtkCellArray* tiles = vtkCellArray::New();
  output->SetPoints(newPts);
  output->SetPolys(tiles);
  if (this->GenerateScalars != NONE)
  {
    vtkIdTypeArray* ts = vtkIdTypeArray::New();
    ts->SetNumberOfComponents(1);
    int idx = output->GetCellData()->AddArray(ts);
    output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    ts->Delete();
  }

  // A locator is used to locate closest points.
  if (!this->Locator)
  {
    vtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(tInput);
  this->Locator->BuildLocator();

  // Computational bounds
  double length = tInput->GetLength();
  double tol = this->Tolerance * length;
  double padding = this->Padding * length;

  // Process the points to generate Voronoi tiles
  double* inPtr = static_cast<double*>(tPoints->GetVoidPointer(0));
  this->NumberOfThreadsUsed = VoronoiTiles::Execute(this->Locator, numPts, inPtr, padding, tol,
    output, this->GenerateScalars, this->PointOfInterest, this->MaximumNumberOfTileClips);

  vtkDebugMacro(<< "Produced " << output->GetNumberOfCells() << " tiles and "
                << output->GetNumberOfPoints() << " points");

  // If requested, generate in the second output a representation of the
  // Voronoi flower error metric for the PointOfInterest.
  if (this->GenerateVoronoiFlower && this->PointOfInterest >= 0 && this->PointOfInterest < numPts)
  {
    // Get the second and third outputs
    vtkInformation* outInfo2 = outputVector->GetInformationObject(1);
    vtkPolyData* output2 = vtkPolyData::SafeDownCast(outInfo2->Get(vtkDataObject::DATA_OBJECT()));

    // Populate a Voronoi tile with the output tile (PointOfIntersect). This
    // assumes a single convex polygon has been output.
    double bds[6], tileX[3], x[3], center[3], factor = 3.5;
    vtkIdType npts;
    const vtkIdType* p;
    output->GetBounds(bds);
    output->GetCenter(center);
    tiles->InitTraversal();
    tiles->GetNextCell(npts, p);
    VTile tile;
    tPoints->GetPoint(this->PointOfInterest, tileX);
    tile.Initialize(this->PointOfInterest, tileX, newPts, npts, p);

    // For now generate a zillion points and keep those that intersect the
    // tile.
    vtkPoints* fPts = vtkPoints::New();
    fPts->SetDataTypeToDouble();
    vtkCellArray* fVerts = vtkCellArray::New();
    fVerts->InsertNextCell(1);
    vtkIdType i, pid;
    for (i = 0, npts = 0; i < 1000000; ++i)
    {
      x[0] = vtkMath::Random(
        center[0] + factor * (bds[0] - center[0]), center[0] + factor * (bds[1] - center[0]));
      x[1] = vtkMath::Random(
        center[1] + factor * (bds[2] - center[1]), center[1] + factor * (bds[3] - center[1]));
      x[2] = 0.0;
      if (tile.IntersectTile(x))
      {
        pid = fPts->InsertNextPoint(x);
        fVerts->InsertCellPoint(pid);
        npts++;
      }
    }
    fVerts->UpdateCellCount(npts);
    output2->SetPoints(fPts);
    output2->SetVerts(fVerts);
    fPts->Delete();
    fVerts->Delete();

    // Now update the vtkSpheres implicit function, and create a third output
    // that has the PointOfInterested-associated tile, with scalar values at
    // each point which are the radii of the error circles (and when taken
    // together form the Voronoi flower).
    vtkInformation* outInfo3 = outputVector->GetInformationObject(2);
    vtkPolyData* output3 = vtkPolyData::SafeDownCast(outInfo3->Get(vtkDataObject::DATA_OBJECT()));

    vtkPoints* centers = vtkPoints::New();
    centers->SetDataTypeToDouble();
    vtkCellArray* singleTile = vtkCellArray::New();
    vtkDoubleArray* radii = vtkDoubleArray::New();
    radii->SetName("Voronoi Flower Radii");

    output3->SetPoints(centers);
    output3->SetPolys(singleTile);
    output3->GetPointData()->SetScalars(radii);

    // Update polydata (third output)
    tile.PopulatePolyData(centers, singleTile, radii);

    // Update implicit function
    this->Spheres->SetCenters(centers);
    this->Spheres->SetRadii(radii);

    centers->Delete();
    singleTile->Delete();
    radii->Delete();
  }

  // Clean up
  tInput->Delete();
  newPts->Delete();
  tiles->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// This filter can process any explicit representation of points.
int vtkVoronoi2D::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//--------------------------------------------------------------------------
// Since users have access to the locator we need to take into account the
// locator's modified time.
vtkMTimeType vtkVoronoi2D::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  vtkMTimeType time = this->Locator->GetMTime();
  return (time > mTime ? time : mTime);
}

//----------------------------------------------------------------------------
void vtkVoronoi2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Projection Plane Mode: " << this->ProjectionPlaneMode << "\n";
  os << indent << "Transform: " << (this->Transform ? "specified" : "none") << "\n";
  os << indent << "Generate Scalars: " << this->GenerateScalars << "\n";
  os << indent << "Point Of Interest: " << this->PointOfInterest << "\n";
  os << indent << "Maximum Number Of Tile Clips: " << this->MaximumNumberOfTileClips << "\n";
  os << indent << "Generate Voronoi Flower: " << (this->GenerateVoronoiFlower ? "On\n" : "Off\n");
}
