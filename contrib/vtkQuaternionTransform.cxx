/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuaternionTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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

#include "vtkQuaternionTransform.h"
#include "vtkLinearTransformInverse.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkQuaternionTransform* vtkQuaternionTransform::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuaternionTransform");
  if(ret)
    {
    return (vtkQuaternionTransform*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuaternionTransform;
}

//----------------------------------------------------------------------------
vtkQuaternionTransform::vtkQuaternionTransform()
{
  this->TransformType = VTK_QUATERNION_TRANSFORM;

  this->Quaternion[0] = 1.0;
  this->Quaternion[1] = 0.0;
  this->Quaternion[2] = 0.0;
  this->Quaternion[3] = 0.0;

  this->Magnification = 1.0; 

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;

  int i,j;
  for (i = 0; i < 3; i++)
    {
    for (j = 0; j < 3; j++)
      {
      this->Matrix3x3[i][j] = 0.0;
      }
    this->Matrix3x3[i][i] = 1.0;
    }

  this->MatrixNeedsUpdate = 0;

  this->PreMultiplyFlag = 1;
}

//----------------------------------------------------------------------------
vtkQuaternionTransform::~vtkQuaternionTransform()
{
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLinearTransform::PrintSelf(os, indent);

  os << indent << "Quaternion: " << "( " <<
     this->Quaternion[0] << ", " << this->Quaternion[1] << ", " <<
     this->Quaternion[2] << ", " << this->Quaternion[3] << ")\n";

  os << indent << "Magnification: " << this->Magnification << "\n";

  os << indent << "Position: " << "( " <<
     this->Position[0] << ", " << this->Position[1] << ", " <<
     this->Position[2] << ")\n";

  os << indent << (this->PreMultiplyFlag ? "PreMultiply\n" : "PostMultiply\n");
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkQuaternionTransform::MakeTransform()
{
  return vtkQuaternionTransform::New();
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::DeepCopy(vtkGeneralTransform *transform)
{
  if (this->TransformType != transform->GetTransformType() &&
      this->TransformType != transform->GetInverse()->GetTransformType())
    {
    vtkErrorMacro(<< "DeepCopy: trying to copy a transform of different type");
    }
  if (transform->GetTransformType() & VTK_INVERSE_TRANSFORM)
    {
    transform = ((vtkLinearTransformInverse *)transform)->GetTransform(); 
    }	
  vtkQuaternionTransform *t = (vtkQuaternionTransform *)transform;

  if (t == this)
    {
    return;
    }

  this->SetPosition(t->Position);
  this->SetMagnification(t->Magnification);
  this->SetQuaternion(t->Quaternion);

  this->PreMultiplyFlag = t->PreMultiplyFlag;

  int i,j;
  for (i = 0; i < 3; i++)
    {
    for (j = 0; j < 3; j++)
      {
      this->Matrix3x3[i][j] = t->Matrix3x3[i][j];
      }
    }

  this->Matrix->DeepCopy(t->Matrix);

  this->MatrixNeedsUpdate = 1;
}

//----------------------------------------------------------------------------
// Update the 3x3 and 4x4 matrices according to the Quaternion,
// Scale and Position.  Updates are only done as necessary.
 
void vtkQuaternionTransform::Update()
{
  if (!this->MatrixNeedsUpdate)
    {
    return;
    }

  double (*matrix)[4] = this->Matrix->Element;
  double (*matrix3x3)[3] = this->Matrix3x3;

  // update the matrix orientation from the quaternion
  if (this->MatrixNeedsUpdate & VTK_QUATERNION_QUATERNION)
    {
    vtkQuaternionTransform::QuaternionToMatrix(this->Quaternion,matrix3x3);
    }			       

  // update the matrix scale
  if (this->MatrixNeedsUpdate & (VTK_QUATERNION_MAGNIFICATION | 
				 VTK_QUATERNION_QUATERNION))
    {
    double scale = this->Magnification;

    matrix[0][0] = matrix3x3[0][0] * scale;
    matrix[1][0] = matrix3x3[1][0] * scale;
    matrix[2][0] = matrix3x3[2][0] * scale;

    matrix[0][1] = matrix3x3[0][1] * scale;
    matrix[1][1] = matrix3x3[1][1] * scale;
    matrix[2][1] = matrix3x3[2][1] * scale;

    matrix[0][2] = matrix3x3[0][2] * scale;
    matrix[1][2] = matrix3x3[1][2] * scale;
    matrix[2][2] = matrix3x3[2][2] * scale;
    }

  // update the translation of the matrix
  if (this->MatrixNeedsUpdate & VTK_QUATERNION_POSITION)
    {
    double *position = this->Position;

    matrix[0][3] = position[0];
    matrix[1][3] = position[1];
    matrix[2][3] = position[2];
    }

  this->Matrix->Modified();
  this->MatrixNeedsUpdate = 0;
}

//----------------------------------------------------------------------------
// create an identity transformation.

inline void vtkQuaternionTransform::Identity()
{ 
  this->Quaternion[0] = 1.0;
  this->Quaternion[1] = 0.0;
  this->Quaternion[2] = 0.0;
  this->Quaternion[3] = 0.0;

  this->Magnification = 1.0; 

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;

  this->Matrix->Identity();
   
  int i,j;
  for (i = 0; i < 3; i++)
    {
    for (j = 0; j < 3; j++)
      {
      this->Matrix3x3[i][j] = 0.0;
      }
    this->Matrix3x3[i][i] = 1.0;
    }

  this->MatrixNeedsUpdate = 0;
  
  this->Modified();
}

//----------------------------------------------------------------------------
// Invert the transformation and update the 4x4 matrix.  The quaternion
// is inverted exactly, while the scale and position are inverted with
// very high precision.

void vtkQuaternionTransform::Inverse()
{
  vtkQuaternionTransform::QuaternionConjugate(this->Quaternion);
  this->MatrixNeedsUpdate |= VTK_QUATERNION_QUATERNION;

  this->Magnification = 1.0/this->Magnification;
  this->MatrixNeedsUpdate |= VTK_QUATERNION_MAGNIFICATION;

  this->Update();

  double (*matrix)[4] = this->Matrix->Element;
  double *position = this->Position;

  double x = -position[0];
  double y = -position[1];
  double z = -position[2];
  
  matrix[0][3] = position[0] = matrix[0][0]*x+matrix[0][1]*y+matrix[0][2]*z;
  matrix[1][3] = position[1] = matrix[1][0]*x+matrix[1][1]*y+matrix[1][2]*z;
  matrix[2][3] = position[2] = matrix[2][0]*x+matrix[2][1]*y+matrix[2][2]*z;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::Concatenate(vtkQuaternionTransform *transform)
{
  double *quaternion = transform->GetQuaternion();
  double scale = transform->GetMagnification();
  double *position = transform->GetPosition();

  if (this->PreMultiplyFlag == 0)
    {
    transform->TransformPoint(this->Position,this->Position);
    this->MatrixNeedsUpdate |= VTK_QUATERNION_POSITION;

    if (quaternion[0] != 1.0) // check for identity quaternion
      { 
      vtkQuaternionTransform::QuaternionMultiply(quaternion,
						 this->Quaternion,
						 this->Quaternion);
      vtkQuaternionTransform::QuaternionNormalize(this->Quaternion);
      this->MatrixNeedsUpdate |= VTK_QUATERNION_QUATERNION;


      }
    }
  else
    {
    this->TransformPoint(position,this->Position);
    this->MatrixNeedsUpdate |= VTK_QUATERNION_POSITION;

    if (fabs(quaternion[0]) != 1.0) // check for identity quaternion
      { 
      vtkQuaternionTransform::QuaternionMultiply(this->Quaternion,
						 quaternion,
						 this->Quaternion);
      vtkQuaternionTransform::QuaternionNormalize(this->Quaternion);
      this->MatrixNeedsUpdate |= VTK_QUATERNION_QUATERNION;
      }
    }

  if (scale != 1.0)
    {
    this->Magnification *= scale;
    this->MatrixNeedsUpdate |= VTK_QUATERNION_MAGNIFICATION;
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::Translate(double x, double y, double z)
{
  if (this->PreMultiplyFlag == 0)
    {
    this->Position[0] += x;
    this->Position[1] += y;
    this->Position[2] += z;
    }
  else
    {
    double position[3];
    position[0] = x;
    position[1] = y;
    position[2] = z;
    this->TransformPoint(position,this->Position);
    }
  this->MatrixNeedsUpdate |= VTK_QUATERNION_POSITION;
  this->Update();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::Scale(double scale)
{
  if (this->PreMultiplyFlag == 0)
    {
    this->Position[0] *= scale;
    this->Position[1] *= scale;
    this->Position[2] *= scale;
    this->MatrixNeedsUpdate |= VTK_QUATERNION_POSITION;
    }
  this->Magnification *= scale;
  this->MatrixNeedsUpdate |= VTK_QUATERNION_MAGNIFICATION;
  this->Update();
  this->Modified(); 
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::InterpolateTransform(vtkQuaternionTransform *t1,
						  vtkQuaternionTransform *t2,
						  double f)
{
  double r = 1.0-f;

  // use spherical linear interpolation for the quaternion
  vtkQuaternionTransform::QuaternionSlerp(t1->GetQuaternion(),
					  t2->GetQuaternion(),
					  this->Quaternion,
					  f);

  // use geometric linear interpolation for the magnification 
  this->Magnification = exp(r*log(t1->GetMagnification()) +
			    f*log(t2->GetMagnification()));

  // use plain-old linear interpolation for the position
  double *p1 = t1->GetPosition();
  double *p2 = t2->GetPosition();
  double *position = this->Position;

  position[0] = p1[0]*r + p2[0]*f;
  position[1] = p1[1]*r + p2[1]*f;
  position[2] = p1[2]*r + p2[2]*f;

  this->MatrixNeedsUpdate = VTK_QUATERNION_QUATERNION |
                            VTK_QUATERNION_MAGNIFICATION | 
                            VTK_QUATERNION_POSITION;
  this->Update();
  this->Modified();
}

//----------------------------------------------------------------------------
// apply a quaternion rotation to the transform
void vtkQuaternionTransform::RotateQuaternion(double w, 
					      double x, double y, double z)
{
  double quat[4];
  
  quat[0] = w;
  quat[1] = x;
  quat[2] = y;
  quat[3] = z;

  double *position = this->Position;

  if (this->PreMultiplyFlag == 0)
    {
    vtkQuaternionTransform::QuaternionMultiply(quat,this->Quaternion,
					       this->Quaternion);
    vtkQuaternionTransform::QuaternionNormalize(this->Quaternion);

    x = position[0];
    y = position[1];
    z = position[2];
  
    double matrix[3][3];
    vtkQuaternionTransform::QuaternionToMatrix(quat,matrix);

    position[0] = matrix[0][0]*x + matrix[0][1]*y + matrix[0][2]*z;
    position[1] = matrix[1][0]*x + matrix[1][1]*y + matrix[1][2]*z;
    position[2] = matrix[2][0]*x + matrix[2][1]*y + matrix[2][2]*z;

    this->MatrixNeedsUpdate |= VTK_QUATERNION_POSITION;
    }
  else
    {
    vtkQuaternionTransform::QuaternionMultiply(this->Quaternion,quat,
					       this->Quaternion);
    vtkQuaternionTransform::QuaternionNormalize(this->Quaternion);
    }

  this->MatrixNeedsUpdate |= VTK_QUATERNION_QUATERNION;
  this->Update();
  this->Modified();
} 

//----------------------------------------------------------------------------
void vtkQuaternionTransform::RotateWXYZ(double theta, 
					double x, double y, double z)
{
  theta *= vtkMath::DoubleDegreesToRadians();
  double f = sin(0.5*theta)/sqrt(x*x+y*y+z*z);
  this->RotateQuaternion(cos(0.5*theta),x*f,y*f,z*f);
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::RotateX(double theta)
{
  theta *= vtkMath::DoubleDegreesToRadians();
  this->RotateQuaternion(cos(0.5*theta),sin(0.5*theta),0.0,0.0);
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::RotateY(double theta)
{
  theta *= vtkMath::DoubleDegreesToRadians();
  this->RotateQuaternion(cos(0.5*theta),0.0,sin(0.5*theta),0.0);
}

//----------------------------------------------------------------------------
void vtkQuaternionTransform::RotateZ(double theta)
{
  theta *= vtkMath::DoubleDegreesToRadians();
  this->RotateQuaternion(cos(0.5*theta),0.0,0.0,sin(0.5*theta));
}

//----------------------------------------------------------------------------
// Static method to find the product of two quaternions.  

void vtkQuaternionTransform::QuaternionMultiply(double q1[4], double q2[4],
						double result[4])
{
  double w1 = q1[0];
  double x1 = q1[1];
  double y1 = q1[2];
  double z1 = q1[3];

  double w2 = q2[0];
  double x2 = q2[1];
  double y2 = q2[2];
  double z2 = q2[3];

  result[0] = w1*w2 - x1*x2 - y1*y2 - z1*z2;
  result[1] = w1*x2 + x1*w2 + y1*z2 - z1*y2;
  result[2] = w1*y2 - x1*z2 + y1*w2 + z1*x2;
  result[3] = w1*z2 + x1*y2 - y1*x2 + z1*w2;
}

//----------------------------------------------------------------------------
// Static method to normalize a quaternion.  The resulting w value will 
// always be positive.

void vtkQuaternionTransform::QuaternionNormalize(double q[4])
{
  double r = sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
  if (q[0] < 0)
    {
    r = -r;
    }
  q[0] /= r;
  q[1] /= r;
  q[2] /= r;
  q[3] /= r;
}

//----------------------------------------------------------------------------
// Static method to find the conjugate of a quaternion.  The conjugate of
// a normalized quaternion is also its inverse.

void vtkQuaternionTransform::QuaternionConjugate(double q[4])
{
  q[1] = -q[1];
  q[2] = -q[2];
  q[3] = -q[3];
}

//----------------------------------------------------------------------------
// Static method to convert a normalized quaternion to an 
// orthogonal 3x3 rotation matrix.
// If the quaternion is not normalized, then the matrix will have an
// additional scale factor equal to (w*w + x*x + y*y + z*z).

void vtkQuaternionTransform::QuaternionToMatrix(double quat[4], 
						double matrix3x3[3][3])
{
  double w = quat[0];
  double x = quat[1];
  double y = quat[2];
  double z = quat[3];

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

  matrix3x3[0][0] = ww + xx - yy - zz; 
  matrix3x3[1][0] = 2.0*(wz + xy);
  matrix3x3[2][0] = 2.0*(-wy + xz);
  
  matrix3x3[0][1] = 2.0*(-wz + xy);  
  matrix3x3[1][1] = ww - xx + yy - zz;
  matrix3x3[2][1] = 2.0*(wx + yz);
  
  matrix3x3[0][2] = 2.0*(wy + xz);
  matrix3x3[1][2] = 2.0*(-wx + yz);
  matrix3x3[2][2] = ww - xx - yy + zz;
}

//----------------------------------------------------------------------------
// Do spherical linear interpolation between quaternion rotations.
void vtkQuaternionTransform::QuaternionSlerp(double q1[4], double q2[4],
					     double result[4], double fraction)
{
  double cosTheta = q1[0]*q2[0] + q1[1]*q2[1] + q1[2]*q2[2] + q1[3]*q2[3];
  double sinTheta = sqrt(1.0-cosTheta*cosTheta);
  double theta = atan2(sinTheta,cosTheta);
  double r = 1.0;
  double f = 0.0;
  if (sinTheta != 0.0)
    {
    r = sin((1.0-fraction)*theta)/sinTheta;
    f = sin(fraction*theta)/sinTheta;
    }

  result[0] = q1[0]*r + q2[0]*f;
  result[1] = q1[1]*r + q2[1]*f;
  result[2] = q1[2]*r + q2[2]*f;
  result[3] = q1[3]*r + q2[3]*f;
}

