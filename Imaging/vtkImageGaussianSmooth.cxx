/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageGaussianSmooth.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageGaussianSmooth, "1.35");
vtkStandardNewMacro(vtkImageGaussianSmooth);

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
  this->Superclass::PrintSelf(os, indent);

  // int idx;  

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
  if (this->InputScalarsSelection)
    {
    os << indent << "InputScalarsSelection: " 
       << this->InputScalarsSelection << endl;
    }
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
void 
vtkImageGaussianSmoothExecute(vtkImageGaussianSmooth *self, int axis,
                              double *kernel, int kernelSize,
                              int inExt[6], int inIncs[3], T *inPtrC,
                              int outExt[6], int outIncs[3], T *outPtrC,
                              int *pcycle, int target, int *pcount, int total)
{
  int maxC, max0, max1;
  int idxC, idx0, idx1, idxK;
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
  inIncK = inIncs[axis];
  // Here is a trick to get the number of components of the output.
  maxC = outIncs[0];
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
                                         vtkDataArray *inArray, int inExt[6],
                                         vtkDataArray *outArray, int outExt[6],
                                         int *pcycle, int target, 
                                         int *pcount, int total)
{
  int idxA, max;
  double *kernel;
  // previousClip and currentClip rembers that the previous was not clipped
  // keeps from recomputing kernels for center pixels.
  int kernelSize = 0;
  int kernelLeftClip, kernelRightClip;
  int previousClipped, currentClipped;
  int radius, size;
  // Deal with data as uchar even though it may not be.
  unsigned char *inPtr;
  unsigned char *outPtr;
  int coords[3], outIncA;
  int inIncs[3];
  int outIncs[3];
  int dataSize;

  // Compute the increments relative to data elements.
  dataSize = inArray->GetDataTypeSize();
  inIncs[0] = inArray->GetNumberOfComponents();
  inIncs[1] = inIncs[0] * (inExt[1]-inExt[0]+1);
  inIncs[2] = inIncs[1] * (inExt[3]-inExt[2]+1);
  outIncs[0] = outArray->GetNumberOfComponents();
  outIncs[1] = outIncs[0] * (outExt[1]-outExt[0]+1);
  outIncs[2] = outIncs[1] * (outExt[3]-outExt[2]+1);

  // Get a pointer of the output.
  outPtr = (unsigned char*)(outArray->GetVoidPointer(0));
  outIncA = outIncs[axis] * dataSize;

  // Determine default starting position of input
  // Coords shift with loop.
  coords[0] = inExt[0];
  coords[1] = inExt[2];
  coords[2] = inExt[4];
  
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
    coords[axis] = idxA - radius; // What input min would be with no bounds.
    kernelLeftClip = inExt[axis*2] - coords[axis];
    if (kernelLeftClip > 0)
      { // front of kernel is cut off ("kernelStart" samples)
      coords[axis] += kernelLeftClip;
      }
    else
      {
      kernelLeftClip = 0;
      }
    // Right boundary condition
    kernelRightClip = (idxA + radius) - inExt[2*axis + 1];
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
    inPtr = (unsigned char*)(inArray->GetVoidPointer(0));

    inPtr += dataSize * ((coords[0]-inExt[0])*inIncs[0] +
                         (coords[1]-inExt[2])*inIncs[1] +
                         (coords[2]-inExt[4])*inIncs[2]);

    switch (inArray->GetDataType())
      {
      case VTK_DOUBLE:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (double *)(inPtr),
                              outExt, outIncs, (double *)(outPtr),
                              pcycle, target, pcount, total);
        break;
      case VTK_FLOAT:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (float *)(inPtr),
                              outExt, outIncs, (float *)(outPtr),
                              pcycle, target, pcount, total);
        break;
      case VTK_LONG:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (long *)(inPtr),
                              outExt, outIncs, (long *)(outPtr),
                              pcycle, target, pcount, total);
        break;
      case VTK_UNSIGNED_LONG:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (unsigned long *)(inPtr),
                              outExt, outIncs, (unsigned long *)(outPtr),
                              pcycle, target, pcount, total);
                                      
        break;
      case VTK_INT:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (int *)(inPtr),
                              outExt, outIncs, (int *)(outPtr),
                              pcycle, target, pcount, total);
        break;
      case VTK_UNSIGNED_INT:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (unsigned int *)(inPtr),
                              outExt, outIncs, (unsigned int *)(outPtr),
                              pcycle, target, pcount, total);
                                      
        break;
      case VTK_SHORT:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (short *)(inPtr),
                              outExt, outIncs, (short *)(outPtr),
                              pcycle, target, pcount, total);
        break;
      case VTK_UNSIGNED_SHORT:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (unsigned short *)(inPtr),
                              outExt, outIncs, (unsigned short *)(outPtr),
                              pcycle, target, pcount, total);
                                      
        break;
      case VTK_CHAR:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (char *)(inPtr),
                              outExt, outIncs, (char *)(outPtr),
                              pcycle, target, pcount, total);
        break;
      case VTK_UNSIGNED_CHAR:
        vtkImageGaussianSmoothExecute(this, axis, kernel, kernelSize,
                              inExt, inIncs, (unsigned char *)(inPtr),
                              outExt, outIncs, (unsigned char *)(outPtr),
                              pcycle, target, pcount, total);
        break;
      default:
        vtkErrorMacro("Unknown scalar type");
        return;
      }
    outPtr = outPtr + outIncA;
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
  vtkDataArray *inArray;
  vtkDataArray *outArray;

  inArray = inData->GetPointData()->GetScalars(this->InputScalarsSelection);
  outArray = outData->GetPointData()->GetScalars();
  
  // for feed back, determine line target to get 50 progress update
  // update is called every target lines. Progress is computed from
  // the number of pixels processed so far.
  count = 0; target = 0; total = 0; cycle = 0;
  if (id == 0)
    {
    // determine the number of pixels.
    total = this->Dimensionality * (outExt[1] - outExt[0] + 1) 
      * (outExt[3] - outExt[2] + 1) * (outExt[5] - outExt[4] + 1)
      * inArray->GetNumberOfComponents();
    // pixels per update (50 updates)
    target = total / 50;
    }
  
  // this filter expects that input is the same type as output.
  if (inArray->GetDataType() != outArray->GetDataType())
    {
    vtkErrorMacro(<< "Execute: input array DataType, " 
                  << inArray->GetDataType()              
                  << ", must match out array DataType " 
                  << outArray->GetDataType());
    return;
    }

  // Decompose
  this->ComputeInputUpdateExtent(inExt, outExt);
  switch (this->Dimensionality)
    {
    case 1:
      this->ExecuteAxis(0, inArray, inExt, outArray, outExt, 
                        &cycle, target, &count, total);
      break;
    case 2:
      int tempExt[6];
      vtkDataArray *tempArray;
      // compute intermediate extent
      tempExt[0] = inExt[0];  tempExt[1] = inExt[1];
      tempExt[2] = outExt[2];  tempExt[3] = outExt[3];
      tempExt[4] = inExt[4];  tempExt[5] = inExt[5];
      // create a temp data for intermediate results
      tempArray = inArray->MakeObject();
      tempArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
      tempArray->SetNumberOfTuples((tempExt[1]-tempExt[0]+1) *
                                   (tempExt[3]-tempExt[2]+1) *
                                   (tempExt[5]-tempExt[4]+1));
      this->ExecuteAxis(1, inArray, inExt, tempArray, tempExt, 
                        &cycle, target, &count, total);
      this->ExecuteAxis(0, tempArray, tempExt, outArray, outExt, 
                  &cycle, target, &count, total);
      // release temporary data
      tempArray->Delete();
      tempArray = NULL;
      break;
    case 3:
      // we do z first because it is most likely smallest
      int temp0Ext[6], temp1Ext[6];
      vtkDataArray *temp0Array, *temp1Array;

      // compute intermediate extents
      temp0Ext[0] = inExt[0];  temp0Ext[1] = inExt[1];
      temp0Ext[2] = inExt[2];  temp0Ext[3] = inExt[3];
      temp0Ext[4] = outExt[4];  temp0Ext[5] = outExt[5];
      // create a temp data for intermediate results
      temp0Array = inArray->MakeObject();
      temp0Array->SetNumberOfComponents(inArray->GetNumberOfComponents());
      temp0Array->SetNumberOfTuples((temp0Ext[1]-temp0Ext[0]+1) *
                                    (temp0Ext[3]-temp0Ext[2]+1) *
                                    (temp0Ext[5]-temp0Ext[4]+1));
      this->ExecuteAxis(2, inArray, inExt, temp0Array, temp0Ext,
                        &cycle, target, &count, total);
      // We try to release the input her for better memory efficiency.
      if (inData->GetReleaseDataFlag())
        {
        inData->ReleaseData();
        inArray = NULL;
        }

      // Next intermediate extent.
      temp1Ext[0] = inExt[0];  temp1Ext[1] = inExt[1];
      temp1Ext[2] = outExt[2];  temp1Ext[3] = outExt[3];
      temp1Ext[4] = outExt[4];  temp1Ext[5] = outExt[5];      
      // create a temp data for intermediate results
      temp1Array = inArray->MakeObject();
      temp1Array->SetNumberOfComponents(inArray->GetNumberOfComponents());
      temp1Array->SetNumberOfTuples((temp1Ext[1]-temp1Ext[0]+1) *
                                    (temp1Ext[3]-temp1Ext[2]+1) *
                                    (temp1Ext[5]-temp1Ext[4]+1));
      this->ExecuteAxis(1, temp0Array, temp0Ext, temp1Array, temp1Ext,
                        &cycle, target, &count, total);
      temp0Array->Delete();
      temp0Array = NULL;

      // Last pass into outArray
      this->ExecuteAxis(0, temp1Array, temp1Ext, outArray, outExt,
                        &cycle, target, &count, total);
      temp1Array->Delete();
      temp1Array = NULL;
      break;
    }  
}















