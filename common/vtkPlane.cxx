/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPlane.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPlane* vtkPlane::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPlane");
  if(ret)
    {
    return (vtkPlane*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPlane;
}





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

// Project a point x onto plane defined by origin and normal. The 
// projected point is returned in xproj. NOTE : normal assumed to
// have magnitude 1.
void vtkPlane::ProjectPoint(float x[3], float origin[3], float normal[3], float xproj[3])
{
  float t, xo[3];

  xo[0] = x[0] - origin[0];
  xo[1] = x[1] - origin[1];
  xo[2] = x[2] - origin[2];

  t = vtkMath::Dot(normal,xo);

  xproj[0] = x[0] - t * normal[0];
  xproj[1] = x[1] - t * normal[1];
  xproj[2] = x[2] - t * normal[2];
}

void vtkPlane::ProjectPoint(double x[3], double origin[3], double normal[3], double xproj[3])
{
  double t, xo[3];

  xo[0] = x[0] - origin[0];
  xo[1] = x[1] - origin[1];
  xo[2] = x[2] - origin[2];

  t = vtkMath::Dot(normal,xo);

  xproj[0] = x[0] - t * normal[0];
  xproj[1] = x[1] - t * normal[1];
  xproj[2] = x[2] - t * normal[2];
}

// Project a point x onto plane defined by origin and normal. The 
// projected point is returned in xproj. NOTE : normal NOT required to
// have magnitude 1.
void vtkPlane::GeneralizedProjectPoint(float x[3], float origin[3],
				       float normal[3], float xproj[3])
{
  float t, xo[3], n2;

  xo[0] = x[0] - origin[0];
  xo[1] = x[1] - origin[1];
  xo[2] = x[2] - origin[2];

  t = vtkMath::Dot(normal,xo);
  n2 = vtkMath::Dot(normal, normal);

  if (n2 != 0)
    {
    xproj[0] = x[0] - t * normal[0]/n2;
    xproj[1] = x[1] - t * normal[1]/n2;
    xproj[2] = x[2] - t * normal[2]/n2;
    }
  else
    {
    xproj[0] = x[0];
    xproj[1] = x[1];
    xproj[2] = x[2];
    }
}


// Evaluate plane equation for point x[3].
float vtkPlane::EvaluateFunction(float x[3])
{
  return ( this->Normal[0]*(x[0]-this->Origin[0]) + 
           this->Normal[1]*(x[1]-this->Origin[1]) + 
           this->Normal[2]*(x[2]-this->Origin[2]) );
}

// Evaluate function gradient at point x[3].
void vtkPlane::EvaluateGradient(float vtkNotUsed(x)[3], float n[3])
{
  for (int i=0; i<3; i++)
    {
    n[i] = this->Normal[i];
    }
}

#define VTK_PLANE_TOL 1.0e-06

// Given a line defined by the two points p1,p2; and a plane defined by the
// normal n and point p0, compute an intersection. The parametric
// coordinate along the line is returned in t, and the coordinates of 
// intersection are returned in x. A zero is returned if the plane and line
// are parallel.
int vtkPlane::IntersectWithLine(float p1[3], float p2[3], float n[3], 
                               float p0[3], float& t, float x[3])
{
  float num, den, p21[3];
  float fabsden, fabstolerance;
  //
  // Compute line vector
  // 
  p21[0] = p2[0] - p1[0];
  p21[1] = p2[1] - p1[1];
  p21[2] = p2[2] - p1[2];

  //
  // Compute denominator.  If ~0, line and plane are parallel.
  // 
  num = vtkMath::Dot(n,p0) - ( n[0]*p1[0] + n[1]*p1[1] + n[2]*p1[2] ) ;
  den = n[0]*p21[0] + n[1]*p21[1] + n[2]*p21[2];
  //
  // If denominator with respect to numerator is "zero", then the line and
  // plane are considered parallel. 
  //

  // trying to avoid an expensive call to fabs()
  if (den < 0.0)
    {
    fabsden = -den;
    }
  else
    {
    fabsden = den;
    }
  if (num < 0.0)
    {
    fabstolerance = -num*VTK_PLANE_TOL;
    }
  else
    {
    fabstolerance = num*VTK_PLANE_TOL;
    }
  if ( fabsden <= fabstolerance )
    {
    return 0;
    }

  t = num / den;

  x[0] = p1[0] + t*p21[0];
  x[1] = p1[1] + t*p21[1];
  x[2] = p1[2] + t*p21[2];

  if ( t >= 0.0 && t <= 1.0 )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

void vtkPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Normal: (" << this->Normal[0] << ", " 
    << this->Normal[1] << ", " << this->Normal[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", " 
    << this->Origin[1] << ", " << this->Origin[2] << ")\n";
}
