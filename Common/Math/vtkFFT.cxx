/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFFT.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFT);

//------------------------------------------------------------------------------
std::vector<vtkFFT::ComplexNumber> vtkFFT::Fft(const std::vector<ComplexNumber>& in)
{
  if (in.empty())
  {
    return {};
  }

  kiss_fft_cfg cfg =
    kiss_fft_alloc(static_cast<int>(in.size()), 0 /*is_inverse_fft*/, nullptr, nullptr);
  if (cfg != nullptr)
  {
    std::vector<vtkFFT::ComplexNumber> result(in.size());

    kiss_fft(cfg, in.data(), &result[0]);
    kiss_fft_free(cfg);

    return result;
  }
  return {};
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ComplexNumber> vtkFFT::Fft(const std::vector<ScalarNumber>& in)
{
  std::vector<ComplexNumber> cplx(in.size());
  std::transform(in.begin(), in.end(), cplx.begin(), [](const ScalarNumber& x) {
    return ComplexNumber{ x, 0 };
  });
  return vtkFFT::Fft(cplx);
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ComplexNumber> vtkFFT::RFft(const std::vector<ScalarNumber>& in)
{
  if (in.empty())
  {
    return {};
  }

  std::size_t outSize = (in.size() / 2) + 1;

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

    kiss_fftr(cfg, in.data(), &result[0]);
    kiss_fftr_free(cfg);

    return result;
  }
  return {};
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

    kiss_fft(cfg, in.data(), &result[0]);
    std::for_each(result.begin(), result.end(), [outSize](vtkFFT::ComplexNumber& x) {
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

    kiss_fftri(cfg, in.data(), &result[0]);
    std::for_each(result.begin(), result.end(),
      [outSize](vtkFFT::ScalarNumber& num) { num /= static_cast<vtkFFT::ScalarNumber>(outSize); });
    kiss_fftr_free(cfg);

    return result;
  }
  return {};
}

//------------------------------------------------------------------------------
std::vector<double> vtkFFT::FftFreq(int windowLength, double sampleSpacing)
{
  if (windowLength < 1)
  {
    return {};
  }

  double freq = 1.0 / (windowLength * sampleSpacing);
  int nshan = (windowLength / 2) + 1;
  double val;
  std::vector<double> res(windowLength);

  res[0] = 0.0;
  for (int i = 1; i < nshan; i++)
  {
    val = i * freq;
    res[i] = val;
    res[windowLength - i] = -val;
  }

  return res;
}

//------------------------------------------------------------------------------
std::vector<double> vtkFFT::RFftFreq(int windowLength, double sampleSpacing)
{
  if (windowLength < 1)
  {
    return {};
  }

  double val = 1.0 / (windowLength * sampleSpacing);
  int N = (windowLength / 2) + 1;
  std::vector<double> res(N);
  for (auto i = 0; i < N; i++)
  {
    res[i] = i * val;
  }

  return res;
}

//------------------------------------------------------------------------------
void vtkFFT::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
