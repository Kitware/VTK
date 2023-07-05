// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkBandFiltering.h>
#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkTable.h>
#include <vtkTableFFT.h>

#include <array>

namespace
{
// These array were generated using the filter and checked visually on a chart.
constexpr std::array<double, 18> EXPECTED_VALUE1 = { 2.59867e-05, 2.59867e-05, 5.07262e-05,
  5.07262e-05, 0.000104954, 0.000104954, 0.000237649, 0.000237649, 0.000860651, 0.000860651,
  11.908427, 11.908427, 0.00472649, 0.00472649, 5.9464, 5.9464, 0.000349909, 0.000349909 };
constexpr std::array<double, 54> EXPECTED_VALUE2 = { 1.20898e-05, 1.20898e-05, 2.11536e-05,
  2.11536e-05, 2.40023e-05, 2.40023e-05, 3.06126e-05, 3.06126e-05, 3.68393e-05, 3.68393e-05,
  4.81976e-05, 4.81976e-05, 6.14968e-05, 6.14968e-05, 7.80998e-05, 7.80998e-05, 9.88132e-05,
  9.88132e-05, 0.000126777, 0.000126777, 0.000165033, 0.000165033, 0.00021851, 0.00021851,
  0.000298669, 0.000298669, 0.00043131, 0.00043131, 0.000680222, 0.000680222, 0.00127487,
  0.00127487, 0.00342693, 0.00342693, 0.0302243, 0.0302243, 28.85516599, 28.85516599, 0.0168144,
  0.0168144, 0.000872449, 0.000872449, 0.000160902, 0.000160902, 7.95249e-05, 7.95249e-05,
  0.000637652, 0.000637652, 14.42116562, 14.42116562, 0.000539186, 0.000539186, 2.67931e-05,
  2.67931e-05 };

template <unsigned long N>
int CheckArray(vtkDataArray* array, std::array<double, N> expected)
{
  const auto arrayRange = vtk::DataArrayValueRange(array);

  bool failure = false;
  if (array->GetNumberOfValues() != static_cast<vtkIdType>(expected.size()))
  {
    failure = true;
    std::cerr << "ERROR: wrong output size" << std::endl;
  }
  else
  {
    vtkIdType valIdx = 0;
    for (const auto value : arrayRange)
    {
      if (!vtkMathUtilities::FuzzyCompare(static_cast<double>(value), expected[valIdx++], 1e-4))
      {
        failure = true;
        break;
      }
    }
  }

  if (failure)
  {
    std::cerr << "Unexpected result.\nResult : {";
    for (const auto value : arrayRange)
    {
      std::cerr << value << ", ";
    }
    std::cerr << "}\nExpected:{";
    for (const auto val : expected)
    {
      std::cerr << val << ", ";
    }
    std::cerr << "}" << std::endl;
  }

  return static_cast<int>(failure);
}
}

// ----------------------------------------------------------------------------
int TestBandFiltering(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  constexpr int N_ELEMENTS = 1000;

  // Fill our data
  vtkNew<vtkTable> input;
  vtkNew<vtkDoubleArray> column;
  column->SetName("Signal");
  column->SetNumberOfComponents(1);
  column->SetNumberOfTuples(N_ELEMENTS);
  for (vtkIdType i = 0; i < N_ELEMENTS; i++)
  {
    column->SetValue(i, std::cos(0.5 * i) + 2.0 * std::cos(2.0 * i));
  }
  input->AddColumn(column);
  int exitCode = EXIT_SUCCESS;

  // Testing Octave band filtering
  vtkNew<vtkBandFiltering> bandFiltering;
  bandFiltering->SetInputData(input);
  bandFiltering->SetBandFilteringMode(vtkBandFiltering::OCTAVE);
  bandFiltering->SetWindowType(vtkTableFFT::HANNING);
  bandFiltering->SetDefaultSamplingRate(1000);
  bandFiltering->Update();

  vtkDataArray* arr =
    vtkDataArray::SafeDownCast(bandFiltering->GetOutput()->GetColumnByName("Signal"));
  exitCode += ::CheckArray(arr, EXPECTED_VALUE1);

  // Check that we have the same result using an external FFT
  vtkNew<vtkTableFFT> tableFFT;
  tableFFT->SetInputData(input);
  tableFFT->SetWindowingFunction(vtkTableFFT::HANNING);
  tableFFT->CreateFrequencyColumnOn();
  tableFFT->ReturnOnesidedOn();
  tableFFT->SetDefaultSampleRate(1000);
  tableFFT->Update();

  bandFiltering->SetInputData(tableFFT->GetOutput());
  bandFiltering->SetApplyFFT(false);
  bandFiltering->Update();

  arr = vtkDataArray::SafeDownCast(bandFiltering->GetOutput()->GetColumnByName("Signal"));
  exitCode += ::CheckArray(arr, EXPECTED_VALUE1);

  // Check third octave result
  bandFiltering->SetBandFilteringMode(vtkBandFiltering::THIRD_OCTAVE);
  bandFiltering->Update();

  arr = vtkDataArray::SafeDownCast(bandFiltering->GetOutput()->GetColumnByName("Signal"));
  exitCode += ::CheckArray(arr, EXPECTED_VALUE2);

  return exitCode;
}
