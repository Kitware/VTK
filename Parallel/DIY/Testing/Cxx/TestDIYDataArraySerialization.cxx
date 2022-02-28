/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataArraySerialization.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAOSDataArrayTemplate.h"
#include "vtkAngularPeriodicDataArray.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSmartPointer.h"
#include "vtkTypeFloat64Array.h"

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/mpi.hpp)
// clang-format on

template <vtkIdType N, vtkIdType M, class ArrayT>
bool TestTemplatedArray(ArrayT* array, bool initialized = false)
{
  using ValueType = vtk::GetAPIType<ArrayT>;
  if (!initialized)
  {
    array->SetName("array-name");
    array->SetNumberOfComponents(N);
    array->SetNumberOfTuples(M);
    for (vtkIdType i = 0; i < M; ++i)
    {
      ValueType tuple[N];
      for (vtkIdType j = 0; j < N; ++j)
      {
        tuple[j] = static_cast<ValueType>(N * i + j);
      }
      array->SetTypedTuple(i, tuple);
    }
  }

  diy::MemoryBuffer bb;
  vtkDIYUtilities::Save(bb, array);
  bb.position = 0L;

  vtkDataArray* ptr(nullptr);
  vtkDIYUtilities::Load(bb, ptr);
  const auto loaded = vtkSmartPointer<vtkDataArray>::Take(ptr);

  if (!loaded || loaded->GetNumberOfComponents() != N || loaded->GetNumberOfTuples() != M)
  {
    vtkLog(ERROR, "Data array not loaded correctly");
    return false;
  }

  for (vtkIdType i = 0; i < M; ++i)
  {
    ValueType tuple[N], expected[N];
    for (vtkIdType j = 0; j < N; ++j)
    {
      expected[j] = static_cast<ValueType>(N * i + j);
    }

    array->GetTypedTuple(i, tuple);
    for (vtkIdType j = 0; j < N; ++j)
    {
      if (std::abs(expected[j] - tuple[j]) > 0)
      {
        vtkLog(ERROR, "Data not loaded correctly: " << expected[j] << " != " << tuple[j]);
        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
int TestDIYDataArraySerialization(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool rc(false);
  const vtkNew<vtkDoubleArray> dblArray;
  rc = TestTemplatedArray<3, 14>(dblArray.GetPointer());

  const vtkNew<vtkSOADataArrayTemplate<double>> soaArray;
  rc |= TestTemplatedArray<7, 2>(soaArray.GetPointer());

  const vtkNew<vtkAOSDataArrayTemplate<int>> intArray;
  rc |= TestTemplatedArray<13, 5>(intArray.GetPointer());

  const vtkNew<vtkAngularPeriodicDataArray<double>> angularArray;
  angularArray->InitializeArray(dblArray);
  rc |= TestTemplatedArray<3, 14>(angularArray.GetPointer(), true);

  return rc ? EXIT_SUCCESS : EXIT_FAILURE;
}
