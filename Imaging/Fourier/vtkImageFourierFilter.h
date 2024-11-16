// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkImageDecomposeFilter.h"
#include "vtkImagingFourierModule.h" // For export macro

/*******************************************************************
                        COMPLEX number stuff
*******************************************************************/

VTK_ABI_NAMESPACE_BEGIN
struct vtkImageComplex_t
{
  double Real;
  double Imag;
};
using vtkImageComplex = struct vtkImageComplex_t;

#define vtkImageComplexEuclidSet(C, R, I)                                                          \
  do                                                                                               \
  {                                                                                                \
    (C).Real = (R);                                                                                \
    (C).Imag = (I);                                                                                \
  } while (false)

#define vtkImageComplexPolarSet(C, M, P)                                                           \
  do                                                                                               \
  {                                                                                                \
    (C).Real = (M)*cos(P);                                                                         \
    (C).Imag = (M)*sin(P);                                                                         \
  } while (false)

#define vtkImageComplexPrint(C)                                                                    \
  do                                                                                               \
  {                                                                                                \
    printf("(%.3f, %.3f)", (C).Real, (C).Imag);                                                    \
  } while (false)

#define vtkImageComplexScale(cOut, S, cIn)                                                         \
  do                                                                                               \
  {                                                                                                \
    (cOut).Real = (cIn).Real * (S);                                                                \
    (cOut).Imag = (cIn).Imag * (S);                                                                \
  } while (false)

#define vtkImageComplexConjugate(cIn, cOut)                                                        \
  do                                                                                               \
  {                                                                                                \
    (cOut).Imag = (cIn).Imag * -1.0;                                                               \
    (cOut).Real = (cIn).Real;                                                                      \
  } while (false)

#define vtkImageComplexAdd(C1, C2, cOut)                                                           \
  do                                                                                               \
  {                                                                                                \
    (cOut).Real = (C1).Real + (C2).Real;                                                           \
    (cOut).Imag = (C1).Imag + (C2).Imag;                                                           \
  } while (false)

#define vtkImageComplexSubtract(C1, C2, cOut)                                                      \
  do                                                                                               \
  {                                                                                                \
    (cOut).Real = (C1).Real - (C2).Real;                                                           \
    (cOut).Imag = (C1).Imag - (C2).Imag;                                                           \
  } while (false)

#define vtkImageComplexMultiply(C1, C2, cOut)                                                      \
  do                                                                                               \
  {                                                                                                \
    vtkImageComplex vtkImageComplex_tMultiplyTemp;                                                 \
    vtkImageComplex_tMultiplyTemp.Real = (C1).Real * (C2).Real - (C1).Imag * (C2).Imag;            \
    vtkImageComplex_tMultiplyTemp.Imag = (C1).Real * (C2).Imag + (C1).Imag * (C2).Real;            \
    cOut = vtkImageComplex_tMultiplyTemp;                                                          \
  } while (false)

// This macro calculates exp(cIn) and puts the result in cOut
#define vtkImageComplexExponential(cIn, cOut)                                                      \
  do                                                                                               \
  {                                                                                                \
    double tmp = exp(cIn.Real);                                                                    \
    cOut.Real = tmp * cos(cIn.Imag);                                                               \
    cOut.Imag = tmp * sin(cIn.Imag);                                                               \
  } while (false)

/******************* End of COMPLEX number stuff ********************/

class VTKIMAGINGFOURIER_EXPORT vtkImageFourierFilter : public vtkImageDecomposeFilter
{
public:
  vtkTypeMacro(vtkImageFourierFilter, vtkImageDecomposeFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // public for templated functions of this object

  /**
   * This function calculates the whole fft of an array.
   * The contents of the input array are changed.
   * (It is engineered for no decimation)
   */
  void ExecuteFft(vtkImageComplex* in, vtkImageComplex* out, int N);

  /**
   * This function calculates the whole fft of an array.
   * The contents of the input array are changed.
   * (It is engineered for no decimation)
   */
  void ExecuteRfft(vtkImageComplex* in, vtkImageComplex* out, int N);

protected:
  vtkImageFourierFilter() = default;
  ~vtkImageFourierFilter() override = default;

  void ExecuteFftStep2(vtkImageComplex* p_in, vtkImageComplex* p_out, int N, int bsize, int fb);
  void ExecuteFftStepN(
    vtkImageComplex* p_in, vtkImageComplex* p_out, int N, int bsize, int n, int fb);
  void ExecuteFftForwardBackward(vtkImageComplex* in, vtkImageComplex* out, int N, int fb);

  /**
   * Override to change extent splitting rules.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkImageFourierFilter(const vtkImageFourierFilter&) = delete;
  void operator=(const vtkImageFourierFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
