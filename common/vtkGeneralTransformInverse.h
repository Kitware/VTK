/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransformInverse.h
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
// .NAME vtkGeneralTransformInverse - inverse of a general transform
// .SECTION Description
// The vtkGeneralTransformInverse is a helper class for vtkGeneralTransform,
// you should avoid using it directly.   
// .SECTION see also
// vtkGeneralTransform


#ifndef __vtkGeneralTransformInverse_h
#define __vtkGeneralTransformInverse_h

#include "vtkGeneralTransform.h"

class VTK_EXPORT vtkGeneralTransformInverse : public vtkGeneralTransform
{
public:
  static vtkGeneralTransformInverse *New();

  vtkTypeMacro(vtkGeneralTransformInverse,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the transform that you want this to be an inverse of.
  virtual void SetOriginalTransform(vtkGeneralTransform *transform);
  vtkGetObjectMacro(OriginalTransform, vtkGeneralTransform);
 
  // Description:
  // Get the internal copy of the inverse transform, which will
  // be of the same type as the OriginalTransform.
  vtkGetObjectMacro(InverseTransform, vtkGeneralTransform);

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
  // Get the inverse of this transform (this returns the OriginalTransform).
  vtkGeneralTransform *GetInverse();

  // Description:
  // Set this transform to the identity transform.  Warning: this modifies
  // the OriginalTransform.
  void Identity();

  // Description:
  // Set this transform to its own inverse.  Warning: this modifies
  // the OriginalTransform.
  void Inverse();

  // Description:
  // Copy another transform into this one.  Warning: this modifies
  // the OriginalTransform.
  void DeepCopy(vtkGeneralTransform *transform);

  // Description:
  // Make another transform of the same type as the OriginalTransform.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Return the modified time of this transform.
  unsigned long GetMTime();

  // Description:
  // Update the inverse transform from the original.
  void Update();

  // Description:
  // Needs a special UnRegister() implementation to avoid
  // circular references.
  void UnRegister(vtkObject * o);

protected:
  vtkGeneralTransformInverse();
  ~vtkGeneralTransformInverse();
  vtkGeneralTransformInverse(const vtkGeneralTransformInverse&) {};
  void operator=(const vtkGeneralTransformInverse&) {};

  int UpdateRequired;
  vtkGeneralTransform *OriginalTransform;
  vtkGeneralTransform *InverseTransform;
};

#endif





