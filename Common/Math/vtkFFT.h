// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 *
 * Some functions provides pointer-based version of themself in order to
 * prevent copying memory when possible.
 */

#ifndef vtkFFT_h
#define vtkFFT_h

#include "vtkAOSDataArrayTemplate.h" // vtkAOSDataArrayTemplate
#include "vtkCommonMathModule.h"     // export macro
#include "vtkDataArray.h"            // vtkDataArray
#include "vtkDataArrayRange.h"       // vtkDataArrayRange
#include "vtkMath.h"                 // vtkMath::Pi
#include "vtkObject.h"
#include "vtkSMPTools.h" // vtkSMPTools::Transform, vtkSMPTools::For

#include "vtk_kissfft.h" // kiss_fft_scalar, kiss_fft_cpx
// clang-format off
#include VTK_KISSFFT_HEADER(kiss_fft.h)
#include VTK_KISSFFT_HEADER(tools/kiss_fftr.h)
// clang-format on

#include <array>       // std::array
#include <cmath>       // std::sin, std::cos, std::sqrt
#include <numeric>     // std::accumulate, std::inner_product
#include <type_traits> // std::enable_if, std::iterator_traits
#include <vector>      // std::vector

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONMATH_EXPORT vtkFFT : public vtkObject
{
public:
  ///@{
  /**
   * Useful type definitions and utilities.
   *
   * ScalarNumber is defined as a floating point number.
   *
   * ComplexNumber is defined as a struct that contains two ScalarNumber.
   * These 2 numbers should be contiguous in memory.
   * First one should be named r and represent the real part,
   * while second one should be named i and represent the imaginary part.
   * This specification is important for the implementation of functions
   * accepting and returning values from vtkDataArrays as it allows to do
   * some zero-copy operations.
   *
   * A vtkScalarNumberArray is a data array with a layout memory compatible
   * with the underlying library for zero copy operations.
   *
   * isFftType is a trait to tell templates if Type is either ScalarNumber
   * or ComplexNumber.
   *
   * Common operators such as +,-,*,/ between ScalarNumber and
   * ComplexNumber are included in this header.
   */
  using ScalarNumber = kiss_fft_scalar;
  using ComplexNumber = kiss_fft_cpx;
  static_assert(sizeof(ComplexNumber) == 2 * sizeof(ScalarNumber),
    "vtkFFT::ComplexNumber definition is not valid");

  using vtkScalarNumberArray = vtkAOSDataArrayTemplate<vtkFFT::ScalarNumber>;
  template <typename T>
  struct isFftType : public std::false_type
  {
  };
  ///@}

  /**
   * Enum containing octave band numbers, named upon their nominal midband frequency.
   * Value multiplied by 3 should be a one-third-octave band number matching an octave band.
   */
  enum Octave
  {
    Hz_31_5 = 5,
    Hz_63 = 6,
    Hz_125 = 7,
    Hz_250 = 8,
    Hz_500 = 9,
    kHz_1 = 10,
    kHz_2 = 11,
    kHz_4 = 12,
    kHz_8 = 13,
    kHz_16 = 14
  };

  /**
   * Enum specifying which octave band we want to compute.
   * Could be a full octave range, or a one-third-octave one for instance.
   */
  enum OctaveSubdivision
  {
    Full,
    FirstHalf,
    SecondHalf,
    FirstThird,
    SecondThird,
    ThirdThird
  };

  ///@{
  /**
   * Compute the one-dimensional DFT for complex input.
   * If input is scalars then the imaginary part is set to 0
   *
   * input has n complex points
   * output has n complex points in case of success and empty in case of failure
   */
  static std::vector<ComplexNumber> Fft(const std::vector<ScalarNumber>& in);
  static void Fft(ScalarNumber* input, std::size_t size, ComplexNumber* result);
  static std::vector<ComplexNumber> Fft(const std::vector<ComplexNumber>& in);
  static void Fft(ComplexNumber* input, std::size_t size, ComplexNumber* result);
#ifndef __VTK_WRAP__
  static vtkSmartPointer<vtkScalarNumberArray> Fft(vtkScalarNumberArray* input);
#endif
  ///@}

  ///@{
  /**
   * Compute the one-dimensional DFT for real input
   *
   *  input has n scalar points
   *  output has (n/2) + 1 complex points in case of success and empty in case of failure
   */
  static std::vector<ComplexNumber> RFft(const std::vector<ScalarNumber>& in);
  static void RFft(ScalarNumber* input, std::size_t size, ComplexNumber* result);
#ifndef __VTK_WRAP__
  static vtkSmartPointer<vtkScalarNumberArray> RFft(vtkScalarNumberArray* input);
#endif
  ///@}

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
  static inline ScalarNumber Abs(const ComplexNumber& in);

  /**
   * Return the squared absolute value of the complex number
   */
  static inline ScalarNumber SquaredAbs(const ComplexNumber& in);

  /**
   * Return the conjugate of the given complex number
   */
  static inline ComplexNumber Conjugate(const ComplexNumber& in);

  /**
   * Return the DFT sample frequencies. Output has @c windowLength size.
   */
  static std::vector<ScalarNumber> FftFreq(int windowLength, double sampleSpacing);

  /**
   * Return the DFT sample frequencies for the real version of the dft (see @c Rfft).
   * Output has @c (windowLength / 2) + 1 size.
   */
  static std::vector<ScalarNumber> RFftFreq(int windowLength, double sampleSpacing);

  /**
   * Return lower and upper frequency from a octave band number / nominal midband frequency.
   * @param[in] octave octave band number associated to nominal midband frequency
   * @param[in] octaveSubdivision (optional) which subdivision of octave wanted (default: Full)
   * @param[in] baseTwo (optional) whether to compute it using base-2 or base-10 (default: base-2)
   * cf. "ANSI S1.11: Specification for Octave, Half-Octave, and Third Octave Band Filter Sets".
   */
  static std::array<double, 2> GetOctaveFrequencyRange(Octave octave,
    OctaveSubdivision octaveSubdivision = OctaveSubdivision::Full, bool baseTwo = true);

  ///@{
  /**
   * Compute consecutive Fourier transforms Welch method without averaging nor
   * scaling the result.
   *
   * @param[in] signal the input signal
   * @param[in] window the window to use per segment. Its size defines the size of FFT and thus the
   * size of the output.
   * @param[in] noverlap number of samples that will overlap between two segment
   * @param[in] detrend if true then each segment will be detrend using the mean value of the
   * segment before applying the FFT.
   * @param[in] onesided if true return a one-sided spectrum for real data. If input is copmlex then
   * this option will be ignored.
   * @param[out] shape if not @c nullptr, return the shape (n,m) of the result. `n` is the number of
   * segment and `m` the number of samples per segment.
   *
   * @return a 1D array that stores all resulting segment. For a shape (N,M), layout is
   * (segment0_sample0, segment0_sample1, ..., segment0_sampleM, segment1_sample0, ...
   * segmentN_sampleM)
   */
#ifndef __VTK_WRAP__
  template <typename T, typename TW, typename std::enable_if<isFftType<T>::value>::type* = nullptr>
  static std::vector<ComplexNumber> OverlappingFft(const std::vector<T>& signal,
    const std::vector<TW>& window, std::size_t noverlap, bool detrend, bool onesided,
    unsigned int* shape = nullptr);

  template <typename TW>
  static vtkFFT::ComplexNumber* OverlappingFft(vtkFFT::vtkScalarNumberArray* signal,
    const std::vector<TW>& window, std::size_t noverlap, bool detrend, bool onesided,
    unsigned int* shape = nullptr);
#endif
  ///@}

  /**
   * Scaling modes for Spectrogram and Csd functions.
   */
  enum Scaling : int
  {
    Density = 0, ///< Cross Spectral \b Density scaling (<b>V^2/Hz</b>)
    Spectrum     ///< Cross \b Spectrum scaling (<b>V^2</b>)
  };

  /**
   * Spectral modes for Spectrogram and Csd functions.
   */
  enum SpectralMode : int
  {
    STFT = 0, ///< Short-Time Fourier Transform, for local sections
    PSD       ///< Power Spectral Density
  };

  ///@{
  /**
   * Compute a spectrogram with consecutive Fourier transforms using Welch method.
   *
   * @param[in] signal the input signal
   * @param[in] window the window to use per segment. Its size defines the size of FFT and thus the
   * size of the output.
   * @param[in] sampleRate sample rate of the input signal
   * @param[in] noverlap number of samples that will overlap between two segment
   * @param[in] detrend if true then each segment will be detrend using the mean value of the
   * segment before applying the FFT.
   * @param[in] onesided if true return a one-sided spectrum for real data. If input is copmlex then
   * this option will be ignored.
   * @param[in] scaling can be either Cross Spectral \b Density (<b>V^2/Hz</b>) or Cross \b Spectrum
   * (<b>V^2</b>)
   * @param[in] mode determine which type of value ares returned. It is very dependent to how you
   * want to use the result afterwards.
   * @param[out] shape if not @c nullptr, return the shape (n,m) of the result. `n` is the number of
   * segment and `m` the number of samples per segment. Shape is inverted if `transpose` is true.
   * @param[in] transpose allows to transpose the resulting the resulting matrix into something of
   * shape (m, n)
   *
   * @return a 1D array that stores all resulting segment. For a shape (N,M), layout is
   * (segment0_sample0, segment0_sample1, ..., segment0_sampleM, segment1_sample0, ...
   * segmentN_sampleM)
   */
#ifndef __VTK_WRAP__
  template <typename T, typename TW, typename std::enable_if<isFftType<T>::value>::type* = nullptr>
  static std::vector<ComplexNumber> Spectrogram(const std::vector<T>& signal,
    const std::vector<TW>& window, double sampleRate, int noverlap, bool detrend, bool onesided,
    vtkFFT::Scaling scaling, vtkFFT::SpectralMode mode, unsigned int* shape = nullptr,
    bool transpose = false);

  template <typename TW>
  static vtkSmartPointer<vtkFFT::vtkScalarNumberArray> Spectrogram(
    vtkFFT::vtkScalarNumberArray* signal, const std::vector<TW>& window, double sampleRate,
    int noverlap, bool detrend, bool onesided, vtkFFT::Scaling scaling, vtkFFT::SpectralMode mode,
    unsigned int* shape = nullptr, bool transpose = false);
#endif
  ///@}

  ///@{
  /**
   * Compute the Cross Spectral Density of a given signal. This is the optimized version for
   * computing the csd of a single signal with itself. It uses Spectrogram behind the hood, and then
   * average all resulting segments of the spectrogram.
   *
   * @param[in] signal the input signal
   * @param[in] window the window to use per segment. Its size defines the size of FFT and thus the
   * size of the output.
   * @param[in] sampleRate sample rate of the input signal
   * @param[in] noverlap number of samples that will overlap between two segment
   * @param[in] detrend if true then each segment will be detrend using the mean value of the
   * segment before applying the FFT.
   * @param[in] onesided if true return a one-sided spectrum for real data. If input is copmlex then
   * this option will be ignored.
   * @param[in] scaling can be either Cross Spectral \b Density (<b>V^2/Hz</b>) or Cross \b Spectrum
   * (<b>V^2</b>)
   *
   * @return a 1D array containing the resulting cross spectral density or spectrum.
   *
   * See vtkFFT::Spectrogram
   */
#ifndef __VTK_WRAP__
  template <typename T, typename TW, typename std::enable_if<isFftType<T>::value>::type* = nullptr>
  static std::vector<vtkFFT::ScalarNumber> Csd(const std::vector<T>& signal,
    const std::vector<TW>& window, double sampleRate, int noverlap, bool detrend, bool onesided,
    vtkFFT::Scaling scaling);

  template <typename TW>
  static vtkSmartPointer<vtkFFT::vtkScalarNumberArray> Csd(vtkScalarNumberArray* signal,
    const std::vector<TW>& window, double sampleRate, int noverlap, bool detrend, bool onesided,
    vtkFFT::Scaling scaling);
#endif
  ///@}

  /**
   * Transpose in place an inlined 2D matrix. This algorithm is not optimized
   * for square matrices but is generic. This will also effectively swap shape values.
   * Worst case complexity is : O( (shape[0]*shape[1])^3/2 )
   *
   * XXX: some fft libraries such as FFTW already propose functions to do that.
   * This should be taken into account if the backend is changed at some point.
   *
   * XXX: An optimized version could be implemented for square matrices
   */
#ifndef __VTK_WRAP__
  template <typename T>
  static void Transpose(T* data, unsigned int* shape);
#endif

  ///@{
  /**
   * Window generator functions. Implementation only needs to be valid for x E [0; size / 2]
   * because kernels are symmetric by definitions. This point is very important for some
   * kernels like Bartlett for example.
   *
   * @warning Most generators need size > 1 !
   *
   * Can be used with @c GenerateKernel1D and @c GenerateKernel2D for generating full kernels.
   */
  using WindowGenerator = ScalarNumber (*)(std::size_t, std::size_t);

  static inline ScalarNumber HanningGenerator(std::size_t x, std::size_t size);
  static inline ScalarNumber BartlettGenerator(std::size_t x, std::size_t size);
  static inline ScalarNumber SineGenerator(std::size_t x, std::size_t size);
  static inline ScalarNumber BlackmanGenerator(std::size_t x, std::size_t size);
  static inline ScalarNumber RectangularGenerator(std::size_t x, std::size_t size);
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

  static vtkFFT* New();
  vtkTypeMacro(vtkFFT, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkFFT() = default;
  ~vtkFFT() override = default;

  /**
   * Templated zero value, specialized for vtkFFT::ComplexNumber
   */
  template <typename T>
  constexpr static T Zero();

#ifndef __VTK_WRAP__
  /**
   * For a given window defined by @c begin and @c end, compute the scaling needed to apply
   * to the resulting FFT. Used in the `Spectrogram` function.
   */
  template <typename InputIt>
  static typename std::iterator_traits<InputIt>::value_type ComputeScaling(
    InputIt begin, InputIt end, Scaling scaling, double fs);

  /**
   * Dispatch the signal to the right FFT function according to the given parameters.
   * Also detrend the signal and multiply it by the window. Used in the `OverlappingFft` function.
   */
  template <typename T, typename TW>
  static void PreprocessAndDispatchFft(const T* segment, const std::vector<TW>& window,
    bool detrend, bool onesided, vtkFFT::ComplexNumber* result);

  /**
   * XXX(c++17): This function should NOT exist and is here just for the sake template unfolding
   * purposes. As long we don't have `constexrp if` this is the easier way to deal with it.
   *
   * @warning this function will always throw an error
   *
   * @see PreprocessAndDispatchFft
   */
  static void RFft(ComplexNumber* input, std::size_t size, ComplexNumber* result);

  /**
   * Scale a fft according to its window and some mode. Used in the `Spectrogram` function.
   */
  template <typename TW>
  static void ScaleFft(ComplexNumber* fft, unsigned int shape[2], const std::vector<TW>& window,
    double sampleRate, bool onesided, vtkFFT::Scaling scaling, vtkFFT::SpectralMode mode);
#endif

private:
  vtkFFT(const vtkFFT&) = delete;
  void operator=(const vtkFFT&) = delete;
};

//------------------------------------------------------------------------------
template <>
struct vtkFFT::isFftType<vtkFFT::ScalarNumber> : public std::true_type
{
};
template <>
struct vtkFFT::isFftType<vtkFFT::ComplexNumber> : public std::true_type
{
};

//------------------------------------------------------------------------------
template <typename T>
constexpr T vtkFFT::Zero()
{
  return static_cast<T>(0);
}
template <>
constexpr vtkFFT::ComplexNumber vtkFFT::Zero()
{
  return vtkFFT::ComplexNumber{ 0.0, 0.0 };
}

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
vtkFFT::ScalarNumber vtkFFT::Abs(const ComplexNumber& in)
{
  return std::sqrt(in.r * in.r + in.i * in.i);
}

//------------------------------------------------------------------------------
vtkFFT::ScalarNumber vtkFFT::SquaredAbs(const ComplexNumber& in)
{
  return in.r * in.r + in.i * in.i;
}

//------------------------------------------------------------------------------
vtkFFT::ComplexNumber vtkFFT::Conjugate(const ComplexNumber& in)
{
  return ComplexNumber{ in.r, -in.i };
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
  std::size_t half = (n / 2) + (n % 2);
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

VTK_ABI_NAMESPACE_END

#include "vtkFFT.txx" // complex templated functions not wrapped by python

#endif // vtkFFT_h
