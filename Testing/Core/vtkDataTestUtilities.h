// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataTestUtilities
 * @brief   Utility functions used for testing vtk data structures.
 */
#ifndef vtkDataTestUtilities_h
#define vtkDataTestUtilities_h

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkLogger.h"

namespace vtk
{

VTK_ABI_NAMESPACE_BEGIN

struct CompareVectorWorker
{
  CompareVectorWorker()
    : ExitValue(EXIT_SUCCESS)
  {
  }
  template <typename ArrayT, typename ExpectedArrayT>
  void operator()(ArrayT* array, ExpectedArrayT* expectedArray)
  {
    const auto range = DataArrayTupleRange(array);
    const auto expectedRange = DataArrayTupleRange(expectedArray);

    const TupleIdType numTuples = range.size();
    const ComponentIdType numComps = range.GetTupleSize();

    std::cout << "Compare " << array->GetName() << std::endl;
    this->ExitValue = EXIT_SUCCESS;
    for (TupleIdType tupleId = 0; tupleId < numTuples; ++tupleId)
    {
      const auto tuple = range[tupleId];
      auto expectedTuple = expectedRange[tupleId];

      for (ComponentIdType compId = 0; compId < numComps; ++compId)
      {
        if (tuple[compId] != expectedTuple[compId])
        {
          std::cerr << "Expecting " << expectedTuple[compId] << " for tuple/component: " << tupleId
                    << "/" << compId << " but got: " << tuple[compId] << std::endl;
          this->ExitValue = EXIT_FAILURE;
          break;
        }
      }
    }
  }
  int ExitValue;
};

inline int CompareVectors(vtkDataArray* array, vtkDataArray* expectedArray)
{
  using Dispatcher = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::AllTypes>;

  // Create the functor:
  CompareVectorWorker worker;

  if (!Dispatcher::Execute(array, expectedArray, worker))
  {
    // If Execute(...) fails, the arrays don't match the constraints.
    // Run the algorithm using the slower vtkDataArray double API instead:
    worker(array, expectedArray);
  }
  return worker.ExitValue;
}

struct ArrayTypeTester
{
  template <class ArrayT1, class ArrayT2>
  void operator()(ArrayT1*, ArrayT2*)
  {
    using ValueType1 = typename ArrayT1::ValueType;
    using ValueType2 = typename ArrayT2::ValueType;
    this->ArraysArePointerCompatible = (sizeof(ValueType1) == sizeof(ValueType2)) &&
      (std::is_integral<ValueType1>::value == std::is_integral<ValueType2>::value);
  }

  vtkAbstractArray* Array;
  bool ArraysArePointerCompatible;
};

inline int TestDataSet(vtkDataSet* data, vtkDataSet* expectedData, bool includeFieldData = false)
{
  if (data == nullptr || expectedData == nullptr)
  {
    std::cerr << "Error: Data not in the format expected." << std::endl;
    return EXIT_FAILURE;
  }

  if (data->GetNumberOfPoints() != expectedData->GetNumberOfPoints())
  {
    std::cerr << "Expecting " << expectedData->GetNumberOfPoints()
              << " points but got: " << data->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  if (data->GetNumberOfCells() != expectedData->GetNumberOfCells())
  {
    std::cerr << "Expecting " << expectedData->GetNumberOfCells()
              << " cells but got: " << data->GetNumberOfCells() << std::endl;
    return EXIT_FAILURE;
  }
  for (int attributeType = 0; attributeType < vtkDataObject::FIELD + (includeFieldData ? 1 : 0);
       ++attributeType)
  {
    int numberRead = data->GetAttributesAsFieldData(attributeType)->GetNumberOfArrays();
    int numberExpected = expectedData->GetAttributesAsFieldData(attributeType)->GetNumberOfArrays();
    if (numberRead != numberExpected)
    {
      std::cerr << "Expecting " << numberExpected << " arrays of type " << attributeType
                << " but got " << numberRead << std::endl;
      return EXIT_FAILURE;
    }
    vtkFieldData* fieldData = data->GetAttributesAsFieldData(attributeType);
    vtkFieldData* expectedFieldData = expectedData->GetAttributesAsFieldData(attributeType);
    for (int i = 0; i < numberRead; ++i)
    {
      // Arrays don't have to be in the same order
      // Arrays with the same name have to match
      vtkDataArray* expectedArray = expectedFieldData->GetArray(i);
      vtkDataArray* array = fieldData->GetArray(expectedArray->GetName());

      using Dispatcher = vtkArrayDispatch::Dispatch2;
      ArrayTypeTester tester;
      Dispatcher::Execute(array, expectedArray, tester);
      if (!tester.ArraysArePointerCompatible)
      {
        vtkLog(ERROR,
          "Read array and expected arrays do not have compatible pointers for "
            << expectedArray->GetName() << "."
            << " Read array: " << array->GetClassName()
            << " Expected array: " << expectedArray->GetClassName());
        return EXIT_FAILURE;
      }

      if (array->GetNumberOfTuples() != expectedArray->GetNumberOfTuples() ||
        array->GetNumberOfComponents() != expectedArray->GetNumberOfComponents())
      {
        std::cerr << "Array " << array->GetName() << " has a different number of "
                  << "tuples/components: " << array->GetNumberOfTuples() << "/"
                  << array->GetNumberOfComponents()
                  << " than expected: " << expectedArray->GetNumberOfTuples() << "/"
                  << expectedArray->GetNumberOfComponents() << std::endl;
        return EXIT_FAILURE;
      }
      vtkDataArray* a = vtkDataArray::SafeDownCast(array);
      vtkDataArray* ea = vtkDataArray::SafeDownCast(expectedArray);
      if (a)
      {
        if (CompareVectors(a, ea))
        {
          return EXIT_FAILURE;
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

VTK_ABI_NAMESPACE_END
} // namespace vtk

#endif
// VTK-HeaderTest-Exclude: vtkDataTestUtilities.h
