/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHomogeneousTransform.cxx
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

#include "vtkHomogeneousTransform.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
vtkHomogeneousTransform::vtkHomogeneousTransform()
{
  this->Matrix = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------------
vtkHomogeneousTransform::~vtkHomogeneousTransform()
{
  if (this->Matrix)
    {
    this->Matrix->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkAbstractTransform::PrintSelf(os, indent);
  os << indent << "Matrix: (" << this->Matrix << ")\n";
  if (this->Matrix)
    {
    this->Matrix->PrintSelf(os, indent.GetNextIndent());
    }
}

//------------------------------------------------------------------------
template <class T1, class T2, class T3>
static inline double vtkHomogeneousTransformPoint(T1 M[4][4],
						  T2 in[3], T3 out[3])
{
  double x = M[0][0]*in[0] + M[0][1]*in[1] + M[0][2]*in[2] + M[0][3];
  double y = M[1][0]*in[0] + M[1][1]*in[1] + M[1][2]*in[2] + M[1][3];
  double z = M[2][0]*in[0] + M[2][1]*in[1] + M[2][2]*in[2] + M[2][3];
  double w = M[3][0]*in[0] + M[3][1]*in[1] + M[3][2]*in[2] + M[3][3];

  double f = 1.0/w;
  out[0] = x*f; 
  out[1] = y*f; 
  out[2] = z*f;

  return f;
}

//------------------------------------------------------------------------
// computes a coordinate transformation and also returns the Jacobian matrix
template <class T1, class T2, class T3, class T4>
static inline void vtkHomogeneousTransformDerivative(T1 M[4][4],
						     T2 in[3], T3 out[3],
						     T4 derivative[3][3])
{
  double f = vtkHomogeneousTransformPoint(M,in,out);

  for (int i = 0; i < 3; i++)
    { 
    derivative[0][i] = (M[0][i] - M[3][i]*out[0])*f;
    derivative[1][i] = (M[1][i] - M[3][i]*out[1])*f;
    derivative[2][i] = (M[2][i] - M[3][i]*out[2])*f;
    }
}

//------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalTransformPoint(const float in[3], 
						     float out[3])
{
  vtkHomogeneousTransformPoint(this->Matrix->Element,in,out);
}

//------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalTransformPoint(const double in[3], 
						     double out[3])
{
  vtkHomogeneousTransformPoint(this->Matrix->Element,in,out);
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalTransformDerivative(const float in[3], 
						    float out[3],
						    float derivative[3][3])
{
  vtkHomogeneousTransformDerivative(this->Matrix->Element,in,out,derivative);
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalTransformDerivative(const double in[3], 
						    double out[3],
						    double derivative[3][3])
{
  vtkHomogeneousTransformDerivative(this->Matrix->Element,in,out,derivative);
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::TransformPoints(vtkPoints *inPts, 
					      vtkPoints *outPts)
{
  int n = inPts->GetNumberOfPoints();
  double (*M)[4] = this->Matrix->Element;
  double point[3];  

  this->Update();

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,point);

    vtkHomogeneousTransformPoint(M,point,point);

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
void vtkHomogeneousTransform::TransformPointsNormalsVectors(vtkPoints *inPts, 
							    vtkPoints *outPts,
							    vtkDataArray *inNms, 
							    vtkDataArray *outNms,
							    vtkDataArray *inVrs, 
							    vtkDataArray *outVrs)
{
  int n = inNms->GetNumberOfTuples();
  double (*M)[4] = this->Matrix->Element;
  double L[4][4];
  double inPnt[3],outPnt[3],inNrm[3],outNrm[3],inVec[3],outVec[3];
  double w;
  
  this->Update();

  if (inNms)
    { // need inverse of the matrix to calculate normals
    vtkMatrix4x4::DeepCopy(*L,this->Matrix);  
    vtkMatrix4x4::Invert(*L,*L);
    vtkMatrix4x4::Transpose(*L,*L);
    }

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,inPnt);

    // do the coordinate transformation, get 1/w
    double f = vtkHomogeneousTransformPoint(M,inPnt,outPnt);
    outPts->InsertNextPoint(outPnt);

    if (inVrs)
      { 
      inVrs->GetTuple(i,inVec);

      // do the linear homogeneous transformation
      outVec[0] = M[0][0]*inVec[0] + M[0][1]*inVec[1] + M[0][2]*inVec[2];
      outVec[1] = M[1][0]*inVec[0] + M[1][1]*inVec[1] + M[1][2]*inVec[2];
      outVec[2] = M[2][0]*inVec[0] + M[2][1]*inVec[1] + M[2][2]*inVec[2];
      w =         M[3][0]*inVec[0] + M[3][1]*inVec[1] + M[3][2]*inVec[2];

      // apply homogeneous correction: note that the f we are using
      // is the one we calculated in the point transformation
      outVec[0] = (outVec[0]-w*outPnt[0])*f;
      outVec[1] = (outVec[1]-w*outPnt[1])*f;
      outVec[2] = (outVec[2]-w*outPnt[2])*f;

      outVrs->InsertNextTuple(outVec);
      }

    if (inNms)
      { 
      inNms->GetTuple(i,inNrm);

      // calculate the w component of the normal
      w = -(inNrm[0]*inPnt[0] + inNrm[1]*inPnt[1] + inNrm[2]*inPnt[2]);

      // perform the transformation in homogeneous coordinates
      outNrm[0] = L[0][0]*inNrm[0]+L[0][1]*inNrm[1]+L[0][2]*inNrm[2]+L[0][3]*w;
      outNrm[1] = L[1][0]*inNrm[0]+L[1][1]*inNrm[1]+L[1][2]*inNrm[2]+L[1][3]*w;
      outNrm[2] = L[2][0]*inNrm[0]+L[2][1]*inNrm[1]+L[2][2]*inNrm[2]+L[2][3]*w;

      // re-normalize
      vtkMath::Normalize(outNrm);
      outNms->InsertNextTuple(outNrm);
      }
    }
}

//----------------------------------------------------------------------------
// update and copy out the current matrix
void vtkHomogeneousTransform::GetMatrix(vtkMatrix4x4 *m)
{
  this->Update(); 
  m->DeepCopy(this->Matrix); 
}

//----------------------------------------------------------------------------
void vtkHomogeneousTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkHomogeneousTransform *t = (vtkHomogeneousTransform *)transform;

  this->Matrix->DeepCopy(t->Matrix);
}

