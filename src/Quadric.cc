/*=========================================================================

  Program:   Visualization Library
  Module:    Quadric.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <math.h>
#include "Quadric.hh"

// Description
// Construct quadric with all coefficients = 1.
vlQuadric::vlQuadric()
{
  this->Coefficients[0] = 1.0;
  this->Coefficients[1] = 1.0;
  this->Coefficients[2] = 1.0;
  this->Coefficients[3] = 1.0;
  this->Coefficients[4] = 1.0;
  this->Coefficients[5] = 1.0;
  this->Coefficients[6] = 1.0;
  this->Coefficients[7] = 1.0;
  this->Coefficients[8] = 1.0;
  this->Coefficients[9] = 1.0;
}

// Description
// Evaluate quadric equation.
float vlQuadric::Evaluate(float x, float y, float z)
{
  float *a = this->Coefficients;
  return ( a[0]*x*x + a[1]*y*y + a[2]*z*z +
           a[3]*x*y + a[4]*y*z + a[5]*x*z +
           a[6]*x + a[7]*y + a[8]*z + a[9] );
}

// Description
// Set the 10 coefficients of the quadric equation.
void vlQuadric::SetCoefficients(float a[10])
{
  int i;
  float *c=this->Coefficients;

  for (i=0; i < 10; i++ )
    if ( a[i] != c[i] )
      break;

  if ( i < 10 )
    {
    this->Modified();
    for (int i=0; i < 10; i++ ) c[i] = a[i];
    }
}

// Description
// Evaluate the normal to the quadric equation.
void vlQuadric::EvaluateNormal(float x, float y, float z, float n[3])
{
  float *a=this->Coefficients;

  n[0] = 2.0*a[0]*x + a[3]*y + a[5]*z + a[6];
  n[1] = 2.0*a[1]*y + a[3]*x + a[4]*z + a[7];
  n[2] = 2.0*a[2]*z + a[4]*y + a[5]*x + a[8];
}


// Description
// Set the 10 coefficients of the quadric equation.
void vlQuadric::SetCoefficients(float a0,float a1,float a2,float a3, float a4, 
                                float a5,float a6,float a7,float a8, float a9)
{
  float a[10];

  a[0] = a0; a[1] = a1; a[2] = a2; a[3] = a3; a[4] = a4; 
  a[5] = a5; a[6] = a6; a[7] = a7; a[8] = a8; a[9] = a9; 

  vlQuadric::SetCoefficients(a);
}
void vlQuadric::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Coefficients: " 
     << "\n\ta0: " << this->Coefficients[0]
     << "\n\ta1: " << this->Coefficients[1]
     << "\n\ta2: " << this->Coefficients[2]
     << "\n\ta3: " << this->Coefficients[3]
     << "\n\ta4: " << this->Coefficients[4]
     << "\n\ta5: " << this->Coefficients[5]
     << "\n\ta6: " << this->Coefficients[6]
     << "\n\ta7: " << this->Coefficients[7]
     << "\n\ta8: " << this->Coefficients[8]
     << "\n\ta9: " << this->Coefficients[9] << "\n";
}
