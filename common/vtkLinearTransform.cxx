/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinearTransform.cxx
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

#include "vtkLinearTransform.h"
#include "vtkMath.h"

//------------------------------------------------------------------------
void vtkLinearTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkHomogeneousTransform::PrintSelf(os, indent);
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
static inline void vtkLinearTransformPoint(T1 matrix[4][4], 
					   T2 in[3], T3 out[3])
{
  T3 x = matrix[0][0]*in[0]+matrix[0][1]*in[1]+matrix[0][2]*in[2]+matrix[0][3];
  T3 y = matrix[1][0]*in[0]+matrix[1][1]*in[1]+matrix[1][2]*in[2]+matrix[1][3];
  T3 z = matrix[2][0]*in[0]+matrix[2][1]*in[1]+matrix[2][2]*in[2]+matrix[2][3];

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3, class T4>
static inline void vtkLinearTransformDerivative(T1 matrix[4][4], 
						T2 in[3], T3 out[3], 
						T4 derivative[3][3])
{
  vtkLinearTransformPoint(matrix,in,out);

  for (int i = 0; i < 3; i++)
    {
    derivative[0][i] = matrix[0][i];
    derivative[1][i] = matrix[1][i];
    derivative[2][i] = matrix[2][i];
    }
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
static inline void vtkLinearTransformVector(T1 matrix[4][4],
					    T2 in[3], T3 out[3]) 
{
  T3 x = matrix[0][0]*in[0] + matrix[0][1]*in[1] + matrix[0][2]*in[2];
  T3 y = matrix[1][0]*in[0] + matrix[1][1]*in[1] + matrix[1][2]*in[2];
  T3 z = matrix[2][0]*in[0] + matrix[2][1]*in[1] + matrix[2][2]*in[2];

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
static inline void vtkLinearTransformNormal(T1 mat[4][4], 
					    T2 in[3], T3 out[3]) 
{
  // to transform the normal, multiply by the transposed inverse matrix
  T1 matrix[4][4];
  memcpy(*matrix,*mat,16*sizeof(T1)); 
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  vtkLinearTransformVector(matrix,in,out);

  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformPoint(const float in[3], 
						float out[3])
{
  vtkLinearTransformPoint(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformPoint(const double in[3], 
						double out[3])
{
  vtkLinearTransformPoint(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformNormal(const float in[3], 
						 float out[3])
{
  vtkLinearTransformNormal(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformNormal(const double in[3], 
						 double out[3])
{
  vtkLinearTransformNormal(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformVector(const float in[3], 
						 float out[3])
{
  vtkLinearTransformVector(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformVector(const double in[3], 
						 double out[3])
{
  vtkLinearTransformVector(this->Matrix->Element,in,out);
}

//----------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformDerivative(const float in[3], 
						     float out[3],
						     float derivative[3][3])
{
  vtkLinearTransformDerivative(this->Matrix->Element,in,out,derivative);
}

//----------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformDerivative(const double in[3], 
						     double out[3],
						     double derivative[3][3])
{
  vtkLinearTransformDerivative(this->Matrix->Element,in,out,derivative);
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
  double point[3];  

  this->Update();

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,point);

    vtkLinearTransformPoint(matrix,point,point);

    outPts->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformNormals(vtkNormals *inNms, 
					  vtkNormals *outNms)
{
  vtkIdType n = inNms->GetNumberOfNormals();
  double norm[3];
  double matrix[4][4];
  
  this->Update();

  // to transform the normal, multiply by the transposed inverse matrix
  vtkMatrix4x4::DeepCopy(*matrix,this->Matrix);  
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  for (vtkIdType i = 0; i < n; i++)
    {
    inNms->GetNormal(i,norm);

    // use TransformVector because matrix is already transposed & inverted
    vtkLinearTransformVector(matrix,norm,norm);
    vtkMath::Normalize(norm);

    outNms->InsertNextNormal(norm);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformNormals(vtkDataArray *inNms, 
					  vtkDataArray *outNms)
{
  int n = inNms->GetNumberOfTuples();
  double norm[3];
  double matrix[4][4];
  
  this->Update();

  // to transform the normal, multiply by the transposed inverse matrix
  vtkMatrix4x4::DeepCopy(*matrix,this->Matrix);  
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  for (int i = 0; i < n; i++)
    {
    inNms->GetTuple(i,norm);

    // use TransformVector because matrix is already transposed & inverted
    vtkLinearTransformVector(matrix,norm,norm);
    vtkMath::Normalize(norm);

    outNms->InsertNextTuple(norm);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformVectors(vtkVectors *inNms, 
					  vtkVectors *outNms)
{
  vtkIdType n = inNms->GetNumberOfVectors();
  double vec[3];
  
  this->Update();

  double (*matrix)[4] = this->Matrix->Element;

  for (vtkIdType i = 0; i < n; i++)
    {
    inNms->GetVector(i,vec);

    vtkLinearTransformVector(matrix,vec,vec);

    outNms->InsertNextVector(vec);
    }
}

