/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSuperquadric.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Mike Halle, Brigham and Women's Hospital


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
/* vtkSuperQuadric originally written by Michael Halle, 
   Brigham and Women's Hospital, July 1998.  

   Based on "Rigid physically based superquadrics", A. H. Barr,
   in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
*/

#include <math.h>
#include "vtkSuperquadric.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSuperquadric* vtkSuperquadric::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSuperquadric");
  if(ret)
    {
    return (vtkSuperquadric*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSuperquadric;
}




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

static const float MAX_FVAL = 1e12;
static float VTK_MIN_SUPERQUADRIC_ROUNDNESS = 1e-24;

void vtkSuperquadric::SetThetaRoundness(float e) 
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

void vtkSuperquadric::SetPhiRoundness(float e) 
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
float vtkSuperquadric::EvaluateFunction(float xyz[3])
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
  
  return (float)(val);
}

// Description
// Evaluate Superquadric function gradient.
void vtkSuperquadric::EvaluateGradient(float vtkNotUsed(xyz)[3], float g[3])
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
