/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadric.cxx
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
#include <math.h>
#include "vtkQuadric.hh"

// Description
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

// Description
// Set the 10 coefficients of the quadric equation.
void vtkQuadric::SetCoefficients(float a[10])
{
  int i;
  float *c=this->Coefficients;

  for (i=0; i < 10; i++ )
    if ( a[i] != c[i] )
      break;

  if ( i < 10 )
    {
    this->Modified();
    for (i=0; i < 10; i++ ) c[i] = a[i];
    }
}

// Description
// Evaluate quadric equation.
float vtkQuadric::EvaluateFunction(float x[3])
{
  float *a = this->Coefficients;
  return ( a[0]*x[0]*x[0] + a[1]*x[1]*x[1] + a[2]*x[2]*x[2] +
           a[3]*x[0]*x[1] + a[4]*x[1]*x[2] + a[5]*x[0]*x[2] +
           a[6]*x[0] + a[7]*x[1] + a[8]*x[2] + a[9] );
}

// Description
// Evaluate the gradient to the quadric equation.
void vtkQuadric::EvaluateGradient(float x[3], float n[3])
{
  float *a=this->Coefficients;

  n[0] = 2.0*a[0]*x[0] + a[3]*x[1] + a[5]*x[2] + a[6];
  n[1] = 2.0*a[1]*x[1] + a[3]*x[0] + a[4]*x[2] + a[7];
  n[2] = 2.0*a[2]*x[2] + a[4]*x[1] + a[5]*x[0] + a[8];
}


// Description
// Set the 10 coefficients of the quadric equation.
void vtkQuadric::SetCoefficients(float a0,float a1,float a2,float a3, float a4, 
                                float a5,float a6,float a7,float a8, float a9)
{
  float a[10];

  a[0] = a0; a[1] = a1; a[2] = a2; a[3] = a3; a[4] = a4; 
  a[5] = a5; a[6] = a6; a[7] = a7; a[8] = a8; a[9] = a9; 

  vtkQuadric::SetCoefficients(a);
}
void vtkQuadric::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

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
