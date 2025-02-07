// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov

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
VTK_ABI_NAMESPACE_BEGIN
// Easy access to the right windowing function using vtkTableFFT enumeration.
// clang-format off
static const std::array<vtkFFT::WindowGenerator, vtkTableFFT::MAX_WINDOWING_FUNCTION> WindowingFunctionsList =
{
  vtkFFT::HanningGenerator,
  vtkFFT::BartlettGenerator,
  vtkFFT::SineGenerator,
  vtkFFT::BlackmanGenerator,
  vtkFFT::RectangularGenerator
};
// clang-format on
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
struct vtkTableFFT::vtkInternal
{
  std::vector<vtkFFT::ScalarNumber> Window;
  vtkTimeStamp WindowTimeStamp;
  vtkMTimeType WindowLastUpdated = 0;
  double SampleRate = 1.0e4;
  vtkIdType OutputSize = 0;

  void UpdateWindow(int window, std::size_t size)
  {
    this->Window.resize(size);
    vtkFFT::GenerateKernel1D(
      this->Window.data(), this->Window.size(), details::WindowingFunctionsList[window]);
  }

  vtkSmartPointer<vtkDataArray> ApplyDirectFFT(vtkDataArray* array, bool onesided, bool normalize)
  {
    vtkNew<vtkFFT::vtkScalarNumberArray> windowedSignal;
    windowedSignal->SetNumberOfComponents(array->GetNumberOfComponents());
    windowedSignal->SetNumberOfTuples(array->GetNumberOfTuples());
    auto inputRange = vtk::DataArrayTupleRange(array);
    auto outRange = vtk::DataArrayTupleRange(windowedSignal);
    vtkSMPTools::For(0, inputRange.size(),
      [&](vtkIdType begin, vtkIdType end)
      {
        auto inputIt = inputRange.cbegin() + begin;
        auto windowIt = this->Window.cbegin() + begin;
        auto outputIt = outRange.begin() + begin;
        for (vtkIdType i = begin; i < end; ++i, ++inputIt, ++windowIt, ++outputIt)
        {
          auto inComponentIt = inputIt->cbegin();
          auto outComponentIt = outputIt->begin();
          for (; inComponentIt != inputIt->cend(); ++inComponentIt, ++outComponentIt)
          {
            *outComponentIt = *inComponentIt * *windowIt;
          }
        }
      });

    vtkSmartPointer<vtkFFT::vtkScalarNumberArray> result =
      onesided ? vtkFFT::RFft(windowedSignal) : vtkFFT::Fft(windowedSignal);

    if (normalize)
    {
      auto resRange = vtk::DataArrayValueRange<2>(result);
      vtkSMPTools::Transform(resRange.begin(), resRange.end(), resRange.begin(),
        [&](vtkFFT::ScalarNumber val) { return val / resRange.size(); });
    }

    return result;
  }

  vtkSmartPointer<vtkDataArray> ApplyWelchFFT(
    vtkDataArray* array, int noverlap, bool detrend, bool onesided, int scaling)
  {
    // If memory layout is already correct we can avoid copying the data
    if (auto* fftArray = vtkFFT::vtkScalarNumberArray::SafeDownCast(array))
    {
      return vtkFFT::Csd(fftArray, this->Window, this->SampleRate, noverlap, detrend, onesided,
        static_cast<vtkFFT::Scaling>(scaling));
    }
    else
    {
      vtkNew<vtkFFT::vtkScalarNumberArray> dcopy;
      dcopy->DeepCopy(array);
      return vtkFFT::Csd(dcopy, this->Window, this->SampleRate, noverlap, detrend, onesided,
        static_cast<vtkFFT::Scaling>(scaling));
    }
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
    if (this->CheckAbort())
    {
      break;
    }
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
        (dataArray->GetNumberOfComponents() == 2 && !this->ReturnOnesided)) &&
      !array->IsA("vtkIdTypeArray"))
    {
      vtkSmartPointer<vtkDataArray> fft = this->DoFFT(dataArray);
      std::string newArrayName = arrayName;
      fft->SetName(newArrayName.c_str());
      output->AddColumn(fft);
    }
    // else pass the array to the output
    else if (!this->ReturnOnesided && !this->AverageFft)
    {
      assert(array->GetNumberOfTuples() == this->Internals->OutputSize);
      output->AddColumn(array);
    }
  }

  // Create the frequency column if needed
  if (this->CreateFrequencyColumn)
  {
    int size = static_cast<int>(this->Internals->Window.size());
    double spacing = 1.0 / this->Internals->SampleRate;

    std::vector<double> stdFreq =
      this->ReturnOnesided ? vtkFFT::RFftFreq(size, spacing) : vtkFFT::FftFreq(size, spacing);

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
  const std::size_t nsamples = input->GetNumberOfRows();
  vtkDataArray* timeArray = nullptr;
  bool complexColumnFound = false;
  for (vtkIdType col = 0; col < input->GetNumberOfColumns(); col++)
  {
    vtkAbstractArray* column = input->GetColumn(col);

    if (vtksys::SystemTools::Strucmp(column->GetName(), "time") == 0)
    {
      timeArray = vtkDataArray::SafeDownCast(input->GetColumn(col));
    }

    complexColumnFound |= (column->GetNumberOfComponents() == 2);
  }

  if (this->ReturnOnesided && complexColumnFound)
  {
    vtkWarningMacro("ReturnOnesided is True but found columns with 2 components"
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
  if (this->AverageFft)
  {
    nfft = std::min(static_cast<std::size_t>(this->BlockSize), nsamples);
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
  this->Internals->OutputSize = this->ReturnOnesided ? (nfft / 2) + 1 : nfft;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkTableFFT::DoFFT(vtkDataArray* input)
{
  if (this->AverageFft)
  {
    return this->Internals->ApplyWelchFFT(
      input, this->BlockOverlap, this->Detrend, this->ReturnOnesided, this->ScalingMethod);
  }
  else
  {
    return this->Internals->ApplyDirectFFT(input, this->ReturnOnesided, this->Normalize);
  }
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
  os << indent << "ReturnOnesided: " << this->ReturnOnesided << std::endl;
  os << indent << "BlockSize: " << this->BlockSize << std::endl;
  os << indent << "WindowingFunction: " << this->WindowingFunction << std::endl;
}
VTK_ABI_NAMESPACE_END
