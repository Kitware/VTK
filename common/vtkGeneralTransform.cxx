/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransform.cxx
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

#include "vtkGeneralTransform.h"
#include "vtkGeneralTransformInverse.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
void vtkGeneralTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
}

//------------------------------------------------------------------------
// Update() and perform the transformation.
void vtkGeneralTransform::TransformPoint(const float input[3],
					 float output[3])
{
  this->Update();
  this->InternalTransformPoint(input,output);
}

//----------------------------------------------------------------------------
// Convert double to float, then do the transformation.  A subclass
// can override this method to provide true double-precision transformations.
void vtkGeneralTransform::TransformPoint(const double input[3], 
					 double output[3])
{
  float point[3];
  point[0] = input[0];
  point[1] = input[1];
  point[2] = input[2];
  this->TransformPoint(point,point);
  output[0] = point[0];
  output[1] = point[1];
  output[2] = point[2];
}

//----------------------------------------------------------------------------
// These two functions are definitely not thread safe, and should
// really only be called from python or tcl.
float *vtkGeneralTransform::TransformFloatPoint(float x, 
						float y, 
						float z)
{
  this->InternalFloatPoint[0] = x;
  this->InternalFloatPoint[1] = y;
  this->InternalFloatPoint[2] = z;
  this->TransformPoint(this->InternalFloatPoint,this->InternalFloatPoint);
  return this->InternalFloatPoint;
}

//----------------------------------------------------------------------------
double *vtkGeneralTransform::TransformDoublePoint(double x,
						  double y,
						  double z)
{
  this->InternalDoublePoint[0] = x;
  this->InternalDoublePoint[1] = y;
  this->InternalDoublePoint[2] = z;
  this->TransformPoint(this->InternalDoublePoint,this->InternalDoublePoint);
  return this->InternalDoublePoint;
}

//----------------------------------------------------------------------------
// Transform a series of points.
void vtkGeneralTransform::TransformPoints(vtkPoints *in, 
					  vtkPoints *out)
{
  this->Update();

  float point[3];
  int i;
  int n = in->GetNumberOfPoints();

  for (i = 0; i < n; i++)
    {
    in->GetPoint(i,point);
    this->InternalTransformPoint(point,point);
    out->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the 
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform. 
void vtkGeneralTransform::TransformPointsNormalsVectors(vtkPoints *inPts,
							vtkPoints *outPts,
							vtkNormals *inNms, 
							vtkNormals *outNms,
							vtkVectors *inVrs,
							vtkVectors *outVrs)
{
  this->Update();

  float matrix[3][3];
  float coord[3];

  int i;
  int n = inPts->GetNumberOfPoints();

  for (i = 0; i < n; i++)
    {
    inPts->GetPoint(i,coord);
    this->InternalTransformDerivative(coord,coord,matrix);
    outPts->InsertNextPoint(coord);
    
    if (inVrs)
      {
      inVrs->GetVector(i,coord);
      vtkGeneralTransform::Multiply3x3(matrix,coord,coord);
      outVrs->InsertNextVector(coord);
      }
    
    if (inNms)
      {
      inNms->GetNormal(i,coord);
      vtkGeneralTransform::Transpose3x3(matrix,matrix);
      vtkGeneralTransform::LinearSolve3x3(matrix,coord,coord);
      vtkMath::Normalize(coord);
      outNms->InsertNextNormal(coord);
      }
    }
}

//----------------------------------------------------------------------------
// The vtkGeneralTransformInverse is a special-purpose class.
// See vtkGeneralTransformInverse.h for more details.
vtkGeneralTransform *vtkGeneralTransform::GetInverse()
{
  if (this->MyInverse == NULL)
    {
    vtkGeneralTransformInverse *inverse = vtkGeneralTransformInverse::New();
    inverse->SetInverse(this);
    // we create a circular reference here, it is dealt with
    // in UnRegister
    this->MyInverse = inverse;
    }
  return this->MyInverse;
}

//----------------------------------------------------------------------------
// We need to handle the circular reference between a transform and its
// inverse.
void vtkGeneralTransform::UnRegister(vtkObject *o)
{
  if (this->InUnRegister)
    { // we don't want to go into infinite recursion...
    this->ReferenceCount--;
    return;
    }

  // see if 'this' is referenced by this->MyInverse
  if (this->MyInverse && this->MyInverse->GetInverse() == this &&
      this->ReferenceCount == 2 &&
      this->MyInverse->GetReferenceCount() == 1)
    { // break the cycle
    this->InUnRegister = 1;
    this->MyInverse->Delete();
    this->MyInverse = NULL;
    this->InUnRegister = 0;
    }

  this->vtkObject::UnRegister(o);
}

//----------------------------------------------------------------------------
// helper function, swap two 3-vectors
static inline void SwapVectors(float v1[3], float v2[3])
{
  float tmpvec[3];
  memcpy(tmpvec,v1,3*sizeof(float));
  memcpy(v1,v2,3*sizeof(float));
  memcpy(v2,tmpvec,3*sizeof(float));
}

//----------------------------------------------------------------------------
// Unrolled LU factorization of a 3x3 matrix with pivoting.
// This decomposition is non-standard in that the diagonal
// elements are inverted, to convert a division to a multiplication
// in the backsubstitution.
void vtkGeneralTransform::LUFactor3x3(float A[3][3], int index[3])
{
  int i,maxI;
  float tmp,largest;
  float scale[3];

  // Loop over rows to get implicit scaling information

  for ( i = 0; i < 3; i++ ) 
    {
    largest =  fabs(A[i][0]);
    if ((tmp = fabs(A[i][1])) > largest)
      {
      largest = tmp;
      }
    if ((tmp = fabs(A[i][2])) > largest)
      {
      largest = tmp;
      }
    scale[i] = 1.0f/largest;
    }
  
  // Loop over all columns using Crout's method

  // first column
  largest = scale[0]*fabs(A[0][0]);
  maxI = 0;
  if ((tmp = scale[1]*fabs(A[1][0])) >= largest) 
    {
    largest = tmp;
    maxI = 1;
    }
  if ((tmp = scale[2]*fabs(A[2][0])) >= largest) 
    {
    maxI = 2;
    }
  if (maxI != 0) 
    {
    SwapVectors(A[maxI],A[0]);
    scale[maxI] = scale[0];
    }
  index[0] = maxI;

  A[0][0] = 1.0f/A[0][0];
  A[1][0] *= A[0][0];
  A[2][0] *= A[0][0];
    
  // second column
  A[1][1] -= A[1][0]*A[0][1];
  A[2][1] -= A[2][0]*A[0][1];
  largest = scale[1]*fabs(A[1][1]);
  maxI = 1;
  if ((tmp = scale[2]*fabs(A[2][1])) >= largest) 
    {
    maxI = 2;
    SwapVectors(A[2],A[1]);
    scale[2] = scale[1];
    }
  index[1] = maxI;
  A[1][1] = 1.0f/A[1][1];
  A[2][1] *= A[1][1];

  // third column
  A[1][2] -= A[1][0]*A[0][2];
  A[2][2] -= A[2][0]*A[0][2] + A[2][1]*A[1][2];
  largest = scale[2]*fabs(A[2][2]);
  index[2] = 2;
  A[2][2] = 1.0f/A[2][2];
}

//----------------------------------------------------------------------------
// Backsubsitution with an LU-decomposed matrix.  This is the standard
// LU decomposition, except that the diagonals elements have been inverted.
void vtkGeneralTransform::LUSolve3x3(const float A[3][3], const int index[3], 
				     float x[3])
{
  float sum;

  // forward substitution
  
  sum = x[index[0]];
  x[index[0]] = x[0];
  x[0] = sum;

  sum = x[index[1]];
  x[index[1]] = x[1];
  x[1] = sum - A[1][0]*x[0];

  sum = x[index[2]];
  x[index[2]] = x[2];
  x[2] = sum - A[2][0]*x[0] - A[2][1]*x[1];

  // back substitution
  
  x[2] = x[2]*A[2][2];
  x[1] = (x[1] - A[1][2]*x[2])*A[1][1];
  x[0] = (x[0] - A[0][1]*x[1] - A[0][2]*x[2])*A[0][0];
}  

//----------------------------------------------------------------------------
// this method solves Ay = x for y
void vtkGeneralTransform::LinearSolve3x3(const float A[3][3], const float x[3],
					 float y[3])
{
  int index[3];
  float B[3][3];
  memcpy(B,A,9*sizeof(float));
  memcpy(y,x,3*sizeof(float));

  vtkGeneralTransform::LUFactor3x3(B,index);
  vtkGeneralTransform::LUSolve3x3(B,index,y);
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::Multiply3x3(const float A[3][3], const float v[3], 
				      float u[3])
{
  float x = v[0]; 
  float y = v[1]; 
  float z = v[2];

  u[0] = A[0][0]*x + A[0][1]*y + A[0][2]*z;
  u[1] = A[1][0]*x + A[1][1]*y + A[1][2]*z;
  u[2] = A[2][0]*x + A[2][1]*y + A[2][2]*z;
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::Multiply3x3(const float A[3][3], const float B[3][3],
				      float C[3][3])
{
  float D[3][3];
  memcpy(D,B,9*sizeof(float));

  for (int i = 0; i < 3; i++)
    {
    C[0][i] = A[0][0]*D[0][i] + A[0][1]*D[1][i] + A[0][2]*D[2][i];
    C[1][i] = A[1][0]*D[0][i] + A[1][1]*D[1][i] + A[1][2]*D[2][i];
    C[2][i] = A[2][0]*D[0][i] + A[2][1]*D[1][i] + A[2][2]*D[2][i];
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::Transpose3x3(const float A[3][3], float AT[3][3])
{
  float tmp;
  tmp = A[1][0];
  AT[1][0] = A[0][1];
  AT[0][1] = tmp;
  tmp = A[0][2];
  AT[2][0] = A[0][2];
  AT[0][2] = tmp;
  tmp = A[2][1];
  AT[2][1] = A[1][2];
  AT[1][2] = tmp;

  AT[0][0] = A[0][0];
  AT[1][1] = A[1][1];
  AT[2][2] = A[2][2];
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::Invert3x3(const float A[3][3], float AI[3][3])
{
  int index[3];
  float tmp[3][3];

  memcpy(AI,A,9*sizeof(float));

  // invert one column at a time
  vtkGeneralTransform::LUFactor3x3(AI,index);
  for (int i = 0; i < 3; i++)
    {
    float *x = tmp[i];
    x[0] = x[1] = x[2] = 0.0f;
    x[i] = 1.0f;
    vtkGeneralTransform::LUSolve3x3(AI,index,x);
    }
  for (int j = 0; j < 3; j++)
    {
    float *x = tmp[j];
    AI[0][j] = x[0];
    AI[1][j] = x[1];
    AI[2][j] = x[2];      
    }
}

//----------------------------------------------------------------------------
void vtkGeneralTransform::Identity3x3(float A[3][3])
{
  memset(A,0,9*sizeof(float));
  A[0][0] = A[1][1] = A[2][2] = 1.0f;
}


