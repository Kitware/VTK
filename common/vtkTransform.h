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

// .NAME vtkTransform - a general matrix transformation class
// .SECTION Description
// vtkTransform maintains a stack of 4x4 transformation matrices.  A
// variety of methods are provided to manipulate the translation,
// scale, and rotation components of the matrix.  Methods operate on
// the matrix at the top of the stack. Many objects, such as vtkActor and
// vtkCamera, use this class for performing their matrix operations.
// It is very important to realize that this class performs all of
// its operations in a right handed coordinate system with right
// handed rotations. Some other graphics libraries use left handed 
// coordinate systems and rotations.

// .SECTION Caveats
// By default the initial matrix is the identity matrix.

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
  // Description:
  // Constructs a transform and sets the following defaults
  // preMultiplyFlag = 1 stackSize = 10. It then
  // creates an identity matrix as the top matrix on the stack.
  static vtkTransform *New();

  vtkTypeMacro(vtkTransform,vtkLinearTransform);
  void PrintSelf (vtkOstream& os, vtkIndent indent);

  // Description:
  // Make a new transform of the same type.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Creates an identity matrix and makes it the current transformation matrix.
  void Identity ();

  // Description:
  // Deletes the transformation on the top of the stack and sets the top 
  // to the next transformation on the stack.
  void Pop ();

  // Description:
  // Sets the internal state of the transform to post multiply. All
  // subsequent matrix operations will occur after those already represented
  // in the current transformation matrix.  The default is PreMultiply.
  void PostMultiply ();

  // Description:
  // Sets the internal state of the transform to pre multiply. All subsequent
  // matrix operations will occur before those already represented in the
  // current transformation matrix.  The default is PreMultiply.
  void PreMultiply ();

  // Description:
  // Pushes the current transformation matrix onto the transformation stack.
  void Push ();

  // Description:
  // Creates an x rotation matrix and concatenates it with the current
  // transformation matrix. The angle is specified in degrees.
  void RotateX ( float angle);

  // Description:
  // Creates a y rotation matrix and concatenates it with the current
  // transformation matrix. The angle is specified in degrees.
  void RotateY ( float angle);

  // Description:
  // Creates a z rotation matrix and concatenates it with the current
  // transformation matrix. The angle is specified in degrees.
  void RotateZ (float angle);

  // Description:
  // Creates a matrix that rotates angle degrees about an axis through the
  // origin and x, y, z. It then concatenates this matrix with the current
  // transformation matrix.
//BTX
  void RotateWXYZ ( float angle, float x, float y, float z);
//ETX
  void RotateWXYZ (double angle, double x, double y, double z);

  // Description:
  // Scales the current transformation matrix in the x, y and z directions.
  // A scale factor of zero will automatically be replaced with one.
//BTX
  void Scale ( float x, float y, float z);
//ETX
  void Scale ( double x, double y, double z);
  // Description:
  // Translate the current transformation matrix by the vector {x, y, z}.
//BTX
  void Translate ( float x, float y, float z);
//ETX
  void Translate ( double x, double y, double z);
  // Description:
  // Transposes the current transformation matrix.
  void Transpose();

  // Description:
  // Obtain the transpose of the current transformation matrix.
  void GetTranspose(vtkMatrix4x4 *transpose);

  // Description:
  // Invert the current transformation matrix.
  void Inverse();

  // Return an inverse transform which will always update itself
  // to match this transform.
  vtkGeneralTransform *GetInverse() { 
    return vtkLinearTransform::GetInverse(); }

  // Description:
  // Return the inverse of the current transformation matrix.
  void GetInverse(vtkMatrix4x4 *inverse);

  // Description:
  // Get the x, y, z orientation angles from the transformation matrix as an
  // array of three floating point values.
  float *GetOrientation();

  // Description:
  // Get the x, y, z orientation angles from the transformation matrix.
  void GetOrientation(float *prx, float *pry, float *prz);
  void GetOrientation(float& rx, float& ry, float& rz)
    {this->GetOrientation(&rx,&ry,&rz);}

  // Description:
  // Return the wxyz quaternion representing the current orientation.
  float *GetOrientationWXYZ();  

  // Description:
  // Return the position from the current transformation matrix as an array
  // of three floating point numbers. This is simply returning the translation 
  // component of the 4x4 matrix.
  float *GetPosition();

  // Description:
  // Return the x, y, z positions from the current transformation matrix.
  // This is simply returning the translation component of the 4x4 matrix.
  void GetPosition (float *px, float *py, float *pz);
  void GetPosition (float& x, float& y, float& z)
    {this->GetPosition(&x, &y, &z);}

  // Description:
  // Return the x, y, z scale factors of the current transformation matrix as 
  // an array of three float numbers.
  float *GetScale();

  // Description:
  // Return the x, y, z scale factors of the current transformation matrix.
  void GetScale (float *psx, float *psy, float *psz);
  void GetScale (float& sx, float& sy, float& sz)
    {this->GetScale(&sx, &sy, &sz);}

  // Description:
  // Set the current matrix directly (copies m).
  void SetMatrix(vtkMatrix4x4 *m);

  // Description:
  // Set the current matrix directly (copies Elements).
  void SetMatrix(const double Elements[16]);

  // Description:
  // Concatenates the input matrix with the current transformation matrix.
  // The resulting matrix becomes the new current transformation matrix.
  // The setting of the PreMultiply flag determines whether the matrix
  // is PreConcatenated or PostConcatenated.
  void Concatenate(vtkMatrix4x4 *matrix);
  void Concatenate(const double Elements[16]);

  // Description:
  // Make this transform a copy of the specified transform.
  void DeepCopy(vtkGeneralTransform *t);

  // Description:
  // Returns the current transformation matrix.
  void GetMatrix(vtkMatrix4x4 *m);

  // Description:
  // Use this method only if you wish to compute the transformation in
  // homogenous (x,y,z,w) coordinates, otherwise use TransformPoint().
  // This method calls this->Matrix->MultiplyPoint().
  void MultiplyPoint (const float in[4],float out[4]) {
    this->Matrix->MultiplyPoint(in,out);};
  void MultiplyPoint (const double in[4],double out[4]) {      
    this->Matrix->MultiplyPoint(in,out);};

  // Description:
  // This method is deprecated.  Use TransformPoints instead.
  void MultiplyPoints(vtkPoints *inPts, vtkPoints *outPts) {
    this->TransformPoints(inPts,outPts); };
  
  // Description:
  // This method is deprecated.  Use TransformVectors instead.
  void MultiplyVectors(vtkVectors *inVectors, vtkVectors *outVectors) {
    this->TransformVectors(inVectors,outVectors); };

  // Description:
  // This method is deprecated.  Use TransformNormals instead.
  void MultiplyNormals(vtkNormals *inNormals, vtkNormals *outNormals) {
    this->TransformNormals(inNormals,outNormals); };

  // Description:
  // Do not use these functions -- they are here only to provide
  // combatibility with legacy code.  Use TransformPoint() or 
  // GetMatrixPointer()->MultiplyPoint() instead.
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
  vtkMatrix4x4& GetMatrix() {return *(this->GetMatrixPointer());};
  void Concatenate(vtkMatrix4x4 &matrix){this->Concatenate(&matrix);};
  void GetMatrix(vtkMatrix4x4 &m){this->GetMatrix(&m);};
  void SetMatrix(vtkMatrix4x4 &m){this->SetMatrix(&m);};
  void GetTranspose (vtkMatrix4x4 &transpose){this->GetTranspose(&transpose);};
  void GetInverse(vtkMatrix4x4& inverse){this->GetInverse(&inverse);};

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
