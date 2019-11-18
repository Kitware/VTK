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
#include "vtkAbstractArray.h"

// Helpers:
#include "vtkAOSDataArrayTemplate.h"
#include "vtkBitArray.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkSOADataArrayTemplate.h"
#ifdef VTK_USE_SCALED_SOA_ARRAYS
#include "vtkScaledSOADataArrayTemplate.h"
#endif
#include "vtkStringArray.h"

#include <cstdint>
#include <cstdio>

// Forward declare the test function:
namespace
{

//------------------------------------------------------------------------------
struct UseFree
{
  static const int value = vtkAbstractArray::VTK_DATA_ARRAY_FREE;
};

struct UseDelete
{
  static const int value = vtkAbstractArray::VTK_DATA_ARRAY_DELETE;
};

struct UseAlignedFree
{
  static const int value = vtkAbstractArray::VTK_DATA_ARRAY_ALIGNED_FREE;
};

struct UseLambda
{
  static const int value = vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED;
};

int timesLambdaFreeCalled = 0;

//------------------------------------------------------------------------------
#define testAssert(expr, errorMessage)                                                             \
  if (!(expr))                                                                                     \
  {                                                                                                \
    ++errors;                                                                                      \
    vtkGenericWarningMacro(<< "Assertion failed: " #expr << "\n" << errorMessage);                 \
  }

//------------------------------------------------------------------------------
void* make_allocation(UseFree, std::size_t size, int)
{
  return malloc(size);
}

void* make_allocation(UseDelete, std::size_t size, int type)
{
  // Std::string is weird. When UseDelete is passed it binds to a custom
  // free function that casts the memory to std::string*. So to not violate
  // this behavior we need to allocate as a string
  if (type == VTK_STRING)
  {
    return new std::string[size];
  }
  else
  {
    return new uint8_t[size];
  }
}

void* make_allocation(UseAlignedFree, std::size_t size, int)
{
#if defined(_WIN32)
  return _aligned_malloc(size, 16);
#else
  return malloc(size);
#endif
}

void* make_allocation(UseLambda, std::size_t size, int)
{
  return new uint8_t[size];
}

//------------------------------------------------------------------------------
template <typename FreeType>
void assign_user_free(FreeType, vtkAbstractArray*)
{
}

void assign_user_free(UseLambda, vtkAbstractArray* array)
{
  array->SetArrayFreeFunction([](void* ptr) {
    delete[] reinterpret_cast<uint8_t*>(ptr);
    timesLambdaFreeCalled++;
  });
}

//------------------------------------------------------------------------------
template <typename FreeType>
int assign_void_array(
  FreeType, vtkAbstractArray* array, void* ptr, std::size_t size, bool vtkShouldFree)
{
  int errors = 0;
  if (vtkSOADataArrayTemplate<double>* is_soa =
        vtkArrayDownCast<vtkSOADataArrayTemplate<double> >(array))
  {
    is_soa->SetNumberOfComponents(1);
    is_soa->SetArray(0, reinterpret_cast<double*>(ptr), static_cast<vtkIdType>(size), false,
      !vtkShouldFree, FreeType::value);
  }
#ifdef VTK_USE_SCALED_SOA_ARRAYS
  else if (vtkScaledSOADataArrayTemplate<double>* is_scale_soa =
             vtkArrayDownCast<vtkScaledSOADataArrayTemplate<double> >(array))
  {
    is_scale_soa->SetNumberOfComponents(1);
    is_scale_soa->SetArray(0, reinterpret_cast<double*>(ptr), static_cast<vtkIdType>(size), false,
      !vtkShouldFree, FreeType::value);
  }
#endif
  else
  {
    const int save = vtkShouldFree ? 0 : 1;
    array->SetVoidArray(ptr, static_cast<vtkIdType>(size), save, FreeType::value);
    testAssert(array->GetVoidPointer(0) == ptr, "assignment failed");
  }
  return errors;
}

//------------------------------------------------------------------------------
template <typename FreeType>
int ExerciseDelete(FreeType f)
{
  int errors = 0;

  std::cout << "Starting tests for free type: " << f.value << std::endl;

  std::vector<vtkAbstractArray*> arrays;
  arrays.push_back(vtkStringArray::New());
  arrays.push_back(vtkBitArray::New());
  arrays.push_back(vtkFloatArray::New());
  arrays.push_back(vtkAOSDataArrayTemplate<double>::New());
  arrays.push_back(vtkSOADataArrayTemplate<double>::New());
#ifdef VTK_USE_SCALED_SOA_ARRAYS
  arrays.push_back(vtkScaledSOADataArrayTemplate<double>::New());
#endif
  const std::size_t size = 5000;
  for (auto it = arrays.begin(); it != arrays.end(); ++it)
  {

    vtkAbstractArray* array = *it;

    // test setting the array's memory and having it not free the memory
    void* ptr = make_allocation(f, size, array->GetDataType());
    errors += assign_void_array(f, array, ptr, size, false);

    // ask array to free memory, ptr should still be valid
    array->Initialize();

    // This time ask the array to free the memory
    errors += assign_void_array(f, array, ptr, size, true);

    // if we are a lambda we need to set the real free function
    assign_user_free(f, array);

    // free the memory for real this time
    array->Initialize();
  }

  for (auto it = arrays.begin(); it != arrays.end(); ++it)
  {
    vtkAbstractArray* array = *it;
    array->Delete();
  }

  return errors;
}

} // end anon namespace

//-------------Test Entry Point-------------------------------------------------
int TestArrayFreeFunctions(int, char*[])
{
  int errors = 0;

  errors += ExerciseDelete(UseFree{});
  errors += ExerciseDelete(UseDelete{});
  errors += ExerciseDelete(UseAlignedFree{});
  errors += ExerciseDelete(UseLambda{});
  if (timesLambdaFreeCalled != 5)
  {
    std::cerr << "Test failed! Lambda free not called " << std::endl;
  }
  if (errors > 0)
  {
    std::cerr << "Test failed! Error count: " << errors << std::endl;
  }

  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
