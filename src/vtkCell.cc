/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkCell.hh"

// Description:
// Construct cell.
vtkCell::vtkCell():
Points(VTK_CELL_SIZE), PointIds(VTK_CELL_SIZE)
{
  this->Points.ReferenceCountingOff();
}  

//
// Instantiate cell from outside
//
void vtkCell::Initialize(int npts, int *pts, vtkPoints *p)
{
  this->PointIds.Reset();
  for (int i=0; i<npts; i++)
    {
    this->PointIds.InsertId(i,pts[i]);
    this->Points.SetPoint(i,p->GetPoint(pts[i]));
    }
}
 
#define VTK_RIGHT 0
#define VTK_LEFT 1
#define VTK_MIDDLE 2

// Description:
// Bounding box intersection modified from Graphics Gems Vol I.
// Note: the intersection ray is assumed normalized, such that
// valid intersections can only occur between [0,1]. Method returns non-zero
// value if bounding box is hit. Origin[3] starts the ray, dir[3] is the 
// components of the ray in the x-y-z directions, coord[3] is the location 
// of hit, and t is the parametric coordinate along line.
char vtkCell::HitBBox (float bounds[6], float origin[3], float dir[3], 
                      float coord[3], float& t)
{
  char    inside=1;
  char    quadrant[3];
  int     i, whichPlane=0;
  float   maxT[3], candidatePlane[3];
//
//  First find closest planes
//
  for (i=0; i<3; i++) 
    {
    if ( origin[i] < bounds[2*i] ) 
      {
      quadrant[i] = VTK_LEFT;
      candidatePlane[i] = bounds[2*i];
      inside = 0;
      }
    else if ( origin[i] > bounds[2*i+1] ) 
      {
      quadrant[i] = VTK_RIGHT;
      candidatePlane[i] = bounds[2*i+1];
      inside = 0;
      }
    else 
      {
      quadrant[i] = VTK_MIDDLE;
      }
    }
//
//  Check whether origin of ray is inside bbox
//
  if (inside) 
    {
    coord[0] = origin[0];
    coord[1] = origin[1];
    coord[2] = origin[2];
    t = 0;
    return 1;
    }
  
//
//  Calculate parametric distances to plane
//
  for (i=0; i<3; i++)
    {
    if ( quadrant[i] != VTK_MIDDLE && dir[i] != 0.0 )
      {
      maxT[i] = (candidatePlane[i]-origin[i]) / dir[i];
      }
    else
      {
      maxT[i] = -1.0;
      }
    }
//
//  Find the largest parametric value of intersection
//
  for (i=0; i<3; i++)
    if ( maxT[whichPlane] < maxT[i] )
      whichPlane = i;
//
//  Check for valid intersection along line
//
  if ( maxT[whichPlane] > 1.0 || maxT[whichPlane] < 0.0 )
    return 0;
  else
    t = maxT[whichPlane];
//
//  Intersection point along line is okay.  Check bbox.
//
  for (i=0; i<3; i++) 
    {
    if (whichPlane != i) 
      {
      coord[i] = origin[i] + maxT[whichPlane]*dir[i];
      if ( coord[i] < bounds[2*i] || coord[i] > bounds[2*i+1] )
        return 0;
      } 
    else 
      {
      coord[i] = candidatePlane[i];
      }
    }

    return 1;
}

// Description:
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Return pointer
// to array of six float values.
float *vtkCell::GetBounds ()
{
  float *x;
  int i, j;
  static float bounds[6];

  bounds[0] = bounds[2] = bounds[4] =  VTK_LARGE_FLOAT;
  bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;

  for (i=0; i<this->Points.GetNumberOfPoints(); i++)
    {
    x = this->Points.GetPoint(i);
    for (j=0; j<3; j++)
      {
      if ( x[j] < bounds[2*j] ) bounds[2*j] = x[j];
      if ( x[j] > bounds[2*j+1] ) bounds[2*j+1] = x[j];
      }
    }
  return bounds;
}

// Description:
// Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Copy result into
// user provided array.
void vtkCell::GetBounds(float bounds[6])
{
  float *b=this->GetBounds();
  for (int i=0; i < 6; i++) bounds[i] = b[i];
}

// Description:
// Compute Length squared of cell (i.e., bounding box diagonal squared).
float vtkCell::GetLength2 ()
{
  float diff, l=0.0;
  float *bounds;
  int i;

  bounds = this->GetBounds();

  for (i=0; i<3; i++)
    {
    diff = bounds[2*i+1] - bounds[2*i];
    l += diff * diff;
    }
 
  return l;
}

void vtkCell::PrintSelf(ostream& os, vtkIndent indent)
{
  float *bounds;
  int i;

  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->PointIds.GetNumberOfIds() << "\n";
  bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";

  os << indent << "  Point ids are: ";
  for (i=0; this->PointIds.GetNumberOfIds(); i++)
    {
    os << ", " << this->PointIds.GetId(i);
    if ( i && !(i % 12) ) os << "\n\t";
    }
  os << indent << "\n";
}
