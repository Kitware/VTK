/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCylinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCylinder.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkCylinder);

//----------------------------------------------------------------------------
// Construct cylinder radius of 0.5.
vtkCylinder::vtkCylinder()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Axis[0] = 0.0;
  this->Axis[1] = 1.0;
  this->Axis[2] = 0.0;
  this->Radius = 0.5;
}

//----------------------------------------------------------------------------
// Evaluate cylinder equation F(x,y,z) along specified Axis. Note that this is
// basically a distance to line computation, compared to the cylinder radius.
double vtkCylinder::EvaluateFunction(double x[3])
{
  // Determine distance^2 of point to axis. Note that cylinder Axis is
  // always normalized and always non-zero.
  double x2C[3];
  x2C[0] = x[0] - this->Center[0];
  x2C[1] = x[1] - this->Center[1];
  x2C[2] = x[2] - this->Center[2];

  // projection onto cylinder axis
  double proj = vtkMath::Dot(this->Axis,x2C);

  // return distance^2 - R^2
  return ( (vtkMath::Dot(x2C,x2C) - proj*proj) - this->Radius*this->Radius );
}

//----------------------------------------------------------------------------
// Evaluate cylinder function gradient (along potentially oriented axis). The
// gradient is always in the radial direction, and thus must be projected
// onto the three x-y-z coordinate axes.
void vtkCylinder::EvaluateGradient(double x[3], double g[3])
{
  // Determine the radial vector from the point x to the line. This
  // means finding the closest point to the line. Get parametric
  // location along cylinder axis. Remember Axis is normalized.
  double t = this->Axis[0]*(x[0]-this->Center[0]) +
    this->Axis[1]*(x[1]-this->Center[1]) + this->Axis[2]*(x[2]-this->Center[2]);

  // Compute closest point
  double cp[3];
  cp[0] = this->Center[0] + t*this->Axis[0];
  cp[1] = this->Center[1] + t*this->Axis[1];
  cp[2] = this->Center[2] + t*this->Axis[2];

  // Gradient is 2*r. Project onto x-y-z axes.
  g[0] = 2.0 * (x[0] - cp[0]);
  g[1] = 2.0 * (x[1] - cp[1]);
  g[2] = 2.0 * (x[2] - cp[2]);
}

//----------------------------------------------------------------------------
// Specify the cylinder axis. Normalize if necessary.
void vtkCylinder::SetAxis(double ax, double ay, double az)
{
  double axis[3];
  axis[0] = ax;
  axis[1] = ay;
  axis[2] = az;
  this->SetAxis(axis);
}

//----------------------------------------------------------------------------
// Specify the cylinder axis. Reject non-zero axis vectors. It normalizes the
// axis vector.
void vtkCylinder::SetAxis(double a[3])
{
  // If axis length is zero, then don't change it
  if ( vtkMath::Normalize(a) < DBL_EPSILON )
  {
    return;
  }

  if ( a[0] != this->Axis[0] || a[1] != this->Axis[1] ||
       a[2] != this->Axis[2] )
  {
    this->Modified();
    this->Axis[0] = a[0];
    this->Axis[1] = a[1];
    this->Axis[2] = a[2];
  }
}

//----------------------------------------------------------------------------
void vtkCylinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Center: " << "( " << this->Center[0] << ", " <<
     this->Center[1] << ", " << this->Center[2] << " )";

  os << indent << "Axis: " << "( " << this->Axis[0] << ", " <<
     this->Axis[1] << ", " << this->Axis[2] << " )";

  os << indent << "Radius: " << this->Radius << "\n";
}
