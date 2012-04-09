/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadric.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkStandardNewMacro(vtkQuadric);

// Construct quadric with all coefficients = 1.
vtkQuadric::vtkQuadric()
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

// Set the 10 coefficients of the quadric equation.
void vtkQuadric::SetCoefficients(double a[10])
{
  int i;
  double *c=this->Coefficients;

  for (i=0; i < 10; i++ )
    {
    if ( a[i] != c[i] )
      {
      break;
      }
    }

  if ( i < 10 )
    {
    this->Modified();
    for (i=0; i < 10; i++ )
      {
      c[i] = a[i];
      }
    }
}

// Evaluate quadric equation.
double vtkQuadric::EvaluateFunction(double x[3])
{
  double *a = this->Coefficients;
  return ( a[0]*x[0]*x[0] + a[1]*x[1]*x[1] + a[2]*x[2]*x[2] +
           a[3]*x[0]*x[1] + a[4]*x[1]*x[2] + a[5]*x[0]*x[2] +
           a[6]*x[0] + a[7]*x[1] + a[8]*x[2] + a[9] );
}

// Evaluate the gradient to the quadric equation.
void vtkQuadric::EvaluateGradient(double x[3], double n[3])
{
  double *a=this->Coefficients;

  n[0] = 2.0*a[0]*x[0] + a[3]*x[1] + a[5]*x[2] + a[6];
  n[1] = 2.0*a[1]*x[1] + a[3]*x[0] + a[4]*x[2] + a[7];
  n[2] = 2.0*a[2]*x[2] + a[4]*x[1] + a[5]*x[0] + a[8];
}


// Set the 10 coefficients of the quadric equation.
void vtkQuadric::SetCoefficients(double a0,double a1,double a2,double a3, double a4,
                                double a5,double a6,double a7,double a8, double a9)
{
  double a[10];

  a[0] = a0; a[1] = a1; a[2] = a2; a[3] = a3; a[4] = a4;
  a[5] = a5; a[6] = a6; a[7] = a7; a[8] = a8; a[9] = a9;

  vtkQuadric::SetCoefficients(a);
}
void vtkQuadric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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
