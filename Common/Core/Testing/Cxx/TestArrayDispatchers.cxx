/*==============================================================================

  Program:   Visualization Toolkit
  Module:    TestArrayDispatchers.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

// We define our own set of arrays for the dispatch list. This allows the test
// to run regardless of the compiled dispatch configuration. Note that this is
// only possible because we do not use dispatches that are compiled into other
// translation units, but only explicit dispatches that we generate here.
#define vtkArrayDispatchArrayList_h // Skip loading the actual header

#include "vtkAOSDataArrayTemplate.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkTypeList.h"

namespace vtkArrayDispatch {
typedef vtkTypeList::Unique<
  vtkTypeList_Create_10(
    vtkAOSDataArrayTemplate<double>,
    vtkAOSDataArrayTemplate<float>,
    vtkAOSDataArrayTemplate<int>,
    vtkAOSDataArrayTemplate<unsigned char>,
    vtkAOSDataArrayTemplate<vtkIdType>,
    vtkSOADataArrayTemplate<double>,
    vtkSOADataArrayTemplate<float>,
    vtkSOADataArrayTemplate<int>,
    vtkSOADataArrayTemplate<unsigned char>,
    vtkSOADataArrayTemplate<vtkIdType>
  )
>::Result Arrays;
} // end namespace vtkArrayDispatch

#include "vtkArrayDispatch.h"
#include "vtkNew.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace {

//==============================================================================
// Our functor for testing.
struct TestWorker
{
  TestWorker()
    : Array1(NULL), Array2(NULL), Array3(NULL)
  {}

  void Reset()
  {
    this->Array1 = NULL;
    this->Array2 = NULL;
    this->Array3 = NULL;
  }

  template <typename Array1T>
  void operator()(Array1T *array1)
  {
    this->Array1 = array1;
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T *array1, Array2T *array2)
  {
    this->Array1 = array1;
    this->Array2 = array2;
  }

  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T *array1, Array2T *array2, Array3T *array3)
  {
    this->Array1 = array1;
    this->Array2 = array2;
    this->Array3 = array3;
  }

  vtkDataArray *Array1;
  vtkDataArray *Array2;
  vtkDataArray *Array3;
};

//==============================================================================
// Container for testing arrays.
struct Arrays
{
  Arrays();
  ~Arrays();

  static vtkAOSDataArrayTemplate<double>        *aosDouble;
  static vtkAOSDataArrayTemplate<float>         *aosFloat;
  static vtkAOSDataArrayTemplate<int>           *aosInt;
  static vtkAOSDataArrayTemplate<unsigned char> *aosUnsignedChar;
  static vtkAOSDataArrayTemplate<vtkIdType>     *aosIdType;

  static vtkSOADataArrayTemplate<double>        *soaDouble;
  static vtkSOADataArrayTemplate<float>         *soaFloat;
  static vtkSOADataArrayTemplate<int>           *soaInt;
  static vtkSOADataArrayTemplate<unsigned char> *soaUnsignedChar;
  static vtkSOADataArrayTemplate<vtkIdType>     *soaIdType;

  static std::vector<vtkDataArray*> aosArrays;
  static std::vector<vtkDataArray*> soaArrays;
  static std::vector<vtkDataArray*> allArrays;
};

vtkAOSDataArrayTemplate<double>        *Arrays::aosDouble;
vtkAOSDataArrayTemplate<float>         *Arrays::aosFloat;
vtkAOSDataArrayTemplate<int>           *Arrays::aosInt;
vtkAOSDataArrayTemplate<unsigned char> *Arrays::aosUnsignedChar;
vtkAOSDataArrayTemplate<vtkIdType>     *Arrays::aosIdType;
vtkSOADataArrayTemplate<double>        *Arrays::soaDouble;
vtkSOADataArrayTemplate<float>         *Arrays::soaFloat;
vtkSOADataArrayTemplate<int>           *Arrays::soaInt;
vtkSOADataArrayTemplate<unsigned char> *Arrays::soaUnsignedChar;
vtkSOADataArrayTemplate<vtkIdType>     *Arrays::soaIdType;

std::vector<vtkDataArray*> Arrays::aosArrays;
std::vector<vtkDataArray*> Arrays::soaArrays;
std::vector<vtkDataArray*> Arrays::allArrays;

//==============================================================================
// Miscellaneous Debris
typedef std::vector<vtkDataArray*>::iterator ArrayIter;

typedef vtkTypeList_Create_5(vtkAOSDataArrayTemplate<double>,
                             vtkAOSDataArrayTemplate<float>,
                             vtkAOSDataArrayTemplate<int>,
                             vtkAOSDataArrayTemplate<unsigned char>,
                             vtkAOSDataArrayTemplate<vtkIdType>) AoSArrayList;
typedef vtkTypeList_Create_5(vtkSOADataArrayTemplate<double>,
                             vtkSOADataArrayTemplate<float>,
                             vtkSOADataArrayTemplate<int>,
                             vtkSOADataArrayTemplate<unsigned char>,
                             vtkSOADataArrayTemplate<vtkIdType>) SoAArrayList;

typedef vtkTypeList::Append<AoSArrayList, SoAArrayList>::Result AllArrayList;

//------------------------------------------------------------------------------
// Return true if the VTK type tag is an integral type.
inline bool isIntegral(int vtkType)
{
  switch (vtkType)
  {
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    case VTK_UNSIGNED_CHAR:
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
    case VTK_INT:
    case VTK_UNSIGNED_INT:
    case VTK_LONG:
    case VTK_UNSIGNED_LONG:
    case VTK_ID_TYPE:
    case VTK_LONG_LONG:
    case VTK_UNSIGNED_LONG_LONG:
#if !defined(VTK_LEGACY_REMOVE)
    case VTK___INT64:
    case VTK_UNSIGNED___INT64:
#endif
      return true;
  }
  return false;
}

//------------------------------------------------------------------------------
// Return true if the VTK type tag is a real (e.g. floating-point) type.
inline bool isReal(int vtkType)
{
  switch (vtkType)
  {
    case VTK_FLOAT:
    case VTK_DOUBLE:
      return true;
  }
  return false;
}

//------------------------------------------------------------------------------
// Check condition during test.
#define testAssert(expr, errorMessage) \
  if (!(expr)) \
  { \
    ++errors; \
    vtkGenericWarningMacro(<<"Assertion failed: " #expr << "\n" \
                           << errorMessage); \
  }

//------------------------------------------------------------------------------
int TestDispatch()
{
  int errors = 0;

  vtkArrayDispatch::Dispatch dispatcher;
  TestWorker worker;

  for (ArrayIter it = Arrays::allArrays.begin(),
       itEnd = Arrays::allArrays.end(); it != itEnd; ++it)
  {
    vtkDataArray *array = *it;
    testAssert(dispatcher.Execute(array, worker), "Dispatch failed.");
    testAssert(worker.Array1 == array, "Array 1 does not match input.");
    worker.Reset();
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatchByArray()
{
  int errors = 0;

  vtkArrayDispatch::DispatchByArray<AoSArrayList> dispatcher;
  TestWorker worker;

  // AoS arrays: All should pass:
  for (ArrayIter it = Arrays::aosArrays.begin(),
       itEnd = Arrays::aosArrays.end(); it != itEnd; ++it)
  {
    vtkDataArray *array = *it;
    testAssert(dispatcher.Execute(array, worker), "Dispatch failed.");
    testAssert(worker.Array1 == array, "Array 1 does not match input.");
    worker.Reset();
  }

  // AoS arrays: All should fail:
  for (ArrayIter it = Arrays::soaArrays.begin(),
       itEnd = Arrays::soaArrays.end(); it != itEnd; ++it)
  {
    vtkDataArray *array = *it;
    testAssert(!dispatcher.Execute(array, worker),
               "Dispatch should have failed.");
    testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
    worker.Reset();
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatchByValueType()
{
  int errors = 0;

  // Create dispatcher that only generates code paths for arrays with reals.
  vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals> dispatcher;
  TestWorker worker;

  for (ArrayIter it = Arrays::allArrays.begin(),
       itEnd = Arrays::allArrays.end(); it != itEnd; ++it)
  {
    vtkDataArray *array = *it;
    bool isValid = isReal(array->GetDataType());

    if (isValid)
    {
      testAssert(dispatcher.Execute(array, worker), "Dispatch failed.");
      testAssert(worker.Array1 == array, "Array 1 does not match input.");
    }
    else
    {
      testAssert(!dispatcher.Execute(array, worker),
                 "Dispatch should have failed.");
      testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
    }

    worker.Reset();
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatch2ByArray()
{
  int errors = 0;

  // Restrictions:
  // Array1: SoA
  // Array2: AoS
  vtkArrayDispatch::Dispatch2ByArray<
      SoAArrayList,
      AoSArrayList
      > dispatcher;
  TestWorker worker;

  for (ArrayIter it1 = Arrays::allArrays.begin(),
       itEnd1 = Arrays::allArrays.end(); it1 != itEnd1; ++it1)
  {
    vtkDataArray *array1 = *it1;
    bool array1Valid =
        array1->GetArrayType() == vtkAbstractArray::SoADataArrayTemplate;

    for (ArrayIter it2 = Arrays::allArrays.begin(),
         itEnd2 = Arrays::allArrays.end(); it2 != itEnd2; ++it2)
    {
      vtkDataArray *array2 = *it2;
      bool array2Valid =
          array2->GetArrayType() == vtkAbstractArray::AoSDataArrayTemplate;

      if (array1Valid && array2Valid)
      {
        testAssert(dispatcher.Execute(array1, array2, worker),
                   "Dispatch failed.");
        testAssert(worker.Array1 == array1, "Array 1 does not match input.");
        testAssert(worker.Array2 == array2, "Array 2 does not match input.");
      }
      else
      {
        testAssert(!dispatcher.Execute(array1, array2, worker),
                   "Dispatch should have failed.");
        testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
        testAssert(worker.Array2 == NULL, "Array 2 should be NULL.");
      }

      worker.Reset();
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatch2ByValueType()
{
  int errors = 0;

  // Restrictions:
  // Array1: Integers
  // Array2: Reals
  vtkArrayDispatch::Dispatch2ByValueType<
      vtkArrayDispatch::Integrals,
      vtkArrayDispatch::Reals
      > dispatcher;
  TestWorker worker;

  for (ArrayIter it1 = Arrays::allArrays.begin(),
       itEnd1 = Arrays::allArrays.end(); it1 != itEnd1; ++it1)
  {
    vtkDataArray *array1 = *it1;
    bool array1Valid = isIntegral(array1->GetDataType());

    for (ArrayIter it2 = Arrays::allArrays.begin(),
         itEnd2 = Arrays::allArrays.end(); it2 != itEnd2; ++it2)
    {
      vtkDataArray *array2 = *it2;
      bool array2Valid = isReal(array2->GetDataType());

      if (array1Valid && array2Valid)
      {
        testAssert(dispatcher.Execute(array1, array2, worker),
                   "Dispatch failed.");
        testAssert(worker.Array1 == array1, "Array 1 does not match input.");
        testAssert(worker.Array2 == array2, "Array 2 does not match input.");
      }
      else
      {
        testAssert(!dispatcher.Execute(array1, array2, worker),
                   "Dispatch should have failed.");
        testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
        testAssert(worker.Array2 == NULL, "Array 2 should be NULL.");
      }

      worker.Reset();
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatch2ByArrayWithSameValueType()
{
  int errors = 0;

  // Restrictions:
  // - Types must match
  vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<
      AoSArrayList,
      SoAArrayList
      > dispatcher;
  TestWorker worker;

  for (ArrayIter it1 = Arrays::allArrays.begin(),
       itEnd1 = Arrays::allArrays.end(); it1 != itEnd1; ++it1)
  {
    vtkDataArray *array1 = *it1;
    bool array1Valid =
        array1->GetArrayType() == vtkAbstractArray::AoSDataArrayTemplate;

    for (ArrayIter it2 = Arrays::allArrays.begin(),
         itEnd2 = Arrays::allArrays.end(); it2 != itEnd2; ++it2)
    {
      vtkDataArray *array2 = *it2;
      bool array2Valid =
          array2->GetArrayType() == vtkAbstractArray::SoADataArrayTemplate &&
          vtkDataTypesCompare(array1->GetDataType(), array2->GetDataType());

      if (array1Valid && array2Valid)
      {
        testAssert(dispatcher.Execute(array1, array2, worker),
                   "Dispatch failed.");
        testAssert(worker.Array1 == array1, "Array 1 does not match input.");
        testAssert(worker.Array2 == array2, "Array 2 does not match input.");
      }
      else
      {
        testAssert(!dispatcher.Execute(array1, array2, worker),
                   "Dispatch should have failed.");
        testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
        testAssert(worker.Array2 == NULL, "Array 2 should be NULL.");
      }

      worker.Reset();
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatch2BySameValueType()
{
  int errors = 0;

  // Restrictions:
  // - Types must match
  // - Only integral types.
  vtkArrayDispatch::Dispatch2BySameValueType<
      vtkArrayDispatch::Integrals
      > dispatcher;
  TestWorker worker;

  for (ArrayIter it1 = Arrays::allArrays.begin(),
       itEnd1 = Arrays::allArrays.end(); it1 != itEnd1; ++it1)
  {
    vtkDataArray *array1 = *it1;
    bool array1Valid = isIntegral(array1->GetDataType());

    for (ArrayIter it2 = Arrays::allArrays.begin(),
         itEnd2 = Arrays::allArrays.end(); it2 != itEnd2; ++it2)
    {
      vtkDataArray *array2 = *it2;
      bool array2Valid = vtkDataTypesCompare(array1->GetDataType(),
                                             array2->GetDataType()) != 0;

      if (array1Valid && array2Valid)
      {
        testAssert(dispatcher.Execute(array1, array2, worker),
                   "Dispatch failed.");
        testAssert(worker.Array1 == array1, "Array 1 does not match input.");
        testAssert(worker.Array2 == array2, "Array 2 does not match input.");
      }
      else
      {
        testAssert(!dispatcher.Execute(array1, array2, worker),
                   "Dispatch should have failed.");
        testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
        testAssert(worker.Array2 == NULL, "Array 2 should be NULL.");
      }

      worker.Reset();
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatch3ByArray()
{
  int errors = 0;

  // Restrictions:
  // Array1: SoA
  // Array2: AoS
  // Array3: AoS/SoA float arrays
  vtkArrayDispatch::Dispatch3ByArray<
      SoAArrayList,
      AoSArrayList,
      vtkTypeList_Create_2(vtkAOSDataArrayTemplate<float>,
                           vtkSOADataArrayTemplate<float>)
      > dispatcher;
  TestWorker worker;

  for (ArrayIter it1 = Arrays::allArrays.begin(),
       itEnd1 = Arrays::allArrays.end(); it1 != itEnd1; ++it1)
  {
    vtkDataArray *array1 = *it1;
    bool array1Valid =
        array1->GetArrayType() == vtkAbstractArray::SoADataArrayTemplate;

    for (ArrayIter it2 = Arrays::allArrays.begin(),
         itEnd2 = Arrays::allArrays.end(); it2 != itEnd2; ++it2)
    {
      vtkDataArray *array2 = *it2;
      bool array2Valid =
          array2->GetArrayType() == vtkAbstractArray::AoSDataArrayTemplate;

      for (ArrayIter it3 = Arrays::allArrays.begin(),
           itEnd3 = Arrays::allArrays.end(); it3 != itEnd3; ++it3)
      {
        vtkDataArray *array3 = *it3;
        bool array3Valid = array3->GetDataType() == VTK_FLOAT;

        if (array1Valid && array2Valid && array3Valid)
        {
          testAssert(dispatcher.Execute(array1, array2, array3, worker),
                     "Dispatch failed.");
          testAssert(worker.Array1 == array1, "Array 1 does not match input.");
          testAssert(worker.Array2 == array2, "Array 2 does not match input.");
          testAssert(worker.Array3 == array3, "Array 3 does not match input.");
        }
        else
        {
          testAssert(!dispatcher.Execute(array1, array2, array3, worker),
                     "Dispatch should have failed.");
          testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
          testAssert(worker.Array2 == NULL, "Array 2 should be NULL.");
          testAssert(worker.Array3 == NULL, "Array 3 should be NULL.");
        }

        worker.Reset();
      }
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatch3ByValueType()
{
  int errors = 0;

  // Restrictions:
  // Array1: Must be real type.
  // Array2: Must be integer type.
  // Array3: Must be unsigned char type.
  vtkArrayDispatch::Dispatch3ByValueType<
      vtkArrayDispatch::Reals,
      vtkArrayDispatch::Integrals,
      vtkTypeList_Create_1(unsigned char)
      > dispatcher;
  TestWorker worker;

  for (ArrayIter it1 = Arrays::allArrays.begin(),
       itEnd1 = Arrays::allArrays.end(); it1 != itEnd1; ++it1)
  {
    vtkDataArray *array1 = *it1;
    bool array1Valid = isReal(array1->GetDataType());

    for (ArrayIter it2 = Arrays::allArrays.begin(),
         itEnd2 = Arrays::allArrays.end(); it2 != itEnd2; ++it2)
    {
      vtkDataArray *array2 = *it2;
      bool array2Valid = isIntegral(array2->GetDataType());

      for (ArrayIter it3 = Arrays::allArrays.begin(),
           itEnd3 = Arrays::allArrays.end(); it3 != itEnd3; ++it3)
      {
        vtkDataArray *array3 = *it3;
        bool array3Valid = vtkDataTypesCompare(array3->GetDataType(),
                                               VTK_UNSIGNED_CHAR) != 0;

        if (array1Valid && array2Valid && array3Valid)
        {
          testAssert(dispatcher.Execute(array1, array2, array3, worker),
                     "Dispatch failed.");
          testAssert(worker.Array1 == array1, "Array 1 does not match input.");
          testAssert(worker.Array2 == array2, "Array 2 does not match input.");
          testAssert(worker.Array3 == array3, "Array 3 does not match input.");
        }
        else
        {
          testAssert(!dispatcher.Execute(array1, array2, array3, worker),
                     "Dispatch should have failed.");
          testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
          testAssert(worker.Array2 == NULL, "Array 2 should be NULL.");
          testAssert(worker.Array3 == NULL, "Array 3 should be NULL.");
        }

        worker.Reset();
      }
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatch3ByArrayWithSameValueType()
{
  int errors = 0;

  // Restrictions:
  // - Array1: SoA
  // - Array2: AoS
  // - Array3: Any array type
  // - All arrays have same ValueType
  vtkArrayDispatch::Dispatch3ByArrayWithSameValueType<
      SoAArrayList,
      AoSArrayList,
      AllArrayList
      > dispatcher;
  TestWorker worker;

  for (ArrayIter it1 = Arrays::allArrays.begin(),
       itEnd1 = Arrays::allArrays.end(); it1 != itEnd1; ++it1)
  {
    vtkDataArray *array1 = *it1;
    bool array1Valid =
        array1->GetArrayType() == vtkAbstractArray::SoADataArrayTemplate;

    for (ArrayIter it2 = Arrays::allArrays.begin(),
         itEnd2 = Arrays::allArrays.end(); it2 != itEnd2; ++it2)
    {
      vtkDataArray *array2 = *it2;
      bool array2Valid =
          array2->GetArrayType() == vtkAbstractArray::AoSDataArrayTemplate &&
          vtkDataTypesCompare(array1->GetDataType(), array2->GetDataType());

      for (ArrayIter it3 = Arrays::allArrays.begin(),
           itEnd3 = Arrays::allArrays.end(); it3 != itEnd3; ++it3)
      {
        vtkDataArray *array3 = *it3;
        bool array3Valid = vtkDataTypesCompare(array1->GetDataType(),
                                               array3->GetDataType()) != 0;

        if (array1Valid && array2Valid && array3Valid)
        {
          testAssert(dispatcher.Execute(array1, array2, array3, worker),
                     "Dispatch failed.");
          testAssert(worker.Array1 == array1, "Array 1 does not match input.");
          testAssert(worker.Array2 == array2, "Array 2 does not match input.");
          testAssert(worker.Array3 == array3, "Array 3 does not match input.");
        }
        else
        {
          testAssert(!dispatcher.Execute(array1, array2, array3, worker),
                     "Dispatch should have failed.");
          testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
          testAssert(worker.Array2 == NULL, "Array 2 should be NULL.");
          testAssert(worker.Array3 == NULL, "Array 3 should be NULL.");
        }

        worker.Reset();
      }
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatch3BySameValueType()
{
  int errors = 0;

  // Restrictions:
  // - All arrays must have same ValueType
  // - ValueType must be float, double, or unsigned char.
  vtkArrayDispatch::Dispatch3BySameValueType<
      vtkTypeList::Append<vtkArrayDispatch::Reals, unsigned char>::Result
      > dispatcher;
  TestWorker worker;

  for (ArrayIter it1 = Arrays::allArrays.begin(),
       itEnd1 = Arrays::allArrays.end(); it1 != itEnd1; ++it1)
  {
    vtkDataArray *array1 = *it1;
    bool array1Valid = isReal(array1->GetDataType()) ||
                       vtkDataTypesCompare(array1->GetDataType(),
                                           VTK_UNSIGNED_CHAR);

    for (ArrayIter it2 = Arrays::allArrays.begin(),
         itEnd2 = Arrays::allArrays.end(); it2 != itEnd2; ++it2)
    {
      vtkDataArray *array2 = *it2;
      bool array2Valid = vtkDataTypesCompare(array1->GetDataType(),
                                             array2->GetDataType()) != 0;

      for (ArrayIter it3 = Arrays::allArrays.begin(),
           itEnd3 = Arrays::allArrays.end(); it3 != itEnd3; ++it3)
      {
        vtkDataArray *array3 = *it3;
        bool array3Valid = vtkDataTypesCompare(array1->GetDataType(),
                                               array3->GetDataType()) != 0;

        if (array1Valid && array2Valid && array3Valid)
        {
          testAssert(dispatcher.Execute(array1, array2, array3, worker),
                     "Dispatch failed.");
          testAssert(worker.Array1 == array1, "Array 1 does not match input.");
          testAssert(worker.Array2 == array2, "Array 2 does not match input.");
          testAssert(worker.Array3 == array3, "Array 3 does not match input.");
        }
        else
        {
          testAssert(!dispatcher.Execute(array1, array2, array3, worker),
                     "Dispatch should have failed.");
          testAssert(worker.Array1 == NULL, "Array 1 should be NULL.");
          testAssert(worker.Array2 == NULL, "Array 2 should be NULL.");
          testAssert(worker.Array3 == NULL, "Array 3 should be NULL.");
        }

        worker.Reset();
      }
    }
  }

  return errors;
}

//------------------------------------------------------------------------------
Arrays::Arrays()
{
  aosDouble = vtkAOSDataArrayTemplate<double>::New();
  aosFloat = vtkAOSDataArrayTemplate<float>::New();
  aosInt = vtkAOSDataArrayTemplate<int>::New();
  aosUnsignedChar = vtkAOSDataArrayTemplate<unsigned char>::New();
  aosIdType = vtkAOSDataArrayTemplate<vtkIdType>::New();

  soaDouble = vtkSOADataArrayTemplate<double>::New();
  soaFloat = vtkSOADataArrayTemplate<float>::New();
  soaInt = vtkSOADataArrayTemplate<int>::New();
  soaUnsignedChar = vtkSOADataArrayTemplate<unsigned char>::New();
  soaIdType = vtkSOADataArrayTemplate<vtkIdType>::New();

  aosArrays.push_back(aosDouble);
  aosArrays.push_back(aosFloat);
  aosArrays.push_back(aosInt);
  aosArrays.push_back(aosUnsignedChar);
  aosArrays.push_back(aosIdType);

  soaArrays.push_back(soaDouble);
  soaArrays.push_back(soaFloat);
  soaArrays.push_back(soaInt);
  soaArrays.push_back(soaUnsignedChar);
  soaArrays.push_back(soaIdType);

  std::copy(aosArrays.begin(), aosArrays.end(), std::back_inserter(allArrays));
  std::copy(soaArrays.begin(), soaArrays.end(), std::back_inserter(allArrays));
}

//------------------------------------------------------------------------------
Arrays::~Arrays()
{
  aosDouble->Delete();
  aosFloat->Delete();
  aosInt->Delete();
  aosUnsignedChar->Delete();
  aosIdType->Delete();
  soaDouble->Delete();
  soaFloat->Delete();
  soaInt->Delete();
  soaUnsignedChar->Delete();
  soaIdType->Delete();
}

} // end anon namespace

//------------------------------------------------------------------------------
int TestArrayDispatchers(int, char *[])
{
  int errors = 0;
  Arrays arrays;
  (void)arrays; // unused, just manages static memory

  errors += TestDispatch();
  errors += TestDispatchByArray();
  errors += TestDispatchByValueType();
  errors += TestDispatch2ByArray();
  errors += TestDispatch2ByValueType();
  errors += TestDispatch2ByArrayWithSameValueType();
  errors += TestDispatch2BySameValueType();
  errors += TestDispatch3ByArray();
  errors += TestDispatch3ByValueType();
  errors += TestDispatch3ByArrayWithSameValueType();
  errors += TestDispatch3BySameValueType();

  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
