/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdentityTransform.cxx
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

#include "vtkIdentityTransform.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
vtkIdentityTransform *vtkIdentityTransform::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkIdentityTransform");
  if(ret)
    {
    return (vtkIdentityTransform*)ret;
    }

  return new vtkIdentityTransform;
}

//----------------------------------------------------------------------------
vtkIdentityTransform::vtkIdentityTransform()
{
}

//----------------------------------------------------------------------------
vtkIdentityTransform::~vtkIdentityTransform()
{
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLinearTransform::PrintSelf(os, indent);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalDeepCopy(vtkAbstractTransform *)
{
  // nothin' to do
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkIdentityTransform::MakeTransform()
{
  return vtkIdentityTransform::New();
}

//------------------------------------------------------------------------
template<class T2, class T3>
static inline void vtkIdentityTransformPoint(T2 in[3], T3 out[3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
}  

//------------------------------------------------------------------------
template<class T2, class T3, class T4>
static inline void vtkIdentityTransformDerivative(T2 in[3], T3 out[3], 
						  T4 derivative[3][3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];

  vtkMath::Identity3x3(derivative);
}  

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformPoint(const float in[3], 
						  float out[3])
{
  vtkIdentityTransformPoint(in,out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformPoint(const double in[3], 
						  double out[3])
{
  vtkIdentityTransformPoint(in,out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformNormal(const float in[3], 
						   float out[3])
{
  vtkIdentityTransformPoint(in,out);
  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformNormal(const double in[3], 
						   double out[3])
{
  vtkIdentityTransformPoint(in,out);
  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformVector(const float in[3], 
						   float out[3])
{
  vtkIdentityTransformPoint(in,out);
}

//------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformVector(const double in[3], 
						   double out[3])
{
  vtkIdentityTransformPoint(in,out);
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformDerivative(const float in[3], 
						       float out[3],
						       float derivative[3][3])
{
  vtkIdentityTransformDerivative(in,out,derivative);
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformDerivative(const double in[3], 
						       double out[3],
						       double derivative[3][3])
{
  vtkIdentityTransformDerivative(in,out,derivative);
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the 
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform. 
void vtkIdentityTransform::TransformPointsNormalsVectors(vtkPoints *inPts, 
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
void vtkIdentityTransform::TransformPoints(vtkPoints *inPts, 
					   vtkPoints *outPts)
{
  int n = inPts->GetNumberOfPoints();
  double point[3];  

  for (int i = 0; i < n; i++)
    {
    inPts->GetPoint(i,point);
    outPts->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::TransformNormals(vtkNormals *inNms, 
					    vtkNormals *outNms)
{
  int n = inNms->GetNumberOfNormals();
  double normal[3];
  
  for (int i = 0; i < n; i++)
    {
    inNms->GetNormal(i,normal);
    outNms->InsertNextNormal(normal);
    }
}

void vtkIdentityTransform::TransformNormals(vtkDataArray *inNms, 
					    vtkDataArray *outNms)
{
  int n = inNms->GetNumberOfTuples();
  double normal[3];
  
  for (int i = 0; i < n; i++)
    {
    inNms->GetTuple(i,normal);
    outNms->InsertNextTuple(normal);
    }
}

//----------------------------------------------------------------------------
void vtkIdentityTransform::TransformVectors(vtkVectors *inNms, 
					    vtkVectors *outNms)
{
  int n = inNms->GetNumberOfVectors();
  double vect[3];
  
  for (int i = 0; i < n; i++)
    {
    inNms->GetVector(i,vect);
    outNms->InsertNextVector(vect);
    }
}



