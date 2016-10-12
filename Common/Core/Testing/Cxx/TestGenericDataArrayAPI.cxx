/*==============================================================================

  Program:   Visualization Toolkit
  Module:    TestGenericDataArrayAPI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

#include "vtkGenericDataArray.h"

// Helpers:
#include "vtkAOSDataArrayTemplate.h"
#include "vtkArrayDispatch.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <string>
#include <typeinfo>
#include <vector>

// Concrete classes for testing:
#include "vtkAOSDataArrayTemplate.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

// About this test:
//
// This test runs a battery of unit tests that exercise the vtkGenericDataArray
// API on concrete implementations of their subclasses. It is designed to be
// easily extended to cover new array implementations and additional unit tests.
//
// This test has three main components:
// - Entry point: TestGenericDataArrayAPI(). Add new array classes here.
// - Unit test caller: ExerciseGenericDataArray(). Templated on value and array
//   types. Calls individual unit test functions to excerise the array methods.
//   Add new unit test calls here.
// - Unit test functions: Test_[methodSignature](). Templated on value type,
//   array type, and possibly other parameters to simplify implementations.
//   These should use the DataArrayAPI macros as needed

// Forward declare the test function:
namespace {
template <typename ScalarT, typename ArrayT> int ExerciseGenericDataArray();
} // end anon namespace

//------------------------------------------------------------------------------
//-------------Test Entry Point-------------------------------------------------
//------------------------------------------------------------------------------

int TestGenericDataArrayAPI(int, char *[])
{
  int errors = 0;

  // Add array classes here:
  // Defaults:
  errors += ExerciseGenericDataArray<char,               vtkCharArray>();
  errors += ExerciseGenericDataArray<double,             vtkDoubleArray>();
  errors += ExerciseGenericDataArray<float,              vtkFloatArray>();
  errors += ExerciseGenericDataArray<int,                vtkIntArray>();
  errors += ExerciseGenericDataArray<long,               vtkLongArray>();
  errors += ExerciseGenericDataArray<long long,          vtkLongLongArray>();
  errors += ExerciseGenericDataArray<short,              vtkShortArray>();
  errors += ExerciseGenericDataArray<signed char,        vtkSignedCharArray>();
  errors += ExerciseGenericDataArray<unsigned char,      vtkUnsignedCharArray>();
  errors += ExerciseGenericDataArray<unsigned int,       vtkUnsignedIntArray>();
  errors += ExerciseGenericDataArray<unsigned long,      vtkUnsignedLongArray>();
  errors += ExerciseGenericDataArray<unsigned long long, vtkUnsignedLongLongArray>();
  errors += ExerciseGenericDataArray<unsigned short,     vtkUnsignedShortArray>();
  errors += ExerciseGenericDataArray<vtkIdType,          vtkIdTypeArray>();

  // Explicit AoS arrays:
  errors += ExerciseGenericDataArray<char,               vtkAOSDataArrayTemplate<char> >();
  errors += ExerciseGenericDataArray<double,             vtkAOSDataArrayTemplate<double> >();
  errors += ExerciseGenericDataArray<float,              vtkAOSDataArrayTemplate<float> >();
  errors += ExerciseGenericDataArray<int,                vtkAOSDataArrayTemplate<int> >();
  errors += ExerciseGenericDataArray<long,               vtkAOSDataArrayTemplate<long> >();
  errors += ExerciseGenericDataArray<long long,          vtkAOSDataArrayTemplate<long long> >();
  errors += ExerciseGenericDataArray<short,              vtkAOSDataArrayTemplate<short> >();
  errors += ExerciseGenericDataArray<signed char,        vtkAOSDataArrayTemplate<signed char> >();
  errors += ExerciseGenericDataArray<unsigned char,      vtkAOSDataArrayTemplate<unsigned char> >();
  errors += ExerciseGenericDataArray<unsigned int,       vtkAOSDataArrayTemplate<unsigned int> >();
  errors += ExerciseGenericDataArray<unsigned long,      vtkAOSDataArrayTemplate<unsigned long> >();
  errors += ExerciseGenericDataArray<unsigned long long, vtkAOSDataArrayTemplate<unsigned long long> >();
  errors += ExerciseGenericDataArray<unsigned short,     vtkAOSDataArrayTemplate<unsigned short> >();
  errors += ExerciseGenericDataArray<vtkIdType,          vtkAOSDataArrayTemplate<vtkIdType> >();

  // Explicit SoA arrays:
  errors += ExerciseGenericDataArray<char,               vtkSOADataArrayTemplate<char> >();
  errors += ExerciseGenericDataArray<double,             vtkSOADataArrayTemplate<double> >();
  errors += ExerciseGenericDataArray<float,              vtkSOADataArrayTemplate<float> >();
  errors += ExerciseGenericDataArray<int,                vtkSOADataArrayTemplate<int> >();
  errors += ExerciseGenericDataArray<long,               vtkSOADataArrayTemplate<long> >();
  errors += ExerciseGenericDataArray<long long,          vtkSOADataArrayTemplate<long long> >();
  errors += ExerciseGenericDataArray<short,              vtkSOADataArrayTemplate<short> >();
  errors += ExerciseGenericDataArray<signed char,        vtkSOADataArrayTemplate<signed char> >();
  errors += ExerciseGenericDataArray<unsigned char,      vtkSOADataArrayTemplate<unsigned char> >();
  errors += ExerciseGenericDataArray<unsigned int,       vtkSOADataArrayTemplate<unsigned int> >();
  errors += ExerciseGenericDataArray<unsigned long,      vtkSOADataArrayTemplate<unsigned long> >();
  errors += ExerciseGenericDataArray<unsigned long long, vtkSOADataArrayTemplate<unsigned long long> >();
  errors += ExerciseGenericDataArray<unsigned short,     vtkSOADataArrayTemplate<unsigned short> >();
  errors += ExerciseGenericDataArray<vtkIdType,          vtkSOADataArrayTemplate<vtkIdType> >();

  if (errors > 0)
  {
    std::cerr << "Test failed! Error count: " << errors << std::endl;
  }
  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

//------------------------------------------------------------------------------
//------------Unit Test Macros--------------------------------------------------
//------------------------------------------------------------------------------

#define DataArrayAPIInit(_signature) \
  int errors = 0; \
  std::string signature = _signature

#define DataArrayAPIUpdateSignature(_signature) \
  signature = _signature

#define DataArrayAPIFinish() return errors

#define DataArrayAPICreateTestArray(name) vtkNew<ArrayT> name

#define DataArrayAPICreateReferenceArray(name) \
  vtkSmartPointer<vtkDataArray> name##DA = CreateDataArray<ScalarT>(); \
  vtkAOSDataArrayTemplate<ScalarT> *name = \
  vtkAOSDataArrayTemplate<ScalarT>::SafeDownCast(name##DA.GetPointer()); \
  assert("Reference array is vtkAOSDataArrayTemplate" && name != NULL)

#define DataArrayAPICreateReferenceArrayWithType(name, valueType) \
  vtkSmartPointer<vtkDataArray> name##DA = CreateDataArray<valueType>(); \
  vtkAOSDataArrayTemplate<valueType> *name = \
    vtkAOSDataArrayTemplate<valueType>::SafeDownCast(name##DA.GetPointer()); \
  assert("Reference array is vtkAOSDataArrayTemplate" && name != NULL)

#define DataArrayAPINonFatalError(x) \
  { \
    ArrayT *errorTempArray = ArrayT::New(); \
    std::cerr << "Line " << __LINE__ << ": " \
              << "Failure in test of '" << signature << "' " \
              << "for array type '" << errorTempArray->GetClassName() << "'" \
              << ":\n" << x << std::endl; \
    errorTempArray->Delete(); \
    ++errors; \
  }

#define DataArrayAPIError(x) \
  DataArrayAPINonFatalError(x) \
  return errors;

namespace {

// Convenience function to create a concrete data array from a template type:
template <typename ScalarT>
vtkSmartPointer<vtkDataArray> CreateDataArray()
{
  vtkSmartPointer<vtkDataArray> array;
  array.TakeReference(vtkDataArray::CreateDataArray(
                        vtkTypeTraits<ScalarT>::VTK_TYPE_ID));
  assert("CreateArray failed for scalar type." && array.GetPointer());
  return array;
}

//------------------------------------------------------------------------------
//------------------Unit Test Implementations-----------------------------------
//------------------------------------------------------------------------------

// ValueType GetValue(vtkIdType valueIdx) const
// No range checking/allocation.
template <typename ScalarT, typename ArrayT>
int Test_valT_GetValue_valueIdx_const()
{
  DataArrayAPIInit("ValueType GetValue(vtkIdType valueIdx) const");

  DataArrayAPICreateTestArray(array);
  vtkIdType comps = 9;
  vtkIdType tuples = 5;
  array->SetNumberOfComponents(comps);
  array->SetNumberOfTuples(tuples);

  // Initialize:
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    array->SetValue(i, static_cast<ScalarT>(i % 16));
  }

  // Verify:
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    ScalarT test = array->GetValue(i);
    ScalarT ref = static_cast<ScalarT>(i % 16);
    if (test != ref)
    {
      DataArrayAPIError("Data mismatch at value index '" << i << "'. Expected '"
                        << ref << "', got '" << test << "'.");
    }
  }

  DataArrayAPIFinish();
}

//void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
template <typename ScalarT, typename ArrayT>
int Test_void_GetTypedTuple_tupleIdx_tuple()
{
  DataArrayAPIInit("void GetTypedTuple(vtkIdType tupleIdx, ValueType *tuple)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    source->SetValue(i, static_cast<ScalarT>(i % 17));
  }

  // Test the returned tuples:
  vtkIdType refValue = 0;
  std::vector<ScalarT> tuple(comps);
  for (vtkIdType tupleIdx = 0; tupleIdx < tuples; ++tupleIdx)
  {
    source->GetTypedTuple(tupleIdx, &tuple[0]);
    for (int compIdx = 0; compIdx < comps; ++compIdx)
    {
      if (tuple[compIdx] != static_cast<ScalarT>(refValue))
      {
        DataArrayAPIError("Data mismatch at tuple " << tupleIdx << ", "
                          "component " << compIdx << ": Expected '" << refValue
                          << "', got '" << tuple[compIdx] << "'.");
      }
      ++refValue;
      refValue %= 17;
    }
  }

  DataArrayAPIFinish();
}

// ValueType GetTypedComponent(vtkIdType tupleIdx, int comp) const
template <typename ScalarT, typename ArrayT>
int Test_valT_GetTypedComponent_tupleIdx_comp_const()
{
  DataArrayAPIInit("ValueType GetTypedComponent("
                   "vtkIdType tupleIdx, int comp) const");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    source->SetValue(i, static_cast<ScalarT>(i % 17));
  }

  // Test the returned tuples:
  vtkIdType refValue = 0;
  for (vtkIdType i = 0; i < tuples; ++i)
  {
    for (int j = 0; j < comps; ++j)
    {
      if (source->GetTypedComponent(i, j) != static_cast<ScalarT>(refValue))
      {
        DataArrayAPIError("Data mismatch at tuple " << i << ", "
                          "component " << j << ": Expected '" << refValue
                          << "', got '" << source->GetTypedComponent(i, j)
                          << "'.");
      }
      ++refValue;
      refValue %= 17;
    }
  }

  DataArrayAPIFinish();
}

// void SetValue(vtkIdType valueIdx, ValueType value)
template <typename ScalarT, typename ArrayT>
int Test_void_SetValue_valueIdx_value()
{
  DataArrayAPIInit("void SetValue(vtkIdType valueIdx, ValueType value)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array using tested function:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    source->SetValue(i, static_cast<ScalarT>(((i + 1) * (i + 2)) % 17));
  }

  // Validate:
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    ScalarT ref = static_cast<ScalarT>(((i + 1) * (i + 2)) % 17);
    const typename ArrayT::ValueType test = source->GetValue(i);
    if (ref != test)
    {
      DataArrayAPIError("Data mismatch at value " << i << ": Expected '"
                        << ref << "', got '" << test << "'.");
    }
  }

  DataArrayAPIFinish();
}

// void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
template <typename ScalarT, typename ArrayT>
int Test_void_SetTypedTuple_tupleIdx_tuple()
{
  DataArrayAPIInit("void SetTypedTuple(vtkIdType tupleIdx, "
                   "const ValueType* tuple)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 5;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
  {
    std::vector<ScalarT> tuple;
    for (int c = 0; c < comps; ++c)
    {
      tuple.push_back(static_cast<ScalarT>(((t * comps) + c) % 17));
    }
    source->SetTypedTuple(t, &tuple[0]);
  }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
  {
    for (int c = 0; c < comps; ++c)
    {
      ScalarT ref = static_cast<ScalarT>(((t * comps) + c) % 17);
      ScalarT test = source->GetTypedComponent(t, c);
      if (ref != test)
      {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected " << ref << ", got " << test << ".");
      }
    }
  }

  DataArrayAPIFinish();
}

// void SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
template <typename ScalarT, typename ArrayT>
int Test_void_SetTypedComponent_tupleIdx_comp_value()
{
  DataArrayAPIInit("void SetTypedComponent(vtkIdType tupleIdx, int comp, "
                   "ValueType value)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array using tested function:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < tuples; ++i)
  {
    for (int j = 0; j < comps; ++j)
    {
      source->SetTypedComponent(i, j,
                                static_cast<ScalarT>(((i + 1) * (j + 1)) % 17));
    }
  }

  // Test the returned tuples:
  std::vector<ScalarT> tuple(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
  {
    source->GetTypedTuple(i, &tuple[0]);
    for (int j = 0; j < comps; ++j)
    {
      ScalarT test = tuple[j];
      ScalarT ref = static_cast<ScalarT>((i + 1) * (j + 1) % 17);
      if (ref != test)
      {
        DataArrayAPIError("Data mismatch at tuple " << i << ", component " << j
                          << ": Expected '" << ref << "', got '" << test
                          << "'.");
      }
    }
  }

  DataArrayAPIFinish();
}

// vtkIdType LookupTypedValue(ValueType value)
// void LookupTypedValue(ValueType value, vtkIdList* ids)
template <typename ScalarT, typename ArrayT>
int Test_LookupTypedValue_allSigs()
{
  DataArrayAPIInit("LookupTypedValue");

  DataArrayAPICreateTestArray(array);

  // Map Value --> ValueIdxs. We'll use this to validate the lookup results.
  typedef std::map<ScalarT, vtkIdList*> RefMap;
  typedef typename RefMap::iterator RefMapIterator;
  RefMap refMap;
  // These are the values we'll be looking for.
  for (ScalarT val = 0; val < 17; ++val)
  {
    refMap.insert(std::make_pair(val, vtkIdList::New()));
  }

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  array->SetNumberOfComponents(comps);
  array->SetNumberOfTuples(tuples);
  for (vtkIdType valIdx = 0; valIdx < comps * tuples; ++valIdx)
  {
    ScalarT val = static_cast<ScalarT>(valIdx % 17);
    array->SetValue(valIdx, val);
    // Update our reference map:
    RefMapIterator it = refMap.find(val);
    assert("Value exists in reference map." && it != refMap.end());
    it->second->InsertNextId(valIdx);
  }

  // Test the lookup functions.
  vtkNew<vtkIdList> testIdList;
  for (RefMapIterator it = refMap.begin(), itEnd = refMap.end(); it != itEnd;
       ++it)
  {
    const ScalarT &val = it->first;
    vtkIdList *refIdList = it->second; // Presorted due to insertion order
    vtkIdType *refIdBegin = refIdList->GetPointer(0);
    vtkIdType *refIdEnd = refIdList->GetPointer(refIdList->GetNumberOfIds());

    // Docs are unclear about this. Does it return the first value, or just any?
    // We'll assume any since it's unspecified.
    DataArrayAPIUpdateSignature("vtkIdType LookupTypedValue(ValueType value)");
    vtkIdType testId = array->LookupTypedValue(val);
    if (!std::binary_search(refIdBegin, refIdEnd, testId))
    {
      // NonFatal + break so we can clean up.
      DataArrayAPINonFatalError("Looking up value '" << val
                                << "' returned valueIdx '" << testId
                                << "', which maps to value '"
                                << array->GetValue(testId) << "'.");
      break;
    }

    // Now for the list overload:
    DataArrayAPIUpdateSignature(
          "void LookupTypedValue(ValueType value, vtkIdList* ids)");
    array->LookupTypedValue(val, testIdList.GetPointer());
    if (testIdList->GetNumberOfIds() != refIdList->GetNumberOfIds())
    {
      // NonFatal + break so we can clean up.
      DataArrayAPINonFatalError("Looking up value '" << val << "' returned "
                                << testIdList->GetNumberOfIds() << " ids, but "
                                << refIdList->GetNumberOfIds()
                                << "were expected.");
      break;
    }
    vtkIdType *testIdBegin = testIdList->GetPointer(0);
    vtkIdType *testIdEnd = testIdList->GetPointer(refIdList->GetNumberOfIds());
    // Ensure the test ids are sorted
    std::sort(testIdBegin, testIdEnd);
    if (!std::equal(testIdBegin, testIdEnd, refIdBegin))
    {
      // NonFatal + break so we can clean up.
      DataArrayAPINonFatalError("Looking up all value indices for value '"
                                << val
                                << "' did not return the expected result.");
      break;
    }
  }

  // Cleanup:
  for (RefMapIterator it = refMap.begin(), itEnd = refMap.end(); it != itEnd;
       ++it)
  {
    it->second->Delete();
    it->second = NULL;
  }

  DataArrayAPIFinish();
}

// vtkIdType InsertNextValue(ValueType v)
template <typename ScalarT, typename ArrayT>
int Test_vtkIdType_InsertNextValue_v()
{
  DataArrayAPIInit("vtkIdType InsertNextValue(ValueType v)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array using tested function:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    vtkIdType insertLoc = source->InsertNextValue(static_cast<ScalarT>(i % 17));
    if (insertLoc != i)
    {
      DataArrayAPIError("Returned location incorrect. Expected '" << i
                        << "', got '" << insertLoc << "'.");
    }
    if (source->GetSize() < i + 1)
    {
      DataArrayAPIError("Size should be at least " << i + 1
                        << " values, but is only " << source->GetSize() << ".");
    }
    if (source->GetMaxId() != i)
    {
      DataArrayAPIError("MaxId should be " << i << ", but is "
                        << source->GetMaxId() << " instead.");
    }
  }

  // Validate:
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    ScalarT ref = static_cast<ScalarT>(i % 17);
    const typename ArrayT::ValueType test = source->GetValue(i);
    if (ref != test)
    {
      DataArrayAPIError("Data mismatch at value " << i << ": Expected '"
                        << ref << "', got '" << test << "'.");
    }
  }

  DataArrayAPIFinish();
}

// void InsertValue(vtkIdType idx, ValueType v)
template <typename ScalarT, typename ArrayT>
int Test_void_InsertValue_idx_v()
{
  DataArrayAPIInit("void InsertValue(vtkIdType idx, ValueType v)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array using tested function:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    source->InsertValue(i, static_cast<ScalarT>(i % 17));

    if (source->GetSize() < i + 1)
    {
      DataArrayAPIError("Size should be at least " << i + 1
                        << " values, but is only " << source->GetSize() << ".");
    }
    if (source->GetMaxId() != i)
    {
      DataArrayAPIError("MaxId should be " << i << ", but is "
                        << source->GetMaxId() << " instead.");
    }
  }

  // Validate:
  for (vtkIdType i = 0; i < comps * tuples; ++i)
  {
    ScalarT ref = static_cast<ScalarT>(i % 17);
    const typename ArrayT::ValueType test = source->GetValue(i);
    if (ref != test)
    {
      DataArrayAPIError("Data mismatch at value " << i << ": Expected '"
                        << ref << "', got '" << test << "'.");
    }
  }

  DataArrayAPIFinish();
}

// void InsertTypedTuple(vtkIdType idx, const ValueType *t)
template <typename ScalarT, typename ArrayT>
int Test_void_InsertTypedTuple_idx_t()
{
  DataArrayAPIInit("void InsertTypedTuple(vtkIdType idx, const ValueType *t)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 5;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType t = 0; t < tuples; ++t)
  {
    std::vector<ScalarT> tuple;
    for (int c = 0; c < comps; ++c)
    {
      tuple.push_back(static_cast<ScalarT>(((t * comps) + c) % 17));
    }
    source->InsertTypedTuple(t, &tuple[0]);
    if (source->GetSize() < ((t + 1) * comps))
    {
      DataArrayAPIError("Size should be at least " << ((t + 1) * comps)
                        << " values, but is only " << source->GetSize() << ".");
    }
    if (source->GetMaxId() != ((t + 1) * comps) - 1)
    {
      DataArrayAPIError("MaxId should be " << ((t + 1) * comps) - 1
                        << ", but is " << source->GetMaxId() << " instead.");
    }
  }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
  {
    for (int c = 0; c < comps; ++c)
    {
      if (source->GetTypedComponent(t, c) !=
          static_cast<ScalarT>(((t * comps) + c) % 17))
      {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected "
                          << static_cast<ScalarT>(((t * comps) + c) % 17)
                          << ", got " << source->GetTypedComponent(t, c)
                          << ".");
      }
    }
  }

  DataArrayAPIFinish();
}

// vtkIdType InsertNextTypedTuple(const ValueType *t)
template <typename ScalarT, typename ArrayT>
int Test_vtkIdType_InsertNextTypedTuple_t()
{
  DataArrayAPIInit("vtkIdType InsertNextTypedTuple(const ValueType *t)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 5;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType t = 0; t < tuples; ++t)
  {
    std::vector<ScalarT> tuple;
    for (int c = 0; c < comps; ++c)
    {
      tuple.push_back(static_cast<ScalarT>(((t * comps) + c) % 17));
    }
    vtkIdType insertLoc = source->InsertNextTypedTuple(&tuple[0]);
    if (insertLoc != t)
    {
      DataArrayAPIError("Returned location incorrect. Expected '" << t
                        << "', got '" << insertLoc << "'.");
    }
    if (source->GetSize() < ((t + 1) * comps))
    {
      DataArrayAPIError("Size should be at least " << ((t + 1) * comps)
                        << " values, but is only " << source->GetSize() << ".");
    }
    if (source->GetMaxId() != ((t + 1) * comps) - 1)
    {
      DataArrayAPIError("MaxId should be " << ((t + 1) * comps) - 1
                        << ", but is " << source->GetMaxId() << " instead.");
    }
  }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
  {
    for (int c = 0; c < comps; ++c)
    {
      if (source->GetTypedComponent(t, c) !=
          static_cast<ScalarT>(((t * comps) + c) % 17))
      {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected "
                          << static_cast<ScalarT>(((t * comps) + c) % 17)
                          << ", got " << source->GetTypedComponent(t, c)
                          << ".");
      }
    }
  }

  DataArrayAPIFinish();
}

// vtkIdType GetNumberOfValues()
template <typename ScalarT, typename ArrayT>
int Test_vtkIdType_GetNumberOfValues()
{
  DataArrayAPIInit("vtkIdType InsertNextTypedTuple(const ValueType *t)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 5;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);

  if (source->GetNumberOfValues() != comps * tuples)
  {
    DataArrayAPIError("Returned number of values: "
                      << source->GetNumberOfValues() << ", expected "
                      << (comps * tuples) << ".");
  }

  DataArrayAPIFinish();
}

// ValueType *GetValueRange()
// void GetValueRange(ValueType range[2])
// ValueType *GetValueRange(int comp)
// void GetValueRange(ValueType range[2], int comp)
template <typename ScalarT, typename ArrayT>
int Test_GetValueRange_all_overloads()
{
  DataArrayAPIInit("GetValueRange");

  DataArrayAPICreateTestArray(array);

  // Initialize arrays:
  vtkIdType comps = 6;
  vtkIdType tuples = 9;
  array->SetNumberOfComponents(comps);
  array->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
  {
    for (int c = 0; c < comps; ++c)
    {
      array->SetComponent(t, c, (t + 1) * (c + 1));
    }
  }

  // Just the range of the first component:
  DataArrayAPIUpdateSignature("ValueType* GetValueRange()");
  ScalarT *rangePtr = array->GetValueRange();
  ScalarT expectedRange[2] = { static_cast<ScalarT>(1),
                               static_cast<ScalarT>(tuples) };
  if (rangePtr[0] != expectedRange[0] ||
      rangePtr[1] != expectedRange[1])
  {
    DataArrayAPINonFatalError("First component range expected to be: ["
                              << expectedRange[0] << ", " << expectedRange[1]
                              << "], got [" << rangePtr[0] << ", "
                              << rangePtr[1] << "].");
  }

  DataArrayAPIUpdateSignature("void GetValueRange(ValueType range[2])");
  ScalarT rangeArray[2];
  array->GetValueRange(rangeArray);
  if (rangeArray[0] != expectedRange[0] ||
      rangeArray[1] != expectedRange[1])
  {
    DataArrayAPINonFatalError("First component range expected to be: ["
                              << expectedRange[0] << ", " << expectedRange[1]
                              << "], got [" << rangeArray[0] << ", "
                              << rangeArray[1] << "].");
  }

  DataArrayAPIUpdateSignature("ValueType* GetValueRange(int comp)");
  for (int c = 0; c < comps; ++c)
  {
    expectedRange[0] = static_cast<ScalarT>(c + 1);
    expectedRange[1] = static_cast<ScalarT>(tuples * (c + 1));
    rangePtr = array->GetValueRange(c);
    if (rangePtr[0] != expectedRange[0] ||
        rangePtr[1] != expectedRange[1])
    {
      DataArrayAPINonFatalError("Component " << c << " range expected to be: ["
                                << expectedRange[0] << ", " << expectedRange[1]
                                << "], got [" << rangePtr[0] << ", "
                                << rangePtr[1] << "].");
    }
  }

  DataArrayAPIUpdateSignature("void GetValueRange(ValueType range[2], int comp)");
  for (int c = 0; c < comps; ++c)
  {
    expectedRange[0] = static_cast<ScalarT>(c + 1);
    expectedRange[1] = static_cast<ScalarT>(tuples * (c + 1));
    array->GetValueRange(rangeArray, c);
    if (rangeArray[0] != expectedRange[0] ||
        rangeArray[1] != expectedRange[1])
    {
      DataArrayAPINonFatalError("Component " << c << " range expected to be: ["
                                << expectedRange[0] << ", " << expectedRange[1]
                                << "], got [" << rangeArray[0] << ", "
                                << rangeArray[1] << "].");
    }
  }

  DataArrayAPIFinish();
}

//------------------------------------------------------------------------------
//-----------Unit Test Function Caller------------------------------------------
//------------------------------------------------------------------------------

template <typename ScalarT, typename ArrayT>
int ExerciseGenericDataArray()
{
  int errors = 0;

  errors += Test_valT_GetValue_valueIdx_const<ScalarT, ArrayT>();
  errors += Test_void_GetTypedTuple_tupleIdx_tuple<ScalarT, ArrayT>();
  errors += Test_valT_GetTypedComponent_tupleIdx_comp_const<ScalarT, ArrayT>();
  errors += Test_void_SetValue_valueIdx_value<ScalarT, ArrayT>();
  errors += Test_void_SetTypedTuple_tupleIdx_tuple<ScalarT, ArrayT>();
  errors += Test_void_SetTypedComponent_tupleIdx_comp_value<ScalarT, ArrayT>();
  errors += Test_LookupTypedValue_allSigs<ScalarT, ArrayT>();
  errors += Test_vtkIdType_InsertNextValue_v<ScalarT, ArrayT>();
  errors += Test_void_InsertValue_idx_v<ScalarT, ArrayT>();
  errors += Test_void_InsertTypedTuple_idx_t<ScalarT, ArrayT>();
  errors += Test_vtkIdType_InsertNextTypedTuple_t<ScalarT, ArrayT>();
  errors += Test_vtkIdType_GetNumberOfValues<ScalarT, ArrayT>();
  errors += Test_GetValueRange_all_overloads<ScalarT, ArrayT>();

  return errors;
} // end ExerciseDataArray

} // end anon namespace

#undef DataArrayAPIInit
#undef DataArrayAPIUpdateSignature
#undef DataArrayAPIFinish
#undef DataArrayAPICreateTestArray
#undef DataArrayAPICreateReferenceArray
#undef DataArrayAPICreateReferenceArrayWithType
#undef DataArrayAPINonFatalError
#undef DataArrayAPIError
