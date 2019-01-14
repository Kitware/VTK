/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageFourierFilter.h"

#include "vtkMath.h"
#include <cmath>

/*=========================================================================
        Vectors of complex numbers.
=========================================================================*/

//----------------------------------------------------------------------------
// This function calculates one step of a FFT.
// It is specialized for a factor of 2.
// It is engineered for no decimation.
// (forward: fb = 1, backward: fb = -1)
void vtkImageFourierFilter::ExecuteFftStep2(vtkImageComplex *p_in,
                                            vtkImageComplex *p_out,
                                            int N, int bsize, int fb)
{
  int i1, i2;
  vtkImageComplex *p1, *p2, *p3;
  vtkImageComplex q, fact1, fact, temp;

  /* Copy the links with no factors. */
  p1 = p_in;
  p3 = p_out;
  for(i1 = 0; i1 < N / (bsize * 2); ++i1)   // loop 0->1
  {
    p2 = p1;
    for(i2 = 0; i2 < bsize; ++i2)    // loop 0->2
    {
      *p3 = *p2;         // out[0] = in[0];  out[1] = in[1];
      ++p2;
      ++p3;
    }
    p2 = p1;
    for(i2 = 0; i2 < bsize; ++i2)
    {
      *p3 = *p2;         // out[2] = in[0];   out[3] = in[1];
      ++p2;
      ++p3;
    }
    p1 = p1 + bsize;
  }

  /* Add the links with factors. */
  fact1.Real = 1.0;
  fact1.Imag = 0.0;
  q.Real = 0.0;
  q.Imag = -(2.0 * vtkMath::Pi()) * fb / (bsize * 2.0);
  vtkImageComplexExponential(q, q);
  p3 = p_out;
  for(i1 = 0; i1 < N / (bsize * 2); ++i1)
  {
    fact = fact1;
    p2 = p1;
    for(i2 = 0; i2 < bsize; ++i2)
    {
      vtkImageComplexMultiply(fact, *p2, temp);
      vtkImageComplexAdd(temp, *p3, *p3);
      vtkImageComplexMultiply(q, fact, fact);
      ++p2;    // out[0] += in[2];   out[1] += -i*in[3];
      ++p3;
    }
    p2 = p1;
    for(i2 = 0; i2 < bsize; ++i2)
    {
      vtkImageComplexMultiply(fact, *p2, temp);
      vtkImageComplexAdd(temp, *p3, *p3);
      vtkImageComplexMultiply(q, fact, fact);
      ++p2;
      ++p3;
    }
    p1 = p1 + bsize;
  }
}

//----------------------------------------------------------------------------
// This function calculates one step of a FFT (using any factor).
// It is engineered for no decimation.
//  N: length of arrays
//  bsize: Size of FFT so far (should be scaled by n after this step)
//  n: size of this steps butterfly.
//  fb: forward: fb = 1, backward: fb = -1
void vtkImageFourierFilter::ExecuteFftStepN(vtkImageComplex *p_in,
                                            vtkImageComplex *p_out,
                                            int N, int bsize, int n, int fb)
{
  int i0, i1, i2, i3;
  vtkImageComplex *p1, *p2, *p3;
  vtkImageComplex q, fact, temp;

  p3 = p_out;
  for(i0 = 0; i0 < N; ++i0)
  {
    p3->Real = 0.0;
    p3->Imag = 0.0;
    ++p3;
  }

  p1 = p_in;
  for(i0 = 0; i0 < n; ++i0)
  {
    q.Real = 0.0;
    q.Imag = -(2.0 * vtkMath::Pi()) * i0 * fb / (bsize*1.0*n);
    vtkImageComplexExponential(q, q);
    p3 = p_out;
    for(i1 = 0; i1 < N / (bsize * n); ++i1)
    {
      fact.Real = 1.0;
      fact.Imag = 0.0;
      for(i3 = 0; i3 < n; ++i3)
      {
        p2 = p1;
        for(i2 = 0; i2 < bsize; ++i2)
        {
          vtkImageComplexMultiply(fact, *p2, temp);
          vtkImageComplexAdd(temp, *p3, *p3);
          vtkImageComplexMultiply(q, fact, fact);
          ++p2;
          ++p3;
        }
      }

      p1 = p1 + bsize;
    }
  }
}



//----------------------------------------------------------------------------
// This function calculates the whole fft (or rfft) of an array.
// The contents of the input array are changed.
// It is engineered for no decimation so input and output cannot be equal.
// (fb = 1) => fft, (fb = -1) => rfft;
void vtkImageFourierFilter::ExecuteFftForwardBackward(vtkImageComplex *in,
                                                      vtkImageComplex *out,
                                                      int N, int fb)
{
  vtkImageComplex *p1, *p2, *p3;
  int block_size = 1;
  int rest_size = N;
  int n = 2;
  int idx;

  // If this is a reverse transform (scale accordingly).
  if(fb == -1)
  {
    p1 = in;
    for(idx = 0; idx < N; ++idx)
    {
      p1->Real = p1->Real / N;
      p1->Imag = p1->Imag / N;
      ++p1;
    }
  }
  p1 = in;
  p2 = out;
  while(block_size < N && n <= N)
  {
    if((rest_size % n) == 0)
    {
      // n is a prime factor, perform one "butterfly" stage of the fft.
      if(n == 2)
      {
        this->ExecuteFftStep2(p1, p2, N, block_size, fb);
      }
      else
      {
        this->ExecuteFftStepN(p1, p2, N, block_size, n, fb);
      }
      block_size = block_size * n;
      rest_size = rest_size / n;
      // switch input and output.
      p3 = p1;
      p1 = p2;
      p2 = p3;
    }
    else
    {
      // n is not a prime factor. increment n to see if n+1 is.
      ++n;
    }
  }
  // If the results ended up in the input, copy to output.
  if(p1 != out)
  {
    for(n = 0; n < N; ++n)
    {
      *out++ = *p1++;
    }
  }
}



//----------------------------------------------------------------------------
// This function calculates the whole fft of an array.
// The contents of the input array are changed.
// (It is engineered for no decimation)
void vtkImageFourierFilter::ExecuteFft(vtkImageComplex *in,
                                       vtkImageComplex *out, int N)
{
  this->ExecuteFftForwardBackward(in, out, N, 1);
}

//----------------------------------------------------------------------------
// This function calculates the whole fft of an array.
// The contents of the input array are changed.
// (It is engineered for no decimation)
void vtkImageFourierFilter::ExecuteRfft(vtkImageComplex *in,
                                        vtkImageComplex *out, int N)
{
  this->ExecuteFftForwardBackward(in, out, N, -1);
}

//----------------------------------------------------------------------------
// Called each axis over which the filter is executed.
int vtkImageFourierFilter::RequestData(vtkInformation* request,
                                       vtkInformationVector** inputVector,
                                       vtkInformationVector* outputVector)
{
  // ensure that iteration axis is not split during threaded execution
  this->SplitPathLength = 0;
  for (int axis = 2; axis >= 0; --axis)
  {
    if (axis != this->Iteration)
    {
      this->SplitPath[this->SplitPathLength++] = axis;
    }
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}


