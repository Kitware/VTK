// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkFFT_txx
#define vtkFFT_txx

#include "vtkFFT.h"

#ifndef __VTK_WRAP__

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
template <typename InputIt>
typename std::iterator_traits<InputIt>::value_type vtkFFT::ComputeScaling(
  InputIt begin, InputIt end, Scaling scaling, double fs)
{
  using T = typename std::iterator_traits<InputIt>::value_type;
  constexpr T zero = vtkFFT::Zero<T>();
  if (scaling == Scaling::Density)
  {
    return 1.0 /
      (fs * std::inner_product(begin, end, begin, zero, std::plus<T>(), std::multiplies<T>()));
  }
  else
  {
    const T sum = std::accumulate(begin, end, zero);
    assert("Window should not be null" && sum != zero);
    return 1.0 / std::pow(sum, 2.0);
  }
}

//------------------------------------------------------------------------------
template <typename T, typename TW>
void vtkFFT::PreprocessAndDispatchFft(const T* signal, const std::vector<TW>& window, bool detrend,
  bool onesided, vtkFFT::ComplexNumber* result)
{
  assert("pre: window should not be empty or of size 1" && window.size() > 1);

  std::vector<T> segment(window.size());
  const T mean = detrend
    ? std::accumulate(signal, signal + window.size(), vtkFFT::Zero<T>()) / window.size()
    : vtkFFT::Zero<T>();
  std::transform(signal, signal + window.size(), window.cbegin(), segment.begin(),
    [mean](T val, TW win) { return (val - mean) * win; });

  if (onesided)
  {
    vtkFFT::RFft(segment.data(), segment.size(), result);
  }
  else
  {
    vtkFFT::Fft(segment.data(), segment.size(), result);
  }
}

//------------------------------------------------------------------------------
template <typename T, typename TW, typename std::enable_if<vtkFFT::isFftType<T>::value>::type*>
std::vector<vtkFFT::ComplexNumber> vtkFFT::OverlappingFft(const std::vector<T>& sig,
  const std::vector<TW>& window, std::size_t noverlap, bool detrend, bool onesided,
  unsigned int* shape)
{
  const std::size_t segmentOffset = window.size() - noverlap;
  const std::size_t nsegment = (sig.size() - noverlap) / segmentOffset;
  const std::size_t nfft = onesided ? window.size() / 2 + 1 : window.size();
  if (shape != nullptr)
  {
    shape[0] = static_cast<unsigned int>(nsegment);
    shape[1] = static_cast<unsigned int>(nfft);
  }

  std::vector<vtkFFT::ComplexNumber> result(nsegment * nfft);
  vtkSMPTools::For(0, nsegment,
    [&](std::size_t begin, std::size_t end)
    {
      for (std::size_t i = begin; i < end; ++i)
      {
        vtkFFT::PreprocessAndDispatchFft(
          sig.data() + i * segmentOffset, window, detrend, onesided, result.data() + i * nfft);
      }
    });

  return result;
}

//------------------------------------------------------------------------------
template <typename TW>
vtkFFT::ComplexNumber* vtkFFT::OverlappingFft(vtkFFT::vtkScalarNumberArray* signal,
  const std::vector<TW>& window, std::size_t noverlap, bool detrend, bool onesided,
  unsigned int* shape)
{
  const std::size_t segmentOffset = window.size() - noverlap;
  const std::size_t nsegment = (signal->GetNumberOfTuples() - noverlap) / segmentOffset;
  const std::size_t nfft = onesided ? window.size() / 2 + 1 : window.size();
  const std::size_t outSize = nsegment * nfft;
  if (shape != nullptr)
  {
    shape[0] = static_cast<unsigned int>(nsegment);
    shape[1] = static_cast<unsigned int>(nfft);
  }

  vtkFFT::ComplexNumber* result = new vtkFFT::ComplexNumber[outSize];

  vtkSMPTools::For(0, nsegment,
    [&](std::size_t begin, std::size_t end)
    {
      for (std::size_t i = begin; i < end; ++i)
      {
        if (signal->GetNumberOfComponents() == 1)
        {
          auto* beginSegment =
            reinterpret_cast<ScalarNumber*>(signal->GetVoidPointer(i * segmentOffset));
          PreprocessAndDispatchFft(beginSegment, window, detrend, onesided, result + i * nfft);
        }
        else //: signal->GetNumberOfComponents() == 2
        {
          auto* beginSegment =
            reinterpret_cast<ComplexNumber*>(signal->GetVoidPointer(i * segmentOffset * 2));
          PreprocessAndDispatchFft(beginSegment, window, detrend, onesided, result + i * nfft);
        }
      }
    });

  return result;
}

//------------------------------------------------------------------------------
template <typename TW>
void vtkFFT::ScaleFft(ComplexNumber* fft, unsigned int shape[2], const std::vector<TW>& window,
  double sampleRate, bool onesided, vtkFFT::Scaling scaling, vtkFFT::SpectralMode mode)
{
  double scale = vtkFFT::ComputeScaling(window.begin(), window.end(), scaling, sampleRate);
  if (mode == vtkFFT::SpectralMode::STFT)
  {
    scale = std::sqrt(scale);
  }
  else if (mode == vtkFFT::SpectralMode::PSD && onesided)
  {
    scale *= 2.0;
  }
  const unsigned int totalSize = shape[0] * shape[1];

  // Now we scale the result according to the user parameters
  if (mode == vtkFFT::SpectralMode::PSD)
  {
    vtkSMPTools::Transform(fft, fft + totalSize, fft,
      [scale](ComplexNumber val) { return vtkFFT::Conjugate(val) * val * scale; });

    // We actually don't want to scale by 2.0 the first and nyquist-th (if any) values of each
    // segment when onesided is on.
    if (onesided)
    {
      for (unsigned int i = 0; i < shape[0]; ++i)
      {
        const unsigned int idx = i * shape[1];
        fft[idx] = fft[idx] * 0.5;
        if (window.size() % 2 == 0)
        {
          fft[idx + shape[1] - 1] = fft[idx + shape[1] - 1] * 0.5;
        }
      }
    }
  }
  else if (mode == vtkFFT::SpectralMode::STFT)
  {
    vtkSMPTools::Transform(
      fft, fft + totalSize, fft, [scale](ComplexNumber val) { return val * scale; });
  }
}

//------------------------------------------------------------------------------
template <typename T, typename TW, typename std::enable_if<vtkFFT::isFftType<T>::value>::type*>
std::vector<vtkFFT::ComplexNumber> vtkFFT::Spectrogram(const std::vector<T>& sig,
  const std::vector<TW>& window, double sampleRate, int noverlap, bool detrend, bool onesided,
  vtkFFT::Scaling scaling, vtkFFT::SpectralMode mode, unsigned int* shape, bool transpose)
{
  if (sig.size() <= 1 || window.size() <= 1 || window.size() > sig.size())
  {
    vtkGenericWarningMacro("vtkFFT::Spectrogram -> Invalid input shape, aborting.");
    return {};
  }

  // Compute all parameters
  // Result cannot be onesided if input is complex
  onesided = onesided && !std::is_same<T, vtkFFT::ComplexNumber>::value;
  if (noverlap < 0 || noverlap >= static_cast<int>(window.size()))
  {
    noverlap = static_cast<int>(window.size()) / 2;
  }

  // OverlappingFft returns a 2D matrix of FFTs but inlined in 1D
  std::vector<ComplexNumber> result =
    vtkFFT::OverlappingFft(sig, window, noverlap, detrend, onesided, shape);

  vtkFFT::ScaleFft(result.data(), shape, window, sampleRate, onesided, scaling, mode);

  if (transpose)
  {
    vtkFFT::Transpose(result.data(), shape);
  }

  return result;
}

//------------------------------------------------------------------------------
template <typename TW>
vtkSmartPointer<vtkFFT::vtkScalarNumberArray> vtkFFT::Spectrogram(vtkScalarNumberArray* signal,
  const std::vector<TW>& window, double sampleRate, int noverlap, bool detrend, bool onesided,
  Scaling scaling, SpectralMode mode, unsigned int* shape, bool transpose)
{
  auto sig = vtk::DataArrayTupleRange(signal);
  if (sig.size() <= 1 || window.size() <= 1 ||
    window.size() > static_cast<std::size_t>(sig.size()) || sig.GetTupleSize() > 2)
  {
    vtkGenericWarningMacro("vtkFFT::Spectrogram -> Invalid input shape, aborting.");
    return {};
  }

  // Compute all parameters
  // Result cannot be onesided if input is complex
  onesided = onesided && sig.GetTupleSize() == 1;
  if (noverlap < 0 || noverlap >= static_cast<int>(window.size()))
  {
    noverlap = static_cast<int>(window.size()) / 2;
  }

  // OverlappingFft returns a 2D matrix of FFTs but inlined in 1D
  vtkFFT::ComplexNumber* result =
    vtkFFT::OverlappingFft(signal, window, noverlap, detrend, onesided, shape);

  vtkFFT::ScaleFft(result, shape, window, sampleRate, onesided, scaling, mode);

  if (transpose)
  {
    vtkFFT::Transpose(result, shape);
  }

  auto resultArray = vtkSmartPointer<vtkFFT::vtkScalarNumberArray>::New();
  resultArray->SetNumberOfComponents(2);
  resultArray->SetArray(
    &result[0].r, shape[0] * shape[1] * 2, 0, vtkFFT::vtkScalarNumberArray::VTK_DATA_ARRAY_DELETE);

  return resultArray;
}

//------------------------------------------------------------------------------
template <typename T>
void vtkFFT::Transpose(T* data, unsigned int* shape)
{
  // Algorithm has been adapted from this StackOverflow answer:
  // https://stackoverflow.com/a/9320349/9865192
  const unsigned int size = shape[0] * shape[1];
  const unsigned int mn1 = size - 1;
  std::vector<bool> visited(size, false);
  for (unsigned int cycle = 0; cycle < size; cycle++)
  {
    if (visited[cycle])
    {
      continue;
    }

    unsigned int current = cycle;
    do
    {
      current = (current == mn1) ? mn1 : (shape[0] * current) % mn1;
      std::swap(data[current], data[cycle]);
      visited[current] = true;
    } while (current != cycle);
  }
  std::swap(shape[0], shape[1]);
}

//------------------------------------------------------------------------------
template <typename T, typename TW, typename std::enable_if<vtkFFT::isFftType<T>::value>::type*>
std::vector<vtkFFT::ScalarNumber> vtkFFT::Csd(const std::vector<T>& sig,
  const std::vector<TW>& window, double sampleRate, int noverlap, bool detrend, bool onesided,
  vtkFFT::Scaling scaling)
{
  if (sig.size() <= 1 || window.size() <= 1 || window.size() > sig.size())
  {
    vtkGenericWarningMacro("vtkFFT::Csd -> Invalid input shape, aborting.");
    return {};
  }

  // Spectrogram returns a 2D matrix of FFTs but inlined in 1D
  unsigned int shape[2];
  std::vector<vtkFFT::ComplexNumber> result = vtkFFT::Spectrogram(sig, window, sampleRate, noverlap,
    detrend, onesided, scaling, vtkFFT::SpectralMode::PSD, shape);

  const double meanFactor = 1.0 / shape[0];
  std::vector<vtkFFT::ScalarNumber> average(shape[1], 0);
  for (unsigned int i = 0; i < shape[0]; ++i)
  {
    auto begin = result.cbegin() + i * shape[1];
    auto end = begin + shape[1];
    vtkSMPTools::Transform(begin, end, average.cbegin(), average.begin(),
      [meanFactor](vtkFFT::ComplexNumber x, vtkFFT::ScalarNumber y)
      { return vtkFFT::Abs(x) * meanFactor + y; });
  }

  return average;
}

//------------------------------------------------------------------------------
template <typename TW>
vtkSmartPointer<vtkFFT::vtkScalarNumberArray> vtkFFT::Csd(vtkScalarNumberArray* signal,
  const std::vector<TW>& window, double sampleRate, int noverlap, bool detrend, bool onesided,
  vtkFFT::Scaling scaling)
{
  if (signal->GetNumberOfTuples() <= 1 || window.size() <= 1 ||
    window.size() > static_cast<std::size_t>(signal->GetNumberOfTuples()) ||
    signal->GetNumberOfComponents() > 2)
  {
    vtkGenericWarningMacro("vtkFFT::Csd -> Invalid input shape, aborting.");
    return {};
  }

  // Spectrogram returns a 2D matrix of FFTs but inlined in 1D
  unsigned int shape[2];
  vtkSmartPointer<vtkFFT::vtkScalarNumberArray> result = vtkFFT::Spectrogram(signal, window,
    sampleRate, noverlap, detrend, onesided, scaling, vtkFFT::SpectralMode::PSD, shape);

  const double meanFactor = 1.0 / shape[0];
  auto average = vtkSmartPointer<vtkFFT::vtkScalarNumberArray>::New();
  average->SetNumberOfComponents(1);
  average->SetNumberOfValues(shape[1]);
  auto averageRange = vtk::DataArrayValueRange<1>(average);
  vtkSMPTools::Fill(averageRange.begin(), averageRange.end(), 0.0);

  auto resRange = vtk::DataArrayTupleRange(result);
  using ConstTupleRef = typename decltype(resRange)::ConstTupleReferenceType;
  for (unsigned int i = 0; i < shape[0]; ++i)
  {
    auto begin = resRange.cbegin() + i * shape[1];
    auto end = begin + shape[1];
    vtkSMPTools::Transform(begin, end, averageRange.cbegin(), averageRange.begin(),
      [meanFactor](ConstTupleRef x, vtkFFT::ScalarNumber y) {
        return vtkFFT::Abs(ComplexNumber{ x[0], x[1] }) * meanFactor + y;
      });
  }

  return average;
}

VTK_ABI_NAMESPACE_END
#endif // __VTK_WRAP__
#endif // vtkFFT_txx
