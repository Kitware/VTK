// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkDoubleArray.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkTable.h>
#include <vtkTableFFT.h>

#include <array>

namespace details
{
// ----------------------------------------------------------------------------
void PrintArray(vtkDataArray* array)
{
  std::cerr << "[ ";
  auto range = vtk::DataArrayTupleRange(array);
  for (auto tuple : range)
  {
    std::cerr << "{";
    for (auto value : tuple)
    {
      std::cerr << value << ";";
    }
    std::cerr << "} ";
  }
  std::cerr << "]" << std::endl;
}

// ----------------------------------------------------------------------------
constexpr int Length = 8;
// Inputs
constexpr std::array<double, Length> col1 = { { Length, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 } };
constexpr std::array<double, Length> col2 = { { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 } };
constexpr std::array<double, Length> time = { { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 } };
// Expected output
constexpr std::array<double, Length* 2l> e_col1 = { { Length, 0.0, Length, 0.0, Length, 0.0, Length,
  0.0, Length, 0.0, Length, 0.0, Length, 0.0, Length, 0.0 } };
constexpr std::array<double, Length* 2l> e_col2 = { { Length, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 } };
constexpr std::array<double, Length> e_freq = { { 0.0, 0.125, 0.25, 0.375, -0.5, -0.375, -0.25,
  -0.125 } };
constexpr std::array<double, Length> e_freq2 = { { 0.0, 1.25, 2.5, 3.75, -5.0, -3.75, -2.5,
  -1.25 } };

// ----------------------------------------------------------------------------
void InitializeTableInput(vtkTable* table)
{
  vtkNew<vtkDoubleArray> column1;
  column1->SetNumberOfTuples(Length);
  column1->SetNumberOfComponents(1);
  column1->SetArray(const_cast<double*>(col1.data()), Length, /*save*/ 1);
  column1->SetName("Data1");

  vtkNew<vtkDoubleArray> column2;
  column2->SetNumberOfTuples(Length);
  column2->SetNumberOfComponents(1);
  column2->SetArray(const_cast<double*>(col2.data()), Length, /*save*/ 1);
  column2->SetName("Data2");

  vtkNew<vtkDoubleArray> columnTime;
  columnTime->SetNumberOfTuples(Length);
  columnTime->SetNumberOfComponents(1);
  columnTime->SetArray(const_cast<double*>(time.data()), Length, /*save*/ 1);
  columnTime->SetName("Time");

  table->AddColumn(column1);
  table->AddColumn(column2);
  table->AddColumn(columnTime);
}

// ----------------------------------------------------------------------------
void InitializeTableComplex(vtkTable* input, vtkTable* output)
{
  vtkNew<vtkDoubleArray> data;
  data->SetNumberOfComponents(2);
  data->SetNumberOfTuples(Length);
  data->SetName("Data");
  for (vtkIdType i = 0; i < Length; ++i)
  {
    data->SetTuple2(i, (i + 1) % 2, i % 2);
  }
  input->AddColumn(data);

  vtkNew<vtkDoubleArray> result;
  result->SetNumberOfComponents(2);
  result->SetNumberOfTuples(Length);
  result->SetName("Data");
  result->Fill(0.0);
  result->SetTuple2(0, Length * 0.5, Length * 0.5);
  result->SetTuple2(Length / 2, Length * 0.5, -Length * 0.5);
  output->AddColumn(result);
}

// ----------------------------------------------------------------------------
void InitializeTableReference(vtkTable* table)
{
  vtkNew<vtkDoubleArray> column1;
  column1->SetNumberOfTuples(Length);
  column1->SetNumberOfComponents(2);
  column1->SetArray(const_cast<double*>(e_col1.data()), Length * 2l, /*save*/ 1);
  column1->SetName("Data1");

  vtkNew<vtkDoubleArray> column2;
  column2->SetNumberOfTuples(Length);
  column2->SetNumberOfComponents(2);
  column2->SetArray(const_cast<double*>(e_col2.data()), Length * 2l, /*save*/ 1);
  column2->SetName("Data2");

  vtkNew<vtkDoubleArray> columnFreq;
  columnFreq->SetNumberOfTuples(Length);
  columnFreq->SetNumberOfComponents(1);
  columnFreq->SetArray(const_cast<double*>(e_freq.data()), Length, /*save*/ 1);
  columnFreq->SetName("Frequency");

  vtkNew<vtkDoubleArray> columnFreq2;
  columnFreq2->SetNumberOfTuples(Length);
  columnFreq2->SetNumberOfComponents(1);
  columnFreq2->SetArray(const_cast<double*>(e_freq2.data()), Length, /*save*/ 1);
  columnFreq2->SetName("Frequency2");

  table->AddColumn(column1);
  table->AddColumn(column2);
  table->AddColumn(columnFreq);
  table->AddColumn(columnFreq2);
}

// ----------------------------------------------------------------------------
void AddFieldToTableInput(vtkTable* table)
{
  vtkNew<vtkDoubleArray> columnFieldData;
  columnFieldData->SetNumberOfTuples(Length);
  columnFieldData->SetNumberOfComponents(1);
  columnFieldData->SetArray(const_cast<double*>(col2.data()), Length, /*save*/ 1);
  columnFieldData->SetName("vtkDummyData");
  table->AddColumn(columnFieldData);
}

// ----------------------------------------------------------------------------
bool FuzzyCompare(vtkDataArray* inArray, vtkDataArray* expected, double epsilon)
{
  auto inRange = vtk::DataArrayValueRange(inArray);
  auto expRange = vtk::DataArrayValueRange(expected);
  using ValueT = decltype(inRange)::ValueType;
  bool status = std::equal(inRange.cbegin(), inRange.cend(), expRange.cbegin(),
    [epsilon](ValueT x, ValueT y) { return vtkMathUtilities::NearlyEqual(x, y, epsilon); });

  if (!status)
  {
    std::cerr << "[TestTableFFT] FAILURE for column <" << inArray->GetName() << ">" << std::endl;
    std::cerr << "Expected : ";
    PrintArray(expected);
    std::cerr << "But got  : ";
    PrintArray(inArray);
  }

  return status;
}

// ----------------------------------------------------------------------------
bool FuzzyCompare(vtkTable* in, vtkTable* expected, double epsilon)
{
  bool status = true;

  for (vtkIdType col = 0; col < in->GetNumberOfColumns(); ++col)
  {
    vtkDataArray* inArray = vtkArrayDownCast<vtkDataArray>(in->GetColumn(col));
    vtkDataArray* expArray = vtkArrayDownCast<vtkDataArray>(expected->GetColumn(col));

    status = status && FuzzyCompare(inArray, expArray, epsilon);
  }

  return status;
}

}

// ----------------------------------------------------------------------------
int TestTableFFT(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int status = EXIT_SUCCESS;

  vtkNew<vtkTableFFT> fftFilter;

  // Test empty
  vtkNew<vtkTable> empty;
  fftFilter->SetInputData(empty);
  fftFilter->SetWindowingFunction(-55); // Wrong windowing function
  fftFilter->Update();
  status += static_cast<int>(!details::FuzzyCompare(fftFilter->GetOutput(), empty, 1.0e-6));

  // Test actual data
  vtkNew<vtkTable> input;
  details::InitializeTableInput(input);
  fftFilter->SetInputData(input);
  fftFilter->CreateFrequencyColumnOn();
  fftFilter->SetWindowingFunction(vtkTableFFT::RECTANGULAR);
  fftFilter->Update();
  vtkNew<vtkTable> reference;
  details::InitializeTableReference(reference);
  status += static_cast<int>(!details::FuzzyCompare(fftFilter->GetOutput(), reference, 1.0e-6));

  // Test with a different sampling rate
  input->RemoveColumnByName("Time");
  fftFilter->SetInputData(input);
  fftFilter->SetDefaultSampleRate(10);
  fftFilter->Update();
  auto* result = vtkDataArray::SafeDownCast(fftFilter->GetOutput()->GetColumnByName("Frequency"));
  auto* expected = vtkDataArray::SafeDownCast(reference->GetColumnByName("Frequency2"));
  status += static_cast<int>(!details::FuzzyCompare(result, expected, 1.0e-6));

  // Test with complex numbers input
  input->RemoveAllColumns();
  reference->RemoveAllColumns();
  details::InitializeTableComplex(input, reference);
  fftFilter->SetInputData(input);
  fftFilter->ReturnOnesidedOff();
  fftFilter->CreateFrequencyColumnOff();
  fftFilter->Update();
  result = vtkDataArray::SafeDownCast(fftFilter->GetOutput()->GetColumn(0));
  expected = vtkDataArray::SafeDownCast(reference->GetColumnByName("Data"));
  status += static_cast<int>(!details::FuzzyCompare(result, expected, 1.0e-6));

  // Test with output column size different from the input
  details::AddFieldToTableInput(input);
  fftFilter->ReturnOnesidedOff();
  fftFilter->SetAverageFft(true);
  fftFilter->SetBlockSize(5);
  fftFilter->Update();

  return status;
}
