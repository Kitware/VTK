/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <limits.h>
#include <float.h>
#include <math.h>
#include "vtkImageCache.h"
#include "vtkImageReslice.h"

//----------------------------------------------------------------------------
vtkImageReslice::vtkImageReslice()
{
  this->OutputSpacing[0] = 1;
  this->OutputSpacing[1] = 1;
  this->OutputSpacing[2] = 1;

  this->OutputOrigin[0] = FLT_MAX; // flag to set defaults later
  this->OutputExtent[0] = INT_MAX; // ditto

  this->Interpolate = 0; // nearest-neighbor interpolation by default
  this->BackgroundLevel = 0;
}

//----------------------------------------------------------------------------
void vtkImageReslice::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);

  os << indent << "ResliceTransform: " << this->ResliceTransform << "\n";
  if (this->ResliceTransform) 
    this->ResliceTransform->PrintSelf(os,indent.GetNextIndent());
  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " <<
    this->OutputSpacing[1] << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " <<
    this->OutputOrigin[1] << " " << this->OutputOrigin[2] << "\n";
  os << indent << "OutputExtent: " << this->OutputExtent[0] << " " <<
    this->OutputExtent[1] << " " << this->OutputExtent[2] << " " <<
    this->OutputExtent[3] << " " << this->OutputExtent[4] << " " <<
    this->OutputExtent[5] << "\n";
  os << indent << "Interpolate: " << this->Interpolate << "\n";
  os << indent << "BackgroundLevel: " << this->BackgroundLevel << "\n";
}

//----------------------------------------------------------------------------
void vtkImageReslice::ComputeIndexMatrix(vtkMatrix4x4 *matrix)
{
  int i;
  float inOrigin[3];
  float inSpacing[3];
  float outOrigin[3];
  float outSpacing[3];

  this->GetInput()->GetSpacing(inSpacing);
  this->GetInput()->GetOrigin(inOrigin);
  this->GetOutput()->GetSpacing(outSpacing);
  this->GetOutput()->GetOrigin(outOrigin);  
  
  vtkTransform *transform = vtkTransform::New();
  vtkMatrix4x4 *inMatrix = vtkMatrix4x4::New();
  vtkMatrix4x4 *outMatrix = vtkMatrix4x4::New();

  if (this->GetResliceTransform())
    {
    transform->SetMatrix(*(this->GetResliceTransform()->GetMatrixPointer()));
    }
  
  // the outMatrix takes OutputData indices to OutputData coordinates,
  // the inMatrix takes InputData coordinates to InputData indices

  for (i = 0; i < 3; i++) 
    {
    inMatrix->Element[i][i] = 1/inSpacing[i];
    inMatrix->Element[i][3] = -inOrigin[i]/inSpacing[i];
    outMatrix->Element[i][i] = outSpacing[i];
    outMatrix->Element[i][3] = outOrigin[i];
    }
  
  transform->PreMultiply();
  transform->Concatenate(outMatrix);
  transform->PostMultiply();
  transform->Concatenate(inMatrix);

  transform->GetMatrix(matrix);
  
  transform->Delete();
  inMatrix->Delete();
  outMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkImageReslice::ComputeRequiredInputUpdateExtent(int inExt[6], 
						       int outExt[6])
{
  int i,j,k;
  int idX,idY,idZ;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  float *xAxis, *yAxis, *zAxis, *origin;
  double point[4],w;

  // break transform down (to match vtkImageResliceExecute)
  this->ComputeIndexMatrix(matrix);
  matrix->Transpose();
  xAxis = matrix->Element[0];
  yAxis = matrix->Element[1];
  zAxis = matrix->Element[2];
  origin = matrix->Element[3];

  for (i = 0; i < 3; i++)
    {
    inExt[2*i] = INT_MAX;
    inExt[2*i+1] = INT_MIN;
    }
  
  for (i = 0; i < 8; i++)
    {
    // calculate transform using method in vtkImageResliceExecute
    idX = outExt[i%2];
    idY = outExt[2+(i/2)%2];
    idZ = outExt[4+(i/4)%2];
    
    for (j = 0; j < 4; j++) 
      {
      point[j] = origin[j] + idZ*zAxis[j];
      point[j] = point[j] + idY*yAxis[j];
      }
    
    w = point[3] + idX*xAxis[3];
    
    if (this->Interpolate)
      {
      for (j = 0; j < 3; j++) 
	{
	k = int(float((point[j]+idX*xAxis[j])/w));
	if (k < inExt[2*j]) inExt[2*j] = k; 
	k = int(float((point[j]+idX*xAxis[j])/w))+1;
	if (k > inExt[2*j+1]) inExt[2*j+1] = k;
	}
      }
    else
      {
      for (j = 0; j < 3; j++) 
	{
	k = int((point[j]+idX*xAxis[j])/w + 0.5);
	if (k < inExt[2*j]) inExt[2*j] = k; 
	k = int((point[j]+idX*xAxis[j])/w + 0.5);
	if (k > inExt[2*j+1]) inExt[2*j+1] = k;
	}
      }
    }
  matrix->Delete();
}


//----------------------------------------------------------------------------
void vtkImageReslice::ExecuteImageInformation() 
{
  int i,j;
  float inPoint[4], outPoint[4];
  float inOrigin[3],maxOut[3],minOut[3],tmp;
  float *inSpacing;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  int *inExt;

  inExt = this->Input->GetWholeExtent();
  inSpacing = this->Input->GetSpacing();
  this->Input->GetOrigin(inOrigin);
  
  // default extent covers entire input extent
  if (this->OutputExtent[0] == INT_MAX)
    {
    for (i = 0; i < 3; i++)
      {
      minOut[i] = FLT_MAX;
      maxOut[i] = FLT_MIN;
      }
    
    for (i = 0; i < 8; i++)
      {
      inPoint[0] = inOrigin[0] + inExt[i%2]*inSpacing[0];
      inPoint[1] = inOrigin[1] + inExt[2+(i/2)%2]*inSpacing[1];
      inPoint[2] = inOrigin[2] + inExt[4+(i/4)%2]*inSpacing[2];
      inPoint[3] = 1;
      
      if (this->ResliceTransform)
	{
	this->ResliceTransform->GetMatrix(matrix);
	}
      matrix->Invert();
      matrix->MultiplyPoint(inPoint,outPoint);
      
      for (j = 0; j < 3; j++) 
	{
	tmp = outPoint[j]/outPoint[3];
	if (tmp > maxOut[j])
	  maxOut[j] = tmp;
	if (tmp < minOut[j])
	  minOut[j] = tmp;
	}
      }
    
    for (i = 0; i < 3; i++)
      {
      this->OutputExtent[2*i] = 0;
      this->OutputExtent[2*i+1] = 
	int(ceil((maxOut[i]-minOut[i])/this->OutputSpacing[i]));
      }
    
    this->OutputExtent[4] += 1;
    this->OutputExtent[5] += 1;
    }
  // default origin places centre of output over centre of input
  if (this->OutputOrigin[0] == FLT_MAX)
    {
    if (this->ResliceTransform)
      this->ResliceTransform->GetMatrix(matrix);
    matrix->Invert();
    // set translational portion of matrix to zero
    matrix->Element[0][3] = 0;
    matrix->Element[1][3] = 0;
    matrix->Element[2][3] = 0;
    
    for (i = 0; i < 3; i++)
      {
      inPoint[i] = inOrigin[i] + inSpacing[i]*(inExt[2*i]+inExt[2*i+1])*0.5;
      }
    inPoint[3] = 1;
    matrix->MultiplyPoint(inPoint,outPoint);
    for (i = 0; i < 3; i++)
      {
      this->OutputOrigin[i] = outPoint[i]/outPoint[3] 
	- this->OutputSpacing[i]
	*(this->OutputExtent[2*i]+this->OutputExtent[2*i+1])*0.5;
      }
    }
  
  this->Output->SetWholeExtent(this->OutputExtent);
  this->Output->SetSpacing(this->OutputSpacing);
  this->Output->SetOrigin(this->OutputOrigin);
  matrix->Delete();
}

//----------------------------------------------------------------------------
// helper functions for vtkImageResliceExecute

// find approximate intersection of line with the plane x = x_min,
// y = y_min, or z = z_min (lower limit of data extent) 

static int intersectionLow(double *point, float *axis, int *sign,
			   int *limit, int ai, int interpolate)
{
  int r;
  
  if (interpolate) 
    {
    // aproximate value
    r = int((limit[ai]*point[3]-point[ai])
	    /(axis[ai]-limit[ai]*axis[3]) + 0.5);
    
    // move back and forth to find the point just inside the extent
    while (int(float((point[ai]+r*axis[ai])/double(point[3]+r*axis[3]))) 
	   < limit[ai])
      r += sign[ai];
    
    while (int(float((point[ai]+(r-sign[ai])*axis[ai])
		     /double(point[3]+(r-sign[ai])*axis[3]))) 
	   >= limit[ai])
      r -= sign[ai];
    }
  else
    {
    r = int(((limit[ai]+0.5)*point[3]-point[ai])
	    /(axis[ai]-(limit[ai]+0.5)*axis[3]) + 0.5);
    
    while (int( (point[ai]+r*axis[ai])/double(point[3]+r*axis[3]) + 0.5 ) 
	   < limit[ai])
      r += sign[ai];
    
    while (int( (point[ai]+(r-sign[ai])*axis[ai])
		/double(point[3]+(r-sign[ai])*axis[3]) + 0.5 ) 
	   >= limit[ai])
      r -= sign[ai];
    }
  return r;
}

// same as above, but for x = x_max
static int intersectionHigh(double *point, float *axis, int *sign, 
			    int *limit, int ai, int interpolate)
{
  int r;
  
  if (interpolate)
    {
    r = int((limit[ai]*point[3]-point[ai])
	    /(axis[ai]-limit[ai]*axis[3]) + 0.5); 
    
    while (int(float((point[ai]+r*axis[ai])/double(point[3]+r*axis[3]))) 
	   > limit[ai])
      r -= sign[ai];
    
    while (int(float((point[ai]+(r+sign[ai])*axis[ai])
		     /double(point[3]+(r+sign[ai])*axis[3]))) 
	   <= limit[ai])
      r += sign[ai];
    }
  else
    {
    r = int(((limit[ai]-0.5)*point[3]-point[ai])
	    /(axis[ai]-(limit[ai]-0.5)*axis[3]) + 0.5); 
    
    while (int( (point[ai]+r*axis[ai])/double(point[3]+r*axis[3]) + 0.5 ) 
	   > limit[ai])
      r -= sign[ai];
    
    while (int( (point[ai]+(r+sign[ai])*axis[ai])
		/double(point[3]+(r+sign[ai])*axis[3]) + 0.5 ) 
	   <= limit[ai])
      r += sign[ai];
    }
  
  return r;
}

static int isBounded(double *point, float *xAxis, int *inMin, 
		     int *inMax, int ai, int r, int interpolate)
{
  int bi,ci,bp,cp;
  double w;
  
  bi = ai+1; if (bi > 2) bi -= 3;
  ci = ai+2; if (ci > 2) ci -= 3;
  
  if (interpolate) 
    {
    w = point[3]+r*xAxis[3];
    bp = int(float((point[bi]+r*xAxis[bi])/w));
    cp = int(float((point[ci]+r*xAxis[ci])/w));
    }
  else
    {
    w = point[3]+r*xAxis[3];
    bp = int((point[bi]+r*xAxis[bi])/w + 0.5);
    cp = int((point[ci]+r*xAxis[ci])/w + 0.5);
    }
  return (bp >= inMin[bi] && bp <= inMax[bi] &&
	  cp >= inMin[ci] && cp <= inMax[ci]);
}

// this huge mess finds out where the current output raster
// line intersects the input volume 
int vtkImageReslice::FindExtent(int& r1, int& r2, double *point, float *xAxis, 
		      int *inMin, int *inMax, int interpolate)
{
  int i, ix, iy, iz;
  int sign[3];
  int indx1[4],indx2[4];
  double w1,w2;

  // find signs of components of x axis 
  // (this is complicated due to the homogenous coordinate)
  for (i = 0; i < 3; i++)
    {
    if (point[i]/point[3] <= (point[i]+xAxis[i])/(point[3]+xAxis[3]))
      {
      sign[i] = 1;
      }
    else 
      {
      sign[i] = -1;
      }
    } 
  
  // order components of xAxis from largest to smallest
  
  ix = 0;
  for (i = 1; i < 3; i++)
    {
    if (xAxis[i]*xAxis[i] > xAxis[ix]*xAxis[ix])
      {
      ix = i;
      }
    }
  
  iy = ((ix > 1) ? ix-2 : ix+1);
  iz = ((ix > 0) ? ix-1 : ix+2);

  if (xAxis[iz]*xAxis[iz] > xAxis[iy]*xAxis[iy])
    {
    i = iy;
    iy = iz;
    iz = i;
    }

  r1 = intersectionLow(point,xAxis,sign,inMin,ix,interpolate);
  r2 = intersectionHigh(point,xAxis,sign,inMax,ix,interpolate);
  
  // find points of intersections
  // first, find w
  w1 = point[3]+r1*xAxis[3];
  w2 = point[3]+r2*xAxis[3];
  
  if (interpolate)
    {
    for (i = 0; i < 3; i++)
      {
      indx1[i] = int(float((point[i]+r1*xAxis[i])/w1));
      indx2[i] = int(float((point[i]+r2*xAxis[i])/w2));
      }
    }
  else
    {
    for (i = 0; i < 3; i++)
      {
      indx1[i] = int((point[i]+r1*xAxis[i])/w1+0.5);
      indx2[i] = int((point[i]+r2*xAxis[i])/w2+0.5);
      }
    }
  if (isBounded(point,xAxis,inMin,inMax,ix,r1,interpolate))
    { // passed through x face, check opposing face
    if (isBounded(point,xAxis,inMin,inMax,ix,r2,interpolate))
      {
      return sign[ix];
      }
    
    if (indx2[iy] < inMin[iy])
      { // check y face
      r2 = intersectionLow(point,xAxis,sign,inMin,iy,interpolate);
      if (isBounded(point,xAxis,inMin,inMax,iy,r2,interpolate))
	{
	return sign[ix];
	}
      }
    else if (indx2[iy] > inMax[iy])
      { // check other y face
      r2 = intersectionHigh(point,xAxis,sign,inMax,iy,interpolate);
      if (isBounded(point,xAxis,inMin,inMax,iy,r2,interpolate))
	{
	return sign[ix];
	}
      }
    
    if (indx2[iz] < inMin[iz])
      { // check z face
      r2 = intersectionLow(point,xAxis,sign,inMin,iz,interpolate);
      if (isBounded(point,xAxis,inMin,inMax,iz,r2,interpolate))
	{
	return sign[ix];
	}
      }
    else if (indx2[iz] > inMax[iz])
      { // check other z face
      r2 = intersectionHigh(point,xAxis,sign,inMax,iz,interpolate);
      if (isBounded(point,xAxis,inMin,inMax,iz,r2,interpolate))
	{
	return sign[ix];
	}
      }
    }
  
  if (isBounded(point,xAxis,inMin,inMax,ix,r2,interpolate))
    { // passed through the opposite x face
    if (indx1[iy] < inMin[iy])
	{ // check y face
	r1 = intersectionLow(point,xAxis,sign,inMin,iy,interpolate);
	if (isBounded(point,xAxis,inMin,inMax,iy,r1,interpolate))
	  {
	  return sign[ix];
	  }
	}
    else if (indx1[iy] > inMax[iy])
      { // check other y face
      r1 = intersectionHigh(point,xAxis,sign,inMax,iy,interpolate);
      if (isBounded(point,xAxis,inMin,inMax,iy,r1,interpolate))
	{
	return sign[ix];
	}
      }
    
    if (indx1[iz] < inMin[iz])
      { // check z face
      r1 = intersectionLow(point,xAxis,sign,inMin,iz,interpolate);
      if (isBounded(point,xAxis,inMin,inMax,iz,r1,interpolate))
	{
	return sign[ix];
	}
      }
    else if (indx1[iz] > inMax[iz])
      { // check other z face
      r1 = intersectionHigh(point,xAxis,sign,inMax,iz,interpolate);
      if (isBounded(point,xAxis,inMin,inMax,iz,r1,interpolate))
	{
	return sign[ix];
	}
      }
    }
  
  if ((indx1[iy] >= inMin[iy] && indx2[iy] < inMin[iy]) ||
      (indx1[iy] < inMin[iy] && indx2[iy] >= inMin[iy]))
    { // line might pass through bottom face
    r1 = intersectionLow(point,xAxis,sign,inMin,iy,interpolate);
    if (isBounded(point,xAxis,inMin,inMax,iy,r1,interpolate))
      {
      if ((indx1[iy] <= inMax[iy] && indx2[iy] > inMax[iy]) ||
	  (indx1[iy] > inMax[iy] && indx2[iy] <= inMax[iy]))
	{ // line might pass through top face
	r2 = intersectionHigh(point,xAxis,sign,inMax,iy,interpolate);
	if (isBounded(point,xAxis,inMin,inMax,iy,r2,interpolate))
	  {
	  return sign[iy];
	  }
	}
      
      if (indx1[iz] < inMin[iz] && indx2[iy] < inMin[iy] ||
	  indx2[iz] < inMin[iz] && indx1[iy] < inMin[iy])
	{ // line might pass through in-to-screen face
	r2 = intersectionLow(point,xAxis,sign,inMin,iz,interpolate);
	if (isBounded(point,xAxis,inMin,inMax,iz,r2,interpolate))
	  {
	  return sign[iy];
	  }
	}
      else if (indx1[iz] > inMax[iz] && indx2[iy] < inMin[iy] ||
	       indx2[iz] > inMax[iz] && indx1[iy] < inMin[iy])
	{ // line might pass through out-of-screen face
	r2 = intersectionHigh(point,xAxis,sign,inMax,iz,interpolate);
	if (isBounded(point,xAxis,inMin,inMax,iz,r2,interpolate))
	  {
	  return sign[iy];
	  }
	} 
      }
    }
  
  if ((indx1[iy] <= inMax[iy] && indx2[iy] > inMax[iy]) ||
      (indx1[iy] > inMax[iy] && indx2[iy] <= inMax[iy]))
    { // line might pass through top face
    r2 = intersectionHigh(point,xAxis,sign,inMax,iy,interpolate);
    if (isBounded(point,xAxis,inMin,inMax,iy,r2,interpolate))
      {
      if (indx1[iz] < inMin[iz] && indx2[iy] > inMax[iy] ||
	  indx2[iz] < inMin[iz] && indx1[iy] > inMax[iy])
	{ // line might pass through in-to-screen face
	r1 = intersectionLow(point,xAxis,sign,inMin,iz,interpolate);
	if (isBounded(point,xAxis,inMin,inMax,iz,r1,interpolate))
	  {
	  return sign[iy];
	  }
	}
      else if (indx1[iz] > inMax[iz] && indx2[iy] > inMax[iy] || 
	       indx2[iz] > inMax[iz] && indx1[iy] > inMax[iy])
	{ // line might pass through out-of-screen face
	r1 = intersectionHigh(point,xAxis,sign,inMax,iz,interpolate);
	if (isBounded(point,xAxis,inMin,inMax,iz,r1,interpolate))
	  {
	  return sign[iy];
	  }
	}
      } 
    }
  
  if ((indx1[iz] >= inMin[iz] && indx2[iz] < inMin[iz]) ||
      (indx1[iz] < inMin[iz] && indx2[iz] >= inMin[iz]))
    { // line might pass through in-to-screen face
    r1 = intersectionLow(point,xAxis,sign,inMin,iz,interpolate);
    if (isBounded(point,xAxis,inMin,inMax,iz,r1,interpolate))
      {
      if (indx1[iz] > inMax[iz] || indx2[iz] > inMax[iz])
	{ // line might pass through out-of-screen face
	r2 = intersectionHigh(point,xAxis,sign,inMax,iz,interpolate);
	if (isBounded(point,xAxis,inMin,inMax,iz,r2,interpolate))
	  {
	  return sign[iz];
	  }
	}
      }
    }
  
  r1 = r2 = -1;
  return 1;
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageResliceExecute(vtkImageReslice *self,
				   vtkImageData *inData, T *inPtr,
				   vtkImageData *outData, T *outPtr,
				   int outExt[6], int id)
{
  int i;
  int idX, idY, idZ;
  int outIncX, outIncY, outIncZ;
  int inIdX, inIdY, inIdZ;
  int inExt[6];
  int inMax[3], inMin[3];
  int inIncX, inIncY, inIncZ;
  unsigned long count = 0;
  unsigned long target;
  int r1,r2;
  double inPoint0[4];
  double inPoint1[4];
  float *xAxis, *yAxis, *zAxis, *origin;
  T Background;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  int terpOffset;
  double w;

  // 1 pixel buffer required if interpolation is used
  terpOffset = ((self->GetInterpolate() != 0) ? 1 : 0); 
  
  // Color for area outside of input volume extent
  Background = T(self->GetBackgroundLevel());
  
  // find maximum input range
  inData->GetExtent(inExt);
  
  for (i = 0; i < 3; i++)
    {
    inMin[i] = inExt[2*i];
    inMax[i] = inExt[2*i+1] - terpOffset;
    }
  
  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  outIncX = inData->GetNumberOfScalarComponents();
  
  // change transform matrix so that instead of taking 
  // input coords -> output coords it takes output indices -> input indices
  self->ComputeIndexMatrix(matrix);
  
  // break matrix into a set of axes plus an origin
  // (this allows us to calculate the transform Incrementally)
  matrix->Transpose();
  xAxis = matrix->Element[0];
  yAxis = matrix->Element[1];
  zAxis = matrix->Element[2];
  origin = matrix->Element[3];
  
  // Loop through ouput pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    inPoint0[0] = origin[0]+idZ*zAxis[0]; // incremental transform
    inPoint0[1] = origin[1]+idZ*zAxis[1]; 
    inPoint0[2] = origin[2]+idZ*zAxis[2]; 
    inPoint0[3] = origin[3]+idZ*zAxis[3]; 
    
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      inPoint1[0] = inPoint0[0]+idY*yAxis[0]; // incremental transform
      inPoint1[1] = inPoint0[1]+idY*yAxis[1];
      inPoint1[2] = inPoint0[2]+idY*yAxis[2];
      inPoint1[3] = inPoint0[3]+idY*yAxis[3];
      
      if (!id) 
	{
	if (!(count%target)) 
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      
      // find intersections of x raster line with the input extent
      if (self->FindExtent(r1,r2,inPoint1,xAxis,inMin,inMax,terpOffset) < 0)
	{
	i = r1;
	r1 = r2;
	r2 = i;
	}
      
      // bound r1,r2 within reasonable limits
      r1 = ((r1 > outExt[1]+1) ? outExt[1]+1 : r1); 
      r2 = ((r2 > outExt[1]) ? outExt[1] : r2); 
      
      // clear pixels until we reach input extent
      if (outIncX == 1) // optimize for single scalar
	{
	for (idX = outExt[0]; idX < r1; idX++) 
	  {
	  *outPtr++ = Background;
	  }
	}
      else             // multiple scalars
	for (idX = outExt[0]; idX < r1; idX++)
	  for (i = 0; i < outIncX; i++)
	    *outPtr++ = Background;
      
      // interpolate pixels while inside input extent
      if (self->GetInterpolate())
	{ // do trilinear interpolation
	for (; idX <= r2; idX++)
	  {
	  float x,y,z,fx,fy,fz,rx,ry,rz,ryrz,ryfz,fyrz,fyfz;
	  T v000,v001,v010,v011,v100,v101,v110,v111;
	  int factX,factY,factZ,factX1,factY1,factZ1;
	  
	  w = inPoint1[3]+idX*xAxis[3]; // don't forget w!  
	  inIdX = int(x=float((inPoint1[0]+idX*xAxis[0])/w))-inExt[0];
	  inIdY = int(y=float((inPoint1[1]+idX*xAxis[1])/w))-inExt[2];
	  inIdZ = int(z=float((inPoint1[2]+idX*xAxis[2])/w))-inExt[4];
	  factX = inIdX*inIncX;
	  factY = inIdY*inIncY;
	  factZ = inIdZ*inIncZ;
	  factX1 = (inIdX+1)*inIncX;
	  factY1 = (inIdY+1)*inIncY;
	  factZ1 = (inIdZ+1)*inIncZ;
	  
	  for (i = 0; i < outIncX; i++) 
	    {
	    v000 = *(inPtr+factX+factY+factZ);
	    v001 = *(inPtr+factX+factY+factZ1);
	    v010 = *(inPtr+factX+factY1+factZ);
	    v011 = *(inPtr+factX+factY1+factZ1);
	    v100 = *(inPtr+factX1+factY+factZ);
	    v101 = *(inPtr+factX1+factY+factZ1);
	    v110 = *(inPtr+factX1+factY1+factZ);
	    v111 = *(inPtr+factX1+factY1+factZ1);
	    
	    rx = 1 - (fx = x-(inIdX+inExt[0]));
	    ry = 1 - (fy = y-(inIdY+inExt[2]));
	    rz = 1 - (fz = z-(inIdZ+inExt[4]));
	    
	    ryrz = ry*rz;
	    ryfz = ry*fz;
	    fyrz = fy*rz;
	    fyfz = fy*fz;
	    
	    *outPtr++ = 
	      T(rx*(ryrz*v000+ryfz*v001+fyrz*v010+fyfz*v011)
		+ fx*(ryrz*v100+ryfz*v101+fyrz*v110+fyfz*v111));
	    
	    inPtr++;
	    }
	  inPtr -= outIncX;
	  }
	}
      else
	{ // do nearest-neighbor interpolation
	if (outIncX == 1)
	  { // optimize for single scalar (keep that inner loop tight!)
	  for (; idX <= r2; idX++)
	    {
	    w = inPoint1[3]+idX*xAxis[3]; // don't forget w!  
	    inIdX = int((inPoint1[0]+idX*xAxis[0])/w+0.5)-inExt[0];
	    inIdY = int((inPoint1[1]+idX*xAxis[1])/w+0.5)-inExt[2];
	    inIdZ = int((inPoint1[2]+idX*xAxis[2])/w+0.5)-inExt[4];
	    
	    *outPtr++ = *(inPtr+inIdX*inIncX+inIdY*inIncY
			  +inIdZ*inIncZ);
	    }
	  }
	else
	  { // multiple scalar values
	  for (; idX <= r2; idX++)
	    {
	    w = inPoint1[3]+idX*xAxis[3]; // don't forget w!  
	    inIdX = int((inPoint1[0]+idX*xAxis[0])/w+0.5)-inExt[0];
	    inIdY = int((inPoint1[1]+idX*xAxis[1])/w+0.5)-inExt[2];
	    inIdZ = int((inPoint1[2]+idX*xAxis[2])/w+0.5)-inExt[4];
	    
	    for (i = 0; i < outIncX; i++)
	      *outPtr++ = *(inPtr+inIdX*inIncX+inIdY*inIncY
			    +inIdZ*inIncZ);
	    }
	  }
	}
      
      // clear pixels now that we are beyond input extent
      if (outIncX == 1) // optimize for single scalar
	for (; idX <= outExt[1]; idX++) 
	  *outPtr++ = Background;
      else // multiple scalars
	for (; idX <= outExt[1]; idX++)
	  for (i = 0; i < outIncX; i++)
	    *outPtr++ = Background;
      
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
  matrix->Delete();
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageReslice::ThreadedExecute(vtkImageData *inData, 
				      vtkImageData *outData,
				      int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(inData->GetExtent());
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
            << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageResliceExecute(this, inData, (float *)(inPtr), 
			     outData, (float *)(outPtr),outExt, id);
      break;
    case VTK_INT:
      vtkImageResliceExecute(this, inData, (int *)(inPtr), 
			     outData, (int *)(outPtr),outExt, id);
      break;
    case VTK_SHORT:
      vtkImageResliceExecute(this, inData, (short *)(inPtr), 
			     outData, (short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageResliceExecute(this, inData, (unsigned short *)(inPtr), 
			     outData, (unsigned short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageResliceExecute(this, inData, (unsigned char *)(inPtr), 
			     outData, (unsigned char *)(outPtr),outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}


