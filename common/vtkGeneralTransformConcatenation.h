/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformConcatenation.h
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
// .NAME vtkGeneralTransformConcatenation - concatenation of general transforms
// .SECTION Description
// vtkGeneralTransformConcatenation is a special GeneralTransform which
// allows concatenation of heterogenous transform types.  The transforms
// are not actually concatenated, but this is simulated by passing each
// input point through each transform in turn.
// .SECTION see also
// vtkPerspectiveTransformConcatenation vtkLinearTransformConcatenation


#ifndef __vtkGeneralTransformConcatenation_h
#define __vtkGeneralTransformConcatenation_h

#include "vtkGeneralTransform.h"

class VTK_EXPORT vtkGeneralTransformConcatenation : public vtkGeneralTransform
{
public:
  static vtkGeneralTransformConcatenation *New();

  vtkTypeMacro(vtkGeneralTransformConcatenation,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Concatenate the current transform with the specified transform(s),
  // taking the PreMultiply flag into consideration.  If you specify
  // multiple transforms, then (assuming that your current transform
  // is called 't') the result is t*t1*t2*t3*t4 in PreMultiply mode,
  // or t1*t2*t3*t4*t in PostMultiply mode.
  void Concatenate(vtkGeneralTransform *transform);
  void Concatenate(vtkGeneralTransform *t1,
		   vtkGeneralTransform *t2) { this->Concatenate(t1,t2,0,0); };
  void Concatenate(vtkGeneralTransform *t1,
		   vtkGeneralTransform *t2,
		   vtkGeneralTransform *t3) { this->Concatenate(t1,t2,t3,0); };
  void Concatenate(vtkGeneralTransform *t1,
		   vtkGeneralTransform *t2,
		   vtkGeneralTransform *t3,
		   vtkGeneralTransform *t4);

  // Description:
  // Sets the internal state of the transform to post multiply. All
  // subsequent matrix operations will occur after those already represented
  // in the current transformation matrix.  The default is PreMultiply.
  void PostMultiply();

  // Description:
  // Sets the internal state of the transform to pre multiply. All subsequent
  // matrix operations will occur before those already represented in the
  // current transformation matrix.  The default is PreMultiply.
  void PreMultiply();

  // Description:
  // Invert the transformation. 
  void Inverse();

  // Description:
  // Create an identity transformation.
  void Identity();

  // Description:
  // Make another transform of the same type.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Copy another transformation into this one.
  void DeepCopy(vtkGeneralTransform *transform);

  // Description:
  // Return modified time of transformation.
  unsigned long GetMTime();

  // Description:
  // Update the concatenated transform.
  void Update();

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
  vtkGeneralTransformConcatenation();
  ~vtkGeneralTransformConcatenation();
  vtkGeneralTransformConcatenation(const vtkGeneralTransformConcatenation&) {};
  void operator=(const vtkGeneralTransformConcatenation&) {};

  int InverseFlag;
  int PreMultiplyFlag;

  int NumberOfTransforms;
  int MaxNumberOfTransforms;
  vtkGeneralTransform **TransformList;
  vtkGeneralTransform **InverseList;
};

//BTX
//----------------------------------------------------------------------------
inline void vtkGeneralTransformConcatenation::PreMultiply()
{
  if (this->PreMultiplyFlag)
    {
    return;
    }
  this->PreMultiplyFlag = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
inline void vtkGeneralTransformConcatenation::PostMultiply()
{
  if (!this->PreMultiplyFlag)
    {
    return;
    }
  this->PreMultiplyFlag = 0;
  this->Modified();
}
//ETX

#endif





