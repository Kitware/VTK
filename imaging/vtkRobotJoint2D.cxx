/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobotJoint2D.cxx
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

#include "vtkRobotJoint2D.h"

//----------------------------------------------------------------------------
vtkRobotJoint2D::vtkRobotJoint2D()
{
  this->Theta = 0.0;
  this->Pivot[0] = this->Pivot[1] = 0.0;
  this->RobotA = NULL;
  this->RobotB = NULL;
}


//----------------------------------------------------------------------------
vtkRobotJoint2D::~vtkRobotJoint2D()
{
  if (this->RobotA)
    {
    this->RobotA->Delete();
    }
  
  if (this->RobotB)
    {
    this->RobotB->Delete();
    }
}




//----------------------------------------------------------------------------
void vtkRobotJoint2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkRobot2D::PrintSelf(os, indent);
  os << indent << "Pivot: " << this->Pivot[0] << ", " << this->Pivot[1]<< "\n";
  os << indent << "Theta: " << this->Theta << "\n";
  os << indent << "RobotA:\n";
  this->RobotA->PrintSelf(os, indent.GetNextIndent());
  os << indent << "RobotB:\n";
  this->RobotB->PrintSelf(os, indent.GetNextIndent());
}





//----------------------------------------------------------------------------
// Description:
// Translate the robot (X, Y, sin(Theta), cos(Theta)), and then draw it.
void vtkRobotJoint2D::TransformDraw(float x, float y, float s, float c,
				    vtkImageDraw *canvas)
{
  float st, ct;
  float sn, cn;
  float xt, yt;
  float xn, yn;

  // Rotate the transform
  st = sin(this->Theta);
  ct = cos(this->Theta);
  cn = ct * c - st * s;
  sn = st * c + ct * s;
  
  // Find the new shift. (why not just use transform matrix?)
  // first find the transformed pivot point.
  xt = c * this->Pivot[0] - s * this->Pivot[1];
  yt = s * this->Pivot[0] + c * this->Pivot[1];
  // rotate pivot point (around the origin)
  xn = ct * xt - st * yt;
  yn = st * xt + ct * yt;
  // Remove the shift.
  xn = x + xt - xn;
  yn = y + yt - yn;
  
  // Draw the robot with the new transform.
  this->RobotA->TransformDraw(x, y, s, c, canvas);
  this->RobotB->TransformDraw(xn, yn, sn, cn, canvas);
}


//----------------------------------------------------------------------------
// Description:
// Returns the bounds of the two when theta = 0.
void vtkRobotJoint2D::GetBounds(float bounds[4])
{
  float temp[4];
  
  this->RobotA->GetBounds(bounds);
  this->RobotB->GetBounds(temp);
  bounds[0] = (bounds[0] < temp[0]) ? bounds[0] : temp[0];
  bounds[1] = (bounds[1] > temp[1]) ? bounds[1] : temp[1];
  bounds[2] = (bounds[2] < temp[2]) ? bounds[2] : temp[2];
  bounds[3] = (bounds[3] > temp[3]) ? bounds[3] : temp[3];
}

  
//----------------------------------------------------------------------------
// Description:
// Returns 1 if the robot is in collision.  each pixel of distanceMap should
// contain the distance to a boundary.  (Manhatten distance).
int vtkRobotJoint2D::TransformCollide(vtkImageRegion *distanceMap,
				      float x, float y, float s, float c)
{
  float st, ct;
  float sn, cn;
  float xt, yt;
  float xn, yn;

  // Rotate the transform
  st = sin(this->Theta);
  ct = cos(this->Theta);
  cn = ct * c - st * s;
  sn = st * c + ct * s;
  
  // Find the new shift. (why not just use transform matrix?)
  // first find the transformed pivot point.
  xt = c * this->Pivot[0] - s * this->Pivot[1];
  yt = s * this->Pivot[0] + c * this->Pivot[1];
  // rotate pivot point (around the origin)
  xn = ct * xt - st * yt;
  yn = st * xt + ct * yt;
  // Remove the shift.
  xn = x + xt - xn;
  yn = y + yt - yn;
  
  if (this->RobotA->TransformCollide(distanceMap, x, y, s, c))
    {
    return 1;
    }
  return this->RobotB->TransformCollide(distanceMap, xn, yn, sn, cn);
}

  









