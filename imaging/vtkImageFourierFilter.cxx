/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include <math.h>
#include "vtkImageFourierFilter.h"




/*=========================================================================
	Vectors of complex numbers.
=========================================================================*/




//----------------------------------------------------------------------------
void vtkImageFourierFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);

  // avoid a warning
  _vtkImageComplexMultiplyTemp.Real = 0.0;
}




//----------------------------------------------------------------------------
// Description:
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
  q.Imag = -2.0 * 3.141592654 * (float)(fb) / (float)(bsize * 2);
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
// Description:
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
  p3 = p_out;
  for(i0 = 0; i0 < n; ++i0)
    {
    q.Real = 0.0;
    q.Imag = -2.0 * 3.141592654 * (float)(i0) * (float)(fb) / (float)(bsize*n);
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
// Description:
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
      p1->Real = p1->Real / (float)(N);
      p1->Imag = p1->Imag / (float)(N);
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
// Description:
// This function calculates the whole fft of an array.
// The contents of the input array are changed.
// (It is engineered for no decimation)
void vtkImageFourierFilter::ExecuteFft(vtkImageComplex *in, 
				       vtkImageComplex *out, int N)
{
  this->ExecuteFftForwardBackward(in, out, N, 1);
}

//----------------------------------------------------------------------------
// Description:
// This function calculates the whole fft of an array.
// The contents of the input array are changed.
// (It is engineered for no decimation)
void vtkImageFourierFilter::ExecuteRfft(vtkImageComplex *in, 
					vtkImageComplex *out, int N)
{
  this->ExecuteFftForwardBackward(in, out, N, -1);
}
 

