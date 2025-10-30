// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"

#include "MockDataArray.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellArrayIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkImplicitArray.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSmartPointer.h"
#include "vtkTriangle.h"

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>

namespace
{

[[noreturn]] void ThrowAssertError(const std::string& msg)
{
  // You can set breakpoints here:
  throw std::runtime_error(msg);
}

#define TEST_ASSERT(cond)                                                                          \
  do                                                                                               \
  {                                                                                                \
    std::cout << "=> Check " << #cond << " ... ";                                                  \
    if (!(cond))                                                                                   \
    {                                                                                              \
      std::cout << "false" << std::endl;                                                           \
      ThrowAssertError(vtkQuoteMacro(__FILE__) ":" vtkQuoteMacro(                                  \
        __LINE__) ": test assertion failed: (" #cond ")");                                         \
    }                                                                                              \
    std::cout << "true" << std::endl;                                                              \
  } while (false)

template <vtkCellArray::StorageTypes StorageType, typename... ArrayTypes>
struct CellArrayFactory
{
  static vtkSmartPointer<vtkCellArray> New() { return nullptr; }
};

template <>
struct CellArrayFactory<vtkCellArray::StorageTypes::Int32>
{
  static vtkSmartPointer<vtkCellArray> New()
  {
    auto cellArray = vtkSmartPointer<vtkCellArray>::New();
    cellArray->Use32BitStorage();
    TEST_ASSERT(cellArray->GetStorageType() == vtkCellArray::StorageTypes::Int32);
    return cellArray;
  }
};

template <>
struct CellArrayFactory<vtkCellArray::StorageTypes::Int64>
{
  static vtkSmartPointer<vtkCellArray> New()
  {
    auto cellArray = vtkSmartPointer<vtkCellArray>::New();
    cellArray->Use64BitStorage();
    TEST_ASSERT(cellArray->GetStorageType() == vtkCellArray::StorageTypes::Int64);
    return cellArray;
  }
};

template <>
struct CellArrayFactory<vtkCellArray::StorageTypes::FixedSizeInt32>
{
  static vtkSmartPointer<vtkCellArray> New()
  {
    auto cellArray = vtkSmartPointer<vtkCellArray>::New();
    cellArray->UseFixedSize32BitStorage(3);
    TEST_ASSERT(cellArray->GetStorageType() == vtkCellArray::StorageTypes::FixedSizeInt32);
    return cellArray;
  }
};

template <>
struct CellArrayFactory<vtkCellArray::StorageTypes::FixedSizeInt64>
{
  static vtkSmartPointer<vtkCellArray> New()
  {
    auto cellArray = vtkSmartPointer<vtkCellArray>::New();
    cellArray->UseFixedSize64BitStorage(3);
    TEST_ASSERT(cellArray->GetStorageType() == vtkCellArray::StorageTypes::FixedSizeInt64);
    return cellArray;
  }
};

struct DummyWorker
{
  template <typename OffsetsT, typename ConnectivityT>
  [[maybe_unused]] void operator()(OffsetsT*, ConnectivityT*)
  {
    void();
  }
};

template <typename ConnectivityArrayT, typename OffsetsArrayT>
struct CellArrayFactory<vtkCellArray::StorageTypes::Generic, ConnectivityArrayT, OffsetsArrayT>
{
  static vtkSmartPointer<vtkCellArray> New()
  {
    using OffsetsValueT = typename OffsetsArrayT::ValueType;
    auto cellArray = vtkSmartPointer<vtkCellArray>::New();
    // By passing array types which are NOT in vtkCellArray::InputArrayList,
    // vtkCellArray can be put into the "Generic" storage mode.
    vtkNew<ConnectivityArrayT> placeholderConn;
    vtkNew<OffsetsArrayT> placeholderOffsets;
    if (auto countingOffsets = vtkAffineArray<OffsetsValueT>::FastDownCast(placeholderOffsets))
    {
      countingOffsets->ConstructBackend(3, 0);
    }
    // initialize the offsets array with one element i.e, number of elements in the connectivity.
    placeholderOffsets->InsertNextValue(0);
    cellArray->SetData(placeholderOffsets, placeholderConn);
    DummyWorker worker;
    using Dispatcher =
      vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<vtkArrayDispatch::StorageOffsetsArrays,
        vtkArrayDispatch::StorageConnectivityArrays>;
    // Ensure that the arrays are indeed not in the InputArrayList
    if (!Dispatcher::Execute(
          cellArray->GetOffsetsArray(), cellArray->GetConnectivityArray(), worker))
    {
      TEST_ASSERT(cellArray->GetStorageType() == vtkCellArray::StorageTypes::Generic);
    }
    else
    {
      TEST_ASSERT(cellArray->GetStorageType() != vtkCellArray::StorageTypes::Generic);
    }
    return cellArray;
  }
};

template <bool FixedSize>
vtkSmartPointer<vtkIdList> GetCellIds(vtkIdType cellId, bool reverse = false, vtkIdType offset = 0)
{
  auto ids = vtkSmartPointer<vtkIdList>::New();
  if (FixedSize)
  {
    if (cellId == 0)
    {
      ids->InsertNextId(0);
      ids->InsertNextId(1);
      ids->InsertNextId(2);
    }
    else if (cellId == 1)
    {
      ids->InsertNextId(3);
      ids->InsertNextId(4);
      ids->InsertNextId(5);
    }
    else if (cellId == 2)
    {
      ids->InsertNextId(7);
      ids->InsertNextId(8);
      ids->InsertNextId(9);
    }
    else
    {
      throw std::runtime_error("Invalid cellId");
    }
  }
  else
  {
    if (cellId == 0)
    {
      ids->InsertNextId(0);
      ids->InsertNextId(1);
      ids->InsertNextId(2);
      ids->InsertNextId(3);
      ids->InsertNextId(4);
    }
    else if (cellId == 1)
    {
      ids->InsertNextId(3);
      ids->InsertNextId(4);
      ids->InsertNextId(5);
    }
    else if (cellId == 2)
    {
      ids->InsertNextId(7);
      ids->InsertNextId(8);
      ids->InsertNextId(9);
      ids->InsertNextId(4);
      ids->InsertNextId(2);
      ids->InsertNextId(1);
    }
    else
    {
      throw std::runtime_error("Invalid cellId");
    }
  }
  if (reverse)
  {
    std::reverse(ids->begin(), ids->end());
  }
  if (offset > 0)
  {
    for (vtkIdType i = 0; i < ids->GetNumberOfIds(); ++i)
    {
      ids->SetId(i, ids->GetId(i) + offset);
    }
  }
  return ids;
}

template <bool FixedSize>
void FillCellArray(vtkCellArray* cellArray, bool reverse = false)
{
  cellArray->InsertNextCell(GetCellIds<FixedSize>(0, reverse));
  cellArray->InsertNextCell(GetCellIds<FixedSize>(1, reverse));
  cellArray->InsertNextCell(GetCellIds<FixedSize>(2, reverse));
}

template <bool FixedSize>
bool ValidateCell(vtkIdType cellId, vtkIdType npts, const vtkIdType* pts, bool reverse = false,
  vtkIdType offset = 0)
{
  auto ids = GetCellIds<FixedSize>(cellId, reverse, offset);
  TEST_ASSERT(npts == ids->GetNumberOfIds());
  bool equal = true;
  for (vtkIdType i = 0; i < npts; ++i)
  {
    equal &= pts[i] == ids->GetId(i);
  }
  return equal;
}

template <bool FixedSize>
void ValidateCellArray(vtkCellArray* cellArray, bool reverse = false, vtkIdType offset = 0)
{
  vtkIdType npts;
  const vtkIdType* pts;
  auto it = vtk::TakeSmartPointer(cellArray->NewIterator());
  it->GoToFirstCell();

  for (int i = 0; i < 3; ++i)
  {
    TEST_ASSERT(!it->IsDoneWithTraversal());
    it->GetCurrentCell(npts, pts);
    TEST_ASSERT(ValidateCell<FixedSize>(i, npts, pts, reverse, offset));
    it->GoToNextCell();
  }

  TEST_ASSERT(it->IsDoneWithTraversal());
}

void TestAllocate(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Initialize();
  cellArray->AllocateEstimate(100, 4);
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetSize() == 101);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetSize() == 400);

  cellArray->Initialize();
  cellArray->AllocateExact(100, 256);
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetSize() == 101);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetSize() == 256);
}

void TestResize(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Initialize();
  cellArray->ResizeExact(128, 256);
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetNumberOfValues() == 129);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetNumberOfValues() == 256);
}

void TestInitialize(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Initialize();
  cellArray->ResizeExact(128, 256);
  cellArray->Initialize();
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetNumberOfValues() == 1);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetNumberOfValues() == 0);
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetSize() == 1);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetSize() == 0);
}

template <bool FixedSize>
void TestSqueeze(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Initialize();
  cellArray->AllocateExact(128, 256);

  FillCellArray<FixedSize>(cellArray);

  cellArray->Squeeze();

  TEST_ASSERT(cellArray->GetOffsetsArray()->GetNumberOfValues() == 4);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetNumberOfValues() == (FixedSize ? 9 : 14));
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetSize() == 4);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetSize() == (FixedSize ? 9 : 14));

  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
  ValidateCellArray<FixedSize>(cellArray);
}

void TestReset(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Initialize();
  cellArray->ResizeExact(128, 256);
  cellArray->Reset();
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetNumberOfValues() == 1);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetNumberOfValues() == 0);
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetSize() >= 129);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetSize() >= 256);
}

template <bool FixedSize>
void TestIsValidOffsets(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->IsValid());
  FillCellArray<FixedSize>(cellArray);
  TEST_ASSERT(cellArray->IsValid());

  cellArray->GetOffsetsArray()->Reset();
  TEST_ASSERT(!cellArray->IsValid());
  // This should reuse the old buffer:
  cellArray->GetOffsetsArray()->SetNumberOfValues(4);
  TEST_ASSERT(cellArray->IsValid());

  cellArray->GetOffsetsArray()->SetComponent(1, 0, 5);
  cellArray->GetOffsetsArray()->SetComponent(2, 0, 3);
  TEST_ASSERT(!cellArray->IsValid());
  cellArray->GetOffsetsArray()->SetComponent(1, 0, 3);
  cellArray->GetOffsetsArray()->SetComponent(2, 0, 5);
  TEST_ASSERT(cellArray->IsValid());
}

template <bool FixedSize>
void TestIsValidConnectivity(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->IsValid());
  FillCellArray<FixedSize>(cellArray);
  TEST_ASSERT(cellArray->IsValid());

  cellArray->GetConnectivityArray()->SetNumberOfValues(5);
  TEST_ASSERT(!cellArray->IsValid());
  cellArray->GetConnectivityArray()->SetNumberOfValues(4);
  TEST_ASSERT(!cellArray->IsValid());
  cellArray->GetConnectivityArray()->SetNumberOfValues(FixedSize ? 9 : 14);
  TEST_ASSERT(cellArray->IsValid());
}

template <bool FixedSize>
void TestGetNumberOfCells(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 0);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfCells() == 0);

  FillCellArray<FixedSize>(cellArray);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfCells() == 0);
}

template <bool FixedSize>
void TestGetNumberOfOffsets(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);

  FillCellArray<FixedSize>(cellArray);

  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 4);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
}

template <bool FixedSize>
void TestGetNumberOfConnectivityIds(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);

  FillCellArray<FixedSize>(cellArray);

  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == (FixedSize ? 9 : 14));

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

template <bool FixedSize>
void TestNewIterator(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  {
    auto iter = vtk::TakeSmartPointer(cellArray->NewIterator());
    TEST_ASSERT(iter->IsDoneWithTraversal());
    iter->GoToFirstCell();
    TEST_ASSERT(iter->IsDoneWithTraversal());
  }

  cellArray->Initialize();

  {
    auto iter = vtk::TakeSmartPointer(cellArray->NewIterator());
    TEST_ASSERT(iter->IsDoneWithTraversal());
    iter->GoToFirstCell();
    TEST_ASSERT(iter->IsDoneWithTraversal());
  }

  FillCellArray<FixedSize>(cellArray);

  ValidateCellArray<FixedSize>(cellArray);

  cellArray->Initialize();

  {
    auto iter = vtk::TakeSmartPointer(cellArray->NewIterator());
    TEST_ASSERT(iter->IsDoneWithTraversal());
    iter->GoToFirstCell();
    TEST_ASSERT(iter->IsDoneWithTraversal());
  }
}

template <typename OffsetsArrayType, typename ConnectivityArrayType>
void TestSetDataImpl(vtkSmartPointer<vtkCellArray> cellArray, bool checkNoCopy)
{
  vtkLogScopeFunction(INFO);
  using OffsetsValueType = typename OffsetsArrayType::ValueType;
  using ConnectivityValueType = typename ConnectivityArrayType::ValueType;
  constexpr bool FixedSize = std::is_base_of_v<vtkAffineArray<OffsetsValueType>, OffsetsArrayType>;
  vtkNew<vtkCellArray> test;
  test->DeepCopy(cellArray); // copy config settings

  vtkNew<OffsetsArrayType> offsets;
  if (auto countingOffsets = vtkAffineArray<OffsetsValueType>::SafeDownCast(offsets))
  {
    countingOffsets->ConstructBackend(GetCellIds<FixedSize>(0)->GetNumberOfIds(), 0);
  }
  vtkNew<ConnectivityArrayType> conn;
  offsets->InsertNextValue(0);
  for (vtkIdType i = 0; i < 3; ++i)
  {
    auto ids = GetCellIds<FixedSize>(i);
    offsets->InsertNextValue(offsets->GetValue(i) + ids->GetNumberOfIds());
    for (vtkIdType j = 0; j < ids->GetNumberOfIds(); ++j)
    {
      conn->InsertNextValue(ids->GetId(j));
    }
  }
  test->SetData(offsets, conn);

  if (std::is_base_of_v<vtkAOSDataArrayTemplate<OffsetsValueType>, OffsetsArrayType> &&
    std::is_base_of_v<vtkAOSDataArrayTemplate<ConnectivityValueType>, ConnectivityArrayType>)
  {
    if (sizeof(OffsetsValueType) == 4 && sizeof(ConnectivityValueType) == 4)
    {
      TEST_ASSERT(!test->IsStorage64Bit());
      TEST_ASSERT(test->IsStorage32Bit());
      TEST_ASSERT(!test->IsStorageFixedSize64Bit());
      TEST_ASSERT(!test->IsStorageFixedSize32Bit());
      TEST_ASSERT(!test->IsStorageGeneric());
    }
    else
    {
      TEST_ASSERT(test->IsStorage64Bit());
      TEST_ASSERT(!test->IsStorage32Bit());
      TEST_ASSERT(!test->IsStorageFixedSize64Bit());
      TEST_ASSERT(!test->IsStorageFixedSize32Bit());
      TEST_ASSERT(!test->IsStorageGeneric());
    }
  }
  else if (std::is_base_of_v<vtkAffineArray<OffsetsValueType>, OffsetsArrayType> &&
    std::is_base_of_v<vtkAOSDataArrayTemplate<ConnectivityValueType>, ConnectivityArrayType>)
  {
    if (sizeof(OffsetsValueType) == 4 && sizeof(ConnectivityValueType) == 4)
    {
      TEST_ASSERT(!test->IsStorage64Bit());
      TEST_ASSERT(!test->IsStorage32Bit());
      TEST_ASSERT(test->IsStorageFixedSize32Bit());
      TEST_ASSERT(!test->IsStorageFixedSize64Bit());
      TEST_ASSERT(!test->IsStorageGeneric());
    }
    else
    {
      TEST_ASSERT(!test->IsStorage64Bit());
      TEST_ASSERT(!test->IsStorage32Bit());
      TEST_ASSERT(test->IsStorageFixedSize64Bit());
      TEST_ASSERT(!test->IsStorageFixedSize32Bit());
      TEST_ASSERT(!test->IsStorageGeneric());
    }
  }
  else
  {
    TEST_ASSERT(!test->IsStorage64Bit());
    TEST_ASSERT(!test->IsStorage32Bit());
    TEST_ASSERT(!test->IsStorageFixedSize64Bit());
    TEST_ASSERT(!test->IsStorageFixedSize32Bit());
    TEST_ASSERT(test->IsStorageGeneric());
  }

  if (checkNoCopy)
  {
    TEST_ASSERT(test->GetConnectivityArray()->GetVoidPointer(0) == conn->GetPointer(0));
    if (!std::is_base_of_v<vtkAffineArray<OffsetsValueType>, OffsetsArrayType>)
    {
      TEST_ASSERT(test->GetOffsetsArray()->GetVoidPointer(0) == offsets->GetPointer(0));
    }
  }

  TEST_ASSERT(test->GetNumberOfCells() == 3);
  TEST_ASSERT(test->GetNumberOfConnectivityIds() == (FixedSize ? 9 : 14));
  TEST_ASSERT(test->GetNumberOfOffsets() == 4);
  {
    ValidateCellArray<FixedSize>(test);
  }
}

void TestSetData(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // These are documented to not deep copy the input arrays.
  TestSetDataImpl<vtkCellArray::ArrayType32, vtkCellArray::ArrayType32>(cellArray, true);
  TestSetDataImpl<vtkCellArray::ArrayType64, vtkCellArray::ArrayType64>(cellArray, true);
  TestSetDataImpl<vtkIdTypeArray, vtkIdTypeArray>(cellArray, true);
  TestSetDataImpl<vtkCellArray::AffineArrayType32, vtkCellArray::ArrayType32>(cellArray, true);
  TestSetDataImpl<vtkCellArray::AffineArrayType64, vtkCellArray::ArrayType64>(cellArray, true);
  TestSetDataImpl<vtkAffineArray<vtkIdType>, vtkIdTypeArray>(cellArray, true);

  // These should work, but may deep copy:
  TestSetDataImpl<vtkTypeInt32Array, vtkTypeInt32Array>(cellArray, false);
  TestSetDataImpl<vtkTypeInt64Array, vtkTypeInt64Array>(cellArray, false);
  TestSetDataImpl<vtkIntArray, vtkIntArray>(cellArray, false);
  TestSetDataImpl<vtkLongArray, vtkLongArray>(cellArray, false);
  TestSetDataImpl<vtkLongLongArray, vtkLongLongArray>(cellArray, false);
  TestSetDataImpl<vtkAffineArray<vtkTypeInt32>, vtkTypeInt32Array>(cellArray, false);
  TestSetDataImpl<vtkAffineArray<vtkTypeInt64>, vtkTypeInt64Array>(cellArray, false);
  TestSetDataImpl<vtkAffineArray<int>, vtkIntArray>(cellArray, false);
  TestSetDataImpl<vtkAffineArray<long>, vtkLongArray>(cellArray, false);
  TestSetDataImpl<vtkAffineArray<long long>, vtkLongLongArray>(cellArray, false);

  // These are documented to not deep copy the input arrays.
  TestSetDataImpl<MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeUInt8>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeUInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeUInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeUInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeUInt8>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeUInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeUInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeUInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeUInt8>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeUInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeUInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeUInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeUInt8>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeUInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeUInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeUInt64>>(cellArray, true);

  TestSetDataImpl<MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeUInt8>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeUInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeUInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeUInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeUInt8>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeUInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeUInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeUInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeUInt8>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeUInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeUInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeUInt64>>(cellArray, true);

  TestSetDataImpl<MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt16>, MockDataArray<vtkTypeInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt32>, MockDataArray<vtkTypeInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeInt64>, MockDataArray<vtkTypeInt64>>(cellArray, true);

  TestSetDataImpl<MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt8>, MockDataArray<vtkTypeInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt16>, MockDataArray<vtkTypeInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt32>, MockDataArray<vtkTypeInt64>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeInt16>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeInt32>>(cellArray, true);
  TestSetDataImpl<MockDataArray<vtkTypeUInt64>, MockDataArray<vtkTypeInt64>>(cellArray, true);
}

struct TestIsStorage64BitImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT*, ConnectivityT*, bool expect64Bit) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    constexpr bool is64Bit = sizeof(ValueType) == 8 &&
      std::is_base_of_v<vtkAOSDataArrayTemplate<ValueType>, OffsetsT> &&
      std::is_base_of_v<vtkAOSDataArrayTemplate<ValueType>, ConnectivityT>;

    TEST_ASSERT(is64Bit == expect64Bit);
  }
};

void TestIsStorage64Bit(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Dispatch(TestIsStorage64BitImpl{}, cellArray->IsStorage64Bit());
}

struct TestIsStorage32BitImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT*, ConnectivityT*, bool expect32Bit) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    constexpr bool is32Bit = sizeof(ValueType) == 4 &&
      std::is_base_of_v<vtkAOSDataArrayTemplate<ValueType>, OffsetsT> &&
      std::is_base_of_v<vtkAOSDataArrayTemplate<ValueType>, ConnectivityT>;

    TEST_ASSERT(is32Bit == expect32Bit);
  }
};

void TestIsStorage32Bit(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Dispatch(TestIsStorage32BitImpl{}, cellArray->IsStorage32Bit());
}

struct TestIsStorageFixedSize64BitImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT*, ConnectivityT*, bool expectFixedSize64Bit) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    constexpr bool isFixedSize64Bit = sizeof(ValueType) == 8 &&
      std::is_base_of_v<vtkAffineArray<ValueType>, OffsetsT> &&
      std::is_base_of_v<vtkAOSDataArrayTemplate<ValueType>, ConnectivityT>;

    TEST_ASSERT(isFixedSize64Bit == expectFixedSize64Bit);
  }
};

void TestIsStorageFixedSize32Bit(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Dispatch(TestIsStorageFixedSize64BitImpl{}, cellArray->IsStorageFixedSize64Bit());
}

struct TestIsStorageFixedSize32BitImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT*, ConnectivityT*, bool expectFixedSize32Bit) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    constexpr bool isFixedSize32Bit = sizeof(ValueType) == 4 &&
      std::is_base_of_v<vtkAffineArray<ValueType>, OffsetsT> &&
      std::is_base_of_v<vtkAOSDataArrayTemplate<ValueType>, ConnectivityT>;

    TEST_ASSERT(isFixedSize32Bit == expectFixedSize32Bit);
  }
};

void TestIsStorageFixedSize64Bit(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Dispatch(TestIsStorageFixedSize32BitImpl{}, cellArray->IsStorageFixedSize32Bit());
}

struct TestIsStorageGenericImpl : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT*, ConnectivityT*, bool expectGeneric) const
  {
    using ValueType = GetAPIType<OffsetsT>;

    constexpr bool isGeneric = !std::is_base_of_v<vtkAffineArray<ValueType>, OffsetsT> &&
      !std::is_base_of_v<vtkAOSDataArrayTemplate<ValueType>, OffsetsT> &&
      !std::is_base_of_v<vtkAOSDataArrayTemplate<ValueType>, ConnectivityT> &&
      std::is_base_of_v<vtkDataArray, OffsetsT> && std::is_base_of_v<vtkDataArray, ConnectivityT>;

    TEST_ASSERT(isGeneric == expectGeneric);
  }
};

void TestIsStorageGeneric(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Dispatch(TestIsStorageGenericImpl{}, cellArray->IsStorageGeneric());
}

void TestUse32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  FillCellArray<true>(cellArray);
  cellArray->Use32BitStorage();

  TEST_ASSERT(cellArray->IsStorage32Bit());
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestUse64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  FillCellArray<true>(cellArray);
  cellArray->Use64BitStorage();

  TEST_ASSERT(cellArray->IsStorage64Bit());
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestUseDefaultStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  FillCellArray<true>(cellArray);
  cellArray->UseDefaultStorage();

  TEST_ASSERT(cellArray->IsStorage64Bit() == (sizeof(vtkIdType) == 8));
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestUseFixedSize32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  FillCellArray<true>(cellArray);
  cellArray->UseFixedSize32BitStorage(3);

  TEST_ASSERT(cellArray->IsStorageFixedSize32Bit());
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestUseFixedSize64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  FillCellArray<true>(cellArray);
  cellArray->UseFixedSize64BitStorage(3);

  TEST_ASSERT(cellArray->IsStorageFixedSize64Bit());
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestUseFixedSizeDefaultStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  FillCellArray<true>(cellArray);
  cellArray->UseFixedSizeDefaultStorage(3);

  TEST_ASSERT(cellArray->IsStorageFixedSize64Bit() == (sizeof(vtkIdType) == 8));
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestCanConvertTo32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->CanConvertTo32BitStorage());

  if (cellArray->IsStorage64Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo32BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, 1, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(!cellArray->CanConvertTo32BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else if (cellArray->IsStorage32Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo32BitStorage());
  }
  else if (cellArray->IsStorageFixedSize64Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo32BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, 1, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(!cellArray->CanConvertTo32BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo32BitStorage());
  }
}

void TestCanConvertTo64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->CanConvertTo64BitStorage());

  if (cellArray->IsStorage64Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, 1, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT64_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else if (cellArray->IsStorage32Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());
  }
  else if (cellArray->IsStorageFixedSize64Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, 1, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT64_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else
  {
    cellArray->InsertNextCell({ 0, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());
  }
}

void TestCanConvertToFixedSize32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->CanConvertToFixedSize32BitStorage());

  if (cellArray->IsStorage64Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize32BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, 1, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(!cellArray->CanConvertToFixedSize32BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else if (cellArray->IsStorage32Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize32BitStorage());
  }
  else if (cellArray->IsStorageFixedSize64Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize32BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, 1, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(!cellArray->CanConvertToFixedSize32BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo32BitStorage());
  }
}

void TestCanConvertToFixedSize64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->CanConvertTo64BitStorage());

  if (cellArray->IsStorage64Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize64BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, 1, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(cellArray->CanConvertToFixedSize64BitStorage());
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT64_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize64BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else if (cellArray->IsStorage32Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize64BitStorage());
  }
  else if (cellArray->IsStorageFixedSize64Bit())
  {
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize64BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, 1, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(cellArray->CanConvertToFixedSize64BitStorage());
    cellArray->InsertNextCell({ 0, 1, VTK_TYPE_INT64_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize64BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else
  {
    cellArray->InsertNextCell({ 0, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertToFixedSize64BitStorage());
  }
}

template <bool FixedSize>
void TestConvertTo32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  TEST_ASSERT(cellArray->ConvertTo32BitStorage());
  TEST_ASSERT(cellArray->IsStorage32Bit());

  // Ensure that data is still good:
  ValidateCellArray<FixedSize>(cellArray);
}

template <bool FixedSize>
void TestConvertTo64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  TEST_ASSERT(cellArray->ConvertTo64BitStorage());
  TEST_ASSERT(cellArray->IsStorage64Bit());

  // Ensure that data is still good:
  ValidateCellArray<FixedSize>(cellArray);
}

void TestConvertToFixedSize32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<true>(cellArray);

  TEST_ASSERT(cellArray->ConvertToFixedSize32BitStorage());
  TEST_ASSERT(cellArray->IsStorageFixedSize32Bit());

  // Ensure that data is still good:
  ValidateCellArray<true>(cellArray);
}

void TestConvertToFixedSize64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<true>(cellArray);

  TEST_ASSERT(cellArray->ConvertToFixedSize64BitStorage());
  TEST_ASSERT(cellArray->IsStorageFixedSize64Bit());

  // Ensure that data is still good:
  ValidateCellArray<true>(cellArray);
}

void TestGetOffsetsArray(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  switch (cellArray->GetStorageType())
  {
    case vtkCellArray::Int64:
      TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsArray64());
      break;
    case vtkCellArray::Int32:
      TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsArray32());
      break;
    case vtkCellArray::FixedSizeInt64:
      TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsAffineArray64());
      break;
    case vtkCellArray::FixedSizeInt32:
      TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsAffineArray32());
      break;
    case vtkCellArray::Generic:
    default:
      TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsArray());
      break;
  }
}

void TestGetConnectivityArray(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  switch (cellArray->GetStorageType())
  {
    case vtkCellArray::Int64:
      TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray64());
      break;
    case vtkCellArray::Int32:
      TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray32());
      break;
    case vtkCellArray::FixedSizeInt64:
      TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray64());
      break;
    case vtkCellArray::FixedSizeInt32:
      TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray32());
      break;
    case vtkCellArray::Generic:
    default:
      TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray());
      break;
  }
}

template <bool FixedSize>
void TestIsHomogeneous(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->IsHomogeneous() == 0);
  cellArray->Initialize();
  TEST_ASSERT(cellArray->IsHomogeneous() == 0);

  FillCellArray<true>(cellArray);
  TEST_ASSERT(cellArray->IsHomogeneous() == 3);

  FillCellArray<true>(cellArray);
  TEST_ASSERT(cellArray->IsHomogeneous() == 3);

  if (!FixedSize)
  {
    cellArray->InsertNextCell({ 5, 6 });
    TEST_ASSERT(cellArray->IsHomogeneous() == -1);
  }

  cellArray->Initialize();
  TEST_ASSERT(cellArray->IsHomogeneous() == 0);
}

template <bool FixedSize>
void TestTraversalSizePointer(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  vtkIdType npts;
  const vtkIdType* pts;

  cellArray->InitTraversal();
  TEST_ASSERT(cellArray->GetTraversalCellId() == 0);
  TEST_ASSERT(cellArray->GetNextCell(npts, pts) != 0);
  TEST_ASSERT(ValidateCell<FixedSize>(0, npts, pts));
  TEST_ASSERT(cellArray->GetTraversalCellId() == 1);
  TEST_ASSERT(cellArray->GetNextCell(npts, pts) != 0);
  TEST_ASSERT(ValidateCell<FixedSize>(1, npts, pts));
  TEST_ASSERT(cellArray->GetTraversalCellId() == 2);
  TEST_ASSERT(cellArray->GetNextCell(npts, pts) != 0);
  TEST_ASSERT(ValidateCell<FixedSize>(2, npts, pts));

  cellArray->SetTraversalCellId(1);
  TEST_ASSERT(cellArray->GetTraversalCellId() == 1);
  TEST_ASSERT(cellArray->GetNextCell(npts, pts) != 0);
  TEST_ASSERT(ValidateCell<FixedSize>(1, npts, pts));
  TEST_ASSERT(cellArray->GetTraversalCellId() == 2);
  TEST_ASSERT(cellArray->GetNextCell(npts, pts) != 0);
  TEST_ASSERT(ValidateCell<FixedSize>(2, npts, pts));

  TEST_ASSERT(cellArray->GetNextCell(npts, pts) == 0);
}

template <bool FixedSize>
void TestTraversalIdList(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  vtkNew<vtkIdList> ids;
  cellArray->InitTraversal();
  TEST_ASSERT(cellArray->GetNextCell(ids) != 0);
  TEST_ASSERT(ValidateCell<FixedSize>(0, ids->GetNumberOfIds(), ids->GetPointer(0)));
  TEST_ASSERT(cellArray->GetNextCell(ids) != 0);
  TEST_ASSERT(ValidateCell<FixedSize>(1, ids->GetNumberOfIds(), ids->GetPointer(0)));
  TEST_ASSERT(cellArray->GetNextCell(ids) != 0);
  TEST_ASSERT(ValidateCell<FixedSize>(2, ids->GetNumberOfIds(), ids->GetPointer(0)));

  TEST_ASSERT(cellArray->GetNextCell(ids) == 0);
}

template <bool FixedSize>
void TestGetCellAtId(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  auto validate = [&](const vtkIdType cellId)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ValidateCell<FixedSize>(cellId, npts, pts));

    vtkNew<vtkIdList> ids;
    cellArray->GetCellAtId(cellId, ids);
    TEST_ASSERT(ValidateCell<FixedSize>(cellId, ids->GetNumberOfIds(), ids->GetPointer(0)));
  };

  validate(2);
  validate(0);
  validate(1);
}

template <bool FixedSize>
void TestGetCellSize(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  TEST_ASSERT(cellArray->GetCellSize(1) == GetCellIds<FixedSize>(1)->GetNumberOfIds());
  TEST_ASSERT(cellArray->GetCellSize(0) == GetCellIds<FixedSize>(0)->GetNumberOfIds());
  TEST_ASSERT(cellArray->GetCellSize(2) == GetCellIds<FixedSize>(2)->GetNumberOfIds());
}

template <bool FixedSize>
void TestInsertNextCell(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 0);

  auto ids0 = GetCellIds<FixedSize>(0);
  TEST_ASSERT(cellArray->InsertNextCell(ids0) == 0);
  TEST_ASSERT(cellArray->GetNumberOfCells() == 1);

  TEST_ASSERT(cellArray->InsertNextCell({ 3, 4, 5 }) == 1); // same as GetCellIds<FixedSize>(1)
  TEST_ASSERT(cellArray->GetNumberOfCells() == 2);

  auto ids2 = GetCellIds<FixedSize>(2);
  TEST_ASSERT(cellArray->InsertNextCell(ids2->GetNumberOfIds(), ids2->GetPointer(0)) == 2);
  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);

  vtkNew<vtkTriangle> triangle;
  auto ids3 = triangle->GetPointIds();
  ids3->SetId(0, 4);
  ids3->SetId(1, 2);
  ids3->SetId(2, 1);
  TEST_ASSERT(cellArray->InsertNextCell(triangle) == 3);
  TEST_ASSERT(cellArray->GetNumberOfCells() == 4);

  vtkIdType npts;
  const vtkIdType* pts;

  cellArray->GetCellAtId(0, npts, pts);
  TEST_ASSERT(ValidateCell<FixedSize>(0, npts, pts));
  cellArray->GetCellAtId(1, npts, pts);
  TEST_ASSERT(ValidateCell<FixedSize>(1, npts, pts));
  cellArray->GetCellAtId(2, npts, pts);
  TEST_ASSERT(ValidateCell<FixedSize>(2, npts, pts));
  cellArray->GetCellAtId(3, npts, pts);
  TEST_ASSERT(npts == 3);
  TEST_ASSERT(pts[0] == 4);
  TEST_ASSERT(pts[1] == 2);
  TEST_ASSERT(pts[2] == 1);
}

template <bool FixedSize>
void TestIncrementalCellInsertion(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  auto ids0 = GetCellIds<FixedSize>(0);
  TEST_ASSERT(cellArray->InsertNextCell(ids0->GetNumberOfIds()) == 0);
  for (vtkIdType i = 0; i < ids0->GetNumberOfIds(); ++i)
  {
    cellArray->InsertCellPoint(ids0->GetId(i));
  }
  auto ids1 = GetCellIds<FixedSize>(1);
  TEST_ASSERT(cellArray->InsertNextCell(ids1->GetNumberOfIds()) == 1);
  for (vtkIdType i = 0; i < ids1->GetNumberOfIds(); ++i)
  {
    cellArray->InsertCellPoint(ids1->GetId(i));
  }
  auto ids2 = GetCellIds<FixedSize>(2);
  TEST_ASSERT(cellArray->InsertNextCell(3) == 2); // 3 for Fixed Size, 6 for variable
  cellArray->InsertCellPoint(ids2->GetId(0));
  cellArray->UpdateCellCount(ids2->GetNumberOfIds()); // changing count to 6 for variable size
  for (vtkIdType i = 1; i < ids2->GetNumberOfIds(); ++i)
  {
    cellArray->InsertCellPoint(ids2->GetId(i));
  }

  ValidateCellArray<FixedSize>(cellArray);
}

template <bool FixedSize>
void TestReverseCellAtId(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  auto validate = [&](const vtkIdType cellId, bool reverse)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ValidateCell<FixedSize>(cellId, npts, pts, reverse));
  };

  validate(0, false);
  validate(1, false);
  validate(2, false);

  cellArray->ReverseCellAtId(2);

  validate(0, false);
  validate(1, false);
  validate(2, true);

  cellArray->ReverseCellAtId(0);

  validate(0, true);
  validate(1, false);
  validate(2, true);
}

template <bool FixedSize>
void TestReplaceCellAtId(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  auto validate = [&](const vtkIdType cellId, bool reverse)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ValidateCell<FixedSize>(cellId, npts, pts, reverse));
  };

  validate(0, false);
  validate(1, false);
  validate(2, false);

  {
    auto id2Reverse = GetCellIds<FixedSize>(2, true);
    cellArray->ReplaceCellAtId(2, id2Reverse);
  }

  validate(0, false);
  validate(1, false);
  validate(2, true);

  {
    auto id0 = GetCellIds<FixedSize>(0, true);
    cellArray->ReplaceCellAtId(0, id0->GetNumberOfIds(), id0->GetPointer(0));
  }

  validate(0, true);
  validate(1, false);
  validate(2, true);

  cellArray->ReplaceCellAtId(1, { 5, 4, 3 }); // reverse 3, 4, 5

  validate(0, true);
  validate(1, true);
  validate(2, true);
}

template <bool FixedSize>
void TestGetMaxCellSize(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  auto ids0 = GetCellIds<FixedSize>(0);
  auto ids1 = GetCellIds<FixedSize>(1);
  auto ids2 = GetCellIds<FixedSize>(2);

  TEST_ASSERT(cellArray->GetMaxCellSize() == 0);
  cellArray->InsertNextCell(ids0);
  TEST_ASSERT(cellArray->GetMaxCellSize() == ids0->GetNumberOfIds());
  cellArray->InsertNextCell(ids1);
  TEST_ASSERT(cellArray->GetMaxCellSize() == ids0->GetNumberOfIds());
  cellArray->InsertNextCell(ids2);
  TEST_ASSERT(cellArray->GetMaxCellSize() == ids2->GetNumberOfIds());
}

template <bool FixedSize>
void TestDeepCopy(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  vtkNew<vtkCellArray> other;
  other->DeepCopy(cellArray);

  TEST_ASSERT(cellArray->GetStorageType() == other->GetStorageType());
  ValidateCellArray<FixedSize>(other);

  cellArray->InsertNextCell({ 0, 1, 2 });
  TEST_ASSERT(cellArray->GetNumberOfCells() == other->GetNumberOfCells() + 1);
}

template <bool FixedSize>
void TestShallowCopy(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  vtkNew<vtkCellArray> other;
  other->ShallowCopy(cellArray);

  TEST_ASSERT(cellArray->GetStorageType() == other->GetStorageType());
  ValidateCellArray<FixedSize>(other);

  cellArray->InsertNextCell({ 0, 1, 2 });
  TEST_ASSERT(cellArray->GetNumberOfCells() == other->GetNumberOfCells());
  TEST_ASSERT(cellArray->GetOffsetsArray() == other->GetOffsetsArray());
  TEST_ASSERT(cellArray->GetConnectivityArray() == other->GetConnectivityArray());
}

template <bool FixedSize>
void TestAppendImpl(vtkSmartPointer<vtkCellArray> first, vtkSmartPointer<vtkCellArray> second)
{
  FillCellArray<FixedSize>(first, false);

  FillCellArray<FixedSize>(second, true);

  vtkNew<vtkCellArray> concat;
  concat->DeepCopy(first);
  concat->Append(second, 10); // add 10 to all point ids from second
  TEST_ASSERT(concat->GetNumberOfCells() == 6);

  auto validate = [&](const vtkIdType cellId, bool reverse, vtkIdType offset)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    concat->GetCellAtId(cellId, npts, pts);
    vtkIdType cellIdQuery = cellId < 3 ? cellId : cellId % 3;
    TEST_ASSERT(ValidateCell<FixedSize>(cellIdQuery, npts, pts, reverse, offset));
  };

  validate(0, false, 0);
  validate(1, false, 0);
  validate(2, false, 0);

  validate(3, true, 10);
  validate(4, true, 10);
  validate(5, true, 10);
}

template <bool FixedSize>
void TestAppend32(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);
  TestAppendImpl<FixedSize>(cellArray, CellArrayFactory<vtkCellArray::Int32>::New());
}

template <bool FixedSize>
void TestAppend64(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);
  TestAppendImpl<FixedSize>(cellArray, CellArrayFactory<vtkCellArray::Int64>::New());
}

template <bool FixedSize>
void TestLegacyFormatImportExportAppend(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray<FixedSize>(cellArray);

  vtkNew<vtkIdTypeArray> legacy;
  cellArray->ExportLegacyFormat(legacy);

  {
    std::vector<vtkIdType> expected;
    for (int i = 0; i < 3; ++i)
    {
      auto ids = GetCellIds<FixedSize>(i);
      expected.push_back(ids->GetNumberOfIds());
      for (vtkIdType j = 0; j < ids->GetNumberOfIds(); ++j)
      {
        expected.push_back(ids->GetId(j));
      }
    }
    auto legacyRange = vtk::DataArrayValueRange<1>(legacy);
    TEST_ASSERT(std::equal(expected.cbegin(), expected.cend(), legacyRange.cbegin()));
  }

  cellArray->Initialize();
  cellArray->ImportLegacyFormat(legacy);

  auto validate = [&](const vtkIdType cellId, vtkIdType offset = 0)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    vtkIdType cellIdQuery = cellId < 3 ? cellId : cellId % 3;
    TEST_ASSERT(ValidateCell<FixedSize>(cellIdQuery, npts, pts, false, offset));
  };

  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
  validate(0);
  validate(1);
  validate(2);

  // check that the next import doesn't have this
  cellArray->InsertNextCell(GetCellIds<FixedSize>(0, true));
  cellArray->ImportLegacyFormat(legacy->GetPointer(0), legacy->GetNumberOfTuples());

  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
  validate(0);
  validate(1);
  validate(2);

  cellArray->AppendLegacyFormat(legacy, 10);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 6);
  validate(0, 0);
  validate(1, 0);
  validate(2, 0);
  validate(3, 10);
  validate(4, 10);
  validate(5, 10);

  cellArray->AppendLegacyFormat(legacy->GetPointer(0), legacy->GetNumberOfTuples(), 20);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 9);
  validate(0, 0);
  validate(1, 0);
  validate(2, 0);
  validate(3, 10);
  validate(4, 10);
  validate(5, 10);
  validate(6, 20);
  validate(7, 20);
  validate(8, 20);
}

const char* StorageTypeToString(vtkCellArray::StorageTypes storageType)
{
  switch (storageType)
  {
    case vtkCellArray::Int64:
      return "Int64";
    case vtkCellArray::Int32:
      return "Int32";
    case vtkCellArray::FixedSizeInt64:
      return "FixedSizeInt64";
    case vtkCellArray::FixedSizeInt32:
      return "FixedSizeInt32";
    case vtkCellArray::Generic:
    default:
      return "Generic";
  }
}

template <vtkCellArray::StorageTypes StorageType, bool FixedSize, typename... ArrayTypes>
void RunTests()
{
  vtkLogScopeF(INFO, "Testing %s storage.", StorageTypeToString(StorageType));

  TestAllocate(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestResize(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestInitialize(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestReset(CellArrayFactory<StorageType, ArrayTypes...>::New());
  if (!FixedSize)
  {
    // Squeeze is a no-op for fixed size storage.
    TestSqueeze<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
    // Offsets can't be set manually for fixed size storage.
    TestIsValidOffsets<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  }
  TestIsValidConnectivity<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestGetNumberOfCells<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetNumberOfOffsets<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetNumberOfConnectivityIds<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestNewIterator<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestSetData(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestIsStorage64Bit(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestIsStorage32Bit(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestIsStorageFixedSize64Bit(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestIsStorageFixedSize32Bit(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestIsStorageGeneric(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestUse32BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestUse64BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestUseDefaultStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestUseFixedSize32BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestUseFixedSize64BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestUseFixedSizeDefaultStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestCanConvertTo32BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestCanConvertTo64BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestCanConvertToFixedSize32BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestCanConvertToFixedSize64BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestConvertTo32BitStorage<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestConvertTo64BitStorage<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestConvertToFixedSize32BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestConvertToFixedSize64BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestGetOffsetsArray(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetConnectivityArray(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestIsHomogeneous<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestTraversalSizePointer<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestTraversalIdList<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestGetCellAtId<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetCellSize<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestInsertNextCell<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestIncrementalCellInsertion<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestReverseCellAtId<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestReplaceCellAtId<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestGetMaxCellSize<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestDeepCopy<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestShallowCopy<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestAppend32<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestAppend64<FixedSize>(CellArrayFactory<StorageType, ArrayTypes...>::New());

  TestLegacyFormatImportExportAppend<FixedSize>(
    CellArrayFactory<StorageType, ArrayTypes...>::New());
}

} // end anon namespace
