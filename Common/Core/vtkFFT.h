/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkFFT
 * @brief perform Discrete Fourier Transforms
 *
 * vtkFFT provides methods to perform Discrete Fourier Transforms.
 * These include providing forward and reverse Fourier transforms.
 */

#ifndef vtkFFT_h
#define vtkFFT_h

#include "vtkObject.h"
#include "vtk_kissfft.h"
// clang-format off
#include VTK_KISSFFT_HEADER(kiss_fft.h)
#include VTK_KISSFFT_HEADER(tools/kiss_fftr.h)
// clang-format on
#include <complex>
#include <vector>

class VTKCOMMONCORE_EXPORT vtkFFT : public vtkObject
{
public:
  using ScalarNumber = kiss_fft_scalar;
  using ComplexNumber = kiss_fft_cpx;

  static vtkFFT* New();
  vtkTypeMacro(vtkFFT, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute the one-dimensional discrete Fourier Transform for real input
   *
   *  input has nfft scalar points
   *  output has nfft/2+1 complex points
   */
  static std::vector<ComplexNumber> FftDirect(const std::vector<ScalarNumber>& in);

  /**
   * Compute the inverse of DFT
   *
   *  input has  nfft/2+1 complex points
   *  output has nfft scalar points
   */
  static std::vector<ScalarNumber> FftInverse(const std::vector<ComplexNumber>& in);

  /**
   * Return the module of complex number
   */
  static double ComplexModule(const ComplexNumber& in);

  /**
   * Return the Discrete Fourier Transform sample frequencies
   */
  static std::vector<double> RFftFreq(int windowLength, double sampleSpacing);

protected:
  vtkFFT() = default;
  ~vtkFFT() override = default;

private:
  vtkFFT(const vtkFFT&) = delete;
  void operator=(const vtkFFT&) = delete;
};

#endif
