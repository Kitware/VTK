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
 * vtkFFT provides methods to perform Discrete Fourier Transforms (DFT).
 * These include providing forward and reverse Fourier transforms.
 * The current implementation uses the third-party library kissfft.
 *
 * The terminology tries to follow the Numpy terminology, that is :
 *  - Fft means the Fast Fourier Transform algorithm
 *  - Prefix `R` stands for Real (meaning optimized function for real inputs)
 *  - Prefix `I` stands for Inverse
 */

#ifndef vtkFFT_h
#define vtkFFT_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkMath.h"             // For vtkMath::Pi
#include "vtkObject.h"

#include "vtk_kissfft.h" // For kiss_fft_scalar, kiss_fft_cpx
// clang-format off
#include VTK_KISSFFT_HEADER(kiss_fft.h)
#include VTK_KISSFFT_HEADER(tools/kiss_fftr.h)
// clang-format on

#include <vector> // For std::vector
#include <cmath>  // for std::sin, std::cos, std::sqrt

class VTKCOMMONMATH_EXPORT vtkFFT : public vtkObject
{
public:
  using ScalarNumber = kiss_fft_scalar;
  using ComplexNumber = kiss_fft_cpx;

  static vtkFFT* New();
  vtkTypeMacro(vtkFFT, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Compute the one-dimensional DFT for complex input.
   * If input is scalars then the imaginary part is set to 0
   *
   * input has n complex points
   * output has n complex points in case of success and empty in case of failure
   */
  static std::vector<ComplexNumber> Fft(const std::vector<ComplexNumber>& in);
  static std::vector<ComplexNumber> Fft(const std::vector<ScalarNumber>& in);
  ///@}

  /**
   * Compute the one-dimensional DFT for real input
   *
   *  input has n scalar points
   *  output has (n/2) + 1 complex points in case of success and empty in case of failure
   */
  static std::vector<ComplexNumber> RFft(const std::vector<ScalarNumber>& in);

  /**
   * Compute the inverse of @c Fft. The input should be ordered in the same way as is returned by @c
   * Fft, i.e.,
   *  - in[0] should contain the zero frequency term,
   *  - in[1:n//2] should contain the positive-frequency terms,
   *  - in[n//2 + 1:] should contain the negative-frequency terms.
   *
   *  input has n complex points
   *  output has n scalar points in case of success and empty in case of failure
   */
  static std::vector<ComplexNumber> IFft(const std::vector<ComplexNumber>& in);

  /**
   * Compute the inverse of @c RFft. The input is expected to be in the form returned by @c Rfft,
   * i.e. the real zero-frequency term followed by the complex positive frequency terms in
   * order of increasing frequency.
   *
   *  input has  (n/2) + 1 complex points
   *  output has n scalar points in case of success and empty in case of failure
   */
  static std::vector<ScalarNumber> IRFft(const std::vector<ComplexNumber>& in);

  /**
   * Return the absolute value (also known as norm, modulus, or magnitude) of complex number
   */
  static inline double Abs(const ComplexNumber& in);

  /**
   * Return the squared absolute value of the complex number
   */
  static inline double SquaredAbs(const ComplexNumber& in);

  /**
   * Return the DFT sample frequencies. Output has @c windowLength size.
   */
  static std::vector<double> FftFreq(int windowLength, double sampleSpacing);

  /**
   * Return the DFT sample frequencies for the real version of the dft (see @c Rfft).
   * Output has @c (windowLength / 2) + 1 size.
   */
  static std::vector<double> RFftFreq(int windowLength, double sampleSpacing);

  ///@{
  /**
   * Window generator functions. Implementation only needs to be valid for x E [0; size / 2]
   * because kernels are symmetric by definitions. This point is very important for some
   * kernels like Bartlett for example.
   *
   * @warning Hanning and Bartlett generators needs a size > 1 !
   *
   * Can be used with @c GenerateKernel1D and @c GenerateKernel2D for generating full kernels.
   */
  using WindowGenerator = double (*)(std::size_t, std::size_t);

  static inline double HanningGenerator(std::size_t x, std::size_t size);
  static inline double BartlettGenerator(std::size_t x, std::size_t size);
  static inline double SineGenerator(std::size_t x, std::size_t size);
  static inline double BlackmanGenerator(std::size_t x, std::size_t size);
  static inline double RectangularGenerator(std::size_t x, std::size_t size);
  ///@}

  /**
   * Given a window generator function, create a symmetric 1D kernel.
   * @c kernel is the pointer to the raw data array
   */
  template <typename T>
  static void GenerateKernel1D(T* kernel, std::size_t n, WindowGenerator generator);

  /**
   * Given a window generator function, create a symmetric 2D kernel.
   * @c kernel is the pointer to the raw 2D data array.
   */
  template <typename T>
  static void GenerateKernel2D(T* kernel, std::size_t n, std::size_t m, WindowGenerator generator);

protected:
  vtkFFT() = default;
  ~vtkFFT() override = default;

private:
  vtkFFT(const vtkFFT&) = delete;
  void operator=(const vtkFFT&) = delete;
};

//------------------------------------------------------------------------------
inline vtkFFT::ComplexNumber operator+(
  const vtkFFT::ComplexNumber& lhs, const vtkFFT::ComplexNumber& rhs)
{
  return vtkFFT::ComplexNumber{ lhs.r + rhs.r, lhs.i + rhs.i };
}
inline vtkFFT::ComplexNumber operator-(
  const vtkFFT::ComplexNumber& lhs, const vtkFFT::ComplexNumber& rhs)
{
  return vtkFFT::ComplexNumber{ lhs.r - rhs.r, lhs.i - rhs.i };
}
inline vtkFFT::ComplexNumber operator*(
  const vtkFFT::ComplexNumber& lhs, const vtkFFT::ComplexNumber& rhs)
{
  return vtkFFT::ComplexNumber{ (lhs.r * rhs.r) - (lhs.i * rhs.i),
    (lhs.r * rhs.i) + (lhs.i * rhs.r) };
}
inline vtkFFT::ComplexNumber operator*(
  const vtkFFT::ComplexNumber& lhs, const vtkFFT::ScalarNumber& rhs)
{
  return vtkFFT::ComplexNumber{ lhs.r * rhs, lhs.i * rhs };
}
inline vtkFFT::ComplexNumber operator/(
  const vtkFFT::ComplexNumber& lhs, const vtkFFT::ComplexNumber& rhs)
{
  const double divisor = rhs.r * rhs.r + rhs.i * rhs.i;
  return vtkFFT::ComplexNumber{ ((lhs.r * rhs.r) + (lhs.i * rhs.i)) / divisor,
    ((lhs.i * rhs.r) - (lhs.r * rhs.i)) / divisor };
}
inline vtkFFT::ComplexNumber operator/(
  const vtkFFT::ComplexNumber& lhs, const vtkFFT::ScalarNumber& rhs)
{
  return vtkFFT::ComplexNumber{ lhs.r / rhs, lhs.i / rhs };
}

//------------------------------------------------------------------------------
double vtkFFT::Abs(const ComplexNumber& in)
{
  return std::sqrt(in.r * in.r + in.i * in.i);
}

//------------------------------------------------------------------------------
double vtkFFT::SquaredAbs(const ComplexNumber& in)
{
  return in.r * in.r + in.i * in.i;
}

//------------------------------------------------------------------------------
double vtkFFT::HanningGenerator(std::size_t x, std::size_t size)
{
  return 0.5 * (1.0 - std::cos(2.0 * vtkMath::Pi() * x / (size - 1)));
}

//------------------------------------------------------------------------------
double vtkFFT::BartlettGenerator(std::size_t x, std::size_t size)
{
  return 2.0 * x / (size - 1);
}

//------------------------------------------------------------------------------
double vtkFFT::SineGenerator(std::size_t x, std::size_t size)
{
  return std::sin(vtkMath::Pi() * x / (size - 1));
}

//------------------------------------------------------------------------------
double vtkFFT::BlackmanGenerator(std::size_t x, std::size_t size)
{
  const double cosin = std::cos((2.0 * vtkMath::Pi() * x) / (size - 1));
  return 0.42 - 0.5 * cosin + 0.08 * (2.0 * cosin * cosin - 1.0);
}

//------------------------------------------------------------------------------
double vtkFFT::RectangularGenerator(std::size_t, std::size_t)
{
  return 1.0;
}

//------------------------------------------------------------------------------
template <typename T>
void vtkFFT::GenerateKernel1D(T* kernel, std::size_t n, WindowGenerator generator)
{
  const std::size_t half = (n / 2) + (n % 2);
  for (std::size_t i = 0; i < half; ++i)
  {
    kernel[i] = kernel[n - 1 - i] = generator(i, n);
  }
}

//------------------------------------------------------------------------------
template <typename T>
void vtkFFT::GenerateKernel2D(T* kernel, std::size_t n, std::size_t m, WindowGenerator generator)
{
  const std::size_t halfX = (n / 2) + (n % 2);
  const std::size_t halfY = (m / 2) + (m % 2);
  for (std::size_t i = 0; i < halfX; ++i)
  {
    for (std::size_t j = 0; j < halfY; ++j)
    {
      // clang-format off
      kernel[i][j]
      = kernel[n - 1 - i][j]
      = kernel[i][m - 1 - j]
      = kernel[n - 1 - i][m - 1 - j]
      = generator(i, n) * generator(j, m);
      // clang-format on
    }
  }
}

#endif
