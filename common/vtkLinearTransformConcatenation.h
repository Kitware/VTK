/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearTransformConcatenation.h
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
// .NAME vtkLinearTransformConcatenation - concatenation of linear 
// transformations
// .SECTION Description
// vtkLinearTransformConcatenation is a special LinearTransform 
// which allows concatenation of 4x4 matrices in a pipelined manner.
// .SECTION see also
// vtkGeneralTransformConcatenation vtkLinearTransformConcatenation


#ifndef __vtkLinearTransformConcatenation_h
#define __vtkLinearTransformConcatenation_h

#include "vtkLinearTransform.h"
#include "vtkMutexLock.h"

class VTK_EXPORT vtkLinearTransformConcatenation : 
  public vtkLinearTransform
{
public:
  static vtkLinearTransformConcatenation *New();

  vtkTypeMacro(vtkLinearTransformConcatenation,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Concatenate the current transform with the specified transform(s),
  // taking the PreMultiply flag into consideration.  If you specify
  // multiple transforms, then (assuming that your current transform
  // is called 't') the result is t*t1*t2*t3*t4 in PreMultiply mode,
  // or t1*t2*t3*t4*t in PostMultiply mode.
  void Concatenate(vtkLinearTransform *transform) {
    this->Concatenation->Concatenate(transform); };
  void Concatenate(vtkLinearTransform *t1,
		   vtkLinearTransform *t2) { 
    this->Concatenate(t1,t2,0,0); };
  void Concatenate(vtkLinearTransform *t1,
		   vtkLinearTransform *t2,
		   vtkLinearTransform *t3) { 
    this->Concatenate(t1,t2,t3,0); };
  void Concatenate(vtkLinearTransform *t1,
		   vtkLinearTransform *t2,
		   vtkLinearTransform *t3,
		   vtkLinearTransform *t4) {
    this->Concatenation->Concatenate(t1,t2,t3,t4); };

  // Description:
  // Sets the internal state of the transform to post multiply. All
  // subsequent matrix operations will occur after those already represented
  // in the current transformation matrix.  The default is PreMultiply.
  void PostMultiply() { this->Concatenation->PostMultiply(); };

  // Description:
  // Sets the internal state of the transform to pre multiply. All subsequent
  // matrix operations will occur before those already represented in the
  // current transformation matrix.  The default is PreMultiply.
  void PreMultiply() { this->Concatenation->PreMultiply(); };

  // Description:
  // Invert the transformation. 
  void Inverse() { this->Concatenation->Inverse(); };

  // Description:
  // Create an identity transformation.
  void Identity() { this->Concatenation->Identity(); };

  // Description:
  // Return modified time of transformation.
  unsigned long GetMTime();

  // Description:
  // This method does no type checking, use DeepCopy instead.
  void InternalDeepCopy(vtkGeneralTransform *transform);

  // Description:
  // Make another transform of the same type.
  vtkGeneralTransform *MakeTransform();

protected:
  vtkLinearTransformConcatenation();
  ~vtkLinearTransformConcatenation();
  vtkLinearTransformConcatenation(
	  const vtkLinearTransformConcatenation&) {};
  void operator=(const vtkLinearTransformConcatenation&) {};

  // Description:
  // Update the concatenated transform.
  void InternalUpdate();
};

#endif





