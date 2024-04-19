// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInformation.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkPolyData.h"

//----------------------------------------------------------------
// The vtkInformationDataObjectKey uses reinterpret_cast to cast between
// vtkDataObject* and vtkObjectBase*, instead of using static_cast.  See
// the comments in vtkInformationDataObjectKey.cxx for the reasons.
// In this test, we do compile-time and run-time checks to ensure that
// reinterpret_cast does not give the wrong answer.

//----------------------------------------------------------------
// First, some compile-time checks via static_assert, but use conversion
// to void pointer as a surrogate for reinterpret_cast since the latter
// cannot be used in a static_assert.

namespace
{

// This will be used to test static casts with multiple inheritance
struct OtherBase
{
  double x{ 0.0 };
};

// create our own data object type for testing
class TestClass
  : public vtkDataObject
  , public OtherBase
{
public:
  // public constructor, destructor to allow static allocation
  TestClass() = default;
  ~TestClass() override { ReferenceCount = 0; }
};

// a static instantiation for static testing
TestClass inst;

// pointers where C++ knows the object type
constexpr vtkDataObject* inst_dp = &inst;
constexpr vtkObjectBase* inst_bp = &inst;
constexpr OtherBase* inst_op = &inst;
// these hold the address without knowing the object type
constexpr void* inst_void_dp = inst_dp;
constexpr void* inst_void_bp = inst_bp;
constexpr void* inst_void_op = inst_op;

// well defined, will always succeed
static_assert(inst_bp == inst_dp, "Simple typed pointer comparison failed");

// undefined by C++ standard, but expected to be equal
static_assert(inst_void_bp == inst_void_dp, "Single-inherited base address mismatch");

// undefined by C++ standard, but expected to be different
static_assert(inst_void_bp != inst_void_op, "Multi-inherited base addresses match");

} // anonymous namespace

//----------------------------------------------------------------
// The run-time tests
int TestStoreDataObject()
{
  int rval = 0;

  // test storing data object in vtkInformation and then retrieving
  vtkNew<vtkInformation> information;
  vtkNew<vtkPolyData> polydata;

  information->Set(vtkDataObject::DATA_OBJECT(), polydata);
  vtkDataObject* data = information->Get(vtkDataObject::DATA_OBJECT());

  if (data != polydata)
  {
    std::cerr << "Error: Failed to store polydata in vtkInformation" << std::endl;
    rval = 1;
  }

  // vtkInformationDataObjectKey requires these two casts to equivalent
  if (reinterpret_cast<vtkObjectBase*>(data) != static_cast<vtkObjectBase*>(data))
  {
    std::cerr << "Error: Object address changed by static_cast<vtkObjectBase*>(vtkDataObject*)"
              << std::endl;
    rval = 1;
  }

  return rval;
}

int TestInformationDataObjectKey(int, char*[])
{
  return TestStoreDataObject();
}
