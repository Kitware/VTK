/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Trans.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <stdlib.h>
#include <math.h>
#include "Trans.hh"
#include "vtkMath.hh"

// Description:
// Constructs a transform. Sets the following defaults
// preMultiplyFlag = 1
// stackSize = 10
// creates an identity matrix as the top matrix on the stack.

vtkTransform::vtkTransform ()
{
  // pre multiply is on
  this->PreMultiplyFlag = 1;
  // create a reasonable size stack
  this->StackSize = 10;
  // allocate the stack
  this->Stack = new vtkMatrix4x4 *[this->StackSize];
  // put a matrix on the top
  *this->Stack = new vtkMatrix4x4;
  // initialize the bottom of the stack
  this->StackBottom = this->Stack;
  // initialize current matrix to identity
  this->Identity ();

  this->Point[0] = this->Point[1] = this->Point[2] = this->Point[3] = 0.0;
  this->Orientation[0] = this->Orientation[1] = this->Orientation[2] = 0.0;

  this->Modified ();
}

// Description:
// Copy constructor
vtkTransform::vtkTransform (const vtkTransform& t)
{
  int i;
  vtkMatrix4x4 *stack;

  this->PreMultiplyFlag = t.PreMultiplyFlag;
  this->StackSize = t.StackSize;
  this->Stack = new vtkMatrix4x4 *[this->StackSize];

  // now copy each matrix in the stack
  for (stack = *this->Stack, i = 0; i < this->StackSize; i++)
    {
    this->Stack[i] = new vtkMatrix4x4(*(t.Stack[i]));
    }

  this->StackBottom = this->Stack + (this->StackSize - 1);
}

// Description:
// Deletes the transformation on the top of the stack and sets the top 
// to the next transformation on the stack.

void vtkTransform::Pop ()
{
  // if we're at the bottom of the stack, don't pop
  if (this->Stack == this->StackBottom) return;

  // free the stack matrix storage
  (*this->Stack)->Delete();
  *this->Stack = NULL;

  // update the stack
  this->Stack--;

  this->Modified ();
}

// Description:
// Sets the internal state of the transform to
// post multiply. All matrix subsequent matrix
// operations will occur after those already represented
// in the current transformation matrix.
void vtkTransform::PostMultiply ()
{
  if (this->PreMultiplyFlag != 0) {
    this->PreMultiplyFlag = 0;
    this->Modified ();
  }
}

// Description:
// Sets the internal state of the transform to
// pre multiply. All matrix subsequent matrix
// operations will occur before those already represented
// in the current transformation matrix.
void vtkTransform::PreMultiply ()
{
  if (this->PreMultiplyFlag != 1) {
    this->PreMultiplyFlag = 1;
    this->Modified ();
  }
}

// Description:
// Pushes the current transformation matrix onto the
// transformation stack.
void vtkTransform::Push ()
{
  vtkMatrix4x4 ctm;

  ctm = **this->Stack;
  this->Stack++;
  if ((this->Stack - this->StackBottom) > this->StackSize) {
    this->Stack--;
    return;
  }
  // allocate a new matrix on the stack

  *this->Stack = new vtkMatrix4x4;

  // set the new matrix to the previous top of stack matrix
  **this->Stack = ctm;

  this->Modified ();
}

// Description:
// Creates an x rotation matrix andn concatenates it with 
// the current transformation matrix.
void vtkTransform::RotateX ( float angle)
{
  vtkMatrix4x4 ctm;
  vtkMath math;
  float radians = angle * math.DegreesToRadians();
  float cosAngle, sinAngle;

  if (angle != 0.0) 
    {
    cosAngle = cos (radians);
    sinAngle = sin (radians);

    ctm.Element[0][0] = 1.0;
    ctm.Element[1][1] =  cosAngle;
    ctm.Element[2][1] =  sinAngle;
    ctm.Element[1][2] = -sinAngle;
    ctm.Element[2][2] =  cosAngle;
    ctm.Element[3][3] = 1.0;

    // concatenate with current transformation matrix
    this->Concatenate (ctm);
    }
}

// Description:
// Rotate about y-axis
void vtkTransform::RotateY ( float angle)
  //  Creates a y rotation matrix and concatenates it with 
  //  the current transformation matrix.
{
  vtkMatrix4x4 ctm;
  vtkMath math;
  float radians = angle * math.DegreesToRadians();
  float cosAngle, sinAngle;

  if (angle != 0.0) 
    {
    cosAngle = cos (radians);
    sinAngle = sin (radians);

    ctm.Element[0][0] = cosAngle;
    ctm.Element[1][1] = 1.0;
    ctm.Element[2][0] = -sinAngle;
    ctm.Element[0][2] = sinAngle;
    ctm.Element[2][2] = cosAngle;
    ctm.Element[3][3] = 1.0;

    // concatenate with current transformation matrix
    this->Concatenate (ctm);
    }
}

// Description:
// Rotate about y-axis
void vtkTransform::RotateZ (float angle)
  //  Creates a z rotation matrix and concatenates it with 
  //  the current transformation matrix.
{
  vtkMatrix4x4 ctm;
  vtkMath math;
  float radians = angle * math.DegreesToRadians();
  float cosAngle, sinAngle;

  if (angle != 0.0) 
    {
    cosAngle = cos (radians);
    sinAngle = sin (radians);

    ctm.Element[0][0] =  cosAngle;
    ctm.Element[1][0] =  sinAngle;
    ctm.Element[0][1] = -sinAngle;
    ctm.Element[1][1] =  cosAngle;
    ctm.Element[2][2] = 1.0;
    ctm.Element[3][3] = 1.0;

    // concatenate with current transformation matrix
    this->Concatenate (ctm);
    }
}

// Description:
// Creates a matrix that rotates angle degrees about an axis
// through the origin and x, y, z. Then concatenates
// this matrix with the current transformation matrix.
void vtkTransform::RotateWXYZ ( float angle, float x, float y, float z)
{
  vtkMatrix4x4 ctm;
  float   radians;
  float   w;
  float   quat[4];
  float   sinAngle;
  float   cosAngle;
  vtkMath math;

  // build a rotation matrix and concatenate it
  quat[0] = angle;
  quat[1] = x;
  quat[2] = y;
  quat[3] = z;

  // convert degrees to radians
  radians = quat[0] * math.DegreesToRadians() / 2;

  cosAngle = cos (radians);
  sinAngle = sin (radians);

  // normalize x, y, z
  if ( math.Normalize(quat+1) == 0.0 )
    {
    vtkErrorMacro(<<"Trying to rotate around zero-length axis");
    return;
    }

  w = cosAngle;
  x = quat[1] * sinAngle;
  y = quat[2] * sinAngle;
  z = quat[3] * sinAngle;

  // matrix calculation is taken from Ken Shoemake's
  // "Animation Rotation with Quaternion Curves",
  // Comput. Graphics, vol. 19, No. 3 , p. 253

  ctm.Element[0][0] = 1 - 2 * y * y - 2 * z * z;
  ctm.Element[1][1] = 1 - 2 * x * x - 2 * z * z;
  ctm.Element[2][2] = 1 - 2 * x * x - 2 * y * y;
  ctm.Element[1][0] =  2 * x * y + 2 * w * z;
  ctm.Element[2][0] =  2 * x * z - 2 * w * y;
  ctm.Element[0][1] =  2 * x * y - 2 * w * z;
  ctm.Element[2][1] =  2 * y * z + 2 * w * x;
  ctm.Element[0][2] =  2 * x * z + 2 * w * y;
  ctm.Element[1][2] =  2 * y * z - 2 * w * x;

  // concatenate with current transformation matrix
  this->Concatenate (ctm);
}

// Description:
// Scale in x, y, z directions using current transformation matrix.
void vtkTransform::Scale ( float x, float y, float z)
{
  vtkMatrix4x4 ctm;

  if (x != 1.0 || y != 1.0 || z != 1.0) 
    {
    ctm.Element[0][0] = x;
    if (ctm.Element[0][0] == 0.0) 
      {
      vtkErrorMacro(<< "scale: x scale is 0.0, reset to 1.0\n");
      ctm.Element[0][0] = 1.0;
      }

    ctm.Element[1][1] = y;
    if (ctm.Element[1][1] == 0.0) 
      {
      vtkErrorMacro(<<  "scale: y scale is 0.0, reset to 1.0\n");
      ctm.Element[1][1] = 1.0;
      }

    ctm.Element[2][2] = z;
    if (ctm.Element[2][2] == 0.0) 
      {
      vtkErrorMacro(<< "scale: z scale is 0.0, reset to 1.0\n");
      ctm.Element[2][2] = 1.0;
      }

    ctm.Element[3][3] = 1.0;

    // concatenate with current transformation matrix
    this->Concatenate (ctm);
    }
}

// Description:
// Translate in x, y, z directions using current transformation matrix.
void vtkTransform::Translate ( float x, float y, float z)
{
  vtkMatrix4x4 ctm;

  if (x != 0.0 || y != 0.0 || z != 0.0) 
    {
    ctm.Element[0][0] = 1.0;
    ctm.Element[1][1] = 1.0;
    ctm.Element[2][2] = 1.0;
    ctm.Element[3][3] = 1.0;

    ctm.Element[0][3] = x;
    ctm.Element[1][3] = y;
    ctm.Element[2][3] = z;

    // concatenate with current transformation matrix
    this->Concatenate (ctm);
    }
}

// Description:
// Obtain transpose of current transformation matrix.
void vtkTransform::GetTranspose (vtkMatrix4x4& (transpose))
{
  vtkMatrix4x4 temp;
  int i, j;

  for (i = 0; i < 4; i++) 
    {
    for (j = 0; j < 4; j++) 
      {
      temp.Element[j][i] = (**this->Stack).Element[i][j];
      }
    }    
  transpose = temp;
}

// Description:
// Invert current transformation matrix.
void vtkTransform::Inverse ()
{
  (**this->Stack).Invert (**this->Stack, **this->Stack);

  this->Modified ();
}

// Description:
// Return inverse of current transformation matrix.
void vtkTransform::GetInverse ( vtkMatrix4x4& inverse)
{
  inverse.Invert (**this->Stack, inverse);
}

// Description:
// Get the x, y, z orientation angles from transformation matrix.
void vtkTransform::GetOrientation(float& rx, float& ry, float &rz)
{
  float *orientation=this->GetOrientation();
  rx = orientation[0];
  ry = orientation[1];
  rz = orientation[2];
}

// Description:
// Get the x, y, z orientation angles from transformation matrix as an array
// of three floating point values.
float *vtkTransform::GetOrientation ()
{
#define AXIS_EPSILON .01
  float	scale_x, scale_y, scale_z;
  vtkMatrix4x4  temp;
  float   x,y,z;
  float   d;
  float   d1;
  float   d2;
  float   dot;
  float   alpha;
  float   phi;
  float   theta;
  float   cos_phi, sin_phi;
  float   cos_theta, sin_theta;
  float   cos_alpha, sin_alpha;
  float   x2, y2, z2;
  float   x3, y3, z3;
  float   x3p, y3p;
  vtkMath math;

  // copy the matrix into local storage

  temp = **this->Stack;

  // get scale factors

  this->GetScale (scale_x, scale_y, scale_z);

  // first rotate about y axis

  x2 = temp.Element[2][0] / scale_x;
  y2 = temp.Element[2][1] / scale_y;
  z2 = temp.Element[2][2] / scale_z;

  x3 = temp.Element[1][0] / scale_x;
  y3 = temp.Element[1][1] / scale_y;
  z3 = temp.Element[1][2] / scale_z;

  dot = x2 * x2 + z2 * z2;
  d1 = sqrt (dot);

  if (d1 < AXIS_EPSILON) 
    {
    cos_theta = 1.0;
    sin_theta = 0.0;
    }
  else 
    {
    cos_theta = z2 / d1;
    sin_theta = x2 / d1;
    }

  theta = atan2 (sin_theta, cos_theta);

  y = -theta / math.DegreesToRadians();

  // now rotate about x axis

  dot = x2 * x2 + y2 * y2 + z2 * z2;
  d = sqrt (dot);

  if (d < AXIS_EPSILON) 
    {
    sin_phi = 0.0;
    cos_phi = 1.0;
    }
  else if (d1 < AXIS_EPSILON) 
    {
    sin_phi = y2 / d;
    cos_phi = z2 / d;
    }
  else 
    {
    sin_phi = y2 / d;
    cos_phi = ( x2 * x2 + z2 * z2) / (d1 * d);
    }

  phi = atan2 (sin_phi, cos_phi);

  x = phi / math.DegreesToRadians();

  // finally, rotate about z
  x3p = x3 * cos_theta - z3 * sin_theta;
  y3p = - sin_phi * sin_theta * x3 + cos_phi * y3 - sin_phi * cos_theta * z3;
  dot = x3p * x3p + y3p * y3p;

  d2 = sqrt (dot);
  if (d2 < AXIS_EPSILON) 
    {
    cos_alpha = 1.0;
    sin_alpha = 0.0;
    }
  else 
    {
    cos_alpha = y3p / d2;
    sin_alpha = x3p / d2;
    }

  alpha = atan2 (sin_alpha, cos_alpha);

  z = alpha / math.DegreesToRadians();

  this->Orientation[0] = x;
  this->Orientation[1] = y;
  this->Orientation[2] = z;

  return this->Orientation;
}

// Description:
// Return the x, y, z positions from the current transformation matrix.
void vtkTransform::GetPosition (float & x,float & y,float & z)
{
  x = (**this->Stack).Element[0][3];
  y = (**this->Stack).Element[1][3];
  z = (**this->Stack).Element[2][3];
}

// Description:
// Return the position from the current transformation matrix as an array
// of three floating point numbers.
float *vtkTransform::GetPosition()
{
  static float pos[3];

  pos[0] = (**this->Stack).Element[0][3];
  pos[1] = (**this->Stack).Element[1][3];
  pos[2] = (**this->Stack).Element[2][3];

  return pos;
}

// Description:
// Return the x, y, z scale factors of the current transformation matrix.
void vtkTransform::GetScale (float& x, float& y, float& z)
{
  float *scale=this->GetScale();

  x = scale[0];
  y = scale[1];
  z = scale[2];
}

// Description:
// Return the scale factors of the current transformation matrix as an
// array of three float numbers.
float *vtkTransform::GetScale()
{
  int	i;
  static float scale[3];
  vtkMatrix4x4 temp;

  // copy the matrix into local storage

  temp = **this->Stack;

  // find scale factors

  for (i = 0; i < 3; i++) 
    {
    scale[i] = sqrt (temp.Element[i][0] * temp.Element[i][0] +
                     temp.Element[i][1] * temp.Element[i][1] +
                     temp.Element[i][2] * temp.Element[i][2]);
    }

  return scale;
}

// Description:
// Returns the current transformation matrix.
vtkMatrix4x4 & vtkTransform::GetMatrix ()
{
  return **this->Stack;;
}

// Description:
// Set the matrix directly.
void vtkTransform::SetMatrix(vtkMatrix4x4& m)
{
  **this->Stack = m;
}

// Description:
// Creates an identity matrix and makes it the current transformation matrix.
void vtkTransform::Identity ()
{
  vtkMatrix4x4 ctm;
  int i;

  ctm = 0.0;

  for (i = 0; i < 4; i++) 
    {
    ctm.Element[i][i] = 1.0;
    }
  **this->Stack = ctm;
}

// Description:
// Concatenates input matrix with the current transformation matrix.
// The resulting matrix becomes the new current transformation matrix.

void vtkTransform::Concatenate (vtkMatrix4x4 & matrix)
{
  if (this->PreMultiplyFlag) 
    {
    this->Multiply4x4 (**this->Stack, matrix, **this->Stack);
    }
  else 
    {
    this->Multiply4x4 (matrix, **this->Stack, **this->Stack);
    }
  this->Modified ();
}

// Description:
// Multiplies matrices a and b and stores result in c.
void vtkTransform::Multiply4x4 ( vtkMatrix4x4 & a, vtkMatrix4x4 & b, vtkMatrix4x4 & c)
{
  int i, j, k;
  vtkMatrix4x4 result;

  result = 0.0;
  for (i = 0; i < 4; i++) 
    {
    for (k = 0; k < 4; k++) 
      {
      for (j = 0; j < 4; j++) 
        {
        result.Element[i][k] += a.Element[i][j] * b.Element[j][k];
        }
      }
    }
  c = result;
}

// Description:
// Transposes the current transformation matrix.

void vtkTransform::Transpose ()
{
  this->GetTranspose (**this->Stack);
}

// Description:
// Returns the current transformation matrix.

void vtkTransform::GetMatrix (vtkMatrix4x4 & ctm)
{
  ctm = **this->Stack;
}

// Description:
// Destructor. Deletes all matrices on the stack and the stack

vtkTransform::~vtkTransform ()
{
  // delete all matrices on the stack

  while (this->Stack != this->StackBottom) this->Pop ();

  // delete the bottom matrix
  (*this->Stack)->Delete();

  // delete the stack itself
  delete this->Stack;
}

void vtkTransform::PrintSelf (ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);

  os << indent << "Current Transformation:" << "\n";

  (**this->Stack).PrintSelf (os, indent.GetNextIndent());
}

// Description:
// Returns point transformed by the current transformation matrix. Point is
// expressed in homogeneous coordinates.
float *vtkTransform::GetPoint()
{
  if (this->PreMultiplyFlag)
    {
    this->Stack[0]->Transpose();
    this->Stack[0]->PointMultiply(this->Point,this->Point);
    this->Stack[0]->Transpose();
    }
  else
    {
    this->Stack[0]->PointMultiply(this->Point,this->Point);
    }
  return this->Point;
}

void vtkTransform::GetPoint(float p[4])
{
  float *x=this->vtkTransform::GetPoint();
  for (int i=0; i<4; i++) p[i] = x[i];
}

// Description:
// Multiplies list of points (inPts) by current transformation matrix.
// Transformed points are appended to output list (outPts).
void vtkTransform::MultiplyPoints(vtkPoints *inPts, vtkPoints *outPts)
{
  float newX[3];
  float *x;
  int ptId, i;
  int numPts = inPts->GetNumberOfPoints();

  for (ptId=0; ptId < numPts; ptId++)
    {
    x = inPts->GetPoint(ptId);
    for (i=0; i<3; i++)
      {
      newX[i] = (**this->Stack).Element[i][0] * x[0] +
                (**this->Stack).Element[i][1] * x[1] +
                (**this->Stack).Element[i][2] * x[2] +
                (**this->Stack).Element[i][3];
      }

    outPts->InsertNextPoint(newX);
    }
}

// Description:
// Multiplies list of vectors (inVectors) by current transformation matrix. 
// Transformed vectors are appended to output list (outVectors).
// Special multiplication since these are vectors. Multiplies vectors
// by the transposed inverse of the matrix, ignoring the translational
// components.

void vtkTransform::MultiplyVectors(vtkVectors *inVectors, vtkVectors *outVectors)
{
  float newV[3];
  float *v;
  int ptId, i;
  int numVectors = inVectors->GetNumberOfVectors();
  vtkMath math;

  this->Push();
  this->Inverse();
  this->Transpose();

  for (ptId=0; ptId < numVectors; ptId++)
    {
    v = inVectors->GetVector(ptId);
    for (i=0; i<3; i++)
      {
      newV[i] = (**this->Stack).Element[i][0] * v[0] +
                (**this->Stack).Element[i][1] * v[1] +
                (**this->Stack).Element[i][2] * v[2];
      }

    math.Normalize(newV);
    outVectors->InsertNextVector(newV);
    }
  this->Pop();
}

// Description:
// Multiplies list of normals (inNormals) by current transformation matrix.
// Transformed normals are appended to output list (outNormals).
// Special multiplication since these are vectors. Multiplies vectors
// by the transposed inverse of the matrix, ignoring the translational
// components.

void vtkTransform::MultiplyNormals(vtkNormals *inNormals, vtkNormals *outNormals)
{
  float newN[3];
  float *n;
  int ptId, i;
  int numNormals = inNormals->GetNumberOfNormals();
  vtkMath math;

  this->Push();
  this->Inverse();
  this->Transpose();

  for (ptId=0; ptId < numNormals; ptId++)
    {
    n = inNormals->GetNormal(ptId);
    for (i=0; i<3; i++)
      {
      newN[i] = (**this->Stack).Element[i][0] * n[0] +
                (**this->Stack).Element[i][1] * n[1] +
                (**this->Stack).Element[i][2] * n[2];
      }

    math.Normalize(newN);
    outNormals->InsertNextNormal(newN);
    }
  this->Pop();
}
