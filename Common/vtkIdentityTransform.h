/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdentityTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
// .NAME vtkIdentityTransform - a transform that doesn't do anything
// .SECTION Description
// vtkIdentityTransform is a transformation which will simply pass coordinate
// data unchanged.  All other transform types can also do this, however,
// the vtkIdentityTransform does so with much greater efficiency.
// .SECTION see also
// vtkLinearTransform


#ifndef __vtkIdentityTransform_h
#define __vtkIdentityTransform_h

#include "vtkLinearTransform.h"

class VTK_COMMON_EXPORT vtkIdentityTransform : public vtkLinearTransform
{
public:
  static vtkIdentityTransform *New();

  vtkTypeMacro(vtkIdentityTransform,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.  
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a series of normals, and append the
  // results to outNms.  
  void TransformNormals(vtkNormals *inNms, vtkNormals *outNms)
    { this->TransformNormals(inNms->GetData(), outNms->GetData()); }
  void TransformNormals(vtkDataArray *inNms, vtkDataArray *outNms);

  // Description:
  // Apply the transformation to a series of vectors, and append the
  // results to outVrs.  
  void TransformVectors(vtkDataArray *inVrs, vtkDataArray *outVrs);
  void TransformVectors(vtkVectors *inVrs, vtkVectors *outVrs)
    { this->TransformVectors(inVrs->GetData(), outVrs->GetData()); }

  // Description:
  // Apply the transformation to a combination of points, normals
  // and vectors.  
  void TransformPointsNormalsVectors(vtkPoints *inPts, 
				     vtkPoints *outPts, 
				     vtkDataArray *inNms, 
				     vtkDataArray *outNms,
				     vtkDataArray *inVrs, 
				     vtkDataArray *outVrs);
  void TransformPointsNormalsVectors(vtkPoints *inPts, 
				     vtkPoints *outPts, 
				     vtkNormals *inNms, 
				     vtkNormals *outNms,
				     vtkVectors *inVrs, 
				     vtkVectors *outVrs)
    { 
      this->TransformPointsNormalsVectors(inPts, outPts,
					  inNms->GetData(), outNms->GetData(),
					  inVrs->GetData(), outVrs->GetData());
    }

  // Invert the transformation.  This doesn't do anything to the 
  // identity transformation.
  void Inverse() {};

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);
  void InternalTransformPoint(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformNormal(const float in[3], float out[3]);
  void InternalTransformNormal(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformVector(const float in[3], float out[3]);
  void InternalTransformVector(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
				   float derivative[3][3]);
  void InternalTransformDerivative(const double in[3], double out[3],
				   double derivative[3][3]);

  // Description:
  // Make a transform of the same type.  This will actually
  // return the same transform.
  vtkAbstractTransform *MakeTransform();

protected:
  vtkIdentityTransform();
  ~vtkIdentityTransform();

  void InternalDeepCopy(vtkAbstractTransform *t);

private:
  vtkIdentityTransform(const vtkIdentityTransform&);  // Not implemented.
  void operator=(const vtkIdentityTransform&);  // Not implemented.
};

#endif





