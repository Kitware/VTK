/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
// vtkMatrix4x4 vtkTransformCollection vtkTransformFilter
// vtkTransformPolyDataFilter

#ifndef __vtkTransform_h
#define __vtkTransform_h

#include "vtkObject.h"
#include "vtkMatrix4x4.h"
#include "vtkPoints.h"
#include "vtkNormals.h"
#include "vtkVectors.h"

class VTK_EXPORT vtkTransform : public vtkObject
{
 public:
  // Description:
  // Constructs a transform and sets the following defaults
  // preMultiplyFlag = 1 stackSize = 10. It then
  // creates an identity matrix as the top matrix on the stack.
  vtkTransform ();

  // Description:
  // Copy constructor. Creates an instance of vtkTransform and then
  // copies its instance variables from the values in t. 
  vtkTransform (const vtkTransform& t);

  ~vtkTransform ();
  static vtkTransform *New() {return new vtkTransform;};
  const char *GetClassName () {return "vtkTransform";};
  void PrintSelf (ostream& os, vtkIndent indent);
  vtkTransform &operator=(const vtkTransform &t);

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
  // in the current transformation matrix.
  void PostMultiply ();

  // Description:
  // Sets the internal state of the transform to pre multiply. All subsequent
  // matrix operations will occur before those already represented in the
  // current transformation matrix.
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
  void RotateWXYZ ( float angle, float x, float y, float z);

  // Description:
  // Scales the current transformation matrix in the x, y and z directions.
  // A scale factor of zero will automatically be replaced with one.
  void Scale ( float x, float y, float z);

  // Description:
  // Translate the current transformation matrix by the vector {x, y, z}.
  void Translate ( float x, float y, float z);

  // Description:
  // Transposes the current transformation matrix.
  void Transpose();

  // Description:
  // Obtain the transpose of the current transformation matrix.
  void GetTranspose (vtkMatrix4x4 *transpose);

  // Description:
  // Invert the current transformation matrix.
  void Inverse();

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
  void SetMatrix(vtkMatrix4x4& m);

  // Description:
  // Returns the current transformation matrix.
  vtkMatrix4x4 *GetMatrixPointer();
  
  // Description:
  // Returns the current transformation matrix.
  void GetMatrix (vtkMatrix4x4 *m);

  // Description:
  // Concatenates the input matrix with the current transformation matrix.
  // The resulting matrix becomes the new current transformation matrix.
  // The setting of the PreMultiply flag determines whether the matrix
  // is PreConcatenated or PostConcatenated.
  void Concatenate(vtkMatrix4x4 *matrix);

  // Description:
  // Multiplies matrices a and b and stores the result in c.
  void Multiply4x4(vtkMatrix4x4 *a, vtkMatrix4x4 *b, vtkMatrix4x4 *c);

  // Description:
  // Multiply a xyzw point by the transform and store the result in out.
  void MultiplyPoint (float in[4],float out[4]);

  // Description:
  // Multiplies a list of points (inPts) by the current transformation matrix.
  // Transformed points are appended to the output list (outPts).
  void MultiplyPoints(vtkPoints *inPts, vtkPoints *outPts);
  
  // Description:
  // Multiplies a list of vectors (inVectors) by the current transformation 
  // matrix. The transformed vectors are appended to the output list 
  // (outVectors). This is a special multiplication, since these are vectors. 
  // It multiplies vectors by the transposed inverse of the matrix, ignoring 
  // the translational components.
  void MultiplyVectors(vtkVectors *inVectors, vtkVectors *outVectors);

  // Description:
  // Multiplies a list of normals (inNormals) by the current transformation 
  // matrix. The transformed normals are appended to the output list 
  // (outNormals). This is a special multiplication, since these are normals.
  void MultiplyNormals(vtkNormals *inNormals, vtkNormals *outNormals);

  // Description:
  // Returns the result of multiplying the currently set Point by the current 
  // transformation matrix. Point is expressed in homogeneous coordinates.
  // The setting of the PreMultiplyFlag will determine if the Point is
  // Pre or Post multiplied.
  float *GetPoint();
  void GetPoint(float p[4]);

  // Description:
  // Set the point to use in the GetPoint calculations.
  vtkSetVector4Macro(Point,float);

  // Description:
  // For legacy compatability. Do not use.
  void Multiply4x4(vtkMatrix4x4 &a, vtkMatrix4x4 &b, vtkMatrix4x4 &c)
    {this->Multiply4x4(&a,&b,&c);}
  void Concatenate(vtkMatrix4x4 &matrix){this->Concatenate(&matrix);}
  void GetMatrix (vtkMatrix4x4 &m){this->GetMatrix(&m);}
  void GetTranspose (vtkMatrix4x4 &transpose){this->GetTranspose(&transpose);}
  void GetInverse(vtkMatrix4x4& inverse){this->GetInverse(&inverse);}
  vtkMatrix4x4& GetMatrix() {return *(this->GetMatrixPointer());}
  
private:
  int PreMultiplyFlag;
  int StackSize;
  vtkMatrix4x4 **Stack;
  vtkMatrix4x4 **StackBottom;
  float Point[4];
  float Orientation[3];

};

inline void vtkTransform::MultiplyPoint (float in[4],float out[4]) 
{
  this->Stack[0]->MultiplyPoint(in,out);
}

#endif
