/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphere.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkSphere.h"

// Description
// Construct sphere with center at (0,0,0) and radius=0.5.
vtkSphere::vtkSphere()
{
  this->Radius = 0.5;

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
}

// Description
// Evaluate sphere equation ((x-x0)^2 + (y-y0)^2 + (z-z0)^2) - R^2.
float vtkSphere::EvaluateFunction(float x[3])
{
  return ( ((x[0] - this->Center[0]) * (x[0] - this->Center[0]) + 
           (x[1] - this->Center[1]) * (x[1] - this->Center[1]) + 
           (x[2] - this->Center[2]) * (x[2] - this->Center[2])) - 
           this->Radius*this->Radius );
}

// Description
// Evaluate sphere gradient.
void vtkSphere::EvaluateGradient(float x[3], float n[3])
{
  n[0] = 2.0 * (x[0] - this->Center[0]);
  n[1] = 2.0 * (x[1] - this->Center[1]);
  n[2] = 2.0 * (x[2] - this->Center[2]);
}

void vtkSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << ")\n";
}
