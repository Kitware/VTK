/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBandFiltering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
constexpr std::array<double, 6> EXPECTED_VALUE1 = { 0.326591, 0.340033, 0.407533, 0.46924, 1.15643,
  4.9792 };

constexpr std::array<double, 20> EXPECTED_VALUE2 = { 0, 0, 89.5688, 0, 68.191, 89.5924, 70.4061,
  81.0587, 89.6872, 80.003, 85.7241, 82.8791, 86.8522, 87.555, 89.3785, 92.708, 98.1622, 111.79,
  96.5032, 87.3854 };

constexpr std::array<double, 20> EXPECTED_VALUE3 = { 0, 0, 139.54, 0, 138.134, 137.498, 137.011,
  136.599, 135.875, 134.984, 134.216, 133.314, 132.309, 131.162, 129.834, 128.125, 126.043, 123.828,
  115.938, 116.618 };
}

// ----------------------------------------------------------------------------
int TestBandFiltering(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Fill our data
  vtkNew<vtkTable> input;
  vtkNew<vtkDoubleArray> column;
  column->SetName("Pa 1");
  column->SetNumberOfTuples(100);
  column->SetNumberOfComponents(1);
  for (vtkIdType i = 0; i < 100; i++)
  {
    column->InsertNextTuple1(sin(2 * i) + sin(vtkMath::Pi() * i));
  }
  input->AddColumn(column);

  // Testing Octave band filtering
  vtkNew<vtkBandFiltering> bandFiltering;
  bandFiltering->SetInputData(input);
  bandFiltering->SetBandFilteringMode(vtkBandFiltering::OCTAVE);
  bandFiltering->Update();

  for (vtkIdType col = 0; col < bandFiltering->GetOutput()->GetNumberOfColumns(); col++)
  {
    vtkDoubleArray* arr = vtkDoubleArray::SafeDownCast(bandFiltering->GetOutput()->GetColumn(col));

    if (!arr)
    {
      return 1;
    }

    if (vtksys::SystemTools::Strucmp(arr->GetName(), "Frequency") == 0)
    {
      continue;
    }

    if (arr->GetNumberOfValues() != 6)
    {
      std::cerr << "Wrong number of values. Expected 6 but got " << arr->GetNumberOfValues()
                << std::endl;
      return 1;
    }

    for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
      if (!vtkMathUtilities::NearlyEqual(arr->GetValue(i), ::EXPECTED_VALUE1[i], 1.0e-06))
      {
        std::cerr << "Wrong value in Pa for the octave band filtering. Expected "
                  << ::EXPECTED_VALUE1[i] << " but got " << arr->GetValue(i) << std::endl;
        return 1;
      }
    }
  }

  // Testing Third Octave filtering with an fft input
  vtkNew<vtkDoubleArray> newColumn;
  newColumn->SetName("Pa 2");
  newColumn->SetNumberOfTuples(100);
  newColumn->SetNumberOfComponents(1);
  for (vtkIdType i = 0; i < 100; i++)
  {
    newColumn->InsertNextTuple1(cos(2 * i) + cos(vtkMath::Pi() * i));
  }
  input->AddColumn(newColumn);

  vtkNew<vtkTableFFT> tableFFT;
  tableFFT->SetInputData(input);
  tableFFT->CreateFrequencyColumnOn();
  tableFFT->Update();

  bandFiltering->SetInputData(tableFFT->GetOutput());
  bandFiltering->SetApplyFFT(false);
  bandFiltering->SetOutputInDecibel(true);
  bandFiltering->SetReferenceValue(2e-05);
  bandFiltering->SetBandFilteringMode(vtkBandFiltering::THIRD_OCTAVE);
  bandFiltering->Update();

  for (vtkIdType col = 0; col < bandFiltering->GetOutput()->GetNumberOfColumns(); col++)
  {
    vtkDoubleArray* arr = vtkDoubleArray::SafeDownCast(bandFiltering->GetOutput()->GetColumn(col));

    if (!arr)
    {
      return 1;
    }

    if (vtksys::SystemTools::Strucmp(arr->GetName(), "Frequency") == 0)
    {
      continue;
    }

    if (arr->GetNumberOfValues() != 20)
    {
      std::cerr << "Wrong number of values. Expected 20 but got " << arr->GetNumberOfValues()
                << std::endl;
      return 1;
    }

    for (vtkIdType i = 0; i < arr->GetNumberOfValues(); i++)
    {
      double expectedValue = ::EXPECTED_VALUE2[i];
      if (vtksys::SystemTools::Strucmp(arr->GetName(), "Pa 2") == 0)
      {
        expectedValue = ::EXPECTED_VALUE3[i];
      }
      if (!vtkMathUtilities::NearlyEqual(arr->GetValue(i), expectedValue, 1.0e-05))
      {
        std::cerr << "Wrong value in Db for the third octave band filtering. Expected "
                  << expectedValue << " but got " << arr->GetValue(i) << std::endl;
        return 1;
      }
    }
  }

  return 0;
}
