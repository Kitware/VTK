/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include <math.h>
#include "vtkImageGaussianSmooth.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageGaussianSmooth* vtkImageGaussianSmooth::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageGaussianSmooth");
  if(ret)
    {
    return (vtkImageGaussianSmooth*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageGaussianSmooth;
}




//----------------------------------------------------------------------------
vtkImageGaussianSmooth::vtkImageGaussianSmooth()
{
  this->Dimensionality = 3; // note: this overrides Standard deviation.
  this->StandardDeviations[0] = 2.0;
  this->StandardDeviations[1] = 2.0;
  this->StandardDeviations[2] = 2.0;
  this->RadiusFactors[0] = 1.5;
  this->RadiusFactors[1] = 1.5;
  this->RadiusFactors[2] = 1.5;
}

//----------------------------------------------------------------------------
vtkImageGaussianSmooth::~vtkImageGaussianSmooth()
{
}

//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::PrintSelf(ostream& os, vtkIndent indent)
{
  // int idx;
  
  this->vtkImageToImageFilter::PrintSelf(os, indent);

  //os << indent << "BoundaryRescale: " << this->BoundaryRescale << "\n";

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";

  os << indent << "RadiusFactors: ( "
     << this->RadiusFactors[0] << ", "
     << this->RadiusFactors[1] << ", "
     << this->RadiusFactors[2] << " )\n";

  os << indent << "StandardDeviations: ( "
     << this->StandardDeviations[0] << ", "
     << this->StandardDeviations[1] << ", "
     << this->StandardDeviations[2] << " )\n";
}

//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::ComputeKernel(double *kernel, int min, int max,
					   double std)
{
  int x;
  double sum;
  
  // handle special case
  if (std == 0.0)
    {
    kernel[0] = 1.0;
    return;
    }
  
  // fill in kernel
  sum = 0.0;
  for (x = min; x <= max; ++x)
    {
    sum += kernel[x-min] = 
      exp(- ((double)(x*x)) / (std * std * 2.0));
    }

  // normalize
  for (x = min; x <= max; ++x)
    {
    kernel[x-min] /= sum;
    }
}
  

//----------------------------------------------------------------------------
void vtkImageGaussianSmooth::ComputeInputUpdateExtent(int inExt[6], 
						      int outExt[6])
{
  int *wholeExtent;
  int idx, radius;

  // copy
  memcpy((void *)inExt, (void *)outExt, 6 * sizeof(int));
  // Expand filtered axes
  wholeExtent = this->GetInput()->GetWholeExtent();
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    radius = (int)(this->StandardDeviations[idx] * this->RadiusFactors[idx]);
    inExt[idx*2] -= radius;
    if (inExt[idx*2] < wholeExtent[idx*2])
      {
      inExt[idx*2] = wholeExtent[idx*2];
      }

    inExt[idx*2+1] += radius;
    if (inExt[idx*2+1] > wholeExtent[idx*2+1])
      {
      inExt[idx*2+1] = wholeExtent[idx*2+1];
      }
    }
}


//----------------------------------------------------------------------------
// For a given position along the convolution axis, this method loops over 
// all other axes, and performs the convolution. Boundary conditions handled
// previously.
template <class T>
static void 
vtkImageGaussianSmoothExecute(vtkImageGaussianSmooth *self, int axis,
		      double *kernel, int kernelSize,
		      vtkImageData *inData, T *inPtrC,
		      vtkImageData *outData, int outExt[6], T *outPtrC,
		      int *pcycle, int target, int *pcount, int total)
{
  int maxC, max0, max1;
  int idxC, idx0, idx1, idxK;
  int *inIncs, *outIncs;
  int inInc0, inInc1, inIncK, outInc0, outInc1;
  T *outPtr1, *outPtr0;
  T *inPtr1, *inPtr0, *inPtrK;
  double *ptrK, sum;
  
  // avoid warnings
  self = self;
  max0 = max1 = inInc0 = inInc1 = outInc0 = outInc1 = 0;
  
  // I am counting on the fact that tight loops (component on outside)
  // is more important than cache misses from shuffled access.

  // Do the correct shuffling of the axes (increments, extents)
  inIncs = inData->GetIncrements();
  outIncs = outData->GetIncrements();
  inIncK = inIncs[axis];
  maxC = outData->GetNumberOfScalarComponents();
  switch (axis)
    {
    case 0:
      inInc0 = inIncs[1];  inInc1 = inIncs[2];
      outInc0 = outIncs[1];  outInc1 = outIncs[2];
      max0 = outExt[3] - outExt[2] + 1;   max1 = outExt[5] - outExt[4] + 1;
      break;
    case 1:
      inInc0 = inIncs[0];  inInc1 = inIncs[2];
      outInc0 = outIncs[0];  outInc1 = outIncs[2];
      max0 = outExt[1] - outExt[0] + 1;   max1 = outExt[5] - outExt[4] + 1;
      break;
    case 2:
      inInc0 = inIncs[0];  inInc1 = inIncs[1];
      outInc0 = outIncs[0];  outInc1 = outIncs[1];
      max0 = outExt[1] - outExt[0] + 1;   max1 = outExt[3] - outExt[2] + 1;
      break;
    }
  
  for (idxC = 0; idxC < maxC; ++idxC)
    {
    inPtr1 = inPtrC;
    outPtr1 = outPtrC;    
    for (idx1 = 0; idx1 < max1; ++idx1)
      {
      inPtr0 = inPtr1;
      outPtr0 = outPtr1;    
      for (idx0 = 0; idx0 < max0; ++idx0)
	{
	inPtrK = inPtr0;
	ptrK = kernel;
	sum = 0.0;
	// too bad this short loop has to be the inner most loop
	for (idxK = 0; idxK < kernelSize; ++idxK)
	  {
	  sum += *ptrK * (double)(*inPtrK);
	  ++ptrK;
	  inPtrK += inIncK;
	  }
	*outPtr0 = (T)(sum);
	inPtr0 += inInc0;
	outPtr0 += outInc0;
	}
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      // we finished a row ... do we update ???
      if (total)
	{ // yes this is the main thread
	*pcycle += max0;
	if (*pcycle > target)
	  { // yes
	  *pcycle -= target;
	  *pcount += target;
	  self->UpdateProgress((float)(*pcount) / (float)total);
	  //fprintf(stderr, "count: %d, total: %d, progress: %f\n",
	  //*pcount, total, (float)(*pcount) / (float)total);
	  }
	}
      }
    
    ++inPtrC;
    ++outPtrC;
    }
}


//----------------------------------------------------------------------------
// This method convolves over one axis. It loops over the convolved axis,
// and handles boundary conditions.
void vtkImageGaussianSmooth::ExecuteAxis(int axis, 
					 vtkImageData *inData, int inExt[6],
					 vtkImageData *outData, int outExt[6],
					 int *pcycle, int target, 
					 int *pcount, int total)
{
  int idxA, max;
  int *wholeExtent, wholeMax, wholeMin;
  double *kernel;
  // previousClip and currentClip rembers that the previous was not clipped
  // keeps from recomputing kernels for center pixels.
  int kernelSize = 0;
  int kernelLeftClip, kernelRightClip;
  int previousClipped, currentClipped;
  int radius, size;
  void *inPtr;
  void *outPtr;
  int coords[3], *outIncs, outIncA;
  
  // Get the correct starting pointer of the output
  outPtr = outData->GetScalarPointerForExtent(outExt);
  outIncs = outData->GetIncrements();
  outIncA = outIncs[axis];
  
  // trick to account for the scalar type of the output(used to be only float)
  switch (outData->GetScalarType())
    {
    case VTK_DOUBLE:
      outIncA *= sizeof(double);
      break;
    case VTK_FLOAT:
      outIncA *= sizeof(float);
      break;
    case VTK_LONG:
      outIncA *= sizeof(long);
      break;
    case VTK_UNSIGNED_LONG:
      outIncA *= sizeof(unsigned long);
      break;
    case VTK_INT:
      outIncA *= sizeof(int);
      break;
    case VTK_UNSIGNED_INT:
      outIncA *= sizeof(unsigned int);
      break;
    case VTK_SHORT:
      outIncA *= sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      outIncA *= sizeof(unsigned short);
      break;
    case VTK_CHAR:
      outIncA *= sizeof(char);
      break;
    case VTK_UNSIGNED_CHAR:
      outIncA *= sizeof(unsigned char);
      break;
    default:
      vtkErrorMacro("Unknown scalar type");
      return;
    }

  // Determine default starting position of input
  coords[0] = inExt[0];
  coords[1] = inExt[2];
  coords[2] = inExt[4];
  
  // get whole extent for boundary checking ...
  wholeExtent = this->GetInput()->GetWholeExtent();
  wholeMin = wholeExtent[axis*2];
  wholeMax = wholeExtent[axis*2+1];  

  // allocate memory for the kernel
  radius = (int)(this->StandardDeviations[axis] * this->RadiusFactors[axis]);
  size = 2*radius + 1;
  kernel = new double[size];
  
  // loop over the convolution axis
  previousClipped = currentClipped = 1;
  max = outExt[axis*2+1];
  for (idxA = outExt[axis*2]; idxA <= max; ++idxA)
    {
    // left boundary condition
    coords[axis] = idxA - radius;
    kernelLeftClip = wholeMin - coords[axis];
    if (kernelLeftClip > 0)
      { // front of kernel is cut off ("kernelStart" samples)
      coords[axis] += kernelLeftClip;
      }
    else
      {
      kernelLeftClip = 0;
      }
    // Right boundary condition
    kernelRightClip = (idxA + radius) - wholeMax;
    if (kernelRightClip < 0)
      {
      kernelRightClip = 0;
      }
    
    // We can only use previous kernel if it is not clipped and new
    // kernel is also not clipped.
    currentClipped = kernelLeftClip + kernelRightClip;
    if (currentClipped || previousClipped)
      {
      this->ComputeKernel(kernel, -radius+kernelLeftClip, 
			  radius-kernelRightClip,
			  (double)(this->StandardDeviations[axis]));
      kernelSize = size - kernelLeftClip - kernelRightClip;
      }
    previousClipped = currentClipped;
    
    /* now do the convolution on the rest of the axes */
    inPtr = inData->GetScalarPointer(coords);
    switch (inData->GetScalarType())
      {
      case VTK_DOUBLE:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
				      inData, (double *)(inPtr),
				      outData, outExt, (double *)(outPtr),
				      pcycle, target, pcount, total);
	break;
      case VTK_FLOAT:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
				      inData, (float *)(inPtr),
				      outData, outExt, (float *)(outPtr),
				      pcycle, target, pcount, total);
	break;
      case VTK_LONG:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
				      inData, (long *)(inPtr),
				      outData, outExt, (long *)(outPtr),
				      pcycle, target, pcount, total);
	break;
      case VTK_UNSIGNED_LONG:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
			      inData, (unsigned long *)(inPtr),
			      outData, outExt, (unsigned long *)(outPtr),
			      pcycle, target, pcount, total);
				      
	break;
      case VTK_INT:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
				      inData, (int *)(inPtr),
				      outData, outExt, (int *)(outPtr),
				      pcycle, target, pcount, total);
	break;
      case VTK_UNSIGNED_INT:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
			      inData, (unsigned int *)(inPtr),
			      outData, outExt, (unsigned int *)(outPtr),
			      pcycle, target, pcount, total);
				      
	break;
      case VTK_SHORT:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
				      inData, (short *)(inPtr),
				      outData, outExt, (short *)(outPtr),
				      pcycle, target, pcount, total);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
			      inData, (unsigned short *)(inPtr),
			      outData, outExt, (unsigned short *)(outPtr),
			      pcycle, target, pcount, total);
				      
	break;
      case VTK_CHAR:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
				      inData, (char *)(inPtr),
				      outData, outExt, (char *)(outPtr),
				      pcycle, target, pcount, total);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
			      inData, (unsigned char *)(inPtr),
			      outData, outExt, (unsigned char *)(outPtr),
			      pcycle, target, pcount, total);
	break;
      default:
	vtkErrorMacro("Unknown scalar type");
	return;
      }
    outPtr = (void *)((unsigned char *)outPtr + outIncA);
    }
  
  // get rid of temporary kernel
  delete [] kernel;
}
  
//----------------------------------------------------------------------------
// This method decomposes the gaussian and smooths along each axis.
void vtkImageGaussianSmooth::ThreadedExecute(vtkImageData *inData, 
					     vtkImageData *outData,
					     int outExt[6], int id)
{
  int inExt[6];
  int target, count, total, cycle;
  
  // for feed back, determine line target to get 50 progress update
  // update is called every target lines. Progress is computed from
  // the number of pixels processed so far.
  count = 0; target = 0; total = 0; cycle = 0;
  if (id == 0)
    {
    // determine the number of pixels.
    total = this->Dimensionality * (outExt[1] - outExt[0] + 1) 
      * (outExt[3] - outExt[2] + 1) * (outExt[5] - outExt[4] + 1)
      * this->GetInput()->GetNumberOfScalarComponents();
    // pixels per update (50 updates)
    target = total / 50;
    }
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()              << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }

  // Decompose
  this->ComputeInputUpdateExtent(inExt, outExt);
  switch (this->Dimensionality)
    {
    case 1:
      this->ExecuteAxis(0, inData, inExt, outData, outExt, 
		  &cycle, target, &count, total);
      break;
    case 2:
      int tempExt[6];
      vtkImageData *tempData;
      // compute intermediate extent
      tempExt[0] = inExt[0];  tempExt[1] = inExt[1];
      tempExt[2] = outExt[2];  tempExt[3] = outExt[3];
      tempExt[4] = inExt[4];  tempExt[5] = inExt[5];
      // create a temp data for intermediate results
      tempData = vtkImageData::New();
      tempData->SetExtent(tempExt);
      tempData->SetNumberOfScalarComponents(inData->GetNumberOfScalarComponents());
      tempData->SetScalarType(inData->GetScalarType());
      this->ExecuteAxis(1, inData, inExt, tempData, tempExt, 
		  &cycle, target, &count, total);
      this->ExecuteAxis(0, tempData, tempExt, outData, outExt, 
		  &cycle, target, &count, total);
      // release temporary data
      tempData->Delete();
      tempData = NULL;
      break;
    case 3:
      // we do z first because it is most likely smallest
      int temp0Ext[6], temp1Ext[6];
      vtkImageData *temp0Data, *temp1Data;
      // compute intermediate extents
      temp0Ext[0] = inExt[0];  temp0Ext[1] = inExt[1];
      temp0Ext[2] = inExt[2];  temp0Ext[3] = inExt[3];
      temp0Ext[4] = outExt[4];  temp0Ext[5] = outExt[5];

      temp1Ext[0] = inExt[0];  temp1Ext[1] = inExt[1];
      temp1Ext[2] = outExt[2];  temp1Ext[3] = outExt[3];
      temp1Ext[4] = outExt[4];  temp1Ext[5] = outExt[5];
      
      // create a temp data for intermediate results
      temp0Data = vtkImageData::New();
      temp0Data->SetExtent(temp0Ext);
      temp0Data->SetNumberOfScalarComponents(inData->GetNumberOfScalarComponents());
      temp0Data->SetScalarType(inData->GetScalarType());

      temp1Data = vtkImageData::New();
      temp1Data->SetExtent(temp1Ext);
      temp1Data->SetNumberOfScalarComponents(inData->GetNumberOfScalarComponents());
      temp1Data->SetScalarType(inData->GetScalarType());
      this->ExecuteAxis(2, inData, inExt, temp0Data, temp0Ext,
		  &cycle, target, &count, total);
      this->ExecuteAxis(1, temp0Data, temp0Ext, temp1Data, temp1Ext,
		  &cycle, target, &count, total);
      temp0Data->Delete();
      temp0Data = NULL;
      this->ExecuteAxis(0, temp1Data, temp1Ext, outData, outExt,
		  &cycle, target, &count, total);
      temp1Data->Delete();
      temp1Data = NULL;
      break;
    }  
}















