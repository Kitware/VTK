/*=========================================================================

  Program:   ParaView
  Module:    vtkPointsProjectedHull.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPointsProjectedHull.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPointsProjectedHull, "1.1");
vtkStandardNewMacro(vtkPointsProjectedHull);

static const int xdim=0, ydim=1, zdim=2;
static const int xmin=0, xmax=1, ymin=2, ymax=3;
static double firstPt[3];

//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2 on the line
//            <0 for P2 right of the line
#define VTK_ISLEFT(P0, P1, P2) \
(((P1)[0] - (P0)[0])*((P2)[1] - (P0)[1]) - ((P2)[0] - (P0)[0])*((P1)[1] - (P0)[1]))

vtkPointsProjectedHull::vtkPointsProjectedHull()
{
  this->initFlags();
}
vtkPointsProjectedHull::~vtkPointsProjectedHull()
{
  this->clearAllocations();
}
void vtkPointsProjectedHull::Initialize()
{
  this->clearAllocations();
  this->initFlags();

  vtkPoints::Initialize();
}
void vtkPointsProjectedHull::Update()
{
  this->clearAllocations();
  this->initFlags();
}
void vtkPointsProjectedHull::initFlags()
{
  int i;

  this->Pts = NULL;
  this->Npts = 0;

  for (i=0; i<3; i++)
    {
    this->ccwHull[i] = NULL;
    }

  for (i=0; i<3; i++)
    {
    this->hullSize[i]     = 0;
    }
}
void vtkPointsProjectedHull::clearAllocations()
{
  int i;
  for (i=0; i<3; i++)
    {
    if (this->ccwHull[i])
      {
      delete [] this->ccwHull[i];
      this->ccwHull[i] = NULL;
      }
    }
  if (this->Pts)
    {
    delete [] this->Pts;
    this->Pts = NULL; 
    }
}
#define VTK_GETCCWHULL(which, dim) \
int vtkPointsProjectedHull::GetCcwHull##which(float *pts, int len)\
{                                                   \
  int i;                                            \
  double *dpts = new double [len*2];                \
  int copypts = this->GetCcwHull##which(dpts, len); \
  for (i=0; i<copypts*2; i++)                       \
    {                                               \
    pts[i] = (float)dpts[i];                        \
    }                                               \
  delete [] dpts;                \
  return copypts;                \
}                                \
int vtkPointsProjectedHull::GetCcwHull##which(double *pts, int len)\
{                                                 \
  if ((this->hullSize[dim] == 0) ||               \
      (this->GetMTime() > this->hullTime[dim]))   \
    {                                             \
    grahamScanAlgorithm(dim);                             \
    }                                                       \
  int copylen = (this->hullSize[dim] <= len) ? this->hullSize[dim] : len; \
  if (copylen <= 0) return 0; \
  memcpy(pts, this->ccwHull[dim], sizeof(double) * 2 * copylen); \
  return copylen;             \
}
VTK_GETCCWHULL(X, 0);
VTK_GETCCWHULL(Y, 1);
VTK_GETCCWHULL(Z, 2);

#define VTK_GETSIZECCWHULL(which, dim) \
int vtkPointsProjectedHull::GetSizeCcwHull##which()\
{                                                  \
  if ((this->hullSize[dim] == 0) ||                \
      (this->GetMTime() > this->hullTime[dim]))   \
    {                                             \
    grahamScanAlgorithm(dim);                      \
    }                                              \
  return this->hullSize[dim];                      \
}
VTK_GETSIZECCWHULL(X, 0);
VTK_GETSIZECCWHULL(Y, 1);
VTK_GETSIZECCWHULL(Z, 2);

#define VTK_RECTANGLEINTERSECTION(which, dim) \
int vtkPointsProjectedHull::rectangleIntersection##which(vtkPoints *R) \
{                                                                      \
  double bounds[6];                                            \
  R->Modified();                                               \
  R->GetBounds(bounds);                                        \
  double hmin, hmax, vmin, vmax;                               \
                                                               \
  hmin = (double)bounds[(dim*2+2)%6];                          \
  hmax = (double)bounds[(dim*2+2)%6+1];                        \
  vmin = (double)bounds[(dim*2+4)%6];                          \
  vmax = (double)bounds[(dim*2+4)%6 + 1];                      \
                                                               \
  return rectangleIntersection##which(hmin, hmax, vmin, vmax); \
}                                                              \
int vtkPointsProjectedHull::rectangleIntersection##which(float hmin, \
                                 float hmax, float vmin, float vmax)\
{                                                                    \
  return rectangleIntersection##which((double)hmin, (double)hmax, \
                                     (double)vmin, (double)vmax); \
}                                                              \
int vtkPointsProjectedHull::rectangleIntersection##which(double hmin, \
                         double hmax, double vmin, double vmax)       \
{                                                                     \
  if ((this->hullSize[dim] == 0) ||                           \
      (this->GetMTime() > this->hullTime[dim]))               \
    {                                                         \
    grahamScanAlgorithm(dim);                                 \
    }                                                           \
  return rectangleIntersection(hmin, hmax, vmin, vmax ,dim);  \
}
VTK_RECTANGLEINTERSECTION(X, 0);
VTK_RECTANGLEINTERSECTION(Y, 1);
VTK_RECTANGLEINTERSECTION(Z, 2);

// Does the axis-aligned rectangle R intersect the convex polygon 
// given by the counter-clockwise enumeration of it's vertices.
//  
// Graphics Gems IV, Rectangle-Polygon intersection: Rectangle R
//   intersects polygon P if and only if (1) the bounding box
//   of P intersects R and (2) R does not lie entirely outside
//   any infinite line defined by P's edges.  (Outside means if
//   you are walking the line in the direction given by the
//   ccw orientation of the points of P, R lies completely
//   in the half plane on your right.) (Ned Greene)

int vtkPointsProjectedHull::rectangleIntersection(double hmin, double hmax, 
                                 double vmin, double vmax, int dim)
{
  if (rectangleBoundingBoxIntersection(hmin,hmax,vmin,vmax,dim) == 0)
    {
      return 0;
    }

  if (rectangleOutside(hmin,hmax,vmin,vmax, dim) == 1)
    {
      return 0;
    }

  return 1;
}
//
// Suppose the points are projected orthogonally in the dir
// of the positive x, y or z axis.  Compute the points (2 components)
// of the convex hull of that projection, in counter clockwise order.
//
//  "Right hand rule":
//
//     |             |              |
//    Z|            X|             Y|
//     |             |              |
//     |             |              |
//     ------- Y     -------- Z     -------X
//   along X-axis  along Y-axis    along Z-axis
//
// Algorithm comes from Graphics Gems IV
//
extern "C" 
{
  int vtkPointsProjectedHullIncrVertAxis(const void *p1, const void *p2);
  int vtkPointsProjectedHullCCW(const void *p1, const void *p2);
}
int vtkPointsProjectedHull::grahamScanAlgorithm(int dir)
{
int horizAxis = 0, vertAxis = 0;
int i,j;

  if ((this->Npts == 0) || (this->GetMTime() > this->PtsTime))
    {
    getPoints();
    }

  switch (dir)
    {
    case xdim:
       horizAxis = ydim;
       vertAxis = zdim;
       break;

    case ydim:
       horizAxis = zdim;
       vertAxis = xdim;
       break;

    case zdim:
       horizAxis = xdim;
       vertAxis = ydim;
       break;
    }

  // Find the lowest, rightmost point in the set

  double *hullPts = new double[this->Npts*2];

  for (i=0; i<this->Npts; i++)
    {
    hullPts[i*2]     = this->Pts[i*3 + horizAxis];
    hullPts[i*2 + 1] = this->Pts[i*3 + vertAxis];
    }

  qsort(hullPts, this->Npts, sizeof(double) * 2, vtkPointsProjectedHullIncrVertAxis);

  int firstId = 0;

  for (i=1; i<this->Npts; i++)
    {
    if (hullPts[i*2 + 1] != hullPts[1]) break;

    if (hullPts[i*2] > hullPts[firstId*2])
      {
       firstId = i;
      }
    }

  firstPt[0] = hullPts[firstId * 2];     // lowest, rightmost
  firstPt[1] = hullPts[firstId * 2 + 1];

  if (firstId != 0)
    {
    hullPts[2*firstId]     = hullPts[0];
    hullPts[2*firstId + 1] = hullPts[1];
    hullPts[0] = firstPt[0];
    hullPts[1] = firstPt[1];
    }
  // If there are duplicates of the first point in the
  // projection, the vtkPointsProjectedHullCCW sort will fail.

  int dups = 0;

  for (j=1, i=1; j < this->Npts; j++)
    {
    if ( !dups && (hullPts[j*2+1] != hullPts[1])) break;
  
    if ( (hullPts[j*2+1] != hullPts[1]) || (hullPts[j*2] != hullPts[0]))
      {
      if (j > i)
        {
        hullPts[i*2]   = hullPts[j*2];
        hullPts[i*2+1] = hullPts[j*2+1];
        }
      i++;
      }
    else
      {
      dups++;
      }
    }
  int nHullPts = this->Npts - dups;
  

  // Sort in counter clockwise order the other points by the angle
  //   they make with the line through firstPt parallel to the 
  //   horizontal axis.
  //   (For a tie: choose the point furthest from firstPt.)
  //   P2 makes a greater angle than P1 if it is left of the line
  //   formed by firstPt - P1.

  qsort(hullPts+2, nHullPts - 1, sizeof(double)*2, vtkPointsProjectedHullCCW);

  // Remove sequences of duplicate points, and remove interior
  //   points on the same ray from the initial point

  nHullPts = removeExtras(hullPts, nHullPts);

  // Now make list of points on the convex hull.

  int top = 1;

  for (i=2; i<nHullPts; i++)
    {
    int newpos = positionInHull(hullPts, hullPts + top*2,
                                   hullPts + i*2);

      hullPts[newpos*2]    = hullPts[i*2];
      hullPts[newpos*2+ 1] = hullPts[i*2+ 1];
    
    top = newpos;
    }
  nHullPts = top + 1;

  // hull bounding box

  double x0 = hullPts[0];
  double x1 = hullPts[0];
  double y0 = hullPts[1];
  double y1 = hullPts[1];

  for (i=1; i<nHullPts; i++)
    {
    if (hullPts[2*i] < x0) x0 = hullPts[2*i];
    else if (hullPts[2*i] > x1) x1 = hullPts[2*i];

    if (hullPts[2*i+1] < y0) y0 = hullPts[2*i+1];
    else if (hullPts[2*i+1] > y1) y1 = hullPts[2*i+1];
    }
  this->hullBBox[dir][xmin] = (float)x0;
  this->hullBBox[dir][xmax] = (float)x1;
  this->hullBBox[dir][ymin] = (float)y0;
  this->hullBBox[dir][ymax] = (float)y1;

  this->hullSize[dir] = nHullPts;

  if (this->ccwHull[dir]) delete [] this->ccwHull[dir];

  this->ccwHull[dir] = new double[nHullPts*2];

  memcpy(this->ccwHull[dir], hullPts, sizeof(double) * 2 * nHullPts);

  delete [] hullPts;

  this->hullTime[dir].Modified();

  return 0;
}
double vtkPointsProjectedHull::distance(double *p1, double *p2)
{       
    return (p1[0] - p2[0])*(p1[0] - p2[0]) +
                 (p1[1] - p2[1])*(p1[1] - p2[1]);
}   
int vtkPointsProjectedHull::removeExtras(double *pts, int n)
{
  int i, prev, skipMe, coord;
  double where;

    prev = 0;

    for (i=1; i<n; i++)
      {
        skipMe = 0;

        // case: point is equal to previous point

        if ((pts[i*2] == pts[prev*2]) &&
            (pts[i*2+ 1] == pts[prev*2+ 1]))
          {
            skipMe = 1;
          }

        // case: point is at same angle as previous point - 
        //   discard the point that is closest to first point
    
        else if (prev >= 1)
          {
            where = VTK_ISLEFT(pts, 
                           pts + prev*2,
                           pts + i*2);


            if (where == 0)  // on same ray from first point
              {
                double d1 = distance(pts, pts + prev*2);
                double d2 = distance(pts, pts + i*2);
    
                if (d2 > d1)  // save only the most distant
                  {
                    for (coord=0; coord<2; coord++)
                      {
                         pts[prev*2+ coord] = pts[i*2+ coord];
                      } 
                  }
                skipMe = 1;
              }
          }

        if (!skipMe)
          {
            prev++;
            if (prev < i)
              {
                for (coord=0; coord<2; coord++)
                  {
                     pts[prev*2+ coord] = pts[i*2+ coord];
                  }
              }
          }
      }

    return prev+1;   // size of new list
}
int vtkPointsProjectedHull::positionInHull(double *base, double *top, double *pt)
{
double *p1, *p2;
double where;

    p2 = top;
    p1 = p2 - 2;

    // Because of the way the vertices are sorted, the new vertex
    // is part of convex hull so far.  But the previous vertex
    // could now be inside the convex hull if the new vertex is to
    // the right of the line formed by the previous two vertices.
    
    while (p2 > base)
      {     
        where = VTK_ISLEFT(p1, p2, pt);

        // If vertex is to left of line, don't remove previous points

        if (where > 0) break;

        // If vertex is to right of line, remove previous point and
        //   check again.  If vertex is on line, previous point is
        //   redundant.

        p2 -= 2;   // pop top of stack
        p1 -= 2;
      }

    // return the position in the list where the new vertex goes

    return ((p2 - base) / 2) + 1;
}
void vtkPointsProjectedHull::getPoints()
{
  int i;

  if (this->Pts)
    {
     delete [] this->Pts;
    }
  this->Npts = this->Data->GetNumberOfTuples();

  this->Pts = new double [this->Npts*3];

  for (i=0; i<this->Npts; i++)
    {
    this->Pts[i*3]     = this->Data->GetComponent(i, 0);
    this->Pts[i*3 + 1] = this->Data->GetComponent(i, 1);
    this->Pts[i*3 + 2] = this->Data->GetComponent(i, 2);
    }

  this->PtsTime.Modified();
}
int vtkPointsProjectedHull::
rectangleBoundingBoxIntersection(double hmin, double hmax, 
                                double vmin, double vmax, int dim)
{       
  float *r2Bounds = this->hullBBox[dim];

  if ((hmin >= r2Bounds[xmax]) ||
      (hmax <= r2Bounds[xmin]) ||
      (vmin >= r2Bounds[ymax]) ||
      (vmax <= r2Bounds[ymin]))
    {
    return 0;     
    }
    
  return 1;
}

#define sameDirection(a, b) \
   ((((a) == 0) && ((b) < 0)) || (((a) > 0) && ((b) > 0)))

int vtkPointsProjectedHull::
outsideHorizontalLine(double vmin, double vmax, 
                      double *p0, double *, double *insidePt)
{
    if (insidePt[1] > p0[1])
      {
        if (vmax <= p0[1]) return 1;
        else              return 0;
      }
    else
      {
        if (vmin >= p0[1]) return 1;
        else              return 0;
      }
}
int vtkPointsProjectedHull::
outsideVerticalLine(double hmin, double hmax,
                      double *p0, double *, double *insidePt)
{
    if (insidePt[0] > p0[0])
      {
        if (hmax <= p0[0]) return 1;
        else              return 0;
      }
    else
      {
        if (hmin >= p0[0]) return 1;
        else              return 0;
      }
}
int vtkPointsProjectedHull::
outsideLine(double hmin, double hmax, double vmin, double vmax,
            double *p0, double *p1, double *insidePt)
{   
  int i;

  if ((p1[1] - p0[1]) == 0) 
    return outsideHorizontalLine(vmin, vmax, p0, p1, insidePt);
    
  if ((p1[0] - p0[0]) == 0) 
    return outsideVerticalLine(hmin, hmax, p0, p1, insidePt);

  // Are any of the points of the rectangle in the same half-plane as the
  //    inside point?
    
  double ip = VTK_ISLEFT(p0, p1, insidePt);
  double rp;

  double pts[4][2];

  pts[0][0] = hmin; pts[0][1] = vmin;
  pts[1][0] = hmin; pts[1][1] = vmax;
  pts[2][0] = hmax; pts[2][1] = vmax;
  pts[3][0] = hmax; pts[3][1] = vmin;
        
  for (i=0; i < 4; i++)
    {
     rp = VTK_ISLEFT(p0, p1, pts[i]);
        
     if (  ((rp < 0) && (ip < 0)) ||
           ((rp > 0) && (ip > 0))    )
       {
          return 0;
       } 
    }

  return 1;
}
int vtkPointsProjectedHull::rectangleOutside(double hmin, double hmax,
                                  double vmin, double vmax, int dir)
{
  int i;

  int npts = this->hullSize[dir];

  if (npts == 2)
    {
    return this->rectangleOutside1DPolygon(hmin, hmax, vmin, vmax, dir);
    }

  // a representative point inside the polygon

  double *insidePt = new double[2];

  insidePt[0] = this->ccwHull[dir][0];
  insidePt[1] = this->ccwHull[dir][1];

  insidePt[0] += this->ccwHull[dir][4];
  insidePt[1] += this->ccwHull[dir][5];

  if (npts == 3)
    {
    insidePt[0] += this->ccwHull[dir][2];
    insidePt[1] += this->ccwHull[dir][3];

    insidePt[0] /= 3;
    insidePt[1] /= 3;
    }
  else
    {
    insidePt[0] /= 2;
    insidePt[1] /= 2;
    }

  // For each infinite line given by the line segments of the
  // polygon, determine if rectangle is entirely outside that line.
  // If so, it must be outside the polygon.

  for (i=0; i < npts-1; i++)
    {
    if (outsideLine(hmin,hmax,vmin,vmax, 
                  this->ccwHull[dir] + 2*i,
                  this->ccwHull[dir] + 2*i + 2,
                  insidePt))
      {
      return 1;
      }
    }

  delete [] insidePt;

  return 0; 
}
int vtkPointsProjectedHull::rectangleOutside1DPolygon(double hmin, double hmax,
                                  double vmin, double vmax, int dir)
{
  int i;

  double *p0 = this->ccwHull[dir];
  double *p1 = this->ccwHull[dir] + 2;

  double pts[4][2];

  pts[0][0] = hmin; pts[0][1] = vmin;
  pts[1][0] = hmin; pts[1][1] = vmax;
  pts[2][0] = hmax; pts[2][1] = vmax;
  pts[3][0] = hmax; pts[3][1] = vmin;

  double side;
  double reference=0.0;

  for (i=0; i<4; i++)
    {
    side = VTK_ISLEFT(p0, p1,pts[i]);

    if (reference != 0.0)
      {
      if (side != reference)
        {
        return 0;   // two points are on opposite sides of the line
        }
      }
    else if (side != 0.0)
      {
      reference = side;
      }
    }

  // all four vertices are either on the line or on the same side
  // of the line

  return 1; 
}

// The sort functions

extern "C"
{
  int vtkPointsProjectedHullIncrVertAxis(const void *p1, const void *p2)
  {
  double *a, *b;

      a = (double *)p1;
      b = (double *)p2;

      if (a[1] < b[1])       return -1;
      else if (a[1] == b[1]) return 0;
      else                   return 1;
  }

  int vtkPointsProjectedHullCCW(const void *p1, const void *p2)
  {
  double *a, *b;
  double val;

      a = (double *)p1;
      b = (double *)p2;

      // sort in counter clockwise order from first point 

      val = VTK_ISLEFT(firstPt, a, b);

      if (val < 0)       return 1;   // b is right of line firstPt->a
      else if (val == 0) return 0;   // b is on line firstPt->a
      else               return -1;  // b is left of line firstPt->a
  }
}

void vtkPointsProjectedHull::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Pts: " << this->Pts << endl;
  os << indent << "Npts: " << this->Npts << endl;
  os << indent << "PtsTime: " << this->PtsTime << endl;

  os << indent << "ccwHull X: " << this->ccwHull[0] << endl;
  os << indent << "hullBBox X: [" ;
        os << this->hullBBox[0][0] << ", " <<  this->hullBBox[0][1] << "] [";
        os << this->hullBBox[0][2] << ", " <<  this->hullBBox[0][3] << "] ";
  os << indent << "hullSize X: " << this->hullSize[0] << endl;
  os << indent << "hullTime X: " << this->hullTime[0] << endl;

  os << indent << "ccwHull Y: " << this->ccwHull[1] << endl;
  os << indent << "hullBBox Y: [" ;
        os << this->hullBBox[1][0] << ", " <<  this->hullBBox[1][1] << "] [";
        os << this->hullBBox[1][2] << ", " <<  this->hullBBox[1][3] << "] ";
  os << indent << "hullSize Y: " << this->hullSize[1] << endl;
  os << indent << "hullTime Y: " << this->hullTime[1] << endl;

  os << indent << "ccwHull Z: " << this->ccwHull[2] << endl;
  os << indent << "hullBBox Z: [" ;
        os << this->hullBBox[2][0] << ", " <<  this->hullBBox[2][1] << "] [";
        os << this->hullBBox[2][2] << ", " <<  this->hullBBox[2][3] << "] ";
  os << indent << "hullSize Z: " << this->hullSize[2] << endl;
  os << indent << "hullTime Z: " << this->hullTime[2] << endl;
}
