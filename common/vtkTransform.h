/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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

// .NAME vtkTransform - describes linear transformations via a 4x4 matrix
// .SECTION Description
// A vtkTransform can be used to describe the full range of linear (also
// known as affine) coordinate transformations in three dimensions,
// which are internally represented as a 4x4 homogenous transformation
// matrix.  When you create a new vtkTransform, it is always initialized
// to the identity tranformation.
// <P>Most of the methods for manipulating this transformation,
// e.g. Translate, Rotate, and Concatenate, can operate in either
// PreMultiply (the default) or PostMultiply mode.  In PreMultiply
// mode, the translation, concatenation, etc. will occur before any
// transformations which are represented by the current matrix.  In
// PostMultiply mode, the additional tranformation will occur after 
// any transformations represented by the current matrix.
// <P>This class performs all of its operations in a right handed
// coordinate system with right handed rotations. Some other graphics
// libraries use left handed coordinate systems and rotations.  
// .SECTION See Also
// vtkLinearTransformConcatenation vtkMatrix4x4 vtkTransformCollection 
// vtkTransformFilter vtkTransformPolyDataFilter vtkProjectionTransform

#ifndef __vtkTransform_h
#define __vtkTransform_h

#include "vtkLinearTransform.h"
#include "vtkMatrix4x4.h"

class VTK_EXPORT vtkTransform : public vtkLinearTransform
{
 public:
  static vtkTransform *New();
  vtkTypeMacro(vtkTransform,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates an identity matrix and makes it the current transformation matrix.
  void Identity();

  // Description:
  // Invert the current transformation matrix.
  void Inverse();

  // Description:
  // Transposes the current transformation matrix.
  void Transpose();

  // Description:
  // Sets the internal state of the transform to pre multiply. All subsequent
  // matrix operations will occur before those already represented in the
  // current transformation matrix.  The default is PreMultiply.
  void PreMultiply();

   // Description:
  // Sets the internal state of the transform to post multiply. All
  // subsequent matrix operations will occur after those already represented
  // in the current transformation matrix.  The default is PreMultiply.
  void PostMultiply();

  // Description:
  // Create a translation matrix and concatenate it with the current
  // matrix according to PreMultiply or PostMultiply semantics.
  void Translate(double x, double y, double z);
  void Translate(const double x[3]) { this->Translate(x[0], x[1], x[2]); };
  void Translate(const float x[3]) { this->Translate(x[0], x[1], x[2]); };

  // Description:
  // Create a rotation matrix and concatenate it with the current matrix
  // according to PreMultiply or PostMultiply semantics.
  // The angle is in degrees, and (x,y,z) specifies the axis that the
  // rotation will be performed around. 
  void RotateWXYZ(double angle, double x, double y, double z);
  void RotateWXYZ(double angle, const double axis[3]) {
    this->RotateWXYZ(angle, axis[0], axis[1], axis[2]); };
  void RotateWXYZ(double angle, const float axis[3]) {
    this->RotateWXYZ(angle, axis[0], axis[1], axis[2]); };

  // Description:
  // Create a rotation matrix about the X, Y, or Z axis and concatenate
  // it with the current matrix according to PreMultiply or PostMultiply
  // semantics.  The angle is expressed in degrees.
  void RotateX(double angle) { this->RotateWXYZ(angle, 1, 0, 0); };
  void RotateY(double angle) { this->RotateWXYZ(angle, 0, 1, 0); };
  void RotateZ(double angle) { this->RotateWXYZ(angle, 0, 0, 1); };

  // Description:
  // Create a scale matrix (i.e. set the diagonal elements to x, y, z)
  // and concatenate it with the current matrix according to PreMultiply 
  // or PostMultiply semantics.
  void Scale(double x, double y, double z);
  void Scale(const double s[3]) { this->Scale(s[0], s[1], s[2]); };
  void Scale(const float s[3]) { this->Scale(s[0], s[1], s[2]); };

  // Description:
  // Set the current matrix directly.  This can be used in combination
  // with vtkCamera::GetPerspectiveTransformMatrix() to copy a camera's
  // perspective transform.
  void SetMatrix(vtkMatrix4x4 *matrix) { this->SetMatrix(*matrix->Element); };
  void SetMatrix(const double Elements[16]);

  // Description:
  // Concatenates the input matrix with the current transformation matrix.
  // The resulting matrix becomes the new current transformation matrix.
  // The setting of the PreMultiply flag determines whether the matrix
  // is PreConcatenated or PostConcatenated.
  void Concatenate(vtkMatrix4x4 *matrix) { 
    this->Concatenate(*matrix->Element); };
  void Concatenate(const double Elements[16]);

  // Description:
  // Get the x, y, z orientation angles from the transformation matrix as an
  // array of three floating point values.  Warning: if a Scale() has been
  // applied with different x,y,z scale factors, then this method will
  // only return the correct answer if the Scale() was the first transform
  // applied.
  void GetOrientation(double orient[3]);
  void GetOrientation(float orient[3]) {
    double temp[3]; this->GetOrientation(temp); 
    orient[0] = temp[0]; orient[1] = temp[1]; orient[2] = temp[2]; };
  float *GetOrientation() { 
    this->GetOrientation(this->ReturnValue); return this->ReturnValue; };

  // Description:
  // Return the wxyz quaternion representing the current orientation.
  // Warning: if a Scale() has been applied with different x,y,z scale 
  // factors, then this method will only return the correct answer if
  // the Scale() was the last transform applied.
  void GetOrientationWXYZ(double wxyz[4]);
  void GetOrientationWXYZ(float wxyz[3]) {
    double temp[4]; this->GetOrientation(temp); 
    wxyz[0]=temp[0]; wxyz[1]=temp[1]; wxyz[2]=temp[2]; wxyz[3]=temp[3];};
  float *GetOrientationWXYZ() { 
    this->GetOrientationWXYZ(this->ReturnValue); return this->ReturnValue; };

  // Description:
  // Return the position from the current transformation matrix as an array
  // of three floating point numbers. This is simply returning the translation 
  // component of the 4x4 matrix.
  void GetPosition(double pos[3]);
  void GetPosition(float pos[3]) {
    double temp[3]; this->GetPosition(temp); 
    pos[0] = temp[0]; pos[1] = temp[1]; pos[2] = temp[2]; };
  float *GetPosition() { 
    this->GetPosition(this->ReturnValue); return this->ReturnValue; };

  // Description:
  // Return the x, y, z scale factors of the current transformation matrix as 
  // an array of three float numbers.  Warning: this method returns a 
  // meaningless answer unless the Scale() applied to the matrix was the
  // first transform applied.
  void GetScale(double scale[3]);
  void GetScale(float scale[3]) {
    double temp[3]; this->GetScale(temp); 
    scale[0] = temp[0]; scale[1] = temp[1]; scale[2] = temp[2]; };
  float *GetScale() { 
    this->GetScale(this->ReturnValue); return this->ReturnValue; };

  // Description:
  // Return the inverse of the current transformation matrix.
  void GetInverse(vtkMatrix4x4 *inverse);

  // Description:
  // Obtain the transpose of the current transformation matrix.
  void GetTranspose(vtkMatrix4x4 *transpose);

  // Description:
  // Pushes the current transformation matrix onto the transformation stack.
  void Push();

  // Description:
  // Deletes the transformation on the top of the stack and sets the top 
  // to the next transformation on the stack.
  void Pop();

  // Return an inverse transform which will always update itself
  // to match this transform.
  vtkGeneralTransform *GetInverse() { 
    return vtkLinearTransform::GetInverse(); }

  // Description:
  // This method does no type checking, use DeepCopy instead.
  void InternalDeepCopy(vtkGeneralTransform *t);

  // Description:
  // Make a new transform of the same type.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Use this method only if you wish to compute the transformation in
  // homogenous (x,y,z,w) coordinates, otherwise use TransformPoint().
  // This method calls this->Matrix->MultiplyPoint().
  void MultiplyPoint(const float in[4], float out[4]) {
    this->Matrix->MultiplyPoint(in,out);};
  void MultiplyPoint(const double in[4], double out[4]) {      
    this->Matrix->MultiplyPoint(in,out);};

  // Description:
  // These methods are obsolete.  Use TransformPoints, TransformVectors,
  // or TransformNormals instead.
  void MultiplyPoints(vtkPoints *inPts, vtkPoints *outPts) {
    this->TransformPoints(inPts,outPts); };
  void MultiplyVectors(vtkVectors *inVectors, vtkVectors *outVectors) {
    this->TransformVectors(inVectors,outVectors); };
  void MultiplyNormals(vtkNormals *inNormals, vtkNormals *outNormals) {
    this->TransformNormals(inNormals,outNormals); };

  // Description:
  // Do not use these functions -- they are here only to provide
  // combatibility with legacy code.  Use TransformPoint() or 
  // GetMatrix()->MultiplyPoint() instead.
  // <p>Returns the result of multiplying the currently set Point by the 
  // current transformation matrix. Point is expressed in homogeneous 
  // coordinates.  The setting of the PreMultiplyFlag will determine if 
  // the Point is Pre or Post multiplied.  
  vtkSetVector4Macro(Point,float);
  vtkSetVector4Macro(DoublePoint,double);
  float *GetPoint();
  double *GetDoublePoint();
  void GetPoint(float p[4]);

  // Description:
  // For legacy compatibility. Do not use.
  void Multiply4x4(vtkMatrix4x4 *a, vtkMatrix4x4 *b, vtkMatrix4x4 *c) {
    vtkMatrix4x4::Multiply4x4(a,b,c); }; 
  void Multiply4x4(const double a[16], const double b[16], double c[16]) {
    vtkMatrix4x4::Multiply4x4(a,b,c); }; 
  void Multiply4x4(vtkMatrix4x4 &a, vtkMatrix4x4 &b, vtkMatrix4x4 &c) {
    vtkMatrix4x4::Multiply4x4(&a,&b,&c); }; 
  void Concatenate(vtkMatrix4x4 &matrix){this->Concatenate(&matrix);};
  void SetMatrix(vtkMatrix4x4 &m){this->SetMatrix(&m);};
  void GetTranspose (vtkMatrix4x4 &transpose){this->GetTranspose(&transpose);};
  void GetInverse(vtkMatrix4x4& inverse){this->GetInverse(&inverse);};
//BTX
  void GetOrientation(float *prx, float *pry, float *prz) {
    double temp[3]; this->GetOrientation(temp); 
    *prx = temp[0]; *pry = temp[1]; *prz = temp[2]; };    
  void GetOrientation(float& rx, float& ry, float& rz)
    {this->GetOrientation(&rx,&ry,&rz);}
  void GetPosition(float *px, float *py, float *pz) {
    double temp[3]; this->GetPosition(temp); 
    *px = temp[0]; *py = temp[1]; *pz = temp[2]; };
  void GetPosition(float& x, float& y, float& z)
    {this->GetPosition(&x, &y, &z);}
  void GetScale(float *psx, float *psy, float *psz) {
    double temp[3]; this->GetScale(temp); 
    *psx = temp[0]; *psy = temp[1]; *psz = temp[2]; };
  void GetScale(float& x, float& y, float& z)
    {this->GetScale(&x, &y, &z);}
//ETX

protected:
  vtkTransform ();
  ~vtkTransform ();
  vtkTransform (const vtkTransform& t);
  void operator=(const vtkTransform&) {};

  int PreMultiplyFlag;
  int StackSize;
  vtkMatrix4x4 **Stack;
  vtkMatrix4x4 **StackBottom;
  float Point[4];
  double DoublePoint[4];
  float ReturnValue[4];
};

#endif
