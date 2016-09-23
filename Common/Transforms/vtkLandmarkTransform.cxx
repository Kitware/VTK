/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLandmarkTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLandmarkTransform.h"

#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkLandmarkTransform);

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
  this->Superclass::PrintSelf(os, indent);
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
  vtkIdType i;
  int j;

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

  const vtkIdType N_PTS = this->SourceLandmarks->GetNumberOfPoints();
  if(N_PTS != this->TargetLandmarks->GetNumberOfPoints())
  {
    vtkErrorMacro("Update: Source and Target Landmarks contain a different number of points");
    return;
  }

  // -- if no points, stop here

  if (N_PTS == 0)
  {
    this->Matrix->Identity();
    return;
  }

  // -- find the centroid of each set --

  double source_centroid[3]={0,0,0};
  double target_centroid[3]={0,0,0};
  double p[3];
  for(i=0;i<N_PTS;i++)
  {
    this->SourceLandmarks->GetPoint(i, p);
    source_centroid[0] += p[0];
    source_centroid[1] += p[1];
    source_centroid[2] += p[2];
    this->TargetLandmarks->GetPoint(i, p);
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

  // -- if only one point, stop right here

  if (N_PTS == 1)
  {
    this->Matrix->Identity();
    this->Matrix->Element[0][3] = target_centroid[0] - source_centroid[0];
    this->Matrix->Element[1][3] = target_centroid[1] - source_centroid[1];
    this->Matrix->Element[2][3] = target_centroid[2] - source_centroid[2];
    return;
  }

  // -- build the 3x3 matrix M --

  double M[3][3];
  double AAT[3][3];
  for(i=0;i<3;i++)
  {
    AAT[i][0] = M[i][0]=0.0F; // fill M with zeros
    AAT[i][1] = M[i][1]=0.0F;
    AAT[i][2] = M[i][2]=0.0F;
  }
  vtkIdType pt;
  double a[3],b[3];
  double sa=0.0F,sb=0.0F;
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

      // for the affine transform, compute ((a.a^t)^-1 . a.b^t)^t.
      // a.b^t is already in M.  here we put a.a^t in AAT.
      if (this->Mode == VTK_LANDMARK_AFFINE)
      {
        AAT[i][0] += a[i]*a[0];
        AAT[i][1] += a[i]*a[1];
        AAT[i][2] += a[i]*a[2];
      }
    }
    // accumulate scale factors (if desired)
    sa += a[0]*a[0]+a[1]*a[1]+a[2]*a[2];
    sb += b[0]*b[0]+b[1]*b[1]+b[2]*b[2];
  }

  if(this->Mode == VTK_LANDMARK_AFFINE)
  {
    // AAT = (a.a^t)^-1
    vtkMath::Invert3x3(AAT,AAT);

    // M = (a.a^t)^-1 . a.b^t
    vtkMath::Multiply3x3(AAT,M,M);

    // this->Matrix = M^t
    for(i=0;i<3;++i)
    {
      for(j=0;j<3;++j)
      {
        this->Matrix->Element[i][j] = M[j][i];
      }
    }
  }
  else
  {
    // compute required scaling factor (if desired)
    double scale = (double)sqrt(sb/sa);

    // -- build the 4x4 matrix N --

    double Ndata[4][4];
    double *N[4];
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

    double eigenvectorData[4][4];
    double *eigenvectors[4],eigenvalues[4];

    eigenvectors[0] = eigenvectorData[0];
    eigenvectors[1] = eigenvectorData[1];
    eigenvectors[2] = eigenvectorData[2];
    eigenvectors[3] = eigenvectorData[3];

    vtkMath::JacobiN(N,4,eigenvalues,eigenvectors);

    // the eigenvector with the largest eigenvalue is the quaternion we want
    // (they are sorted in decreasing order for us by JacobiN)
    double w,x,y,z;

    // first: if points are collinear, choose the quaternion that
    // results in the smallest rotation.
    if (eigenvalues[0] == eigenvalues[1] || N_PTS == 2)
    {
      double s0[3],t0[3],s1[3],t1[3];
      this->SourceLandmarks->GetPoint(0,s0);
      this->TargetLandmarks->GetPoint(0,t0);
      this->SourceLandmarks->GetPoint(1,s1);
      this->TargetLandmarks->GetPoint(1,t1);

      double ds[3],dt[3];
      double rs = 0, rt = 0;
      for (i = 0; i < 3; i++)
      {
        ds[i] = s1[i] - s0[i];      // vector between points
        rs += ds[i]*ds[i];
        dt[i] = t1[i] - t0[i];
        rt += dt[i]*dt[i];
      }

      // normalize the two vectors
      rs = sqrt(rs);
      ds[0] /= rs; ds[1] /= rs; ds[2] /= rs;
      rt = sqrt(rt);
      dt[0] /= rt; dt[1] /= rt; dt[2] /= rt;

      // take dot & cross product
      w = ds[0]*dt[0] + ds[1]*dt[1] + ds[2]*dt[2];
      x = ds[1]*dt[2] - ds[2]*dt[1];
      y = ds[2]*dt[0] - ds[0]*dt[2];
      z = ds[0]*dt[1] - ds[1]*dt[0];

      double r = sqrt(x*x + y*y + z*z);
      double theta = atan2(r,w);

      // construct quaternion
      w = cos(theta/2);
      if (r != 0)
      {
        r = sin(theta/2)/r;
        x = x*r;
        y = y*r;
        z = z*r;
      }
      else // rotation by 180 degrees: special case
      {
        // rotate around a vector perpendicular to ds
        vtkMath::Perpendiculars(ds,dt,0,0);
        r = sin(theta/2);
        x = dt[0]*r;
        y = dt[1]*r;
        z = dt[2]*r;
      }
    }
    else // points are not collinear
    {
      w = eigenvectors[0][0];
      x = eigenvectors[1][0];
      y = eigenvectors[2][0];
      z = eigenvectors[3][0];
    }

    // convert quaternion to a rotation matrix

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
vtkMTimeType vtkLandmarkTransform::GetMTime()
{
  vtkMTimeType result = this->vtkLinearTransform::GetMTime();
  vtkMTimeType mtime;

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
vtkAbstractTransform *vtkLandmarkTransform::MakeTransform()
{
  return vtkLandmarkTransform::New();
}

//----------------------------------------------------------------------------
void vtkLandmarkTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkLandmarkTransform *t = (vtkLandmarkTransform *)transform;

  this->SetMode(t->Mode);
  this->SetSourceLandmarks(t->SourceLandmarks);
  this->SetTargetLandmarks(t->TargetLandmarks);

  this->Modified();
}


