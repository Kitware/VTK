/*=========================================================================

  Program:   Visualization Toolkit
  Module:    
  Language:  C++
  Date:      
  Version:   


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
/* vtkSuperQuadric originally written by Michael Halle, 
   Brigham and Women's Hospital, July 1998.  

   Based on "Rigid physically based superquadrics", A. H. Barr,
   in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
*/


#include <math.h>
#include "vtkSuperquadric.h"

// Description
// Construct Superquadric radius of 0.5.
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

static const float MAX_FVAL = 1e12;
static float VTK_MIN_SUPERQUADRIC_ROUNDNESS = 1e-24;


void vtkSuperquadric::SetThetaRoundness(float e) 
{
  if(e < VTK_MIN_SUPERQUADRIC_ROUNDNESS)
    e = VTK_MIN_SUPERQUADRIC_ROUNDNESS;

  if (this->ThetaRoundness != e)
    {
    this->ThetaRoundness = e;
    this->Modified();
    }
}

void vtkSuperquadric::SetPhiRoundness(float e) 
{
  if(e < VTK_MIN_SUPERQUADRIC_ROUNDNESS)
    e = VTK_MIN_SUPERQUADRIC_ROUNDNESS;

  if (this->PhiRoundness != e)
    {
    this->PhiRoundness = e;
    this->Modified();
    }
}

// Evaluate Superquadric equation
float vtkSuperquadric::EvaluateFunction(float xyz[3])
{
  float e = this->ThetaRoundness;
  float n = this->PhiRoundness;
  float p[3], s[3];
  float val;

  s[0] = this->Scale[0] * this->Size;
  s[1] = this->Scale[1] * this->Size;
  s[2] = this->Scale[2] * this->Size;

  if(this->Toroidal) {
    float tval;
    float alpha;

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
  
  return(val);
}

// Description
// Evaluate Superquadric function gradient.
void vtkSuperquadric::EvaluateGradient(float xyz[3], float g[3])
{
  // bogus! lazy!
  // if someone wants to figure these out, they are each the
  // partial of x, then y, then z with respect to f as shown above.
  // Careful for the fabs().

  g[0] = g[1] = g[2] = 0.0; 
}

void vtkSuperquadric::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

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
