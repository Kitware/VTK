/*==============================================================================

  Program:   Visualization Toolkit
  Module:    TestDataArrayAPI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

#include "vtkDataArray.h"

// Helpers:
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTypedDataArray.h"
#include "vtkTypeTraits.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

// Concrete classes for testing:
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

// About this test:
//
// This test runs a battery of unit tests that exercise the vtkDataArray API
// on concrete implementations of their subclasses. It is designed to be easily
// extended to cover new array implementations and addition unit tests.
//
// This test has three main components:
// - Entry point: TestDataArrayAPI(). Add new array classes here.
// - Unit test caller: ExerciseDataArray(). Templated on value and array types.
//   Calls individual unit test functions to excerise the array methods.
//   Add new unit test calls here.
// - Unit test functions: Test_[methodSignature](). Templated on value type,
//   array type, and possibly other parameters to simplify implementations.
//   These should use the DataArrayAPI macros as needed


// Forward declare the test function:
namespace {
template <typename ScalarT, typename ArrayT>
int ExerciseDataArray();
} // end anon namespace

//------------------------------------------------------------------------------
//-------------Test Entry Point-------------------------------------------------
//------------------------------------------------------------------------------

int TestDataArrayAPI(int, char *[])
{
  int errors = 0;

  // Add array classes here:
  // Defaults:
  errors += ExerciseDataArray<char, vtkCharArray>();
  errors += ExerciseDataArray<float, vtkFloatArray>();
  errors += ExerciseDataArray<double, vtkDoubleArray>();
  errors += ExerciseDataArray<vtkIdType, vtkIdTypeArray>();
  errors += ExerciseDataArray<int, vtkIntArray>();
  errors += ExerciseDataArray<long, vtkLongArray>();
  errors += ExerciseDataArray<long long, vtkLongLongArray>();
  errors += ExerciseDataArray<short, vtkShortArray>();
  errors += ExerciseDataArray<signed char, vtkSignedCharArray>();
  errors += ExerciseDataArray<unsigned char, vtkUnsignedCharArray>();
  errors += ExerciseDataArray<unsigned int, vtkUnsignedIntArray>();
  errors += ExerciseDataArray<unsigned long, vtkUnsignedLongArray>();
  errors += ExerciseDataArray<unsigned long long, vtkUnsignedLongLongArray>();
  errors += ExerciseDataArray<unsigned short, vtkUnsignedShortArray>();

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
  vtkTypedDataArray<ScalarT> *name = \
  vtkTypedDataArray<ScalarT>::SafeDownCast(name##DA.GetPointer()); \
  assert("Reference array is vtkTypedDataArray" && name != NULL)

#define DataArrayAPICreateReferenceArrayWithType(name, valueType) \
  vtkSmartPointer<vtkDataArray> name##DA = CreateDataArray<valueType>(); \
  vtkTypedDataArray<valueType> *name = \
    vtkTypedDataArray<valueType>::SafeDownCast(name##DA.GetPointer()); \
  assert("Reference array is vtkTypedDataArray" && name != NULL)

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

// void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);
// Set the tuple at i in this array using tuple j from source.
// Types must match.
// No range checking/allocation.
template <typename ScalarT, typename ArrayT>
int Test_void_SetTuple_i_j_source()
{
  DataArrayAPIInit(
        "void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source)");

  DataArrayAPICreateTestArray(dest);
  DataArrayAPICreateReferenceArray(source);
  vtkIdType comps = 9;
  vtkIdType tuples = 5;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);

  // Initialize source:
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source->SetValue(i, static_cast<ScalarT>(i % 16));
    }

  // Use SetTuple to populate dest. Tuple ordering is changed according to
  // tupleMap (destTuple = tupleMap[srcTuple])
  vtkIdType tupleMap[5] = {1, 0, 3, 4, 2};
  dest->SetNumberOfComponents(comps);
  dest->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    dest->SetTuple(tupleMap[i], i, source);
    }

  // Verify:
  std::vector<ScalarT> srcTuple(comps);
  std::vector<ScalarT> destTuple(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    source->GetTupleValue(i, &srcTuple[0]);
    dest->GetTupleValue(tupleMap[i], &destTuple[0]);
    if (!std::equal(srcTuple.begin(), srcTuple.end(), destTuple.begin()))
      {
      std::ostringstream srcTupleStr;
      std::ostringstream destTupleStr;
      for (int j = 0; j < comps; ++j)
        {
        srcTupleStr << srcTuple[j] << " ";
        destTupleStr << destTuple[j] << " ";
        }
      DataArrayAPIError("Data mismatch at source tuple '" << i << "' and "
                        "destination tuple '" << tupleMap[i] << "':\n"
                        "src: " << srcTupleStr.str() << "\n"
                        "dest: " << destTupleStr.str() << "\n");
      }
    }

  DataArrayAPIFinish();
}

// void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source)
// Insert the jth tuple in the source array, at ith location in this array.
// Allocates memory as needed.
template <typename ScalarT, typename ArrayT>
int Test_void_InsertTuple_i_j_source()
{
  DataArrayAPIInit("void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source)");

  DataArrayAPICreateTestArray(dest);
  DataArrayAPICreateReferenceArray(source);
  vtkIdType comps = 9;
  vtkIdType tuples = 5;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);

  // Initialize source:
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source->SetValue(i, static_cast<ScalarT>(i % 16));
    }

  // Use InsertTuple to populate dest. Tuple ordering is changed according to
  // tupleMap (destTuple = tupleMap[srcTuple])
  vtkIdType tupleMap[5] = {1, 0, 3, 4, 2};

  // dest is empty -- this call should allocate memory as needed.
  dest->SetNumberOfComponents(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    dest->InsertTuple(tupleMap[i], i, source);
    }

  // Verify:
  std::vector<ScalarT> srcTuple(comps);
  std::vector<ScalarT> destTuple(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    source->GetTupleValue(i, &srcTuple[0]);
    dest->GetTupleValue(tupleMap[i], &destTuple[0]);
    if (!std::equal(srcTuple.begin(), srcTuple.end(), destTuple.begin()))
      {
      std::ostringstream srcTupleStr;
      std::ostringstream destTupleStr;
      for (int j = 0; j < comps; ++j)
        {
        srcTupleStr << srcTuple[j] << " ";
        destTupleStr << destTuple[j] << " ";
        }
      DataArrayAPIError("Data mismatch at source tuple '" << i << "' and "
                        "destination tuple '" << tupleMap[i] << "':\n"
                        "src: " << srcTupleStr.str() << "\n"
                        "dest: " << destTupleStr.str() << "\n");
      }
    }

  DataArrayAPIFinish();
}

// vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source)
// Insert the jth tuple in the source array at the end of this array.
// Allocate memory as needed.
// Return the tuple index of the inserted data.
template <typename ScalarT, typename ArrayT>
int Test_vtkIdType_InsertNextTuple_j_source()
{
  DataArrayAPIInit(
        "vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source)");

  DataArrayAPICreateTestArray(dest);
  DataArrayAPICreateReferenceArray(source);
  vtkIdType comps = 9;
  vtkIdType tuples = 5;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);

  // Initialize source:
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source->SetValue(i, static_cast<ScalarT>(i % 16));
    }

  // Tuple ordering is changed according to tupleMap via:
  // srcTuple = tupleMap[destTuple]
  vtkIdType tupleMap[5] = {1, 0, 3, 4, 2};

  // dest is empty -- this call should allocate memory as needed.
  dest->SetNumberOfComponents(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    vtkIdType tupleIdx = dest->InsertNextTuple(tupleMap[i], source);
    if (i != tupleIdx)
      {
      DataArrayAPIError("Returned tuple index incorrect. Returned '"
                        << tupleIdx << "', expected '" << i << "'.");
      }
    }

  // Verify:
  std::vector<ScalarT> srcTuple(comps);
  std::vector<ScalarT> destTuple(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    source->GetTupleValue(tupleMap[i], &srcTuple[0]);
    dest->GetTupleValue(i, &destTuple[0]);
    if (!std::equal(srcTuple.begin(), srcTuple.end(), destTuple.begin()))
      {
      std::ostringstream srcTupleStr;
      std::ostringstream destTupleStr;
      for (int j = 0; j < comps; ++j)
        {
        srcTupleStr << srcTuple[j] << " ";
        destTupleStr << destTuple[j] << " ";
        }
      DataArrayAPIError("Data mismatch at source tuple '" << tupleMap[i]
                        << "' and destination tuple '" << i << "':\n"
                        "src: " << srcTupleStr.str() << "\n"
                        "dest: " << destTupleStr.str() << "\n");
      }
    }

  DataArrayAPIFinish();
}

// void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output)
// Output array is preallocated.
template <typename ScalarT, typename ArrayT>
int Test_void_GetTuples_ptIds_output()
{
  DataArrayAPIInit(
        "void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output)");

  DataArrayAPICreateTestArray(source);
  DataArrayAPICreateReferenceArray(output);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source->SetValue(i, static_cast<ScalarT>(i % 17));
    }

  // Initialize the output array and id array. Grab tuples 1, 4, 7, & 10:
  vtkNew<vtkIdList> ids;
  for (vtkIdType tupleId = 1; tupleId < tuples; tupleId += 3)
    {
    ids->InsertNextId(tupleId);
    }
  output->SetNumberOfComponents(comps);
  output->SetNumberOfTuples(ids->GetNumberOfIds());

  // Test the call:
  source->GetTuples(ids.GetPointer(), output);

  // Verify:
  std::vector<ScalarT> srcTuple(comps);
  std::vector<ScalarT> outTuple(comps);
  for (vtkIdType i = 0; i < ids->GetNumberOfIds(); ++i)
    {
    vtkIdType tupleIdx = ids->GetId(i);
    source->GetTupleValue(tupleIdx, &srcTuple[0]);
    output->GetTupleValue(i, &outTuple[0]);
    if (!std::equal(srcTuple.begin(), srcTuple.end(), outTuple.begin()))
      {
      std::ostringstream srcTupleStr;
      std::ostringstream outTupleStr;
      for (int j = 0; j < comps; ++j)
        {
        srcTupleStr << srcTuple[j] << " ";
        outTupleStr << outTuple[j] << " ";
        }
      DataArrayAPIError("Data mismatch at source tuple '" << tupleIdx
                        << "' and output tuple '" << i << "':\n"
                        "src: " << srcTupleStr.str() << "\n"
                        "dest: " << outTupleStr.str() << "\n");
      }
    }

  DataArrayAPIFinish();
}

// void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output)
// Copies p1 --> p2 *inclusive*.
// Output array must be preallocated.
// void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output)
// Output array is preallocated.
template <typename ScalarT, typename ArrayT>
int Test_void_GetTuples_p1_p2_output()
{
  DataArrayAPIInit(
        "void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output)");

  DataArrayAPICreateTestArray(source);
  DataArrayAPICreateReferenceArray(output);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source->SetValue(i, static_cast<ScalarT>(i % 17));
    }

  // Initialize the output array. We'll be grabbing tuples 3 through 8.
  vtkIdType p1 = 3;
  vtkIdType p2 = 8;
  vtkIdType outTupleCount = p2 - p1 + 1; // +1 because the range is inclusive.
  output->SetNumberOfComponents(comps);
  output->SetNumberOfTuples(outTupleCount);

  // Test the call:
  source->GetTuples(p1, p2, output);

  // Verify:
  std::vector<ScalarT> srcTuple(comps);
  std::vector<ScalarT> outTuple(comps);
  for (vtkIdType i = p1; i < outTupleCount; ++i)
    {
    vtkIdType tupleIdx = p1 + i;
    source->GetTupleValue(tupleIdx, &srcTuple[0]);
    output->GetTupleValue(i, &outTuple[0]);
    if (!std::equal(srcTuple.begin(), srcTuple.end(), outTuple.begin()))
      {
      std::ostringstream srcTupleStr;
      std::ostringstream outTupleStr;
      for (int j = 0; j < comps; ++j)
        {
        srcTupleStr << srcTuple[j] << " ";
        outTupleStr << outTuple[j] << " ";
        }
      DataArrayAPIError("Data mismatch at source tuple '" << tupleIdx
                        << "' and output tuple '" << i << "':\n"
                        "src: " << srcTupleStr.str() << "\n"
                        "dest: " << outTupleStr.str() << "\n");
      }
    }

  DataArrayAPIFinish();
}

// double* GetTuple(vtkIdType i)
template <typename ScalarT, typename ArrayT>
int Test_doubleptr_GetTuple_i()
{
  DataArrayAPIInit("double* GetTuple(vtkIdType i)");

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
  for (vtkIdType tupleIdx = 0; tupleIdx < tuples; ++tupleIdx)
    {
    double *tuple = source->GetTuple(tupleIdx);
    for (int compIdx = 0; compIdx < comps; ++compIdx)
      {
      if (tuple[compIdx] != static_cast<double>(refValue))
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

// void GetTuple(vtkIdType i, double *tuple)
// tuple must be large enough, of course
// double* GetTuple(vtkIdType i)
template <typename ScalarT, typename ArrayT>
int Test_void_GetTuple_i_tuple()
{
  DataArrayAPIInit("void GetTuple(vtkIdType i, double *tuple)");

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
  std::vector<double> tuple(comps);
  for (vtkIdType tupleIdx = 0; tupleIdx < tuples; ++tupleIdx)
    {
    source->GetTuple(tupleIdx, &tuple[0]);
    for (int compIdx = 0; compIdx < comps; ++compIdx)
      {
      if (tuple[compIdx] != static_cast<double>(refValue))
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

// double GetComponent(vtkIdType i, int j)
// Return the value at tuple i, component j
template <typename ScalarT, typename ArrayT>
int Test_double_GetComponent_i_j()
{
  DataArrayAPIInit("double GetComponent(vtkIdType i, int j)");

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
      if (source->GetComponent(i, j) != static_cast<double>(refValue))
        {
        DataArrayAPIError("Data mismatch at tuple " << i << ", "
                          "component " << j << ": Expected '" << refValue
                          << "', got '" << source->GetComponent(i, j) << "'.");
        }
      ++refValue;
      refValue %= 17;
      }
    }

  DataArrayAPIFinish();
}

// void SetComponent(vtkIdType i, int j, double c)
// Set tuple i, component j to value c.
// Must preallocate.
template <typename ScalarT, typename ArrayT>
int Test_void_SetComponent_i_j_c()
{
  DataArrayAPIInit("void SetComponent(vtkIdType i, int j, double c)");

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
      source->SetComponent(i, j, static_cast<double>(((i + 1) * (j + 1)) % 17));
      }
    }

  // Test the returned tuples:
  std::vector<double> tuple(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    source->GetTuple(i, &tuple[0]);
    for (int j = 0; j < comps; ++j)
      {
      if (tuple[j] != static_cast<double>((i + 1) * (j + 1) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << i << ", component " << j
                          << ": Expected '" << ((i + 1) * (j + 1) % 17)
                          << "', got '" << tuple[j] << "'.");
        }
      }
    }

  DataArrayAPIFinish();
}

// void InsertComponent(vtkIdType i, int j, double c)
// Set tuple i component j to value c.
// Allocates memory as needed.
template <typename ScalarT, typename ArrayT>
int Test_void_InsertComponent_i_j_c()
{
  DataArrayAPIInit("void InsertComponent(vtkIdType i, int j, double c)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array using tested function:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    for (int j = 0; j < comps; ++j)
      {
      source->InsertComponent(i, j,
                              static_cast<double>(((i + 1) * (j + 1)) % 17));
      }
    }

  // Test the returned tuples:
  std::vector<double> tuple(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    source->GetTuple(i, &tuple[0]);
    for (int j = 0; j < comps; ++j)
      {
      if (tuple[j] != static_cast<double>((i + 1) * (j + 1) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << i << ", component " << j
                          << ": Expected '" << ((i + 1) * (j + 1) % 17)
                          << "', got '" << tuple[j] << "'.");
        }
      }
    }

  DataArrayAPIFinish();
}

// void FillComponent(int j, double c)
// For each tuple, set component j to value c.
template <typename ScalarT, typename ArrayT>
int Test_void_FillComponent_j_c()
{
  DataArrayAPIInit("void FillComponent(int j, double c)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array using tested function:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType j = 0; j < comps; ++j)
    {
    source->FillComponent(j, static_cast<double>(((j + 1) * j) % 17));
    }

  // Test the returned tuples:
  std::vector<double> tuple(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    source->GetTuple(i, &tuple[0]);
    for (int j = 0; j < comps; ++j)
      {
      if (tuple[j] != static_cast<double>(((j + 1) * j) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << i << ", component " << j
                          << ": Expected '" << (((j + 1) * j) % 17)
                          << "', got '" << tuple[j] << "'.");
        }
      }
    }

  DataArrayAPIFinish();
}

// void* WriteVoidPointer(vtkIdType id, vtkIdType number)
// Ensure that there are at least (id + number) values allocated in the array.
// Update MaxId to ensure that any new values are marked as in-use.
// Return a void pointer to the value at index id.
// TODO This couldn't really work with the new vtkGenericDataArray stuff.
// Deprecate?
template <typename ScalarT, typename ArrayT>
int Test_voidptr_WriteVoidPointer_id_number()
{
  DataArrayAPIInit("void* WriteVoidPointer(vtkIdType id, vtkIdType number)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  vtkIdType values = comps * tuples;
  source->SetNumberOfComponents(comps);

  // Fill the array by writing to the returned void pointer. Trying various
  // write lengths that aren't just multiples of the tuple size.
  vtkIdType pos = 0;
  int writeLength = 1;
  while (true)
    {
    if (pos + writeLength > values)
      {
      writeLength = values - pos;
      }
    if (writeLength <= 0)
      {
      break;
      }

    void *voidPtr = source->WriteVoidPointer(pos, writeLength);

    // Verify that conditions are met:
    if (source->GetMaxId() != pos + writeLength - 1)
      {
      DataArrayAPIError("MaxId was not incremented to account for write length."
                        " MaxId is: " << source->GetMaxId() << ", expected: "
                        << (pos + writeLength - 1) << ".");
      }
    if (source->GetSize() < pos + writeLength)
      {
      DataArrayAPIError("Size was not increased to account for write length. "
                        "Size is: " << source->GetSize() << ", expected: "
                        << (pos + writeLength) << ".");
      }

    // Cast the pointer and write to it:
    ScalarT *ptr = static_cast<ScalarT*>(voidPtr);
    for (int i = 0; i < writeLength; ++i)
      {
      ptr[i] = static_cast<ScalarT>(((pos + 1) * pos) % 17);
      ++pos;
      }
    writeLength += 1;
    }

  // Test the returned tuples:
  vtkIdType v = 0;
  std::vector<double> tuple(comps);
  for (vtkIdType i = 0; i < tuples; ++i)
    {
    source->GetTuple(i, &tuple[0]);
    for (int j = 0; j < comps; ++j)
      {
      if (tuple[j] != static_cast<double>(((v + 1) * v) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << i << ", component " << j
                          << ": Expected '" << (((v + 1) * v) % 17)
                          << "', got '" << tuple[j] << "'.");
        }
      ++v;
      }
    }

  DataArrayAPIFinish();
}

// unsigned long GetActualMemorySize()
// Returns size in kilobytes (1024 bytes) that is >= the size needed to
// represent this array.
template <typename ScalarT, typename ArrayT>
int Test_ulong_GetActualMemorySize()
{
  DataArrayAPIInit("unsigned long GetActualMemorySize()");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 500;
  vtkIdType values = comps * tuples;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);

  unsigned long memSizeBytes = source->GetActualMemorySize() * 1024;
  if (memSizeBytes < values * sizeof(ScalarT))
    {
    DataArrayAPIError("Reported size is too small. Expected at least "
                      << (values * sizeof(ScalarT)) << " bytes, got: "
                      << memSizeBytes << " bytes.");
    }

  DataArrayAPIFinish();
}

// void CreateDefaultLookupTable()
// GetLookupTable should be non-NULL after calling.
template <typename ScalarT, typename ArrayT>
int Test_void_CreateDefaultLookupTable()
{
  DataArrayAPIInit("void CreateDefaultLookupTable()");

  DataArrayAPICreateTestArray(source);

  source->CreateDefaultLookupTable();
  if (source->GetLookupTable() == NULL)
    {
    DataArrayAPIError("Lookup table was not created.");
    }

  DataArrayAPIFinish();
}

// int IsNumeric()
// Should be true for all data array subclasses:
template <typename ScalarT, typename ArrayT>
int Test_int_IsNumeric()
{
  DataArrayAPIInit("int IsNumeric()");

  DataArrayAPICreateTestArray(source);

  if (!source->IsNumeric())
    {
    DataArrayAPIError("IsNumeric() is false.");
    }

  DataArrayAPIFinish();
}

// int GetElementComponentSize()
template <typename ScalarT, typename ArrayT>
int Test_int_GetElementComponentSize()
{
  DataArrayAPIInit("int GetElementComponentSize()");

  DataArrayAPICreateTestArray(source);

  if (source->GetElementComponentSize() != sizeof(ScalarT))
    {
    DataArrayAPIError("Expected '" << sizeof(ScalarT) << "', got: '"
                      << source->GetElementComponentSize() << "'.");
    }

  DataArrayAPIFinish();
}

// void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
//                       vtkAbstractArray *source, double *weights)
// Sets the ith tuple in this array, using the source data, indices, and weights
// provided.
template <typename ScalarT, typename ArrayT>
int Test_void_InterpolateTuple_i_indices_source_weights()
{
  DataArrayAPIInit("void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices, "
                   "vtkAbstractArray *source, double *weights)");

  DataArrayAPICreateReferenceArray(source);
  DataArrayAPICreateTestArray(output);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source->SetValue(i, static_cast<ScalarT>(i % 17));
    }

  // Tuples to interpolate:
  vtkNew<vtkIdList> ids;
  ids->InsertNextId(0);
  ids->InsertNextId(1);
  ids->InsertNextId(5);
  ids->InsertNextId(7);
  ids->InsertNextId(8);
  double weights[] = {0.5, 1.0, 0.25, 1.0, 0.8};

  output->SetNumberOfComponents(comps);
  output->SetNumberOfTuples(1);

  output->InterpolateTuple(0, ids.GetPointer(), source, weights);

  // Validate result:
  for (int c = 0; c < comps; ++c)
    {
    // Compute component:
    double ref = 0.;
    for (vtkIdType t = 0; t < ids->GetNumberOfIds(); ++t)
      {
      ref += weights[t] * source->GetComponent(ids->GetId(t), c);
      }

    // Clamp to ScalarT range:
    ref = std::max(ref, static_cast<double>(vtkTypeTraits<ScalarT>::Min()));
    ref = std::min(ref, static_cast<double>(vtkTypeTraits<ScalarT>::Max()));

    // Round for non-floating point types:
    ScalarT refT;
    if (vtkTypeTraits<ScalarT>::VTK_TYPE_ID == VTK_FLOAT ||
        vtkTypeTraits<ScalarT>::VTK_TYPE_ID == VTK_DOUBLE)
      {
      refT = static_cast<ScalarT>(ref);
      }
    else
      {
      refT = static_cast<ScalarT>((ref >= 0.) ? (ref + 0.5) : (ref - 0.5));
      }

    ScalarT testT = output->GetValue(c);

    if (refT != testT)
      {
      DataArrayAPIError("Interpolated value incorrect: Got '"
                        << static_cast<double>(testT) << "', expected '"
                        << static_cast<double>(refT) << "'.");
      }
    } // foreach component

  DataArrayAPIFinish();
}

// void InterpolateTuple(vtkIdType i,
//                       vtkIdType id1, vtkAbstractArray *source1,
//                       vtkIdType id2, vtkAbstractArray *source2, double t)
// Interpolate tuple id1 from source1 and id2 form source2 and store the result
// in this tuple at tuple index i. t belongs to [0,1] and is the interpolation
// weight, with t=0 meaning 100% from source1.
// TODO the implementation of this method could use some attention:
// - BIT implementation looks wrong (k is not used?) What does bit interpolation
//   even mean?
// - Docs are unclear -- id1 and id2 are documented as "value indices", which
//   is (sort of) how the bit implementation treats them, but they really seem
//   like they should be tuple indices.
template <typename ScalarT, typename ArrayT>
int Test_void_InterpolateTuple_i_id1_source1_id2_source2_t()
{
  DataArrayAPIInit("void InterpolateTuple(vtkIdType i, "
                   "vtkIdType id1, vtkAbstractArray *source1, "
                   "vtkIdType id2, vtkAbstractArray *source2, double t)");

  DataArrayAPICreateReferenceArray(source1);
  DataArrayAPICreateReferenceArray(source2);
  DataArrayAPICreateTestArray(output);

  // Initialize source arrays:
  vtkIdType comps = 9;
  vtkIdType tuples = 10;
  source1->SetNumberOfComponents(comps);
  source1->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source1->SetValue(i, static_cast<ScalarT>(i % 17));
    }

  source2->SetNumberOfComponents(comps);
  source2->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source2->SetValue(i, static_cast<ScalarT>(((i + 3) * 2) % 17));
    }

  output->SetNumberOfComponents(comps);
  output->SetNumberOfTuples(1);

  vtkIdType id1 = 4;
  vtkIdType id2 = 8;
  double t = 0.25;
  output->InterpolateTuple(0, id1, source1, id2, source2, t);

  // Validate result:
  for (int c = 0; c < comps; ++c)
    {
    // Compute component:
    ScalarT ref = static_cast<ScalarT>(
          source1->GetValue(id1 * comps + c) * (1. - t) +
          source2->GetValue(id2 * comps + c) * t);

    ScalarT test = output->GetValue(c);

    if (ref != test)
      {
      DataArrayAPIError("Interpolated value incorrect: Got '"
                        << static_cast<double>(test) << "', expected '"
                        << static_cast<double>(ref) << "'.");
      }
    } // foreach component

  DataArrayAPIFinish();
}

//  double 	GetTuple1 (vtkIdType i)
//  double * 	GetTuple2 (vtkIdType i)
//  double * 	GetTuple3 (vtkIdType i)
//  double * 	GetTuple4 (vtkIdType i)
//  double * 	GetTuple6 (vtkIdType i)
//  double * 	GetTuple9 (vtkIdType i)
// Returns the ith tuple.
template <typename ScalarT, typename ArrayT, int N>
int Test_doubleptr_GetTupleN_i()
{
  std::ostringstream sigBuilder;
  sigBuilder << "double" << ((N == 1) ? "" : "*") << " GetTuple" << N
             << "(vtkIdType i)";
  DataArrayAPIInit(sigBuilder.str());

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = N;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType i = 0; i < comps * tuples; ++i)
    {
    source->SetValue(i, static_cast<ScalarT>(i % 17));
    }

  // Validate API:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    std::vector<double> tuple(N);
    switch(N)
      {
      case 1:
        tuple[0] = source->GetTuple1(t);
        break;
#define vtkDataArrayAPIGetTupleNCase(_N) \
      case _N: \
        { \
        double *tmpPtr = source->GetTuple##_N(t); \
        std::copy(tmpPtr, tmpPtr + _N, tuple.begin()); \
        } \
        break
      vtkDataArrayAPIGetTupleNCase(2);
      vtkDataArrayAPIGetTupleNCase(3);
      vtkDataArrayAPIGetTupleNCase(4);
      vtkDataArrayAPIGetTupleNCase(6);
      vtkDataArrayAPIGetTupleNCase(9);
#undef vtkDataArrayAPIGetTupleNCase
      default:
        DataArrayAPIError("Unrecognized tuple size: GetTuple" << N << "().");
        DataArrayAPIFinish();
      }

    for (int c = 0; c < comps; ++c)
      {
      double test = tuple[c];
      double ref = static_cast<double>((t * comps + c) % 17);
      if (test != ref)
        {
        DataArrayAPIError("Incorrect value returned for tuple " << t
                          << "component " << c << ": Got " << test
                          << ", expected " << ref << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void SetTuple(vtkIdType i, const float *tuple)
// void SetTuple(vtkIdType i, const double *tuple)
template <typename ScalarT, typename ArrayT, typename TupleArgT>
int Test_void_SetTuple_i_tuple()
{
  std::ostringstream sigBuilder;
  sigBuilder << "void SetTuple(vtkIdType i, "
             << vtkTypeTraits<TupleArgT>::Name() << " *tuple)";
  DataArrayAPIInit(sigBuilder.str());

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 5;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    std::vector<TupleArgT> tuple;
    for (int c = 0; c < comps; ++c)
      {
      tuple.push_back(static_cast<TupleArgT>(((t * comps) + c) % 17));
      }
    source->SetTuple(t, &tuple[0]);
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      if (source->GetComponent(t, c) !=
          static_cast<double>(((t * comps) + c) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected "
                          << static_cast<double>(((t * comps) + c) % 17)
                          << ", got " << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void SetTuple1(vtkIdType i, double value)
// void SetTuple2(vtkIdType i, double val0, double val1)
// void SetTuple3(vtkIdType i, double val0, double val1, ...)
// void SetTuple4(vtkIdType i, double val0, double val1, ...)
// void SetTuple6(vtkIdType i, double val0, double val1, ...)
// void SetTuple9(vtkIdType i, double val0, double val1, ...)
template <typename ScalarT, typename ArrayT, int N>
int Test_void_SetTupleN_i()
{
  std::ostringstream sigBuilder;
  sigBuilder << "void SetTuple" << N << "(vtkIdType i, double val0, ...)";
  DataArrayAPIInit(sigBuilder.str());

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = N;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    std::vector<double> tuple;
    for (int c = 0; c < comps; ++c)
      {
      tuple.push_back(static_cast<double>(((t * comps) + c) % 17));
      }
    switch (N)
      {
      case 1:
        source->SetTuple1(t, tuple[0]);
        break;
      case 2:
        source->SetTuple2(t, tuple[0], tuple[1]);
        break;
      case 3:
        source->SetTuple3(t, tuple[0], tuple[1], tuple[2]);
        break;
      case 4:
        source->SetTuple4(t, tuple[0], tuple[1], tuple[2], tuple[3]);
        break;
      case 6:
        source->SetTuple6(t, tuple[0], tuple[1], tuple[2], tuple[3], tuple[4],
                          tuple[5]);
        break;
      case 9:
        source->SetTuple9(t, tuple[0], tuple[1], tuple[2], tuple[3], tuple[4],
                          tuple[5], tuple[6], tuple[7], tuple[8]);
        break;
      default:
        DataArrayAPIError("Invalid N: " << N << ".");
      }
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      if (source->GetComponent(t, c) !=
          static_cast<double>(((t * comps) + c) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected "
                          << static_cast<double>(((t * comps) + c) % 17)
                          << ", got " << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void InsertTuple(vtkIdType i, const float *tuple)
// void InsertTuple(vtkIdType i, const double *tuple)
// Allocates memory as needed.
template <typename ScalarT, typename ArrayT, typename TupleArgT>
int Test_void_InsertTuple_i_tuple()
{
  std::ostringstream sigBuilder;
  sigBuilder << "void InsertTuple(vtkIdType i, "
             << vtkTypeTraits<TupleArgT>::Name() << " *tuple)";
  DataArrayAPIInit(sigBuilder.str());

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 5;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    std::vector<TupleArgT> tuple;
    for (int c = 0; c < comps; ++c)
      {
      tuple.push_back(static_cast<TupleArgT>(((t * comps) + c) % 17));
      }
    source->InsertTuple(t, &tuple[0]);
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      if (source->GetComponent(t, c) !=
          static_cast<double>(((t * comps) + c) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected "
                          << static_cast<double>(((t * comps) + c) % 17)
                          << ", got " << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void InsertTuple1(vtkIdType i, double value)
// void InsertTuple2(vtkIdType i, double val0, double val1)
// void InsertTuple3(vtkIdType i, double val0, double val1, ...)
// void InsertTuple4(vtkIdType i, double val0, double val1, ...)
// void InsertTuple6(vtkIdType i, double val0, double val1, ...)
// void InsertTuple9(vtkIdType i, double val0, double val1, ...)
// Allocates memory as needed.
template <typename ScalarT, typename ArrayT, int N>
int Test_void_InsertTupleN_i()
{
  std::ostringstream sigBuilder;
  sigBuilder << "void InsertTuple" << N << "(vtkIdType i, double val0, ...)";
  DataArrayAPIInit(sigBuilder.str());

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = N;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    std::vector<double> tuple;
    for (int c = 0; c < comps; ++c)
      {
      tuple.push_back(static_cast<double>(((t * comps) + c) % 17));
      }
    switch (N)
      {
      case 1:
        source->InsertTuple1(t, tuple[0]);
        break;
      case 2:
        source->InsertTuple2(t, tuple[0], tuple[1]);
        break;
      case 3:
        source->InsertTuple3(t, tuple[0], tuple[1], tuple[2]);
        break;
      case 4:
        source->InsertTuple4(t, tuple[0], tuple[1], tuple[2], tuple[3]);
        break;
      case 6:
        source->InsertTuple6(t, tuple[0], tuple[1], tuple[2], tuple[3],
                             tuple[4], tuple[5]);
        break;
      case 9:
        source->InsertTuple9(t, tuple[0], tuple[1], tuple[2], tuple[3],
                             tuple[4], tuple[5], tuple[6], tuple[7], tuple[8]);
        break;
      default:
        DataArrayAPIError("Invalid N: " << N << ".");
      }
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      if (source->GetComponent(t, c) !=
          static_cast<double>(((t * comps) + c) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected "
                          << static_cast<double>(((t * comps) + c) % 17)
                          << ", got " << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// vtkIdType InsertNextTuple(const float *tuple)
// vtkIdType InsertNextTuple(const double *tuple)
// Allocates memory as needed.
template <typename ScalarT, typename ArrayT, typename TupleArgT>
int Test_void_InsertNextTuple_tuple()
{
  std::ostringstream sigBuilder;
  sigBuilder << "void InsertNextTuple(" << vtkTypeTraits<TupleArgT>::Name()
             << " *tuple)";
  DataArrayAPIInit(sigBuilder.str());

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 5;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    std::vector<TupleArgT> tuple;
    for (int c = 0; c < comps; ++c)
      {
      tuple.push_back(static_cast<TupleArgT>(((t * comps) + c) % 17));
      }
    source->InsertNextTuple(&tuple[0]);
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      if (source->GetComponent(t, c) !=
          static_cast<double>(((t * comps) + c) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected "
                          << static_cast<double>(((t * comps) + c) % 17)
                          << ", got " << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void InsertNextTuple1(double value)
// void InsertNextTuple2(double val0, double val1)
// void InsertNextTuple3(double val0, double val1, ...)
// void InsertNextTuple4(double val0, double val1, ...)
// void InsertNextTuple6(double val0, double val1, ...)
// void InsertNextTuple9(double val0, double val1, ...)
// Allocates memory as needed
template <typename ScalarT, typename ArrayT, int N>
int Test_void_InsertNextTupleN()
{
  std::ostringstream sigBuilder;
  sigBuilder << "void InsertNextTuple" << N << "(double val0, ...)";
  DataArrayAPIInit(sigBuilder.str());

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = N;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    std::vector<double> tuple;
    for (int c = 0; c < comps; ++c)
      {
      tuple.push_back(static_cast<double>(((t * comps) + c) % 17));
      }
    switch (comps)
      {
      case 1:
        source->InsertNextTuple1(tuple[0]);
        break;
      case 2:
        source->InsertNextTuple2(tuple[0], tuple[1]);
        break;
      case 3:
        source->InsertNextTuple3(tuple[0], tuple[1], tuple[2]);
        break;
      case 4:
        source->InsertNextTuple4(tuple[0], tuple[1], tuple[2], tuple[3]);
        break;
      case 6:
        source->InsertNextTuple6(tuple[0], tuple[1], tuple[2], tuple[3],
                                 tuple[4], tuple[5]);
        break;
      case 9:
        source->InsertNextTuple9(tuple[0], tuple[1], tuple[2], tuple[3],
                                 tuple[4], tuple[5], tuple[6], tuple[7],
                                 tuple[8]);
        break;
      default:
        DataArrayAPIError("Invalid N: " << N << ".");
      }
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      if (source->GetComponent(t, c) !=
          static_cast<double>(((t * comps) + c) % 17))
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected "
                          << static_cast<double>(((t * comps) + c) % 17)
                          << ", got " << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void RemoveTuple(vtkIdType id)
template <typename ScalarT, typename ArrayT>
int Test_void_RemoveTuple_id()
{
  DataArrayAPIInit("void RemoveTuple(vtkIdType id)");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 6;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      source->SetComponent(t, c, static_cast<double>(((t * comps) + c) % 17));
      }
    }

  vtkIdType id = 3; // Tuple index to remove
  source->RemoveTuple(id);

  tuples -= 1;
  if (source->GetNumberOfTuples() != tuples)
    {
    DataArrayAPIError("Number of tuples did not change after RemoveTuple.");
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      double ref = ((t < id ? 0 : comps) + (t * comps) + c) % 17;
      if (source->GetComponent(t, c) != ref)
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected " << ref << ", got "
                          << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void RemoveFirstTuple()
template <typename ScalarT, typename ArrayT>
int Test_void_RemoveFirstTuple()
{
  DataArrayAPIInit("void RemoveFirstTuple()");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 6;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      source->SetComponent(t, c, static_cast<double>(((t * comps) + c) % 17));
      }
    }

  source->RemoveFirstTuple();

  tuples -= 1;
  if (source->GetNumberOfTuples() != tuples)
    {
    DataArrayAPIError("Number of tuples did not change after RemoveTuple.");
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      double ref = (comps + (t * comps) + c) % 17;
      if (source->GetComponent(t, c) != ref)
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected " << ref << ", got "
                          << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void RemoveLastTuple()
template <typename ScalarT, typename ArrayT>
int Test_void_RemoveLastTuple()
{
  DataArrayAPIInit("void RemoveLastTuple()");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 6;
  vtkIdType tuples = 10;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      source->SetComponent(t, c, static_cast<double>(((t * comps) + c) % 17));
      }
    }

  source->RemoveLastTuple();

  tuples -= 1;
  if (source->GetNumberOfTuples() != tuples)
    {
    DataArrayAPIError("Number of tuples did not change after RemoveTuple.");
    }

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      double ref = ((t * comps) + c) % 17;
      if (source->GetComponent(t, c) != ref)
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected " << ref << ", got "
                          << source->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void GetData(vtkIdType tupleMin, vtkIdType tupleMax, int compMin,
//              int compMax, vtkDoubleArray *data)
template <typename ScalarT, typename ArrayT>
int Test_void_GetData_tupleMin_tupleMax_compMin_compMax_data()
{
  DataArrayAPIInit("void RemoveLastTuple()");

  DataArrayAPICreateTestArray(source);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 40;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      source->SetComponent(t, c, static_cast<double>(((t * comps) + c) % 17));
      }
    }

  // Extract some data:
  int compMin = 2;
  int compMax = 7;
  int dataComps = compMax - compMin + 1;
  vtkIdType tupleMin = 7;
  vtkIdType tupleMax = 32;
  vtkIdType dataTuples = tupleMax - tupleMin + 1;
  vtkNew<vtkDoubleArray> data;
  data->SetNumberOfComponents(dataComps);
  data->SetNumberOfTuples(dataTuples);
  source->GetData(tupleMin, tupleMax, compMin, compMax, data.GetPointer());

  // Verify:
  for (vtkIdType t = 0; t < dataTuples; ++t)
    {
    vtkIdType sourceTuple = t + tupleMin;
    for (int c = 0; c < dataComps; ++c)
      {
      int sourceComp = c + compMin;
      double ref = source->GetComponent(sourceTuple, sourceComp);
      double test = data->GetComponent(t, c);
      if (ref != test)
        {
        DataArrayAPIError("Mismatch at data tuple " << t << " component " << c
                          << ": Expected " << ref << ", got " << test << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// void DeepCopy(vtkAbstractArray *aa)
// void DeepCopy(vtkDataArray *da)
// Test copying into and from the target array type.
// Allocates memory as needed.
// ArgT switches between the two overloads.
// OtherT is the type of array that will be copied to/from.
template <typename ScalarT, typename ArrayT, typename ArgT, typename OtherT>
int Test_void_DeepCopy_array()
{
  std::string argTName = (typeid(ArgT) == typeid(vtkAbstractArray)
                          ? "vtkAbstractArray" : "vtkDataArray");
  std::ostringstream sigBuilder;
  sigBuilder << "void DeepCopy(" << argTName << " *array)";
  DataArrayAPIInit(sigBuilder.str());

  std::string testType = vtkTypeTraits<ScalarT>::Name();
  std::string otherType = vtkTypeTraits<OtherT>::Name();

  DataArrayAPICreateTestArray(source);
  DataArrayAPICreateReferenceArrayWithType(middle, OtherT);
  DataArrayAPICreateTestArray(target);

  // Initialize source array:
  vtkIdType comps = 9;
  vtkIdType tuples = 40;
  source->SetNumberOfComponents(comps);
  source->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      source->SetComponent(t, c, static_cast<double>(((t * comps) + c) % 17));
      }
    }

  // Copy to intermediate:
  middle->DeepCopy(static_cast<ArgT*>(source.GetPointer()));

  // Verify intermediate:
  if (middle->GetNumberOfComponents() != comps ||
      middle->GetNumberOfTuples() != tuples)
    {
    DataArrayAPIError("Incorrect size of array after copying from test array "
                      "(scalar type: '" << testType << "') to reference array "
                      "(scalar type: '" << otherType << "'): Expected number "
                      "of (tuples, components): " << "(" << tuples << ", "
                      << comps << "), got (" << middle->GetNumberOfTuples()
                      << ", " << middle->GetNumberOfComponents() << ").");
    }

  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      double ref = ((t * comps) + c) % 17;
      double test = middle->GetComponent(t, c);
      if (ref != test)
        {
        DataArrayAPIError("Data mismatch after copying from test array (scalar "
                          "type: '" << testType << "') to reference array "
                          "(scalar type: '" << otherType << "'): " "Data "
                          "mismatch at tuple " << t << " component " << c
                          << ": Expected " << ref << ", got " << test << ".");
        }
      }
    }

  // Copy to final:
  target->DeepCopy(static_cast<ArgT*>(middle));

  // Verify final:
  if (target->GetNumberOfComponents() != comps ||
      target->GetNumberOfTuples() != tuples)
    {
    DataArrayAPIError("Incorrect size of array after copying from reference "
                      "array (scalar type: '" << otherType << "') to test "
                      "array (scalar type: '" << testType << "'): "
                      "Expected number of (tuples, components): "
                      << "(" << tuples << ", " << comps << "), got ("
                      << target->GetNumberOfTuples() << ", "
                      << target->GetNumberOfComponents() << ").");
    }

  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      double ref = ((t * comps) + c) % 17;
      double test = target->GetComponent(t, c);
      if (ref != test)
        {
        DataArrayAPIError("Data mismatch after copying from reference array "
                          "(scalar type: '" << otherType << "') to test array "
                          "(scalar type: '" << testType << "'): " "Data "
                          "mismatch at tuple " << t << " component " << c
                          << ": Expected " << ref << ", got " << test << ".");
        }
      }
    }

  DataArrayAPIFinish();
}


// void CopyComponent(int j, vtkDataArray *from, int fromComponent)
// For all tuples, copy the fromComponent component from the 'from' array
// into the jth component in this.
template <typename ScalarT, typename ArrayT>
int Test_void_CopyComponent_j_from_fromComponent()
{
  DataArrayAPIInit(
        "void CopyComponent(int j, vtkDataArray *from, int fromComponent)");

  DataArrayAPICreateTestArray(target);
  DataArrayAPICreateReferenceArray(from);

  // Initialize arrays:
  vtkIdType comps = 11;
  vtkIdType tuples = 10;
  from->SetNumberOfComponents(comps);
  from->SetNumberOfTuples(tuples);
  target->SetNumberOfComponents(comps);
  target->SetNumberOfTuples(tuples);
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      target->SetComponent(t, c, static_cast<double>(((t * comps) + c) % 17));
      from->SetComponent(t, c, static_cast<double>(
                           (((t + 1) * comps) + (c + 1)) % 17));
      }
    }

  int j = 2;
  int fromComponent = 8;

  target->CopyComponent(j, from, fromComponent);

  // Verify:
  for (vtkIdType t = 0; t < tuples; ++t)
    {
    for (int c = 0; c < comps; ++c)
      {
      double ref;
      if (c == j) // Use the target's value formula
        {
        ref = static_cast<double>(
              (((t + 1) * comps) + (fromComponent + 1)) % 17);
        }
      else // Use the source's value formula
        {
        ref = ((t * comps) + c) % 17;
        }
      if (target->GetComponent(t, c) != ref)
        {
        DataArrayAPIError("Data mismatch at tuple " << t << " component " << c
                          << ": Expected " << ref << ", got "
                          << target->GetComponent(t, c) << ".");
        }
      }
    }

  DataArrayAPIFinish();
}

// double* GetRange()
// void GetRange(double range[2])
// double* GetRange(int comp)
// void GetRange(double range[2], int comp)
template <typename ScalarT, typename ArrayT>
int Test_GetRange_all_overloads()
{
  DataArrayAPIInit("GetRange");

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
  DataArrayAPIUpdateSignature("double* GetRange()");
  double *rangePtr = array->GetRange();
  double expectedRange[2] = { 1., static_cast<double>(tuples) };
  if (rangePtr[0] != expectedRange[0] ||
      rangePtr[1] != expectedRange[1])
    {
    DataArrayAPINonFatalError("First component range expected to be: ["
                              << expectedRange[0] << ", " << expectedRange[1]
                              << "], got [" << rangePtr[0] << ", "
                              << rangePtr[1] << "].");
    }

  DataArrayAPIUpdateSignature("void GetRange(double range[2])");
  double rangeArray[2];
  array->GetRange(rangeArray);
  if (rangeArray[0] != expectedRange[0] ||
      rangeArray[1] != expectedRange[1])
    {
    DataArrayAPINonFatalError("First component range expected to be: ["
                              << expectedRange[0] << ", " << expectedRange[1]
                              << "], got [" << rangeArray[0] << ", "
                              << rangeArray[1] << "].");
    }

  DataArrayAPIUpdateSignature("double* GetRange(int comp)");
  for (int c = 0; c < comps; ++c)
    {
    expectedRange[0] = c + 1;
    expectedRange[1] = tuples * (c + 1);
    rangePtr = array->GetRange(c);
    if (rangePtr[0] != expectedRange[0] ||
        rangePtr[1] != expectedRange[1])
      {
      DataArrayAPINonFatalError("Component " << c << " range expected to be: ["
                                << expectedRange[0] << ", " << expectedRange[1]
                                << "], got [" << rangePtr[0] << ", "
                                << rangePtr[1] << "].");
      }
    }

  DataArrayAPIUpdateSignature("void GetRange(double range[2], int comp)");
  for (int c = 0; c < comps; ++c)
    {
    expectedRange[0] = c + 1;
    expectedRange[1] = tuples * (c + 1);
    array->GetRange(rangeArray, c);
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
int ExerciseDataArray()
{
  int errors = 0;

  errors += Test_void_SetTuple_i_j_source<ScalarT, ArrayT>();
  errors += Test_void_InsertTuple_i_j_source<ScalarT, ArrayT>();
  errors += Test_vtkIdType_InsertNextTuple_j_source<ScalarT, ArrayT>();
  errors += Test_void_GetTuples_ptIds_output<ScalarT, ArrayT>();
  errors += Test_void_GetTuples_p1_p2_output<ScalarT, ArrayT>();
  errors += Test_doubleptr_GetTuple_i<ScalarT, ArrayT>();
  errors += Test_void_GetTuple_i_tuple<ScalarT, ArrayT>();
  errors += Test_double_GetComponent_i_j<ScalarT, ArrayT>();
  errors += Test_void_SetComponent_i_j_c<ScalarT, ArrayT>();
  errors += Test_void_InsertComponent_i_j_c<ScalarT, ArrayT>();
  errors += Test_voidptr_WriteVoidPointer_id_number<ScalarT, ArrayT>();
  errors += Test_ulong_GetActualMemorySize<ScalarT, ArrayT>();
  errors += Test_void_CreateDefaultLookupTable<ScalarT, ArrayT>();
  errors += Test_int_IsNumeric<ScalarT, ArrayT>();
  errors += Test_int_GetElementComponentSize<ScalarT, ArrayT>();
  errors += Test_void_InterpolateTuple_i_indices_source_weights<ScalarT, ArrayT>();
  errors += Test_void_InterpolateTuple_i_id1_source1_id2_source2_t<ScalarT, ArrayT>();
  errors += Test_doubleptr_GetTupleN_i<ScalarT, ArrayT, 1>();
  errors += Test_doubleptr_GetTupleN_i<ScalarT, ArrayT, 2>();
  errors += Test_doubleptr_GetTupleN_i<ScalarT, ArrayT, 3>();
  errors += Test_doubleptr_GetTupleN_i<ScalarT, ArrayT, 4>();
  errors += Test_doubleptr_GetTupleN_i<ScalarT, ArrayT, 6>();
  errors += Test_doubleptr_GetTupleN_i<ScalarT, ArrayT, 9>();
  errors += Test_void_SetTuple_i_tuple<ScalarT, ArrayT, float>();
  errors += Test_void_SetTuple_i_tuple<ScalarT, ArrayT, double>();
  errors += Test_void_SetTupleN_i<ScalarT, ArrayT, 1>();
  errors += Test_void_SetTupleN_i<ScalarT, ArrayT, 2>();
  errors += Test_void_SetTupleN_i<ScalarT, ArrayT, 3>();
  errors += Test_void_SetTupleN_i<ScalarT, ArrayT, 4>();
  errors += Test_void_SetTupleN_i<ScalarT, ArrayT, 6>();
  errors += Test_void_SetTupleN_i<ScalarT, ArrayT, 9>();
  errors += Test_void_InsertTuple_i_tuple<ScalarT, ArrayT, float>();
  errors += Test_void_InsertTuple_i_tuple<ScalarT, ArrayT, double>();
  errors += Test_void_InsertTupleN_i<ScalarT, ArrayT, 1>();
  errors += Test_void_InsertTupleN_i<ScalarT, ArrayT, 2>();
  errors += Test_void_InsertTupleN_i<ScalarT, ArrayT, 3>();
  errors += Test_void_InsertTupleN_i<ScalarT, ArrayT, 4>();
  errors += Test_void_InsertTupleN_i<ScalarT, ArrayT, 6>();
  errors += Test_void_InsertTupleN_i<ScalarT, ArrayT, 9>();
  errors += Test_void_InsertNextTuple_tuple<ScalarT, ArrayT, float>();
  errors += Test_void_InsertNextTuple_tuple<ScalarT, ArrayT, double>();
  errors += Test_void_InsertNextTupleN<ScalarT, ArrayT, 1>();
  errors += Test_void_InsertNextTupleN<ScalarT, ArrayT, 2>();
  errors += Test_void_InsertNextTupleN<ScalarT, ArrayT, 3>();
  errors += Test_void_InsertNextTupleN<ScalarT, ArrayT, 4>();
  errors += Test_void_InsertNextTupleN<ScalarT, ArrayT, 6>();
  errors += Test_void_InsertNextTupleN<ScalarT, ArrayT, 9>();
  errors += Test_void_RemoveTuple_id<ScalarT, ArrayT>();
  errors += Test_void_RemoveFirstTuple<ScalarT, ArrayT>();
  errors += Test_void_RemoveLastTuple<ScalarT, ArrayT>();
  errors += Test_void_GetData_tupleMin_tupleMax_compMin_compMax_data<ScalarT, ArrayT>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, char>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, float>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, double>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, vtkIdType>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, int>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, long>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, long long>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, short>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, signed char>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, unsigned char>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, unsigned int>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, unsigned long>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, unsigned long long>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkAbstractArray, unsigned short>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, char>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, float>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, double>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, vtkIdType>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, int>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, long>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, long long>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, short>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, signed char>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, unsigned char>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, unsigned int>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, unsigned long>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, unsigned long long>();
  errors += Test_void_DeepCopy_array<ScalarT, ArrayT, vtkDataArray, unsigned short>();
  errors += Test_void_CopyComponent_j_from_fromComponent<ScalarT, ArrayT>();
  errors += Test_GetRange_all_overloads<ScalarT, ArrayT>();

  return errors;
} // end ExerciseDataArray

} // end anon namespace
