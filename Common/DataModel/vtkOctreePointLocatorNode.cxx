/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOctreePointLocatorNode.cxx

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

#include "vtkOctreePointLocatorNode.h"

#include "vtkBox.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlanesIntersection.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkOctreePointLocatorNode);

//----------------------------------------------------------------------------
vtkOctreePointLocatorNode::vtkOctreePointLocatorNode() :
  NumberOfPoints(0), Children(NULL), ID(-1), MinID(-1)
{
  // set the min and max data value and bounds since we won't know it
  // for a while
  for(int i=0;i<3;i++)
  {
    this->MinDataBounds[i] = VTK_DOUBLE_MAX;
    this->MaxDataBounds[i] = VTK_DOUBLE_MIN;
    this->MinBounds[i] = VTK_DOUBLE_MAX;
    this->MaxBounds[i] = VTK_DOUBLE_MIN;
  }
}

//----------------------------------------------------------------------------
vtkOctreePointLocatorNode::~vtkOctreePointLocatorNode()
{
  this->DeleteChildNodes();
}

//----------------------------------------------------------------------------
void vtkOctreePointLocatorNode::CreateChildNodes()
{
  if(!this->Children)
  {
    int i;
    double midpoint[3];
    for(i=0;i<3;i++)
    {
      midpoint[i] = (this->MinBounds[i]+this->MaxBounds[i])*.5;
    }
    this->Children = new vtkOctreePointLocatorNode*[8];
    for(i=0;i<8;i++)
    {
      this->Children[i] = vtkOctreePointLocatorNode::New();
      double NewMin[3], NewMax[3];
      for(int j=0;j<3;j++)
      {
        if(!((i>>j) & 1))
        {
          NewMin[j] = this->MinBounds[j];
          NewMax[j] = midpoint[j];
        }
        else
        {
          NewMin[j] = midpoint[j];
          NewMax[j] = this->MaxBounds[j];
        }
      }
      this->Children[i]->SetMinBounds(NewMin);
      this->Children[i]->SetMaxBounds(NewMax);
    }
  }
}

//----------------------------------------------------------------------------
void vtkOctreePointLocatorNode::DeleteChildNodes()
{
  if(this->Children)
  {
    for(int i=0;i<8;i++)
    {
      this->Children[i]->Delete();
    }
    delete []this->Children;
    this->Children = 0;
  }
}

//----------------------------------------------------------------------------
vtkOctreePointLocatorNode* vtkOctreePointLocatorNode::GetChild(int i)
{
  if(this->Children)
  {
    return this->Children[i];
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkOctreePointLocatorNode::GetSubOctantIndex(
  double* point, int CheckContainment)
{
  int i, index = 0;
  if(CheckContainment)
  {
    for(i=0;i<3;i++)
    {
      if(point[i] <= this->MinBounds[i] || point[i] > this->MaxBounds[i])
      {
        return -1;
      }
    }
  }

  for(i=0;i<3;i++)
  {
    if(point[i] > (this->MinBounds[i]+this->MaxBounds[i])*.5)
    {
      index += 1 << i;
    }
  }
  return index;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocatorNode::ComputeOctreeNodeInformation(
  vtkOctreePointLocatorNode* Parent, int & NextLeafId, int & NextMinId,
  float* coordinates)
{
  this->MinID = NextMinId;
  if(this->Children)
  {
    int i;
    for(i=0;i<8;i++)
    {
      this->Children[i]->ComputeOctreeNodeInformation(
        this, NextLeafId, NextMinId, coordinates);
    }
    // a non-leaf region can get its data bounds from its children...
    this->SetMinDataBounds(this->Children[0]->GetMinDataBounds());
    this->SetMaxDataBounds(this->Children[0]->GetMaxDataBounds());

    for(i=1;i<8;i++)
    {
      double* min = this->Children[i]->GetMinDataBounds();
      double* max = this->Children[i]->GetMaxDataBounds();
      for(int j=0;j<3;j++)
      {
        if(min[j] < this->MinDataBounds[j])
        {
          this->MinDataBounds[j] = min[j];
        }
        if(max[j] > this->MaxDataBounds[j])
        {
          this->MaxDataBounds[j] = max[j];
        }
      }
    }
  }
  else
  {
    this->ID = NextLeafId;
    NextLeafId++;
    NextMinId = this->MinID+this->NumberOfPoints;
    if(this->NumberOfPoints == 0)
    {
      // since there are no points in this, set the data bounds
      // such that they won't affect anything else
      this->SetMinDataBounds(Parent->GetMaxBounds());
      this->SetMaxDataBounds(Parent->GetMinBounds());
    }
    else
    {
      int i;
      float* coordptr = coordinates+3*this->MinID;
      for(i=0;i<3;i++)
      {
        this->MinDataBounds[i] = this->MaxDataBounds[i] = coordptr[i];
      }
      for(i=1;i<this->NumberOfPoints;i++)
      {
        coordptr += 3;
        for(int j=0;j<3;j++)
        {
          if(coordptr[j] < this->MinDataBounds[j])
          {
            this->MinDataBounds[j] = coordptr[j];
          }
          else if(coordptr[j] > this->MaxDataBounds[j])
          {
            this->MaxDataBounds[j] = coordptr[j];
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkOctreePointLocatorNode::SetBounds(
  double xMin,double xMax,double yMin,double yMax,double zMin,double zMax)
{
   this->MinBounds[0] = xMin; this->MaxBounds[0] = xMax;
   this->MinBounds[1] = yMin; this->MaxBounds[1] = yMax;
   this->MinBounds[2] = zMin; this->MaxBounds[2] = zMax;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocatorNode::GetBounds(double *b) const
{
   b[0] = this->MinBounds[0]; b[1] = this->MaxBounds[0];
   b[2] = this->MinBounds[1]; b[3] = this->MaxBounds[1];
   b[4] = this->MinBounds[2]; b[5] = this->MaxBounds[2];
}

//----------------------------------------------------------------------------
void vtkOctreePointLocatorNode::SetDataBounds(
  double xMin,double xMax,double yMin,double yMax,double zMin,double zMax)
{
   this->MinDataBounds[0] = xMin; this->MaxDataBounds[0] = xMax;
   this->MinDataBounds[1] = yMin; this->MaxDataBounds[1] = yMax;
   this->MinDataBounds[2] = zMin; this->MaxDataBounds[2] = zMax;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocatorNode::GetDataBounds(double *b) const
{
   b[0] = this->MinDataBounds[0]; b[1] = this->MaxDataBounds[0];
   b[2] = this->MinDataBounds[1]; b[3] = this->MaxDataBounds[1];
   b[4] = this->MinDataBounds[2]; b[5] = this->MaxDataBounds[2];
}

//----------------------------------------------------------------------------
// Squared distance from any point anywhere to the boundary of spatial region
//
double vtkOctreePointLocatorNode::GetDistance2ToBoundary(
  double x, double y, double z, vtkOctreePointLocatorNode* top,
  int useDataBounds=0)
{
  return this->_GetDistance2ToBoundary(x, y, z, NULL, 0, top, useDataBounds);
}

//----------------------------------------------------------------------------
// Squared distance from any point anywhere to the boundary of spatial region,
// and give me the point on the boundary closest to this point.
//
double vtkOctreePointLocatorNode::GetDistance2ToBoundary(
  double x, double y, double z, double *p,
  vtkOctreePointLocatorNode* top, int useDataBounds=0)
{
  return this->_GetDistance2ToBoundary(x, y, z, p, 0, top, useDataBounds);
}

//----------------------------------------------------------------------------
// The point is inside the region, and I want the distance (squared)
// to the closest "interior" wall, one that is not an outer boundary of
// the entire space.
//
double vtkOctreePointLocatorNode::GetDistance2ToInnerBoundary(
  double x, double y, double z, vtkOctreePointLocatorNode* top)
{
  return this->_GetDistance2ToBoundary(x, y, z, NULL, 1, top, 0);
}

//----------------------------------------------------------------------------
double vtkOctreePointLocatorNode::_GetDistance2ToBoundary(
  double x, double y, double z,  // from this point
  double *p,              // set to point on boundary that is closest
  int innerBoundaryOnly, // ignore boundaries on "outside"
  vtkOctreePointLocatorNode* top,    // the top of the octree
  int useDataBounds=0)   // use bounds of data within region instead
{
  double minDistance, dist;
  double edgePt[3];
  double cornerPt[3];
  double pt3[3];

  double *min, *max;

  if (useDataBounds)
  {
    min = this->MinDataBounds; max = this->MaxDataBounds;  // data inside region
  }
  else
  {
    min = this->MinBounds; max = this->MaxBounds;   // region itself
  }

  double *outerBoundaryMin=NULL;
  double *outerBoundaryMax=NULL;

  if (innerBoundaryOnly)
  {
    // We want the distance to the nearest inner boundary, because we
    // are only interested in boundaries such that there may be points on
    // the other side.  This option only makes sense when the point supplied
    // is inside this node (region).

    outerBoundaryMin = (useDataBounds? top->MinDataBounds : top->MinBounds);
    outerBoundaryMax = (useDataBounds? top->MaxDataBounds : top->MaxBounds);
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

      dist = x - xmin;
      if ((xmin != outerBoundaryMin[0]) &&
          ((dist < minDistance) || first))
      {
        mindim = 0;
        minDistance = dist;
        first = 0;
      }
      dist = xmax - x;
      if ((xmax != outerBoundaryMax[0]) &&
          ((dist < minDistance) || first))
      {
        mindim = 1;
        minDistance = dist;
        first = 0;
      }
      dist = y - ymin;
      if ((ymin != outerBoundaryMin[1]) &&
          ((dist < minDistance) || first))
      {
        mindim = 2;
        minDistance = dist;
        first = 0;
      }
      dist = ymax - y;
      if ((ymax != outerBoundaryMax[1]) &&
          ((dist < minDistance) || first))
      {
        mindim = 3;
        minDistance = dist;
        first = 0;
      }
      dist = z - zmin;
      if ((zmin != outerBoundaryMin[2]) &&
          ((dist < minDistance) || first))
      {
        mindim = 4;
        minDistance = dist;
        first = 0;
      }
      dist = zmax - z;
      if ((zmax != outerBoundaryMax[2]) &&
          ((dist < minDistance) || first))
      {
        mindim = 5;
        minDistance = dist;
      }
    }

    // if there are no inner boundaries we don't want to square.
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

//----------------------------------------------------------------------------
int vtkOctreePointLocatorNode::ContainsPoint(double x, double y, double z,
                                 int useDataBounds=0)
{
  double *min, *max;

  if (useDataBounds)
  {
    min = this->MinDataBounds;
    max = this->MaxDataBounds;
  }
  else
  {
    min = this->MinBounds;
    max = this->MaxBounds;
  }

  if( (min[0] >= x) ||
      (max[0] < x) ||
      (min[1] >= y) ||
      (max[1] < y) ||
      (min[2] >= z) ||
      (max[2] < z))
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkOctreePointLocatorNode::IntersectsRegion(
  vtkPlanesIntersection *pi, int useDataBounds)
{
  double xMin, xMax, yMin, yMax, zMin, zMax;
  vtkPoints *box = vtkPoints::New();

  box->SetNumberOfPoints(8);

  double *min, *max;

  if (useDataBounds)
  {
    min = this->MinDataBounds;
    max = this->MaxDataBounds;
  }
  else
  {
    min = this->MinBounds;
    max = this->MaxBounds;
  }

  xMin = min[0]; xMax = max[0];
  yMin = min[1]; yMax = max[1];
  zMin = min[2]; zMax = max[2];

  box->SetPoint(0, xMax, yMin, zMax);
  box->SetPoint(1, xMax, yMin, zMin);
  box->SetPoint(2, xMax, yMax, zMin);
  box->SetPoint(3, xMax, yMax, zMax);
  box->SetPoint(4, xMin, yMin, zMax);
  box->SetPoint(5, xMin, yMin, zMin);
  box->SetPoint(6, xMin, yMax, zMin);
  box->SetPoint(7, xMin, yMax, zMax);

  int intersects = pi->IntersectsRegion(box);

  box->Delete();

  return intersects;
}

//----------------------------------------------------------------------------
void vtkOctreePointLocatorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "Children: " << this->Children << endl;
  os << indent << "ID: " << this->ID << endl;
  os << indent << "MinID: " << this->MinID << endl;
  os << indent << "MinBounds: " << this->MinBounds[0] << " "
     << this->MinBounds[1] << " " << this->MinBounds[2] << endl;
  os << indent << "MaxBounds: " << this->MaxBounds[0] << " "
     << this->MaxBounds[1] << " " << this->MaxBounds[2] << endl;
  os << indent << "MinDataBounds: " << this->MinDataBounds[0] << " "
     << this->MinDataBounds[1] << " " << this->MinDataBounds[2] << endl;
  os << indent << "MaxDataBounds: " << this->MaxDataBounds[0] << " "
     << this->MaxDataBounds[1] << " " << this->MaxDataBounds[2] << endl;
}
