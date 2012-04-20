/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoundingBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoundingBox.h"
#include <vtkMath.h>
#include <vtkPlane.h>
#include <assert.h>
#include <math.h>

// ---------------------------------------------------------------------------
namespace
{
  inline double Sign(const double& a)
  {
    return a>0.0?  1.0 : ( a<0? -1.0 : 0.0);
  }
  inline bool OppSign(const double& a, const double& b)
  {
    return (a<=0 && b>=0) || (a>=0 && b<=0);
  }
};

// ---------------------------------------------------------------------------
void vtkBoundingBox::AddPoint(double px, double py, double pz)
{
  double p[3];
  p[0] = px;
  p[1] = py;
  p[2] = pz;
  this->AddPoint(p);
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::AddPoint(double p[3])
{
  int i;
  for (i = 0; i < 3; i++)
    {
    if (p[i] < this->MinPnt[i])
      {
      this->MinPnt[i] = p[i];
      }

    if (p[i] > this->MaxPnt[i])
      {
      this->MaxPnt[i] = p[i];
      }
    }
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::AddBox(const vtkBoundingBox &bbox)
{
  double bds[6];
  bbox.GetBounds(bds);
  this->AddBounds(bds);
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::AddBounds(double bounds[6])
{
  bool this_valid = (this->IsValid() != 0);
  bool other_valid = (vtkBoundingBox::IsValid(bounds) != 0);

  if (!other_valid)
    {
    return;
    }

  if (other_valid && !this_valid)
    {
    this->SetBounds(bounds);
    return;
    }

  if (bounds[0] < this->MinPnt[0])
    {
    this->MinPnt[0] = bounds[0];
    }

  if (bounds[1] > this->MaxPnt[0])
    {
    this->MaxPnt[0] = bounds[1];
    }

  if (bounds[2] < this->MinPnt[1])
    {
    this->MinPnt[1] = bounds[2];
    }

  if (bounds[3] > this->MaxPnt[1])
    {
    this->MaxPnt[1] = bounds[3];
    }

  if (bounds[4] < this->MinPnt[2])
    {
    this->MinPnt[2] = bounds[4];
    }

  if (bounds[5] > this->MaxPnt[2])
    {
    this->MaxPnt[2] = bounds[5];
    }
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::SetBounds(double xMin, double xMax,
                              double yMin, double yMax,
                              double zMin, double zMax)
{
  this->MinPnt[0] = xMin;
  this->MaxPnt[0] = xMax;
  this->MinPnt[1] = yMin;
  this->MaxPnt[1] = yMax;
  this->MinPnt[2] = zMin;
  this->MaxPnt[2] = zMax;

  int i;
  for (i = 0; i < 3; i++)
    {
    if (this->MaxPnt[i] < this->MinPnt[i])
      {
      this->MaxPnt[i] = this->MinPnt[i];
      }
    }
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::SetMinPoint(double x, double y, double z)
{
  this->MinPnt[0] = x;
  if (x > this->MaxPnt[0])
    {
    this->MaxPnt[0] = x;
    }

  this->MinPnt[1] = y;
  if (y > this->MaxPnt[1])
    {
    this->MaxPnt[1] = y;
    }

  this->MinPnt[2] = z;
  if (z > this->MaxPnt[2])
    {
    this->MaxPnt[2] = z;
    }

}

// ---------------------------------------------------------------------------
void vtkBoundingBox::SetMaxPoint(double x, double y, double z)
{
  this->MaxPnt[0] = x;
  if (x < this->MinPnt[0])
    {
    this->MinPnt[0] = x;
    }

  this->MaxPnt[1] = y;
  if (y < this->MinPnt[1])
    {
    this->MinPnt[1] = y;
    }

  this->MaxPnt[2] = z;
  if (z < this->MinPnt[2])
    {
    this->MinPnt[2] = z;
    }
}
// ---------------------------------------------------------------------------
void vtkBoundingBox::Inflate(double delta)
{
  this->MinPnt[0] -= delta;
  this->MaxPnt[0] += delta;
  this->MinPnt[1] -= delta;
  this->MaxPnt[1] += delta;
  this->MinPnt[2] -= delta;
  this->MaxPnt[2] += delta;
}
// ---------------------------------------------------------------------------
int vtkBoundingBox::IntersectBox(const vtkBoundingBox &bbox)
{
  // if either box is not valid don't do the opperation
  if (!(this->IsValid() && bbox.IsValid()))
    {
    return 0;
    }

  int i, intersects;
  double pMin[3], pMax[3];
  for (i = 0; i < 3; i++)
    {
    intersects = 0;
    if ((bbox.MinPnt[i] >= this->MinPnt[i]) &&
        (bbox.MinPnt[i] <= this->MaxPnt[i]))
      {
      intersects = 1;
      pMin[i] = bbox.MinPnt[i];
      }
    else if ((this->MinPnt[i] >= bbox.MinPnt[i]) &&
             (this->MinPnt[i] <= bbox.MaxPnt[i]))
      {
      intersects = 1;
      pMin[i] = this->MinPnt[i];
      }
    if ((bbox.MaxPnt[i] >= this->MinPnt[i]) &&
        (bbox.MaxPnt[i] <= this->MaxPnt[i]))
      {
      intersects = 1;
      pMax[i] = bbox.MaxPnt[i];
      }
    else if ((this->MaxPnt[i] >= bbox.MinPnt[i]) &&
             (this->MaxPnt[i] <= bbox.MaxPnt[i]))
      {
      intersects = 1;
      pMax[i] = this->MaxPnt[i];
      }
    if (!intersects)
      {
      return 0;
      }
    }

  // OK they did intersect - set the box to be the result
  for (i = 0; i < 3; i++)
    {
    this->MinPnt[i] = pMin[i];
    this->MaxPnt[i] = pMax[i];
    }
  return 1;
}

// ---------------------------------------------------------------------------
int vtkBoundingBox::Intersects(const vtkBoundingBox &bbox) const
{
  // if either box is not valid they don't intersect
  if (!(this->IsValid() && bbox.IsValid()))
    {
    return 0;
    }
  int i;
  for (i = 0; i < 3; i++)
    {
    if ((bbox.MinPnt[i] >= this->MinPnt[i]) &&
        (bbox.MinPnt[i] <= this->MaxPnt[i]))
      {
      continue;
      }
    if ((this->MinPnt[i] >= bbox.MinPnt[i]) &&
             (this->MinPnt[i] <= bbox.MaxPnt[i]))
      {
      continue;
      }
    if ((bbox.MaxPnt[i] >= this->MinPnt[i]) &&
        (bbox.MaxPnt[i] <= this->MaxPnt[i]))
      {
      continue;
      }
    if ((this->MaxPnt[i] >= bbox.MinPnt[i]) &&
             (this->MaxPnt[i] <= bbox.MaxPnt[i]))
      {
      continue;
      }
    return 0;
    }
  return 1;
}

// ---------------------------------------------------------------------------
int vtkBoundingBox::Contains(const vtkBoundingBox &bbox) const
{
  // if either box is not valid or they don't intersect
  if (!this->Intersects(bbox))
    {
    return 0;
    }
  const double* pt = bbox.GetMinPoint();
  if (!this->ContainsPoint(pt[0],pt[1],pt[2]))
    {
    return 0;
    }
  pt = bbox.GetMaxPoint();
  if (!this->ContainsPoint(pt[0],pt[1],pt[2]))
    {
    return 0;
    }
  return 1;
}

// ---------------------------------------------------------------------------
double vtkBoundingBox::GetMaxLength() const
{
  double l[3];
  this->GetLengths(l);
  if (l[0] > l[1])
    {
    if (l[0] > l[2])
      {
      return l[0];
      }
    return l[2];
    }
  else if (l[1] > l[2])
    {
    return l[1];
    }
  return l[2];
}

// ---------------------------------------------------------------------------
double vtkBoundingBox::GetDiagonalLength() const
{
  assert("pre: not_empty" && this->IsValid());

  double l[3];
  this->GetLengths(l);

  return sqrt(l[0]*l[0]+l[1]*l[1]+l[2]*l[2]);
}

// ---------------------------------------------------------------------------
// Description:
// Scale each dimension of the box by some given factor.
// If the box is not valid, it stays unchanged.
// If the scalar factor is negative, bounds are flipped: for example,
// if (xMin,xMax)=(-2,4) and sx=-3, (xMin,xMax) becomes (-12,6).
void vtkBoundingBox::Scale(double sx,
                           double sy,
                           double sz)
{
  if(this->IsValid())
    {
      if(sx>=0.0)
        {
          this->MinPnt[0]*=sx;
          this->MaxPnt[0]*=sx;
        }
      else
        {
          double tmp=this->MinPnt[0];
          this->MinPnt[0]=sx*this->MaxPnt[0];
          this->MaxPnt[0]=sx*tmp;
        }
      if(sy>=0.0)
        {
          this->MinPnt[1]*=sy;
          this->MaxPnt[1]*=sy;
        }
      else
        {
          double tmp=this->MinPnt[1];
          this->MinPnt[1]=sy*this->MaxPnt[1];
          this->MaxPnt[1]=sy*tmp;
        }
      if(sz>=0.0)
        {
          this->MinPnt[2]*=sz;
          this->MaxPnt[2]*=sz;
        }
      else
        {
          double tmp=this->MinPnt[2];
          this->MinPnt[2]=sz*this->MaxPnt[2];
          this->MaxPnt[2]=sz*tmp;
        }
    }
}

// ---------------------------------------------------------------------------
void vtkBoundingBox::Scale(double s[3])
{
  this->Scale(s[0],s[1],s[2]);
}


// ---------------------------------------------------------------------------
// Desciption:
// Intersect this box with the half space defined by plane.
 //Returns 1 if there is intersection---which implies that the box has been modified
// Returns 0 otherwise
// The algorithm:
//   Because the change can only happens in one axis aligned direction,
//    we first figure out which direction it is (stored in dir), then
//    update the bounding interval in that direction based on intersection
//    of the plane with the four edges

bool vtkBoundingBox::IntersectPlane(double origin[3],double normal[3])
{
  double* bounds[2] = {this->MinPnt,this->MaxPnt};
  assert(this->IsValid());

  //Index[0..2] represents the order of traversing the corners of a cube
  //  in (x,y,z), (y,x,z) and (z,x,y) ordering, respectively
  static const int Index[3][8] =
    {  {0, 1, 2, 3, 4, 5, 6, 7},
       {0, 1, 4, 5, 2, 3, 6, 7},
       {0, 2, 4, 6, 1, 3, 5, 7}};


  double d[8]={0,0,0,0,0,0,0,0}; //stores the signed distance to a plane
    {
    int index(-1);
    for(int ix = 0; ix<=1; ix++)
      {
      for(int iy = 0; iy<=1; iy++)
        {
        for(int iz = 0; iz<=1; iz++)
          {
          double x[3]={bounds[ix][0],bounds[iy][1],bounds[iz][2]};
          d[++index] = vtkPlane::Evaluate(normal,origin,x);
          }
        }
      }
    }

  int dir(-1);
  for(dir=2;dir>=0; dir--)
    {
    //in each direction, we test if the vertices of two orthogonal faces
    //are on either side of the plane
    if( OppSign(d[Index[dir][0]], d[Index[dir][4]])  &&
        OppSign(d[Index[dir][1]], d[Index[dir][5]])  &&
        OppSign(d[Index[dir][2]], d[Index[dir][6]])  &&
        OppSign(d[Index[dir][3]], d[Index[dir][7]]) )
      {
      break;
      }
    }
  if(dir<0)
    {
      return false;
    }

  double sign = Sign(normal[dir]);
  double size = fabs((bounds[1][dir] - bounds[0][dir])*normal[dir]);
  double t = sign>0? 1 : 0;
  for(int i=0; i<4; i++)
    {
      if(size==0) continue;  //shouldn't happen
      double ti = fabs(d[Index[dir][i]])/ size;
      if(sign>0 && ti<t )
        {
        t = ti;
        }
      if(sign<0 && ti>t)
        {
        t = ti;
        }
    }
  double bound = (1.0-t)*bounds[0][dir] + t*bounds[1][dir];

  if(sign>0)
    {
    bounds[0][dir] = bound;
    }
  else
    {
    bounds[1][dir] = bound;
    }

  return true;
}
