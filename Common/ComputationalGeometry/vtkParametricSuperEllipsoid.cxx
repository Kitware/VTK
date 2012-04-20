/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricSuperEllipsoid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricSuperEllipsoid.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include <math.h>

vtkStandardNewMacro(vtkParametricSuperEllipsoid);

//----------------------------------------------------------------------------
vtkParametricSuperEllipsoid::vtkParametricSuperEllipsoid() :
  XRadius(1)
  , YRadius(1)
  , ZRadius(1)
  , N1(1)
  , N2(1)
{
  this->MinimumU = 0;
  this->MinimumV = 0;
  this->MaximumV = vtkMath::Pi();
  this->MaximumU = 2.0*vtkMath::Pi();

  this->JoinU = 1;
  this->JoinV = 0;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 1;
  this->DerivativesAvailable = 0;
}

//----------------------------------------------------------------------------
vtkParametricSuperEllipsoid::~vtkParametricSuperEllipsoid()
{
}

//----------------------------------------------------------------------------
void vtkParametricSuperEllipsoid::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  for ( int i = 0; i < 3; ++i)
    {
    Pt[i] = Du[i] = Dv[i] = 0;
    }

  double cu = cos(u);
  double su = sin(u);
  double cv = cos(v);
  double sv = sin(v);

  double tmp  = this->Power(sv,N1);

  // The point
  Pt[0] = this->XRadius * tmp * this->Power(cu,this->N2);
  Pt[1] = this->YRadius * tmp * this->Power(su,this->N2);
  Pt[2] = this->ZRadius * this->Power(cv,this->N1);
}

//----------------------------------------------------------------------------
double vtkParametricSuperEllipsoid::EvaluateScalar(double*, double*, double*)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricSuperEllipsoid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "X scale factor: " << this->XRadius << "\n";
  os << indent << "Y scale factor: " << this->YRadius << "\n";
  os << indent << "Z scale factor: " << this->ZRadius << "\n";
  os << indent << "Squareness in the z-axis: " << this->N1 << "\n";
  os << indent << "Squareness in the x-y plane: " << this->N2 << "\n";
}

//----------------------------------------------------------------------------
double vtkParametricSuperEllipsoid::Power ( double x, double n )
{
  if ( x == 0 )
    {
    return 0;
    }
  else if ( x < 0 )
    {
    return -pow(-x,n);
    }
  else
    {
    return pow(x,n);
    }
}
