/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkWarpTransform - superclass for nonlinear geometric transformations
// .SECTION Description
// vtkWarpTransform provides a generic interface for nonlinear 
// warp transformations.  These are also commonly known as morphological
// transformations.
// .SECTION see also
// vtkThinPlateSplineTransform, vtkGridTransform


#ifndef __vtkWarpTransform_h
#define __vtkWarpTransform_h

#include "vtkGeneralTransform.h"

class VTK_EXPORT vtkWarpTransform : public vtkGeneralTransform
{
public:

  vtkTypeMacro(vtkWarpTransform,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Invert the transformation. 
  void Inverse();

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);

  // Description:
  // This will calculate the transformation as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
				   float derivative[3][3]);

protected:
  vtkWarpTransform() { this->InverseFlag = 0; };
  ~vtkWarpTransform() {};
  vtkWarpTransform(const vtkWarpTransform&) {};
  void operator=(const vtkWarpTransform&) {};

  // Description:
  // The inverse of a warp transformation is usually calculated using
  // an interative technique such as Newton's method.  The InverseFlag
  // specifies whether we should calculate the inverse transformation
  // instead of providing the forward transformation.
  virtual void ForwardTransformPoint(const float in[3], float out[3]) = 0;
  virtual void ForwardTransformDerivative(const float in[3], float out[3],
					  float derivative[3][3]) = 0;
  virtual void InverseTransformPoint(const float in[3], float out[3]) = 0;

  int InverseFlag;
};

#endif





