/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuaternionTransform.h
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
// .NAME vtkQuaternionTransform - a linear transform that preserves angles
// .SECTION Description
// A vtkQuaternionTransform is guaranteed to preserve all angles.  It is a
// linear transformation consisting of a (w,x,y,z) quaternion rotation, 
// a scale factor, and an (x,y,z) translation.  It provides higher 
// precision than a 4x4 matrix transformation, at the cost of flexibility.
// .SECTION see also
// vtkTransform

#ifndef __vtkQuaternionTransform_h
#define __vtkQuaternionTransform_h

#include "vtkLinearTransform.h"

#define VTK_QUATERNION_QUATERNION    0x1
#define VTK_QUATERNION_MAGNIFICATION 0x2
#define VTK_QUATERNION_POSITION      0x4

class VTK_EXPORT vtkQuaternionTransform : public vtkLinearTransform
{
public:
  static vtkQuaternionTransform *New();

  vtkTypeMacro(vtkQuaternionTransform,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the quaternion rotation for the transformation.  The quaternion
  // must be normalized and the w value must be positive. 
  void SetQuaternion(double w, double x, double y, double z);
  void SetQuaternion(double *quaternion);
  vtkGetVector4Macro(Quaternion,double);

  // Description:
  // Get the scale for the transform.  If the scale is 1, then this
  // is a rigid-body transformation.
  void SetMagnification(double scale);
  vtkGetMacro(Magnification,double);

  // Description:
  // Get the position of the transform (this is the translation applied 
  // after the rotation and scale). 
  void SetPosition(double x, double y, double z);
  void SetPosition(double *position);
  vtkGetVector3Macro(Position,double);

  // Description:
  // Translate the position by (x,y,z). 
  void Translate(double x, double y, double z);

  // Description:
  // Scale the transformation. 
  void Scale(double scale);

  // Description:
  // Rotate around the specified axis by the specified angle (in degrees).
  void RotateWXYZ(double theta, double x, double y, double z);
  void RotateX(double theta);
  void RotateY(double theta);
  void RotateZ(double theta);

  // Description:
  // Concatenate with another quaternion transformation.
  void Concatenate(vtkQuaternionTransform *transform);

  // Description:
  // When using Translate, Scale, Rotate*, or Concatenate the
  // PreMultiply flag determines whether the specified transformation
  // should be applied before or after the current transformation.
  // The default is PreMultiply, i.e. apply the specified transformation
  // before the current transformation.
  void PreMultiply();
  void PostMultiply();

  // Description:
  // Set this transform to be an interpolation between two other
  // transforms.
  void InterpolateTransform(vtkQuaternionTransform *t1,
			    vtkQuaternionTransform *t2,
			    double fraction);

  // Description:
  // Make this into an identity transformation. 
  void Identity();

  // Description:
  // Invert the transformation. 
  void Inverse();

  // Description:
  // Make another transform of the same type.
  vtkGeneralTransform *MakeTransform();

  // Description:
  // Copy this transform from another of the same type.
  void DeepCopy(vtkGeneralTransform *transform);

  // Update the matrix from the quaternion.
  void Update();

protected:
  vtkQuaternionTransform();
  ~vtkQuaternionTransform();
  vtkQuaternionTransform(const vtkQuaternionTransform&) {};
  void operator=(const vtkQuaternionTransform&) {};

  void RotateQuaternion(double w, double x, double y, double z);

//BTX
  // Description:
  // A few static functions for dealing with quaternions.
  static void QuaternionMultiply(double quat1[4], double quat2[4], 
				 double result[4]);
  static void QuaternionNormalize(double quat[4]);
  static void QuaternionConjugate(double quat[4]);
  static void QuaternionToMatrix(double quat[4], double matrix3x3[3][3]);
  static void QuaternionSlerp(double q1[4], double q2[4], 
			      double result[4], double fraction); 
//ETX

  double Quaternion[4];
  double Magnification;
  double Position[3];

  double Matrix3x3[3][3];

  int PreMultiplyFlag;

  int MatrixNeedsUpdate;
};

//----------------------------------------------------------------------------
inline void vtkQuaternionTransform::SetQuaternion(double w, double x, 
						  double y, double z)
{
  if (this->Quaternion[0] == w &&
      this->Quaternion[1] == x &&
      this->Quaternion[2] == y &&
      this->Quaternion[3] == z)
    {
    return;
    }

  this->Quaternion[0] = w;
  this->Quaternion[1] = x;
  this->Quaternion[2] = y;
  this->Quaternion[3] = z;

  this->MatrixNeedsUpdate |= VTK_QUATERNION_QUATERNION;
  this->Update();
  this->Modified();
}

//----------------------------------------------------------------------------
inline void vtkQuaternionTransform::SetQuaternion(double quat[4])
{
  this->SetQuaternion(quat[0],quat[1],quat[2],quat[3]);
}

//----------------------------------------------------------------------------
inline void vtkQuaternionTransform::SetMagnification(double scale)
{
  if (this->Magnification == scale)
    {
    return;
    }

  this->Magnification = scale;
  this->MatrixNeedsUpdate |= VTK_QUATERNION_MAGNIFICATION;
  this->Update();
  this->Modified();
}

//----------------------------------------------------------------------------
inline void vtkQuaternionTransform::SetPosition(double x, double y, double z)
{
  if (this->Position[0] == x &&
      this->Position[1] == y &&
      this->Position[2] == z)
    {
    return;
    }

  this->Position[0] = x;
  this->Position[1] = y;
  this->Position[2] = z;

  this->MatrixNeedsUpdate |= VTK_QUATERNION_POSITION;
  this->Update();
  this->Modified();
}

//----------------------------------------------------------------------------
inline void vtkQuaternionTransform::SetPosition(double pos[3])
{
  this->SetPosition(pos[0],pos[1],pos[2]);
}

//----------------------------------------------------------------------------
inline void vtkQuaternionTransform::PreMultiply()
{
  if (this->PreMultiplyFlag)
    {
    return;
    }
  this->PreMultiplyFlag = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
inline void vtkQuaternionTransform::PostMultiply()
{
  if (!this->PreMultiplyFlag)
    {
    return;
    }
  this->PreMultiplyFlag = 0;
  this->Modified();
}

#endif






