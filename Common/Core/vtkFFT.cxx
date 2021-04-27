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

#include "vtkMath.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFT);

//------------------------------------------------------------------------------
std::vector<vtkFFT::ComplexNumber> vtkFFT::FftDirect(const std::vector<ScalarNumber>& in)
{
  kiss_fftr_cfg cfg = kiss_fftr_alloc(in.size(), 0 /*is_inverse_fft*/, nullptr, nullptr);
  if (cfg != nullptr)
  {
    auto outSize = (in.size() / 2) + 1;
    std::vector<vtkFFT::ComplexNumber> result(outSize);

    kiss_fftr(cfg, in.data(), &result[0]);
    kiss_fftr_free(cfg);

    return result;
  }
  return {};
}

//------------------------------------------------------------------------------
std::vector<vtkFFT::ScalarNumber> vtkFFT::FftInverse(const std::vector<vtkFFT::ComplexNumber>& in)
{
  auto outSize = (in.size() - 1) * 2;
  kiss_fftr_cfg cfg = kiss_fftr_alloc(outSize, 1 /*is_inverse_fft*/, nullptr, nullptr);
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
double vtkFFT::Abs(const ComplexNumber& in)
{
  return sqrt(in.r * in.r + in.i * in.i);
}

//------------------------------------------------------------------------------
std::vector<double> vtkFFT::RFftFreq(int windowLength, double sampleSpacing)
{
  double val = 1.0 / (windowLength * sampleSpacing);
  int N = (windowLength + 1) / 2;
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
