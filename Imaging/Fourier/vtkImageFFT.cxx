/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFFT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageFFT.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageFFT);

//----------------------------------------------------------------------------
// This extent of the components changes to real and imaginary values.
int vtkImageFFT::IterativeRequestInformation(
  vtkInformation* vtkNotUsed(input), vtkInformation* output)
{
  vtkDataObject::SetPointDataActiveScalarInfo(output, VTK_DOUBLE, 2);
  return 1;
}

//----------------------------------------------------------------------------
static void vtkImageFFTInternalRequestUpdateExtent(int *inExt, int *outExt,
                                                   int *wExt, int iteration)
{
  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[iteration*2] = wExt[iteration*2];
  inExt[iteration*2 + 1] = wExt[iteration*2 + 1];
}

//----------------------------------------------------------------------------
// This method tells the superclass that the whole input array is needed
// to compute any output region.
int vtkImageFFT::IterativeRequestUpdateExtent(
  vtkInformation* input, vtkInformation* output)
{
  int *outExt = output->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  int *wExt = input->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int inExt[6];
  vtkImageFFTInternalRequestUpdateExtent(inExt,outExt,wExt,this->Iteration);
  input->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),inExt,6);

  return 1;
}

//----------------------------------------------------------------------------
// This templated execute method handles any type input, but the output
// is always doubles.
template <class T>
void vtkImageFFTExecute(vtkImageFFT *self,
                        vtkImageData *inData, int inExt[6], T *inPtr,
                        vtkImageData *outData, int outExt[6], double *outPtr,
                        int id)
{
  vtkImageComplex *inComplex;
  vtkImageComplex *outComplex;
  vtkImageComplex *pComplex;
  //
  int inMin0, inMax0;
  vtkIdType inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  //
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  vtkIdType outInc0, outInc1, outInc2;
  double *outPtr0, *outPtr1, *outPtr2;
  //
  int idx0, idx1, idx2, inSize0, numberOfComponents;
  unsigned long count = 0;
  unsigned long target;
  double startProgress;

  startProgress =
    self->GetIteration()/static_cast<double>(self->GetNumberOfIterations());

  // Reorder axes (The outs here are just placeholdes
  self->PermuteExtent(inExt, inMin0, inMax0, outMin1,outMax1,outMin2,outMax2);
  self->PermuteExtent(outExt, outMin0,outMax0,outMin1,outMax1,outMin2,outMax2);
  self->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);

  inSize0 = inMax0 - inMin0 + 1;

  // Input has to have real components at least.
  numberOfComponents = inData->GetNumberOfScalarComponents();
  if (numberOfComponents < 1)
    {
    vtkGenericWarningMacro("No real components");
    return;
    }

  // Allocate the arrays of complex numbers
  inComplex = new vtkImageComplex[inSize0];
  outComplex = new vtkImageComplex[inSize0];

  target = static_cast<unsigned long>((outMax2-outMin2+1)*(outMax1-outMin1+1)
                                      * self->GetNumberOfIterations() / 50.0);
  target++;

  // loop over other axes
  inPtr2 = inPtr;
  outPtr2 = outPtr;
  for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = outMin1; !self->AbortExecute && idx1 <= outMax1; ++idx1)
      {
      if (!id)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target) + startProgress);
          }
        count++;
        }
      // copy into complex numbers
      inPtr0 = inPtr1;
      pComplex = inComplex;
      for (idx0 = inMin0; idx0 <= inMax0; ++idx0)
        {
        pComplex->Real = static_cast<double>(*inPtr0);
        pComplex->Imag = 0.0;
        if (numberOfComponents > 1)
          { // yes we have an imaginary input
          pComplex->Imag = static_cast<double>(inPtr0[1]);;
          }
        inPtr0 += inInc0;
        ++pComplex;
        }

      // Call the method that performs the fft
      self->ExecuteFft(inComplex, outComplex, inSize0);

      // copy into output
      outPtr0 = outPtr1;
      pComplex = outComplex + (outMin0 - inMin0);
      for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
        {
        *outPtr0 = static_cast<double>(pComplex->Real);
        outPtr0[1] = static_cast<double>(pComplex->Imag);
        outPtr0 += outInc0;
        ++pComplex;
        }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }

  delete [] inComplex;
  delete [] outComplex;
}


//----------------------------------------------------------------------------
// This method is passed input and output Datas, and executes the fft
// algorithm to fill the output from the input.
// Not threaded yet.
void vtkImageFFT::ThreadedRequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector ),
  vtkImageData ***inDataVec,
  vtkImageData **outDataVec,
  int outExt[6],
  int threadId)
{
  vtkImageData* inData = inDataVec[0][0];
  vtkImageData* outData = outDataVec[0];
  void *inPtr, *outPtr;
  int inExt[6];
  int *wExt = inputVector[0]->GetInformationObject(0)->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  vtkImageFFTInternalRequestUpdateExtent(inExt,outExt,wExt,this->Iteration);

  inPtr = inData->GetScalarPointerForExtent(inExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);

  // this filter expects that the output be doubles.
  if (outData->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "Execute: Output must be be type double.");
    return;
    }

  // this filter expects input to have 1 or two components
  if (outData->GetNumberOfScalarComponents() != 1 &&
      outData->GetNumberOfScalarComponents() != 2)
    {
    vtkErrorMacro(<< "Execute: Cannot handle more than 2 components");
    return;
    }

  // choose which templated function to call.
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(vtkImageFFTExecute(this, inData, inExt,
                                        static_cast<VTK_TT *>(inPtr), outData,
                                        outExt,
                                        static_cast<double *>(outPtr),
                                        threadId));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}



//----------------------------------------------------------------------------
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of peices resulting from a successful split.
// This can be from 1 to "total".
// If 1 is returned, the extent cannot be split.
int vtkImageFFT::SplitExtent(int splitExt[6], int startExt[6],
                             int num, int total)
{
  int splitAxis;
  int min, max;

  vtkDebugMacro("SplitExtent: ( " << startExt[0] << ", " << startExt[1] << ", "
                << startExt[2] << ", " << startExt[3] << ", "
                << startExt[4] << ", " << startExt[5] << "), "
                << num << " of " << total);

  // start with same extent
  memcpy(splitExt, startExt, 6 * sizeof(int));

  splitAxis = 2;
  min = startExt[4];
  max = startExt[5];
  while ((splitAxis == this->Iteration) || (min == max))
    {
    splitAxis--;
    if (splitAxis < 0)
      { // cannot split
      vtkDebugMacro("  Cannot Split");
      return 1;
      }
    min = startExt[splitAxis*2];
    max = startExt[splitAxis*2+1];
    }

  // determine the actual number of pieces that will be generated
  if ((max - min + 1) < total)
    {
    total = max - min + 1;
    }

  if (num >= total)
    {
    vtkDebugMacro("  SplitRequest (" << num
                  << ") larger than total: " << total);
    return total;
    }

  // determine the extent of the piece
  splitExt[splitAxis*2] = min + (max - min + 1)*num/total;
  if (num == total - 1)
    {
    splitExt[splitAxis*2+1] = max;
    }
  else
    {
    splitExt[splitAxis*2+1] = (min-1) + (max - min + 1)*(num+1)/total;
    }

  vtkDebugMacro("  Split Piece: ( " <<splitExt[0]<< ", " <<splitExt[1]<< ", "
                << splitExt[2] << ", " << splitExt[3] << ", "
                << splitExt[4] << ", " << splitExt[5] << ")");

  return total;
}


















