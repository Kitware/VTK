/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.cxx
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
#include "vtkPlane.h"
#include "vtkMath.h"


// Description
// Construct plane passing through origin and normal to z-axis.
vtkPlane::vtkPlane()
{
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

// Description
// Project a point x onto plane defined by origin and normal. The 
// projected point is returned in xproj. NOTE : normal assumed to
// have magnitude 1.
void vtkPlane::ProjectPoint(float x[3], float origin[3], float normal[3], float xproj[3])
{
  int i;
  float t, xo[3];

  for (i=0; i<3; i++) xo[i] = x[i] - origin[i];
  t = vtkMath::Dot(normal,xo);
  for (i=0; i<3; i++) xproj[i] = x[i] - t * normal[i];
}

// Description
// Evaluate plane equation for point x[3].
float vtkPlane::EvaluateFunction(float x[3])
{
  return ( this->Normal[0]*(x[0]-this->Origin[0]) + 
           this->Normal[1]*(x[1]-this->Origin[1]) + 
           this->Normal[2]*(x[2]-this->Origin[2]) );
}

// Description
// Evaluate function gradient at point x[3].
void vtkPlane::EvaluateGradient(float vtkNotUsed(x)[3], float n[3])
{
  for (int i=0; i<3; i++) n[i] = this->Normal[i];
}

#define TOL 1.0e-06

// Description:
// Given a line defined by the two points p1,p2; and a plane defined by the
// normal n and point p0, compute an intersection. The parametric
// coordinate along the line is returned in t, and the coordinates of 
// intersection are returned in x. A zero is returned if the plane and line
// are parallel.
int vtkPlane::IntersectWithLine(float p1[3], float p2[3], float n[3], 
                               float p0[3], float& t, float x[3])
{
  float num, den, p21[3];
  int i;
//
// Compute line vector
// 
  for (i=0; i<3; i++) p21[i] = p2[i] - p1[i];
//
// Compute denominator.  If ~0, line and plane are parallel.
// 
  num = vtkMath::Dot(n,p0) - ( n[0]*p1[0] + n[1]*p1[1] + n[2]*p1[2] ) ;
  den = n[0]*p21[0] + n[1]*p21[1] + n[2]*p21[2];
//
// If denominator with respect to numerator is "zero", then the line and
// plane are considered parallel. 
//
  if ( fabs(den) <= fabs(TOL*num)) return 0;

  t = num / den;
  for (i=0; i<3; i++) x[i] = p1[i] + t*p21[i];

  if ( t >= 0.0 && t <= 1.0 ) return 1;
  else return 0;
}

void vtkPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Normal: (" << this->Normal[0] << ", " 
    << this->Normal[1] << ", " << this->Normal[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", " 
    << this->Origin[1] << ", " << this->Origin[2] << ")\n";
}
