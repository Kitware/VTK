/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.cxx
  Language:  C++
  Date:      $Date$

  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageReslice.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkImageReslice* vtkImageReslice::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageReslice");
  if(ret)
    {
    return (vtkImageReslice*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageReslice;
}

//----------------------------------------------------------------------------
vtkImageReslice::vtkImageReslice()
{
  this->OutputSpacing[0] = 1;
  this->OutputSpacing[1] = 1;
  this->OutputSpacing[2] = 1;

  this->OutputOrigin[0] = FLT_MAX; // flag to set defaults later
  this->OutputExtent[0] = INT_MAX; // ditto

  this->Wrap = 0; // don't wrap
  this->Mirror = 0; // don't mirror
  this->InterpolationMode = VTK_RESLICE_NEAREST; // no interpolation
  this->Optimization = 1; // optimizations seem to finally be stable...

  this->BackgroundColor[0] = 0;
  this->BackgroundColor[1] = 0;
  this->BackgroundColor[2] = 0;
  this->BackgroundColor[3] = 0;

  this->ResliceAxes = NULL;
  this->ResliceTransform = NULL;
  this->IndexMatrix = NULL;
}

//----------------------------------------------------------------------------
vtkImageReslice::~vtkImageReslice()
{
  this->SetResliceTransform(NULL);
  this->SetResliceAxes(NULL);
  if (this->IndexMatrix)
    {
    this->IndexMatrix->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "ResliceAxes: " << this->ResliceAxes << "\n";
  if (this->ResliceAxes)
    {
    this->ResliceAxes->PrintSelf(os,indent.GetNextIndent());
    }
  os << indent << "ResliceTransform: " << this->ResliceTransform << "\n";
  if (this->ResliceTransform)
    {
    this->ResliceTransform->PrintSelf(os,indent.GetNextIndent());
    }
  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " <<
    this->OutputSpacing[1] << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " <<
    this->OutputOrigin[1] << " " << this->OutputOrigin[2] << "\n";
  os << indent << "OutputExtent: " << this->OutputExtent[0] << " " <<
    this->OutputExtent[1] << " " << this->OutputExtent[2] << " " <<
    this->OutputExtent[3] << " " << this->OutputExtent[4] << " " <<
    this->OutputExtent[5] << "\n";
  os << indent << "Wrap: " << (this->Wrap ? "On\n":"Off\n");
  os << indent << "Mirror: " << (this->Mirror ? "On\n":"Off\n");
  os << indent << "InterpolationMode: " 
     << this->GetInterpolationModeAsString() << "\n";
  os << indent << "Optimization: " << (this->Optimization ? "On\n":"Off\n");
  os << indent << "BackgroundColor: " << this->BackgroundColor[0] << " " <<
    this->BackgroundColor[1] << " " << this->BackgroundColor[2] << " " <<
    this->BackgroundColor[3] << "\n";
}

//----------------------------------------------------------------------------
// Account for the MTime of the transform and its matrix when determinging
// the MTime of the filter

unsigned long int vtkImageReslice::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;

  if ( this->ResliceTransform != NULL )
    {
    time = this->ResliceTransform->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    time = this->ResliceTransform->GetMatrixPointer()->GetMTime();
    mTime = ( time > mTime ? time : mTime );    
    }
  if ( this->ResliceAxes != NULL)
    {
    time = this->ResliceAxes->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
// The transform matrix supplied by the user converts output coordinates
// to input coordinates.  
// To speed up the pixel lookup, the following function provides a
// matrix which converts output pixel indices to input pixel indices.

vtkMatrix4x4 *vtkImageReslice::GetIndexMatrix()
{
  // first verify that we have to update the matrix
  if (this->IndexMatrix)
    {
    if (this->IndexMatrix->GetMTime() > this->GetMTime())
      {
      return this->IndexMatrix;
      }
    }
  else
    {
    this->IndexMatrix = vtkMatrix4x4::New();
    }

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

  if (this->GetResliceAxes())
    {
    transform->SetMatrix(*this->GetResliceAxes());
    }
  if (this->GetResliceTransform())
    {
    transform->PostMultiply();
    transform->Concatenate(this->GetResliceTransform()->GetMatrixPointer());
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

  transform->GetMatrix(this->IndexMatrix);
  
  transform->Delete();
  inMatrix->Delete();
  outMatrix->Delete();

  return this->IndexMatrix;
}

//----------------------------------------------------------------------------
static void ComputeInputUpdateExtentOptimized(vtkImageReslice *self,
					      int inExt[6], 
					      int outExt[6]);

void vtkImageReslice::ComputeRequiredInputUpdateExtent(int inExt[6], 
					               int outExt[6])
{
  if (this->GetOptimization())
    {
    ComputeInputUpdateExtentOptimized(this,inExt,outExt);
    return;
    }

  int i,j,k;
  float point[4];

  // convert matrix from world coordinates to pixel indices
  vtkMatrix4x4 *matrix = this->GetIndexMatrix();

  for (i = 0; i < 3; i++)
    {
    inExt[2*i] = INT_MAX;
    inExt[2*i+1] = INT_MIN;
    }

  // check the coordinates of the 8 corners of the output extent
  for (i = 0; i < 8; i++)  
    {
    // get output coords
    point[0] = outExt[i%2];
    point[1] = outExt[2+(i/2)%2];
    point[2] = outExt[4+(i/4)%2];
    point[3] = 1.0;

    // convert to input coords
    matrix->MultiplyPoint(point,point);

    point[0] /= point[3];
    point[1] /= point[3];
    point[2] /= point[3];
    point[3] = 1.0;

    // set 
    if (this->GetInterpolationMode() != VTK_RESLICE_NEAREST)
      {
      int extra = (this->GetInterpolationMode() == VTK_RESLICE_CUBIC); 
      for (j = 0; j < 3; j++) 
	{
	k = int(floor(point[j]))-extra;
	if (k < inExt[2*j]) 
	  {
	  inExt[2*j] = k;
	  } 
	k = int(ceil(point[j]))+extra;	
	if (k > inExt[2*j+1])
	  {
	  inExt[2*j+1] = k;
	  }
	}
      }
    else
      {
      for (j = 0; j < 3; j++) 
	{
	k = int(floor(point[j] + 0.5));
	if (k < inExt[2*j])
	  { 
	  inExt[2*j] = k;
	  } 
	if (k > inExt[2*j+1]) 
	  {
	  inExt[2*j+1] = k;
	  }
	}
      }
    }

  int *wholeExtent = this->GetInput()->GetWholeExtent();
  // Clip, just to make sure we hit _some_ of the input extent
  for (i = 0; i < 6; i++)
    {
    j = i - i%2;
    if (inExt[i] < wholeExtent[j])
      {
      inExt[i] = wholeExtent[j];
      }
    j++;
    if (inExt[i] > wholeExtent[j])
      {
      inExt[i] = wholeExtent[j];
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::ExecuteInformation(vtkImageData *input, 
					 vtkImageData *output) 
{
  int i,j;
  float inPoint[4], outPoint[4];
  float inOrigin[3],maxOut[3],minOut[3],tmp;
  float *inSpacing;

  int *inWholeExt;

  input->UpdateInformation();
  inWholeExt = input->GetWholeExtent();
  inSpacing = input->GetSpacing();
  input->GetOrigin(inOrigin);
  
  vtkTransform *transform = vtkTransform::New();

  if (this->GetResliceAxes())
    {
    transform->SetMatrix(*this->GetResliceAxes());
    }
  if (this->GetResliceTransform())
    {
    transform->PostMultiply();
    transform->Concatenate(this->GetResliceTransform()->GetMatrixPointer());
    }
  
  vtkMatrix4x4 *matrix = transform->GetMatrixPointer();

  // because vtkMatrix4x4::Inverse() doesn't cut it,
  // use vtkMath::InvertMatrix()
  double mat1data[4][4];
  double mat2data[4][4];
  double *mat1[4];
  double *mat2[4];
    
  int tmpIntSpace[4];
  double tmpDoubleSpace[4];

  for (i = 0; i < 4; i++)
    {
    mat1[i] = mat1data[i];
    mat2[i] = mat2data[i];
    for (j = 0; j < 4; j++)
      {  // InvertMatrix transposes the matrix (?)
      mat1[i][j] = matrix->GetElement(i,j);
      }
    }
 
  if (vtkMath::InvertMatrix(mat1,mat2,4,tmpIntSpace,tmpDoubleSpace) == 0)
    {
    vtkErrorMacro(<< "ExecuteInformation: reslicing transform not \
invertible");
    }

  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      matrix->SetElement(i,j,mat2[j][i]);
      }
    }

  // default extent covers entire input extent
  if (this->OutputExtent[0] == INT_MAX)
    {
    for (i = 0; i < 3; i++)
      {
      minOut[i] = FLT_MAX;
      maxOut[i] = -FLT_MAX;
      }
    
    for (i = 0; i < 8; i++)
      {
      inPoint[0] = inOrigin[0] + inWholeExt[i%2]*inSpacing[0];
      inPoint[1] = inOrigin[1] + inWholeExt[2+(i/2)%2]*inSpacing[1];
      inPoint[2] = inOrigin[2] + inWholeExt[4+(i/4)%2]*inSpacing[2];
      inPoint[3] = 1;
      
      matrix->MultiplyPoint(inPoint,outPoint);
      
      for (j = 0; j < 3; j++) 
	{
	tmp = outPoint[j]/outPoint[3];
	if (tmp > maxOut[j])
	  {
	  maxOut[j] = tmp;
	  }
	if (tmp < minOut[j])
	  {
	  minOut[j] = tmp;
	  }
	}
      }
    
    for (i = 0; i < 3; i++)
      {
      this->OutputExtent[2*i] = inWholeExt[2*i];
      this->OutputExtent[2*i+1] = inWholeExt[2*i]+
	int(ceil((maxOut[i]-minOut[i]+1)/this->OutputSpacing[i])) - 1;
      }    
    }

  // default origin places centre of output over centre of input
  if (this->OutputOrigin[0] == FLT_MAX)
    {
    for (i = 0; i < 3; i++)
      {
      inPoint[i] = inOrigin[i] + 
	inSpacing[i]*(inWholeExt[2*i]+inWholeExt[2*i+1])*0.5;
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
  
  output->SetWholeExtent(this->OutputExtent);
  output->SetSpacing(this->OutputSpacing);
  output->SetOrigin(this->OutputOrigin);
  output->SetScalarType(input->GetScalarType());
  output->SetNumberOfScalarComponents(input->GetNumberOfScalarComponents());

  transform->Delete();
}

//----------------------------------------------------------------------------
//  Interplolation subroutines and associated code
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// rounding functions, split and optimized for each type
// (because we don't want to round if the result is a float!)

// in the case of a tie between integers, the larger integer wins.

static inline void vtkResliceRound(double val, unsigned char& rnd)
{
  rnd = (unsigned char)(val+0.5);
}

static inline void vtkResliceRound(double val, short& rnd)
{
  rnd = (short)((int)(val+32768.5)-32768);
}

static inline void vtkResliceRound(double val, unsigned short& rnd)
{
  rnd = (unsigned short)(val+0.5);
}

static inline void vtkResliceRound(double val, int& rnd)
{
  rnd = (int)(floor(val+0.5));
}

static inline void vtkResliceRound(double val, float& rnd)
{
  rnd = (float)(val);
}

//----------------------------------------------------------------------------
// clamping functions for each type

static inline void vtkResliceClamp(double val, unsigned char& clamp)
{
  if (val < VTK_UNSIGNED_CHAR_MIN)
    { 
    val = VTK_UNSIGNED_CHAR_MIN;
    }
  if (val > VTK_UNSIGNED_CHAR_MAX)
    { 
    val = VTK_UNSIGNED_CHAR_MAX;
    }
  vtkResliceRound(val,clamp);
}

static inline void vtkResliceClamp(double val, short& clamp)
{
  if (val < VTK_SHORT_MIN)
    { 
    val = VTK_SHORT_MIN;
    }
  if (val > VTK_SHORT_MAX)
    { 
    val = VTK_SHORT_MAX;
    }
  vtkResliceRound(val,clamp);
}

static inline void vtkResliceClamp(double val, unsigned short& clamp)
{
  if (val < VTK_UNSIGNED_SHORT_MIN)
    { 
    val = VTK_UNSIGNED_SHORT_MIN;
    }
  if (val > VTK_UNSIGNED_SHORT_MAX)
    { 
    val = VTK_UNSIGNED_SHORT_MAX;
    }
  vtkResliceRound(val,clamp);
}

static inline void vtkResliceClamp(double val, int& clamp)
{
  if (val < VTK_INT_MIN) 
    {
    val = VTK_INT_MIN;
    }
  if (val > VTK_INT_MAX) 
    {
    val = VTK_INT_MAX;
    }
  vtkResliceRound(val,clamp);
}

static inline void vtkResliceClamp(double val, float& clamp)
{
  if (val < VTK_FLOAT_MIN)
    { 
    val = VTK_FLOAT_MIN;
    }
  if (val > VTK_FLOAT_MAX) 
    {
    val = VTK_FLOAT_MAX;
    }
  vtkResliceRound(val,clamp);
}

//----------------------------------------------------------------------------
// Perform a wrap to limit an index to [0,range).
// Ensures correct behaviour when the index is negative.
 
static inline int vtkInterpolateWrap(int num, int range)
{
  if ((num %= range) < 0)
    {
    num += range; // required for some % implementations
    } 
  return num;
}

//----------------------------------------------------------------------------
// Perform a mirror to limit an index to [0,range).
 
static inline int vtkInterpolateMirror(int num, int range)
{
  if (num < 0)
    {
    num = -num-1;
    }
  int count = num/range;
  num %= range;
  if (count & 0x1)
    {
    num = range-num-1;
    }
  return num;
}

//----------------------------------------------------------------------------
// Do trilinear interpolation of the input data 'inPtr' of extent 'inExt'
// at the 'point'.  The result is placed at 'outPtr'.  
// If the lookup data is beyond the extent 'inExt', set 'outPtr' to
// the background color 'background'.  
// The number of scalar components in the data is 'numscalars'
template <class T>
static int vtkTrilinearInterpolation(float *point, T *inPtr, T *outPtr,
				     T *background, int numscalars, 
				     int inExt[6],
				     int vtkNotUsed(inDim)[3],
				     int inInc[3])
{
  int i;

  // the +1/-1 avoids round-to-zero truncation between -1 and 0,
  // and is cheaper than doing floor()
  int floorX = int(point[0]+1)-1;
  int floorY = int(point[1]+1)-1;
  int floorZ = int(point[2]+1)-1;

  float fx = point[0]-floorX;
  float fy = point[1]-floorY;
  float fz = point[2]-floorZ;

  int inIdX = floorX-inExt[0];
  int inIdY = floorY-inExt[2];
  int inIdZ = floorZ-inExt[4];

  // the doInterpX,Y,Z variables are 0 if interpolation
  // does not have to be done in the specified direction,
  // i.e. if the x, y or z lookup indices have no fractional
  // component. 
  int doInterpX = (fx != 0);
  int doInterpY = (fy != 0);
  int doInterpZ = (fz != 0);
  
  if (inIdX < 0 || inIdX+doInterpX > inExt[1]-inExt[0]
      || inIdY < 0 || inIdY+doInterpY > inExt[3]-inExt[2]
      || inIdZ < 0 || inIdZ+doInterpZ > inExt[5]-inExt[4] )
    {// out of bounds: clear to background color 
    if (background)
      {
      for (i = 0; i < numscalars; i++)
	{ 
        *outPtr++ = *background++;
	}
      }
    return 0;
    }
  else 
    {// do trilinear interpolation
    int factX = inIdX*inInc[0];
    int factY = inIdY*inInc[1];
    int factZ = inIdZ*inInc[2];

    int factX1 = (inIdX+doInterpX)*inInc[0];
    int factY1 = (inIdY+doInterpY)*inInc[1];
    int factZ1 = (inIdZ+doInterpZ)*inInc[2];
    
    int i000 = factX+factY+factZ;
    int i001 = factX+factY+factZ1;
    int i010 = factX+factY1+factZ;
    int i011 = factX+factY1+factZ1;
    int i100 = factX1+factY+factZ;
    int i101 = factX1+factY+factZ1;
    int i110 = factX1+factY1+factZ;
    int i111 = factX1+factY1+factZ1;

    float rx = 1 - fx;
    float ry = 1 - fy;
    float rz = 1 - fz;
      
    float ryrz = ry*rz;
    float ryfz = ry*fz;
    float fyrz = fy*rz;
    float fyfz = fy*fz;

    for (i = 0; i < numscalars; i++) 
      {
      vtkResliceRound((rx*(ryrz*inPtr[i000]+ryfz*inPtr[i001]+
			   fyrz*inPtr[i010]+fyfz*inPtr[i011])
		       + fx*(ryrz*inPtr[i100]+ryfz*inPtr[i101]+
			     fyrz*inPtr[i110]+fyfz*inPtr[i111])),
		      *outPtr++);
      inPtr++;
      }
    return 1;
    }
}			  

// trilinear interpolation with wrap-around behaviour
template <class T>
static int vtkTrilinearInterpolationRepeat(float *point, T *inPtr, T *outPtr,
					   T *mirror, int numscalars, 
					   int inExt[6], int inDim[3],
					   int inInc[3])
{
  int i;

  int floorX = int(point[0]+1)-1;
  int floorY = int(point[1]+1)-1;
  int floorZ = int(point[2]+1)-1;

  float fx = point[0]-floorX;
  float fy = point[1]-floorY;
  float fz = point[2]-floorZ;

  // this corrects for differences between int() and floor()
  if (fx < 0)
    {
    fx = point[0] - (--floorX);
    }
  if (fy < 0)
    {
    fy = point[1] - (--floorY);
    }
  if (fz < 0)
    {
    fz = point[2] - (--floorZ);
    }

  int inIdX = floorX-inExt[0];
  int inIdY = floorY-inExt[2];
  int inIdZ = floorZ-inExt[4];

  int factX, factY, factZ;
  int factX1, factY1, factZ1;

  if (*mirror)
    {
    factX = vtkInterpolateMirror(inIdX,inDim[0])*inInc[0];
    factY = vtkInterpolateMirror(inIdY,inDim[1])*inInc[1];
    factZ = vtkInterpolateMirror(inIdZ,inDim[2])*inInc[2];

    factX1 = vtkInterpolateMirror(inIdX+1,inDim[0])*inInc[0];
    factY1 = vtkInterpolateMirror(inIdY+1,inDim[1])*inInc[1];
    factZ1 = vtkInterpolateMirror(inIdZ+1,inDim[2])*inInc[2];
    }
  else
    {
    factX = vtkInterpolateWrap(inIdX,inDim[0])*inInc[0];
    factY = vtkInterpolateWrap(inIdY,inDim[1])*inInc[1];
    factZ = vtkInterpolateWrap(inIdZ,inDim[2])*inInc[2];

    factX1 = vtkInterpolateWrap(inIdX+1,inDim[0])*inInc[0];
    factY1 = vtkInterpolateWrap(inIdY+1,inDim[1])*inInc[1];
    factZ1 = vtkInterpolateWrap(inIdZ+1,inDim[2])*inInc[2];
    }

  int i000 = factX+factY+factZ;
  int i001 = factX+factY+factZ1;
  int i010 = factX+factY1+factZ;
  int i011 = factX+factY1+factZ1;
  int i100 = factX1+factY+factZ;
  int i101 = factX1+factY+factZ1;
  int i110 = factX1+factY1+factZ;
  int i111 = factX1+factY1+factZ1;

  float rx = 1 - fx;
  float ry = 1 - fy;
  float rz = 1 - fz;
  
  float ryrz = ry*rz;
  float ryfz = ry*fz;
  float fyrz = fy*rz;
  float fyfz = fy*fz;

  for (i = 0; i < numscalars; i++) 
    {
    vtkResliceRound((rx*(ryrz*inPtr[i000]+ryfz*inPtr[i001]+
			 fyrz*inPtr[i010]+fyfz*inPtr[i011])
		     + fx*(ryrz*inPtr[i100]+ryfz*inPtr[i101]+
			   fyrz*inPtr[i110]+fyfz*inPtr[i111])),
		    *outPtr++);
    inPtr++;
    }
  return 1;
}			  

// Do nearest-neighbor interpolation of the input data 'inPtr' of extent 
// 'inExt' at the 'point'.  The result is placed at 'outPtr'.  
// If the lookup data is beyond the extent 'inExt', set 'outPtr' to
// the background color 'background'.  
// The number of scalar components in the data is 'numscalars'

template <class T>
static int vtkNearestNeighborInterpolation(float *point, T *inPtr, T *outPtr,
                                           T *background, int numscalars, 
                                           int inExt[6],
					   int vtkNotUsed(inDim)[3],
					   int inInc[3])
{
  int i;
  int inIdX = int(point[0]+1.5)-inExt[0]-1;
  int inIdY = int(point[1]+1.5)-inExt[2]-1;
  int inIdZ = int(point[2]+1.5)-inExt[4]-1;
  
  if (inIdX < 0 || inIdX > inExt[1]-inExt[0]
      || inIdY < 0 || inIdY > inExt[3]-inExt[2]
      || inIdZ < 0 || inIdZ > inExt[5]-inExt[4] )
    {
    if (background)
      {
      for (i = 0; i < numscalars; i++)
	{
	*outPtr++ = *background++;
	}
      }
    return 0;
    }
  else 
    {
    inPtr += inIdX*inInc[0]+inIdY*inInc[1]+inIdZ*inInc[2];
    for (i = 0; i < numscalars; i++)
      {
      *outPtr++ = *inPtr++;
      }
    return 1;
    }
} 

// nearest-neighbor interpolation with wrap-around behaviour
template <class T>
static int vtkNearestNeighborInterpolationRepeat(float *point, T *inPtr, 
						 T *outPtr,
						 T *mirror, int numscalars, 
						 int inExt[6], 
						 int inDim[3],
						 int inInc[3])
{
  int i;

  // round-to-zero vs. round-to-neg-infinity strikes again
  float vX = point[0]+1.5;
  float vY = point[1]+1.5;
  float vZ = point[2]+1.5;

  int floorX = int(vX)-1;
  int floorY = int(vY)-1;
  int floorZ = int(vZ)-1;

  if (vX < floorX+1) 
    {
    floorX--;
    }
  if (vY < floorY+1)
    { 
    floorY--;
    }
  if (vZ < floorZ+1) 
    {
    floorZ--;
    }

  int inIdX, inIdY, inIdZ;

  if (*mirror)
    {
      inIdX = vtkInterpolateMirror(floorX-inExt[0],inDim[0]);
      inIdY = vtkInterpolateMirror(floorY-inExt[2],inDim[1]);
      inIdZ = vtkInterpolateMirror(floorZ-inExt[4],inDim[2]);
    }
  else
    {
      inIdX = vtkInterpolateWrap(floorX-inExt[0],inDim[0]);
      inIdY = vtkInterpolateWrap(floorY-inExt[2],inDim[1]);
      inIdZ = vtkInterpolateWrap(floorZ-inExt[4],inDim[2]);
    }
  
  inPtr += inIdX*inInc[0]+inIdY*inInc[1]+inIdZ*inInc[2];
  for (i = 0; i < numscalars; i++)
    {
    *outPtr++ = *inPtr++;
    }
  return 1; 
} 

// Do tricubic interpolation of the input data 'inPtr' of extent 'inExt' 
// at the 'point'.  The result is placed at 'outPtr'.  
// The number of scalar components in the data is 'numscalars'

// The tricubic interpolation ensures that both the intensity and
// the first derivative of the intensity are smooth across the
// image.  The first derivative is estimated using a 
// centered-difference calculation.


// helper function: set up the lookup indices and the interpolation 
// coefficients

void vtkImageResliceSetInterpCoeffs(float F[4],int *l, int *m, float f, 
		     int interpMode)
{   
  float fp1,fm1,fm2;

  switch (interpMode)
    {
    case 7:     // cubic interpolation
      *l = 0; *m = 4; 
      fm1 = f-1;
      F[0] = -f*fm1*fm1/2;
      F[1] = ((3*f-2)*f-2)*fm1/2;
      F[2] = -((3*f-4)*f-1)*f/2;
      F[3] = f*f*fm1/2;
      break;
    case 0:     // no interpolation
    case 2:
    case 4:
    case 6:
      *l = 1; *m = 2; 
      F[1] = 1;
      break;
    case 1:     // linear interpolation
      *l = 1; *m = 3;
      F[1] = 1-f;
      F[2] = f;
      break;
    case 3:     // quadratic interpolation
      *l = 1; *m = 4; 
      fm1 = f-1; fm2 = fm1-1;
      F[1] = fm1*fm2/2;
      F[2] = -f*fm2;
      F[3] = f*fm1/2;
      break;
    case 5:     // quadratic interpolation
      *l = 0; *m = 3; 
      fp1 = f+1; fm1 = f-1; 
      F[0] = f*fm1/2;
      F[1] = -fp1*fm1;
      F[2] = fp1*f/2;
      break;
    }
}

// tricubic interpolation
template <class T>
static int vtkTricubicInterpolation(float *point, T *inPtr, T *outPtr,
				    T *background, int numscalars, 
				    int inExt[6],
				    int vtkNotUsed(inDim)[3],
				    int inInc[3])
{
  int i;
  int factX[4],factY[4],factZ[4];

  // the +1/-1 avoids round-to-zero truncation between -1 and 0,
  // and is cheaper than doing floor()
  int floorX = int(point[0]+1)-1;
  int floorY = int(point[1]+1)-1;
  int floorZ = int(point[2]+1)-1;

  float fx = point[0]-floorX;
  float fy = point[1]-floorY;
  float fz = point[2]-floorZ;

  int inIdX = floorX-inExt[0];
  int inIdY = floorY-inExt[2];
  int inIdZ = floorZ-inExt[4];

  // the doInterpX,Y,Z variables are 0 if interpolation
  // does not have to be done in the specified direction,
  // i.e. if the x, y or z lookup indices have no fractional
  // component.   
  int doInterpX = (fx != 0);
  int doInterpY = (fy != 0);
  int doInterpZ = (fz != 0);

  // check whether we can do cubic interpolation, quadratic, linear, or none
  // in each of the three directions
  if (inIdX < 0 || inIdX+doInterpX > inExt[1]-inExt[0] ||
      inIdY < 0 || inIdY+doInterpY > inExt[3]-inExt[2] ||
      inIdZ < 0 || inIdZ+doInterpZ > inExt[5]-inExt[4])
    {// out of bounds: clear to background color
    if (background)
      {
      for (i = 0; i < numscalars; i++)
	{ 
	*outPtr++ = *background++;
	}
      }
    return 0;
    }
  else 
    {// do tricubic interpolation
    float fX[4],fY[4],fZ[4];
    double vY,vZ,val;
    T *inPtr1, *inPtr2;
    int j,k,l,jl,jm,kl,km,ll,lm;
    
    for (i = 0; i < 4; i++)
      {
      factX[i] = (inIdX-1+i)*inInc[0];
      factY[i] = (inIdY-1+i)*inInc[1];
      factZ[i] = (inIdZ-1+i)*inInc[2];
      }

    // depending on whether we are at the edge of the 
    // input extent, choose the appropriate interpolation
    // method to use

    int interpModeX = ((inIdX > 0) << 2) + 
                      ((inIdX+2 <= inExt[1]-inExt[0]) << 1) +
                      doInterpX;
    int interpModeY = ((inIdY > 0) << 2) + 
                      ((inIdY+2 <= inExt[3]-inExt[2]) << 1) +
                      doInterpY;
    int interpModeZ = ((inIdZ > 0) << 2) + 
	              ((inIdZ+2 <= inExt[5]-inExt[4]) << 1) +
		      doInterpZ;

    vtkImageResliceSetInterpCoeffs(fX,&ll,&lm,fx,interpModeX);
    vtkImageResliceSetInterpCoeffs(fY,&kl,&km,fy,interpModeY);
    vtkImageResliceSetInterpCoeffs(fZ,&jl,&jm,fz,interpModeZ);

    // Finally, here is the tricubic interpolation
    // (or cubic-cubic-linear, or cubic-nearest-cubic, etc)
    for (i = 0; i < numscalars; i++)
      {
      val = 0;
      for (j = jl; j < jm; j++)
	{
	inPtr1 = inPtr + factZ[j];
	vZ = 0;
	for (k = kl; k < km; k++)
	  {
	  inPtr2 = inPtr1 + factY[k];
	  vY = 0;
	  for (l = ll; l < lm; l++)
	    {
	    vY += *(inPtr2+factX[l]) * fX[l]; 
	    }
	  vZ += vY*fY[k]; 
	  }
	val += vZ*fZ[j];
	}
      vtkResliceClamp(val,*outPtr++); // clamp to limits of type
      inPtr++;
      }
    return 1;
    }
}		  

// tricubic interpolation with wrap-around behaviour
template <class T>
static int vtkTricubicInterpolationRepeat(float *point, T *inPtr, T *outPtr,
					  T *mirror, int numscalars, 
					  int inExt[6], int inDim[3],
					  int inInc[3])
{
  int i;
  int factX[4],factY[4],factZ[4];

  int floorX = int(point[0]+1)-1;
  int floorY = int(point[1]+1)-1;
  int floorZ = int(point[2]+1)-1;

  float fx = point[0]-floorX;
  float fy = point[1]-floorY;
  float fz = point[2]-floorZ;

  // this corrects for differences between int() and floor()
  if (fx < 0)
    {
    fx = point[0] - (--floorX);
    }
  if (fy < 0)
    {
    fy = point[1] - (--floorY);
    }
  if (fz < 0)
    {
    fz = point[2] - (--floorZ);
    }

  int inIdX = floorX-inExt[0];
  int inIdY = floorY-inExt[2];
  int inIdZ = floorZ-inExt[4];

  float fX[4],fY[4],fZ[4];
  double vY,vZ,val;
  T *inPtr1, *inPtr2;
  int j,k,l;

  if (*mirror)
    {
    for (i = 0; i < 4; i++)
      {
      factX[i] = vtkInterpolateMirror(inIdX-1+i,inDim[0])*inInc[0];
      factY[i] = vtkInterpolateMirror(inIdY-1+i,inDim[1])*inInc[1];
      factZ[i] = vtkInterpolateMirror(inIdZ-1+i,inDim[2])*inInc[2];
      }
    }
  else
    {
    for (i = 0; i < 4; i++)
      {
      factX[i] = vtkInterpolateWrap(inIdX-1+i,inDim[0])*inInc[0];
      factY[i] = vtkInterpolateWrap(inIdY-1+i,inDim[1])*inInc[1];
      factZ[i] = vtkInterpolateWrap(inIdZ-1+i,inDim[2])*inInc[2];
      }
    }

  vtkImageResliceSetInterpCoeffs(fX,&i,&i,fx,7);
  vtkImageResliceSetInterpCoeffs(fY,&i,&i,fy,7);
  vtkImageResliceSetInterpCoeffs(fZ,&i,&i,fz,7);

  // Finally, here is the tricubic interpolation
  for (i = 0; i < numscalars; i++)
    {
    val = 0;
    for (j = 0; j < 4; j++)
      {
      inPtr1 = inPtr + factZ[j];
      vZ = 0;
      for (k = 0; k < 4; k++)
	{
	inPtr2 = inPtr1 + factY[k];
	vY = 0;
	for (l = 0; l < 4; l++)
	  {
	  vY += *(inPtr2+factX[l]) * fX[l]; 
	  }
	vZ += vY*fY[k]; 
	}
      val += vZ*fZ[j];
      }
    vtkResliceClamp(val,*outPtr++); // clamp to limits of type
    inPtr++;
    }
  return 1;
}		  

//----------------------------------------------------------------------------
// End of interpolation code
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// (this one function is pretty much the be-all and end-all of the
// filter)
template <class T>
static void vtkImageResliceExecute(vtkImageReslice *self,
				   vtkImageData *inData, T *inPtr,
				   vtkImageData *outData, T *outPtr,
				   int outExt[6], int id, vtkMatrix4x4 *matrix)
{
  int i, numscalars;
  int idX, idY, idZ;
  int outIncX, outIncY, outIncZ;
  int inExt[6], inWholeExt[6], inInc[3], inDim[3];
  unsigned long count = 0;
  unsigned long target;
  float inPoint[4],outPoint[4];
  T *background;
  int (*interpolate)(float *point, T *inPtr, T *outPtr,
		     T *background, int numscalars, 
		     int inExt[6], int inWholeExt[6], 
		     int inInc[3]);
  
  // find maximum input range
  inData->GetExtent(inExt);
  inData->GetWholeExtent(inWholeExt);
  
  for (i = 0; i < 3; i++)
    {
    inDim[i] = inWholeExt[2*i+1]-inWholeExt[2*i]+1;
    }

  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numscalars = inData->GetNumberOfScalarComponents();
  
  // Color for area outside of input volume extent  
  background = new T[numscalars];
  for (i = 0; i < numscalars; i++)
    {
    if (i < 4)
      {
      vtkResliceClamp(self->GetBackgroundColor()[i],background[i]);
      }
    else
      {
      background[i] = 0;
      }
    }

  // Set interpolation method
  if (self->GetWrap() || self->GetMirror())
    {
    switch (self->GetInterpolationMode())
      {
      case VTK_RESLICE_NEAREST:
	interpolate = &vtkNearestNeighborInterpolationRepeat;
	break;
      case VTK_RESLICE_LINEAR:
	interpolate = &vtkTrilinearInterpolationRepeat;
	break;
      case VTK_RESLICE_CUBIC:
	interpolate = &vtkTricubicInterpolationRepeat;
	break;
      }
    // kludge to differentiate between wrap and mirror
    background[0] = self->GetMirror();
    }
  else
    {
    switch (self->GetInterpolationMode())
      {
      case VTK_RESLICE_NEAREST:
	interpolate = &vtkNearestNeighborInterpolation;
	break;
      case VTK_RESLICE_LINEAR:
	interpolate = &vtkTrilinearInterpolation;
	break;
      case VTK_RESLICE_CUBIC:
	interpolate = &vtkTricubicInterpolation;
	break;
      }
    }    

  // Loop through ouput pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (!id) 
	{
	if (!(count%target)) 
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      
      for (idX = outExt[0]; idX <= outExt[1]; idX++)
	{
	outPoint[0] = idX;
	outPoint[1] = idY;
	outPoint[2] = idZ;
	outPoint[3] = 1;

	matrix->MultiplyPoint(outPoint,inPoint); // apply transform

	inPoint[0] /= inPoint[3]; // deal with w if the transform
	inPoint[1] /= inPoint[3]; //   was a perspective transform
	inPoint[2] /= inPoint[3];
	inPoint[3] = 1;
	
	interpolate(inPoint, inPtr, outPtr, background, 
		    numscalars, inExt, inDim, inInc);

	outPtr += numscalars; 
	}
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
  delete [] background;
}

//----------------------------------------------------------------------------
// The remainder of this file is the 'optimized' version of the code.
//----------------------------------------------------------------------------
static void ComputeInputUpdateExtentOptimized(vtkImageReslice *self,
					      int inExt[6], 
					      int outExt[6])
{
  int i,j,k;
  int idX,idY,idZ;
  double xAxis[4], yAxis[4], zAxis[4], origin[4];
  double point[4],w;

  // convert matrix from world coordinates to pixel indices
  vtkMatrix4x4 *matrix = self->GetIndexMatrix();
  for (i = 0; i < 4; i++)
    {
    xAxis[i] = matrix->GetElement(i,0);
    yAxis[i] = matrix->GetElement(i,1);
    zAxis[i] = matrix->GetElement(i,2);
    origin[i] = matrix->GetElement(i,3);
    }

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
    
    if (self->GetInterpolationMode() != VTK_RESLICE_NEAREST)
      {
      for (j = 0; j < 3; j++) 
	{
	int extra = (self->GetInterpolationMode() == VTK_RESLICE_CUBIC); 
	k = int(floor((point[j]+idX*xAxis[j])/w))-extra;
	if (k < inExt[2*j])
	  { 
	  inExt[2*j] = k;
	  } 
	k = int(ceil((point[j]+idX*xAxis[j])/w))+extra;
	if (k > inExt[2*j+1])
	  { 
	  inExt[2*j+1] = k;
	  }
	}
      }
    else
      {
      for (j = 0; j < 3; j++) 
	{
	k = int(floor((point[j]+idX*xAxis[j])/w + 0.5));
	if (k < inExt[2*j]) 
	  {
	  inExt[2*j] = k;
	  } 
	if (k > inExt[2*j+1])
	  { 
	  inExt[2*j+1] = k;
	  }
	}
      }
    }

  int *wholeExtent = self->GetInput()->GetWholeExtent();
  // Clip, just to make sure we hit _some_ of the input extent
  for (i = 0; i < 6; i++)
    {
    j = i - i%2;
    if (inExt[i] < wholeExtent[j])
      {
      inExt[i] = wholeExtent[j];
      }
    j++;
    if (inExt[i] > wholeExtent[j])
      {
      inExt[i] = wholeExtent[j];
      }
    }
}

//----------------------------------------------------------------------------
// helper functions for vtkOptimizedExecute()

// find approximate intersection of line with the plane x = x_min,
// y = y_min, or z = z_min (lower limit of data extent) 

static int intersectionLow(double *point, double *axis, int *sign,
			   int *limit, int ai, int *outExt)
{
  // approximate value of r
  int r;
  double rd = (limit[ai]*point[3]-point[ai])
    /(axis[ai]-limit[ai]*axis[3]) + 0.5;
   
  if (rd < outExt[2*ai]) 
    {
    r = outExt[2*ai];
    }
  else if (rd > outExt[2*ai+1])
    {
    r = outExt[2*ai+1];
    }
  else
    {
    r = int(rd);
    }
  
  // move back and forth to find the point just inside the extent
  while (int( (point[ai]+r*axis[ai])/double(point[3]+r*axis[3]) + 1.5 )-1 
	 < limit[ai])
    {
    r += sign[ai];
    }

  while (int( (point[ai]+(r-sign[ai])*axis[ai])
	      /double(point[3]+(r-sign[ai])*axis[3]) + 1.5 )-1 
	 >= limit[ai])
    {
    r -= sign[ai];
    }

  return r;
}

// same as above, but for x = x_max
static int intersectionHigh(double *point, double *axis, int *sign, 
			    int *limit, int ai, int *outExt)
{
  int r;
  double rd = (limit[ai]*point[3]-point[ai])
      /(axis[ai]-limit[ai]*axis[3]) + 0.5; 
    
  if (rd < outExt[2*ai])
    { 
    r = outExt[2*ai];
    }
  else if (rd > outExt[2*ai+1])
    {
    r = outExt[2*ai+1];
    }
  else
    {
    r = int(rd);
    }
  
  while (int( (point[ai]+r*axis[ai])/double(point[3]+r*axis[3]) + 1.5 )-1 
	 > limit[ai])
    {
    r -= sign[ai];
    }

  while (int( (point[ai]+(r+sign[ai])*axis[ai])
	      /double(point[3]+(r+sign[ai])*axis[3]) + 1.5 )-1 
	 <= limit[ai])
    {
    r += sign[ai];
    }

  return r;
}

static int isBounded(double *point, double *xAxis, int *inMin, 
		     int *inMax, int ai, int r)
{
  int bi = ai+1; 
  int ci = ai+2;
  if (bi > 2) 
    { 
    bi -= 3; // coordinate index must be 0, 1 or 2 
    } 
  if (ci > 2)
    { 
    ci -= 3;
    }
  double w = point[3]+r*xAxis[3];
  int bp = int((point[bi]+r*xAxis[bi])/w + 1.5)-1;
  int cp = int((point[ci]+r*xAxis[ci])/w + 1.5)-1;
  
  return (bp >= inMin[bi] && bp <= inMax[bi] &&
	  cp >= inMin[ci] && cp <= inMax[ci]);
}

// this huge mess finds out where the current output raster
// line intersects the input volume 
int vtkImageReslice::FindExtent(int& r1, int& r2, double *point, 
                                double *xAxis, 
		      int *inMin, int *inMax, int *outExt)
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

  r1 = intersectionLow(point,xAxis,sign,inMin,ix,outExt);
  r2 = intersectionHigh(point,xAxis,sign,inMax,ix,outExt);
  
  // find points of intersections
  // first, find w
  w1 = point[3]+r1*xAxis[3];
  w2 = point[3]+r2*xAxis[3];
  
  for (i = 0; i < 3; i++)
    {
    indx1[i] = int((point[i]+r1*xAxis[i])/w1+1.5)-1;
    indx2[i] = int((point[i]+r2*xAxis[i])/w2+1.5)-1;
    }
  if (isBounded(point,xAxis,inMin,inMax,ix,r1))
    { // passed through x face, check opposing face
    if (isBounded(point,xAxis,inMin,inMax,ix,r2))
      {
      return sign[ix];
      }
    
    if (indx2[iy] < inMin[iy])
      { // check y face
      r2 = intersectionLow(point,xAxis,sign,inMin,iy,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iy,r2))
	{
	return sign[ix];
	}
      }
    else if (indx2[iy] > inMax[iy])
      { // check other y face
      r2 = intersectionHigh(point,xAxis,sign,inMax,iy,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iy,r2))
	{
	return sign[ix];
	}
      }
    
    if (indx2[iz] < inMin[iz])
      { // check z face
      r2 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iz,r2))
	{
	return sign[ix];
	}
      }
    else if (indx2[iz] > inMax[iz])
      { // check other z face
      r2 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iz,r2))
	{
	return sign[ix];
	}
      }
    }
  
  if (isBounded(point,xAxis,inMin,inMax,ix,r2))
    { // passed through the opposite x face
    if (indx1[iy] < inMin[iy])
	{ // check y face
	r1 = intersectionLow(point,xAxis,sign,inMin,iy,outExt);
	if (isBounded(point,xAxis,inMin,inMax,iy,r1))
	  {
	  return sign[ix];
	  }
	}
    else if (indx1[iy] > inMax[iy])
      { // check other y face
      r1 = intersectionHigh(point,xAxis,sign,inMax,iy,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iy,r1))
	{
	return sign[ix];
	}
      }
    
    if (indx1[iz] < inMin[iz])
      { // check z face
      r1 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iz,r1))
	{
	return sign[ix];
	}
      }
    else if (indx1[iz] > inMax[iz])
      { // check other z face
      r1 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
      if (isBounded(point,xAxis,inMin,inMax,iz,r1))
	{
	return sign[ix];
	}
      }
    }
  
  if ((indx1[iy] >= inMin[iy] && indx2[iy] < inMin[iy]) ||
      (indx1[iy] < inMin[iy] && indx2[iy] >= inMin[iy]))
    { // line might pass through bottom face
    r1 = intersectionLow(point,xAxis,sign,inMin,iy,outExt);
    if (isBounded(point,xAxis,inMin,inMax,iy,r1))
      {
      if ((indx1[iy] <= inMax[iy] && indx2[iy] > inMax[iy]) ||
	  (indx1[iy] > inMax[iy] && indx2[iy] <= inMax[iy]))
	{ // line might pass through top face
	r2 = intersectionHigh(point,xAxis,sign,inMax,iy,outExt);
	if (isBounded(point,xAxis,inMin,inMax,iy,r2))
	  {
	  return sign[iy];
	  }
	}
      
      if (indx1[iz] < inMin[iz] && indx2[iy] < inMin[iy] ||
	  indx2[iz] < inMin[iz] && indx1[iy] < inMin[iy])
	{ // line might pass through in-to-screen face
	r2 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
	if (isBounded(point,xAxis,inMin,inMax,iz,r2))
	  {
	  return sign[iy];
	  }
	}
      else if (indx1[iz] > inMax[iz] && indx2[iy] < inMin[iy] ||
	       indx2[iz] > inMax[iz] && indx1[iy] < inMin[iy])
	{ // line might pass through out-of-screen face
	r2 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
	if (isBounded(point,xAxis,inMin,inMax,iz,r2))
	  {
	  return sign[iy];
	  }
	} 
      }
    }
  
  if ((indx1[iy] <= inMax[iy] && indx2[iy] > inMax[iy]) ||
      (indx1[iy] > inMax[iy] && indx2[iy] <= inMax[iy]))
    { // line might pass through top face
    r2 = intersectionHigh(point,xAxis,sign,inMax,iy,outExt);
    if (isBounded(point,xAxis,inMin,inMax,iy,r2))
      {
      if (indx1[iz] < inMin[iz] && indx2[iy] > inMax[iy] ||
	  indx2[iz] < inMin[iz] && indx1[iy] > inMax[iy])
	{ // line might pass through in-to-screen face
	r1 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
	if (isBounded(point,xAxis,inMin,inMax,iz,r1))
	  {
	  return sign[iy];
	  }
	}
      else if (indx1[iz] > inMax[iz] && indx2[iy] > inMax[iy] || 
	       indx2[iz] > inMax[iz] && indx1[iy] > inMax[iy])
	{ // line might pass through out-of-screen face
	r1 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
	if (isBounded(point,xAxis,inMin,inMax,iz,r1))
	  {
	  return sign[iy];
	  }
	}
      } 
    }
  
  if ((indx1[iz] >= inMin[iz] && indx2[iz] < inMin[iz]) ||
      (indx1[iz] < inMin[iz] && indx2[iz] >= inMin[iz]))
    { // line might pass through in-to-screen face
    r1 = intersectionLow(point,xAxis,sign,inMin,iz,outExt);
    if (isBounded(point,xAxis,inMin,inMax,iz,r1))
      {
      if (indx1[iz] > inMax[iz] || indx2[iz] > inMax[iz])
	{ // line might pass through out-of-screen face
	r2 = intersectionHigh(point,xAxis,sign,inMax,iz,outExt);
	if (isBounded(point,xAxis,inMin,inMax,iz,r2))
	  {
	  return sign[iz];
	  }
	}
      }
    }
  
  r1 = r2 = -1;
  return 1;
}

// The vtkOptimizedExecute() function uses an optimization which
// is conceptually simple, but complicated to implement.

// In the un-optimized version, each output voxel
// is converted into a set of look-up indices for the input data;
// then, the indices are checked to ensure they lie within the
// input data extent.

// In the optimized version below, the check is done in reverse:
// it is first determined which output voxels map to look-up indices
// within the input data extent.  Then, further calculations are
// done only for those voxels.  This means that 1) minimal work
// is done for voxels which map to regions outside fo the input
// extent (they are just set to the background color) and 2)
// the inner loops of the look-up and interpolation are
// tightened relative to the un-uptimized version. 

template <class T>
static void vtkOptimizedExecute(vtkImageReslice *self,
				vtkImageData *inData, T *inPtr,
				vtkImageData *outData, T *outPtr,
				int outExt[6], int id, vtkMatrix4x4 *matrix)
{
  int i, numscalars;
  int idX, idY, idZ;
  int outIncX, outIncY, outIncZ;
  int inIdX, inIdY, inIdZ;
  int inExt[6],inWholeExt[6];
  int inMax[3], inMin[3], inDim[3];
  int inInc[3];
  unsigned long count = 0;
  unsigned long target;
  int r1,r2;
  double inPoint0[4];
  double inPoint1[4];
  float inPoint[4];
  double xAxis[4], yAxis[4], zAxis[4], origin[4];
  double w;
  T *background;
  int (*interpolate)(float *point, T *inPtr, T *outPtr,
		     T *background, int numscalars, 
		     int inExt[6], int inDim[3], 
		     int inInc[3]);

  // find maximum input range
  self->GetInput()->GetWholeExtent(inWholeExt);
  self->GetInput()->GetExtent(inExt);

  for (i = 0; i < 3; i++)
    {
    inMin[i] = inWholeExt[2*i];
    inMax[i] = inWholeExt[2*i+1];
    inDim[i] = inMax[i]-inMin[i]+1;
    }
  
  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numscalars = inData->GetNumberOfScalarComponents();
  
  // break matrix into a set of axes plus an origin
  // (this allows us to calculate the transform Incrementally)
  for (i = 0; i < 4; i++)
    {
    xAxis[i] = matrix->GetElement(i,0);
    yAxis[i] = matrix->GetElement(i,1);
    zAxis[i] = matrix->GetElement(i,2);
    origin[i] = matrix->GetElement(i,3);
    }

  // set up background levels
  background = new T[numscalars];
  for (i = 0; i < numscalars; i++)
    {
    if (i < 4)
      {
      background[i] = T(self->GetBackgroundColor()[i]);
      }
    else
      {
      background[i] = 0;
      }
    }

  // Set interpolation method
  if (self->GetWrap() || self->GetMirror())
    {
    switch (self->GetInterpolationMode())
      {
      case VTK_RESLICE_NEAREST:
	interpolate = &vtkNearestNeighborInterpolationRepeat;
	break;
      case VTK_RESLICE_LINEAR:
	interpolate = &vtkTrilinearInterpolationRepeat;
	break;
      case VTK_RESLICE_CUBIC:
	interpolate = &vtkTricubicInterpolationRepeat;
	break;
      }
    // kludge to differentiate between wrap and mirror
    background[0] = self->GetMirror();
    }
  else
    {
    switch (self->GetInterpolationMode())
      {
      case VTK_RESLICE_NEAREST:
	interpolate = &vtkNearestNeighborInterpolation;
	break;
      case VTK_RESLICE_LINEAR:
	interpolate = &vtkTrilinearInterpolation;
	break;
      case VTK_RESLICE_CUBIC:
	interpolate = &vtkTricubicInterpolation;
	break;
      }
    }    

  // Loop through output pixels
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
      
      if (self->GetWrap() || self->GetMirror())
	{ // wrap-pad like behaviour
	for (idX = outExt[0]; idX <= outExt[1]; idX++)
	  {
	  w = inPoint1[3]+idX*xAxis[3];
	  inPoint[0] = (inPoint1[0]+idX*xAxis[0])/w;
	  inPoint[1] = (inPoint1[1]+idX*xAxis[1])/w;
	  inPoint[2] = (inPoint1[2]+idX*xAxis[2])/w;
	  inPoint[3] = 1;

	  interpolate(inPoint, inPtr, outPtr, background, 
		      numscalars, inExt, inDim, inInc);
	  outPtr += numscalars;
	  }
	}
      else
	{
	// find intersections of x raster line with the input extent
	if (self->FindExtent(r1,r2,inPoint1,xAxis,inMin,inMax,outExt) < 0)
	  {
	  i = r1;
	  r1 = r2;
	  r2 = i;
	  }

	// bound r1,r2 within reasonable limits
	if (r1 < outExt[0]) 
	  {
	  r1 = outExt[0];
	  }
	if (r2 > outExt[1]) 
	  {
	  r2 = outExt[1];
	  }
	if (r1 > r2) 
	  {
	  r1 = outExt[0];
	  r2 = outExt[0]-1;
	  }

	// clear pixels to left of input extent
	if (numscalars == 1) // optimize for single scalar
	  {
	  for (idX = outExt[0]; idX < r1; idX++) 
	    {
	    *outPtr++ = background[0];
	    }
	  }
	else             // multiple scalars
	  {
	  for (idX = outExt[0]; idX < r1; idX++)
	    {
	    for (i = 0; i < numscalars; i++)
	      {
	      *outPtr++ = background[i];
	      }
	    }
	  }
	
	if (self->GetInterpolationMode() != VTK_RESLICE_NEAREST)
	  { // Trilinear or tricubic
	  for (idX = r1; idX <= r2; idX++)
	    {
	    w = inPoint1[3]+idX*xAxis[3];
	    inPoint[0] = (inPoint1[0]+idX*xAxis[0])/w;
	    inPoint[1] = (inPoint1[1]+idX*xAxis[1])/w;
	    inPoint[2] = (inPoint1[2]+idX*xAxis[2])/w;
	    inPoint[3] = 1;
	    
	    interpolate(inPoint, inPtr, outPtr, background, 
			numscalars, inExt, inDim, inInc);
	    outPtr += numscalars;
	    }
	  }
	else
	  {  // Nearest-Neighbor, no extent checks
	  T *inPtr1;

	  for (idX = r1; idX <= r2; idX++)
	    {
	    w = inPoint1[3]+idX*xAxis[3]; // don't forget w!  
	    // the +1.5/-1 thing avoids int() vs. floor() difference 
	    inIdX = int((inPoint1[0]+idX*xAxis[0])/w+1.5)-inExt[0]-1;
	    inIdY = int((inPoint1[1]+idX*xAxis[1])/w+1.5)-inExt[2]-1;
	    inIdZ = int((inPoint1[2]+idX*xAxis[2])/w+1.5)-inExt[4]-1;
	    
	    inPtr1 = inPtr+inIdX*inInc[0]+inIdY*inInc[1]
	      +inIdZ*inInc[2];

	    for (i = 0; i < numscalars; i++)
	      {
	      *outPtr++ = *inPtr1++;
	      }
	    }
	  }
	
  
	// clear pixels to right of input extent
	if (numscalars == 1) // optimize for single scalar
	  {
	  for (idX = r2+1; idX <= outExt[1]; idX++)
	    { 
	    *outPtr++ = background[0];
	    }
	  }
	else // multiple scalars
	  {
	  for (idX = r2+1; idX <= outExt[1]; idX++)
	    {
	    for (i = 0; i < numscalars; i++)
	      {
	      *outPtr++ = background[i];
	      }
	    }
	  }
	}

      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
  delete [] background;
}

// vtkOptimizedPermuteExecute is specifically optimized for
// cases where the IndexMatrix has only one non-zero component
// per row, i.e. when the matrix is permutation+scale+translation.
// All of the interpolation coefficients are calculated ahead
// of time instead of on a pixel-by-pixel basis.

template <class T>
static void vtkOptimizedPermuteExecuteLinear(vtkImageReslice *self,
					     vtkImageData *inData, T *inPtr,
					     vtkImageData *outData, T *outPtr,
					     int outExt[6], int id,
					     vtkMatrix4x4 *matrix)
{
  int i, j, k, numscalars;
  int idX, idY, idZ;
  int outIncX, outIncY, outIncZ;
  int inExt[6];
  int inInc[3];
  int clipExt[6];
  unsigned long count = 0;
  unsigned long target;
  int r1,r2;
  T *background;

  // find maximum input range
  self->GetInput()->GetExtent(inExt);

  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numscalars = inData->GetNumberOfScalarComponents();

  // set up background levels
  background = new T[numscalars];
  for (i = 0; i < numscalars; i++)
    {
    if (i < 4)
      {
      background[i] = T(self->GetBackgroundColor()[i]);
      }
    else
      {
      background[i] = 0;
      }
    }

  for (i = 0; i < 6; i++)
    {
    clipExt[i] = outExt[i];
    }

  int *traversal[3];
  float *constants[3];
  double newmat[4][4];
  int region;

  for (j = 0; j < 4; j++)
    {
    for (i = 0; i < 4; i++)
      {
      newmat[i][j] = matrix->GetElement(i,j);
      }
    } 

  int trunc, inId, doInterp;
  float point,f;
  
  // set up input traversal table for linear interpolation  
  for (j = 0; j < 3; j++)
    {
    traversal[j] = new int[(outExt[2*j+1]+1)*2];
    constants[j] = new float[(outExt[2*j+1]+1)*2];
    region = 0;
    for (i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      for (k = 0; k < 3; k++)
	{ // find which element is nonzero
	if (newmat[k][j] != 0)
	  {
	  break;
	  }
	}
      point = newmat[k][3]+i*newmat[k][j];
      trunc = int(point+1)-1;
      f = point-trunc;
      constants[j][2*i] = 1-f;
      constants[j][2*i+1] = f;
      
      doInterp = (f != 0);
      inId = trunc - inExt[2*k];
      
      traversal[j][2*i] = inId*inInc[k];
      traversal[j][2*i+1] = (inId+doInterp)*inInc[k];
      
      if (inId < 0 || inId+doInterp > inExt[2*k+1]-inExt[2*k])
	{
	if (region == 1)
	  { // leaving the input extent
	  region = 2;
	  clipExt[2*j+1] = i-1;
	  }
	}
      else 
	{
	if (region == 0)
	  { // entering the input extent
	  region = 1;
	  clipExt[2*j] = i;	   
	  }
	}
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = outExt[2*j+1]+1;
      }
    }

  // Loop through output pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    int idZ0 = 2*idZ;
    int idZ1 = 2*idZ+1;

    int i0 = traversal[2][idZ0]; 
    int i1 = traversal[2][idZ1];

    float rz = constants[2][idZ0];
    float fz = constants[2][idZ1];
    
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      int idY0 = 2*idY;
      int idY1 = 2*idY+1;

      int i00 = traversal[1][idY0] + i0;
      int i01 = traversal[1][idY0] + i1;
      int i10 = traversal[1][idY1] + i0;
      int i11 = traversal[1][idY1] + i1;

      float ry = constants[1][idY0];
      float fy = constants[1][idY1];

      float ryrz = ry*rz;
      float ryfz = ry*fz;
      float fyrz = fy*rz;
      float fyfz = fy*fz;

      if (!id) 
	{
	if (!(count%target)) 
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}

      // do extent check
      if (idZ < clipExt[4] || idZ > clipExt[5] ||
	  idY < clipExt[2] || idY > clipExt[3])
	{
	r1 = outExt[1]+1;
	r2 = outExt[1];
	}
      else
	{
	r1 = clipExt[0];
	r2 = clipExt[1];
	}
  
      // clear pixels to left of input extent
      for (idX = outExt[0]; idX < r1; idX++)
	{
	for (i = 0; i < numscalars; i++)
	  {
	  *outPtr++ = background[i];
	  }
	}

      for (idX = r1; idX <= r2; idX++)
	{
        int idX0 = 2*idX;
	int idX1 = 2*idX+1;
	
	int t0 = traversal[0][idX0];
	int t1 = traversal[0][idX1];
	
	int i000 = t0 + i00; 
	int i001 = t0 + i01; 
	int i010 = t0 + i10; 
	int i011 = t0 + i11; 
	int i100 = t1 + i00; 
	int i101 = t1 + i01; 
	int i110 = t1 + i10; 
	int i111 = t1 + i11; 
	
	float rx = constants[0][idX0];
	float fx = constants[0][idX1];
	
	T *inPtr0 = inPtr;

	for (i = 0; i < numscalars; i++)
	  {
	  vtkResliceRound((rx*(ryrz*inPtr0[i000]+ryfz*inPtr0[i001]+
			       fyrz*inPtr0[i010]+fyfz*inPtr0[i011])
			   + fx*(ryrz*inPtr0[i100]+ryfz*inPtr0[i101]+
				 fyrz*inPtr0[i110]+fyfz*inPtr0[i111])),
			  *outPtr++);
	  inPtr0++;
	  }
	}

      // clear pixels to right of input extent
      for (idX = r2+1; idX <= outExt[1]; idX++)
        {
	for (i = 0; i < numscalars; i++)
	  {
	  *outPtr++ = background[i];
	  }
	}

      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }

  for (j = 0; j < 3; j++)
    {
    delete [] traversal[j];
    delete [] constants[j];
    }
  delete [] background;
}

template <class T>
static void vtkOptimizedPermuteExecuteCubic(vtkImageReslice *self,
                                            vtkImageData *inData, T *inPtr,
                                            vtkImageData *outData, T *outPtr,
					    int outExt[6], int id,
					    vtkMatrix4x4 *matrix)
{
  int i, j, k, l, numscalars;
  int idX, idY, idZ;
  int outIncX, outIncY, outIncZ;
  int inExt[6];
  int inInc[3];
  int clipExt[6];
  unsigned long count = 0;
  unsigned long target;
  int r1,r2;
  T *background;

  // find maximum input range
  self->GetInput()->GetExtent(inExt);

  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numscalars = inData->GetNumberOfScalarComponents();

  // set up background levels
  background = new T[numscalars];
  for (i = 0; i < numscalars; i++)
    {
    if (i < 4)
      {
      background[i] = T(self->GetBackgroundColor()[i]);
      }
    else
      {
      background[i] = 0;
      }
    }

  for (i = 0; i < 6; i++)
    {
    clipExt[i] = outExt[i];
    }

  int *traversal[3];
  int *low[3];
  int *high[3];
  float *constants[3];
  double newmat[4][4];
  int region;

  for (j = 0; j < 4; j++)
    {
    for (i = 0; i < 4; i++)
      {
      newmat[i][j] = matrix->GetElement(i,j);
      }
    } 

  int trunc, inId, doInterp, interpMode;
  float point,f;
  
  // set up input traversal table for cubic interpolation  
  for (j = 0; j < 3; j++)
    {
    traversal[j] = new int[(outExt[2*j+1]+1)*4];
    constants[j] = new float[(outExt[2*j+1]+1)*4];
    low[j] = new int[(outExt[2*j+1]+1)];
    high[j] = new int[(outExt[2*j+1]+1)];

    region = 0;
    for (i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      for (k = 0; k < 3; k++)
	{ // find which element is nonzero
	if (newmat[k][j] != 0)
	  {
	  break;
	  }
	}
      point = newmat[k][3]+i*newmat[k][j];
      trunc = int(point+1)-1;
      f = point-trunc;      
      doInterp = (f != 0);
      inId = trunc - inExt[2*k];
      
      interpMode = ((inId > 0) << 2) + 
	           ((inId+2 <= inExt[2*k+1]-inExt[2*k]) << 1) +
	           doInterp;

      vtkImageResliceSetInterpCoeffs(&constants[j][4*i],&low[j][i],
				     &high[j][i],f,interpMode);

      traversal[j][4*i] = (inId-1)*inInc[k];
      traversal[j][4*i+1] = (inId+0)*inInc[k];
      traversal[j][4*i+2] = (inId+1)*inInc[k];
      traversal[j][4*i+3] = (inId+2)*inInc[k];

      if (inId < 0 || inId+doInterp > inExt[2*k+1]-inExt[2*k])
	{
	if (region == 1)
	  { // leaving the input extent
	  region = 2;
	  clipExt[2*j+1] = i-1;
	  }
	}
      else 
	{
	if (region == 0)
	  { // entering the input extent
	  region = 1;
	  clipExt[2*j] = i;	   
	  }
	}
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = outExt[2*j+1]+1;
      }
    }

  // Loop through output pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    int lz = low[2][idZ];
    int hz = high[2][idZ];
    float fZ[4];
    int iZ[4];

    for (i = lz; i < hz; i++)
      {
      fZ[i] = constants[2][4*idZ+i];
      iZ[i] = traversal[2][4*idZ+i];
      }

    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      int ly = low[1][idY];
      int hy = high[1][idY];
      float fY[4];
      int iY[4];
      
      for (i = ly; i < hy; i++)
	{
        fY[i] = constants[1][4*idY+i];
	iY[i] = traversal[1][4*idY+i];
	}

      if (!id) 
	{
	if (!(count%target)) 
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}

      // do extent check
      if (idZ < clipExt[4] || idZ > clipExt[5] ||
	  idY < clipExt[2] || idY > clipExt[3])
	{
	r1 = outExt[1]+1;
	r2 = outExt[1];
	}
      else
	{
	r1 = clipExt[0];
	r2 = clipExt[1];
	}
  
      // clear pixels to left of input extent
      for (idX = outExt[0]; idX < r1; idX++)
	{
	for (i = 0; i < numscalars; i++)
	  {
	  *outPtr++ = background[i];
	  }
	}

      for (idX = r1; idX <= r2; idX++)
	{
	int lx = low[0][idX];
	int hx = high[0][idX];
	float fX[4];
	int iX[4];

	for (i = lx; i < hx; i++)
	  {
	  fX[i] = constants[0][4*idX+i];
	  iX[i] = traversal[0][4*idX+i];
	  }
	
	T *inPtr0 = inPtr;
	T *inPtr1,*inPtr2;
	double val,vY,vZ;

	for (l = 0; l < numscalars; l++)
	  {
	  val = 0;
	  for (k = lz; k < hz; k++)
	    {
	    inPtr1 = inPtr0 + iZ[k];
	    vZ = 0;
	    for (j = ly; j < hy; j++)
	      {
	      inPtr2 = inPtr1 + iY[j];
	      vY = 0;
	      for (i = lx; i < hx; i++)
		{
		vY += *(inPtr2+iX[i]) * fX[i]; 
		}
	      vZ += vY*fY[j]; 
	      }
	    val += vZ*fZ[k];
	    }
	  vtkResliceClamp(val,*outPtr++); // clamp to limits of type
	  inPtr0++;
	  }
	}
      
      // clear pixels to right of input extent
      for (idX = r2+1; idX <= outExt[1]; idX++)
        {
	for (i = 0; i < numscalars; i++)
	  {
	  *outPtr++ = background[i];
	  }
	}

      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }

  for (j = 0; j < 3; j++)
    {
    delete [] traversal[j];
    delete [] constants[j];
    }
  delete [] background;
}

template <class T>
static void vtkOptimizedPermuteExecute(vtkImageReslice *self,
				       vtkImageData *inData, T *inPtr,
				       vtkImageData *outData, T *outPtr,
				       int outExt[6], int id,
				       vtkMatrix4x4 *matrix)
{
  if (self->GetInterpolationMode() == VTK_RESLICE_LINEAR)
    {
    vtkOptimizedPermuteExecuteLinear(self,inData,inPtr,outData,outPtr,
				     outExt,id,matrix);
    return;
    }
  else if (self->GetInterpolationMode() == VTK_RESLICE_CUBIC)
    {
    vtkOptimizedPermuteExecuteCubic(self,inData,inPtr,outData,outPtr,
				    outExt,id,matrix);
    return;
    }

  int i, j, k, numscalars;
  int idX, idY, idZ;
  int outIncX, outIncY, outIncZ;
  int inExt[6];
  int inInc[3];
  int clipExt[6];
  unsigned long count = 0;
  unsigned long target;
  int r1,r2;
  T *background,*inPtr0,*inPtr1,*inPtr2;

  // find maximum input range
  self->GetInput()->GetExtent(inExt);

  target = (unsigned long)
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  numscalars = inData->GetNumberOfScalarComponents();

  // set up background levels
  background = new T[numscalars];
  for (i = 0; i < numscalars; i++)
    {
    if (i < 4)
      {
      background[i] = T(self->GetBackgroundColor()[i]);
      }
    else
      {
      background[i] = 0;
      }
    }

  for (i = 0; i < 6; i++)
    {
    clipExt[i] = outExt[i];
    }

  int *traversal[3];
  double newmat[4][4];
  int region;

  for (j = 0; j < 4; j++)
    {
    for (i = 0; i < 4; i++)
      {
      newmat[i][j] = matrix->GetElement(i,j);
      }
    } 

  int inId;

  // set up input traversal table for nearest-neighbor interpolation  
  for (j = 0; j < 3; j++)
    {
    traversal[j] = new int[outExt[2*j+1]+1];
    region = 0;
    for (i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      for (k = 0; k < 3; k++)
	{ // find which element is nonzero
	if (newmat[k][j] != 0)
	  {
	  break;
	  }
	}
      inId = int((newmat[k][3]+i*newmat[k][j])+1.5)-inExt[2*k]-1;
      traversal[j][i] = inId*inInc[k];
      if (inId < 0 || inId > inExt[2*k+1]-inExt[2*k])
	{
	if (region == 1)
	  { // leaving the input extent
          region = 2;
	  clipExt[2*j+1] = i-1;
	  }
	}
      else 
	{
	if (region == 0)
	  { // entering the input extent
	  region = 1;
	  clipExt[2*j] = i;	   
	  }
	}
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = outExt[2*j+1]+1;
      }
    }
  
  // Loop through output pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    inPtr0 = inPtr + traversal[2][idZ]; 
    
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      inPtr1 = inPtr0 + traversal[1][idY];

      if (!id) 
	{
	if (!(count%target)) 
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}

      // do extent check
      if (idZ < clipExt[4] || idZ > clipExt[5] ||
	  idY < clipExt[2] || idY > clipExt[3])
	{
	r1 = outExt[1]+1;
	r2 = outExt[1];
	}
      else
	{
	r1 = clipExt[0];
	r2 = clipExt[1];
	}
  
      // clear pixels to left of input extent
      for (idX = outExt[0]; idX < r1; idX++)
	{
	for (i = 0; i < numscalars; i++)
	  {
	  *outPtr++ = background[i];
	  }
	}

      for (idX = r1; idX <= r2; idX++)
	{
	inPtr2 = inPtr1 + traversal[0][idX];
	
	for (i = 0; i < numscalars; i++)
	  {
	  *outPtr++ = *inPtr2++;
	  }
	}

      // clear pixels to right of input extent
      for (idX = r2+1; idX <= outExt[1]; idX++)
        {
	for (i = 0; i < numscalars; i++)
	  {
	  *outPtr++ = background[i];
	  }
	}

      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }

  for (j = 0; j < 3; j++)
    {
    delete [] traversal[j];
    }
  delete [] background;
}

// check a matrix to ensure that it is a permutation+scale+translation
// matrix

static int vtkIsPermutationMatrix(vtkMatrix4x4 *matrix)
{
  int i,j,k;

  for (i = 0; i < 3; i++)
    {
    if (matrix->GetElement(3,i) != 0.0)
      {
      return 0;
      }
    }
  if (matrix->GetElement(3,3) != 1.0)
    {
    return 0;
    }
  for (j = 0; j < 3; j++)
    {
    k = 0;
    for (i = 0; i < 3; i++)
      {
      if (matrix->GetElement(i,j) != 0.0)
	{
	k++;
	}
      }
    if (k != 1)
      {
      return 0;
      }
    }
  return 1;
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

  // change transform matrix so that instead of taking 
  // input coords -> output coords it takes output indices -> input indices
  vtkMatrix4x4 *matrix = this->GetIndexMatrix();
  
  if (this->Optimization == 2 && vtkIsPermutationMatrix(matrix) &&
      !this->Wrap && !this->Mirror)
    {
    switch (inData->GetScalarType())
      {
      case VTK_FLOAT:
	vtkOptimizedPermuteExecute(this, inData, (float *)(inPtr), 
			    outData, (float *)(outPtr),outExt, id,
			    matrix);
	break;
      case VTK_INT:
	vtkOptimizedPermuteExecute(this, inData, (int *)(inPtr), 
			    outData, (int *)(outPtr),outExt, id,
			    matrix);
	break;
      case VTK_SHORT:
	vtkOptimizedPermuteExecute(this, inData, (short *)(inPtr), 
			    outData, (short *)(outPtr),outExt, id,
			    matrix);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkOptimizedPermuteExecute(this, inData, (unsigned short *)(inPtr), 
			    outData, (unsigned short *)(outPtr),outExt,id,
			    matrix);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkOptimizedPermuteExecute(this, inData, (unsigned char *)(inPtr), 
			    outData, (unsigned char *)(outPtr),outExt, id,
			    matrix);
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown input ScalarType");
	return;
      }
    }
  else if (this->Optimization)
    {
    switch (inData->GetScalarType())
      {
      case VTK_FLOAT:
	vtkOptimizedExecute(this, inData, (float *)(inPtr), 
			    outData, (float *)(outPtr),outExt, id,
			    matrix);
	break;
      case VTK_INT:
	vtkOptimizedExecute(this, inData, (int *)(inPtr), 
			    outData, (int *)(outPtr),outExt, id,
			    matrix);
	break;
      case VTK_SHORT:
	vtkOptimizedExecute(this, inData, (short *)(inPtr), 
			    outData, (short *)(outPtr),outExt, id,
			    matrix);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkOptimizedExecute(this, inData, (unsigned short *)(inPtr), 
			    outData, (unsigned short *)(outPtr),outExt,id,
			    matrix);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkOptimizedExecute(this, inData, (unsigned char *)(inPtr), 
			    outData, (unsigned char *)(outPtr),outExt, id,
			    matrix);
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown input ScalarType");
	return;
      }
    }
  else
    {
    switch (inData->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageResliceExecute(this, inData, (float *)(inPtr), 
			       outData, (float *)(outPtr),outExt, id,
			       matrix);
	break;
      case VTK_INT:
	vtkImageResliceExecute(this, inData, (int *)(inPtr), 
			       outData, (int *)(outPtr),outExt, id,
			       matrix);
	break;
      case VTK_SHORT:
	vtkImageResliceExecute(this, inData, (short *)(inPtr), 
			       outData, (short *)(outPtr),outExt, id,
			       matrix);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageResliceExecute(this, inData, (unsigned short *)(inPtr), 
			       outData, (unsigned short *)(outPtr),outExt,id,
			       matrix);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageResliceExecute(this, inData, (unsigned char *)(inPtr), 
			       outData, (unsigned char *)(outPtr),outExt, id,
			       matrix);
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown input ScalarType");
	return;
      }
    }
}


