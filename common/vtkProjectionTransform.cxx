/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectionTransform.cxx
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
OF THIS EVEN, SOFTWARE IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdlib.h>
#include "vtkProjectionTransform.h"
#include "vtkPerspectiveTransformInverse.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

// Useful for viewing a double[16] as a double[4][4]
typedef double (*SqMatPtr)[4];


//----------------------------------------------------------------------------
vtkProjectionTransform* vtkProjectionTransform::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProjectionTransform");
  if(ret)
    {
    return (vtkProjectionTransform*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProjectionTransform;
}

//----------------------------------------------------------------------------
vtkProjectionTransform::vtkProjectionTransform()
{
  this->PreMultiplyFlag = 1;
}

//----------------------------------------------------------------------------
vtkProjectionTransform::~vtkProjectionTransform()
{
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPerspectiveTransform::PrintSelf(os, indent);
  os << indent << (this->PreMultiplyFlag ? "PreMultiply\n" : "PostMultiply\n");
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkProjectionTransform::MakeTransform()
{
  return vtkProjectionTransform::New();
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::DeepCopy(vtkGeneralTransform *transform)
{
  if (strcmp("vtkPerspectiveTransformInverse",transform->GetClassName())==0)
    {
    transform = ((vtkPerspectiveTransformInverse *)transform)->GetTransform();
    }
  if (strcmp("vtkProjectionTransform",transform->GetClassName()) != 0)
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    return;
    }

  vtkProjectionTransform *t = (vtkProjectionTransform *)transform;  

  if (t == this)
    {
    return;
    }

  this->PreMultiplyFlag = t->PreMultiplyFlag;
  this->Matrix->DeepCopy(t->Matrix);
  this->Modified();
}

//----------------------------------------------------------------------------
// Creates an identity matrix.
void vtkProjectionTransform::Identity()
{
  this->Matrix->Identity();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::Inverse()
{ 
  this->Matrix->Invert();
  this->Modified(); 
}

//----------------------------------------------------------------------------
// Set the current matrix directly.
void vtkProjectionTransform::SetMatrix(const double Elements[16])
{
  this->Matrix->DeepCopy(Elements);
  this->Modified();
}

//----------------------------------------------------------------------------
// Concatenates the input matrix with the current matrix.
// The setting of the PreMultiply flag determines whether the matrix
// is PreConcatenated or PostConcatenated.
void vtkProjectionTransform::Concatenate(const double Elements[16])
{
  if (this->PreMultiplyFlag) 
    {
    vtkMatrix4x4::Multiply4x4(*this->Matrix->Element, Elements, 
			      *this->Matrix->Element);
    }
  else 
    {
    vtkMatrix4x4::Multiply4x4(Elements, *this->Matrix->Element, 
			      *this->Matrix->Element);
    }
  this->Matrix->Modified();
  this->Modified();
}

//----------------------------------------------------------------------------
// Sets the internal state of the ProjectionTransform to
// post multiply. All subsequent matrix
// operations will occur after those already represented
// in the current ProjectionTransformation matrix.
void vtkProjectionTransform::PostMultiply()
{
  if (this->PreMultiplyFlag != 0) 
    {
    this->PreMultiplyFlag = 0;
    this->Modified ();
    }
}

//----------------------------------------------------------------------------
// Sets the internal state of the ProjectionTransform to
// pre multiply. All subsequent matrix
// operations will occur before those already represented
// in the current ProjectionTransformation matrix.
void vtkProjectionTransform::PreMultiply()
{
  if (this->PreMultiplyFlag != 1) 
    {
    this->PreMultiplyFlag = 1;
    this->Modified ();
    }
}

//----------------------------------------------------------------------------
// Utility for adjusting the window range to a new one.  Usually the
// previous range was ([-1,+1],[-1,+1]) as per Ortho and Frustum, and you
// are mapping them to display coordinates.
void vtkProjectionTransform::AdjustViewport(double oldXMin, double oldXMax, 
					    double oldYMin, double oldYMax,
					    double newXMin, double newXMax, 
					    double newYMin, double newYMax)
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = (newXMax - newXMin)/(oldXMax - oldXMin);
  matrix[1][1] = (newYMax - newYMin)/(oldYMax - oldYMin);

  matrix[0][3] = (newXMin*oldXMax - newXMax*oldXMin)/(oldXMax - oldXMin);
  matrix[1][3] = (newYMin*oldYMax - newYMax*oldYMin)/(oldYMax - oldYMin);

  this->Concatenate(*matrix);
}  

//----------------------------------------------------------------------------
// Utility for adjusting the min/max range of the Z buffer.  Usually
// the oldZMin, oldZMax are [-1,+1] as per Ortho and Frustum, and
// you are mapping the Z buffer to a new range.
void vtkProjectionTransform::AdjustZBuffer(double oldZMin, double oldZMax,
					   double newZMin, double newZMax)
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[2][2] = (newZMax - newZMin)/(oldZMax - oldZMin);
  matrix[2][3] = (newZMin*oldZMax - newZMax*oldZMin)/(oldZMax - oldZMin);

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
// The orthographic projection maps [xmin,xmax], [ymin,ymax], [-znear,-zfar]
// to [-1,+1], [-1,+1], [-1,+1].
// From the OpenGL Programmer's guide, 2nd Ed.
void vtkProjectionTransform::Ortho(double xmin, double xmax,
				   double ymin, double ymax,
				   double znear, double zfar)
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = 2/(xmax - xmin);
  matrix[1][1] = 2/(ymax - ymin);
  matrix[2][2] = -2/(zfar - znear);
  
  matrix[0][3] = -(xmin + xmax)/(xmax - xmin);
  matrix[1][3] = -(ymin + ymax)/(ymax - ymin);
  matrix[2][3] = -(znear + zfar)/(zfar - znear);

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
// The frustrum projection maps a frustum with the front plane at -znear
// which has extent [xmin,xmax],[ymin,ymax] and a back plane at -zfar
// to [-1,+1], [-1,+1], [-1,+1].
// From the OpenGL Programmer's guide, 2nd Ed.
void vtkProjectionTransform::Frustum(double xmin, double xmax,
				     double ymin, double ymax,
				     double znear, double zfar)
{
  double matrix[4][4];

  matrix[0][0] =  2*znear/(xmax - xmin);
  matrix[1][0] =  0;
  matrix[2][0] =  0;
  matrix[3][0] =  0;

  matrix[0][1] =  0;
  matrix[1][1] =  2*znear/(ymax - ymin);
  matrix[2][1] =  0;
  matrix[3][1] =  0;

  matrix[0][2] =  (xmin + xmax)/(xmax - xmin);
  matrix[1][2] =  (ymin + ymax)/(ymax - ymin);
  matrix[2][2] = -(znear + zfar)/(zfar - znear);
  matrix[3][2] = -1;

  matrix[0][3] =  0;
  matrix[1][3] =  0;
  matrix[2][3] = -2*znear*zfar/(zfar - znear);
  matrix[3][3] =  0;

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::Perspective(double angle, double aspect,
					 double znear, double zfar)
{
  double ymax =  tan(angle*vtkMath::DoubleDegreesToRadians()/2)*znear;
  double ymin = -ymax; 

  double xmax =  ymax*aspect;
  double xmin = -xmax;

  this->Frustum(xmin, xmax, ymin, ymax, znear, zfar);
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::Stereo(double angle, double focaldistance)
{
  double shear = tan(angle*vtkMath::DoubleDegreesToRadians());

  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);
  
  // create a shear in Z
  matrix[0][2] = -shear;
  
  // shift by the separation between the eyes
  matrix[0][3] = -shear*focaldistance;

  // concatenate with the current matrix
  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::SetupCamera(const double position[3],
					 const double focalPoint[3],
					 const double viewUp[3])
{
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  // the view directions correspond to the rows of the rotation matrix,
  // so we'll make the connection explicit
  double *viewSideways =    matrix[0];
  double *orthoViewUp =     matrix[1];
  double *viewPlaneNormal = matrix[2]; 

  // set the view plane normal from the view vector
  viewPlaneNormal[0] = position[0] - focalPoint[0];
  viewPlaneNormal[1] = position[1] - focalPoint[1];
  viewPlaneNormal[2] = position[2] - focalPoint[2];
  vtkMath::Normalize(viewPlaneNormal);

  // orthogonalize viewUp and compute viewSideways
  vtkMath::Cross(viewUp,viewPlaneNormal,viewSideways);
  vtkMath::Normalize(viewSideways);
  vtkMath::Cross(viewPlaneNormal,viewSideways,orthoViewUp);

  // translate by the vector from the position to the origin
  double delta[4];
  delta[0] = -position[0];
  delta[1] = -position[1];
  delta[2] = -position[2];
  delta[3] = 0.0; // yes, this should be zero, not one

  vtkMatrix4x4::MultiplyPoint(*matrix,delta,delta);

  matrix[0][3] = delta[0]; 
  matrix[1][3] = delta[1]; 
  matrix[2][3] = delta[2]; 

  // apply the transformation
  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::Translate(double x, double y, double z)
{
  if (x == 0.0 && y == 0.0 && z == 0.0) 
    {
    return;
    }

  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][3] = x;
  matrix[1][3] = y;
  matrix[2][3] = z;

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::RotateWXYZ(double angle, 
					double x, double y, double z)
{
  if (x == 0.0 && y == 0.0 && z == 0.0) 
    {
    vtkErrorMacro(<<"Trying to rotate around zero-length axis");
    return;
    }

  if (angle == 0)
    {
    return;
    }

  // convert to radians
  angle = angle*vtkMath::DoubleDegreesToRadians();

  // make a normalized quaternion
  double w = cos(0.5*angle);
  double f = sin(0.5*angle)/sqrt(x*x+y*y+z*z);
  x *= f;
  y *= f;
  z *= f;

  // convert the quaternion to a matrix
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  double ww = w*w;
  double wx = w*x;
  double wy = w*y;
  double wz = w*z;

  double xx = x*x;
  double yy = y*y;
  double zz = z*z;

  double xy = x*y;
  double xz = x*z;
  double yz = y*z;

  double ss = (ww - xx - yy - zz)/2;

  matrix[0][0] = ( ss + xx)*2;
  matrix[1][0] = ( wz + xy)*2;
  matrix[2][0] = (-wy + xz)*2;

  matrix[0][1] = (-wz + xy)*2;
  matrix[1][1] = ( ss + yy)*2;
  matrix[2][1] = ( wx + yz)*2;

  matrix[0][2] = ( wy + xz)*2;
  matrix[1][2] = (-wx + yz)*2;
  matrix[2][2] = ( ss + zz)*2;

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
void vtkProjectionTransform::Scale(double x, double y, double z)
{
  if (x == 0.0 && y == 0.0 && z == 0.0) 
    {
    return;
    }

  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = x;
  matrix[1][1] = y;
  matrix[2][2] = z;

  this->Concatenate(*matrix);
}

  
