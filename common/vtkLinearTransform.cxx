/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearTransform.cxx
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

#include "vtkLinearTransform.h"
#include "vtkLinearTransformInverse.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
void vtkLinearTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGeneralTransform::PrintSelf(os, indent);
  if (this->Matrix)
    {
    os << indent << "Matrix: " << this->Matrix << "\n";
    this->Matrix->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "Matrix: (none)" << "\n";
    }
}

//------------------------------------------------------------------------
template <class T>
static inline void vtkLinearTransformPoint(const T in[3], T out[3], 
					   double matrix[4][4])
{
  T x = in[0];
  T y = in[1];
  T z = in[2];

  out[0] = matrix[0][0]*x + matrix[0][1]*y + matrix[0][2]*z + matrix[0][3];
  out[1] = matrix[1][0]*x + matrix[1][1]*y + matrix[1][2]*z + matrix[1][3];
  out[2] = matrix[2][0]*x + matrix[2][1]*y + matrix[2][2]*z + matrix[2][3];
}

//------------------------------------------------------------------------
template <class T>
static inline void vtkLinearTransformNormal(const T in[3], T out[3], 
					    double mat[4][4])
{
  // to transform the normal, multiply by the transposed inverse matrix
  double matrix[4][4];
  memcpy(matrix,mat,16*sizeof(double));
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  T x = in[0];
  T y = in[1];
  T z = in[2];

  out[0] = matrix[0][0]*x + matrix[0][1]*y + matrix[0][2]*z;
  out[1] = matrix[1][0]*x + matrix[1][1]*y + matrix[1][2]*z;
  out[2] = matrix[2][0]*x + matrix[2][1]*y + matrix[2][2]*z;
}

//------------------------------------------------------------------------
template <class T>
static inline void vtkLinearTransformVector(const T in[3], T out[3], 
					    double matrix[4][4])
{
  T x = in[0];
  T y = in[1];
  T z = in[2];

  out[0] = matrix[0][0]*x + matrix[0][1]*y + matrix[0][2]*z;
  out[1] = matrix[1][0]*x + matrix[1][1]*y + matrix[1][2]*z;
  out[2] = matrix[2][0]*x + matrix[2][1]*y + matrix[2][2]*z;
}

//------------------------------------------------------------------------
void vtkLinearTransform::TransformPoint(const float in[3], float out[3])
{
  this->Update();

  vtkLinearTransformPoint(in,out,this->Matrix->Element);
}

//------------------------------------------------------------------------
void vtkLinearTransform::TransformPoint(const double in[3], double out[3])
{
  this->Update();

  vtkLinearTransformPoint(in,out,this->Matrix->Element);
}

//------------------------------------------------------------------------
void vtkLinearTransform::TransformNormal(const float in[3], float out[3])
{
  this->Update();

  vtkLinearTransformNormal(in,out,this->Matrix->Element);
}

//------------------------------------------------------------------------
void vtkLinearTransform::TransformNormal(const double in[3], double out[3])
{
  this->Update();

  vtkLinearTransformNormal(in,out,this->Matrix->Element);
}

//------------------------------------------------------------------------
void vtkLinearTransform::TransformVector(const float in[3], float out[3])
{
  this->Update();

  vtkLinearTransformVector(in,out,this->Matrix->Element);
}

//------------------------------------------------------------------------

void vtkLinearTransform::TransformVector(const double in[3], double out[3])
{
  this->Update();

  vtkLinearTransformVector(in,out,this->Matrix->Element);
}

//----------------------------------------------------------------------------
// These four functions are definitely not thread safe, and should
// really only be called from python or tcl.
float *vtkLinearTransform::TransformFloatNormal(float x, 
						float y, 
						float z)
{
  this->InternalFloatPoint[0] = x;
  this->InternalFloatPoint[1] = y;
  this->InternalFloatPoint[2] = z;
  this->TransformNormal(this->InternalFloatPoint,this->InternalFloatPoint);
  return this->InternalFloatPoint;
}

//----------------------------------------------------------------------------
double *vtkLinearTransform::TransformDoubleNormal(double x,
						 double y,
						 double z)
{
  this->InternalDoublePoint[0] = x;
  this->InternalDoublePoint[1] = y;
  this->InternalDoublePoint[2] = z;
  this->TransformNormal(this->InternalDoublePoint,this->InternalDoublePoint);
  return this->InternalDoublePoint;
}

//----------------------------------------------------------------------------
float *vtkLinearTransform::TransformFloatVector(float x, 
						float y, 
						float z)
{
  this->InternalFloatPoint[0] = x;
  this->InternalFloatPoint[1] = y;
  this->InternalFloatPoint[2] = z;
  this->TransformVector(this->InternalFloatPoint,this->InternalFloatPoint);
  return this->InternalFloatPoint;
}

//----------------------------------------------------------------------------
double *vtkLinearTransform::TransformDoubleVector(double x,
						  double y,
						  double z)
{
  this->InternalDoublePoint[0] = x;
  this->InternalDoublePoint[1] = y;
  this->InternalDoublePoint[2] = z;
  this->TransformVector(this->InternalDoublePoint,this->InternalDoublePoint);
  return this->InternalDoublePoint;
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformPoint(const float in[3], 
						float out[3])
{
  vtkLinearTransformPoint(in,out,this->Matrix->Element);
}

//----------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformDerivative(const float in[3], 
						     float out[3],
						     float derivative[3][3])
{
  double (*matrix)[4] = this->Matrix->Element;

  vtkLinearTransformPoint(in,out,matrix);

  for (int i = 0; i < 3; i++)
    {
    derivative[i][0] = matrix[i][0];
    derivative[i][1] = matrix[i][1];
    derivative[i][2] = matrix[i][2];
    }
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the 
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform. 
void vtkLinearTransform::TransformPointsNormalsVectors(vtkPoints *inPts, 
						       vtkPoints *outPts,
						       vtkNormals *inNms, 
						       vtkNormals *outNms,
						       vtkVectors *inVrs, 
						       vtkVectors *outVrs)
{
  this->TransformPoints(inPts,outPts);
  if (inNms)
    {
    this->TransformNormals(inNms,outNms);
    }
  if (inVrs)
    {
    this->TransformVectors(inVrs,outVrs);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformPoints(vtkPoints *inPts, 
					 vtkPoints *outPts)
{
  int n = inPts->GetNumberOfPoints();
  double (*matrix)[4] = this->Matrix->Element;
  float point[3];  

  this->Update();

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,point);

    vtkLinearTransformPoint(point,point,matrix);

    outPts->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformNormals(vtkNormals *inNms, 
					  vtkNormals *outNms)
{
  int n = inNms->GetNumberOfNormals();
  float norm[3];
  double matrix[4][4];
  
  this->Update();

  // to transform the normal, multiply by the transposed inverse matrix
  vtkMatrix4x4::DeepCopy(*matrix,this->Matrix);  
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  for (int i = 0; i < n; i++)
    {
    inNms->GetNormal(i,norm);

    // use TransformVector because matrix is already transposed & inverted
    vtkLinearTransformVector(norm,norm,matrix);
    vtkMath::Normalize(norm);

    outNms->InsertNextNormal(norm);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformVectors(vtkVectors *inNms, 
					  vtkVectors *outNms)
{
  int n = inNms->GetNumberOfVectors();
  float vec[3];
  
  this->Update();

  double (*matrix)[4] = this->Matrix->Element;

  for (int i = 0; i < n; i++)
    {
    inNms->GetVector(i,vec);

    vtkLinearTransformVector(vec,vec,matrix);

    outNms->InsertNextVector(vec);
    }
}

//----------------------------------------------------------------------------
// The vtkLinearTransformInverse is a special-purpose class.
// See vtkLinearTransformInverse.h for more details.
vtkGeneralTransform *vtkLinearTransform::GetInverse()
{
  if (this->MyInverse == NULL)
    {
    vtkLinearTransformInverse *inverse = vtkLinearTransformInverse::New();
    inverse->SetInverse(this);
    this->MyInverse = inverse;
    }
  return (vtkLinearTransform *)this->MyInverse;
}

