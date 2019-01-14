/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSuperquadric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* vtkSuperQuadric originally written by Michael Halle,
   Brigham and Women's Hospital, July 1998.

   Based on "Rigid physically based superquadrics", A. H. Barr,
   in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
*/

#include "vtkSuperquadric.h"
#include "vtkObjectFactory.h"

#include <cmath>

vtkStandardNewMacro(vtkSuperquadric);

// Construct with superquadric radius of 0.5, toroidal off, center at 0.0,
// scale (1,1,1), size 0.5, phi roundness 1.0, and theta roundness 0.0.
vtkSuperquadric::vtkSuperquadric()
{
  this->Toroidal = 0;
  this->Thickness = 0.3333;
  this->PhiRoundness = 0.0;
  this->SetPhiRoundness(1.0);
  this->ThetaRoundness = 0.0;
  this->SetThetaRoundness(1.0);
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
  this->Size = .5;
}

static const double MAX_FVAL = 1e12;
static double VTK_MIN_SUPERQUADRIC_ROUNDNESS = 1e-24;

void vtkSuperquadric::SetThetaRoundness(double e)
{
  if(e < VTK_MIN_SUPERQUADRIC_ROUNDNESS)
  {
    e = VTK_MIN_SUPERQUADRIC_ROUNDNESS;
  }

  if (this->ThetaRoundness != e)
  {
    this->ThetaRoundness = e;
    this->Modified();
  }
}

void vtkSuperquadric::SetPhiRoundness(double e)
{
  if(e < VTK_MIN_SUPERQUADRIC_ROUNDNESS)
  {
    e = VTK_MIN_SUPERQUADRIC_ROUNDNESS;
  }

  if (this->PhiRoundness != e)
  {
    this->PhiRoundness = e;
    this->Modified();
  }
}

// Evaluate Superquadric equation
double vtkSuperquadric::EvaluateFunction(double xyz[3])
{
  double e = this->ThetaRoundness;
  double n = this->PhiRoundness;
  double p[3], s[3];
  double val;

  s[0] = this->Scale[0] * this->Size;
  s[1] = this->Scale[1] * this->Size;
  s[2] = this->Scale[2] * this->Size;

  if(this->Toroidal) {
    double tval;
    double alpha;

    alpha = (1.0 / this->Thickness);
    s[0] /= (alpha + 1.0);
    s[1] /= (alpha + 1.0);
    s[2] /= (alpha + 1.0);

    p[0] = (xyz[0] - this->Center[0]) / s[0];
    p[1] = (xyz[1] - this->Center[1]) / s[1];
    p[2] = (xyz[2] - this->Center[2]) / s[2];

    tval = pow((pow(fabs(p[2]), 2.0/e) + pow(fabs(p[0]), 2.0/e)), e/2.0);
    val  = pow(fabs(tval - alpha), 2.0/n) + pow(fabs(p[1]), 2.0/n) - 1.0;
  }
  else { // Ellipsoidal
    p[0] = (xyz[0] - this->Center[0]) / s[0];
    p[1] = (xyz[1] - this->Center[1]) / s[1];
    p[2] = (xyz[2] - this->Center[2]) / s[2];

    val = pow((pow(fabs(p[2]), 2.0/e) + pow(fabs(p[0]), 2.0/e)), e/n) +
      pow(fabs(p[1]),2.0/n) - 1.0;
  }

  if(val > MAX_FVAL){
    val = MAX_FVAL;
  }
  else if(val < -MAX_FVAL){
    val = -MAX_FVAL;
  }

  return val;
}

// Description
// Evaluate Superquadric function gradient.
void vtkSuperquadric::EvaluateGradient(double vtkNotUsed(xyz)[3], double g[3])
{
  // bogus! lazy!
  // if someone wants to figure these out, they are each the
  // partial of x, then y, then z with respect to f as shown above.
  // Careful for the fabs().

  g[0] = g[1] = g[2] = 0.0;
}

void vtkSuperquadric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Toroidal: " << (this->Toroidal ? "On\n" : "Off\n");
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "ThetaRoundness: " << this->ThetaRoundness << "\n";
  os << indent << "PhiRoundness: " << this->PhiRoundness << "\n";
  os << indent << "Center: ("
     << this->Center[0] << ", "
     << this->Center[1] << ", "
     << this->Center[2] << ")\n";
  os << indent << "Scale: ("
     << this->Scale[0] << ", "
     << this->Scale[1] << ", "
     << this->Scale[2] << ")\n";

}
