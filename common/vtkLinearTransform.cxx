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
  os << indent << "Matrix: " << this->Matrix << "\n";
  this->Matrix->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------
template <class T>
static inline void vtkLinearTransformPoint(T in[3], T out[3], 
					   double matrix[4][4])
{
  float x = in[0];
  float y = in[1];
  float z = in[2];

  out[0] = matrix[0][0]*x + matrix[0][1]*y + matrix[0][2]*z + matrix[0][3];
  out[1] = matrix[1][0]*x + matrix[1][1]*y + matrix[1][2]*z + matrix[1][3];
  out[2] = matrix[2][0]*x + matrix[2][1]*y + matrix[2][2]*z + matrix[2][3];
}

//------------------------------------------------------------------------
void vtkLinearTransform::TransformPoint(const float in[3], float out[3])
{
  this->Update();

  vtkLinearTransformPoint((float *)in,out,this->Matrix->Element);
}

//------------------------------------------------------------------------

void vtkLinearTransform::TransformPoint(const double in[3], double out[3])
{
  this->Update();

  vtkLinearTransformPoint((double *)in,out,this->Matrix->Element);
}

//------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformPoint(const float in[3], 
						float out[3])
{
  vtkLinearTransformPoint((float *)in,out,this->Matrix->Element);
}

//----------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformDerivative(const float in[3], 
						     float out[3],
						     float derivative[3][3])
{
  double (*matrix)[4] = this->Matrix->Element;

  vtkLinearTransformPoint((float *)in,out,matrix);

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

    vtkLinearTransformPoint((float *)point,point,matrix);

    outPts->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformNormals(vtkNormals *inNms, 
					  vtkNormals *outNms)
{
  int n = inNms->GetNumberOfNormals();
  float in[3],out[3];
  double matrix[4][4];
  
  this->Update();

  vtkMatrix4x4::DeepCopy(*matrix,this->Matrix);  
  vtkMatrix4x4::Invert(*matrix,*matrix);
  vtkMatrix4x4::Transpose(*matrix,*matrix);

  for (int i = 0; i < n; i++)
    {
    inNms->GetNormal(i,in);

    out[0] = matrix[0][0]*in[0] + matrix[0][1]*in[1] + matrix[0][2]*in[2];
    out[1] = matrix[1][0]*in[0] + matrix[1][1]*in[1] + matrix[1][2]*in[2];
    out[2] = matrix[2][0]*in[0] + matrix[2][1]*in[1] + matrix[2][2]*in[2];
    vtkMath::Normalize(out);

    outNms->InsertNextNormal(out);
    }
}

//----------------------------------------------------------------------------
void vtkLinearTransform::TransformVectors(vtkVectors *inNms, 
					  vtkVectors *outNms)
{
  int n = inNms->GetNumberOfVectors();
  float in[3],out[3];
  
  this->Update();

  double (*matrix)[4] = this->Matrix->Element;

  for (int i = 0; i < n; i++)
    {
    inNms->GetVector(i,in);

    out[0] = matrix[0][0]*in[0] + matrix[0][1]*in[1] + matrix[0][2]*in[2];
    out[1] = matrix[1][0]*in[0] + matrix[1][1]*in[1] + matrix[1][2]*in[2];
    out[2] = matrix[2][0]*in[0] + matrix[2][1]*in[1] + matrix[2][2]*in[2];

    outNms->InsertNextVector(out);
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

