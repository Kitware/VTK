/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearTransform.h
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
// .NAME vtkLinearTransform - abstract superclass for linear transformations
// .SECTION Description
// vtkLinearTransform provides a generic interface for linear 
// (affine or 12 degree-of-freedom) geometric transformations. 
// .SECTION see also
// vtkTransform vtkIdentityTransform 


#ifndef __vtkLinearTransform_h
#define __vtkLinearTransform_h

#include "vtkHomogeneousTransform.h"

class VTK_EXPORT vtkLinearTransform : public vtkHomogeneousTransform
{
public:

  vtkTypeMacro(vtkLinearTransform,vtkHomogeneousTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the transformation to a normal.
  // You can use the same array to store both the input and output.
  void TransformNormal(const float in[3], float out[3]) {
    this->Update(); this->InternalTransformNormal(in,out); };

  // Description:
  // Apply the transformation to a double-precision normal.
  // You can use the same array to store both the input and output.
  void TransformNormal(const double in[3], double out[3]) {
    this->Update(); this->InternalTransformNormal(in,out); };

  // Description:
  // Synonymous with TransformDoubleNormal(x,y,z).
  // Use this if you are programming in python, tcl or Java.
  double *TransformNormal(double x, double y, double z) {
    return this->TransformDoubleNormal(x,y,z); }
  double *TransformNormal(const double normal[3]) {
    return this->TransformDoubleNormal(normal[0],normal[1],normal[2]); };

  // Description:
  // Apply the transformation to an (x,y,z) normal.
  // Use this if you are programming in python, tcl or Java.
  float *TransformFloatNormal(float x, float y, float z) {
    this->InternalFloatPoint[0] = x;
    this->InternalFloatPoint[1] = y;
    this->InternalFloatPoint[2] = z;
    this->TransformNormal(this->InternalFloatPoint,this->InternalFloatPoint);
    return this->InternalFloatPoint; };
  float *TransformFloatNormal(const float normal[3]) {
    return this->TransformFloatNormal(normal[0],normal[1],normal[2]); };

  // Description:
  // Apply the transformation to a double-precision (x,y,z) normal.
  // Use this if you are programming in python, tcl or Java.
  double *TransformDoubleNormal(double x, double y, double z) {
    this->InternalDoublePoint[0] = x;
    this->InternalDoublePoint[1] = y;
    this->InternalDoublePoint[2] = z;
    this->TransformNormal(this->InternalDoublePoint,this->InternalDoublePoint);
    return this->InternalDoublePoint; };
  double *TransformDoubleNormal(const double normal[3]) {
    return this->TransformDoubleNormal(normal[0],normal[1],normal[2]); };

  // Description:
  // Synonymous with TransformDoubleVector(x,y,z).
  // Use this if you are programming in python, tcl or Java.
  double *TransformVector(double x, double y, double z) {
    return this->TransformDoubleVector(x,y,z); }
  double *TransformVector(const double normal[3]) {
    return this->TransformDoubleVector(normal[0],normal[1],normal[2]); };

  // Description:
  // Apply the transformation to a vector.
  // You can use the same array to store both the input and output.
  void TransformVector(const float in[3], float out[3]) {
    this->Update(); this->InternalTransformVector(in,out); };

  // Description:
  // Apply the transformation to a double-precision vector.
  // You can use the same array to store both the input and output.
  void TransformVector(const double in[3], double out[3]) {
    this->Update(); this->InternalTransformVector(in,out); };

  // Description:
  // Apply the transformation to an (x,y,z) vector.
  // Use this if you are programming in python, tcl or Java.
  float *TransformFloatVector(float x, float y, float z) {
      this->InternalFloatPoint[0] = x;
      this->InternalFloatPoint[1] = y;
      this->InternalFloatPoint[2] = z;
      this->TransformVector(this->InternalFloatPoint,this->InternalFloatPoint);
      return this->InternalFloatPoint; };
  float *TransformFloatVector(const float vec[3]) {
    return this->TransformFloatVector(vec[0],vec[1],vec[2]); };

  // Description:
  // Apply the transformation to a double-precision (x,y,z) vector.
  // Use this if you are programming in python, tcl or Java.
  double *TransformDoubleVector(double x, double y, double z) {
    this->InternalDoublePoint[0] = x;
    this->InternalDoublePoint[1] = y;
    this->InternalDoublePoint[2] = z;
    this->TransformVector(this->InternalDoublePoint,this->InternalDoublePoint);
    return this->InternalDoublePoint; };
  double *TransformDoubleVector(const double vec[3]) {
    return this->TransformDoubleVector(vec[0],vec[1],vec[2]); };

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.  
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a series of normals, and append the
  // results to outNms.  
  virtual void TransformNormals(vtkNormals *inNms, vtkNormals *outNms)
    { 
      this->TransformNormals(inNms->GetData(), outNms->GetData());
    }
  virtual void TransformNormals(vtkDataArray *inNms, vtkDataArray *outNms);

  // Description:
  // Apply the transformation to a series of vectors, and append the
  // results to outVrs.  
  virtual void TransformVectors(vtkVectors *inVrs, vtkVectors *outVrs)
    {
      this->TransformVectors(inVrs->GetData(), outVrs->GetData());
    }
  virtual void TransformVectors(vtkDataArray *inVrs, vtkDataArray *outVrs);

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
  // Just like GetInverse, but it includes a typecast to 
  // vtkLinearTransform.
  vtkLinearTransform *GetLinearInverse() { 
    return (vtkLinearTransform *)this->GetInverse(); }; 

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);
  void InternalTransformPoint(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  virtual void InternalTransformNormal(const float in[3], float out[3]);
  virtual void InternalTransformNormal(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  virtual void InternalTransformVector(const float in[3], float out[3]);
  virtual void InternalTransformVector(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
				   float derivative[3][3]);
  void InternalTransformDerivative(const double in[3], double out[3],
				   double derivative[3][3]);

protected:
  vtkLinearTransform() {};
  ~vtkLinearTransform() {};
  vtkLinearTransform(const vtkLinearTransform&);
  void operator=(const vtkLinearTransform&);
};

#endif





