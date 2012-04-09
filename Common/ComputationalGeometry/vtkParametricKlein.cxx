/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricKlein.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricKlein.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkParametricKlein);

//----------------------------------------------------------------------------
vtkParametricKlein::vtkParametricKlein()
{
  // Preset triangulation parameters
  this->MinimumU = 0;
  this->MinimumV = 0;
  this->MaximumU = vtkMath::Pi();
  this->MaximumV = 2.0 * vtkMath::Pi();

  this->JoinU = 0;
  this->JoinV = 1;
  this->TwistU = 0;
  this->TwistV = 0;
  this->ClockwiseOrdering = 1;
  this->DerivativesAvailable = 1;
}

//----------------------------------------------------------------------------
vtkParametricKlein::~vtkParametricKlein()
{
}

//----------------------------------------------------------------------------
void vtkParametricKlein::Evaluate(double uvw[3], double Pt[3], double Duvw[9])
{
  double u = uvw[0];
  double v = uvw[1];
  double *Du = Duvw;
  double *Dv = Duvw + 3;

  double cu = cos(u);
  double su = sin(u);
  double cv = cos(v);
  double sv = sin(v);

  double subX = 3*cv+5*su*cv*cu-30*su-60*su*pow(cu,6)
    +90*su*pow(cu,4);
  double subY = 80*cv*pow(cu,7)*su+48*cv*pow(cu,6)
    -80*cv*pow(cu,5)*su
    -48*cv*pow(cu,4)-5*cv*pow(cu,3)*su-3*cv*pow(cu,2)+
    5*su*cv*cu+3*cv-60*su;
  double subZ = 3+5*su*cu;

  // The point
  Pt[0] = -2.0/15.0*cu*subX;
  Pt[1] = -1.0/15.0*su*subY;
  Pt[2] = 2.0/15.0*sv*subZ;
  
  // The derivatives
  Du[0] = 2.0/15.0*su*subX -2.0/15.0*cu*(5.0*cv*pow(cu,2)
    -5.0*pow(su,2)*cv-30.0*cu-60.0*pow(cu,7)
    +360.0*pow(su,2)*pow(cu,5)
    +90.0*pow(cu,5)-360.0*pow(su,2)*pow(cu,3));
  Dv[0] = -2.0/15.0*cu*(-3.0*sv-5.0*su*sv*cu);
  Du[1] = -1.0/15.0*cu*subY-1.0/15.0*su*(
    -560.0*cv*pow(cu,6)*pow(su,2)
    +80.0*cv*pow(cu,8)-288*cv*pow(cu,5)*su+
    400*cv*pow(cu,4)*pow(su,2)
    -80.0*cv*pow(cu,6)+192*cv*pow(cu,3)*su
    +15.0*pow(su,2)*cv*pow(cu,2)
    -5.0*cv*pow(cu,4)+6.0*su*cv*cu+5.0*cv*pow(cu,2)
    -5.0*pow(su,2)*cv-60.0*cu);
  Dv[1] = -1.0/15.0*su*(-80.0*sv*pow(cu,7)*su-48.0*sv*pow(cu,6)
    +80.0*sv*pow(cu,5)*su+48.0*sv*pow(cu,4)+5.0*sv*pow(cu,3)*su
    +3.0*sv*pow(cu,2)-5.0*su*sv*cu-3.0*sv);
  Du[2] = 2.0/15.0*sv*(5.0*pow(cu,2)-5.0*pow(su,2));
  Dv[2] = 2.0/15.0*cv*subZ;
}

//----------------------------------------------------------------------------
double vtkParametricKlein::EvaluateScalar(double*, double*, double*)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkParametricKlein::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
