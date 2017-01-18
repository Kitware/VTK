/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageFourierFilter
 * @brief   Superclass that implements complex numbers.
 *
 * vtkImageFourierFilter is a class of filters that use complex numbers
 * this superclass is a container for methods that manipulate these structure
 * including fast Fourier transforms.  Complex numbers may become a class.
 * This should really be a helper class.
*/

#ifndef vtkImageFourierFilter_h
#define vtkImageFourierFilter_h


#include "vtkImagingFourierModule.h" // For export macro
#include "vtkImageDecomposeFilter.h"

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
  double tmp = exp(cIn.Real); \
  cOut.Real = tmp * cos(cIn.Imag); \
  cOut.Imag = tmp * sin(cIn.Imag); \
}

/******************* End of COMPLEX number stuff ********************/

class VTKIMAGINGFOURIER_EXPORT vtkImageFourierFilter : public vtkImageDecomposeFilter
{
public:
  vtkTypeMacro(vtkImageFourierFilter,vtkImageDecomposeFilter);


  // public for templated functions of this object

  /**
   * This function calculates the whole fft of an array.
   * The contents of the input array are changed.
   * (It is engineered for no decimation)
   */
  void ExecuteFft(vtkImageComplex *in, vtkImageComplex *out, int N);


  /**
   * This function calculates the whole fft of an array.
   * The contents of the input array are changed.
   * (It is engineered for no decimation)
   */
  void ExecuteRfft(vtkImageComplex *in, vtkImageComplex *out, int N);

protected:
  vtkImageFourierFilter() {}
  ~vtkImageFourierFilter()VTK_OVERRIDE {}

  void ExecuteFftStep2(vtkImageComplex *p_in, vtkImageComplex *p_out,
                       int N, int bsize, int fb);
  void ExecuteFftStepN(vtkImageComplex *p_in, vtkImageComplex *p_out,
                       int N, int bsize, int n, int fb);
  void ExecuteFftForwardBackward(vtkImageComplex *in, vtkImageComplex *out,
                                 int N, int fb);

  /**
   * Override to change extent splitting rules.
   */
  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) VTK_OVERRIDE;

private:
  vtkImageFourierFilter(const vtkImageFourierFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageFourierFilter&) VTK_DELETE_FUNCTION;
};



#endif


// VTK-HeaderTest-Exclude: vtkImageFourierFilter.h
