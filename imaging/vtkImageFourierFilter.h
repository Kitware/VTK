/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierFilter.h
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
// .NAME vtkImageFourierFilter - Superclass that implements complex numbers.
// .SECTION Description
// vtkImageFourierFilter is a class of filters that use complex numbers
// this superclass is a container for methods that manipulate these structure
// including fast Fourier transforms.  Complex numbers may become a class.


#ifndef __vtkImageFourierFilter_h
#define __vtkImageFourierFilter_h


#include "vtkImageFilter.h"


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

// Hack for temporary variable
static vtkImageComplex _vtkImageComplexMultiplyTemp = {0.0, 0.0};
#define vtkImageComplexMultiply(C1, C2, cOut) \
  _vtkImageComplexMultiplyTemp.Real = (C1).Real*(C2).Real-(C1).Imag*(C2).Imag;\
  _vtkImageComplexMultiplyTemp.Imag = (C1).Real*(C2).Imag+(C1).Imag*(C2).Real;\
  cOut = _vtkImageComplexMultiplyTemp;

// This macro calculates exp(cIn) and puts the result in cOut 
#define vtkImageComplexExponential(cIn, cOut) \
{ \
  float temp = exp(cIn.Real); \
  cOut.Real = temp * cos(cIn.Imag); \
  cOut.Imag = temp * sin(cIn.Imag); \
}

/******************* End of COMPLEX number stuff ********************/
//ETX

class VTK_EXPORT vtkImageFourierFilter : public vtkImageFilter
{
public:
  static vtkImageFourierFilter *New() {return new vtkImageFourierFilter;};
  char *GetClassName() {return "vtkImageFourierFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // public for templated functions of this object
  //BTX
  void ExecuteFft(vtkImageComplex *in, vtkImageComplex *out, int N);
  void ExecuteRfft(vtkImageComplex *in, vtkImageComplex *out, int N);
  //ETX
  
protected:
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
