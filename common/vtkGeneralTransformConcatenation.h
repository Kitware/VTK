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
// vtkGeneralTransform


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
  // Concatenate the current transform with the specified transform,
  // taking the PreMultiply flag into consideration.
  void Concatenate(vtkGeneralTransform *transform);

  // Description:
  // Set the order in which subsequent concatenations will be
  // applied.
  void PreMultiply();
  void PostMultiply();

  // Description:
  // Apply the transformation to a coordinate.  You can use the same 
  // array to store both the input and output point.
  void TransformPoint(const float in[3], float out[3]);
  void TransformPoint(const double in[3], double out[3]);

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.  
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a series of normals, and append the
  // results to outNms.  The outPts must have been calculated beforehand.
  // The inPts and outPts are required in order for nonlinear transformations
  // to be properly supported.
  void TransformNormals(vtkPoints *inPts, vtkPoints *outPts, 
			vtkNormals *inNms, vtkNormals *outNms);
  
  // Description:
  // Apply the transformation to a series of vectors, and append the
  // results to outVrs.  The outPts must have been calculated beforehand.
  // The inPts and outPts are required in order for nonlinear transformations
  // to be properly supported.
  void TransformVectors(vtkPoints *inPts, vtkPoints *outPts, 
			vtkVectors *inVrs, vtkVectors *outVrs);

  // Description:
  // Create an identity transformation.
  void Identity();

  // Description:
  // Invert the transformation.
  void Inverse();

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

protected:
  vtkGeneralTransformConcatenation();
  ~vtkGeneralTransformConcatenation();
  vtkGeneralTransformConcatenation(const vtkGeneralTransformConcatenation&) {};
  void operator=(const vtkGeneralTransformConcatenation&) {};

  int PreMultiplyFlag;
  int InverseFlag;

  int NumberOfTransforms;
  int MaxNumberOfTransforms;
  vtkGeneralTransform **TransformList;
  vtkGeneralTransform **InverseTransformList;
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





