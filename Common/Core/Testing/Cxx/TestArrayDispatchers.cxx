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

#include <type_traits> // for std::is_[lr]value_reference
#include <utility>     // for std::move

namespace vtkArrayDispatch
{
typedef vtkTypeList::Unique<                //
  vtkTypeList::Create<                      //
    vtkAOSDataArrayTemplate<double>,        //
    vtkAOSDataArrayTemplate<float>,         //
    vtkAOSDataArrayTemplate<int>,           //
    vtkAOSDataArrayTemplate<unsigned char>, //
    vtkAOSDataArrayTemplate<vtkIdType>,     //
    vtkSOADataArrayTemplate<double>,        //
    vtkSOADataArrayTemplate<float>,         //
    vtkSOADataArrayTemplate<int>,           //
    vtkSOADataArrayTemplate<unsigned char>, //
    vtkSOADataArrayTemplate<vtkIdType>      //
    > >::Result Arrays;
} // end namespace vtkArrayDispatch

#include "vtkArrayDispatch.h"
#include "vtkNew.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace
{

//==============================================================================
// Our functor for testing.
struct TestWorker
{
  TestWorker()
    : Array1(nullptr)
    , Array2(nullptr)
    , Array3(nullptr)
  {
  }

  void Reset()
  {
    this->Array1 = nullptr;
    this->Array2 = nullptr;
    this->Array3 = nullptr;
  }

  template <typename Array1T>
  void operator()(Array1T* array1)
  {
    this->Array1 = array1;
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T* array1, Array2T* array2)
  {
    this->Array1 = array1;
    this->Array2 = array2;
  }

  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T* array1, Array2T* array2, Array3T* array3)
  {
    this->Array1 = array1;
    this->Array2 = array2;
    this->Array3 = array3;
  }

  vtkDataArray* Array1;
  vtkDataArray* Array2;
  vtkDataArray* Array3;
};

//==============================================================================
// Functor for testing parameter forwarding.
struct ForwardedParams
{
  bool Success{ false };

  template <typename ArrayT, typename T1, typename T2>
  void operator()(ArrayT*, T1&& t1, T2&& t2)
  {
    this->Success = std::is_lvalue_reference<T1&&>::value &&
      std::is_rvalue_reference<T2&&>::value && t1 == 42 && t2 == 20;
  }

  template <typename ArrayT1, typename ArrayT2, typename T1, typename T2>
  void operator()(ArrayT1*, ArrayT2*, T1&& t1, T2&& t2)
  {
    this->Success = std::is_lvalue_reference<T1&&>::value &&
      std::is_rvalue_reference<T2&&>::value && t1 == 42 && t2 == 20;
  }

  template <typename ArrayT1, typename ArrayT2, typename ArrayT3, typename T1, typename T2>
  void operator()(ArrayT1*, ArrayT2*, ArrayT3*, T1&& t1, T2&& t2)
  {
    this->Success = std::is_lvalue_reference<T1&&>::value &&
      std::is_rvalue_reference<T2&&>::value && t1 == 42 && t2 == 20;
  }

  void Reset() { this->Success = false; }
};

//==============================================================================
// Functor to test that rvalue functors work:
bool ForwardedFunctorCalled = false; // global for validating calls
struct ForwardedFunctor
{
  void operator()(...) const
  {
    assert(!ForwardedFunctorCalled);
    ForwardedFunctorCalled = true;
  }
};

//==============================================================================
// Container for testing arrays.
struct Arrays
{
  Arrays();
  ~Arrays();

  static vtkAOSDataArrayTemplate<double>* aosDouble;
  static vtkAOSDataArrayTemplate<float>* aosFloat;
  static vtkAOSDataArrayTemplate<int>* aosInt;
  static vtkAOSDataArrayTemplate<unsigned char>* aosUnsignedChar;
  static vtkAOSDataArrayTemplate<vtkIdType>* aosIdType;

  static vtkSOADataArrayTemplate<double>* soaDouble;
  static vtkSOADataArrayTemplate<float>* soaFloat;
  static vtkSOADataArrayTemplate<int>* soaInt;
  static vtkSOADataArrayTemplate<unsigned char>* soaUnsignedChar;
  static vtkSOADataArrayTemplate<vtkIdType>* soaIdType;

  static std::vector<vtkDataArray*> aosArrays;
  static std::vector<vtkDataArray*> soaArrays;
  static std::vector<vtkDataArray*> allArrays;
};

vtkAOSDataArrayTemplate<double>* Arrays::aosDouble;
vtkAOSDataArrayTemplate<float>* Arrays::aosFloat;
vtkAOSDataArrayTemplate<int>* Arrays::aosInt;
vtkAOSDataArrayTemplate<unsigned char>* Arrays::aosUnsignedChar;
vtkAOSDataArrayTemplate<vtkIdType>* Arrays::aosIdType;
vtkSOADataArrayTemplate<double>* Arrays::soaDouble;
vtkSOADataArrayTemplate<float>* Arrays::soaFloat;
vtkSOADataArrayTemplate<int>* Arrays::soaInt;
vtkSOADataArrayTemplate<unsigned char>* Arrays::soaUnsignedChar;
vtkSOADataArrayTemplate<vtkIdType>* Arrays::soaIdType;

std::vector<vtkDataArray*> Arrays::aosArrays;
std::vector<vtkDataArray*> Arrays::soaArrays;
std::vector<vtkDataArray*> Arrays::allArrays;

//==============================================================================
// Miscellaneous Debris
typedef std::vector<vtkDataArray*>::iterator ArrayIter;

typedef vtkTypeList::Create<              //
  vtkAOSDataArrayTemplate<double>,        //
  vtkAOSDataArrayTemplate<float>,         //
  vtkAOSDataArrayTemplate<int>,           //
  vtkAOSDataArrayTemplate<unsigned char>, //
  vtkAOSDataArrayTemplate<vtkIdType>      //
  >
  AoSArrayList;
typedef vtkTypeList::Create<              //
  vtkSOADataArrayTemplate<double>,        //
  vtkSOADataArrayTemplate<float>,         //
  vtkSOADataArrayTemplate<int>,           //
  vtkSOADataArrayTemplate<unsigned char>, //
  vtkSOADataArrayTemplate<vtkIdType>      //
  >
  SoAArrayList;

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
#define testAssert(expr, errorMessage)                                                             \
  if (!(expr))                                                                                     \
  {                                                                                                \
    ++errors;                                                                                      \
    vtkGenericWarningMacro(<< "Assertion failed: " #expr << "\n" << errorMessage);                 \
  }                                                                                                \
  []() {}() /* Swallow semi-colon */

//------------------------------------------------------------------------------
int TestDispatch()
{
  int errors = 0;

  using Dispatcher = vtkArrayDispatch::Dispatch;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array : Arrays::allArrays)
  {
    testAssert(Dispatcher::Execute(array, worker), "Dispatch failed.");
    testAssert(worker.Array1 == array, "Array 1 does not match input.");
    worker.Reset();

    int lval{ 42 };
    int rval{ 20 };
    testAssert(Dispatcher::Execute(array, paramTester, lval, std::move(rval)),
      "Parameter forwarding dispatch failed.");
    testAssert(paramTester.Success, "Parameter forwarding failed.");
    paramTester.Reset();

    testAssert(
      Dispatcher::Execute(array, ForwardedFunctor{}), "Functor forwarding dispatch failed.");
    testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
    ForwardedFunctorCalled = false;
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatchByArray()
{
  int errors = 0;

  using Dispatcher = vtkArrayDispatch::DispatchByArray<AoSArrayList>;
  TestWorker worker;
  ForwardedParams paramTester;

  // AoS arrays: All should pass:
  for (vtkDataArray* array : Arrays::aosArrays)
  {
    testAssert(Dispatcher::Execute(array, worker), "Dispatch failed.");
    testAssert(worker.Array1 == array, "Array 1 does not match input.");
    worker.Reset();

    int lval{ 42 };
    int rval{ 20 };
    testAssert(Dispatcher::Execute(array, paramTester, lval, std::move(rval)),
      "Parameter forwarding dispatch failed.");
    testAssert(paramTester.Success, "Parameter forwarding failed.");
    paramTester.Reset();

    testAssert(
      Dispatcher::Execute(array, ForwardedFunctor{}), "Functor forwarding dispatch failed.");
    testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
    ForwardedFunctorCalled = false;
  }

  // AoS arrays: All should fail:
  for (ArrayIter it = Arrays::soaArrays.begin(), itEnd = Arrays::soaArrays.end(); it != itEnd; ++it)
  {
    vtkDataArray* array = *it;
    testAssert(!Dispatcher::Execute(array, worker), "Dispatch should have failed.");
    testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
    worker.Reset();
  }

  return errors;
}

//------------------------------------------------------------------------------
int TestDispatchByValueType()
{
  int errors = 0;

  // Create dispatcher that only generates code paths for arrays with reals.
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array : Arrays::allArrays)
  {
    bool isValid = isReal(array->GetDataType());

    if (isValid)
    {
      testAssert(Dispatcher::Execute(array, worker), "Dispatch failed.");
      testAssert(worker.Array1 == array, "Array 1 does not match input.");
      worker.Reset();

      int lval{ 42 };
      int rval{ 20 };
      testAssert(Dispatcher::Execute(array, paramTester, lval, std::move(rval)),
        "Parameter forwarding dispatch failed.");
      testAssert(paramTester.Success, "Parameter forwarding failed.");
      paramTester.Reset();

      testAssert(
        Dispatcher::Execute(array, ForwardedFunctor{}), "Functor forwarding dispatch failed.");
      testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
      ForwardedFunctorCalled = false;
    }
    else
    {
      testAssert(!Dispatcher::Execute(array, worker), "Dispatch should have failed.");
      testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
      worker.Reset();
    }
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
  using Dispatcher = vtkArrayDispatch::Dispatch2ByArray<SoAArrayList, AoSArrayList>;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array1 : Arrays::allArrays)
  {
    bool array1Valid = array1->GetArrayType() == vtkAbstractArray::SoADataArrayTemplate;

    for (vtkDataArray* array2 : Arrays::allArrays)
    {
      bool array2Valid = array2->GetArrayType() == vtkAbstractArray::AoSDataArrayTemplate;

      if (array1Valid && array2Valid)
      {
        testAssert(Dispatcher::Execute(array1, array2, worker), "Dispatch failed.");
        testAssert(worker.Array1 == array1, "Array 1 does not match input.");
        testAssert(worker.Array2 == array2, "Array 2 does not match input.");
        worker.Reset();

        int lval{ 42 };
        int rval{ 20 };
        testAssert(Dispatcher::Execute(array1, array2, paramTester, lval, std::move(rval)),
          "Parameter forwarding dispatch failed.");
        testAssert(paramTester.Success, "Parameter forwarding failed.");
        paramTester.Reset();

        testAssert(Dispatcher::Execute(array1, array2, ForwardedFunctor{}),
          "Functor forwarding dispatch failed.");
        testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
        ForwardedFunctorCalled = false;
      }
      else
      {
        testAssert(!Dispatcher::Execute(array1, array2, worker), "Dispatch should have failed.");
        testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
        testAssert(worker.Array2 == nullptr, "Array 2 should be nullptr.");
        worker.Reset();
      }
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
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Integrals, vtkArrayDispatch::Reals>;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array1 : Arrays::allArrays)
  {
    bool array1Valid = isIntegral(array1->GetDataType());

    for (vtkDataArray* array2 : Arrays::allArrays)
    {
      bool array2Valid = isReal(array2->GetDataType());

      if (array1Valid && array2Valid)
      {
        testAssert(Dispatcher::Execute(array1, array2, worker), "Dispatch failed.");
        testAssert(worker.Array1 == array1, "Array 1 does not match input.");
        testAssert(worker.Array2 == array2, "Array 2 does not match input.");
        worker.Reset();

        int lval{ 42 };
        int rval{ 20 };
        testAssert(Dispatcher::Execute(array1, array2, paramTester, lval, std::move(rval)),
          "Parameter forwarding dispatch failed.");
        testAssert(paramTester.Success, "Parameter forwarding failed.");
        paramTester.Reset();

        testAssert(Dispatcher::Execute(array1, array2, ForwardedFunctor{}),
          "Functor forwarding dispatch failed.");
        testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
        ForwardedFunctorCalled = false;
      }
      else
      {
        testAssert(!Dispatcher::Execute(array1, array2, worker), "Dispatch should have failed.");
        testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
        testAssert(worker.Array2 == nullptr, "Array 2 should be nullptr.");
        worker.Reset();
      }
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
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<AoSArrayList, SoAArrayList>;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array1 : Arrays::allArrays)
  {
    bool array1Valid = array1->GetArrayType() == vtkAbstractArray::AoSDataArrayTemplate;

    for (vtkDataArray* array2 : Arrays::allArrays)
    {
      bool array2Valid = array2->GetArrayType() == vtkAbstractArray::SoADataArrayTemplate &&
        vtkDataTypesCompare(array1->GetDataType(), array2->GetDataType());

      if (array1Valid && array2Valid)
      {
        testAssert(Dispatcher::Execute(array1, array2, worker), "Dispatch failed.");
        testAssert(worker.Array1 == array1, "Array 1 does not match input.");
        testAssert(worker.Array2 == array2, "Array 2 does not match input.");
        worker.Reset();

        int lval{ 42 };
        int rval{ 20 };
        testAssert(Dispatcher::Execute(array1, array2, paramTester, lval, std::move(rval)),
          "Parameter forwarding dispatch failed.");
        testAssert(paramTester.Success, "Parameter forwarding failed.");
        paramTester.Reset();

        testAssert(Dispatcher::Execute(array1, array2, ForwardedFunctor{}),
          "Functor forwarding dispatch failed.");
        testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
        ForwardedFunctorCalled = false;
      }
      else
      {
        testAssert(!Dispatcher::Execute(array1, array2, worker), "Dispatch should have failed.");
        testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
        testAssert(worker.Array2 == nullptr, "Array 2 should be nullptr.");
        worker.Reset();
      }
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
  using Dispatcher = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::Integrals>;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array1 : Arrays::allArrays)
  {
    bool array1Valid = isIntegral(array1->GetDataType());

    for (vtkDataArray* array2 : Arrays::allArrays)
    {
      bool array2Valid = vtkDataTypesCompare(array1->GetDataType(), array2->GetDataType()) != 0;

      if (array1Valid && array2Valid)
      {
        testAssert(Dispatcher::Execute(array1, array2, worker), "Dispatch failed.");
        testAssert(worker.Array1 == array1, "Array 1 does not match input.");
        testAssert(worker.Array2 == array2, "Array 2 does not match input.");
        worker.Reset();

        int lval{ 42 };
        int rval{ 20 };
        testAssert(Dispatcher::Execute(array1, array2, paramTester, lval, std::move(rval)),
          "Parameter forwarding dispatch failed.");
        testAssert(paramTester.Success, "Parameter forwarding failed.");
        paramTester.Reset();

        testAssert(Dispatcher::Execute(array1, array2, ForwardedFunctor{}),
          "Functor forwarding dispatch failed.");
        testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
        ForwardedFunctorCalled = false;
      }
      else
      {
        testAssert(!Dispatcher::Execute(array1, array2, worker), "Dispatch should have failed.");
        testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
        testAssert(worker.Array2 == nullptr, "Array 2 should be nullptr.");
        worker.Reset();
      }
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
  using Dispatcher = vtkArrayDispatch::Dispatch3ByArray<SoAArrayList, AoSArrayList,
    vtkTypeList::Create<vtkAOSDataArrayTemplate<float>, vtkSOADataArrayTemplate<float> > >;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array1 : Arrays::allArrays)
  {
    bool array1Valid = array1->GetArrayType() == vtkAbstractArray::SoADataArrayTemplate;

    for (vtkDataArray* array2 : Arrays::allArrays)
    {
      bool array2Valid = array2->GetArrayType() == vtkAbstractArray::AoSDataArrayTemplate;

      for (vtkDataArray* array3 : Arrays::allArrays)
      {
        bool array3Valid = array3->GetDataType() == VTK_FLOAT;

        if (array1Valid && array2Valid && array3Valid)
        {
          testAssert(Dispatcher::Execute(array1, array2, array3, worker), "Dispatch failed.");
          testAssert(worker.Array1 == array1, "Array 1 does not match input.");
          testAssert(worker.Array2 == array2, "Array 2 does not match input.");
          testAssert(worker.Array3 == array3, "Array 3 does not match input.");
          worker.Reset();

          int lval{ 42 };
          int rval{ 20 };
          testAssert(
            Dispatcher::Execute(array1, array2, array3, paramTester, lval, std::move(rval)),
            "Parameter forwarding dispatch failed.");
          testAssert(paramTester.Success, "Parameter forwarding failed.");
          paramTester.Reset();

          testAssert(Dispatcher::Execute(array1, array2, array3, ForwardedFunctor{}),
            "Functor forwarding dispatch failed.");
          testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
          ForwardedFunctorCalled = false;
        }
        else
        {
          testAssert(
            !Dispatcher::Execute(array1, array2, array3, worker), "Dispatch should have failed.");
          testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
          testAssert(worker.Array2 == nullptr, "Array 2 should be nullptr.");
          testAssert(worker.Array3 == nullptr, "Array 3 should be nullptr.");
          worker.Reset();
        }
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
  using Dispatcher = vtkArrayDispatch::Dispatch3ByValueType<vtkArrayDispatch::Reals,
    vtkArrayDispatch::Integrals, vtkTypeList::Create<unsigned char> >;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array1 : Arrays::allArrays)
  {
    bool array1Valid = isReal(array1->GetDataType());

    for (vtkDataArray* array2 : Arrays::allArrays)
    {
      bool array2Valid = isIntegral(array2->GetDataType());

      for (vtkDataArray* array3 : Arrays::allArrays)
      {
        bool array3Valid = vtkDataTypesCompare(array3->GetDataType(), VTK_UNSIGNED_CHAR) != 0;

        if (array1Valid && array2Valid && array3Valid)
        {
          testAssert(Dispatcher::Execute(array1, array2, array3, worker), "Dispatch failed.");
          testAssert(worker.Array1 == array1, "Array 1 does not match input.");
          testAssert(worker.Array2 == array2, "Array 2 does not match input.");
          testAssert(worker.Array3 == array3, "Array 3 does not match input.");
          worker.Reset();

          int lval{ 42 };
          int rval{ 20 };
          testAssert(
            Dispatcher::Execute(array1, array2, array3, paramTester, lval, std::move(rval)),
            "Parameter forwarding dispatch failed.");
          testAssert(paramTester.Success, "Parameter forwarding failed.");
          paramTester.Reset();

          testAssert(Dispatcher::Execute(array1, array2, array3, ForwardedFunctor{}),
            "Functor forwarding dispatch failed.");
          testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
          ForwardedFunctorCalled = false;
        }
        else
        {
          testAssert(
            !Dispatcher::Execute(array1, array2, array3, worker), "Dispatch should have failed.");
          testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
          testAssert(worker.Array2 == nullptr, "Array 2 should be nullptr.");
          testAssert(worker.Array3 == nullptr, "Array 3 should be nullptr.");
          worker.Reset();
        }
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
  using Dispatcher =
    vtkArrayDispatch::Dispatch3ByArrayWithSameValueType<SoAArrayList, AoSArrayList, AllArrayList>;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array1 : Arrays::allArrays)
  {
    bool array1Valid = array1->GetArrayType() == vtkAbstractArray::SoADataArrayTemplate;

    for (vtkDataArray* array2 : Arrays::allArrays)
    {
      bool array2Valid = array2->GetArrayType() == vtkAbstractArray::AoSDataArrayTemplate &&
        vtkDataTypesCompare(array1->GetDataType(), array2->GetDataType());

      for (vtkDataArray* array3 : Arrays::allArrays)
      {
        bool array3Valid = vtkDataTypesCompare(array1->GetDataType(), array3->GetDataType()) != 0;

        if (array1Valid && array2Valid && array3Valid)
        {
          testAssert(Dispatcher::Execute(array1, array2, array3, worker), "Dispatch failed.");
          testAssert(worker.Array1 == array1, "Array 1 does not match input.");
          testAssert(worker.Array2 == array2, "Array 2 does not match input.");
          testAssert(worker.Array3 == array3, "Array 3 does not match input.");
          worker.Reset();

          int lval{ 42 };
          int rval{ 20 };
          testAssert(
            Dispatcher::Execute(array1, array2, array3, paramTester, lval, std::move(rval)),
            "Parameter forwarding dispatch failed.");
          testAssert(paramTester.Success, "Parameter forwarding failed.");
          paramTester.Reset();

          testAssert(Dispatcher::Execute(array1, array2, array3, ForwardedFunctor{}),
            "Functor forwarding dispatch failed.");
          testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
          ForwardedFunctorCalled = false;
        }
        else
        {
          testAssert(
            !Dispatcher::Execute(array1, array2, array3, worker), "Dispatch should have failed.");
          testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
          testAssert(worker.Array2 == nullptr, "Array 2 should be nullptr.");
          testAssert(worker.Array3 == nullptr, "Array 3 should be nullptr.");
          worker.Reset();
        }
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
  using Dispatcher = vtkArrayDispatch::Dispatch3BySameValueType<
    vtkTypeList::Append<vtkArrayDispatch::Reals, unsigned char>::Result>;
  TestWorker worker;
  ForwardedParams paramTester;

  for (vtkDataArray* array1 : Arrays::allArrays)
  {
    bool array1Valid = isReal(array1->GetDataType()) ||
      vtkDataTypesCompare(array1->GetDataType(), VTK_UNSIGNED_CHAR);

    for (vtkDataArray* array2 : Arrays::allArrays)
    {
      bool array2Valid = vtkDataTypesCompare(array1->GetDataType(), array2->GetDataType()) != 0;

      for (vtkDataArray* array3 : Arrays::allArrays)
      {
        bool array3Valid = vtkDataTypesCompare(array1->GetDataType(), array3->GetDataType()) != 0;

        if (array1Valid && array2Valid && array3Valid)
        {
          testAssert(Dispatcher::Execute(array1, array2, array3, worker), "Dispatch failed.");
          testAssert(worker.Array1 == array1, "Array 1 does not match input.");
          testAssert(worker.Array2 == array2, "Array 2 does not match input.");
          testAssert(worker.Array3 == array3, "Array 3 does not match input.");
          worker.Reset();

          int lval{ 42 };
          int rval{ 20 };
          testAssert(
            Dispatcher::Execute(array1, array2, array3, paramTester, lval, std::move(rval)),
            "Parameter forwarding dispatch failed.");
          testAssert(paramTester.Success, "Parameter forwarding failed.");
          paramTester.Reset();

          testAssert(Dispatcher::Execute(array1, array2, array3, ForwardedFunctor{}),
            "Functor forwarding dispatch failed.");
          testAssert(ForwardedFunctorCalled, "Functor forwarding failed.");
          ForwardedFunctorCalled = false;
        }
        else
        {
          testAssert(
            !Dispatcher::Execute(array1, array2, array3, worker), "Dispatch should have failed.");
          testAssert(worker.Array1 == nullptr, "Array 1 should be nullptr.");
          testAssert(worker.Array2 == nullptr, "Array 2 should be nullptr.");
          testAssert(worker.Array3 == nullptr, "Array 3 should be nullptr.");
          worker.Reset();
        }
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
int TestArrayDispatchers(int, char*[])
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
