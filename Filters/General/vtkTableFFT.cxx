// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableFFT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkTableFFT.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFFT.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTimeStamp.h"

#include <array>
#include <cstring>
#include <functional>
#include <iterator>
#include <numeric>

#include <vtksys/SystemTools.hxx>

namespace details
{
// Compute the window energy of a given kernel for ponderation.
template <typename InputIt>
typename std::iterator_traits<InputIt>::value_type WindowEnergy(InputIt begin, InputIt end)
{
  using T = typename std::iterator_traits<InputIt>::value_type;
  constexpr T zero(0);
  return std::inner_product(begin, end, begin, zero, std::plus<T>(), std::multiplies<T>()) /
    std::distance(begin, end);
}

// Easy access to the right windowing function using vtkTableFFT enumeration.
// clang-format off
constexpr static std::array<vtkFFT::WindowGenerator, vtkTableFFT::MAX_WINDOWING_FUNCTION> WindowingFunctionsList =
{
  vtkFFT::HanningGenerator,
  vtkFFT::BartlettGenerator,
  vtkFFT::SineGenerator,
  vtkFFT::BlackmanGenerator,
  vtkFFT::RectangularGenerator
};
// clang-format on
}

//------------------------------------------------------------------------------
struct vtkTableFFT::vtkInternal
{
  std::vector<double> Window = {};
  double WindowPonderation = 0.0;
  vtkTimeStamp WindowTimeStamp;
  vtkMTimeType WindowLastUpdated = 0;

  double SampleRate = 1.0e4;
  vtkIdType OutputSize = 0;

  bool Average = false;

  void UpdateWindow(int window, int size)
  {
    this->Window.resize(size);

    vtkFFT::GenerateKernel1D(
      this->Window.data(), this->Window.size(), details::WindowingFunctionsList[window]);
    this->WindowPonderation = (window == vtkTableFFT::RECTANGULAR)
      ? 1.0
      : details::WindowEnergy(this->Window.begin(), this->Window.end());
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTableFFT);

//------------------------------------------------------------------------------
vtkTableFFT::vtkTableFFT()
  : Internals(new vtkInternal)
{
  this->Internals->WindowTimeStamp.Modified();
}

//------------------------------------------------------------------------------
vtkTableFFT::~vtkTableFFT()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
int vtkTableFFT::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkTable* input = vtkTable::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector);

  if (!input || !output)
  {
    vtkWarningMacro(<< "No input or output.");
    return 0;
  }
  if (input->GetNumberOfRows() == 0)
  {
    // Input is empty, nothing to do
    return 1;
  }

  this->Initialize(input);

  vtkIdType numColumns = input->GetNumberOfColumns();
  for (vtkIdType col = 0; col < numColumns; col++)
  {
    vtkDataArray* array = vtkArrayDownCast<vtkDataArray>(input->GetColumn(col));
    if (!array || array->GetNumberOfComponents() != 1)
    {
      continue;
    }
    const char* name = array->GetName();
    if (name)
    {
      if (vtksys::SystemTools::Strucmp(name, "time") == 0)
      {
        continue;
      }
      if (strcmp(name, "vtkValidPointMask") == 0)
      {
        if (this->OptimizeForRealInput)
        {
          vtkSmartPointer<vtkDataArray> half;
          half.TakeReference(array->NewInstance());
          half->DeepCopy(array);
          half->SetNumberOfTuples(this->Internals->OutputSize);
          half->Squeeze();
          output->AddColumn(half);
        }
        else
        {
          output->AddColumn(array);
        }
        continue;
      }
    }
    if (array->IsA("vtkIdTypeArray"))
    {
      continue;
    }

    vtkSmartPointer<vtkDataArray> fft = this->DoFFT(array);
    auto namefft = std::string("FFT_").append(name);
    fft->SetName(namefft.c_str());
    output->AddColumn(fft);
  }

  if (this->CreateFrequencyColumn)
  {
    vtkIdType size = this->Internals->Window.size();
    double spacing = 1.0 / this->Internals->SampleRate;

    std::vector<double> stdFreq =
      this->OptimizeForRealInput ? vtkFFT::RFftFreq(size, spacing) : vtkFFT::FftFreq(size, spacing);

    vtkNew<vtkDoubleArray> frequencies;
    frequencies->SetName("Frequency");
    frequencies->SetNumberOfValues(stdFreq.size());
    for (std::size_t i = 0; i < stdFreq.size(); ++i)
    {
      frequencies->SetValue(i, stdFreq[i]);
    }

    output->AddColumn(frequencies);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkTableFFT::Initialize(vtkTable* input)
{
  // Get temporal information
  std::size_t nTimestep = input->GetNumberOfRows();
  vtkDataArray* timeArray = nullptr;
  vtkIdType numColumns = input->GetNumberOfColumns();
  for (vtkIdType col = 0; col < numColumns; col++)
  {
    if (vtksys::SystemTools::Strucmp(input->GetColumn(col)->GetName(), "time") == 0)
    {
      timeArray = vtkDataArray::SafeDownCast(input->GetColumn(col));
      break;
    }
  }
  double deltaT = 0.0;
  if (timeArray && timeArray->GetNumberOfTuples() > 1)
  {
    deltaT = timeArray->GetTuple1(1) - timeArray->GetTuple1(0);
  }

  if (deltaT == 0.0)
  {
    if (this->Normalize || this->CreateFrequencyColumn)
    {
      vtkWarningMacro("'Time' information not found, we will assume a 10'000 Hz sampling rate");
    }
    deltaT = 1.0e-4;
  }
  this->Internals->SampleRate = 1.0 / deltaT;

  // Generate windowing function
  std::size_t actualSize = nTimestep;
  this->Internals->Average = this->AverageFft;
  if (this->AverageFft)
  {
    actualSize = vtkMath::NearestPowerOfTwo(this->BlockSize);
    if (actualSize > (nTimestep - this->NumberOfBlock))
    {
      vtkWarningMacro(
        "Cannot average FFT per block : block size is too large compared to the input. "
        << "Computing FFT on the whole input.");
      this->Internals->Average = false;
      actualSize = nTimestep;
    }
  }
  // We're caching the windowing function for more efficiency when applying this filter
  // on different tables multiple times
  if (this->Internals->WindowLastUpdated < this->Internals->WindowTimeStamp.GetMTime())
  {
    this->Internals->UpdateWindow(this->WindowingFunction, static_cast<int>(actualSize));
    this->Internals->WindowLastUpdated = this->Internals->WindowTimeStamp.GetMTime();
  }

  // Get output size
  const std::size_t nfft = this->Internals->Window.size();
  this->Internals->OutputSize = this->OptimizeForRealInput ? (nfft / 2) + 1 : nfft;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkTableFFT::DoFFT(vtkDataArray* input)
{
  const std::size_t nvalues = input->GetNumberOfValues();
  const std::size_t nblocks =
    this->Internals->Average ? static_cast<std::size_t>(std::max(1, this->NumberOfBlock)) : 1;
  const double blockCoef = 1.0 / nblocks;
  const std::size_t nfft = this->Internals->Window.size();
  const std::size_t outSize = this->Internals->OutputSize;
  const std::size_t stepSize = (nblocks <= 1) ? 0 : (nvalues - nfft - 1) / (nblocks - 1);

  std::vector<vtkFFT::ScalarNumber> block(nfft);
  std::vector<vtkFFT::ComplexNumber> resFft(outSize, vtkFFT::ComplexNumber{ 0.0, 0.0 });

  for (std::size_t b = 0; b < nblocks; ++b)
  {
    // Copy data from input to block
    vtkIdType startBlock = b * stepSize;
    for (std::size_t i = 0; i < nfft; ++i)
    {
      block[i] = input->GetTuple1(startBlock + i);
    }

    // Remove mean signal for each block and apply windowing function
    if (this->Normalize || this->WindowingFunction != RECTANGULAR)
    {
      const double mean =
        this->Normalize ? std::accumulate(block.begin(), block.end(), 0.0) / nfft : 0.0;
      const auto& window = this->Internals->Window;
      for (std::size_t i = 0; i < nfft; ++i)
      {
        block[i] = (block[i] - mean) * window[i];
      }
    }

    // Compute fft and increment
    std::vector<vtkFFT::ComplexNumber> fft =
      this->OptimizeForRealInput ? vtkFFT::RFft(block) : vtkFFT::Fft(block);
    for (std::size_t i = 0; i < fft.size(); ++i)
    {
      resFft[i].r += blockCoef * fft[i].r;
      resFft[i].i += blockCoef * fft[i].i;
    }
  }

  vtkSmartPointer<vtkDataArray> output;
  output.TakeReference(input->NewInstance());
  if (this->Normalize)
  {
    const double norm =
      1.0 / (this->Internals->WindowPonderation * nfft * this->Internals->SampleRate);
    output->SetNumberOfComponents(1);
    output->SetNumberOfTuples(outSize);

    output->SetTuple1(0, vtkFFT::Abs(resFft[0]) * norm);
    for (std::size_t i = 1; i < outSize; ++i)
    {
      output->SetTuple1(i, vtkFFT::Abs(resFft[i]) * 2.0 * norm);
    }
  }
  else
  {
    output->SetNumberOfComponents(2);
    output->SetNumberOfTuples(outSize);
    for (std::size_t i = 0; i < outSize; ++i)
    {
      output->SetTuple2(i, resFft[i].r, resFft[i].i);
    }
  }

  return output;
}

//------------------------------------------------------------------------------
void vtkTableFFT::SetAverageFft(bool _arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting AverageFft to " << _arg);
  if (this->AverageFft != _arg)
  {
    this->BlockSize = _arg;
    this->Internals->WindowTimeStamp.Modified();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkTableFFT::SetBlockSize(int _arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting BlockSize to " << _arg);
  if (this->BlockSize != _arg)
  {
    this->BlockSize = _arg;
    this->Internals->WindowTimeStamp.Modified();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkTableFFT::SetWindowingFunction(int _arg)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting "
                << "WindowingFunction"
                   " to "
                << _arg);
  int clamped =
    (_arg < 0 ? 0 : (_arg >= MAX_WINDOWING_FUNCTION ? (MAX_WINDOWING_FUNCTION - 1) : _arg));
  if (this->WindowingFunction != clamped)
  {
    this->WindowingFunction = clamped;
    this->Internals->WindowTimeStamp.Modified();
    this->Modified();
  }
}

//--------------------------------------- --------------------------------------
void vtkTableFFT::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AverageFft: " << this->AverageFft << std::endl;
  os << indent << "Normalize: " << this->Normalize << std::endl;
  os << indent << "OptimizeForRealInput: " << this->OptimizeForRealInput << std::endl;
  os << indent << "NumberOfBlock: " << this->NumberOfBlock << std::endl;
  os << indent << "BlockSize: " << this->BlockSize << std::endl;
  os << indent << "WindowingFunction: " << this->WindowingFunction << std::endl;
}
