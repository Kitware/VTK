/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerspectiveTransform.h
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
// .NAME vtkPerspectiveTransform - superclass for perspective transformations
// .SECTION Description
// vtkPerspectiveTransform provides a generic interface for perspective 
// transformations, i.e. transformations which can be represented by 
// multiplying a 4x4 matrix with a homogenous coordinate. 
// .SECTION see also
// vtkProjectionTransform vtkPerspectiveTransformConcatenation 
// vtkLinearTransform vtkCamera


#ifndef __vtkPerspectiveTransform_h
#define __vtkPerspectiveTransform_h

#include "vtkGeneralTransform.h"
#include "vtkMatrix4x4.h"

class VTK_EXPORT vtkPerspectiveTransform : public vtkGeneralTransform
{
public:

  vtkTypeMacro(vtkPerspectiveTransform,vtkGeneralTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the transformation to a coordinate.  You can use the same 
  // array to store both the input and output point.
  void TransformPoint(const float in[3], float out[3]);

  // Description:
  // Apply the transformation to a double-precision coordinate.
  // You can use the same array to store both the input and output point.
  void TransformPoint(const double in[3], double out[3]);

  // Description:
  // Apply the transformation to an (x,y,z) coordinate.
  // Use this if you are programming in python, tcl or Java.
  float *TransformPoint(float x, float y, float z) {
    return this->vtkGeneralTransform::TransformPoint(x,y,z); }
  float *TransformPoint(const float point[3]) {
    return this->TransformPoint(point[0],point[1],point[2]); };

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.  
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a combination of points, normals
  // and vectors.  
  void TransformPointsNormalsVectors(vtkPoints *inPts, 
				     vtkPoints *outPts, 
				     vtkNormals *inNms, 
				     vtkNormals *outNms,
				     vtkVectors *inVrs, 
				     vtkVectors *outVrs);

  // Description:
  // Get a copy of the internal transformation matrix.  The
  // transform is Updated first, to guarantee that the matrix
  // is valid.
  void GetMatrix(vtkMatrix4x4 *m);

  // Description:
  // Get a pointer to an internal vtkMatrix4x4 that represents
  // the transformation.  An Update() is called on the transform
  // to ensure that the matrix is up-to-date when you get it.
  // You should not store the matrix pointer anywhere because it
  // might become stale.
  vtkMatrix4x4 *GetMatrix() { this->Update(); return this->Matrix; };

  // Description:
  // This is an obsolete method provided for backwards-compatibility.
  vtkMatrix4x4 *GetMatrixPointer() { this->Update(); return this->Matrix; };

  // Description:
  // Get the inverse of this transform.  If you modify this transform,
  // the returned inverse transform will automatically update.
  vtkGeneralTransform *GetInverse();

  // Description:
  // Get the inverse of this transform typecast to a vtkPerspectiveTransform.
  vtkPerspectiveTransform *GetPerspectiveInverse() {
    return (vtkPerspectiveTransform *)this->GetInverse(); };

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);
  void InternalTransformPoint(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
				   float derivative[3][3]);
  void InternalTransformDerivative(const double in[3], double out[3],
				   double derivative[3][3]);

protected:
  vtkPerspectiveTransform() { this->Matrix = vtkMatrix4x4::New(); };
  ~vtkPerspectiveTransform() { if (this->Matrix) { this->Matrix->Delete(); } };
  vtkPerspectiveTransform(const vtkPerspectiveTransform&) {};
  void operator=(const vtkPerspectiveTransform&) {};

  vtkMatrix4x4 *Matrix;
};

#endif





