/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobotTransform2D.cxx
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

#include "vtkRobotTransform2D.h"

//----------------------------------------------------------------------------
vtkRobotTransform2D::vtkRobotTransform2D()
{
  this->Robot = NULL;
  this->X = 0.0;
  this->Y = 0.0;
  this->Theta = 0.0;
}


//----------------------------------------------------------------------------
vtkRobotTransform2D::~vtkRobotTransform2D()
{
  if (this->Robot)
    {
    this->Robot->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkRobotTransform2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkRobot2D::PrintSelf(os, indent);
  os << indent << "X: " << this->X << "\n";
  os << indent << "Y: " << this->Y << "\n";
  os << indent << "Theta: " << this->Theta << "\n";
  os << indent << "Robot:\n";
  this->Robot->PrintSelf(os, indent.GetNextIndent());
}



//----------------------------------------------------------------------------
// Description:
// Translate the robot (X, Y, sin(Theta), cos(Theta)), and then draw it.
void vtkRobotTransform2D::TransformDraw(float x, float y, float s, float c,
					vtkImagePaint *canvas)
{
  float st, ct;
  float xn, yn;
  float sn, cn;
  
  // Rotate the transform
  st = sin(this->Theta);
  ct = cos(this->Theta);
  cn = ct * c - st * s;
  sn = st * c + ct * s;
  xn = ct * x - st * y;
  yn = st * x + ct * y;
  
  // Shift the transform.
  xn += this->X;
  yn += this->Y;
  
  // Draw the robot with the new transform.
  this->Robot->TransformDraw(xn, yn, sn, cn, canvas);
}










//----------------------------------------------------------------------------
// Description:
// Returns the bounds of the robot without the transform.
void vtkRobotTransform2D::GetBounds(float bounds[4])
{
  this->Robot->GetBounds(bounds);
}

  
//----------------------------------------------------------------------------
// Description:
// Returns 1 if the robot is in collision.  Each pixel of distanceMap should
// contain the distance to a boundary.  (Manhatten distance).
int vtkRobotTransform2D::TransformCollide(vtkImageRegion *distanceMap, 
					  float x, float y, float s, float c)
{
  float st, ct;
  float xn, yn;
  float sn, cn;
  
  // Rotate the transform
  st = sin(this->Theta);
  ct = cos(this->Theta);
  cn = ct * c - st * s;
  sn = st * c + ct * s;
  xn = ct * x - st * y;
  yn = st * x + ct * y;
  
  // Shift the transform.
  xn += this->X;
  yn += this->Y;
  
  // Collide the robot with the new transform.
  return this->Robot->TransformCollide(distanceMap, xn, yn, sn, cn);
}

  









