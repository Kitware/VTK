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

#include <complex>
#include <vector>

class VTKCOMMONCORE_EXPORT vtkFFT : public vtkObject
{
public:
  using FFT_UNIT_TYPE = float;
  using ComplexNumber = std::complex<FFT_UNIT_TYPE>;

  static vtkFFT* New();
  vtkTypeMacro(vtkFFT, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute the one-dimensional discrete Fourier Transform for real input
   */
  static void FftDirect(
    const double* in, const int inCount, int* outCount, ComplexNumber*& outData);

  /**
   * Compute the inverse of DFT
   */
  static void FftInverse(
    const ComplexNumber* in, const int inCount, int* outCount, ComplexNumber*& outData);

  /**
   * Convert an array of complex numbers to array of doubles
   */
  static void ComplexesToDoubles(
    double* __restrict out, const ComplexNumber* __restrict in, const int inCount);

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
