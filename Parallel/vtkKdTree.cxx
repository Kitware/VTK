/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKdTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/* NOTE -- this code is not what I'd call production quality code. It does
 * not adhear to VTK style conventions and there are a lot of floats that
 * should probably be doubles. - Ken */  

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkKdTree.h"
#include "vtkBox.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkCamera.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkPlanesIntersection.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkTimerLog.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkPointSet.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkRectilinearGrid.h"

#ifdef _MSC_VER
#pragma warning ( disable : 4100 )
#endif
#include <algorithm>
#include <vtkstd/set>

vtkCxxRevisionMacro(vtkKdTree, "1.12");

// methods for vtkKdNode -------------------------------------------

const char *vtkKdNode::LevelMarker[20]={
"",
" ",
"  ",
"   ",
"    ",
"     ",
"      ",
"       ",
"        ",
"         ",
"          ",
"           ",
"            ",
"             ",
"              ",
"               ",
"                ",
"                 ",
"                  ",
"                   "
};

//----------------------------------------------------------------------------
vtkKdNode::vtkKdNode()
{  
  this->Up = this->Left = this->Right = NULL;
  this->Dim = 3;
  this->Id = -1;
  this->MinId = -1;
  this->MaxId = -1;
  this->NumCells = 0;

  this->CellBoundsCache = NULL;
}

//----------------------------------------------------------------------------
vtkKdNode::~vtkKdNode()
{
}

//----------------------------------------------------------------------------
void vtkKdNode::SetBounds(double x1,double x2,double y1,double y2,double z1,double z2)
{
   this->Min[0] = x1; this->Max[0] = x2;
   this->Min[1] = y1; this->Max[1] = y2;
   this->Min[2] = z1; this->Max[2] = z2;
}

//----------------------------------------------------------------------------
void vtkKdNode::GetBounds(double *b) const
{
   b[0] = this->Min[0]; b[1] = this->Max[0];
   b[2] = this->Min[1]; b[3] = this->Max[1];
   b[4] = this->Min[2]; b[5] = this->Max[2];
}

//----------------------------------------------------------------------------
void vtkKdNode::GetBounds(float *b) const
{
   b[0] = (float)this->Min[0]; b[1] = (float)this->Max[0];
   b[2] = (float)this->Min[1]; b[3] = (float)this->Max[1];
   b[4] = (float)this->Min[2]; b[5] = (float)this->Max[2];
}

//----------------------------------------------------------------------------
void vtkKdNode::SetDataBounds(double x1,double x2,double y1,double y2,double z1,double z2)
{
   this->MinVal[0] = x1; this->MaxVal[0] = x2;
   this->MinVal[1] = y1; this->MaxVal[1] = y2;
   this->MinVal[2] = z1; this->MaxVal[2] = z2;
}

//----------------------------------------------------------------------------
void vtkKdNode::SetDataBounds(float *v)
{
  int x;
  double newbounds[6];

  vtkIdType numCells = this->GetNumberOfCells();

  int i;

  if (this->Up)
    {
    double bounds[6];

    this->Up->GetDataBounds(bounds);
   
    int dim = this->Up->GetDim();
   
    for (i=0; i<3; i++)
      {
      if (i == dim)
        {
        continue;
        }

      newbounds[i*2]  = bounds[i*2];
      newbounds[i*2+1] = bounds[i*2+1];
      } 

    newbounds[dim*2] = newbounds[dim*2+1] = (double)v[dim];

    for (i = dim+3; i< numCells*3; i+=3)
      {
      if (v[i] < newbounds[dim*2])
        {
        newbounds[dim*2] = (double)v[i];
        }
      else if (v[i] > newbounds[dim*2+1]) 
        {
        newbounds[dim*2+1] = (double)v[i];
        }
      }
    }
  else
    {
    for (i=0; i<3; i++)
      {
      newbounds[i*2] = newbounds[i*2+1] = (double)v[i];
      }

    for (x = 3; x< numCells*3; x+=3)
      {
      int y=x+1;
      int z=x+2;

      if (v[x] < newbounds[0]) 
        {
        newbounds[0] = (double)v[x];
        }
      else if (v[x] > newbounds[1]) 
        {
        newbounds[1] = (double)v[x];
        }

      if (v[y] < newbounds[2]) 
        {
        newbounds[2] = (double)v[y];
        }
      else if (v[y] > newbounds[3]) 
        {
        newbounds[3] = (double)v[y];
        }

      if (v[z] < newbounds[4]) 
        {
        newbounds[4] = (double)v[z];
        }
      else if (v[z] > newbounds[5]) 
        {
        newbounds[5] = (double)v[z];
        }
      }
    }

  this->SetDataBounds(newbounds[0], newbounds[1], newbounds[2],
            newbounds[3], newbounds[4], newbounds[5]);
}

//----------------------------------------------------------------------------
void vtkKdNode::GetDataBounds(double *b) const
{
   b[0] = this->MinVal[0]; b[1] = this->MaxVal[0];
   b[2] = this->MinVal[1]; b[3] = this->MaxVal[1];
   b[4] = this->MinVal[2]; b[5] = this->MaxVal[2];
}

//----------------------------------------------------------------------------
void vtkKdNode::GetDataBounds(float *b) const
{     
   b[0] = (float)this->MinVal[0]; b[1] = (float)this->MaxVal[0];
   b[2] = (float)this->MinVal[1]; b[3] = (float)this->MaxVal[1];
   b[4] = (float)this->MinVal[2]; b[5] = (float)this->MaxVal[2];
}     

//----------------------------------------------------------------------------
// Distance (squared) from any point anywhere to the boundary of spatial region
//
float vtkKdNode::GetDistance2ToBoundary(float x, float y, float z, int useDataBounds=0)
{
  return this->_GetDistance2ToBoundary(x, y, z, NULL, 0, useDataBounds);
}

//----------------------------------------------------------------------------
// Distance (squared) from any point anywhere to the boundary of spatial region,
// and give me the point on the boundary closest to this point.
//
float vtkKdNode::GetDistance2ToBoundary(float x, float y, float z, float *p,
                                        int useDataBounds=0)
{
  return this->_GetDistance2ToBoundary(x, y, z, p, 0, useDataBounds);
}

//----------------------------------------------------------------------------
// The point is inside the region, and I want the distance (squared) 
// to the closest "interior" wall, one that is not an outer boundary of
// the entire space.
//
float vtkKdNode::GetDistance2ToInnerBoundary(float x, float y, float z) 
{
  return this->_GetDistance2ToBoundary(x, y, z, NULL, 1, 0);
}

//----------------------------------------------------------------------------
// get distance (squared) to boundary of region
//
float vtkKdNode::_GetDistance2ToBoundary( 
    float x, float y, float z,  // from this point
    float *p,              // set to point on boundary that is closest
    int innerBoundaryOnly, // ignore boundaries on "outside"
    int useDataBounds=0)   // use bounds of data within region instead
{
  float minDistance, dist;
  float edgePt[3];
  float cornerPt[3];
  float pt3[3];

  double *min, *max;

  if (useDataBounds)
    {
    min = this->MinVal; max = this->MaxVal;  // data inside region
    }
  else
    {
    min = this->Min; max = this->Max;   // region itself
    }

  double *outerBoundaryMin=NULL;
  double *outerBoundaryMax=NULL;

  if (innerBoundaryOnly)
    {
    // We want the distance to the nearest inner boundary, because we are only
    // interested in boundaries such that there may be points on the other
    // side.  This option only makes sense when the point supplied is
    // inside this node (region).

    vtkKdNode *top = this;
    vtkKdNode *up = this->Up;

    while(up) { top = up; up = up->Up; }

    outerBoundaryMin = (useDataBounds? top->MinVal : top->Min);
    outerBoundaryMax = (useDataBounds? top->MaxVal : top->Max);
    }

  double xmax = max[0]; double ymax = max[1]; double zmax = max[2];
  double xmin = min[0]; double ymin = min[1]; double zmin = min[2];

  int xless = (x < xmin);
  int xmore = (x > xmax);
  int yless = (y < ymin);
  int ymore = (y > ymax);
  int zless = (z < zmin);
  int zmore = (z > zmax);

  int withinX = !xless && !xmore;
  int withinY = !yless && !ymore;
  int withinZ = !zless && !zmore;

  int mindim=0;

  if (withinX && withinY && withinZ)  // point is inside the box
    {
    if (!innerBoundaryOnly)
      {
      minDistance = (float)(x - xmin);
      mindim = 0;
  
      if ((dist = (float)(xmax - x)) < minDistance)
        {
        mindim = 1;
        minDistance = dist;
        }
      if ((dist = (float)(y - ymin)) < minDistance)
        {
        mindim = 2;
        minDistance = dist;
        }
      if ((dist = (float)(ymax - y)) < minDistance)
        {
        mindim = 3;
        minDistance = dist;
        }
      if ((dist = (float)(z - zmin)) < minDistance)
        {
        mindim = 4;
        minDistance = dist;
        }
      if ((dist = (float)(zmax - z)) < minDistance)
        {
        mindim = 5;
        minDistance = dist;
        }
      }
    else
      {
      int first = 1;
      minDistance = VTK_LARGE_FLOAT; // Suppresses warning message.

      if ((xmin != outerBoundaryMin[0]) && 
          (((dist = (float)(x - xmin)) < minDistance) || first))
        {
        mindim = 0;
        minDistance = dist;
        first = 0;
        }
      if ((xmax != outerBoundaryMax[0]) && 
          (((dist = (float)(xmax - x)) < minDistance) || first))
        {
        mindim = 1;
        minDistance = dist;
        first = 0;
        }
      if ((ymin != outerBoundaryMin[1]) && 
          (((dist = (float)(y - ymin)) < minDistance) || first))
        {
        mindim = 2;
        minDistance = dist;
        first = 0;
        }
      if ((ymax != outerBoundaryMax[1]) && 
          (((dist = (float)(ymax - y)) < minDistance) || first))
        {
        mindim = 3;
        minDistance = dist;
        first = 0;
        }
      if ((zmin != outerBoundaryMin[2]) && 
          (((dist = (float)(z - zmin)) < minDistance) || first))
        {
        mindim = 4;
        minDistance = dist;
        first = 0;
        }
      if ((zmax != outerBoundaryMax[2]) && 
          (((dist = (float)(zmax - z)) < minDistance) || first))
        {
        mindim = 5;
        minDistance = dist;
        }
      }

    minDistance *= minDistance;

    if (p)
      {
      p[0] = x; p[1] = y; p[2] = z;


      if (mindim == 0)      
        {
        p[0] = static_cast<float>(xmin);
        }
      else if (mindim == 1) 
        {
        p[0] = static_cast<float>(xmax);
        }
      else if (mindim == 2) 
        {
        p[1] = static_cast<float>(ymin);
        }
      else if (mindim == 3) 
        {
        p[1] = static_cast<float>(ymax);
        }
      else if (mindim == 4) 
        {
        p[2] = static_cast<float>(zmin);
        }
      else if (mindim == 5) 
        {
        p[2] = static_cast<float>(zmax);
        }
      }
    }
  else if (withinX && withinY)  // point projects orthogonally to a face
    {
    minDistance = static_cast<float>(zless ? zmin - z : z - zmax);
    minDistance *= minDistance;

    if (p)
      {
      p[0] = x; p[1] = y;
      p[2] = static_cast<float>(zless ? zmin : zmax);
      }
    }
  else if (withinX && withinZ)
    {
    minDistance = static_cast<float>(yless ? ymin - y : y - ymax);
    minDistance *= minDistance;

    if (p)
      {
      p[0] = x; p[2] = z;
      p[1] = static_cast<float>(yless ? ymin : ymax);
      }
    }
  else if (withinY && withinZ)
    {
    minDistance = static_cast<float>(xless ? xmin - x : x - xmax);
    minDistance *= minDistance;

    if (p)
      {
      p[1] = y; p[2] = z;
      p[0] = static_cast<float>(xless ? xmin: xmax);
      }
    }
  else if (withinX || withinY || withinZ)   // point is closest to an edge
    {
    edgePt[0] = static_cast<float>(withinX ? x : (xless ? xmin : xmax));
    edgePt[1] = static_cast<float>(withinY ? y : (yless ? ymin : ymax));
    edgePt[2] = static_cast<float>(withinZ ? z : (zless ? zmin : zmax));

    pt3[0] = x; pt3[1] = y; pt3[2] = z;

    minDistance = vtkMath::Distance2BetweenPoints(pt3, edgePt);

    if (p)
      {
      p[0] = edgePt[0]; p[1] = edgePt[1]; p[2] = edgePt[2];
      }
    }
  else                        // point is closest to a corner
    {
    cornerPt[0] = static_cast<float>(xless ? xmin : xmax);
    cornerPt[1] = static_cast<float>(yless ? ymin : ymax);
    cornerPt[2] = static_cast<float>(zless ? zmin : zmax);

    pt3[0] = x; pt3[1] = y; pt3[2] = z;

    minDistance = vtkMath::Distance2BetweenPoints(pt3, cornerPt);

    if (p)
      {
      p[0] = cornerPt[0]; p[1] = cornerPt[1]; p[2] = cornerPt[2];
      }
    }

  return minDistance;
}

//----------------------------------------------------------------------------
void vtkKdNode::AddChildNodes(vtkKdNode *left, vtkKdNode *right)
{     
  this->Left = left;
  this->Right = right;
      
  right->Up = this;
  left->Up  = this;
}       

//----------------------------------------------------------------------------
int vtkKdNode::IntersectsBox(float x0, float x1, float y0, float y1,
                         float z0, float z1, int useDataBounds=0)
{
  return this->IntersectsBox((double)x0, (double)x1, (double)y0, (double)y1,
                         (double)z0, (double)z1, useDataBounds);
}

//----------------------------------------------------------------------------
int vtkKdNode::IntersectsBox(double x0, double x1, double y0, double y1,
                         double z0, double z1, int useDataBounds=0)
{
  double *min, *max;
   
  if (useDataBounds)
    {
    min = this->MinVal;
    max = this->MaxVal;
    }
  else
    {
    min = this->Min;
    max = this->Max;
    }

  if ( (min[0] >= x1) ||
       (max[0] <  x0) ||
       (min[1] >= y1) ||
       (max[1] <  y0) ||
       (min[2] >= z1) ||
       (max[2] <  z0))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkKdNode::IntersectsSphere2(float x, float y, float z, float rSquared,
                                int useDataBounds=0)
{
  return this->IntersectsSphere2((double)x, (double)y, (double)z,
                                 (double)rSquared, useDataBounds);
}

//----------------------------------------------------------------------------
int vtkKdNode::IntersectsSphere2(double x, double y, double z, double rSquared,
                                int useDataBounds=0)
{
  int center = this->ContainsPoint(x, y, z, useDataBounds);

  if (center)
    {
    return 1; 
    }

  float dist2 = this->GetDistance2ToBoundary((float)x, (float)y, (float)z, useDataBounds);
  
  if (dist2 < rSquared)
    {
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKdNode::ContainsBox(float x0, float x1, float y0, float y1,
                         float z0, float z1, int useDataBounds=0)
{
  return this->ContainsBox((double)x0, (double)x1, (double)y0, (double)y1,
                         (double)z0, (double)z1, useDataBounds);
}

//----------------------------------------------------------------------------
int vtkKdNode::ContainsBox(double x0, double x1, double y0, double y1,
                         double z0, double z1, int useDataBounds=0)
{
  double *min, *max;

  if (useDataBounds)
    {
    min = this->MinVal;
    max = this->MaxVal;
    }
  else
    {
    min = this->Min;
    max = this->Max;
    }

  if ( (min[0] >= x0) ||
       (max[0] <  x1) ||
       (min[1] >= y0) ||
       (max[1] <  y1) ||
       (min[2] >= z0) ||
       (max[2] <  z1))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkKdNode::ContainsPoint(float x, float y, float z, int useDataBounds=0)
{
  return this->ContainsPoint((double)x, (double)y, (double)z, useDataBounds);
}

//----------------------------------------------------------------------------
int vtkKdNode::ContainsPoint(double x, double y, double z, int useDataBounds=0)
{
  double *min, *max;

  if (useDataBounds)
    {
    min = this->MinVal;
    max = this->MaxVal;
    }
  else
    {
    min = this->Min;
    max = this->Max;
    }

  // points on a boundary are arbitrarily assigned to the region
  // for which they are on the upper boundary

  if ( (min[0] >= x) ||
       (max[0] <  x) ||
       (min[1] >= y) ||
       (max[1] <  y) ||
       (min[2] >= z) ||
       (max[2] <  z))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkKdNode::IntersectsRegion(vtkPlanesIntersection *pi, int useDataBounds)
{
  double x0, x1, y0, y1, z0, z1;
  vtkPoints *box = vtkPoints::New();

  box->SetNumberOfPoints(8);

  double *min, *max;

  if (useDataBounds)
    {
    min = this->MinVal;
    max = this->MaxVal;
    }
  else
    {
    min = this->Min;
    max = this->Max;
    }

  x0 = min[0]; x1 = max[0];
  y0 = min[1]; y1 = max[1];
  z0 = min[2]; z1 = max[2];

  box->SetPoint(0, x1, y0, z1);
  box->SetPoint(1, x1, y0, z0);
  box->SetPoint(2, x1, y1, z0);
  box->SetPoint(3, x1, y1, z1);
  box->SetPoint(4, x0, y0, z1);
  box->SetPoint(5, x0, y0, z0);
  box->SetPoint(6, x0, y1, z0);
  box->SetPoint(7, x0, y1, z1);

  int intersects = pi->IntersectsRegion(box);

  box->Delete();

  return intersects;
}

//----------------------------------------------------------------------------
int vtkKdNode::IntersectsCell(vtkCell *cell, int useDataBounds, int cellRegion)
{   
  int i;
  
  if ((useDataBounds==0) && (cellRegion >= 0))
    {
    if ( (cellRegion >= this->MinId) && (cellRegion <= this->MaxId))
      {
      return 1;    // the cell centroid is contained in this spatial region
      }
    }

  double *cellBounds;
  int deleteCellBounds = (this->CellBoundsCache == NULL);
  
  if (deleteCellBounds)
    {
    cellBounds = new double [6];
    vtkKdTree::SetCellBounds(cell, cellBounds);
    }
  else
    {
    cellBounds = this->CellBoundsCache;
    }

  int intersects = -1;
  int dim = cell->GetCellDimension();

  if (!this->IntersectsBox(cellBounds[0], cellBounds[1],
                           cellBounds[2], cellBounds[3],
                           cellBounds[4], cellBounds[5], useDataBounds) )
    {
    intersects = 0;   // cell bounding box is outside region
    }
  else if ( this->ContainsBox(cellBounds[0], cellBounds[1],
                         cellBounds[2], cellBounds[3],
                         cellBounds[4], cellBounds[5], useDataBounds) )
    {
    intersects = 1;  // cell bounding box is completely inside region
    }
  else
    {
    // quick test - if any of the points are in this region,
    // then it intersects

    vtkPoints *pts = cell->Points;
    int npts = pts->GetNumberOfPoints();
    double *pt = pts->GetPoint(0);

    for (i=0; i < npts ; i++)
      {
      if (this->ContainsPoint(pt[0],pt[1],pt[2], useDataBounds))
        {
        intersects = 1;
        break;
        }
      pt += 3;
      }

    if ((dim == 0) && (intersects != 1))
      {
      intersects = 0;   // it's a point set and no points intersect
      }
    }
  
  if (intersects != -1)
    { 
    if (deleteCellBounds)
      {
      delete [] cellBounds;
      }
    return intersects;
    }

  vtkPoints *pts = cell->Points;
  int npts = pts->GetNumberOfPoints();

  // determine if cell intersects region

  intersects = 0;

  if (dim == 1)    // lines
    {
    double *p2 = pts->GetPoint(0);
    double *p1;
    double dir[3], x[3], t;

    double regionBounds[6];

    this->GetBounds(regionBounds);

    for (i=0; i < npts - 1 ; i++)
      {
      p1 = p2;
      p2 = p1 + 3;

      dir[0] = p2[0] - p1[0]; dir[1] = p2[1] - p1[1]; dir[2] = p2[2] - p1[2];

      char result = vtkBox::IntersectBox(regionBounds, p1, dir, x, t);

      intersects = (result != 0);

      if (intersects)
        {
        break;
        }
      }
    }
  else if (dim == 2)     // polygons
    {
    double *min, *max;

    if (useDataBounds)
      {
      min = this->MinVal;
      max = this->MaxVal;
      } 
    else
      {
      min = this->Min;
      max = this->Max;
      }
    double regionBounds[6];

    regionBounds[0] = min[0], regionBounds[1] = max[0];
    regionBounds[2] = min[1], regionBounds[3] = max[1];
    regionBounds[4] = min[2], regionBounds[5] = max[2];

    if (cell->GetCellType() == VTK_TRIANGLE_STRIP)
      {
      vtkPoints *triangle = vtkPoints::New();

      triangle->SetNumberOfPoints(3);

      triangle->SetPoint(0, pts->GetPoint(0));
      triangle->SetPoint(1, pts->GetPoint(1));
      
      int newpoint = 2;
      
      for (i=2; i<npts; i++)
        {
        triangle->SetPoint(newpoint, pts->GetPoint(i));
  
        newpoint = (newpoint == 2) ? 0 : newpoint+1;
    
        intersects =
          vtkPlanesIntersection::PolygonIntersectsBBox(regionBounds, triangle);

        if (intersects)
          {
          break;
          }
        }
      triangle->Delete();
      } 
    else
      {
      intersects =
        vtkPlanesIntersection::PolygonIntersectsBBox(regionBounds, pts);
      }
    }   
  else if (dim == 3)     // 3D cells
    {
    vtkPlanesIntersection *pi = vtkPlanesIntersection::Convert3DCell(cell);

    intersects = this->IntersectsRegion(pi, useDataBounds);
    
    pi->Delete();
    }
  
  if (deleteCellBounds)
    {
    delete [] cellBounds;
    }
  
  return intersects;
}

//----------------------------------------------------------------------------
void vtkKdNode::PrintNode(int depth)
{
  if ( (depth < 0) || (depth > 19))
    {
    depth = 19;
    }
      
  if (this->Id > -1)
    {
    printf("%s x (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f) - %d cells, #%d %s\n",
      vtkKdNode::LevelMarker[depth],
      this->Min[0], this->Max[0],
      this->Min[1], this->Max[1],
      this->Min[2], this->Max[2],
      this->NumCells, this->Id, this->Left ? "" : "(leaf node)" );
    }
  else
    {
    printf("%s x (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f) - %d cells, #%d-%d %s\n",
      vtkKdNode::LevelMarker[depth],
      this->Min[0], this->Max[0],
      this->Min[1], this->Max[1],
      this->Min[2], this->Max[2],
      this->NumCells, this->MinId, this->MaxId, this->Left ? "" : "(leaf node)" );
    }
}

//----------------------------------------------------------------------------
void vtkKdNode::PrintVerboseNode(int depth)
{
  if ( (depth < 0) || (depth > 19)) 
    {
    depth = 19;
    }
      
  printf("%s Space (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f)\n",
      vtkKdNode::LevelMarker[depth],
      this->Min[0], this->Max[0],
      this->Min[1], this->Max[1],
      this->Min[2], this->Max[2]);
  printf("%s Data (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f)\n",
      vtkKdNode::LevelMarker[depth],
      this->MinVal[0], this->MaxVal[0],
      this->MinVal[1], this->MaxVal[1],
      this->MinVal[2], this->MaxVal[2]);
  if (this->Id == -1)
    {
    printf("%s %d cells, id range %d - %d, cut next along %d, left %p, right %p, up %p\n",
      vtkKdNode::LevelMarker[depth],                                                              this->NumCells, this->MinId, this->MaxId, this->Dim, this->Left, this->Right, this->Up);
    }
  else
    {
    printf("%s %d cells, id %d, cut next along %d, left %p, right %p, up %p\n",
      vtkKdNode::LevelMarker[depth],
      this->NumCells, this->Id, this->Dim, this->Left, this->Right, this->Up);
    }
  printf("%s dim: %d\n\n",vtkKdNode::LevelMarker[depth],this->Dim);
}
// end of vtkKdNode -------------------------------------------

// Timing data ---------------------------------------------

#include "vtkTimerLog.h"

#define MSGSIZE 60

static char dots[MSGSIZE] = "...........................................................";
static char msg[MSGSIZE];

//----------------------------------------------------------------------------
static char * makeEntry(const char *s)
{
  memcpy(msg, dots, MSGSIZE);
  int len = strlen(s);
  len = (len >= MSGSIZE) ? MSGSIZE-1 : len;

  memcpy(msg, s, len);

  return msg;
}

#define TIMER(s)         \
  if (this->Timing)      \
    {                    \
    char *s2 = makeEntry(s);               \
    if (this->TimerLog == NULL){           \
      this->TimerLog = vtkTimerLog::New(); \
      }                                    \
    this->TimerLog->MarkStartEvent(s2);    \
    }

#define TIMERDONE(s) \
  if (this->Timing){ char *s2 = makeEntry(s); this->TimerLog->MarkEndEvent(s2); }

// Timing data ---------------------------------------------

vtkStandardNewMacro(vtkKdTree);

//----------------------------------------------------------------------------
vtkKdTree::vtkKdTree()
{
  this->FudgeFactor = 0;
  this->MaxLevel = 20;
  this->Level    = 0;

  this->ValidDirections =
  (1 << vtkKdTree::XDIM) | (1 << vtkKdTree::YDIM) | (1 << vtkKdTree::ZDIM);

  this->MinCells = 100;
  this->NumRegions     = 0;

  this->DataSets = NULL;
  this->NumDataSets = 0;

  this->Top      = NULL;
  this->RegionList   = NULL;

  this->Timing = 0;
  this->TimerLog = NULL;

  this->NumDataSetsAllocated = 0;
  this->IncludeRegionBoundaryCells = 0;
  this->GenerateRepresentationUsingDataBounds = 0;
  this->ComputeIntersectionsUsingDataBounds = 0;

  this->InitializeCellLists();
  this->CellRegionList = NULL;

  this->NumberOfLocatorPoints = 0;
  this->LocatorPoints = NULL;
  this->LocatorIds = NULL;
  this->LocatorRegionLocation = NULL;

  this->LastDataCacheSize = 0;
  this->ClearLastBuildCache();
}

//----------------------------------------------------------------------------
void vtkKdTree::DeleteNodes(vtkKdNode *nd)
{   
  if (nd->Left)
    {
     vtkKdTree::DeleteNodes(nd->Left);
     delete nd->Left;
     nd->Left = NULL;
    }
  if (nd->Right)
    {
     vtkKdTree::DeleteNodes(nd->Right);
     delete nd->Right;
     nd->Right = NULL;
    }
  return;
}

//----------------------------------------------------------------------------
void vtkKdTree::InitializeCellLists()
{ 
  this->CellList.dataSet       = NULL;
  this->CellList.regionIds     = NULL;
  this->CellList.nRegions      = 0;
  this->CellList.cells         = NULL;
  this->CellList.boundaryCells = NULL;
  this->CellList.emptyList = NULL;
}

//----------------------------------------------------------------------------
void vtkKdTree::DeleteCellLists()
{ 
  int i;
  int num = this->CellList.nRegions;
  
  if (this->CellList.regionIds)
    {
    delete [] this->CellList.regionIds;
    }
  
  if (this->CellList.cells)
    {
    for (i=0; i<num; i++)
      {
      this->CellList.cells[i]->Delete();
      }

    delete [] this->CellList.cells;
    }

  if (this->CellList.boundaryCells) 
    {
    for (i=0; i<num; i++)
      {
      this->CellList.boundaryCells[i]->Delete();
      }
    delete [] this->CellList.boundaryCells;
    }

  if (this->CellList.emptyList)
    {
    this->CellList.emptyList->Delete();
    }

  this->InitializeCellLists();

  return;
}

//----------------------------------------------------------------------------
vtkKdTree::~vtkKdTree()
{
  if (this->DataSets)
    {
    for (int i=0; i<this->NumDataSetsAllocated; i++)
      {
      if (this->DataSets[i])
        {
        this->DataSets[i]->UnRegister(this);
        }
      }
    delete [] (this->DataSets);
    }

  this->FreeSearchStructure();

  this->DeleteCellLists();

  if (this->CellRegionList)
    {
    delete [] this->CellRegionList;
    this->CellRegionList = NULL;
    }

  if (this->TimerLog)
    {
    this->TimerLog->Delete();
    }

  this->ClearLastBuildCache();
}

//----------------------------------------------------------------------------
// Add and remove data sets.  We don't update this->Modify() here, because
// changing the data sets doesn't necessarily require rebuilding the
// k-d tree.  We only need to build a new k-d tree in BuildLocator if
// the geometry has changed, and we check for that with NewGeometry in
// BuildLocator.  We Modify() for changes that definitely require a
// rebuild of the tree, like changing the depth of the k-d tree.

void vtkKdTree::SetNthDataSet(int idx, vtkDataSet *set)
{
  if (idx < 0)
    {
    vtkErrorMacro(<< "vtkKdTree::SetNthDataSet invalid index");
    return;
    }

  if (idx >= this->NumDataSetsAllocated)
    {
    int oldSize = this->NumDataSetsAllocated;
    int newSize = oldSize + 4;

    vtkDataSet **tmp = this->DataSets;
    this->DataSets = new vtkDataSet * [newSize];

    if (!this->DataSets)
      {
      vtkErrorMacro(<<"vtkKdTree::SetDataSet memory allocation");
      return;
      }

    memset(this->DataSets, 0, sizeof(vtkDataSet *) * newSize);

    if (tmp)
      {
      memcpy(this->DataSets, tmp, sizeof(vtkDataSet *) * oldSize);
      delete [] tmp;
      }

    this->NumDataSetsAllocated = newSize;
    }

  if (this->DataSets[idx] == set)
    {
    return;
    }

  if (this->DataSets[idx])
    {
    this->DataSets[idx]->UnRegister(this);
    this->NumDataSets--;
    }

  this->DataSets[idx] = set;

  if (set)
    {
    this->DataSets[idx]->Register(this);
    this->NumDataSets++;
    }

  if (idx == 0)
    {
    vtkLocator::SetDataSet(set);
    }
}
void vtkKdTree::SetDataSet(vtkDataSet *set)
{
  this->SetNthDataSet(0, set);
}
void vtkKdTree::AddDataSet(vtkDataSet *set)
{
  if (set == NULL)
    {
    return;
    }

  int firstSlot = this->NumDataSetsAllocated;

  for (int i=0; i < this->NumDataSetsAllocated; i++)
    {
    if (this->DataSets[i] == set)
      {
      return;   // already have it
      }
    if ((firstSlot == this->NumDataSetsAllocated) && (this->DataSets[i] == NULL))
      {
      firstSlot = i;
      }
    }

  this->SetNthDataSet(firstSlot, set);
}
void vtkKdTree::RemoveDataSet(vtkDataSet *set)
{
  if (set == NULL) 
    {
    return;
    }

  int i;
  int removeSet = -1;

  for (i=0; i<this->NumDataSetsAllocated; i++)
    {
    if (this->DataSets[i] == set)
      {
       removeSet = i;
       break;
      }
    }
  if (removeSet >= 0)
    {
    this->RemoveDataSet(removeSet);
    }
  else
    {
    vtkErrorMacro( << "vtkKdTree::RemoveDataSet not a valid data set");
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::RemoveDataSet(int which)
{
  if ( (which < 0) || (which >= this->NumDataSetsAllocated))
    {
    vtkErrorMacro( << "vtkKdTree::RemoveDataSet not a valid data set");
    return;
    }

  if (this->CellList.dataSet == this->DataSets[which])
    {
    this->DeleteCellLists();
    }

  if (this->DataSets[which])
    {
    this->DataSets[which]->UnRegister(this);
    this->DataSets[which] = NULL;
    this->NumDataSets--;
    }
}
vtkDataSet *vtkKdTree::GetDataSet(int n)
{
  if ((n < 0) ||
      (n >= this->NumDataSets))
    {
    vtkErrorMacro(<< "vtkKdTree::GetDataSet. invalid data set number");
    return NULL;
    }

  int set = -1;

  for (int i=0; i<this->NumDataSetsAllocated; i++)
    {
      if (this->DataSets[i])
        {
        set++;
        if (set == n)
          {
          break;
          }
        }
    }

  if (set >= 0)
    {
    return this->DataSets[set];
    }
  else
    {
    return NULL;
    }
}
int vtkKdTree::GetDataSet(vtkDataSet *set)
{
  int i;
  int whichSet = -1;

  for (i=0; i<this->NumDataSetsAllocated; i++)
    {
    if (this->DataSets[i] == set)
      {
      whichSet = i;
      break;
      } 
    }
  return whichSet;
}
int vtkKdTree::GetDataSetsNumberOfCells(int from, int to)
{
  int numCells = 0;

  for (int i=from; i<=to; i++)
    {
    if (this->DataSets[i])
      {
      numCells += this->DataSets[i]->GetNumberOfCells();
      }
    }
  
  return numCells;
}

//----------------------------------------------------------------------------
int vtkKdTree::GetNumberOfCells()
{ 
  return this->GetDataSetsNumberOfCells(0, this->NumDataSetsAllocated - 1);
}

//----------------------------------------------------------------------------
void vtkKdTree::GetBounds(float *bounds)
{
  if (this->Top)
    {
    this->Top->GetBounds(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::GetBounds(double *bounds)
{
  if (this->Top)
    {
    this->Top->GetBounds(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::GetRegionBounds(int regionID, float bounds[6])
{
  double b[6];

  vtkKdTree::GetRegionBounds(regionID, b);

  bounds[0] = (float)b[0]; bounds[1] = (float)b[1]; bounds[2] = (float)b[2];
  bounds[3] = (float)b[3]; bounds[4] = (float)b[4]; bounds[5] = (float)b[5];
}

//----------------------------------------------------------------------------
void vtkKdTree::GetRegionBounds(int regionID, double bounds[6])
{
  if ( (regionID < 0) || (regionID >= this->NumRegions))
    {
    vtkErrorMacro( << "vtkKdTree::GetRegionBounds invalid region");
    return;
    }

  vtkKdNode *node = this->RegionList[regionID];

  bounds[0] = node->Min[0];
  bounds[2] = node->Min[1];
  bounds[4] = node->Min[2];
  bounds[1] = node->Max[0];
  bounds[3] = node->Max[1];
  bounds[5] = node->Max[2];
}

//----------------------------------------------------------------------------
void vtkKdTree::GetRegionDataBounds(int regionID, float bounds[6])
{
  double b[6];

  vtkKdTree::GetRegionDataBounds(regionID, b);

  bounds[0] = (float)b[0]; bounds[1] = (float)b[1]; bounds[2] = (float)b[2];
  bounds[3] = (float)b[3]; bounds[4] = (float)b[4]; bounds[5] = (float)b[5];
}

//----------------------------------------------------------------------------
void vtkKdTree::GetRegionDataBounds(int regionID, double bounds[6])
{
  if ( (regionID < 0) || (regionID >= this->NumRegions))
    {
    vtkErrorMacro( << "vtkKdTree::GetRegionDataBounds invalid region");
    return;
    }

  vtkKdNode *node = this->RegionList[regionID];

  bounds[0] = node->MinVal[0];
  bounds[2] = node->MinVal[1];
  bounds[4] = node->MinVal[2];
  bounds[1] = node->MaxVal[0];
  bounds[3] = node->MaxVal[1];
  bounds[5] = node->MaxVal[2];
}

//----------------------------------------------------------------------------
vtkKdNode **vtkKdTree::_GetRegionsAtLevel(int level, vtkKdNode **nodes, vtkKdNode *kd)
{
  if (level > 0)
    {
    vtkKdNode **nodes0 = _GetRegionsAtLevel(level-1, nodes, kd->Left);
    vtkKdNode **nodes1 = _GetRegionsAtLevel(level-1, nodes0, kd->Right);

    return nodes1;
    }
  else
    {
    nodes[0] = kd;
    return nodes+1;
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::GetRegionsAtLevel(int level, vtkKdNode **nodes)
{
  if ( (level < 0) || (level > this->Level)) 
    {
    return;
    }

  vtkKdTree::_GetRegionsAtLevel(level, nodes, this->Top);

  return;
}

//----------------------------------------------------------------------------
void vtkKdTree::GetLeafNodeIds(vtkKdNode *node, vtkIntArray *ids)
{
  if (node->Id < 0)
    {
    vtkKdTree::GetLeafNodeIds(node->Left, ids);
    vtkKdTree::GetLeafNodeIds(node->Right, ids);
    }
  else
    {
    ids->InsertNextValue(node->Id);
    }
  return;
}

//----------------------------------------------------------------------------
float *vtkKdTree::ComputeCellCenters()
{
  vtkDataSet *allSets = NULL;
  return this->ComputeCellCenters(allSets);
}

//----------------------------------------------------------------------------
float *vtkKdTree::ComputeCellCenters(int set)
{
  if ( (set < 0) || 
       (set >= this->NumDataSetsAllocated) ||
       (this->DataSets[set] == NULL))
    {
    vtkErrorMacro(<<"vtkKdTree::ComputeCellCenters no such data set");
    return NULL;
    }
  return this->ComputeCellCenters(this->DataSets[set]);
}

//----------------------------------------------------------------------------
float *vtkKdTree::ComputeCellCenters(vtkDataSet *set)
{
  int i,j;
  int totalCells;

  if (set)
    {
    totalCells = set->GetNumberOfCells();
    }
  else
    {
    totalCells = this->GetNumberOfCells();   // all data sets
    }

  if (totalCells == 0) 
    {
    return NULL;
    }

  float *center = new float [3 * totalCells];

  if (!center)
    {
    return NULL;
    }

  int maxCellSize = 0;

  if (set)
    {
      maxCellSize = set->GetMaxCellSize();
    }
  else
    {
    for (i=0; i<this->NumDataSetsAllocated; i++)
      {
      if (this->DataSets[i])
        {
        int cellSize = this->DataSets[i]->GetMaxCellSize();
        maxCellSize = (cellSize > maxCellSize) ? cellSize : maxCellSize;
        }
      }
    }

  double *weights = new double [maxCellSize];

  float *cptr = center;
  double dcenter[3];

  if (set)
    {
    for (j=0; j<totalCells; j++)
      {
      this->ComputeCellCenter(set->GetCell(j), dcenter, weights);
      cptr[0] = (float)dcenter[0];
      cptr[1] = (float)dcenter[1];
      cptr[2] = (float)dcenter[2];
      cptr += 3;
      }
    }
  else
    {
    for (i=0; i<this->NumDataSetsAllocated; i++)
      {
      if (!this->DataSets[i]) 
        {
        continue;
        }
  
      vtkDataSet *iset = this->DataSets[i];

      int nCells = iset->GetNumberOfCells();

      for (j=0; j<nCells; j++)
        {
        this->ComputeCellCenter(iset->GetCell(j), dcenter, weights);
        cptr[0] = (float)dcenter[0];
        cptr[1] = (float)dcenter[1];
        cptr[2] = (float)dcenter[2];
        cptr += 3;
        }
      }
    }

  delete [] weights;

  return center;
}

//----------------------------------------------------------------------------
void vtkKdTree::ComputeCellCenter(vtkDataSet *set, int cellId, float *center)
{
  double dcenter[3];

  this->ComputeCellCenter(set, cellId, dcenter);

  center[0] = (float)dcenter[0];
  center[1] = (float)dcenter[1];
  center[2] = (float)dcenter[2];
}

//----------------------------------------------------------------------------
void vtkKdTree::ComputeCellCenter(vtkDataSet *set, int cellId, double *center)
{
  int setNum;

  if (set)
    {
    setNum = this->GetDataSet(set);

    if ( setNum < 0)
      {
      vtkErrorMacro(<<"vtkKdTree::ComputeCellCenter invalid data set");
      return;
      } 
    }
  else
    {
    setNum = 0;
    set = this->DataSets[0];
    }
      
  if ( (cellId < 0) || (cellId >= set->GetNumberOfCells()))
    {
    vtkErrorMacro(<<"vtkKdTree::ComputeCellCenter invalid cell ID");
    return;
    }

  double *weights = new double [set->GetMaxCellSize()];

  this->ComputeCellCenter(set->GetCell(cellId), center, weights);

  delete [] weights;

  return;
}

//----------------------------------------------------------------------------
void vtkKdTree::ComputeCellCenter(vtkCell *cell, double *center, double *weights)
{   
  double pcoords[3];
  
  int subId = cell->GetParametricCenter(pcoords);
    
  cell->EvaluateLocation(subId, pcoords, center, weights);
      
  return;
} 

//----------------------------------------------------------------------------
// Build the kdtree structure based on location of cell centroids.
//
void vtkKdTree::BuildLocator()
{
  int nCells=0;
  int i;

  if ((this->Top != NULL) && 
      (this->BuildTime > this->GetMTime()) &&
      (this->NewGeometry() == 0))
    {
    return;
    }

  nCells = this->GetNumberOfCells();

  if (nCells == 0)
    {
     vtkErrorMacro( << "vtkKdTree::BuildLocator - No cells to subdivide");
     return;
    }

  vtkDebugMacro( << "Creating Kdtree" );

  if ((this->Timing) && (this->TimerLog == NULL))
    {
    this->TimerLog = vtkTimerLog::New();
    }

  TIMER("Set up to build k-d tree");

  this->FreeSearchStructure();

  // volume bounds - push out a little if flat

  double setBounds[6], volBounds[6];
  int first = 1;

  for (i=0; i<this->NumDataSetsAllocated; i++)
    {
    if (this->DataSets[i] == NULL) 
      {
      continue;
      }

    if (first)
      {
      this->DataSets[i]->GetBounds(volBounds);
      first = 0;
      }
    else
      {
      this->DataSets[i]->GetBounds(setBounds);
      if (setBounds[0] < volBounds[0]) 
        {
        volBounds[0] = setBounds[0];
        }
      if (setBounds[2] < volBounds[2]) 
        {
        volBounds[2] = setBounds[2];
        }
      if (setBounds[4] < volBounds[4]) 
        {
        volBounds[4] = setBounds[4];
        }
      if (setBounds[1] > volBounds[1]) 
        {
        volBounds[1] = setBounds[1];
        }
      if (setBounds[3] > volBounds[3]) 
        {
        volBounds[3] = setBounds[3];
        }
      if (setBounds[5] > volBounds[5]) 
        {
        volBounds[5] = setBounds[5];
        }
      }
    }

  double diff[3], aLittle = 0.0;
  this->MaxWidth = 0.0;

  for (i=0; i<3; i++)
    {
     diff[i] = volBounds[2*i+1] - volBounds[2*i];
     this->MaxWidth = static_cast<float>(
       (diff[i] > this->MaxWidth) ? diff[i] : this->MaxWidth);
    }

  this->FudgeFactor = this->MaxWidth * 10e-6;

  aLittle = this->MaxWidth / 100.0;

  for (i=0; i<3; i++)
    {
    if (diff[i] <= 0)
      {
      volBounds[2*i]   -= aLittle;
      volBounds[2*i+1] += aLittle;
      }
    else // need lower bound to be strictly less than any point in decomposition
      {
      volBounds[2*i] -= this->FudgeFactor;
      }
    }
  TIMERDONE("Set up to build k-d tree");
   
  // cell centers - basis of spacial decomposition

  TIMER("Create centroid list");

  float *ptarray = this->ComputeCellCenters();

  TIMERDONE("Create centroid list");

  if (!ptarray)
    {
    vtkErrorMacro( << "vtkKdTree::BuildLocator - insufficient memory");
    return;
    }

  // create kd tree structure that balances cell centers

  vtkKdNode *kd = this->Top = new vtkKdNode();

  kd->SetBounds((double)volBounds[0], (double)volBounds[1], 
                (double)volBounds[2], (double)volBounds[3], 
                (double)volBounds[4], (double)volBounds[5]);

  kd->SetNumberOfCells(nCells);

  kd->SetDataBounds((double)volBounds[0], (double)volBounds[1],
                (double)volBounds[2], (double)volBounds[3],
                (double)volBounds[4], (double)volBounds[5]); 

  TIMER("Build tree");

  this->DivideRegion(kd, ptarray, NULL, this->MaxLevel);

  TIMERDONE("Build tree");

  // In the process of building the k-d tree regions,
  //   the cell centers became reordered, so no point
  //   in saving them, for example to build cell lists.

  delete [] ptarray;

  this->SetActualLevel();
  this->BuildRegionList();

  this->UpdateBuildTime();

  return;
}

//----------------------------------------------------------------------------
int vtkKdTree::ComputeLevel(vtkKdNode *kd)
{
  if (!kd)
    {
    return 0;
    }
  
  int iam = 1;

  if (kd->Left != NULL)
    {
     int depth1 = vtkKdTree::ComputeLevel(kd->Left);
     int depth2 = vtkKdTree::ComputeLevel(kd->Right);

     if (depth1 > depth2) 
       {
       iam += depth1;
       }
     else
       {
       iam += depth2;
       }
    }
  return iam;
}

//----------------------------------------------------------------------------
int vtkKdTree::SelectCutDirection(vtkKdNode *kd)
{
  int dim=0, i;

  int xdir = 1 << vtkKdTree::XDIM;
  int ydir = 1 << vtkKdTree::YDIM;
  int zdir = 1 << vtkKdTree::ZDIM;

  // determine direction in which to divide this region

  if (this->ValidDirections == xdir)
    {
    dim = vtkKdTree::XDIM;
    }
  else if (this->ValidDirections == ydir)
    {
    dim = vtkKdTree::YDIM;
    }
  else if (this->ValidDirections == zdir)
    {
    dim = vtkKdTree::ZDIM;
    }
  else
    {
    // divide in the longest direction, for more compact regions

    double diff[3], dataBounds[6], maxdiff;
    kd->GetDataBounds(dataBounds);

    for (i=0; i<3; i++)
      { 
      diff[i] = dataBounds[i*2+1] - dataBounds[i*2];
      }

    maxdiff = -1.0;

    if ((this->ValidDirections & xdir) && (diff[vtkKdTree::XDIM] > maxdiff))
      {
      dim = vtkKdTree::XDIM;
      maxdiff = diff[vtkKdTree::XDIM];
      }

    if ((this->ValidDirections & ydir) && (diff[vtkKdTree::YDIM] > maxdiff))
      {
      dim = vtkKdTree::YDIM;
      maxdiff = diff[vtkKdTree::YDIM];
      }

    if ((this->ValidDirections & zdir) && (diff[vtkKdTree::ZDIM] > maxdiff))
      {
      dim = vtkKdTree::ZDIM;
      }
    }
  return dim;
}

//----------------------------------------------------------------------------
int vtkKdTree::DivideRegion(vtkKdNode *kd, float *c1, int *ids, int nlevels)
{
  if (nlevels == 0) 
    {
    return 0;
    }

  int minCells = this->GetMinCells();

  int npoints = kd->GetNumberOfCells();

  if ((npoints < 2) || (minCells && (minCells > (npoints/2)))) 
    {
    return 0;
    }

  int maxdim = this->SelectCutDirection(kd);

  kd->SetDim(maxdim);

  int dim1 = maxdim;   // best cut direction
  int dim2 = -1;       // other valid cut directions
  int dim3 = -1;

  int otherDirections = this->ValidDirections ^ (1 << maxdim);

  if (otherDirections)
    {
    int x = otherDirections & (1 << vtkKdTree::XDIM);
    int y = otherDirections & (1 << vtkKdTree::YDIM);
    int z = otherDirections & (1 << vtkKdTree::ZDIM);

    if (x)
      {
      dim2 = vtkKdTree::XDIM;

      if (y)
        {
        dim3 = vtkKdTree::YDIM;
        }
      else if (z)
        {
        dim3 = vtkKdTree::ZDIM;
        }
      }
    else if (y)
      {
      dim2 = vtkKdTree::YDIM;

      if (z)
        {
        dim3 = vtkKdTree::ZDIM;
        }
      }
    else if (z)
      {
      dim2 = vtkKdTree::ZDIM;
      }
    }

  this->DoMedianFind(kd, c1, ids, dim1, dim2, dim3);

  if (kd->Left == NULL)
    {
    return 0;   // unable to divide region further
    }

  int nleft = kd->Left->GetNumberOfCells();

  int *leftIds  = ids;
  int *rightIds = ids ? ids + nleft : NULL;
  
  this->DivideRegion(kd->Left, c1, leftIds, nlevels - 1);
  
  this->DivideRegion(kd->Right, c1 + nleft*3, rightIds, nlevels - 1);
  
  return 0;
}

//----------------------------------------------------------------------------
// Rearrange the point array.  Try dim1 first.  If there's a problem
// go to dim2, then dim3.
//
void vtkKdTree::DoMedianFind(vtkKdNode *kd, float *c1, int *ids,
                             int dim1, int dim2, int dim3)
{
  double coord;
  int dim;
  int midpt;

  int npoints = kd->GetNumberOfCells();

  int dims[3];

  dims[0] = dim1; dims[1] = dim2; dims[2] = dim3;

  for (dim = 0; dim < 3; dim++)
    {
    if (dims[dim] < 0) 
      {
      break;
      }

    midpt = vtkKdTree::Select(dims[dim], c1, ids, npoints, coord);

    if (midpt == 0) 
      {
      continue;    // fatal
      }

    kd->SetDim(dims[dim]);

    vtkKdTree::AddNewRegions(kd, c1, midpt, dims[dim], coord);

    break;   // division is fine
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::AddNewRegions(vtkKdNode *kd, float *c1, int midpt, int dim, double coord)
{
  vtkKdNode *left = new vtkKdNode();
  vtkKdNode *right = new vtkKdNode();

  int npoints = kd->GetNumberOfCells();

  int nleft = midpt;
  int nright = npoints - midpt;

  kd->AddChildNodes(left, right);

  double bounds[6];
  kd->GetBounds(bounds);

  left->SetBounds(
     bounds[0], ((dim == vtkKdTree::XDIM) ? coord : bounds[1]),
     bounds[2], ((dim == vtkKdTree::YDIM) ? coord : bounds[3]),
     bounds[4], ((dim == vtkKdTree::ZDIM) ? coord : bounds[5]));
  
  left->SetNumberOfCells(nleft);
  
  right->SetBounds(
     ((dim == vtkKdTree::XDIM) ? coord : bounds[0]), bounds[1],
     ((dim == vtkKdTree::YDIM) ? coord : bounds[2]), bounds[3],
     ((dim == vtkKdTree::ZDIM) ? coord : bounds[4]), bounds[5]); 
  
  right->SetNumberOfCells(nright);
  
  left->SetDataBounds(c1);
  right->SetDataBounds(c1 + nleft*3);
}
// Use Floyd & Rivest (1975) to find the median:
// Given an array X with element indices ranging from L to R, and
// a K such that L <= K <= R, rearrange the elements such that
// X[K] contains the ith sorted element,  where i = K - L + 1, and
// all the elements X[j], j < k satisfy X[j] < X[K], and all the
// elements X[j], j > k satisfy X[j] >= X[K].

#define Exchange(array, ids, x, y) \
  {                                 \
  float temp[3];                    \
  temp[0]        = array[3*x];      \
  temp[1]        = array[3*x + 1];  \
  temp[2]        = array[3*x + 2];  \
  array[3*x]     = array[3*y];      \
  array[3*x + 1] = array[3*y + 1];  \
  array[3*x + 2] = array[3*y + 2];  \
  array[3*y]     = temp[0];         \
  array[3*y + 1] = temp[1];         \
  array[3*y + 2] = temp[2];         \
  if (ids)                          \
    {                               \
    vtkIdType tempid = ids[x];      \
    ids[x]           = ids[y];      \
    ids[y]           = tempid;      \
    }                               \
}

#define sign(x) ((x<0) ? (-1) : (1))
#ifndef max
#define max(x,y) ((x>y) ? (x) : (y))
#endif
#ifndef min
#define min(x,y) ((x<y) ? (x) : (y))
#endif

//----------------------------------------------------------------------------
int vtkKdTree::Select(int dim, float *c1, int *ids, int nvals, double &coord)
{
  int left = 0;
  int mid = nvals / 2;
  int right = nvals -1;

  vtkKdTree::_Select(dim, c1, ids, left, right, mid);

  // We need to be careful in the case where the "mid"
  // value is repeated several times in the array.  We
  // want to roll the dividing index (mid) back to the
  // first occurence in the array, so that there is no
  // ambiguity about which spatial region a given point
  // belongs in.
  //
  // The array has been rearranged (in _Select) like this:
  //
  // All values c1[n], left <= n < mid, satisfy c1[n] <= c1[mid]
  // All values c1[n], mid < n <= right, satisfy c1[n] >= c1[mid]
  //
  // In addition, by careful construction, there is a J <= mid
  // such that
  //
  // All values c1[n], n < J, satisfy c1[n] < c1[mid] STRICTLY
  // All values c1[n], J <= n <= mid, satisfy c1[n] = c1[mid]
  // All values c1[n], mid < n <= right , satisfy c1[n] >= c1[mid]
  //
  // We need to roll back the "mid" value to the "J".  This
  // means our spatial regions are less balanced, but there
  // is no ambiguity regarding which region a point belongs in.

  int midValIndex = mid*3 + dim;

  while ((mid > left) && (c1[midValIndex-3] == c1[midValIndex]))
    {
    mid--;
    midValIndex -= 3;
    }

  if (mid == left)
    {
    return mid;     // failed to divide region
    }

  float leftMax = vtkKdTree::FindMaxLeftHalf(dim, c1, mid);

  coord = ((double)c1[midValIndex] + (double)leftMax) / 2.0;

  return mid;
}

//----------------------------------------------------------------------------
float vtkKdTree::FindMaxLeftHalf(int dim, float *c1, int K)
{
  int i;

  float *Xcomponent = c1 + dim;
  float max = Xcomponent[0];

  for (i=3; i<K*3; i+=3)
    {
    if (Xcomponent[i] > max) 
      {
      max = Xcomponent[i];
      }
    }
  return max;
}

//----------------------------------------------------------------------------
// Note: The indices (L, R, X) into the point array should be vtkIdTypes rather
// than ints, but this change causes the k-d tree build time to double.
// _Select is the heart of this build, called for every sub-interval that
// is to be reordered.  We will leave these as ints now.

void vtkKdTree::_Select(int dim, float *X, int *ids, 
                        int L, int R, int K)
{
  int N, I, J, S, SD, LL, RR;
  float Z, T;

  while (R > L)
    {
    if ( R - L > 600)
      {
      // "Recurse on a sample of size S to get an estimate for the
      // (K-L+1)-th smallest element into X[K], biased slightly so
      // that the (K-L+1)-th element is expected to lie in the
      // smaller set after partitioning"

      N = R - L + 1;
      I = K - L + 1;
      Z = static_cast<float>(log((float)N));
      S = static_cast<int>(.5 * exp(2*Z/3));
      SD = static_cast<int>(.5 * sqrt(Z*S*(N-S)/N) * sign(1 - N/2));
      LL = max(L, K - (I*S/N) + SD);
      RR = min(R, K + (N-1) * S/N + SD);
      _Select(dim, X, ids, LL, RR, K);
      }

    float *Xcomponent = X + dim;   // x, y or z component

    T = Xcomponent[K*3];

    // "the following code partitions X[L:R] about T."

    I = L;
    J = R;

    Exchange(X, ids, L, K);

    if (Xcomponent[R*3] >= T) 
      {
      Exchange(X, ids, R, L);
      }

    while (I < J)
      {
      Exchange(X, ids, I, J);

      while (Xcomponent[(++I)*3] < T);

      while ((J>L) && Xcomponent[(--J)*3] >= T);
      }

    if (Xcomponent[L*3] == T)
      {
      Exchange(X, ids, L, J);
      }
    else
      {
      J++;
      Exchange(X, ids, J, R);
      }

    // "now adjust L,R so they surround the subset containing
    // the (K-L+1)-th smallest element"

    if (J <= K) 
      {
      L = J + 1;
      }
    if (K <= J) 
      {
      R = J - 1;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::SelfRegister(vtkKdNode *kd)
{
  if (kd->Left == NULL)
    {
    this->RegionList[kd->Id] = kd; 
    }
  else
    {
    this->SelfRegister(kd->Left);
    this->SelfRegister(kd->Right);
    }

  return;
}

//----------------------------------------------------------------------------
int vtkKdTree::SelfOrder(int startId, vtkKdNode *kd)
{
  int nextId;

  if (kd->Left == NULL)
    {
    kd->Id = startId;

    kd->MaxId = kd->MinId = startId;

    nextId = startId + 1;
    }
  else
    {
    kd->Id = -1;
    nextId = vtkKdTree::SelfOrder(startId, kd->Left);
    nextId = vtkKdTree::SelfOrder(nextId, kd->Right);

    kd->MinId = startId;
    kd->MaxId = nextId - 1;
    }

  return nextId;
}

//----------------------------------------------------------------------------
// It may be necessary for a user of vtkKdTree to work on convex
// spatial regions.  Here we take a list of region IDs, and return
// the minimum number (N) of convex regions that it can be decomposed
// into, and N lists of the region IDs that make each convex region.
// If the region allocation scheme was "contigous", you are guaranteed
// that the set of regions assigned to each process composes a
// convex spatial region.
//
void vtkKdTree::BuildRegionList()
{
  if (this->Top == NULL) 
    {
    return;
    }

  this->NumRegions = vtkKdTree::SelfOrder(0, this->Top);
  
  this->RegionList = new vtkKdNode * [this->NumRegions];

  this->SelfRegister(this->Top);
}

//----------------------------------------------------------------------------
// K-d tree from points, for finding duplicate and near-by points
//
void vtkKdTree::BuildLocatorFromPoints(vtkPoints *ptArray)
{
  this->BuildLocatorFromPoints(&ptArray, 1);
}

//----------------------------------------------------------------------------
void vtkKdTree::BuildLocatorFromPoints(vtkPoints **ptArrays, int numPtArrays) 
{
  int ptId;
  int i;

  int totalNumPoints = 0;

  for (i = 0; i < numPtArrays; i++)
    {
    totalNumPoints += ptArrays[i]->GetNumberOfPoints();
    }

  if (totalNumPoints < 1)
    {
    vtkErrorMacro(<< "vtkKdTree::BuildLocatorFromPoints - no points");
    return;
    }

  if (totalNumPoints >= VTK_INT_MAX)
    {
     // The heart of the k-d tree build is the recursive median find in
     // _Select.  It rearranges point IDs along with points themselves.
     // When point IDs are stored in an "int" instead of a vtkIdType, 
     // performance doubles.  So we store point IDs in an "int" during
     // the calculation.  This will need to be rewritten if true 64 bit
     // IDs are required.

     vtkErrorMacro(<<
    "BuildLocatorFromPoints - intentional 64 bit error - time to rewrite code");
     return;
    }

  vtkDebugMacro( << "Creating Kdtree" );
  
  if ((this->Timing) && (this->TimerLog == NULL) )
    {
    this->TimerLog = vtkTimerLog::New();
    }

  TIMER("Set up to build k-d tree");

  this->FreeSearchStructure();
  this->ClearLastBuildCache();

  // Fix bounds - (1) push out a little if flat 
  // (2) pull back the x, y and z lower bounds a little bit so that
  // points are clearly "inside" the spatial region.  Point p is
  // "inside" region r = [r1, r2] if r1 < p <= r2.

  double bounds[6], diff[3];

  ptArrays[0]->GetBounds(bounds);

  for (i=1; i<numPtArrays; i++)
    {
    double tmpbounds[6];
    ptArrays[i]->GetBounds(tmpbounds);

    if (tmpbounds[0] < bounds[0]) 
      {
      bounds[0] = tmpbounds[0];
      }
    if (tmpbounds[2] < bounds[2]) 
      {
      bounds[2] = tmpbounds[2];
      }
    if (tmpbounds[4] < bounds[4]) 
      {
      bounds[4] = tmpbounds[4];
      }
    if (tmpbounds[1] > bounds[1]) 
      {
      bounds[1] = tmpbounds[1];
      }
    if (tmpbounds[3] > bounds[3]) 
      {
      bounds[3] = tmpbounds[3];
      }
    if (tmpbounds[5] > bounds[5]) 
      {
      bounds[5] = tmpbounds[5];
      }
    }

  this->MaxWidth = 0.0; 

  for (i=0; i<3; i++)
    {
    diff[i] = bounds[2*i+1] - bounds[2*i];
    this->MaxWidth = (float)
      ((diff[i] > this->MaxWidth) ? diff[i] : this->MaxWidth);
    }

  this->FudgeFactor = this->MaxWidth * 10e-6;

  double aLittle = this->MaxWidth * 10e-2;

  for (i=0; i<3; i++)
    {
    if (diff[i] < aLittle)         // case (1) above
      {
      double temp = bounds[2*i];
      bounds[2*i]   = bounds[2*i+1] - aLittle;
      bounds[2*i+1] = temp + aLittle;
      }
    else                           // case (2) above
      {
      bounds[2*i] -= this->FudgeFactor;
      }
    }

  // root node of k-d tree - it's the whole space
   
  vtkKdNode *kd = this->Top = new vtkKdNode();

  kd->SetBounds((double)bounds[0], (double)bounds[1], 
                (double)bounds[2], (double)bounds[3], 
                (double)bounds[4], (double)bounds[5]);

  kd->SetNumberOfCells(totalNumPoints);

  kd->SetDataBounds((double)bounds[0], (double)bounds[1],
                (double)bounds[2], (double)bounds[3],
                (double)bounds[4], (double)bounds[5]); 


  this->LocatorIds = new int [totalNumPoints];
  this->LocatorPoints = new float [3 * totalNumPoints];

  if ( !this->LocatorPoints || !this->LocatorIds)
    {
    this->FreeSearchStructure();
    vtkErrorMacro(<< "vtkKdTree::BuildLocatorFromPoints - memory allocation");
    return;
    }

  int *ptIds = this->LocatorIds;
  float *points = this->LocatorPoints;

  for (i=0, ptId = 0; i<numPtArrays; i++)
    {
    int npoints = ptArrays[i]->GetNumberOfPoints();
    int nvals = npoints * 3;

    int pointArrayType = ptArrays[i]->GetDataType();

    if (pointArrayType == VTK_FLOAT)
      {
      vtkDataArray *da = ptArrays[i]->GetData();
      vtkFloatArray *fa = vtkFloatArray::SafeDownCast(da);
      memcpy(points + ptId, fa->GetPointer(0), sizeof(float) * nvals );
      ptId += nvals;
      }
    else
      {
      // Hopefully point arrays are usually floats.  This conversion will
      // really slow things down.

      for (vtkIdType ii=0; ii<npoints; ii++)
        {
        double *pt = ptArrays[i]->GetPoint(ii);
        
        points[ptId++] = (float)pt[0]; 
        points[ptId++] = (float)pt[1];
        points[ptId++] = (float)pt[2];
        }
      }
    }

  for (ptId=0; ptId<totalNumPoints; ptId++)
    {
    // _Select dominates DivideRegion algorithm, operating on
    // ints is much fast than operating on long longs

    ptIds[ptId] = ptId;
    }

  TIMERDONE("Set up to build k-d tree");

  TIMER("Build tree");

  this->DivideRegion(kd, points, ptIds, this->MaxLevel);

  this->SetActualLevel();
  this->BuildRegionList();

  // Create a location array for the points of each region

  this->LocatorRegionLocation = new int [this->NumRegions];

  int idx = 0;

  for (int reg = 0; reg < this->NumRegions; reg++)
    {
    this->LocatorRegionLocation[reg] = idx;

    idx += this->RegionList[reg]->NumCells;
    }

  this->NumberOfLocatorPoints = idx;

  TIMERDONE("Build tree");
}

//----------------------------------------------------------------------------
// Query functions subsequent to BuildLocatorFromPoints,
// relating to duplicate and nearby points
//
vtkIdTypeArray *vtkKdTree::BuildMapForDuplicatePoints(float tolerance = 0.0)
{
  int i;

  if (this->LocatorPoints == NULL)
    {
    vtkErrorMacro(<< "vtkKdTree::BuildMapForDuplicatePoints - build locator first");
    return NULL;
    }

  if ((tolerance < 0.0) || (tolerance >= this->MaxWidth))
    {
    vtkErrorMacro(<< "vtkKdTree::BuildMapForDuplicatePoints - invalid tolerance");
    return NULL;
    }

  TIMER("Find duplicate points");

  int *idCount = new int [this->NumRegions];
  int **uniqueFound = new int * [this->NumRegions];

  if (!idCount || !uniqueFound)
    {
    if (idCount) 
      {
      delete [] idCount;
      }
    if (uniqueFound) 
      {
      delete [] uniqueFound;
      }

    vtkErrorMacro(<< "vtkKdTree::BuildMapForDuplicatePoints memory allocation");
    return NULL;
    }

  memset(idCount, 0, sizeof(int) * this->NumRegions);

  for (i=0; i<this->NumRegions; i++)
    {
    uniqueFound[i] = new int [this->RegionList[i]->NumCells];
 
    if (!uniqueFound[i])
      {
      delete [] idCount;
      for (int j=0; j<i; j++) delete [] uniqueFound[j];
      delete [] uniqueFound;
      vtkErrorMacro(<< "vtkKdTree::BuildMapForDuplicatePoints memory allocation");
      return NULL;
      }
    }

  float tolerance2 = tolerance * tolerance;

  vtkIdTypeArray *uniqueIds = vtkIdTypeArray::New();
  uniqueIds->SetNumberOfValues(this->NumberOfLocatorPoints);

  int idx = 0;
  int nextRegionId = 0;
  float *point = this->LocatorPoints;

  while (idx < this->NumberOfLocatorPoints)
    {
    // first point we have in this region

    int currentId = this->LocatorIds[idx];

    int regionId = this->GetRegionContainingPoint(point[0],point[1],point[2]);

    if ((regionId == -1) || (regionId != nextRegionId))
      {
      delete [] idCount;
      for (i=0; i<this->NumRegions; i++) delete [] uniqueFound[i];
      delete [] uniqueFound;
      uniqueIds->Delete();
      vtkErrorMacro(<< "vtkKdTree::BuildMapForDuplicatePoints corrupt k-d tree");
      return NULL; 
      }

    int duplicateFound = -1;

    if ((tolerance > 0.0) && (regionId > 0))
      {
      duplicateFound = this->SearchNeighborsForDuplicate(regionId, point, 
                               uniqueFound, idCount, tolerance, tolerance2);
      }

    if (duplicateFound >= 0)
      {
      uniqueIds->SetValue(currentId, this->LocatorIds[duplicateFound]);
      }
    else
      {
      uniqueFound[regionId][idCount[regionId]++] = idx;
      uniqueIds->SetValue(currentId, currentId);
      }

    // test the other points in this region

    int numRegionPoints = this->RegionList[regionId]->NumCells;

    int secondIdx = idx + 1;
    int nextFirstIdx = idx + numRegionPoints;

    for (int idx2 = secondIdx ; idx2 < nextFirstIdx; idx2++)
      {
      point += 3;
      currentId = this->LocatorIds[idx2];

      duplicateFound = this->SearchRegionForDuplicate(point,
                            uniqueFound[regionId], idCount[regionId], tolerance2);

      if ((tolerance > 0.0) && (duplicateFound < 0) && (regionId > 0)) 
        {
        duplicateFound =  this->SearchNeighborsForDuplicate(regionId, point, 
                                     uniqueFound, idCount, tolerance, tolerance2);
        }

      if (duplicateFound >= 0)
        {
        uniqueIds->SetValue(currentId, this->LocatorIds[duplicateFound]);
        }
      else
        {
        uniqueFound[regionId][idCount[regionId]++] = idx2;
        uniqueIds->SetValue(currentId, currentId);
        }
      }

    idx = nextFirstIdx;
    point += 3;
    nextRegionId++;
    }

  for (i=0; i<this->NumRegions; i++)
    {
    delete [] uniqueFound[i];
    }
  delete [] uniqueFound;
  delete [] idCount;

  TIMERDONE("Find duplicate points");

  return uniqueIds;
}

//----------------------------------------------------------------------------
int vtkKdTree::SearchRegionForDuplicate(float *point, int *pointsSoFar, 
                                        int len, float tolerance2)
{
  int duplicateFound = -1;
  int id;

  for (id=0; id < len; id++)
    {
    int otherId = pointsSoFar[id];
  
    float *otherPoint = this->LocatorPoints + (otherId * 3);
  
    float distance2 = vtkMath::Distance2BetweenPoints(point, otherPoint);
  
    if (distance2 <= tolerance2)
      {
      duplicateFound = otherId;
      break;
      }
    }
  return duplicateFound;
}

//----------------------------------------------------------------------------
int vtkKdTree::SearchNeighborsForDuplicate(int regionId, float *point,
                                    int **pointsSoFar, int *len, 
                                    float tolerance, float tolerance2)
{
  int duplicateFound = -1;

  float dist2 = 
    this->RegionList[regionId]->GetDistance2ToInnerBoundary(point[0],point[1],point[2]);

  if (dist2 >= tolerance2)
    {
    // There are no other regions with data that are within the
    // tolerance distance of this point.

    return duplicateFound;
    }

  // Find all regions that are within the tolerance distance of the point

  int *regionIds = new int [this->NumRegions];

  int dataBounds = this->ComputeIntersectionsUsingDataBounds;
  this->ComputeIntersectionsUsingDataBounds = 1;

#ifdef USE_SPHERE

  // Find all regions which intersect a sphere around the point
  // with radius equal to tolerance.  Compute the intersection using
  // the bounds of data within regions, not the bounds of the region.

  int nRegions = this->IntersectsSphere2(regionIds, this->NumRegions,
                                     point[0], point[1], point[2], tolerance2);
#else

  // Technically, we want all regions that intersect a sphere around the
  // point. But this takes much longer to compute than a box.  We'll compute
  // all regions that intersect a box.  We may occasionally get a region
  // we don't need, but that's OK.

  float box[6];
  box[0] = point[0] - tolerance; box[1] = point[0] + tolerance;
  box[2] = point[1] - tolerance; box[3] = point[1] + tolerance;
  box[4] = point[2] - tolerance; box[5] = point[2] + tolerance;

  int nRegions = this->IntersectsBox(regionIds, this->NumRegions, box);

#endif

  this->ComputeIntersectionsUsingDataBounds = dataBounds;

  for (int reg=0; reg < nRegions; reg++)
    {
    if ((regionIds[reg] == regionId)  || (len[reg] == 0) )
      {
      continue;
      }

    duplicateFound = this->SearchRegionForDuplicate(point,  pointsSoFar[reg], 
                                                  len[reg], tolerance2);

    if (duplicateFound) 
      {
      break;
      }
    }

  delete [] regionIds;

  return duplicateFound;
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindPoint(double x[3])
{
  float fx[3];
  fx[0] = (float)x[0]; fx[1] = (float)x[1]; fx[2] = (float)x[2];
  return this->FindPoint(fx[0], fx[1], fx[2]);
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindPoint(double x, double y, double z)
{
  float fx[3];
  fx[0] = (float)x; fx[1] = (float)y; fx[2] = (float)z;
  return this->FindPoint(fx[0], fx[1], fx[2]);
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindPoint(float x[3])
{
  return this->FindPoint(x[0], x[1], x[2]);
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindPoint(float x, float y, float z)
{
  if (!this->LocatorPoints)
    {
    vtkErrorMacro(<< "vtkKdTree::FindPoint - must build locator first");
    return -1;
    }

  int regionId = this->GetRegionContainingPoint(x, y, z);

  if (regionId == -1)
    {
    return -1;
    }

  int idx = this->LocatorRegionLocation[regionId];

  vtkIdType ptId = -1;

  float *point = this->LocatorPoints + (idx * 3);

  for (int i=0; i < this->RegionList[regionId]->NumCells; i++)
    {
    if ( (point[0] == x) && (point[1] == y) && (point[2] == z))
      {
      ptId = (vtkIdType)this->LocatorIds[idx + i];       
      break;
      } 

    point += 3;
    }

  return ptId;
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindClosestPoint(double x[3], double &dist2)
{
  float fx[3], fdist2;
  fx[0] = (float)x[0]; fx[1] = (float)x[1]; fx[2] = (float)x[2];
  fdist2 = (float)dist2;

  vtkIdType id = this->FindClosestPoint(fx[0], fx[1], fx[2], fdist2);

  dist2 = (double)fdist2;

  return id;
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindClosestPoint(double x, double y, double z, double &dist2)
{
  float fx[3], fdist2;
  fx[0] = (float)x; fx[1] = (float)y; fx[2] = (float)z;
  fdist2 = (float)dist2;

  vtkIdType id = this->FindClosestPoint(fx[0], fx[1], fx[2], fdist2);

  dist2 = (double)fdist2;

  return id;
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindClosestPoint(float x[3], float &dist2)
{
  return this->FindClosestPoint(x[0], x[1], x[2], dist2);
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindClosestPoint(float x, float y, float z, float &dist2)
{
  if (!this->LocatorPoints)
    {
    vtkErrorMacro(<< "vtkKdTree::FindClosestPoint: must build locator first");
    return -1;
    }

  float minDistance2 = 0.0;

  int closeId=-1, newCloseId=-1;  
  float newDistance2 = 4 * this->MaxWidth * this->MaxWidth;

  int regionId = this->GetRegionContainingPoint(x, y, z);

  if (regionId < 0)
    {
    // This point is not inside the space divided by the k-d tree.
    // Find the point on the boundary that is closest to it.

    float pt[3];
    this->Top->GetDistance2ToBoundary(x, y, z, pt, 1);

    double dpt[3];
    dpt[0] = pt[0];
    dpt[1] = pt[1];
    dpt[2] = pt[2];

    // GetDistance2ToBoundary will sometimes return a point *just*
    // *barely* outside the bounds of the region.  Move that point to
    // just barely *inside* instead.

    if (dpt[0] <= this->Top->Min[0]) 
      {
      dpt[0] = this->Top->Min[0] + this->FudgeFactor;
      }
    if (dpt[1] <= this->Top->Min[1]) 
      {
      dpt[1] = this->Top->Min[1] + this->FudgeFactor;
      }
    if (dpt[2] <= this->Top->Min[2]) 
      {
      dpt[2] = this->Top->Min[2] + this->FudgeFactor;
      }
    if (dpt[0] >= this->Top->Max[0]) 
      {
      dpt[0] = this->Top->Max[0] - this->FudgeFactor;
      }
    if (dpt[1] >= this->Top->Max[1])
      {
      dpt[1] = this->Top->Max[1] - this->FudgeFactor;
      }
    if (dpt[2] >= this->Top->Max[2]) 
      {
      dpt[2] = this->Top->Max[2] - this->FudgeFactor;
      }

    regionId = this->GetRegionContainingPoint(dpt[0], dpt[1], dpt[2]);

    float proxyDistance; 

    // XXX atwilso changed this to compute from x, y, z instead of pt
    // -- computing from pt still wasn't giving the right results for
    // far-away points

    closeId = this->_FindClosestPointInRegion(regionId,
                                              x, y, z,
                                              proxyDistance);

    float originalPoint[3], *closePoint;
    originalPoint[0] = x; originalPoint[1] = y; originalPoint[2] = z;
    closePoint = this->LocatorPoints + (closeId * 3);

    minDistance2 = 
        vtkMath::Distance2BetweenPoints(originalPoint, closePoint);

    // Check to see if neighboring regions have a closer point

    newCloseId = 
      this->FindClosestPointInSphere(x, y, z,
                                     minDistance2,    // radius
                                     regionId,        // skip this region
                                     newDistance2);   // distance to closest point
    
    }
  else     // Point is inside a k-d tree region
    {
    closeId = 
      this->_FindClosestPointInRegion(regionId, x, y, z, minDistance2);
  
    if (minDistance2 > 0.0)
      {
      float dist2ToBoundary =
        this->RegionList[regionId]->GetDistance2ToInnerBoundary(x, y, z);
  
      if (dist2ToBoundary < minDistance2)
        {
        // The closest point may be in a neighboring region

        newCloseId = this->FindClosestPointInSphere(x, y, z,
                                     minDistance2,   // radius
                                     regionId,       // skip this region
                                     newDistance2);
        }
      }
    }

  if (newDistance2 < minDistance2 && newCloseId != -1)
    {
    closeId = newCloseId;
    minDistance2 = newDistance2;
    }

  vtkIdType closePointId = (vtkIdType)this->LocatorIds[closeId];

  dist2 = minDistance2;

  return closePointId;
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindClosestPointInRegion(int regionId, float *x, float &dist2)
{
  return this->FindClosestPointInRegion(regionId, x[0], x[1], x[2], dist2);
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::FindClosestPointInRegion(int regionId, 
                                      float x, float y, float z, float &dist2)
{
  int localId = this->_FindClosestPointInRegion(regionId, x, y, z, dist2);

  vtkIdType originalId = -1;

  if (localId >= 0)
    {
    originalId = (vtkIdType)this->LocatorIds[localId];
    }

  return originalId;
}

//----------------------------------------------------------------------------
int vtkKdTree::_FindClosestPointInRegion(int regionId, 
                                     float x, float y, float z, float &dist2)
{
  int minId=0;

  float minDistance2 = 4 * this->MaxWidth * this->MaxWidth;

  int idx = this->LocatorRegionLocation[regionId];

  float *candidate = this->LocatorPoints + (idx * 3);

  for (int i=0; i < this->RegionList[regionId]->NumCells; i++)
    {
    float dx = (x - candidate[0]) * (x - candidate[0]);

    if (dx < minDistance2)
      {
      float dxy = dx + ((y - candidate[1]) * (y - candidate[1]));

      if (dxy < minDistance2)
        {
        float dxyz = dxy + ((z - candidate[2]) * (z - candidate[2]));

        if (dxyz < minDistance2)
          {
          minId = idx + i;
          minDistance2 = dxyz;

          if (dxyz == 0.0) 
            {
            break;
            }
          }
        }
      }

    candidate += 3;
    }

  dist2 = minDistance2;

  return minId;
}
int vtkKdTree::FindClosestPointInSphere(float x, float y, float z, 
                                        float radius, int skipRegion,
                                        float &dist2)
{
  int *regionIds = new int [this->NumRegions];

  int dataBounds = this->ComputeIntersectionsUsingDataBounds;
  this->ComputeIntersectionsUsingDataBounds = 1;

  int nRegions = 
    this->IntersectsSphere2(regionIds, this->NumRegions, x, y, z, radius);

  this->ComputeIntersectionsUsingDataBounds = dataBounds;

  float minDistance2 = 4 * this->MaxWidth * this->MaxWidth;
  int closeId = -1;

  for (int reg=0; reg < nRegions; reg++)
    {
    if (regionIds[reg] == skipRegion) 
      {
      continue;
      }

    int neighbor = regionIds[reg];
    float newDistance2;

    int newCloseId = this->_FindClosestPointInRegion(neighbor,
                                x, y, z, newDistance2);

    if (newDistance2 < minDistance2)
      {
      minDistance2 = newDistance2;
      closeId = newCloseId;
      }
    }
  
  delete [] regionIds;

  dist2 = minDistance2;
  return closeId;
}

//----------------------------------------------------------------------------
vtkIdTypeArray *vtkKdTree::GetPointsInRegion(int regionId)
{
  if ( (regionId < 0) || (regionId >= this->NumRegions))
    {
    vtkErrorMacro(<< "vtkKdTree::GetPointsInRegion invalid region ID");
    return NULL;
    }

  if (!this->LocatorIds)
    {
    vtkErrorMacro(<< "vtkKdTree::GetPointsInRegion build locator first");
    return NULL;
    }

  int numPoints = this->RegionList[regionId]->NumCells;
  int where = this->LocatorRegionLocation[regionId];

  vtkIdTypeArray *ptIds = vtkIdTypeArray::New();
  ptIds->SetNumberOfValues(numPoints);

  int *ids = this->LocatorIds + where;

  for (int i=0; i<numPoints; i++)
    {
    ptIds->SetValue(i, ids[i]);
    }

  return ptIds;
}

//----------------------------------------------------------------------------
// Code to save state/time of last k-d tree build, and to
// determine if a data set's geometry has changed since the
// last build. 
//
void vtkKdTree::ClearLastBuildCache()
{
  if (this->LastDataCacheSize > 0)
    {
    delete [] this->LastInputDataSets;
    delete [] this->LastDataSetType;
    delete [] this->LastInputDataInfo;
    delete [] this->LastBounds;
    delete [] this->LastNumCells;
    delete [] this->LastNumPoints;
    this->LastDataCacheSize = 0;
    }
  this->LastNumDataSets = 0;
  this->LastInputDataSets = NULL;
  this->LastDataSetType = NULL;
  this->LastInputDataInfo = NULL;
  this->LastBounds        = NULL;
  this->LastNumPoints     = NULL;
  this->LastNumCells      = NULL;
}

//----------------------------------------------------------------------------
void vtkKdTree::UpdateBuildTime()
{
  this->BuildTime.Modified();

  // Save enough information so that next time we execute,
  // we can determine whether input geometry has changed.

  if (this->NumDataSets > this->LastDataCacheSize)
    {
    this->ClearLastBuildCache();

    this->LastInputDataSets = new vtkDataSet * [this->NumDataSets];
    this->LastDataSetType = new int [this->NumDataSets];
    this->LastInputDataInfo = new double [9 * this->NumDataSets];
    this->LastBounds = new double [6 * this->NumDataSets];
    this->LastNumPoints = new int [this->NumDataSets];
    this->LastNumCells = new int [this->NumDataSets];
    this->LastDataCacheSize = this->NumDataSets;
    }

  this->LastNumDataSets = this->NumDataSets;

  int nextds = 0;

  for (int i=0; i<this->NumDataSetsAllocated; i++)
    {
    vtkDataSet *in = this->DataSets[i];

    if (in == NULL) 
      {
      continue;
      }

    if (nextds >= this->NumDataSets)
      {
      vtkErrorMacro(<< "vtkKdTree::UpdateBuildTime corrupt counts");
      return;
      }

    this->LastInputDataSets[nextds] = in;

    this->LastNumPoints[nextds] = static_cast<int>(in->GetNumberOfPoints());
    this->LastNumCells[nextds] = static_cast<int>(in->GetNumberOfCells());

    in->GetBounds(this->LastBounds + 6*nextds);
  
    int type = this->LastDataSetType[nextds] = in->GetDataObjectType();

    if ((type == VTK_IMAGE_DATA) || (type == VTK_UNIFORM_GRID))
      {
      double origin[3], spacing[3];
      int dims[3];
  
      if (type == VTK_IMAGE_DATA)
        {
        vtkImageData *id = vtkImageData::SafeDownCast(in);
        id->GetDimensions(dims);
        id->GetOrigin(origin);
        id->GetSpacing(spacing);
        }
      else
        {
        vtkUniformGrid *ug = vtkUniformGrid::SafeDownCast(in);
        ug->GetDimensions(dims);
        ug->GetOrigin(origin);
        ug->GetSpacing(spacing);
        }
  
      this->SetInputDataInfo(nextds, dims, origin, spacing);
      }

    nextds++;
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::SetInputDataInfo(int i, int dims[3], double origin[3], 
                                 double spacing[3])
{
  int idx = 9*i;
  this->LastInputDataInfo[idx++] = (double)dims[0];
  this->LastInputDataInfo[idx++] = (double)dims[1];
  this->LastInputDataInfo[idx++] = (double)dims[2];
  this->LastInputDataInfo[idx++] = origin[0];
  this->LastInputDataInfo[idx++] = origin[1];
  this->LastInputDataInfo[idx++] = origin[2];
  this->LastInputDataInfo[idx++] = spacing[0];
  this->LastInputDataInfo[idx++] = spacing[1];
  this->LastInputDataInfo[idx++] = spacing[2];
}

//----------------------------------------------------------------------------
int vtkKdTree::CheckInputDataInfo(int i, int dims[3], double origin[3], 
                                  double spacing[3])
{
  int sameValues = 1;
  int idx = 9*i; 

  if ((dims[0] != (int)this->LastInputDataInfo[idx]) ||
      (dims[1] != (int)this->LastInputDataInfo[idx+1]) ||
      (dims[2] != (int)this->LastInputDataInfo[idx+2]) ||
      (origin[0] != this->LastInputDataInfo[idx+3]) ||
      (origin[1] != this->LastInputDataInfo[idx+4]) ||
      (origin[2] != this->LastInputDataInfo[idx+5]) ||
      (spacing[0] != this->LastInputDataInfo[idx+6]) ||
      (spacing[1] != this->LastInputDataInfo[idx+7]) ||
      (spacing[2] != this->LastInputDataInfo[idx+8]) ) 
    {
    sameValues = 0;
    }

  return sameValues;
}

//----------------------------------------------------------------------------
int vtkKdTree::NewGeometry()
{
  if (this->NumDataSets != this->LastNumDataSets)
    {
    return 1;
    }

  vtkDataSet **tmp = new vtkDataSet * [this->NumDataSets];
  int nextds = 0;
  for (int i=0; i<this->NumDataSetsAllocated; i++)
    {
    if (this->DataSets[i] == NULL) 
      {
      continue;
      }
    if (nextds >= this->NumDataSets)
      {
      vtkErrorMacro(<<"vtkKdTree::NewGeometry corrupt counts");
      return -1;
      }

    tmp[nextds++] = this->DataSets[i];
    }

  int itsNew = this->NewGeometry(tmp, this->NumDataSets);

  delete [] tmp;

  return itsNew;
}
int vtkKdTree::NewGeometry(vtkDataSet **sets, int numSets)
{
  int newGeometry = 0;
#if 0
  vtkPointSet *ps = NULL;
#endif
  vtkRectilinearGrid *rg = NULL;
  vtkImageData *id = NULL;
  vtkUniformGrid *ug = NULL;
  int same=0;
  int dims[3];
  double origin[3], spacing[3];

  for (int i=0; i<numSets; i++)
    {
    vtkDataSet *in = this->LastInputDataSets[i];
    int type = in->GetDataObjectType();
  
    if (type != this->LastDataSetType[i])
      {
      newGeometry = 1;
      break;
      }
  
    switch (type)
      {
      case VTK_POLY_DATA:
      case VTK_UNSTRUCTURED_GRID:
      case VTK_STRUCTURED_GRID:
  #if 0
        // For now, vtkPExodusReader creates a whole new
        // vtkUnstructuredGrid, even when just changing
        // field arrays.  So we'll just check bounds.
        ps = vtkPointSet::SafeDownCast(in);
        if (ps->GetPoints()->GetMTime() > this->BuildTime)
          {
          newGeometry = 1;
          }
  #else
        if ((sets[i]->GetNumberOfPoints() != this->LastNumPoints[i]) || 
            (sets[i]->GetNumberOfCells() != this->LastNumCells[i]) )
          {
          newGeometry = 1;
          }
        else
          {
          double b[6];
          sets[i]->GetBounds(b);
          double *lastb = this->LastBounds + 6*i;
  
          for (int dim=0; dim<6; dim++)
            {
            if (*lastb++ != b[dim])
              {
              newGeometry = 1;
              break;
              }
            }
          }
  #endif
        break;
      
      case VTK_RECTILINEAR_GRID:
  
        rg = vtkRectilinearGrid::SafeDownCast(in);
        if ((rg->GetXCoordinates()->GetMTime() > this->BuildTime) ||
            (rg->GetYCoordinates()->GetMTime() > this->BuildTime) ||
            (rg->GetZCoordinates()->GetMTime() > this->BuildTime) )
          {
          newGeometry = 1;
          }
        break;
  
      case VTK_IMAGE_DATA:
      case VTK_STRUCTURED_POINTS:
      
        id = vtkImageData::SafeDownCast(in);
  
        id->GetDimensions(dims);
        id->GetOrigin(origin);
        id->GetSpacing(spacing);
  
        same = this->CheckInputDataInfo(i, dims, origin, spacing);
  
        if (!same)
          {
          newGeometry = 1;
          }
  
        break;
  
      case VTK_UNIFORM_GRID:
  
        ug = vtkUniformGrid::SafeDownCast(in);
  
        ug->GetDimensions(dims);
        ug->GetOrigin(origin);
        ug->GetSpacing(spacing);
  
        same = this->CheckInputDataInfo(i, dims, origin, spacing);
        
        if (!same)
          {
          newGeometry = 1;
          }
        else if (ug->GetPointVisibilityArray()->GetMTime() > 
                 this->BuildTime)
          {
          newGeometry = 1;
          }
        else if (ug->GetCellVisibilityArray()->GetMTime() > 
                 this->BuildTime)
          {
          newGeometry = 1;
          }
        break;
  
      default:
        vtkWarningMacro(<< 
          "vtkKdTree::NewGeometry: unanticipated type");
  
        newGeometry = 1;
      }
    if (newGeometry) 
      {
      break;
      }
    }

  return newGeometry;
}

//----------------------------------------------------------------------------
void vtkKdTree::__printTree(vtkKdNode *kd, int depth, int v)
{
  if (v) 
    { 
    kd->PrintVerboseNode(depth);
    }
  else   
    { 
    kd->PrintNode(depth);
    }

  if (kd->Left) 
    { 
    vtkKdTree::__printTree(kd->Left, depth+1, v);
    }
  if (kd->Right) 
    { 
    vtkKdTree::__printTree(kd->Right, depth+1, v);
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::_printTree(int v)
{
  vtkKdTree::__printTree(this->Top, 0, v);
}

//----------------------------------------------------------------------------
void vtkKdTree::PrintRegion(int id)
{
  this->RegionList[id]->PrintNode(0);
}

//----------------------------------------------------------------------------
void vtkKdTree::PrintTree()
{   
  _printTree(0);
}  

//----------------------------------------------------------------------------
void vtkKdTree::PrintVerboseTree()
{      
  _printTree(1);
}          

//----------------------------------------------------------------------------
void vtkKdTree::FreeSearchStructure()
{
  if (this->Top) 
    {
    vtkKdTree::DeleteNodes(this->Top);
    delete this->Top;
    this->Top = NULL;
    }
  if (this->RegionList)
    {
    delete [] this->RegionList;
    this->RegionList = NULL;
    } 

  this->NumRegions = 0;
  this->SetActualLevel();

  this->DeleteCellLists();

  if (this->CellRegionList)
    {
    delete [] this->CellRegionList;
    this->CellRegionList = NULL;
    }

  if (this->LocatorPoints)
    {
    delete [] this->LocatorPoints;
    this->LocatorPoints = NULL;
    }

  if (this->LocatorIds)
    {
    delete [] this->LocatorIds;
    this->LocatorIds = NULL;
    }

  if (this->LocatorRegionLocation)
    {
    delete [] this->LocatorRegionLocation;
    this->LocatorRegionLocation = NULL;
    }
}

//----------------------------------------------------------------------------
// build PolyData representation of all spacial regions------------
//
void vtkKdTree::GenerateRepresentation(int level, vtkPolyData *pd)
{
  if (this->GenerateRepresentationUsingDataBounds)
    {
    this->GenerateRepresentationDataBounds(level, pd);
    }
  else
    {
    this->GenerateRepresentationWholeSpace(level, pd);
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::GenerateRepresentationWholeSpace(int level, vtkPolyData *pd)
{
  int i;

  vtkPoints *pts;
  vtkCellArray *polys;

  if ( this->Top == NULL )
    {
    vtkErrorMacro(<<"vtkKdTree::GenerateRepresentation empty tree");
    return;
    }

  if ((level < 0) || (level > this->Level)) 
    {
    level = this->Level;
    }

  int npoints = 0;
  int npolys  = 0;

  for (i=0 ; i < level; i++)
    {
    int levelPolys = 1 << (i-1);
    npoints += (4 * levelPolys);
    npolys += levelPolys;
    }

  pts = vtkPoints::New();
  pts->Allocate(npoints); 
  polys = vtkCellArray::New();
  polys->Allocate(npolys);

  // level 0 bounding box

  vtkIdType ids[8];
  vtkIdType idList[4];
  double     x[3];
  vtkKdNode    *kd    = this->Top;

  double *min = kd->Min;
  double *max = kd->Max;

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[0] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[1] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[2] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[3] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[4] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[5] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[6] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[7] = pts->InsertNextPoint(x);

  idList[0] = ids[0]; idList[1] = ids[1]; idList[2] = ids[2]; idList[3] = ids[3];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[1]; idList[1] = ids[5]; idList[2] = ids[6]; idList[3] = ids[2];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[5]; idList[1] = ids[4]; idList[2] = ids[7]; idList[3] = ids[6];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[4]; idList[1] = ids[0]; idList[2] = ids[3]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[3]; idList[1] = ids[2]; idList[2] = ids[6]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[1]; idList[1] = ids[0]; idList[2] = ids[4]; idList[3] = ids[5];
  polys->InsertNextCell(4, idList);

  if (kd->Left && (level > 0))
    {
      _generateRepresentationWholeSpace(kd, pts, polys, level-1);
    }

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}

//----------------------------------------------------------------------------
void vtkKdTree::_generateRepresentationWholeSpace(vtkKdNode *kd, 
                                        vtkPoints *pts, 
                                        vtkCellArray *polys, 
                                        int level)
{
  int i;
  double p[4][3];
  vtkIdType ids[4];

  if ((level < 0) || (kd->Left == NULL)) 
    {
    return;
    }

  double *min = kd->Min;
  double *max = kd->Max;  
  double *leftmax = kd->Left->Max;

  // splitting plane

  switch (kd->Dim)
    {
    case XDIM:

      p[0][0] = leftmax[0]; p[0][1] = max[1]; p[0][2] = max[2];
      p[1][0] = leftmax[0]; p[1][1] = max[1]; p[1][2] = min[2];
      p[2][0] = leftmax[0]; p[2][1] = min[1]; p[2][2] = min[2];
      p[3][0] = leftmax[0]; p[3][1] = min[1]; p[3][2] = max[2];

      break;

    case YDIM:

      p[0][0] = min[0]; p[0][1] = leftmax[1]; p[0][2] = max[2];
      p[1][0] = min[0]; p[1][1] = leftmax[1]; p[1][2] = min[2];
      p[2][0] = max[0]; p[2][1] = leftmax[1]; p[2][2] = min[2];
      p[3][0] = max[0]; p[3][1] = leftmax[1]; p[3][2] = max[2];

      break;

    case ZDIM:

      p[0][0] = min[0]; p[0][1] = min[1]; p[0][2] = leftmax[2];
      p[1][0] = min[0]; p[1][1] = max[1]; p[1][2] = leftmax[2];
      p[2][0] = max[0]; p[2][1] = max[1]; p[2][2] = leftmax[2];
      p[3][0] = max[0]; p[3][1] = min[1]; p[3][2] = leftmax[2];

      break;
    }


  for (i=0; i<4; i++) ids[i] = pts->InsertNextPoint(p[i]);

  polys->InsertNextCell(4, ids);

  _generateRepresentationWholeSpace(kd->Left, pts, polys, level-1);
  _generateRepresentationWholeSpace(kd->Right, pts, polys, level-1);
}

//----------------------------------------------------------------------------
void vtkKdTree::GenerateRepresentationDataBounds(int level, vtkPolyData *pd)
{
  int i;
  vtkPoints *pts;
  vtkCellArray *polys;

  if ( this->Top == NULL )
    {
    vtkErrorMacro(<<"vtkKdTree::GenerateRepresentation no tree");
    return;
    }

  if ((level < 0) || (level > this->Level)) 
    {
    level = this->Level;
    }

  int npoints = 0;
  int npolys  = 0;

  for (i=0; i < level; i++)
    {
    int levelBoxes= 1 << i;
    npoints += (8 * levelBoxes);
    npolys += (6 * levelBoxes);
    }

  pts = vtkPoints::New();
  pts->Allocate(npoints); 
  polys = vtkCellArray::New();
  polys->Allocate(npolys);

  _generateRepresentationDataBounds(this->Top, pts, polys, level);

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}

//----------------------------------------------------------------------------
void vtkKdTree::_generateRepresentationDataBounds(vtkKdNode *kd, vtkPoints *pts,
                     vtkCellArray *polys, int level)
{
  if (level > 0)
    {
      if (kd->Left)
        {
        _generateRepresentationDataBounds(kd->Left, pts, polys, level-1);
        _generateRepresentationDataBounds(kd->Right, pts, polys, level-1);
        }

      return;
    }
  vtkKdTree::AddPolys(kd, pts, polys);
}

//----------------------------------------------------------------------------
// PolyData rep. of all spacial regions, shrunk to data bounds-------
//
void vtkKdTree::AddPolys(vtkKdNode *kd, vtkPoints *pts, vtkCellArray *polys)
{
  vtkIdType ids[8];
  vtkIdType idList[4];
  double     x[3];

  double *min;
  double *max;

  if (this->GenerateRepresentationUsingDataBounds)
    {
    min = kd->MinVal;
    max = kd->MaxVal;
    }
  else
    {
    min = kd->Min;
    max = kd->Max;
    }

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[0] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[1] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[2] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[3] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[4] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[5] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[6] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[7] = pts->InsertNextPoint(x);

  idList[0] = ids[0]; idList[1] = ids[1]; idList[2] = ids[2]; idList[3] = ids[3];
  polys->InsertNextCell(4, idList);
  
  idList[0] = ids[1]; idList[1] = ids[5]; idList[2] = ids[6]; idList[3] = ids[2];
  polys->InsertNextCell(4, idList);
  
  idList[0] = ids[5]; idList[1] = ids[4]; idList[2] = ids[7]; idList[3] = ids[6];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[4]; idList[1] = ids[0]; idList[2] = ids[3]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);
  
  idList[0] = ids[3]; idList[1] = ids[2]; idList[2] = ids[6]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[1]; idList[1] = ids[0]; idList[2] = ids[4]; idList[3] = ids[5];
  polys->InsertNextCell(4, idList);
}

//----------------------------------------------------------------------------
// PolyData representation of a list of spacial regions------------
//
void vtkKdTree::GenerateRepresentation(int *regions, int len, vtkPolyData *pd)
{
  int i;
  vtkPoints *pts;
  vtkCellArray *polys;

  if ( this->Top == NULL )
    {
    vtkErrorMacro(<<"vtkKdTree::GenerateRepresentation no tree");
    return;
    }

  int npoints = 8 * len;
  int npolys  = 6 * len;

  pts = vtkPoints::New();
  pts->Allocate(npoints); 
  polys = vtkCellArray::New();
  polys->Allocate(npolys);

  for (i=0; i<len; i++)
    {
    if ((regions[i] < 0) || (regions[i] >= this->NumRegions)) 
      {
      break;
      }

    vtkKdTree::AddPolys(this->RegionList[regions[i]], pts, polys);
    }

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}

//----------------------------------------------------------------------------
//  Cell ID lists
//
void vtkKdTree::SetCellBounds(vtkCell *cell, double *bounds)
{
  vtkPoints *pts = cell->GetPoints();
  pts->Modified();         // VTK bug - so bounds will be re-calculated
  pts->GetBounds(bounds);
}

#define SORTLIST(l, lsize) vtkstd::sort(l, l + lsize)

#define REMOVEDUPLICATES(l, lsize, newsize) \
{                                     \
int ii,jj;                            \
for (ii=0, jj=0; ii<lsize; ii++)      \
  {                                   \
  if ((ii > 0) && (l[ii] == l[jj-1])) \
    {         \
    continue; \
    }         \
  if (jj != ii)    \
    {              \
    l[jj] = l[ii]; \
    }              \
  jj++;            \
}                  \
newsize = jj;      \
}

//----------------------------------------------------------------------------
int vtkKdTree::FindInSortedList(int *list, int size, int val)
{
  int loc = -1;

  if (size < 8)
    {
    for (int i=0; i<size; i++)
      {
      if (list[i] == val)
        { 
        loc = i;
        break;
        }
      }

      return loc;
    }

  int L, R, M;
  L=0;
  R=size-1;

  while (R > L)  
    {
    if (R == L+1)
      {
      if (list[R] == val)
        {
        loc = R;
        }
      else if (list[L] == val)  
        {
        loc = L;
        }
      break;
      }

    M = (R + L) / 2;

    if (list[M] > val)  
      {
      R = M;
      continue;
      }
    else if (list[M] < val)
      {
      L = M;
      continue;
      }
    else
      {
      loc = M;
      break;
      }
    }
  return loc;
}

//----------------------------------------------------------------------------
int vtkKdTree::FoundId(vtkIntArray *ar, int val)
{
  int size = ar->GetNumberOfTuples();
  int *ptr = ar->GetPointer(0);

  int where = vtkKdTree::FindInSortedList(ptr, size, val);

  return (where > -1);
}

//----------------------------------------------------------------------------
int vtkKdTree::findRegion(vtkKdNode *node, float x, float y, float z)
{
  return vtkKdTree::findRegion(node, (double)x, (double)y, (double)z);
}

//----------------------------------------------------------------------------
int vtkKdTree::findRegion(vtkKdNode *node, double x, double y, double z)
{
  int regionId;

  if (!node->ContainsPoint(x, y, z, 0))
    {
    return -1;
    }

  if (node->Left == NULL)
    {
    regionId = node->Id;
    }
  else
    {
    regionId = vtkKdTree::findRegion(node->Left, x, y, z);

    if (regionId < 0)
      {
      regionId = vtkKdTree::findRegion(node->Right, x, y, z);
      }
    }

  return regionId;
}

//----------------------------------------------------------------------------
void vtkKdTree::CreateCellLists()
{
  this->CreateCellLists(this->DataSets[0], (int *)NULL, 0);
  return;
}

//----------------------------------------------------------------------------
void vtkKdTree::CreateCellLists(int *regionList, int listSize)
{
  this->CreateCellLists(this->DataSets[0], regionList, listSize);
  return;
}

//----------------------------------------------------------------------------
void vtkKdTree::CreateCellLists(int dataSet, int *regionList, int listSize)
{
  if ((dataSet < 0) || (dataSet >= NumDataSets))
    {
    vtkErrorMacro(<<"vtkKdTree::CreateCellLists invalid data set");
    return;
    }

  this->CreateCellLists(this->DataSets[dataSet], regionList, listSize);
  return;
}

//----------------------------------------------------------------------------
void vtkKdTree::CreateCellLists(vtkDataSet *set, int *regionList, int listSize)
{
  int i, AllRegions;

  if ( this->GetDataSet(set) < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::CreateCellLists invalid data set");
    return;
    }

  vtkKdTree::_cellList *list = &this->CellList;

  if (list->nRegions > 0)
    {
    this->DeleteCellLists();
    }

  list->emptyList = vtkIdList::New();

  list->dataSet = set;

  if ((regionList == NULL) || (listSize == 0)) 
    {
    list->nRegions = this->NumRegions;    // all regions
    }
  else 
    {
    list->regionIds = new int [listSize];
  
    if (!list->regionIds)
      {
      vtkErrorMacro(<<"vtkKdTree::CreateCellLists memory allocation");
      return;
      }

    memcpy(list->regionIds, regionList, sizeof(int) * listSize);
    SORTLIST(list->regionIds, listSize);
    REMOVEDUPLICATES(list->regionIds, listSize, list->nRegions);
  
    if (list->nRegions == this->NumRegions)
      {
      delete [] list->regionIds;
      list->regionIds = NULL;
      }
    }

  if (list->nRegions == this->NumRegions)
    {
    AllRegions = 1;
    }
  else
    {
    AllRegions = 0; 
    } 
    
  int *idlist = NULL;
  int idListLen = 0;
  
  if (this->IncludeRegionBoundaryCells)
    {
    list->boundaryCells = new vtkIdList * [list->nRegions];

    if (!list->boundaryCells)
      {
      vtkErrorMacro(<<"vtkKdTree::CreateCellLists memory allocation");
      return;
      }
    
    for (i=0; i<list->nRegions; i++)
      {
      list->boundaryCells[i] = vtkIdList::New();
      }
    idListLen = this->NumRegions;
    
    idlist = new int [idListLen];
    }
  
  int *listptr = NULL;
    
  if (!AllRegions)
    {
    listptr = new int [this->NumRegions];

    if (!listptr)
      {
      vtkErrorMacro(<<"vtkKdTree::CreateCellLists memory allocation");
      return;
      }

    for (i=0; i<this->NumRegions; i++)
      {
      listptr[i] = -1;
      }
    }

  list->cells = new vtkIdList * [list->nRegions];

  if (!list->cells)
    {
    vtkErrorMacro(<<"vtkKdTree::CreateCellLists memory allocation");
    return;
    }

  for (i = 0; i < list->nRegions; i++)
    {
    list->cells[i] = vtkIdList::New();

    if (listptr) 
      {
      listptr[list->regionIds[i]] = i;
      }
    }

  // acquire a list in cell Id order of the region Id each
  // cell centroid falls in

  int *regList = this->CellRegionList;

  if (regList == NULL)
    {
    regList = this->AllGetRegionContainingCell();
    }

  int setNum = this->GetDataSet(set);

  if (setNum > 0)
    {
    int ncells = this->GetDataSetsNumberOfCells(0,setNum-1);
    regList += ncells;
    }

  int intersectionOption = this->ComputeIntersectionsUsingDataBounds;
  this->ComputeIntersectionsUsingDataBounds = 0;

  int nCells = set->GetNumberOfCells();

  for (int cellId=0; cellId<nCells; cellId++)
    {
    if (this->IncludeRegionBoundaryCells)
      {
      // Find all regions the cell intersects, including
      // the region the cell centroid lies in.
      // This can be an expensive calculation, intersections
      // of a convex region with axis aligned boxes.

      int nRegions = this->IntersectsCell(idlist, idListLen, cellId,
                                          regList[cellId]);
      
      if (nRegions == 1)
        {
        int idx = (listptr) ? listptr[idlist[0]] : idlist[0];
      
        if (idx >= 0) 
          {
          list->cells[idx]->InsertNextId(cellId);
          }
        }
      else
        {
        for (int r=0; r < nRegions; r++)
          {
          int regionId = idlist[r];
    
          int idx = (listptr) ? listptr[regionId] : regionId;

          if (idx < 0) 
            {
            continue;
            }

          if (regionId == regList[cellId])
            {
            list->cells[idx]->InsertNextId(cellId);
            }
          else
            {
            list->boundaryCells[idx]->InsertNextId(cellId);
            }         
          }
        }
      }
    else 
      {
      // just find the region the cell centroid lies in - easy

      int regionId = regList[cellId];
    
      int idx = (listptr) ? listptr[regionId] : regionId;
  
      if (idx >= 0) 
        {
        list->cells[idx]->InsertNextId(cellId);
        }
      } 
    }

  this->ComputeIntersectionsUsingDataBounds = intersectionOption;

  if (listptr)
    {
    delete [] listptr;
    } 
  if (idlist)
    {
    delete [] idlist;
    }   
}     

//----------------------------------------------------------------------------
vtkIdList * vtkKdTree::GetList(int regionId, vtkIdList **which)
{
  int i;
  struct _cellList *list = &this->CellList;
  vtkIdList *cellIds = NULL;

  if (which && (list->nRegions == this->NumRegions))
    {
    cellIds = which[regionId];
    }
  else if (which)
    {
    for (i=0; i< list->nRegions; i++)
      {
      if (list->regionIds[i] == regionId)
        {
        cellIds = which[i];
        break;
        }
      }
    }
  else
    {
    cellIds = list->emptyList; 
    }

  return cellIds;
}

//----------------------------------------------------------------------------
vtkIdList * vtkKdTree::GetCellList(int regionID)
{
  return this->GetList(regionID, this->CellList.cells);
}

//----------------------------------------------------------------------------
vtkIdList * vtkKdTree::GetBoundaryCellList(int regionID)
{
  return this->GetList(regionID, this->CellList.boundaryCells);
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::GetCellLists(vtkIntArray *regions,
          int set, vtkIdList *inRegionCells, vtkIdList *onBoundaryCells)
{
  if ( (set < 0) || 
       (set >= this->NumDataSetsAllocated) ||
       (this->DataSets[set] == NULL))
    {
    vtkErrorMacro(<<"vtkKdTree::GetCellLists no such data set");
    return 0;
    }
  return this->GetCellLists(regions, this->DataSets[set], 
                            inRegionCells, onBoundaryCells);
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::GetCellLists(vtkIntArray *regions,
          vtkIdList *inRegionCells, vtkIdList *onBoundaryCells)
{
  return this->GetCellLists(regions, this->DataSets[0], 
                           inRegionCells, onBoundaryCells);
}

//----------------------------------------------------------------------------
vtkIdType vtkKdTree::GetCellLists(vtkIntArray *regions, vtkDataSet *set,
          vtkIdList *inRegionCells, vtkIdList *onBoundaryCells)
{
  int reg, regionId;
  vtkIdType cell, cellId, numCells;
  vtkIdList *cellIds;

  vtkIdType totalCells = 0;

  if ( (inRegionCells == NULL) && (onBoundaryCells == NULL))
    {
    return totalCells;
    }

  int nregions = regions->GetNumberOfTuples();

  if (nregions == 0)
    {
    return totalCells;
    }

  // Do I have cell lists for all these regions?  If not, build cell
  // lists for all regions for this data set.

  int rebuild = 0;

  if (this->CellList.dataSet != set)
    {
    rebuild = 1;
    }
  else if (nregions > this->CellList.nRegions)
    {
    rebuild = 1;
    }
  else if ((onBoundaryCells != NULL) && (this->CellList.boundaryCells == NULL))
    {
    rebuild = 1;
    }
  else if (this->CellList.nRegions < this->NumRegions)
    {
    // these two lists should generally be "short"

    int *haveIds = this->CellList.regionIds;

    for (int wantReg=0; wantReg < nregions; wantReg++)
      {
      int wantRegion = regions->GetValue(wantReg);
      int gotId = 0;

      for (int haveReg=0; haveReg < this->CellList.nRegions; haveReg++)
        {
        if (haveIds[haveReg] == wantRegion)
          {
          gotId = 1;
          break;
          }
        }
      if (!gotId)
        {
        rebuild = 1;
        break;
        }
      }
    }

  if (rebuild)
    {
    if (onBoundaryCells != NULL)
      {
      this->IncludeRegionBoundaryCellsOn();
      }
    this->CreateCellLists(set, NULL, 0);  // for all regions
    }

  // OK, we have cell lists for these regions.  Make lists of region
  // cells and boundary cells.

  int CheckSet = (onBoundaryCells && (nregions > 1));

  vtkstd::set<vtkIdType> ids;
  vtkstd::pair<vtkstd::set<vtkIdType>::iterator, bool> idRec;

  int totalRegionCells = 0;
  int totalBoundaryCells = 0;

  vtkIdList **inRegionList = new vtkIdList * [nregions];

  // First the cell IDs with centroid in the regions

  for (reg = 0; reg < nregions; reg++)
    {
    regionId = regions->GetValue(reg);

    inRegionList[reg] = this->GetCellList(regionId);

    totalRegionCells += inRegionList[reg]->GetNumberOfIds();
    }

  if (inRegionCells)
    {
    inRegionCells->Initialize();
    inRegionCells->SetNumberOfIds(totalRegionCells);
    }
        
  int nextCell = 0;
          
  for (reg = 0; reg <  nregions; reg++)
    {     
    cellIds = inRegionList[reg];
      
    numCells = cellIds->GetNumberOfIds();
        
    for (cell = 0; cell < numCells; cell++)
      { 
      if (inRegionCells)
        {
        inRegionCells->SetId(nextCell++, cellIds->GetId(cell));
        }
      
      if (CheckSet)
        {
        // We have to save the ids, so we don't include
        // them as boundary cells.  A cell in one region
        // may be a boundary cell of another region.
      
        ids.insert(cellIds->GetId(cell));
        }
      }
    }
  
  delete [] inRegionList;
  
  if (onBoundaryCells == NULL)
    {
    return totalRegionCells;
    }
  
  // Now the list of all cells on the boundary of the regions,
  // which do not have their centroid in one of the regions

  onBoundaryCells->Initialize();
    
  for (reg = 0; reg < nregions; reg++)
    {
    regionId = regions->GetValue(reg);

    cellIds = this->GetBoundaryCellList(regionId);
    
    numCells = cellIds->GetNumberOfIds();
  
    for (cell = 0; cell < numCells; cell++)
      {
      cellId = cellIds->GetId(cell);
    
      if (CheckSet)
        {
        // if we already included this cell because it is within 
        // one of the regions, or on the boundary of another, skip it

        idRec = ids.insert(cellId);

        if (idRec.second == 0) 
          {
          continue;
          }
        }

      onBoundaryCells->InsertNextId(cellId);
      totalBoundaryCells++;
      }

    totalCells += totalBoundaryCells;
    }

  return totalCells;
}

//----------------------------------------------------------------------------
int vtkKdTree::GetRegionContainingCell(vtkIdType cellID)
{
  return this->GetRegionContainingCell(this->DataSets[0], cellID);
}

//----------------------------------------------------------------------------
int vtkKdTree::GetRegionContainingCell(int set, vtkIdType cellID)
{
  if ( (set < 0) || 
       (set >= this->NumDataSetsAllocated) ||
       (this->DataSets[set] == NULL))
    {
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell no such data set");
    return -1;
    }
  return this->GetRegionContainingCell(this->DataSets[set], cellID);
}

//----------------------------------------------------------------------------
int vtkKdTree::GetRegionContainingCell(vtkDataSet *set, vtkIdType cellID)
{
  int regionID = -1;

  if ( this->GetDataSet(set) < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell no such data set");
    return -1;
    }
  if ( (cellID < 0) || (cellID >= set->GetNumberOfCells()))
    {
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell bad cell ID");
    return -1;
    }
  
  if (this->CellRegionList)
    {
    if (set == this->DataSets[0])        // 99.99999% of the time
      {
      return this->CellRegionList[cellID];
      }
    
    int setNum = this->GetDataSet(set);
    
    int offset = this->GetDataSetsNumberOfCells(0, setNum-1);
    
    return this->CellRegionList[offset + cellID];
    }
  
  float center[3];
  
  this->ComputeCellCenter(set, cellID, center);
  
  regionID = this->GetRegionContainingPoint(center[0], center[1], center[2]);
  
  return regionID;
}

//----------------------------------------------------------------------------
int *vtkKdTree::AllGetRegionContainingCell()
{ 
  if (this->CellRegionList)
    {
    return this->CellRegionList;
    }
  this->CellRegionList = new int [this->GetNumberOfCells()];
  
  if (!this->CellRegionList)
    {
    vtkErrorMacro(<<"vtkKdTree::AllGetRegionContainingCell memory allocation");
    return NULL;
    }
  
  int *listPtr = this->CellRegionList;
  
  for (int set=0; set < this->NumDataSetsAllocated; set++)
    {
    if (this->DataSets[set] == NULL) 
      {
      continue;
      }

    int setCells = this->DataSets[set]->GetNumberOfCells();
    
    float *centers = this->ComputeCellCenters(set);
    
    float *pt = centers;

    for (int cellId = 0; cellId < setCells; cellId++)
      {
      listPtr[cellId] =
        this->GetRegionContainingPoint(pt[0], pt[1], pt[2]);

      pt += 3;
      }

    listPtr += setCells;

    delete [] centers;
    }

  return this->CellRegionList;
}

//----------------------------------------------------------------------------
int vtkKdTree::GetRegionContainingPoint(float x, float y, float z)
{
  return this->GetRegionContainingPoint((double)x, (double)y, (double)z);
}

//----------------------------------------------------------------------------
int vtkKdTree::GetRegionContainingPoint(double x, double y, double z)
{
  return vtkKdTree::findRegion(this->Top, x, y, z);
}

//----------------------------------------------------------------------------
int vtkKdTree::MinimalNumberOfConvexSubRegions(vtkIntArray *regionIdList,
                                               double **convexSubRegions)
{
  int i;
  int nids = regionIdList->GetNumberOfTuples();
  int *ids = regionIdList->GetPointer(0);

  if (nids < 1)
    {
    return 0;
    }

  if (nids == 1)
    {
    if ( (ids[0] < 0) || (ids[0] >= this->NumRegions))
      {
      return 0;
      }

    double *bounds = new double [6];

    this->RegionList[ids[0]]->GetBounds(bounds);

    *convexSubRegions = bounds;

    return 1;
    }

  // create a sorted list of unique region Ids

  int *idList = new int [nids];
  int nUniqueIds;

  memcpy(idList, ids, nids * sizeof(int));

  SORTLIST(idList, nids);

  if ( (idList[0] < 0) || (idList[nids-1] >= this->NumRegions)) 
    {
    delete [] idList;
    return 0;
    }

  REMOVEDUPLICATES(idList, nids, nUniqueIds);

  vtkKdNode **regions = new vtkKdNode * [nUniqueIds];
  
  int nregions = vtkKdTree::__ConvexSubRegions(idList, nUniqueIds, 
                                             this->Top, regions);
  
  double *bounds = new double [nregions * 6];
  
  for (i=0; i<nregions; i++)
    {
    regions[i]->GetBounds(bounds + (i*6));
    }
  
  *convexSubRegions = bounds;
  
  delete [] idList;
  delete [] regions;
  
  return nregions;
}

//----------------------------------------------------------------------------
int vtkKdTree::__ConvexSubRegions(int *ids, int len, vtkKdNode *tree, vtkKdNode **nodes)
{ 
  int nregions = tree->MaxId - tree->MinId + 1;
  
  if (nregions == len)
    {
    *nodes = tree;
    return 1;
    }
  
  if (tree->Left == NULL)
    {
    return 0;
    }
  
  int min = ids[0];
  int max = ids[len-1];
  
  int leftMax = tree->Left->MaxId;
  int rightMin = tree->Right->MinId;
  
  if (max <= leftMax)
    {
    return vtkKdTree::__ConvexSubRegions(ids, len, tree->Left, nodes);
    }
  else if (min >= rightMin)
    {
    return vtkKdTree::__ConvexSubRegions(ids, len, tree->Right, nodes);
    }
  else
    {
    int leftIds = 1;

    for (int i=1; i<len-1; i++)
      {
      if (ids[i] <= leftMax) 
        {
        leftIds++;
        }
      else                   
        {
        break;
        }
      }

    int numNodesLeft =
      vtkKdTree::__ConvexSubRegions(ids, leftIds, tree->Left, nodes);

    int numNodesRight =
      vtkKdTree::__ConvexSubRegions(ids + leftIds, len - leftIds,
                               tree->Right, nodes + numNodesLeft);

    return (numNodesLeft + numNodesRight);
    }
}

//----------------------------------------------------------------------------
int vtkKdTree::DepthOrderRegions(vtkIntArray *regionIds,
                       vtkCamera *camera, vtkIntArray *orderedList)
{
  int nRegions = regionIds->GetNumberOfTuples();
  int nUniqueRegions = 0;
  int *sorted = NULL;

  if (nRegions > 0)
    {
    int *regionPtr = regionIds->GetPointer(0);
    sorted = new int [nRegions];

    memcpy(sorted, regionPtr, sizeof(int) * nRegions);

    SORTLIST(sorted, nRegions);
    REMOVEDUPLICATES(sorted, nRegions, nUniqueRegions);
    }

  vtkIntArray *IdsOfInterest = NULL;

  if (sorted)
    {
    IdsOfInterest = vtkIntArray::New();
    IdsOfInterest->SetArray(sorted, nUniqueRegions, 0); 
    } 

  int size = this->_DepthOrderRegions(IdsOfInterest, camera, orderedList);

  IdsOfInterest->Delete();
 
  return size;
}

//----------------------------------------------------------------------------
int vtkKdTree::DepthOrderAllRegions(vtkCamera *camera, vtkIntArray *orderedList)
{
  return this->_DepthOrderRegions(NULL, camera, orderedList);
}     

//----------------------------------------------------------------------------
int vtkKdTree::_DepthOrderRegions(vtkIntArray *IdsOfInterest,
                                 vtkCamera *camera, vtkIntArray *orderedList)
{
  int nextId = 0;

  int numValues = (IdsOfInterest ? IdsOfInterest->GetNumberOfTuples() :
                                   this->NumRegions);

  orderedList->Initialize();
  orderedList->SetNumberOfValues(numValues);

  double dir[3];

  camera->GetDirectionOfProjection(dir);

  int size = 
    vtkKdTree::__DepthOrderRegions(this->Top, orderedList, IdsOfInterest, dir, nextId);

  if (size < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::DepthOrderRegions k-d tree structure is corrupt");
    orderedList->Initialize();
    return 0;
    }
    
  return size;
}     

//----------------------------------------------------------------------------
int vtkKdTree::__DepthOrderRegions(vtkKdNode *node, 
                                   vtkIntArray *list, vtkIntArray *IdsOfInterest,
                                   double *dir, int nextId)
{
  if (node->Left == NULL)
    {
    if (!IdsOfInterest || vtkKdTree::FoundId(IdsOfInterest, node->Id))
      {
      list->SetValue(nextId, node->Id);
      nextId = nextId+1;
      }

      return nextId;
    }   
                               
  int cutPlane = node->Dim;
    
  if ((cutPlane < 0) || (cutPlane > 2))
    {
    return -1;
    } 
                       
  double closest = dir[cutPlane] * -1;
  
  vtkKdNode *closeNode = (closest < 0) ? node->Left : node->Right;
  vtkKdNode *farNode  = (closest >= 0) ? node->Left: node->Right;
    
  int nextNextId = vtkKdTree::__DepthOrderRegions(closeNode, list, 
                                         IdsOfInterest, dir, nextId);

  if (nextNextId == -1) 
    {
    return -1;
    }
    
  nextNextId = vtkKdTree::__DepthOrderRegions(farNode, list, 
                                     IdsOfInterest, dir, nextNextId);

  return nextNextId;
}


//----------------------------------------------------------------------------
//  Query functions ----------------------------------------------------
//    K-d Trees are particularly efficient with region intersection
//    queries, like finding all regions that intersect a view frustum
//
// Intersection with axis-aligned box----------------------------------
//
int vtkKdTree::IntersectsBox(int regionId, float *x)
{
  return this->IntersectsBox(regionId, (double)x[0], (double)x[1], 
                 (double)x[2], (double)x[3], (double)x[4], (double)x[5]);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsBox(int regionId, double *x)
{
  return this->IntersectsBox(regionId, x[0], x[1], x[2], x[3], x[4], x[5]);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsBox(int regionId, float x0, float x1, 
                           float y0, float y1, float z0, float z1)
{
  return this->IntersectsBox(regionId, (double)x0, (double)x1, 
                      (double)y0, (double)y1, (double)z0, (double)z1);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsBox(int regionId, double x0, double x1, 
                           double y0, double y1, double z0, double z1)
{
  if ( (regionId < 0) || (regionId >= this->NumRegions))
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsBox invalid spatial region ID");
    return 0;
    }

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsBox(x0, x1, y0, y1, z0, z1,
                     this->ComputeIntersectionsUsingDataBounds);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsBox(int *ids, int len, float *x)
{
  return this->IntersectsBox(ids, len,  (double)x[0], (double)x[1], 
                (double)x[2], (double)x[3], (double)x[4], (double)x[5]);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsBox(int *ids, int len, double *x)
{
  return this->IntersectsBox(ids, len,  x[0], x[1], x[2], x[3], x[4], x[5]);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsBox(int *ids, int len, 
                             float x0, float x1,
                             float y0, float y1, float z0, float z1)
{
  return this->IntersectsBox(ids, len,
              (double)x0, (double)x1, (double)y0, (double)y1, 
              (double)z0, (double)z1);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsBox(int *ids, int len, 
                             double x0, double x1,
                             double y0, double y1, double z0, double z1)
{
  int nnodes = 0;

  if (len > 0)
    {
    nnodes = this->_IntersectsBox(this->Top, ids, len,
                             x0, x1, y0, y1, z0, z1);
    }
  return nnodes;
}

//----------------------------------------------------------------------------
int vtkKdTree::_IntersectsBox(vtkKdNode *node, int *ids, int len, 
                             double x0, double x1,
                             double y0, double y1, double z0, double z1)
{
  int result, nnodes1, nnodes2, listlen;
  int *idlist;

  result = node->IntersectsBox(x0, x1, y0, y1, z0, z1,
                             this->ComputeIntersectionsUsingDataBounds);

  if (!result) 
    {
    return 0;
    }

  if (node->Left == NULL)
    {
    ids[0] = node->Id;
    return 1;
    }

  nnodes1 = _IntersectsBox(node->Left, ids, len, x0, x1, y0, y1, z0, z1);

  idlist = ids + nnodes1;
  listlen = len - nnodes1;

  if (listlen > 0)
    {
    nnodes2 = _IntersectsBox(node->Right, idlist, listlen, x0, x1, y0, y1, z0, z1);
    }
  else
    {
    nnodes2 = 0;
    }

  return (nnodes1 + nnodes2);
}

//----------------------------------------------------------------------------
// Intersection with a sphere---------------------------------------
//
int vtkKdTree::IntersectsSphere2(int regionId,
                          float x, float y, float z, float rSquared)
{
  return this->IntersectsSphere2(regionId, (double)x, (double)y, (double)z,
                                 (double)rSquared);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsSphere2(int regionId, double x, double y, double z, 
                                double rSquared) 
{
  if ( (regionId < 0) || (regionId >= this->NumRegions))
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsSpher2 invalid spatial region ID");
    return 0;
    }

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsSphere2(x, y, z, rSquared,
                     this->ComputeIntersectionsUsingDataBounds);
  
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsSphere2(int *ids, int len,
                       float x, float y, float z, float rSquared)
{                            
  return this->IntersectsSphere2(ids, len, (double)x, (double)y,
                                 (double)z, (double)rSquared);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsSphere2(int *ids, int len,
                       double x, double y, double z, double rSquared)
{                            
  int nnodes = 0;
  
  if (len > 0)
    {
    nnodes = this->_IntersectsSphere2(this->Top, ids, len, x, y, z, rSquared);
    }                        
  return nnodes;
} 

//----------------------------------------------------------------------------
int vtkKdTree::_IntersectsSphere2(vtkKdNode *node, int *ids, int len,
                                  double x, double y, double z, double rSquared)
{                            
  int result, nnodes1, nnodes2, listlen;
  int *idlist;
  
  result = node->IntersectsSphere2(x, y, z, rSquared,
                             this->ComputeIntersectionsUsingDataBounds);
                             
  if (!result) 
    {
    return 0;
    }
  
  if (node->Left == NULL)
    {
    ids[0] = node->Id;
    return 1;
    }
    
  nnodes1 = _IntersectsSphere2(node->Left, ids, len, x, y, z, rSquared);
  
  idlist = ids + nnodes1;
  listlen = len - nnodes1;
  
  if (listlen > 0)
    {
    nnodes2 = _IntersectsSphere2(node->Right, idlist, listlen, x, y, z, rSquared);
    }
  else
    {
    nnodes2 = 0;
    }

  return (nnodes1 + nnodes2);
}

//----------------------------------------------------------------------------
// Intersection with arbitrary vtkCell -----------------------------
//
int vtkKdTree::IntersectsCell(int regionId, int cellId, int cellRegion)
{                            
  return this->IntersectsCell(regionId, this->DataSets[0], cellId, cellRegion);
}
  
//----------------------------------------------------------------------------
int vtkKdTree::IntersectsCell(int regionId, vtkDataSet *set, int cellId, int cellRegion)
{             
  if (this->GetDataSet(set) < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid data set");
    return 0;                
    }                          

  vtkCell *cell = set->GetCell(cellId);

  if (!cell)
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid cell ID");
    return 0;                
    }
  
  return this->IntersectsCell(regionId, cell, cellRegion);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsCell(int regionId, vtkCell *cell, int cellRegion)
{                            
  if ( (regionId < 0) || (regionId >= this->NumRegions)) 
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid region ID");
    return 0;
    }

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsCell(cell, this->ComputeIntersectionsUsingDataBounds,
                              cellRegion);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsCell(int *ids, int len, int cellId, int cellRegion)
{
  return this->IntersectsCell(ids, len, this->DataSets[0], cellId, cellRegion);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsCell(int *ids, int len, vtkDataSet *set, int cellId, int cellRegion)
{
  if (this->GetDataSet(set) < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid data set");
    return 0;
    }

  vtkCell *cell = set->GetCell(cellId);

  if (!cell)
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid cell ID");
    return 0;
    }

  return this->IntersectsCell(ids, len, cell, cellRegion);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsCell(int *ids, int len, vtkCell *cell, int cellRegion)
{
  vtkKdTree::SetCellBounds(cell, this->CellBoundsCache);

  this->Top->CellBoundsCache = this->CellBoundsCache;

  return this->_IntersectsCell(this->Top, ids, len, cell, cellRegion);
}

//----------------------------------------------------------------------------
int vtkKdTree::_IntersectsCell(vtkKdNode *node, int *ids, int len,
                                 vtkCell *cell, int cellRegion)
{
  int result, nnodes1, nnodes2, listlen, intersects;
  int *idlist;

  intersects = node->IntersectsCell(cell,
                                this->ComputeIntersectionsUsingDataBounds,
                                cellRegion);

  if (intersects)
    {
    if (node->Left)
      {
      node->Left->CellBoundsCache = node->CellBoundsCache;

      nnodes1 = this->_IntersectsCell(node->Left, ids, len, cell,
                                cellRegion);

      idlist = ids + nnodes1;
      listlen = len - nnodes1;
  
      if (listlen > 0) 
        {       
        node->Right->CellBoundsCache = node->CellBoundsCache;

        nnodes2 = this->_IntersectsCell(node->Right, idlist, listlen, cell,
                                  cellRegion);
        }
      else
        {
        nnodes2 = 0;
        }
  
      result = nnodes1 + nnodes2;
      }
    else
      {
      ids[0] = node->Id;     // leaf node (spatial region)

      result = 1;
      }
    } 
  else
    {
    result = 0;
    }

  node->CellBoundsCache = NULL;

  return result;
}

//----------------------------------------------------------------------------
// Intersection with arbitrary convex region bounded by planes -----------
//
int vtkKdTree::IntersectsRegion(int regionId, vtkPlanes *planes)
{
  return this->IntersectsRegion(regionId, planes, 0, (double *)NULL);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsRegion(int regionId, vtkPlanes *planes, 
                                int nvertices, float *vertices)
{
  int i;

  double *dv = new double[nvertices];
  for (i=0; i<nvertices; i++)
    {
    dv[i] = vertices[i];
    }
  int intersects = this->IntersectsRegion(regionId, 
                                          planes, nvertices, dv);

  if (dv) 
    {
    delete [] dv;
    }
  return intersects;
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsRegion(int regionId, vtkPlanes *planes, 
                                int nvertices, double *vertices)
{
  if ( (regionId < 0) || (regionId >= this->NumRegions)) 
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsRegion invalid region ID");
    return 0;
    }

  vtkPlanesIntersection *pi = vtkPlanesIntersection::New();

  pi->SetNormals(planes->GetNormals());
  pi->SetPoints(planes->GetPoints());

  if (vertices && (nvertices>0)) 
    {
    pi->SetRegionVertices(vertices, nvertices);
    }

  vtkKdNode *node = this->RegionList[regionId];

  int intersects = 
    node->IntersectsRegion(pi, this->ComputeIntersectionsUsingDataBounds);

  pi->Delete();

  return intersects;
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsRegion(int *ids, int len, vtkPlanes *planes) 
{
  return this->IntersectsRegion(ids, len, planes, 0, (double *)NULL);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsRegion(int *ids, int len, vtkPlanes *planes, 
                                int nvertices, float *vertices)
{
  double *dv;
  int i;

  if (nvertices > 0)
    {
    dv = new double[nvertices];
    for (i=0; i<nvertices; i++)
      {
      dv[i] = vertices[i];
      }
    }
  else
    {
    dv = NULL;
    }
  int howmany = this->IntersectsRegion(ids, len, planes, 
                                     nvertices, dv);

  delete [] dv;

  return howmany; 
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsRegion(int *ids, int len, vtkPlanes *planes, 
                                int nvertices, double *vertices)
{
  int nnodes = 0;
  vtkPlanesIntersection *pi = vtkPlanesIntersection::New();

  pi->SetNormals(planes->GetNormals());
  pi->SetPoints(planes->GetPoints());

  if (vertices && (nvertices>0)) 
    {
    pi->SetRegionVertices(vertices, nvertices);
    }

  if (len > 0)
    {
    nnodes = _IntersectsRegion(this->Top, ids, len, pi);
    }

  pi->Delete();

  return nnodes;
}

//----------------------------------------------------------------------------
int vtkKdTree::_IntersectsRegion(vtkKdNode *node, int *ids, int len, 
                                 vtkPlanesIntersection *pi)
{
  int result, nnodes1, nnodes2, listlen;
  int *idlist;

  result = node->IntersectsRegion(pi, this->ComputeIntersectionsUsingDataBounds);

  if (!result) 
    {
    return 0;
    }

  if (node->Left == NULL)
    {
    ids[0] = node->Id;
    return 1;
    }

  nnodes1 = _IntersectsRegion(node->Left, ids, len, pi);

  idlist = ids + nnodes1;
  listlen = len - nnodes1;

  if (listlen > 0)
    {
    nnodes2 = _IntersectsRegion(node->Right, idlist, listlen, pi);
    }
  else
    {
    nnodes2 = 0;
    }

  return (nnodes1 + nnodes2);
}

//----------------------------------------------------------------------------
// Intersection with a view frustum that is a rectangular portion (or all)
//  of a viewport.  
//
int vtkKdTree::IntersectsFrustum(int regionId, vtkRenderer *ren, 
                       float x0, float x1, float y0, float y1)
{
  return this->IntersectsFrustum(regionId, ren,
               (double)x0, (double)x1, (double)y0, (double)y1);

}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsFrustum(int regionId, vtkRenderer *ren, 
                       double x0, double x1, double y0, double y1)
{
  if ( (x0 < -1) || (x1 > 1) || (y0 < -1) || (y1 > 1))
    {
    vtkErrorMacro(<<
      "vtkKdTree::IntersectsFrustum, use view coordinates ([-1,1], [-1,1])");
    return 0;
    }

  if ( (regionId < 0) || (regionId >= this->NumRegions))
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsFrustum invalid region ID");
    return 0;
    }

  vtkPlanesIntersection *planes = 
    vtkPlanesIntersection::ConvertFrustumToWorld(ren, x0, x1, y0, y1);

  vtkKdNode *node = this->RegionList[regionId];

  int intersects = node->IntersectsRegion(planes, 
                   this->ComputeIntersectionsUsingDataBounds);

  return intersects;
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsFrustum(int *ids, int len, vtkRenderer *ren, 
                       float x0, float x1, float y0, float y1)
{
  return this->IntersectsFrustum(ids, len, ren, (double)x0, (double)x1,
                       (double)y0, (double)y1);
}

//----------------------------------------------------------------------------
int vtkKdTree::IntersectsFrustum(int *ids, int len, vtkRenderer *ren, 
                       double x0, double x1, double y0, double y1)
{
  if ( (x0 < -1) || (x1 > 1) || (y0 < -1) || (y1 > 1))
    {
    vtkErrorMacro(<<
      "vtkKdTree::IntersectsFrustum, use view coordinates ([-1,1], [-1,1])");
    return 0;
    }

  vtkPlanesIntersection *planes = 
        vtkPlanesIntersection::ConvertFrustumToWorld(ren, x0, x1, y0, y1);

  int howmany = _IntersectsRegion(this->Top, ids, len, planes);

  planes->Delete();

  return howmany;
}

//----------------------------------------------------------------------------
// These requests change the boundaries of the k-d tree, so must
// update the MTime.
//
void vtkKdTree::NewPartitioningRequest(int req)
{
  if (req != this->ValidDirections)
    {
    this->Modified();
    this->ValidDirections = req;
    }
}

//----------------------------------------------------------------------------
void vtkKdTree::OmitXPartitioning()
{
  this->NewPartitioningRequest((1 << vtkKdTree::YDIM) | (1 << vtkKdTree::ZDIM));
}

//----------------------------------------------------------------------------
void vtkKdTree::OmitYPartitioning()
{
  this->NewPartitioningRequest((1 << vtkKdTree::ZDIM) | (1 << vtkKdTree::XDIM));
}

//----------------------------------------------------------------------------
void vtkKdTree::OmitZPartitioning()
{
  this->NewPartitioningRequest((1 << vtkKdTree::XDIM) | (1 << vtkKdTree::YDIM));
}

//----------------------------------------------------------------------------
void vtkKdTree::OmitXYPartitioning()
{   
  this->NewPartitioningRequest((1 << vtkKdTree::ZDIM));
}      

//----------------------------------------------------------------------------
void vtkKdTree::OmitYZPartitioning()
{         
  this->NewPartitioningRequest((1 << vtkKdTree::XDIM));
}

//----------------------------------------------------------------------------
void vtkKdTree::OmitZXPartitioning()
{
  this->NewPartitioningRequest((1 << vtkKdTree::YDIM));
}

//----------------------------------------------------------------------------
void vtkKdTree::OmitNoPartitioning()
{
  int req = ((1 << vtkKdTree::XDIM)|(1 << vtkKdTree::YDIM)|(1 << vtkKdTree::ZDIM));
  this->NewPartitioningRequest(req);
}

//---------------------------------------------------------------------------
void vtkKdTree::PrintTiming(ostream& os, vtkIndent )
{
  vtkTimerLog::DumpLogWithIndents(&os, (float)0.0);
}

//----------------------------------------------------------------------------
void vtkKdTree::PrintSelf(ostream& os, vtkIndent indent)
{ 
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ValidDirections: " << this->ValidDirections << endl;
  os << indent << "MinCells: " << this->MinCells << endl;
  os << indent << "NumRegions: " << this->NumRegions << endl;

  os << indent << "DataSets: " << this->DataSets << endl;
  os << indent << "NumDataSets: " << this->NumDataSets << endl;

  os << indent << "Top: " << this->Top << endl;
  os << indent << "RegionList: " << this->RegionList << endl;

  os << indent << "Timing: " << this->Timing << endl;
  os << indent << "TimerLog: " << this->TimerLog << endl;

  os << indent << "NumDataSetsAllocated: " << this->NumDataSetsAllocated << endl;
  os << indent << "IncludeRegionBoundaryCells: ";
        os << this->IncludeRegionBoundaryCells << endl;
  os << indent << "GenerateRepresentationUsingDataBounds: ";
        os<< this->GenerateRepresentationUsingDataBounds << endl;
  os << indent << "ComputeIntersectionsUsingDataBounds: ";
        os<< this->ComputeIntersectionsUsingDataBounds << endl;

  if (this->CellList.nRegions > 0)
    {
    os << indent << "CellList.dataSet " << this->CellList.dataSet << endl;
    os << indent << "CellList.regionIds " << this->CellList.regionIds << endl;
    os << indent << "CellList.nRegions " << this->CellList.nRegions << endl;
    os << indent << "CellList.cells " << this->CellList.cells << endl;
    os << indent << "CellList.boundaryCells " << this->CellList.boundaryCells << endl;
    }
  os << indent << "CellRegionList: " << this->CellRegionList << endl;

  os << indent << "LocatorPoints: " << this->LocatorPoints << endl;
  os << indent << "NumberOfLocatorPoints: " << this->NumberOfLocatorPoints << endl;
  os << indent << "LocatorIds: " << this->LocatorIds << endl;
  os << indent << "LocatorRegionLocation: " << this->LocatorRegionLocation << endl;

  os << indent << "FudgeFactor: " << this->FudgeFactor << endl;
  os << indent << "MaxWidth: " << this->MaxWidth << endl;
}
