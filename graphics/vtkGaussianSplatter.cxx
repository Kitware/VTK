/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianSplatter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include "vtkGaussianSplatter.hh"
#include "vtkFloatScalars.hh"

// Description:
// Construct object with dimensions=(50,50,50); automatic computation of 
// bounds; a splat radius of 0.1; an exponent factor of -5; and normal and 
// scalar warping turned on.
vtkGaussianSplatter::vtkGaussianSplatter()
{
  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Radius = 0.1;
  this->ExponentFactor = -5.0;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->NormalWarping = 1;
  this->Eccentricity = 2.5;

  this->ScalarWarping = 1;
  this->ScaleFactor = 1.0;

  this->Capping = 1;
  this->CapValue = 0.0;
}

void vtkGaussianSplatter::SetModelBounds(float xmin, float xmax, float ymin, 
                                         float ymax, float zmin, float zmax)
{
  float bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->SetModelBounds(bounds);
}

//
//  Static variables aid recursion
//
static vtkFloatScalars *NewScalars;
static float Radius2;
static float (vtkGaussianSplatter::*Sample)(float x[3]);
static float (vtkGaussianSplatter::*SampleFactor)(float s);
static char *Visited;
static float Eccentricity2;
static float *P, *N, S;
static float Origin[3], AspectRatio[3];

void vtkGaussianSplatter::Execute()
{
  int numSplatPts, numPts;
  int ptId, i, j, k;
  vtkPointData *pd;
  vtkNormals *inNormals=NULL;
  vtkScalars *inScalars=NULL;
  int loc[3], ip, jp, kp, idir, jdir, kdir;
  vtkStructuredPoints *output = this->GetOutput();
  
  vtkDebugMacro(<< "Splatting data");
  // init a couple variables to prevent compiler warnings
  jp = 0;
  kp = 0;
  
  //
  //  Make sure points are available
  //
  if ( (numSplatPts=this->Input->GetNumberOfPoints()) < 1 )
    {
    vtkErrorMacro(<<"No points to splat!");
    return;
    }
  //
  //  Compute the radius of influence of the points.  If an
  //  automatically generated bounding box has been generated, increase
  //  its size slightly to acoomodate the radius of influence.
  //
  Eccentricity2 = this->Eccentricity * this->Eccentricity;

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
           * this->SampleDimensions[2];
  NewScalars = new vtkFloatScalars(numPts);
  for (i=0; i<numPts; i++) NewScalars->SetScalar(i,0.0);

  Visited = new char[numPts];
  for (i=0; i < numPts; i++) Visited[i] = 0;

  output->SetDimensions(this->GetSampleDimensions());
  this->ComputeModelBounds();
//
//  Set up proper function pointers
//
  pd = this->Input->GetPointData();
  if ( this->NormalWarping && (inNormals=pd->GetNormals()) != NULL )
    Sample = &vtkGaussianSplatter::EccentricGaussian;
  else
    Sample = &vtkGaussianSplatter::Gaussian;

  if ( this->ScalarWarping && (inScalars=pd->GetScalars()) != NULL )
    SampleFactor = &vtkGaussianSplatter::ScalarSampling;
  else
    SampleFactor = &vtkGaussianSplatter::PositionSampling;
//
// Traverse all points - injecting into volume.
// For each input point, determine which cell it is in.  Then start
// the recursive distribution of sampling function.
//
  for (ptId=0; ptId < this->Input->GetNumberOfPoints(); ptId++)
    {
    P = this->Input->GetPoint(ptId);
    if ( inNormals != NULL ) N = inNormals->GetNormal(ptId);
    if ( inScalars != NULL ) S = inScalars->GetScalar(ptId);

    if ( ! (ptId % 5000) && ptId > 0 )
      vtkDebugMacro(<< "Vertex #" << ptId);

    for (i=0; i<3; i++)  
      loc[i] = (int) ((float)(P[i] - Origin[i]) / AspectRatio[i]);
//
//  For each of the eight corners of the cell, need to evaluate sample
//  function and then begin recursive distribution
//
    for (i=0; i < 2; i++) 
      {
      for (j=0; j < 2; j++) 
        {
        for (k=0; k < 2; k++) 
          {
          if ( (ip=loc[0]+i) >= 0 && ip < this->SampleDimensions[0] &&
          (jp=loc[1]+j) >= 0 && jp < this->SampleDimensions[1] &&
          (kp=loc[2]+k) >= 0 && kp < this->SampleDimensions[2] ) 
            {
            idir = (i==0 ? -1 : 1);
            jdir = (j==0 ? -1 : 1);
            kdir = (k==0 ? -1 : 1);
            SplitIJK (ip, idir, jp, jdir, kp, kdir);
            }
          }
        }
      }
    }
//
// If capping is turned on, set the distances of the outside of the volume
// to the CapValue.
//
  if ( this->Capping )
    {
    this->Cap(NewScalars);
    }

  vtkDebugMacro(<< "Splatted " << this->Input->GetNumberOfPoints() << " points");
//
// Update self and release memeory
//
  delete [] Visited;

  output->GetPointData()->SetScalars(NewScalars);
  NewScalars->Delete();
}

// Description:
// Compute the size of the sample bounding box automatically from the
// input data.
void vtkGaussianSplatter::ComputeModelBounds()
{
  float *bounds, maxDist;
  int i, adjustBounds=0;
  vtkStructuredPoints *output = this->GetOutput();
  
  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
  this->ModelBounds[2] >= this->ModelBounds[3] ||
  this->ModelBounds[4] >= this->ModelBounds[5] )
    {
    adjustBounds = 1;
    bounds = this->Input->GetBounds();
    }
  else
    {
    bounds = this->ModelBounds;
    }

  for (maxDist=0.0, i=0; i<3; i++)
    if ( (bounds[2*i+1] - bounds[2*i]) > maxDist )
      maxDist = bounds[2*i+1] - bounds[2*i];

  maxDist *= this->Radius;
  Radius2 = maxDist * maxDist;

  // adjust bounds so model fits strictly inside (only if not set previously)
  if ( adjustBounds )
    {
    for (i=0; i<3; i++)
      {
      this->ModelBounds[2*i] = bounds[2*i] - maxDist;
      this->ModelBounds[2*i+1] = bounds[2*i+1] + maxDist;
      }
    }

  // Set volume origin and aspect ratio
  output->SetOrigin(this->ModelBounds[0],this->ModelBounds[2],
		    this->ModelBounds[4]);
  output->GetOrigin(Origin);
  
  for (i=0; i<3; i++)
    {
    AspectRatio[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
      / (this->SampleDimensions[i] - 1);
    }
  output->SetAspectRatio(AspectRatio);
}

// Description:
// Set the dimensions of the sampling structured point set.
void vtkGaussianSplatter::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

void vtkGaussianSplatter::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if (dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
      dim[2] != SampleDimensions[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
      }
    
    for (dataDim=0, i=0; i<3 ; i++) if (dim[i] > 1) dataDim++;

    if ( dataDim  < 3 )
      {
      vtkErrorMacro(<<"Sample dimensions must define a volume!");
      return;
      }

    for ( i=0; i<3; i++) this->SampleDimensions[i] = dim[i];
    
    this->Modified();
    }
}

void vtkGaussianSplatter::Cap(vtkFloatScalars *s)
{
  int i,j,k;
  int idx;
  int d01=this->SampleDimensions[0]*this->SampleDimensions[1];

// i-j planes
  k = 0;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(i+j*this->SampleDimensions[1], this->CapValue);

  k = this->SampleDimensions[2] - 1;
  idx = k*d01;
  for (j=0; j<this->SampleDimensions[1]; j++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+j*this->SampleDimensions[1], this->CapValue);

// j-k planes
  i = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (j=0; j<this->SampleDimensions[1]; j++)
      s->SetScalar(j*this->SampleDimensions[0]+k*d01, this->CapValue);

  i = this->SampleDimensions[0] - 1;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (j=0; j<this->SampleDimensions[1]; j++)
      s->SetScalar(i+j*this->SampleDimensions[0]+k*d01, this->CapValue);

// i-k planes
  j = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(i+k*d01, this->CapValue);

  j = this->SampleDimensions[1] - 1;
  idx = j*this->SampleDimensions[0];
  for (k=0; k<this->SampleDimensions[2]; k++)
    for (i=0; i<this->SampleDimensions[0]; i++)
      s->SetScalar(idx+i+k*d01, this->CapValue);

}

void vtkGaussianSplatter::SplitIJK (int i, int idir, int j, int jdir, 
                                   int k, int kdir)
{
  int     idx, ip, jp, kp;
  float   cx[3], dist2;

  cx[0] = Origin[0] + AspectRatio[0]*i;
  cx[1] = Origin[1] + AspectRatio[1]*j;
  cx[2] = Origin[2] + AspectRatio[2]*k;

  if ( (dist2= (this->*Sample)(cx)) <= Radius2 ) 
    {

    idx = i + j*this->SampleDimensions[0] + 
          k*this->SampleDimensions[0]*this->SampleDimensions[1];

    this->SetScalar(idx,dist2);
//
//  Continue sampling on opposite cell vertex, cell walls that emanate
//  from this vertex, and cell edges that emanate from this vertex.
//
    ip = i + idir;
    jp = j + jdir;
    kp = k + kdir;

    if ( ip >= 0 && ip < this->SampleDimensions[0] && 
    jp >= 0 && jp < this->SampleDimensions[1] && 
    kp >= 0 && kp < this->SampleDimensions[2] )
      SplitIJK (ip, idir, jp, jdir, kp, kdir);
//
//  Don't forget cell walls emanating from this vertex
//
    if ( ip >= 0 && ip < this->SampleDimensions[0] && 
    jp >= 0 && jp < this->SampleDimensions[1] )
      SplitIJ (ip, idir, jp, jdir, k);

    if ( jp >= 0 && jp < this->SampleDimensions[1] && 
    kp >= 0 && kp < this->SampleDimensions[2] )
      SplitJK (i, jp, jdir, kp, kdir);

    if ( ip >= 0 && ip < this->SampleDimensions[0] && 
    kp >= 0 && kp < this->SampleDimensions[2] )
      SplitIK (ip, idir, j, kp, kdir);
//
//  Don't forget the cell edges emanating from this vertex
//
    if ( ip >= 0 && ip < this->SampleDimensions[0] )
      SplitI (ip, idir, j, k);

    if ( jp >= 0 && jp < this->SampleDimensions[1] )
      SplitJ (i, jp, jdir, k);

    if ( kp >= 0 && kp < this->SampleDimensions[2] )
      SplitK (i, j, kp, kdir);

    }
}

void vtkGaussianSplatter::SplitIJ (int i, int idir, int j, int jdir, int k)
{
  int     idx, ip, jp;
  float   cx[3], dist2;

  cx[0] = Origin[0] + AspectRatio[0]*i;
  cx[1] = Origin[1] + AspectRatio[1]*j;
  cx[2] = Origin[2] + AspectRatio[2]*k;

  if ( (dist2= (this->*Sample)(cx)) <= Radius2 ) 
    {

    idx = i + j*this->SampleDimensions[0] + 
          k*this->SampleDimensions[0]*this->SampleDimensions[1];

    this->SetScalar(idx,dist2);
//
//  Continue sampling on opposite cell vertex and cell edges that
//  emanate from this vertex. 
//
    ip = i + idir;
    jp = j + jdir;

    if ( ip >= 0 && ip < this->SampleDimensions[0] && 
    jp >= 0 && jp < this->SampleDimensions[1] )
      SplitIJ (ip, idir, jp, jdir, k);
//
//  Don't forget the cell edges emanating from this vertex
//
    if ( ip >= 0 && ip < this->SampleDimensions[0] )
      SplitI (ip, idir, j, k);

    if ( jp >= 0 && jp < this->SampleDimensions[1] )
      SplitJ (i, jp, jdir, k);

    }
}

void vtkGaussianSplatter::SplitJK (int i, int j, int jdir, int k, int kdir)
{
  int     idx, jp, kp;
  float   cx[3], dist2;

  cx[0] = Origin[0] + AspectRatio[0]*i;
  cx[1] = Origin[1] + AspectRatio[1]*j;
  cx[2] = Origin[2] + AspectRatio[2]*k;

  if ( (dist2= (this->*Sample)(cx)) <= Radius2 ) 
    {

    idx = i + j*this->SampleDimensions[0] + 
          k*this->SampleDimensions[0]*this->SampleDimensions[1];

    this->SetScalar(idx,dist2);
//
//  Continue sampling on opposite cell vertex and cell edges that
//  emanate from this vertex. 
//
    jp = j + jdir;
    kp = k + kdir;

    if ( jp >= 0 && jp < this->SampleDimensions[1] && 
    kp >= 0 && kp < this->SampleDimensions[2] )
      SplitJK (i, jp, jdir, kp, kdir);
//
//  Don't forget the cell edges emanating from this vertex
//
    if ( jp >= 0 && jp < this->SampleDimensions[1] )
     SplitJ (i, jp, jdir, k);

    if ( kp >= 0 && kp < this->SampleDimensions[2] )
     SplitK (i, j, kp, kdir);
    }
}

void vtkGaussianSplatter::SplitIK (int i, int idir, int j, int k, int kdir)
{
  int     idx, ip, kp;
  float   cx[3], dist2;

  cx[0] = Origin[0] + AspectRatio[0]*i;
  cx[1] = Origin[1] + AspectRatio[1]*j;
  cx[2] = Origin[2] + AspectRatio[2]*k;

  if ( (dist2= (this->*Sample)(cx)) <= Radius2 ) 
    {

    idx = i + j*this->SampleDimensions[0] + 
          k*this->SampleDimensions[0]*this->SampleDimensions[1];

    this->SetScalar(idx,dist2);
//
//  Continue sampling on opposite cell vertex and cell edges that
//  emanate from this vertex. 
//
    ip = i + idir;
    kp = k + kdir;

    if ( ip >= 0 && ip < this->SampleDimensions[0] && kp >= 0 && kp < this->SampleDimensions[2] )
      SplitIK (ip,idir, j, kp,kdir);
//
//  Don't forget the cell edges emanating from this vertex
//
    if ( ip >= 0 && ip < this->SampleDimensions[0] )
      SplitI (ip,idir, j, k);

    if ( kp >= 0 && kp < this->SampleDimensions[2] )
      SplitK (i, j, kp,kdir);
    }
}

void vtkGaussianSplatter::SplitI (int i, int idir, int j, int k)
{
  int     idx, ip;
  float   cx[3], dist2;

  cx[0] = Origin[0] + AspectRatio[0]*i;
  cx[1] = Origin[1] + AspectRatio[1]*j;
  cx[2] = Origin[2] + AspectRatio[2]*k;

  if ( (dist2=(this->*Sample)(cx)) <= Radius2 ) 
    {

    idx = i + j*this->SampleDimensions[0] + 
          k*this->SampleDimensions[0]*this->SampleDimensions[1];

    this->SetScalar(idx,dist2);
//
//  Continue sampling along this edge.
//
    if ( (ip=i+idir) >= 0 && ip < this->SampleDimensions[0] )
       SplitI (ip,idir, j, k);
    }
}

void vtkGaussianSplatter::SplitJ (int i, int j, int jdir, int k)
{
  int     idx, jp;
  float   cx[3], dist2;

  cx[0] = Origin[0] + AspectRatio[0]*i;
  cx[1] = Origin[1] + AspectRatio[1]*j;
  cx[2] = Origin[2] + AspectRatio[2]*k;

  if ( (dist2=(this->*Sample)(cx)) <= Radius2 ) 
    {
    idx = i + j*this->SampleDimensions[0] + 
          k*this->SampleDimensions[0]*this->SampleDimensions[1];

    this->SetScalar(idx,dist2);
//
//  Continue sampling along this edge.
//
    if ( (jp=j+jdir) >= 0 && jp < this->SampleDimensions[1] )
      SplitJ (i, jp,jdir, k);
    }
}

void vtkGaussianSplatter::SplitK (int i, int j, int k, int kdir)
{
  int     idx, kp;
  float   cx[3], dist2;

  cx[0] = Origin[0] + AspectRatio[0]*i;
  cx[1] = Origin[1] + AspectRatio[1]*j;
  cx[2] = Origin[2] + AspectRatio[2]*k;

  if ( (dist2=(this->*Sample)(cx)) <= Radius2 ) 
    {
    idx = i + j*this->SampleDimensions[0] + 
          k*this->SampleDimensions[0]*this->SampleDimensions[1];

    this->SetScalar(idx,dist2);
//
//  Continue sampling along this edge.
//
    if ( (kp=k+kdir) >= 0 && kp < this->SampleDimensions[2] )
      SplitK (i, j, kp,kdir);
    }
}

//
//  Gaussian sampling
//
float vtkGaussianSplatter::Gaussian (float cx[3])
{
  return ((cx[0]-P[0])*(cx[0]-P[0]) + (cx[1]-P[1])*(cx[1]-P[1]) +
          (cx[2]-P[2])*(cx[2]-P[2]) );
}
    
//
//  Ellipsoidal Gaussian sampling
//
float vtkGaussianSplatter::EccentricGaussian (float cx[3])
{
  float   v[3], r2, z2, rxy2, mag;

  v[0] = cx[0] - P[0];
  v[1] = cx[1] - P[1];
  v[2] = cx[2] - P[2];

  r2 = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

  if ( (mag=N[0]*N[0]+N[1]*N[1]+N[2]*N[2]) != 1.0  ) 
    {
    if ( mag == 0.0 ) mag = 1.0;
    else mag = sqrt((double)mag);
    }

  z2 = (v[0]*N[0] + v[1]*N[1] + v[2]*N[2])/mag;
  z2 = z2*z2;

  rxy2 = r2 - z2;

  return (rxy2/Eccentricity2 + z2);
}
    
void vtkGaussianSplatter::SetScalar(int idx, float dist2)
{
  float v = (this->*SampleFactor)(S) * exp((double)
            (this->ExponentFactor*(dist2)/(Radius2)));

  if ( Visited[idx] )
    {
    float s = NewScalars->GetScalar(idx);
    NewScalars->SetScalar(idx,( s > v ? s : v));
    }
  else 
    {
    Visited[idx] = 1;
    NewScalars->SetScalar(idx,v);
    }
}

void vtkGaussianSplatter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToStructuredPointsFilter::PrintSelf(os,indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Exponent Factor: " << this->ExponentFactor << "\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Normal Warping: " << (this->NormalWarping ? "On\n" : "Off\n");
  os << indent << "Eccentricity: " << this->Eccentricity << "\n";

  os << indent << "Scalar Warping: " << (this->ScalarWarping ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";
}

