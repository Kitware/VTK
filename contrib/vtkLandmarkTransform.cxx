/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLandmarkTransform.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tim Hutton and David G. Gobbi who developed this class.

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

#include "vtkLandmarkTransform.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkLandmarkTransform* vtkLandmarkTransform::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLandmarkTransform");
  if(ret)
    {
    return (vtkLandmarkTransform*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLandmarkTransform;
}

//----------------------------------------------------------------------------
vtkLandmarkTransform::vtkLandmarkTransform()
{
  this->Mode = VTK_LANDMARK_SIMILARITY;
  this->SourceLandmarks=NULL;
  this->TargetLandmarks=NULL;
}

//----------------------------------------------------------------------------
vtkLandmarkTransform::~vtkLandmarkTransform()
{
  if(this->SourceLandmarks)
    { 
    this->SourceLandmarks->Delete();
    }
  if(this->TargetLandmarks)
    { 
    this->TargetLandmarks->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkLandmarkTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLinearTransform::PrintSelf(os, indent);
  os << "Mode: " << this->GetModeAsString() << "\n";
  os << "SourceLandmarks: " << this->SourceLandmarks << "\n";
  if(this->SourceLandmarks) 
    {
    this->SourceLandmarks->PrintSelf(os,indent.GetNextIndent());
    }
  os << "TargetLandmarks: " << this->TargetLandmarks << "\n";
  if(this->TargetLandmarks)
    { 
    this->TargetLandmarks->PrintSelf(os,indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
// Update the 4x4 matrix. Updates are only done as necessary.
 
void vtkLandmarkTransform::InternalUpdate()
{
  if (this->SourceLandmarks == NULL || this->TargetLandmarks == NULL)
    {
    this->Matrix->Identity();
    return;
    }

  // --- compute the necessary transform to match the two sets of landmarks ---

  /*
    The solution is based on
    Berthold K. P. Horn (1987),
    "Closed-form solution of absolute orientation using unit quaternions,"
    Journal of the Optical Society of America A, 4:629-642
  */

  // Original python implementation by David G. Gobbi

  const int N_PTS = this->SourceLandmarks->GetNumberOfPoints();
  if(N_PTS != this->TargetLandmarks->GetNumberOfPoints())
    {
    vtkErrorMacro("Update: Source and Target Landmarks contain a different number of points");
    return;
    }

  int i;

  // -- find the centroid of each set --

  float source_centroid[3]={0,0,0};
  float target_centroid[3]={0,0,0};
  float *p;
  for(i=0;i<N_PTS;i++)
    {
    p = this->SourceLandmarks->GetPoint(i);
    source_centroid[0] += p[0];
    source_centroid[1] += p[1];
    source_centroid[2] += p[2];
    p = this->TargetLandmarks->GetPoint(i);
    target_centroid[0] += p[0];
    target_centroid[1] += p[1];
    target_centroid[2] += p[2];
    }
  source_centroid[0] /= N_PTS;
  source_centroid[1] /= N_PTS;
  source_centroid[2] /= N_PTS;
  target_centroid[0] /= N_PTS;
  target_centroid[1] /= N_PTS;
  target_centroid[2] /= N_PTS;

  // -- build the 3x3 matrix M --

  float M[3][3];
  for(i=0;i<3;i++) 
    {
    M[i][0]=0.0F; // fill M with zeros
    M[i][1]=0.0F; 
    M[i][2]=0.0F; 
    }
  int pt;
  float a[3],b[3];
  float sa=0.0F,sb=0.0F;
  for(pt=0;pt<N_PTS;pt++)
    {
    // get the origin-centred point (a) in the source set
    this->SourceLandmarks->GetPoint(pt,a);
    a[0] -= source_centroid[0];
    a[1] -= source_centroid[1];
    a[2] -= source_centroid[2];
    // get the origin-centred point (b) in the target set
    this->TargetLandmarks->GetPoint(pt,b);
    b[0] -= target_centroid[0];
    b[1] -= target_centroid[1];
    b[2] -= target_centroid[2];
    // accumulate the products a*T(b) into the matrix M
    for(i=0;i<3;i++) 
      {
      M[i][0] += a[i]*b[0];
      M[i][1] += a[i]*b[1];
      M[i][2] += a[i]*b[2];
      }
    // accumulate scale factors (if desired)
    sa += a[0]*a[0]+a[1]*a[1]+a[2]*a[2];
    sb += b[0]*b[0]+b[1]*b[1]+b[2]*b[2];
    }
  // compute required scaling factor (if desired)
  float scale = (float)sqrt(sb/sa);

  // -- build the 4x4 matrix N --

  float Ndata[4][4];
  float *N[4];
  for(i=0;i<4;i++)
    {
    N[i] = Ndata[i];
    N[i][0]=0.0F; // fill N with zeros
    N[i][1]=0.0F;
    N[i][2]=0.0F;
    N[i][3]=0.0F;
    }
  // on-diagonal elements
  N[0][0] = M[0][0]+M[1][1]+M[2][2];
  N[1][1] = M[0][0]-M[1][1]-M[2][2];
  N[2][2] = -M[0][0]+M[1][1]-M[2][2];
  N[3][3] = -M[0][0]-M[1][1]+M[2][2];
  // off-diagonal elements
  N[0][1] = N[1][0] = M[1][2]-M[2][1];
  N[0][2] = N[2][0] = M[2][0]-M[0][2];
  N[0][3] = N[3][0] = M[0][1]-M[1][0];

  N[1][2] = N[2][1] = M[0][1]+M[1][0];
  N[1][3] = N[3][1] = M[2][0]+M[0][2];
  N[2][3] = N[3][2] = M[1][2]+M[2][1];

  // -- eigen-decompose N (is symmetric) --

  float eigenvectorData[4][4];
  float *eigenvectors[4],eigenvalues[4];

  eigenvectors[0] = eigenvectorData[0];
  eigenvectors[1] = eigenvectorData[1];
  eigenvectors[2] = eigenvectorData[2];
  eigenvectors[3] = eigenvectorData[3];

  vtkMath::JacobiN(N,4,eigenvalues,eigenvectors);

  // the eigenvector with the largest eigenvalue is the quaternion we want
  // (they are sorted in decreasing order for us by JacobiN)
  float quat[4]; 
  quat[0] = eigenvectors[0][0];
  quat[1] = eigenvectors[1][0];
  quat[2] = eigenvectors[2][0];
  quat[3] = eigenvectors[3][0];

  // convert it to a rotation matrix
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

  this->Matrix->Element[0][0] = ww + xx - yy - zz; 
  this->Matrix->Element[1][0] = 2.0*(wz + xy);
  this->Matrix->Element[2][0] = 2.0*(-wy + xz);

  this->Matrix->Element[0][1] = 2.0*(-wz + xy);  
  this->Matrix->Element[1][1] = ww - xx + yy - zz;
  this->Matrix->Element[2][1] = 2.0*(wx + yz);

  this->Matrix->Element[0][2] = 2.0*(wy + xz);
  this->Matrix->Element[1][2] = 2.0*(-wx + yz);
  this->Matrix->Element[2][2] = ww - xx - yy + zz;

  if (this->Mode != VTK_LANDMARK_RIGIDBODY)
    { // add in the scale factor (if desired)
    for(i=0;i<3;i++) 
      {
      this->Matrix->Element[i][0] *= scale;
      this->Matrix->Element[i][1] *= scale;
      this->Matrix->Element[i][2] *= scale;
      }
    }

  // the translation is given by the difference in the transformed source
  // centroid and the target centroid
  double sx, sy, sz;

  sx = this->Matrix->Element[0][0] * source_centroid[0] +
       this->Matrix->Element[0][1] * source_centroid[1] +
       this->Matrix->Element[0][2] * source_centroid[2];
  sy = this->Matrix->Element[1][0] * source_centroid[0] +
       this->Matrix->Element[1][1] * source_centroid[1] +
       this->Matrix->Element[1][2] * source_centroid[2];
  sz = this->Matrix->Element[2][0] * source_centroid[0] +
       this->Matrix->Element[2][1] * source_centroid[1] +
       this->Matrix->Element[2][2] * source_centroid[2];

  this->Matrix->Element[0][3] = target_centroid[0] - sx;
  this->Matrix->Element[1][3] = target_centroid[1] - sy;
  this->Matrix->Element[2][3] = target_centroid[2] - sz;

  // fill the bottom row of the 4x4 matrix
  this->Matrix->Element[3][0] = 0.0;
  this->Matrix->Element[3][1] = 0.0;
  this->Matrix->Element[3][2] = 0.0;
  this->Matrix->Element[3][3] = 1.0;

  this->Matrix->Modified();
}

//------------------------------------------------------------------------
unsigned long vtkLandmarkTransform::GetMTime()
{
  unsigned long result = this->vtkLinearTransform::GetMTime();
  unsigned long mtime;

  if (this->SourceLandmarks)
    {
    mtime = this->SourceLandmarks->GetMTime(); 
    if (mtime > result)
      {
      result = mtime;
      }
    }
  if (this->TargetLandmarks)
    {
    mtime = this->TargetLandmarks->GetMTime(); 
    if (mtime > result)
      {
      result = mtime;
      }
    }
  return result;
}
//------------------------------------------------------------------------
void vtkLandmarkTransform::SetSourceLandmarks(vtkPoints *source)
{
  if (this->SourceLandmarks == source)
    {
    return;
    }

  if (this->SourceLandmarks)
    {
    this->SourceLandmarks->Delete();
    }

  source->Register(this);
  this->SourceLandmarks = source;

  this->Modified();
}

//------------------------------------------------------------------------
void vtkLandmarkTransform::SetTargetLandmarks(vtkPoints *target)
{
  if (this->TargetLandmarks == target)
    {
    return;
    }

  if (this->TargetLandmarks)
    {
    this->TargetLandmarks->Delete();
    }

  target->Register(this);
  this->TargetLandmarks = target;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLandmarkTransform::Inverse()
{
  vtkPoints *tmp1 = this->SourceLandmarks;
  vtkPoints *tmp2 = this->TargetLandmarks;
  this->TargetLandmarks = tmp1;
  this->SourceLandmarks = tmp2;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkGeneralTransform *vtkLandmarkTransform::MakeTransform()
{
  return vtkLandmarkTransform::New(); 
}

//----------------------------------------------------------------------------
void vtkLandmarkTransform::InternalDeepCopy(vtkGeneralTransform *transform)
{
  vtkLandmarkTransform *t = (vtkLandmarkTransform *)transform;

  this->SetMode(t->Mode);
  this->SetSourceLandmarks(t->SourceLandmarks);
  this->SetTargetLandmarks(t->TargetLandmarks);

  this->Modified();
}


