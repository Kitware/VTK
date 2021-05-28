/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTableFFT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkDoubleArray.h>
#include <vtkMathUtilities.h>
#include <vtkNew.h>
#include <vtkTable.h>
#include <vtkTableFFT.h>

#include <array>

namespace details
{
// ----------------------------------------------------------------------------
void PrintArray(vtkDoubleArray* array)
{
  std::cerr << "[ ";
  for (vtkIdType i = 0; i < array->GetNumberOfTuples(); ++i)
  {
    std::cerr << "{";
    double* tuple = array->GetTuple(i);
    for (int j = 0; j < array->GetNumberOfComponents(); ++j)
    {
      std::cerr << tuple[j];
      if (j < array->GetNumberOfComponents() - 1)
      {
        std::cerr << ";";
      }
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
constexpr std::array<double, Length* 2> e_col1 = { { Length, 0.0, Length, 0.0, Length, 0.0, Length,
  0.0, Length, 0.0, Length, 0.0, Length, 0.0, Length, 0.0 } };
constexpr std::array<double, Length* 2> e_col2 = { { Length, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0, 0.0, 0.0 } };
constexpr std::array<double, Length> e_freq = { { 0.0, 0.125, 0.25, 0.375, -0.5, -0.375, -0.25,
  -0.125 } };

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
void InitializeTableReference(vtkTable* table)
{
  vtkNew<vtkDoubleArray> column1;
  column1->SetNumberOfTuples(Length);
  column1->SetNumberOfComponents(2);
  column1->SetArray(const_cast<double*>(e_col1.data()), Length * 2, /*save*/ 1);
  column1->SetName("Data1");

  vtkNew<vtkDoubleArray> column2;
  column2->SetNumberOfTuples(Length);
  column2->SetNumberOfComponents(2);
  column2->SetArray(const_cast<double*>(e_col2.data()), Length * 2, /*save*/ 1);
  column2->SetName("Data2");

  vtkNew<vtkDoubleArray> columnFreq;
  columnFreq->SetNumberOfTuples(Length);
  columnFreq->SetNumberOfComponents(1);
  columnFreq->SetArray(const_cast<double*>(e_freq.data()), Length, /*save*/ 1);
  columnFreq->SetName("Frequency");

  table->AddColumn(column1);
  table->AddColumn(column2);
  table->AddColumn(columnFreq);
}

// ----------------------------------------------------------------------------
bool FuzzyCompare(vtkTable* in, vtkTable* expected, double epsilon)
{
  bool status = true;

  for (vtkIdType col = 0; col < in->GetNumberOfColumns(); ++col)
  {
    vtkDoubleArray* inArray = vtkDoubleArray::SafeDownCast(in->GetColumn(col));
    vtkDoubleArray* expArray = vtkDoubleArray::SafeDownCast(expected->GetColumn(col));

    for (vtkIdType i = 0; i < inArray->GetNumberOfValues(); ++i)
    {
      if (!vtkMathUtilities::NearlyEqual(inArray->GetValue(i), expArray->GetValue(i), epsilon))
      {
        status = false;

        std::cerr << "[TestTableFFT] FAILURE for column <" << inArray->GetName() << ">"
                  << std::endl;
        std::cerr << "Expected : ";
        PrintArray(expArray);
        std::cerr << "But got  : ";
        PrintArray(inArray);
        break;
      }
    }
  }

  if (status)
  {
    std::cerr << "[TestTableFFT] We're all clear" << std::endl;
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

  // Initialize data
  vtkNew<vtkTable> input;
  details::InitializeTableInput(input);
  vtkNew<vtkTable> reference;
  details::InitializeTableReference(reference);

  // Test actual data
  fftFilter->SetInputData(input);
  fftFilter->CreateFrequencyColumnOn();
  fftFilter->SetWindowingFunction(vtkTableFFT::RECTANGULAR);
  fftFilter->Update();
  status += static_cast<int>(!details::FuzzyCompare(fftFilter->GetOutput(), reference, 1.0e-6));

  return status;
}
