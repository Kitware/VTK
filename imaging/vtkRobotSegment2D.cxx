/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobotSegment2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkRobotSegment2D.h"

//----------------------------------------------------------------------------
vtkRobotSegment2D::vtkRobotSegment2D()
{
  this->PointA[0] = 0.0;
  this->PointA[1] = 0.0;
  this->PointB[0] = 0.0;
  this->PointB[1] = 0.0;
}


//----------------------------------------------------------------------------
vtkRobotSegment2D::~vtkRobotSegment2D()
{
}




//----------------------------------------------------------------------------
void vtkRobotSegment2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkRobot2D::PrintSelf(os, indent);
  os << indent << "PointA: " << this->PointA[0] << ", " << this->PointA[1] 
     << "\n";
  os << indent << "PointB: " << this->PointB[0] << ", " << this->PointB[1] 
     << "\n";
}




//----------------------------------------------------------------------------
// Description:
// Translate the robot (X, Y, sin(Theta), cos(Theta)), and then draw it.
void vtkRobotSegment2D::TransformDraw(float x, float y, float s, float c,
				      vtkImageDraw *canvas)
{
  int xa, ya;
  int xb, yb;
  
  // Apply the transform to the two points
  xa = (int)(floor(c * this->PointA[0] - s * this->PointA[1] + x + 0.5));
  ya = (int)(floor(s * this->PointA[0] + c * this->PointA[1] + y + 0.5));
  xb = (int)(floor(c * this->PointB[0] - s * this->PointB[1] + x + 0.5));
  yb = (int)(floor(s * this->PointB[0] + c * this->PointB[1] + y + 0.5));
  
  // Draw segment.
  canvas->DrawSegment(xa, ya, xb, yb);
}


//----------------------------------------------------------------------------
// Description:
// Bounds of the segment
void vtkRobotSegment2D::GetBounds(float bounds[4])
{
  bounds[0] = (this->PointA[0] < this->PointB[0]) ? 
    this->PointA[0] : this->PointB[0];
  bounds[1] = (this->PointA[0] > this->PointB[0]) ? 
    this->PointA[0] : this->PointB[0];
  bounds[2] = (this->PointA[1] < this->PointB[1]) ? 
    this->PointA[1] : this->PointB[1];
  bounds[3] = (this->PointA[1] > this->PointB[1]) ? 
    this->PointA[1] : this->PointB[1];
}








//----------------------------------------------------------------------------
int vtkRobotSegment2D::TransformCollide(vtkImageRegion *distanceMap,
					float tx, float ty, float s, float c)
{
  float x0,y0, x1,y1;
  short *map;
  short d0, d1;
  float length;
  int *extent;
  int xInc, yInc;
  int x, y;
  
  
  // Apply the transform to the two points
  x0 = (int)(floor(c * this->PointA[0] - s * this->PointA[1] + tx + 0.5));
  y0 = (int)(floor(s * this->PointA[0] + c * this->PointA[1] + ty + 0.5));
  x1 = (int)(floor(c * this->PointB[0] - s * this->PointB[1] + tx + 0.5));
  y1 = (int)(floor(s * this->PointB[0] + c * this->PointB[1] + ty + 0.5));
  
  extent = distanceMap->GetExtent();
  distanceMap->GetIncrements(xInc, yInc);
  map = (short *)(distanceMap->GetScalarPointer());

  // Do the initial check
  length = fabs(x1-x0) + fabs(y1-y0);
  x = (int)(floor(x0 + 0.5));
  y = (int)(floor(y0 + 0.5));
  // Make sure the point is in the map
  if (x < extent[0] || x > extent[1] || y < extent[2] || y > extent[3])
    {
    return 1;
    }
  d0 = *(map + x*xInc + y*yInc);
  x = (int)(floor(x1 + 0.5));
  y = (int)(floor(y1 + 0.5));
  // Make sure the point is in the map
  if (x < extent[0] || x > extent[1] || y < extent[2] || y > extent[3])
    {
    return 1;
    }
  d1 = *(map + x*xInc + y*yInc);
  // check for imediate collision
  if (d0 == 0 || d1 == 0)
    {
    return 1;
    }
  // do not call recursive collide if we have wide clearance
  if ((length >= (float)(d0)-0.5) || (length >= (float)(d1)-0.5))
    {
    // Call recursive segment collide
    if (this->CollideSegment(x0,y0,d0, x1,y1,d1, length, map, xInc, yInc))
      {
      return 1;
      }
    }
  return 0;
}




//----------------------------------------------------------------------------
// This method determines collision space from free space
int vtkRobotSegment2D::CollideSegment(float x0, float y0, short d0,
				      float x1, float y1, short d1,
				      float length, short *map, 
				      int xInc, int yInc)
{
  float xMid, yMid;
  int x, y;
  short dMid;
  
  // Find the middle of the segment
  xMid = (x0 + x1) / 2.0;
  yMid = (y0 + y1) / 2.0;
  x = (int)(floor(xMid + 0.5));
  y = (int)(floor(yMid + 0.5));
  dMid = *(map + x*xInc + y*yInc);
  length *= 0.5;

  // check for imediate collision
  if (dMid == 0)
    {
    return 1;
    }
  // check for wide clearance.
  if (length < d0-1 && length < d1-1 && length < dMid-1)
    {
    return 0;
    }

  // Call recursive segment collide
  if ((length >= (float)(d0)-0.5) || (length >= (float)(dMid)-0.5))
    {
    if (this->CollideSegment(x0,y0,d0, xMid,yMid,dMid, length, map, xInc,yInc))
      {
      return 1;
      }
    }

  if ((length >= (float)(d1)-0.5) || (length >= (float)(dMid)-0.5))
    {
    if (this->CollideSegment(xMid,yMid,dMid, x1,y1,d1, length, map, xInc,yInc))
      {
      return 1;
      }
    }

  return 0;
}

  












