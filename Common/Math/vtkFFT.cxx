// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFFT.h"

#include "vtkDataArrayRange.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"

#include <stdexcept>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkFFT);

//------------------------------------------------------------------------------
std::vector<vtkFFT::ComplexNumber> vtkFFT::Fft(const std::vector<ComplexNumber>& in)
{
  if (in.size() <= 1)
  {
    return {};
  }

  kiss_fft_cfg cfg =
    kiss_fft_alloc(static_cast<int>(in.size()), 0 /*is_inverse_fft*/, nullptr, nullptr);
  if (cfg != nullptr)
  {
    std::vector<vtkFFT::ComplexNumber> result(in.size());

    kiss_fft(cfg, in.data(), result.data());
    kiss_fft_free(cfg);

    return result;
  }
  return {};
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ComplexNumber> vtkFFT::Fft(const std::vector<ScalarNumber>& in)
{
  std::vector<ComplexNumber> cplx(in.size());
  std::transform(in.begin(), in.end(), cplx.begin(),
    [](ScalarNumber x) {
      return ComplexNumber{ x, 0 };
    });
  return vtkFFT::Fft(cplx);
}

//------------------------------------------------------------------------------
void vtkFFT::Fft(ScalarNumber* input, std::size_t size, ComplexNumber* result)
{
  std::vector<ComplexNumber> cplx(size);
  std::transform(input, input + size, cplx.begin(),
    [](ScalarNumber x) {
      return ComplexNumber{ x, 0 };
    });
  vtkFFT::Fft(cplx.data(), cplx.size(), result);
}

//------------------------------------------------------------------------------
void vtkFFT::Fft(ComplexNumber* input, std::size_t size, ComplexNumber* result)
{
  if (size <= 1)
  {
    return;
  }

  kiss_fft_cfg cfg = kiss_fft_alloc(static_cast<int>(size), 0 /*is_inverse_fft*/, nullptr, nullptr);
  if (cfg != nullptr)
  {
    kiss_fft(cfg, input, result);
    kiss_fft_free(cfg);
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkFFT::vtkScalarNumberArray> vtkFFT::Fft(vtkScalarNumberArray* input)
{
  // This function may look a bit unsafe but here's the reasoning : ComplexNumber
  // should be a simple a struct containing only 2 ScalarNumber. Since vtkScalarNumberArray
  // is defined as a Array of Structure of ScalarNumber, the memory layout should be the
  // exact same even though the underlying class is not. So setting the array and
  // getting the void pointer should be safe on most architecture.

  if (input->GetNumberOfComponents() > 2)
  {
    return vtkSmartPointer<vtkScalarNumberArray>::New();
  }

  ComplexNumber* rawInput = static_cast<ComplexNumber*>(input->GetVoidPointer(0));
  const std::size_t size = static_cast<std::size_t>(input->GetNumberOfTuples());
  if (input->GetNumberOfComponents() == 1)
  {
    rawInput = new ComplexNumber[size];
    auto inputRange = vtk::DataArrayValueRange<1>(input);
    using ValueT = decltype(inputRange)::ConstReferenceType;
    vtkSMPTools::Transform(inputRange.begin(), inputRange.end(), rawInput,
      [](ValueT val) {
        return vtkFFT::ComplexNumber{ val, 0.0 };
      });
  }

  auto result = vtkSmartPointer<vtkScalarNumberArray>::New();

  ComplexNumber* rawResult = new ComplexNumber[size];
  vtkFFT::Fft(rawInput, size, rawResult);

  result->SetNumberOfComponents(2);
  result->SetArray(&rawResult[0].r, size * 2, 0, vtkScalarNumberArray::VTK_DATA_ARRAY_DELETE);

  if (input->GetNumberOfComponents() == 1)
  {
    delete[] rawInput;
  }

  return result;
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ComplexNumber> vtkFFT::RFft(const std::vector<ScalarNumber>& in)
{
  if (in.size() <= 1)
  {
    return {};
  }

  const std::size_t outSize = (in.size() / 2) + 1;

  // Real fft optimization needs an input with even size. Falling back to vtkFFT::Fft() if odd sized
  // input
  if ((in.size() % 2) == 1)
  {
    const auto& res = vtkFFT::Fft(in);
    return std::vector<ComplexNumber>(res.begin(), res.begin() + outSize);
  }

  kiss_fftr_cfg cfg =
    kiss_fftr_alloc(static_cast<int>(in.size()), 0 /*is_inverse_fft*/, nullptr, nullptr);
  if (cfg != nullptr)
  {
    std::vector<vtkFFT::ComplexNumber> result(outSize);

    kiss_fftr(cfg, in.data(), result.data());
    kiss_fftr_free(cfg);

    return result;
  }
  return {};
}

//------------------------------------------------------------------------------
void vtkFFT::RFft(ScalarNumber* input, std::size_t size, ComplexNumber* result)
{
  if (size <= 1)
  {
    return;
  }

  // Real fft optimization needs an input with even size.
  // Falling back to vtkFFT::Fft() if input size is odd.
  if ((size % 2) != 0)
  {
    std::vector<ComplexNumber> twoSided(size);
    vtkFFT::Fft(input, size, twoSided.data());
    std::copy(twoSided.data(), twoSided.data() + 1 + size / 2, result);
  }
  else
  {
    kiss_fftr_cfg cfg =
      kiss_fftr_alloc(static_cast<int>(size), 0 /*is_inverse_fft*/, nullptr, nullptr);
    if (cfg != nullptr)
    {
      kiss_fftr(cfg, input, result);
      kiss_fft_free(cfg);
    }
  }
}

//------------------------------------------------------------------------------
void vtkFFT::RFft(ComplexNumber*, std::size_t, ComplexNumber*)
{
  throw std::domain_error("vtkFFT::RFft does not accept complex numbers as its input.");
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkFFT::vtkScalarNumberArray> vtkFFT::RFft(vtkScalarNumberArray* input)
{
  if (input->GetNumberOfComponents() != 1)
  {
    return nullptr;
  }

  // See vtkFFT::FFT(vtkScalarNumberArray*) for reasoning behind why this is safe
  // to do like so
  const std::size_t size = static_cast<std::size_t>(input->GetNumberOfTuples());
  const std::size_t outSize = size / 2 + 1;
  ScalarNumber* rawInput = static_cast<ScalarNumber*>(input->GetVoidPointer(0));
  ComplexNumber* rawResult = new ComplexNumber[outSize];
  vtkFFT::RFft(rawInput, size, rawResult);

  auto result = vtkSmartPointer<vtkScalarNumberArray>::New();
  result->SetNumberOfComponents(2);
  result->SetArray(&rawResult[0].r, outSize * 2, 0, vtkScalarNumberArray::VTK_DATA_ARRAY_DELETE);
  return result;
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ComplexNumber> vtkFFT::IFft(const std::vector<vtkFFT::ComplexNumber>& in)
{
  if (in.empty())
  {
    return {};
  }

  std::size_t outSize = in.size();
  kiss_fft_cfg cfg =
    kiss_fft_alloc(static_cast<int>(outSize), 1 /*is_inverse_fft*/, nullptr, nullptr);
  if (cfg != nullptr)
  {
    std::vector<vtkFFT::ComplexNumber> result(outSize);

    kiss_fft(cfg, in.data(), result.data());
    std::for_each(result.begin(), result.end(),
      [outSize](vtkFFT::ComplexNumber& x) {
        x = vtkFFT::ComplexNumber{ x.r / outSize, x.i / outSize };
      });
    kiss_fft_free(cfg);

    return result;
  }
  return {};
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ScalarNumber> vtkFFT::IRFft(const std::vector<vtkFFT::ComplexNumber>& in)
{
  if (in.size() < 2)
  {
    return {};
  }

  std::size_t outSize = (in.size() - 1) * 2;
  kiss_fftr_cfg cfg =
    kiss_fftr_alloc(static_cast<int>(outSize), 1 /*is_inverse_fft*/, nullptr, nullptr);
  if (cfg != nullptr)
  {
    std::vector<vtkFFT::ScalarNumber> result(outSize);

    kiss_fftri(cfg, in.data(), result.data());
    std::for_each(result.begin(), result.end(),
      [outSize](vtkFFT::ScalarNumber& num) { num /= static_cast<vtkFFT::ScalarNumber>(outSize); });
    kiss_fftr_free(cfg);

    return result;
  }
  return {};
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ScalarNumber> vtkFFT::FftFreq(int windowLength, double sampleSpacing)
{
  if (windowLength < 1)
  {
    return {};
  }

  double freq = 1.0 / (windowLength * sampleSpacing);
  int nshan = (windowLength / 2) + 1;
  std::vector<vtkFFT::ScalarNumber> res(windowLength);

  res[0] = 0.0;
  for (int i = 1; i < nshan; i++)
  {
    const vtkFFT::ScalarNumber val = i * freq;
    res[i] = val;
    res[windowLength - i] = -val;
  }

  return res;
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ScalarNumber> vtkFFT::RFftFreq(int windowLength, double sampleSpacing)
{
  if (windowLength < 1)
  {
    return {};
  }

  const vtkFFT::ScalarNumber val = 1.0 / (windowLength * sampleSpacing);
  const int size = (windowLength / 2) + 1;

  std::vector<double> res(size);
  for (int i = 0; i < size; i++)
  {
    res[i] = i * val;
  }

  return res;
}

std::array<double, 2> vtkFFT::GetOctaveFrequencyRange(
  Octave octave, OctaveSubdivision octaveSubdivision, bool baseTwo)
{
  int bandNumber = static_cast<int>(octave) * 3; // Use one-third-octave band number
  double factor = 0.0;
  if (octaveSubdivision == OctaveSubdivision::FirstThird ||
    octaveSubdivision == OctaveSubdivision::SecondThird ||
    octaveSubdivision == OctaveSubdivision::ThirdThird)
  {
    factor = baseTwo ? std::pow(2.0, 1.0 / 6.0) : std::pow(10.0, 0.05);
    if (octaveSubdivision == OctaveSubdivision::FirstThird)
    {
      --bandNumber;
    }
    if (octaveSubdivision == OctaveSubdivision::ThirdThird)
    {
      ++bandNumber;
    }
  }
  else
  {
    factor = baseTwo ? std::sqrt(2.0) : std::pow(10.0, 0.15);
  }

  const double midbandFrequency =
    baseTwo ? 1000.0 * std::pow(2.0, (bandNumber - 30) / 3.0) : std::pow(10.0, bandNumber / 10.0);
  double lowerFrequency = midbandFrequency / factor;
  double upperFrequency = midbandFrequency * factor;

  if (octaveSubdivision == OctaveSubdivision::FirstHalf)
  {
    upperFrequency = midbandFrequency;
  }
  else if (octaveSubdivision == OctaveSubdivision::SecondHalf)
  {
    lowerFrequency = midbandFrequency;
  }

  return { lowerFrequency, upperFrequency };
}

//------------------------------------------------------------------------------
void vtkFFT::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
