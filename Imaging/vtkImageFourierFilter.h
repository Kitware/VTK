/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierFilter.h
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
// .NAME vtkImageFourierFilter - Superclass that implements complex numbers.
// .SECTION Description
// vtkImageFourierFilter is a class of filters that use complex numbers
// this superclass is a container for methods that manipulate these structure
// including fast Fourier transforms.  Complex numbers may become a class.
// This should really be a helper class.
#ifndef __vtkImageFourierFilter_h
#define __vtkImageFourierFilter_h


#include "vtkImageDecomposeFilter.h"


//BTX
/*******************************************************************
			COMPLEX number stuff
*******************************************************************/


typedef struct{
    double Real;
    double Imag;
  } vtkImageComplex;


#define vtkImageComplexEuclidSet(C, R, I) \
  (C).Real = (R); \
  (C).Imag = (I)

#define vtkImageComplexPolarSet(C, M, P) \
  (C).Real = (M)*cos(P); \
  (C).Imag = (M)*sin(P)

#define vtkImageComplexPrint(C) \
  printf("(%.3f, %.3f)", (C).Real, (C).Imag)

#define vtkImageComplexScale(cOut, S, cIn) \
  (cOut).Real = (cIn).Real * (S); \
  (cOut).Imag = (cIn).Imag * (S)

#define vtkImageComplexConjugate(cIn, cOut) \
  (cOut).Imag = (cIn).Imag * -1.0;    \
  (cOut).Real = (cIn).Real

#define vtkImageComplexAdd(C1, C2, cOut) \
  (cOut).Real = (C1).Real + (C2).Real; \
  (cOut).Imag = (C1).Imag + (C2).Imag

#define vtkImageComplexSubtract(C1, C2, cOut) \
  (cOut).Real = (C1).Real - (C2).Real; \
  (cOut).Imag = (C1).Imag - (C2).Imag

#define vtkImageComplexMultiply(C1, C2, cOut) \
{ \
  vtkImageComplex _vtkImageComplexMultiplyTemp; \
  _vtkImageComplexMultiplyTemp.Real = (C1).Real*(C2).Real-(C1).Imag*(C2).Imag;\
  _vtkImageComplexMultiplyTemp.Imag = (C1).Real*(C2).Imag+(C1).Imag*(C2).Real;\
  cOut = _vtkImageComplexMultiplyTemp; \
}

// This macro calculates exp(cIn) and puts the result in cOut 
#define vtkImageComplexExponential(cIn, cOut) \
{ \
  float tmp = exp(cIn.Real); \
  cOut.Real = tmp * cos(cIn.Imag); \
  cOut.Imag = tmp * sin(cIn.Imag); \
}

/******************* End of COMPLEX number stuff ********************/
//ETX

class VTK_IMAGING_EXPORT vtkImageFourierFilter : public vtkImageDecomposeFilter
{
public:
  vtkTypeMacro(vtkImageFourierFilter,vtkImageDecomposeFilter);
  
  
  // public for templated functions of this object
  //BTX

  // Description:
  // This function calculates the whole fft of an array.
  // The contents of the input array are changed.
  // (It is engineered for no decimation)
  void ExecuteFft(vtkImageComplex *in, vtkImageComplex *out, int N);


  // Description:
  // This function calculates the whole fft of an array.
  // The contents of the input array are changed.
  // (It is engineered for no decimation)
  void ExecuteRfft(vtkImageComplex *in, vtkImageComplex *out, int N);

  //ETX
  
protected:
  vtkImageFourierFilter() {};
  ~vtkImageFourierFilter() {};
  vtkImageFourierFilter(const vtkImageFourierFilter&);
  void operator=(const vtkImageFourierFilter&);

  //BTX
  void ExecuteFftStep2(vtkImageComplex *p_in, vtkImageComplex *p_out, 
		       int N, int bsize, int fb);
  void ExecuteFftStepN(vtkImageComplex *p_in, vtkImageComplex *p_out,
		       int N, int bsize, int n, int fb);
  void ExecuteFftForwardBackward(vtkImageComplex *in, vtkImageComplex *out, 
				 int N, int fb);
  //ETX
};



#endif


