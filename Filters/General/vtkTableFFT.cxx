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

#include <algorithm>
#include <array>
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
    static_cast<T>(std::distance(begin, end));
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

//------------------------------------------------------------------------------
template <typename T>
void CopyTuple(vtkDataArray* source, size_t sindex, std::vector<T>& target, size_t tindex)
{
  target[tindex] = source->GetTuple1(sindex + tindex);
}
template <>
void CopyTuple(
  vtkDataArray* source, size_t sindex, std::vector<vtkFFT::ComplexNumber>& target, size_t tindex)
{
  double* tuple = source->GetTuple2(sindex + tindex);
  target[tindex].r = tuple[0];
  target[tindex].i = tuple[1];
}

//------------------------------------------------------------------------------
template <typename T>
T Zero()
{
  return 0.0;
}
template <>
vtkFFT::ComplexNumber Zero()
{
  return vtkFFT::ComplexNumber{ 0.0, 0.0 };
}

//------------------------------------------------------------------------------
template <typename T>
std::vector<vtkFFT::ComplexNumber> DispatchFFT(const std::vector<T>& in, bool realFft)
{
  return realFft ? vtkFFT::RFft(in) : vtkFFT::Fft(in);
}
template <>
std::vector<vtkFFT::ComplexNumber> DispatchFFT(
  const std::vector<vtkFFT::ComplexNumber>& in, bool vtkNotUsed(realFft))
{
  return vtkFFT::Fft(in);
}
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

  void UpdateWindow(int window, std::size_t size)
  {
    this->Window.resize(size);

    vtkFFT::GenerateKernel1D(
      this->Window.data(), this->Window.size(), details::WindowingFunctionsList[window]);
    this->WindowPonderation = (window == vtkTableFFT::RECTANGULAR)
      ? 1.0
      : details::WindowEnergy(this->Window.begin(), this->Window.end());
  }

  template <typename T>
  std::vector<vtkFFT::ComplexNumber> DoWelchMethod(vtkDataArray* input, size_t nblocks,
    size_t blocksize, bool normalize, bool windowing, bool optimize = false)
  {
    nblocks = this->Average ? std::max(size_t(1), nblocks) : 1;
    const std::size_t nsamples = input->GetNumberOfTuples();
    const std::size_t outSize = this->OutputSize;
    const std::size_t stepSize = (nblocks <= 1) ? 0 : (nsamples - blocksize - 1) / (nblocks - 1);

    std::vector<vtkFFT::ComplexNumber> resFft(outSize, vtkFFT::ComplexNumber{ 0.0, 0.0 });
    const double blockCoef = 1.0 / nblocks;
    for (std::size_t b = 0; b < nblocks; ++b)
    {
      // Copy data from input to block
      vtkIdType startBlock = b * stepSize;
      std::vector<T> block(blocksize);
      for (std::size_t i = 0; i < blocksize; ++i)
      {
        details::CopyTuple<T>(input, startBlock, block, i);
      }

      // Remove mean signal for each block and apply windowing function
      if (normalize || windowing)
      {
        T mean = details::Zero<T>();
        if (normalize)
        {
          mean = std::accumulate(block.begin(), block.end(), details::Zero<T>()) / blocksize;
        }
        const auto& window = this->Window;
        for (std::size_t i = 0; i < blocksize; ++i)
        {
          block[i] = (block[i] - mean) * window[i];
        }
      }

      // Compute fft and increment
      std::vector<vtkFFT::ComplexNumber> fft = details::DispatchFFT(block, optimize);
      for (std::size_t i = 0; i < fft.size(); ++i)
      {
        resFft[i] = resFft[i] + fft[i] * blockCoef;
      }
    }

    return resFft;
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
vtkTableFFT::~vtkTableFFT() = default;

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
    return 1;
  }

  // Initialize internal state such as output size, sampling frequency, etc
  this->Initialize(input);

  // Process every column of the input
  vtkIdType numColumns = input->GetNumberOfColumns();
  for (vtkIdType col = 0; col < numColumns; col++)
  {
    vtkAbstractArray* array = input->GetColumn(col);
    const char* arrayName = array->GetName();
    vtkDataArray* dataArray = vtkDataArray::SafeDownCast(array);

    // If array is the time array, skip
    if (vtksys::SystemTools::Strucmp(arrayName, "time") == 0)
    {
      continue;
    }
    // else if we can and should process the data array for the FFT, do it
    else if (dataArray && !vtksys::SystemTools::StringStartsWith(arrayName, "vtk") &&
      (dataArray->GetNumberOfComponents() == 1 ||
        (dataArray->GetNumberOfComponents() == 2 && !this->OptimizeForRealInput)) &&
      !array->IsA("vtkIdTypeArray"))
    {
      vtkSmartPointer<vtkDataArray> fft = this->DoFFT(dataArray);
      std::string newArrayName =
        this->PrefixOutputArrays ? std::string("FFT_").append(arrayName) : arrayName;
      fft->SetName(newArrayName.c_str());
      output->AddColumn(fft);
    }
    // else pass the array to the output
    else
    {
      if (this->OptimizeForRealInput)
      {
        vtkSmartPointer<vtkAbstractArray> half;
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
    }
  }

  // Create the frequency column if needed
  if (this->CreateFrequencyColumn)
  {
    std::size_t size = this->Internals->Window.size();
    double spacing = 1.0 / this->Internals->SampleRate;

    std::vector<double> stdFreq = this->OptimizeForRealInput
      ? vtkFFT::RFftFreq(static_cast<int>(size), spacing)
      : vtkFFT::FftFreq(static_cast<int>(size), spacing);

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
  // Find time array and compute sample rate
  std::size_t nsamples = input->GetNumberOfRows();
  vtkDataArray* timeArray = nullptr;
  vtkIdType numColumns = input->GetNumberOfColumns();
  bool imaginaryColumnFound = false;
  for (vtkIdType col = 0; col < numColumns; col++)
  {
    vtkAbstractArray* column = input->GetColumn(col);

    if (vtksys::SystemTools::Strucmp(column->GetName(), "time") == 0)
    {
      timeArray = vtkDataArray::SafeDownCast(input->GetColumn(col));
      break;
    }

    imaginaryColumnFound |= (column->GetNumberOfComponents() == 2);
  }

  if (this->OptimizeForRealInput && imaginaryColumnFound)
  {
    vtkWarningMacro("OptimizeForRealInput is True but found columns with 2 components"
                    " (interpreted as imaginary data). Imaginary columns will be ignored.");
  }

  if (timeArray && timeArray->GetNumberOfTuples() > 1)
  {
    double deltaT = timeArray->GetTuple1(1) - timeArray->GetTuple1(0);
    this->Internals->SampleRate = 1.0 / deltaT;
  }
  else
  {
    this->Internals->SampleRate = this->DefaultSampleRate;
  }

  // Check if we can average and compute the size of the windowing function
  std::size_t nfft = nsamples;
  this->Internals->Average = this->AverageFft;
  if (this->AverageFft)
  {
    nfft = vtkMath::NearestPowerOfTwo(static_cast<int>(this->BlockSize));
    if (nfft > (nsamples - this->NumberOfBlock))
    {
      vtkWarningMacro(
        "Cannot average FFT per block : block size is too large compared to the input. "
        << "Computing FFT on the whole input.");
      this->Internals->Average = false;
      nfft = nsamples;
    }
  }

  // Generate windowing function
  // We're caching the windowing function for more efficiency when applying this filter
  // on different tables multiple times
  if (this->Internals->WindowLastUpdated < this->Internals->WindowTimeStamp.GetMTime() ||
    nfft != this->Internals->Window.size())
  {
    this->Internals->UpdateWindow(this->WindowingFunction, nfft);
    this->Internals->WindowLastUpdated = this->Internals->WindowTimeStamp.GetMTime();
  }

  // Get output size
  this->Internals->OutputSize = this->OptimizeForRealInput ? (nfft / 2) + 1 : nfft;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkTableFFT::DoFFT(vtkDataArray* input)
{
  const std::size_t blocksize = this->Internals->Window.size();

  std::vector<vtkFFT::ComplexNumber> fft;
  if (input->GetNumberOfComponents() == 1)
  {
    fft =
      this->Internals->DoWelchMethod<vtkFFT::ScalarNumber>(input, this->NumberOfBlock, blocksize,
        this->Normalize, this->WindowingFunction != RECTANGULAR, this->OptimizeForRealInput);
  }
  else if (input->GetNumberOfComponents() == 2)
  {
    fft = this->Internals->DoWelchMethod<vtkFFT::ComplexNumber>(input, this->NumberOfBlock,
      blocksize, this->Normalize, this->WindowingFunction != RECTANGULAR);
  }

  vtkSmartPointer<vtkDataArray> output;
  output.TakeReference(input->NewInstance());
  if (this->Normalize)
  {
    const double norm =
      1.0 / (this->Internals->WindowPonderation * blocksize * this->Internals->SampleRate);
    output->SetNumberOfComponents(1);
    output->SetNumberOfTuples(fft.size());

    output->SetTuple1(0, vtkFFT::Abs(fft[0]) * norm);
    for (std::size_t i = 1; i < fft.size(); ++i)
    {
      output->SetTuple1(i, vtkFFT::Abs(fft[i]) * 2.0 * norm);
    }
  }
  else
  {
    output->SetNumberOfComponents(2);
    output->SetNumberOfTuples(fft.size());
    for (std::size_t i = 0; i < fft.size(); ++i)
    {
      output->SetTuple2(i, fft[i].r, fft[i].i);
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
    this->AverageFft = _arg;
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
                << "WindowingFunction to " << _arg);
  int clamped = std::min(std::max(_arg, 0), static_cast<int>(MAX_WINDOWING_FUNCTION));
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
