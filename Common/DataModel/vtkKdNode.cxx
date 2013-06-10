/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKdNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkKdNode.h"
#include "vtkCell.h"
#include "vtkBox.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkPlanesIntersection.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkKdNode);
vtkCxxSetObjectMacro(vtkKdNode, Left, vtkKdNode);
vtkCxxSetObjectMacro(vtkKdNode, Right, vtkKdNode);
vtkCxxSetObjectMacro(vtkKdNode, Up, vtkKdNode);

// ----------------------------------------------------------------------------
vtkKdNode::vtkKdNode()
{
  this->Up = this->Left = this->Right = NULL;
  this->Dim = 3;
  this->ID = -1;
  this->MinID = -1;
  this->MaxID = -1;
  this->NumberOfPoints= 0;

  this->Min[0]=0.0;
  this->Min[1]=0.0;
  this->Min[2]=0.0;
  this->Max[0]=0.0;
  this->Max[1]=0.0;
  this->Max[2]=0.0;
  this->MinVal[0]=0.0;
  this->MinVal[1]=0.0;
  this->MinVal[2]=0.0;
  this->MaxVal[0]=0.0;
  this->MaxVal[1]=0.0;
  this->MaxVal[2]=0.0;
}

// ----------------------------------------------------------------------------
vtkKdNode::~vtkKdNode()
{
  this->SetLeft(0);
  this->SetRight(0);
  this->SetUp(0);
}

// ----------------------------------------------------------------------------
void vtkKdNode::SetBounds(double x1,double x2,double y1,double y2,double z1,
                          double z2)
{
   this->Min[0] = x1; this->Max[0] = x2;
   this->Min[1] = y1; this->Max[1] = y2;
   this->Min[2] = z1; this->Max[2] = z2;
}

// ----------------------------------------------------------------------------
void vtkKdNode::SetMinBounds(double *b)
{
  this->Min[0] = b[0];
  this->Min[1] = b[1];
  this->Min[2] = b[2];
}

// ----------------------------------------------------------------------------
void vtkKdNode::SetMaxBounds(double *b)
{
  this->Max[0] = b[0];
  this->Max[1] = b[1];
  this->Max[2] = b[2];
}

// ----------------------------------------------------------------------------
void vtkKdNode::SetMinDataBounds(double *b)
{
  this->MinVal[0] = b[0];
  this->MinVal[1] = b[1];
  this->MinVal[2] = b[2];
}

// ----------------------------------------------------------------------------
void vtkKdNode::SetMaxDataBounds(double *b)
{
  this->MaxVal[0] = b[0];
  this->MaxVal[1] = b[1];
  this->MaxVal[2] = b[2];
}


// ----------------------------------------------------------------------------
void vtkKdNode::GetBounds(double *b) const
{
   b[0] = this->Min[0]; b[1] = this->Max[0];
   b[2] = this->Min[1]; b[3] = this->Max[1];
   b[4] = this->Min[2]; b[5] = this->Max[2];
}

// ----------------------------------------------------------------------------
void vtkKdNode::SetDataBounds(double x1,double x2,double y1,double y2,
                              double z1,double z2)
{
   this->MinVal[0] = x1; this->MaxVal[0] = x2;
   this->MinVal[1] = y1; this->MaxVal[1] = y2;
   this->MinVal[2] = z1; this->MaxVal[2] = z2;
}

// ----------------------------------------------------------------------------
void vtkKdNode::SetDataBounds(float *v)
{
  int x;
  double newbounds[6];

  vtkIdType numPoints = this->GetNumberOfPoints();

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

    newbounds[dim*2] = newbounds[dim*2+1] = static_cast<double>(v[dim]);

    for (i = dim+3; i< numPoints*3; i+=3)
      {
      if (v[i] < newbounds[dim*2])
        {
        newbounds[dim*2] = static_cast<double>(v[i]);
        }
      else if (v[i] > newbounds[dim*2+1])
        {
        newbounds[dim*2+1] = static_cast<double>(v[i]);
        }
      }
    }
  else
    {
    for (i=0; i<3; i++)
      {
      newbounds[i*2] = newbounds[i*2+1] = static_cast<double>(v[i]);
      }

    for (x = 3; x< numPoints*3; x+=3)
      {
      int y=x+1;
      int z=x+2;

      if (v[x] < newbounds[0])
        {
        newbounds[0] = static_cast<double>(v[x]);
        }
      else if (v[x] > newbounds[1])
        {
        newbounds[1] = static_cast<double>(v[x]);
        }

      if (v[y] < newbounds[2])
        {
        newbounds[2] = static_cast<double>(v[y]);
        }
      else if (v[y] > newbounds[3])
        {
        newbounds[3] = static_cast<double>(v[y]);
        }

      if (v[z] < newbounds[4])
        {
        newbounds[4] = static_cast<double>(v[z]);
        }
      else if (v[z] > newbounds[5])
        {
        newbounds[5] = static_cast<double>(v[z]);
        }
      }
    }

  this->SetDataBounds(newbounds[0], newbounds[1], newbounds[2],
            newbounds[3], newbounds[4], newbounds[5]);
}

// ----------------------------------------------------------------------------
void vtkKdNode::GetDataBounds(double *b) const
{
   b[0] = this->MinVal[0]; b[1] = this->MaxVal[0];
   b[2] = this->MinVal[1]; b[3] = this->MaxVal[1];
   b[4] = this->MinVal[2]; b[5] = this->MaxVal[2];
}

// ----------------------------------------------------------------------------
double vtkKdNode::GetDivisionPosition()
{
  if (this->Dim == 3)
    {
    vtkErrorMacro("Called GetDivisionPosition() on a leaf node.");
    return 0.0;
    }

  vtkKdNode *left = this->GetLeft();
  if (!left)
    {
    vtkErrorMacro("Called GetDivisionPosition() on a leaf node.");
    return 0.0;
    }

  return left->GetMaxBounds()[this->Dim];
}

// ----------------------------------------------------------------------------
// Distance (squared) from any point anywhere to the boundary of spatial region
//
double vtkKdNode::GetDistance2ToBoundary(double x, double y, double z,
                                         int useDataBounds=0)
{
  return this->_GetDistance2ToBoundary(x, y, z, NULL, 0, useDataBounds);
}

//----------------------------------------------------------------------------
// Distance (squared) from any point anywhere to the boundary of spatial
// region, and give me the point on the boundary closest to this point.
//
double vtkKdNode::GetDistance2ToBoundary(double x, double y, double z,
                                         double *p,
                                         int useDataBounds=0)
{
  return this->_GetDistance2ToBoundary(x, y, z, p, 0, useDataBounds);
}

// ----------------------------------------------------------------------------
// The point is inside the region, and I want the distance (squared)
// to the closest "interior" wall, one that is not an outer boundary of
// the entire space.
//

double vtkKdNode::GetDistance2ToInnerBoundary(double x, double y, double z)
{
  return this->_GetDistance2ToBoundary(x, y, z, NULL, 1, 0);
}

double vtkKdNode::_GetDistance2ToBoundary(
    double x, double y, double z,  // from this point
    double *p,              // set to point on boundary that is closest
    int innerBoundaryOnly, // ignore boundaries on "outside"
    int useDataBounds=0)   // use bounds of data within region instead
{
  double minDistance, dist;
  double edgePt[3];
  double cornerPt[3];
  double pt3[3];

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
      minDistance = x - xmin;
      mindim = 0;

      if ((dist = xmax - x) < minDistance)
        {
        mindim = 1;
        minDistance = dist;
        }
      if ((dist = y - ymin) < minDistance)
        {
        mindim = 2;
        minDistance = dist;
        }
      if ((dist = ymax - y) < minDistance)
        {
        mindim = 3;
        minDistance = dist;
        }
      if ((dist = z - zmin) < minDistance)
        {
        mindim = 4;
        minDistance = dist;
        }
      if ((dist = zmax - z) < minDistance)
        {
        mindim = 5;
        minDistance = dist;
        }
      }
    else
      {
      int first = 1;
      minDistance = VTK_FLOAT_MAX; // Suppresses warning message.

      if ((xmin != outerBoundaryMin[0]) &&
          (((dist = x - xmin) < minDistance) || first))
        {
        mindim = 0;
        minDistance = dist;
        first = 0;
        }
      if ((xmax != outerBoundaryMax[0]) &&
          (((dist = xmax - x) < minDistance) || first))
        {
        mindim = 1;
        minDistance = dist;
        first = 0;
        }
      if ((ymin != outerBoundaryMin[1]) &&
          (((dist = y - ymin) < minDistance) || first))
        {
        mindim = 2;
        minDistance = dist;
        first = 0;
        }
      if ((ymax != outerBoundaryMax[1]) &&
          (((dist = ymax - y) < minDistance) || first))
        {
        mindim = 3;
        minDistance = dist;
        first = 0;
        }
      if ((zmin != outerBoundaryMin[2]) &&
          (((dist = z - zmin) < minDistance) || first))
        {
        mindim = 4;
        minDistance = dist;
        first = 0;
        }
      if ((zmax != outerBoundaryMax[2]) &&
          (((dist = zmax - z) < minDistance) || first))
        {
        mindim = 5;
        minDistance = dist;
        }
      }

    // if there are no inner boundaries we dont want to square.
    if(minDistance != VTK_FLOAT_MAX)
      {
      minDistance *= minDistance;
      }

    if (p)
      {
      p[0] = x; p[1] = y; p[2] = z;


      if (mindim == 0)
        {
        p[0] = xmin;
        }
      else if (mindim == 1)
        {
        p[0] = xmax;
        }
      else if (mindim == 2)
        {
        p[1] = ymin;
        }
      else if (mindim == 3)
        {
        p[1] = ymax;
        }
      else if (mindim == 4)
        {
        p[2] = zmin;
        }
      else if (mindim == 5)
        {
        p[2] = zmax;
        }
      }
    }
  else if (withinX && withinY)  // point projects orthogonally to a face
    {
    minDistance = (zless ? zmin - z : z - zmax);
    minDistance *= minDistance;

    if (p)
      {
      p[0] = x; p[1] = y;
      p[2] = (zless ? zmin : zmax);
      }
    }
  else if (withinX && withinZ)
    {
    minDistance = (yless ? ymin - y : y - ymax);
    minDistance *= minDistance;

    if (p)
      {
      p[0] = x; p[2] = z;
      p[1] = (yless ? ymin : ymax);
      }
    }
  else if (withinY && withinZ)
    {
    minDistance = (xless ? xmin - x : x - xmax);
    minDistance *= minDistance;

    if (p)
      {
      p[1] = y; p[2] = z;
      p[0] = (xless ? xmin: xmax);
      }
    }
  else if (withinX || withinY || withinZ)   // point is closest to an edge
    {
    edgePt[0] = (withinX ? x : (xless ? xmin : xmax));
    edgePt[1] = (withinY ? y : (yless ? ymin : ymax));
    edgePt[2] = (withinZ ? z : (zless ? zmin : zmax));

    pt3[0] = x; pt3[1] = y; pt3[2] = z;

    minDistance = vtkMath::Distance2BetweenPoints(pt3, edgePt);

    if (p)
      {
      p[0] = edgePt[0]; p[1] = edgePt[1]; p[2] = edgePt[2];
      }
    }
  else                        // point is closest to a corner
    {
    cornerPt[0] = (xless ? xmin : xmax);
    cornerPt[1] = (yless ? ymin : ymax);
    cornerPt[2] = (zless ? zmin : zmax);

    pt3[0] = x; pt3[1] = y; pt3[2] = z;

    minDistance = vtkMath::Distance2BetweenPoints(pt3, cornerPt);

    if (p)
      {
      p[0] = cornerPt[0]; p[1] = cornerPt[1]; p[2] = cornerPt[2];
      }
    }

  return minDistance;
}

// ----------------------------------------------------------------------------
void vtkKdNode::AddChildNodes(vtkKdNode *left, vtkKdNode *right)
{
  this->DeleteChildNodes();

  if (left)
    {
    this->SetLeft(left);
    left->SetUp(this);
    }

  if (right)
    {
    this->SetRight(right);
    right->SetUp(this);
    }
}

// ----------------------------------------------------------------------------
void vtkKdNode::DeleteChildNodes()
{
  if (this->GetLeft())
    {
    this->GetLeft()->SetUp(NULL);
    this->SetLeft(NULL);
    }

  if (this->GetRight())
    {
    this->GetRight()->SetUp(NULL);
    this->SetRight(NULL);
    }
}

// ----------------------------------------------------------------------------
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

  if ( (min[0] > x1) ||
       (max[0] < x0) ||
       (min[1] > y1) ||
       (max[1] < y0) ||
       (min[2] > z1) ||
       (max[2] < z0))
    {
    return 0;
    }
  else
    {
    return 1;
    }
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

  double dist2 = this->GetDistance2ToBoundary(x, y, z, useDataBounds);

  if (dist2 < rSquared)
    {
    return 1;
    }

  return 0;
}

// ----------------------------------------------------------------------------
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

  if ( (min[0] > x0) ||
       (max[0] < x1) ||
       (min[1] > y0) ||
       (max[1] < y1) ||
       (min[2] > z0) ||
       (max[2] < z1))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

// ----------------------------------------------------------------------------
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

  if ( (min[0] > x) ||
       (max[0] < x) ||
       (min[1] > y) ||
       (max[1] < y) ||
       (min[2] > z) ||
       (max[2] < z))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
int vtkKdNode::IntersectsCell(vtkCell *cell, int useDataBounds, int cellRegion,
                              double *bounds)
{
  vtkIdType i;

  if ((useDataBounds==0) && (cellRegion >= 0))
    {
    if ( (cellRegion >= this->MinID) && (cellRegion <= this->MaxID))
      {
      return 1;    // the cell centroid is contained in this spatial region
      }
    }

  double *cellBounds = NULL;
  int deleteCellBounds = (bounds == NULL);

  if (deleteCellBounds)
    {
    cellBounds = new double [6];

    vtkPoints *pts = cell->GetPoints();
    pts->Modified();         // VTK bug - so bounds will be re-calculated
    pts->GetBounds(cellBounds);
    }
  else
    {
    cellBounds = bounds;
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

    vtkPoints *pts = cell->GetPoints();
    vtkIdType npts = pts->GetNumberOfPoints();

    for (i=0; i < npts ; i++)
      {
      double *pt = pts->GetPoint(i);
      if (this->ContainsPoint(pt[0],pt[1],pt[2], useDataBounds))
        {
        intersects = 1;
        break;
        }
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
  vtkIdType npts = pts->GetNumberOfPoints();

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

// ----------------------------------------------------------------------------
void vtkKdNode::PrintNode(int depth)
{
  if ( (depth < 0) || (depth > 19))
    {
    depth = 19;
    }

  for (int i=0; i<depth; i++)
    {
    cout << " ";
    }

  cout << " x (" << this->Min[0] << ", " << this->Max[0] << ") ";
  cout << " y (" << this->Min[1] << ", " << this->Max[1] << ") ";
  cout << " z (" << this->Min[2] << ", " << this->Max[2] << ") ";
  cout << this->NumberOfPoints << " cells, ";

  if (this->ID > -1)
    {
    cout << this->ID << " (leaf node)" << endl;
    }
  else
    {
    cout << this->MinID << " - " << this->MaxID << endl;
    }
}

// ----------------------------------------------------------------------------
void vtkKdNode::PrintVerboseNode(int depth)
{
  int i;

  if ( (depth < 0) || (depth > 19))
    {
    depth = 19;
    }

  for (i=0; i<depth; i++)
    {
    cout << " ";
    }

  cout << " Space ";
  cout << " x (" << this->Min[0] << ", " << this->Max[0] << ") ";
  cout << " y (" << this->Min[1] << ", " << this->Max[1] << ") ";
  cout << " z (" << this->Min[2] << ", " << this->Max[2] << ") " << endl;

  for (i=0; i<depth; i++)
    {
    cout << " ";
    }

  cout << " Data ";
  cout << " x (" << this->MinVal[0] << ", " << this->MaxVal[0] << ") ";
  cout << " y (" << this->MinVal[1] << ", " << this->MaxVal[1] << ") ";
  cout << " z (" << this->MinVal[2] << ", " << this->MaxVal[2] << ") " << endl;

  for (i=0; i<depth; i++)
    {
    cout << " ";
    }

  cout << this->NumberOfPoints << " cells, ";

  if (this->ID == -1)
    {
    cout << "id range " << this->MinID << " - " << this->MaxID << ", ";
    }
  else
    {
    cout << "id " << this->ID << ", ";
    }

  cout << "cut next along " << this->Dim << ", left ";
  cout << static_cast<void *>(this->Left) << ", right ";
  cout << static_cast<void *>(this->Right) << ", up "
       << static_cast<void *>(this->Up) << endl;
}

// ----------------------------------------------------------------------------
void vtkKdNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "Up: " << this->Up << endl;
  os << indent << "Left: " << this->Left << endl;
  os << indent << "Right: " << this->Right << endl;
  os << indent << "Dim: " << this->Dim << endl;
  os << indent << "ID: " << this->ID << endl;
  os << indent << "MinID: " << this->MinID << endl;
  os << indent << "MaxID: " << this->MaxID << endl;
  os << indent << "Min: " << this->Min[0] << " " << this->Min[1] << " "
     << this->Min[2] << endl;
  os << indent << "Max: " << this->Max[0] << " " << this->Max[1] << " "
     << this->Max[2] << endl;
  os << indent << "MinVal: " << this->MinVal[0] << " " << this->MinVal[1]
     << " " << this->MinVal[2] << endl;
  os << indent << "MaxVal: " << this->MaxVal[0] << " " << this->MaxVal[1]
     << " " << this->MaxVal[2] << endl;
}
