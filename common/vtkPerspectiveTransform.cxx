/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerspectiveTransform.cxx
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

#include "vtkPerspectiveTransform.h"
#include "vtkPerspectiveTransformInverse.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
void vtkPerspectiveTransform::PrintSelf(ostream& os, vtkIndent indent)
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
template <class T2, class T3>
static inline void vtkPerspectiveTransformPoint(const double M[4][4],
					        T2 in[3], T3 out[3])
{
  T3 x = M[0][0]*in[0] + M[0][1]*in[1] + M[0][2]*in[2] + M[0][3];
  T3 y = M[1][0]*in[0] + M[1][1]*in[1] + M[1][2]*in[2] + M[1][3];
  T3 z = M[2][0]*in[0] + M[2][1]*in[1] + M[2][2]*in[2] + M[2][3];
  T3 w = M[3][0]*in[0] + M[3][1]*in[1] + M[3][2]*in[2] + M[3][3];

  T3 f = T3(1.0)/w;
  out[0] = x*f; 
  out[1] = y*f; 
  out[2] = z*f; 
}

//------------------------------------------------------------------------
void vtkPerspectiveTransform::TransformPoint(const float in[3], 
					     float out[3])
{
  this->Update();

  vtkPerspectiveTransformPoint(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------

void vtkPerspectiveTransform::TransformPoint(const double in[3], 
					     double out[3])
{
  this->Update();

  vtkPerspectiveTransformPoint(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkPerspectiveTransform::InternalTransformPoint(const float in[3], 
						     float out[3])
{
  vtkPerspectiveTransformPoint(this->Matrix->Element,in,out);
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransform::InternalTransformDerivative(const float in[3], 
						    float out[3],
						    float derivative[3][3])
{
  double (*M)[4] = this->Matrix->Element;
  
  float x = M[0][0]*in[0] + M[0][1]*in[1] + M[0][2]*in[2] + M[0][3];
  float y = M[1][0]*in[0] + M[1][1]*in[1] + M[1][2]*in[2] + M[1][3];
  float z = M[2][0]*in[0] + M[2][1]*in[1] + M[2][2]*in[2] + M[2][3];
  float w = M[3][0]*in[0] + M[3][1]*in[1] + M[3][2]*in[2] + M[3][3];

  float f = 1.0f/w;
  out[0] = x*f; 
  out[1] = y*f; 
  out[2] = z*f; 

  for (int i = 0; i < 3; i++)
    { 
    derivative[0][i] = (M[0][i] - M[3][i]*out[0])*f;
    derivative[1][i] = (M[1][i] - M[3][i]*out[1])*f;
    derivative[2][i] = (M[2][i] - M[3][i]*out[2])*f;
    }
}

//----------------------------------------------------------------------------
void vtkPerspectiveTransform::TransformPoints(vtkPoints *inPts, 
					      vtkPoints *outPts)
{
  int n = inPts->GetNumberOfPoints();
  double (*M)[4] = this->Matrix->Element;
  float point[3];  

  this->Update();

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,point);

    vtkPerspectiveTransformPoint(M,point,point);

    outPts->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the 
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform. 
void vtkPerspectiveTransform::TransformPointsNormalsVectors(vtkPoints *inPts, 
							    vtkPoints *outPts,
							    vtkNormals *inNms, 
							    vtkNormals *outNms,
							    vtkVectors *inVrs, 
							    vtkVectors *outVrs)
{
  int n = inNms->GetNumberOfNormals();
  double (*M)[4] = this->Matrix->Element;
  double L[4][4];
  float inPnt[3],outPnt[3],inNrm[3],outNrm[3],inVec[3],outVec[3];
  float w;
  float f;
  
  this->Update();

  if (inNms)
    { // need inverse transpose of matrix to calculate normals
    vtkMatrix4x4::DeepCopy(*L,this->Matrix);  
    vtkMatrix4x4::Invert(*L,*L);
    vtkMatrix4x4::Transpose(*L,*L);
    }

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,inPnt);

    // do the linear homogenous transformation
    outPnt[0] = M[0][0]*inPnt[0]+M[0][1]*inPnt[1]+M[0][2]*inPnt[2]+M[0][3];
    outPnt[1] = M[1][0]*inPnt[0]+M[1][1]*inPnt[1]+M[1][2]*inPnt[2]+M[1][3];
    outPnt[2] = M[2][0]*inPnt[0]+M[2][1]*inPnt[1]+M[2][2]*inPnt[2]+M[2][3];
    w =         M[3][0]*inPnt[0]+M[3][1]*inPnt[1]+M[3][2]*inPnt[2]+M[3][3];

    // apply perspective correction
    f = 1.0f/w;
    outPnt[0] *= f;
    outPnt[1] *= f;
    outPnt[2] *= f;

    outPts->InsertNextPoint(outPnt);

    if (inVrs)
      { 
      inVrs->GetVector(i,inVec);

      // do the linear homogenous transformation
      outVec[0] = M[0][0]*inVec[0] + M[0][1]*inVec[1] + M[0][2]*inVec[2];
      outVec[1] = M[1][0]*inVec[0] + M[1][1]*inVec[1] + M[1][2]*inVec[2];
      outVec[2] = M[2][0]*inVec[0] + M[2][1]*inVec[1] + M[2][2]*inVec[2];
      w =         M[3][0]*inVec[0] + M[3][1]*inVec[1] + M[3][2]*inVec[2];

      // apply perspective correction: note that the f we are using
      // is the one we calculated in the point transformation
      outVec[0] = (outVec[0]-w*outPnt[0])*f;
      outVec[1] = (outVec[1]-w*outPnt[1])*f;
      outVec[2] = (outVec[2]-w*outPnt[2])*f;

      outVrs->InsertNextVector(outVec);
      }

    if (inNms)
      { 
      inNms->GetNormal(i,inNrm);

      // calculate the w component of the normal
      w = -(inNrm[0]*inPnt[0] + inNrm[1]*inPnt[1] + inNrm[2]*inPnt[2]);

      // perform the transformation in homogenous coordinates
      outNrm[0] = L[0][0]*inNrm[0]+L[0][1]*inNrm[1]+L[0][2]*inNrm[2]+L[0][3]*w;
      outNrm[1] = L[1][0]*inNrm[0]+L[1][1]*inNrm[1]+L[1][2]*inNrm[2]+L[1][3]*w;
      outNrm[2] = L[2][0]*inNrm[0]+L[2][1]*inNrm[1]+L[2][2]*inNrm[2]+L[2][3]*w;

      // re-normalize
      vtkMath::Normalize(outNrm);
      outNms->InsertNextNormal(outNrm);
      }
    }
}

//----------------------------------------------------------------------------
// The vtkPerspectiveTransformInverse is a special-purpose class.
// See vtkPerspectiveTransformInverse.h for more details.
vtkGeneralTransform *vtkPerspectiveTransform::GetInverse()
{
  if (this->MyInverse == NULL)
    {
    vtkPerspectiveTransformInverse *inverse = 
      vtkPerspectiveTransformInverse::New();
    inverse->SetInverse(this);
    this->MyInverse = inverse;
    }
  return (vtkPerspectiveTransform *)this->MyInverse;
}

//----------------------------------------------------------------------------
// update and copy out the current matrix
void vtkPerspectiveTransform::GetMatrix(vtkMatrix4x4 *m)
{
  this->Update(); 
  m->DeepCopy(this->Matrix); 
}


