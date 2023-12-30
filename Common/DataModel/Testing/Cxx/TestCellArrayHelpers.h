// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"

#include "MockDataArray.h"
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
#include "vtkQuad.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSmartPointer.h"

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

template <typename ConnectivityArrayT, typename OffsetsArrayT>
struct CellArrayFactory<vtkCellArray::StorageTypes::Generic, ConnectivityArrayT, OffsetsArrayT>
{
  static vtkSmartPointer<vtkCellArray> New()
  {
    auto cellArray = vtkSmartPointer<vtkCellArray>::New();
    // By passing array types which are NOT in vtkCellArray::InputArrayList,
    // vtkCellArray can be put into the "Generic" storage mode.
    vtkNew<ConnectivityArrayT> placeholderConn;
    vtkNew<OffsetsArrayT> placeholderOffsets;
    // initialize the offsets array with one element i.e, number of elements in the connectivity.
    placeholderOffsets->InsertNextValue(0);
    cellArray->SetData(placeholderOffsets, placeholderConn);
    TEST_ASSERT(cellArray->GetStorageType() == vtkCellArray::StorageTypes::Generic);
    return cellArray;
  }
};

void FillCellArray(vtkCellArray* cellArray)
{
  cellArray->InsertNextCell({ 0, 1, 2, 3, 4 });
  cellArray->InsertNextCell({ 3, 4, 5 });
  cellArray->InsertNextCell({ 7, 8, 9, 4, 2, 1 });
}

void ValidateCellArray(vtkCellArray* cellArray)
{
  vtkIdType npts;
  const vtkIdType* pts;
  auto it = vtk::TakeSmartPointer(cellArray->NewIterator());
  it->GoToFirstCell();

  TEST_ASSERT(!it->IsDoneWithTraversal());
  it->GetCurrentCell(npts, pts);
  TEST_ASSERT(npts == 5);
  TEST_ASSERT(pts[0] == 0);
  TEST_ASSERT(pts[1] == 1);
  TEST_ASSERT(pts[2] == 2);
  TEST_ASSERT(pts[3] == 3);
  TEST_ASSERT(pts[4] == 4);
  it->GoToNextCell();

  TEST_ASSERT(!it->IsDoneWithTraversal());
  it->GetCurrentCell(npts, pts);
  TEST_ASSERT(npts == 3);
  TEST_ASSERT(pts[0] == 3);
  TEST_ASSERT(pts[1] == 4);
  TEST_ASSERT(pts[2] == 5);
  it->GoToNextCell();

  TEST_ASSERT(!it->IsDoneWithTraversal());
  it->GetCurrentCell(npts, pts);
  TEST_ASSERT(npts == 6);
  TEST_ASSERT(pts[0] == 7);
  TEST_ASSERT(pts[1] == 8);
  TEST_ASSERT(pts[2] == 9);
  TEST_ASSERT(pts[3] == 4);
  TEST_ASSERT(pts[4] == 2);
  TEST_ASSERT(pts[5] == 1);
  it->GoToNextCell();

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

void TestSqueeze(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Initialize();
  cellArray->AllocateExact(128, 256);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 5, 6 });
  cellArray->InsertNextCell({ 9, 8, 5 });

  cellArray->Squeeze();

  TEST_ASSERT(cellArray->GetOffsetsArray()->GetNumberOfValues() == 4);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetNumberOfValues() == 8);
  TEST_ASSERT(cellArray->GetOffsetsArray()->GetSize() == 4);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetSize() == 8);

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
  validate(0, { 0, 1, 2 });
  validate(1, { 5, 6 });
  validate(2, { 9, 8, 5 });
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

void TestIsValid(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->IsValid());
  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 0, 4 });
  TEST_ASSERT(cellArray->IsValid());

  cellArray->GetOffsetsArray()->SetComponent(0, 0, 1);
  TEST_ASSERT(!cellArray->IsValid());
  cellArray->GetOffsetsArray()->SetComponent(0, 0, 0);
  TEST_ASSERT(cellArray->IsValid());

  cellArray->GetOffsetsArray()->Reset();
  TEST_ASSERT(!cellArray->IsValid());
  // This should reuse the old buffer:
  cellArray->GetOffsetsArray()->SetNumberOfValues(3);
  TEST_ASSERT(cellArray->IsValid());

  cellArray->GetOffsetsArray()->SetComponent(1, 0, 5);
  cellArray->GetOffsetsArray()->SetComponent(2, 0, 3);
  TEST_ASSERT(!cellArray->IsValid());
  cellArray->GetOffsetsArray()->SetComponent(1, 0, 3);
  cellArray->GetOffsetsArray()->SetComponent(2, 0, 5);
  TEST_ASSERT(cellArray->IsValid());

  cellArray->GetConnectivityArray()->SetNumberOfValues(6);
  TEST_ASSERT(!cellArray->IsValid());
  cellArray->GetConnectivityArray()->SetNumberOfValues(4);
  TEST_ASSERT(!cellArray->IsValid());
  cellArray->GetConnectivityArray()->SetNumberOfValues(5);
  TEST_ASSERT(cellArray->IsValid());
}

void TestGetNumberOfCells(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 0);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfCells() == 0);

  cellArray->InsertNextCell({ 0, 1, 2, 3, 4 });
  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5, 6 });

  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfCells() == 0);
}

void TestGetNumberOfOffsets(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);

  cellArray->InsertNextCell({ 0, 1, 2, 3, 4 });
  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5, 6 });

  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 4);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
}

void TestGetNumberOfConnectivityIds(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);

  cellArray->InsertNextCell({ 0, 1, 2, 3, 4 });
  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5, 6 });

  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 11);

  cellArray->Initialize();

  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

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

  cellArray->InsertNextCell({ 0, 1, 2, 3, 4 });
  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5, 6 });

  {
    vtkIdType npts;
    const vtkIdType* pts;

    auto iter = vtk::TakeSmartPointer(cellArray->NewIterator());
    TEST_ASSERT(!iter->IsDoneWithTraversal());
    iter->GoToFirstCell();

    TEST_ASSERT(!iter->IsDoneWithTraversal());
    iter->GetCurrentCell(npts, pts);
    TEST_ASSERT(npts == 5);
    TEST_ASSERT(pts[0] == 0);
    TEST_ASSERT(pts[1] == 1);
    TEST_ASSERT(pts[2] == 2);
    TEST_ASSERT(pts[3] == 3);
    TEST_ASSERT(pts[4] == 4);
    iter->GoToNextCell();

    TEST_ASSERT(!iter->IsDoneWithTraversal());
    iter->GetCurrentCell(npts, pts);
    TEST_ASSERT(npts == 3);
    TEST_ASSERT(pts[0] == 0);
    TEST_ASSERT(pts[1] == 1);
    TEST_ASSERT(pts[2] == 2);
    iter->GoToNextCell();

    TEST_ASSERT(!iter->IsDoneWithTraversal());
    iter->GetCurrentCell(npts, pts);
    TEST_ASSERT(npts == 3);
    TEST_ASSERT(pts[0] == 4);
    TEST_ASSERT(pts[1] == 5);
    TEST_ASSERT(pts[2] == 6);
    iter->GoToNextCell();

    TEST_ASSERT(iter->IsDoneWithTraversal());
  }

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

  vtkNew<vtkCellArray> test;
  test->DeepCopy(cellArray); // copy config settings

  vtkNew<OffsetsArrayType> offsets;
  vtkNew<ConnectivityArrayType> conn;
  offsets->InsertNextValue(0);
  offsets->InsertNextValue(5);
  offsets->InsertNextValue(8);
  conn->InsertNextValue(0);
  conn->InsertNextValue(1);
  conn->InsertNextValue(2);
  conn->InsertNextValue(3);
  conn->InsertNextValue(4);
  conn->InsertNextValue(10);
  conn->InsertNextValue(12);
  conn->InsertNextValue(13);
  test->SetData(offsets, conn);

  if (std::is_base_of<vtkAOSDataArrayTemplate<OffsetsValueType>, OffsetsArrayType>::value &&
    std::is_base_of<vtkAOSDataArrayTemplate<ConnectivityValueType>, ConnectivityArrayType>::value)
  {
    if (sizeof(OffsetsValueType) == 4 && sizeof(ConnectivityValueType) == 4)
    {
      TEST_ASSERT(!test->IsStorage64Bit());
      TEST_ASSERT(test->IsStorage32Bit());
      TEST_ASSERT(!test->IsStorageGeneric());
    }
    else
    {
      TEST_ASSERT(test->IsStorage64Bit());
      TEST_ASSERT(!test->IsStorage32Bit());
      TEST_ASSERT(!test->IsStorageGeneric());
    }
  }
  else
  {
    TEST_ASSERT(!test->IsStorage64Bit());
    TEST_ASSERT(!test->IsStorage32Bit());
    TEST_ASSERT(test->IsStorageGeneric());
  }

  if (checkNoCopy)
  {
    TEST_ASSERT(test->GetConnectivityArray()->GetVoidPointer(0) == conn->GetVoidPointer(0));
    TEST_ASSERT(test->GetOffsetsArray()->GetVoidPointer(0) == offsets->GetVoidPointer(0));
  }

  TEST_ASSERT(test->GetNumberOfCells() == 2);
  TEST_ASSERT(test->GetNumberOfConnectivityIds() == 8);
  TEST_ASSERT(test->GetNumberOfOffsets() == 3);
  {
    vtkIdType npts;
    const vtkIdType* pts;

    auto iter = vtk::TakeSmartPointer(test->NewIterator());
    TEST_ASSERT(!iter->IsDoneWithTraversal());
    iter->GoToFirstCell();

    TEST_ASSERT(!iter->IsDoneWithTraversal());
    iter->GetCurrentCell(npts, pts);
    TEST_ASSERT(npts == 5);
    TEST_ASSERT(pts[0] == 0);
    TEST_ASSERT(pts[1] == 1);
    TEST_ASSERT(pts[2] == 2);
    TEST_ASSERT(pts[3] == 3);
    TEST_ASSERT(pts[4] == 4);
    iter->GoToNextCell();

    TEST_ASSERT(!iter->IsDoneWithTraversal());
    iter->GetCurrentCell(npts, pts);
    TEST_ASSERT(npts == 3);
    TEST_ASSERT(pts[0] == 10);
    TEST_ASSERT(pts[1] == 12);
    TEST_ASSERT(pts[2] == 13);
    iter->GoToNextCell();

    TEST_ASSERT(iter->IsDoneWithTraversal());
  }
}

void TestSetData(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // These are documented to not deep copy the input arrays.
  TestSetDataImpl<vtkCellArray::ArrayType32, vtkCellArray::ArrayType32>(cellArray, true);
  TestSetDataImpl<vtkCellArray::ArrayType64, vtkCellArray::ArrayType64>(cellArray, true);
  TestSetDataImpl<vtkIdTypeArray, vtkIdTypeArray>(cellArray, true);

  // These should work, but may deep copy:
  TestSetDataImpl<vtkTypeInt32Array, vtkTypeInt32Array>(cellArray, false);
  TestSetDataImpl<vtkTypeInt64Array, vtkTypeInt64Array>(cellArray, false);
  TestSetDataImpl<vtkIntArray, vtkIntArray>(cellArray, false);
  TestSetDataImpl<vtkLongArray, vtkLongArray>(cellArray, false);
  TestSetDataImpl<vtkLongLongArray, vtkLongLongArray>(cellArray, false);

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

    const bool is64Bit = sizeof(ValueType) == 8;

    TEST_ASSERT(is64Bit == expect64Bit);
  }
};

void TestIsStorage64Bit(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Dispatch(TestIsStorage64BitImpl{}, cellArray->IsStorage64Bit());
}

void TestUse32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5, 6 });
  cellArray->Use32BitStorage();

  TEST_ASSERT(!cellArray->IsStorage64Bit());
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestUse64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5, 6 });
  cellArray->Use64BitStorage();

  TEST_ASSERT(cellArray->IsStorage64Bit());
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestUseDefaultStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Add some data to make sure that switching storage re-initializes data:
  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5, 6 });
  cellArray->UseDefaultStorage();

  TEST_ASSERT(cellArray->IsStorage64Bit() == (sizeof(vtkIdType) == 8));
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 1);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 0);
}

void TestCanConvertTo32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->CanConvertTo32BitStorage());

  if (cellArray->IsStorage64Bit())
  {
    cellArray->InsertNextCell({ 0, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo32BitStorage());

#ifdef VTK_USE_64BIT_IDS
    cellArray->InsertNextCell({ 0, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(!cellArray->CanConvertTo32BitStorage());
#endif // VTK_USE_64BIT_IDS
  }
  else
  {
    cellArray->InsertNextCell({ 0, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo32BitStorage());
  }
}

void TestCanConvertTo64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->CanConvertTo64BitStorage());

  if (cellArray->IsStorage64Bit())
  {
    cellArray->InsertNextCell({ 0, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());

#ifdef VTK_USE_64BIT_IDS

    cellArray->InsertNextCell({ 0, static_cast<vtkTypeInt64>(VTK_TYPE_INT32_MAX) + 1 });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());
    cellArray->InsertNextCell({ 0, VTK_TYPE_INT64_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());

#endif // VTK_USE_64BIT_IDS
  }
  else
  {
    cellArray->InsertNextCell({ 0, VTK_TYPE_INT32_MAX });
    TEST_ASSERT(cellArray->CanConvertTo64BitStorage());
  }
}

void TestConvertTo32BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray(cellArray);

  TEST_ASSERT(cellArray->ConvertTo32BitStorage());
  TEST_ASSERT(!cellArray->IsStorage64Bit());

  // Ensure that data is still good:
  ValidateCellArray(cellArray);
}

void TestConvertTo64BitStorage(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray(cellArray);

  TEST_ASSERT(cellArray->ConvertTo64BitStorage());
  TEST_ASSERT(cellArray->IsStorage64Bit());

  // Ensure that data is still good:
  ValidateCellArray(cellArray);
}

void TestGetOffsetsArray(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  switch (cellArray->GetStorageType())
  {
    case vtkCellArray::Generic:
      TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsArray());
      break;
    case vtkCellArray::Int32:
      TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsArray32());
      break;
    case vtkCellArray::Int64:
    default:
      TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsArray64());
      break;
  }
}

void TestGetConnectivityArray(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  switch (cellArray->GetStorageType())
  {
    case vtkCellArray::Generic:
      TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray());
      break;
    case vtkCellArray::Int32:
      TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray32());
      break;
    case vtkCellArray::Int64:
    default:
      TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray64());
      break;
  }
}

void TestIsHomogeneous(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->IsHomogeneous() == 0);
  cellArray->Initialize();
  TEST_ASSERT(cellArray->IsHomogeneous() == 0);

  cellArray->InsertNextCell({ 1, 2, 3, 4 });
  TEST_ASSERT(cellArray->IsHomogeneous() == 4);

  cellArray->InsertNextCell({ 5, 6, 7, 8 });
  TEST_ASSERT(cellArray->IsHomogeneous() == 4);

  cellArray->InsertNextCell({ 5, 6 });
  TEST_ASSERT(cellArray->IsHomogeneous() == -1);

  cellArray->Initialize();
  TEST_ASSERT(cellArray->IsHomogeneous() == 0);
}

void TestTraversalSizePointer(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5 });
  cellArray->InsertNextCell({ 9, 4, 5, 1 });

  auto validate = [&](const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    TEST_ASSERT(cellArray->GetNextCell(npts, pts) != 0);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  cellArray->InitTraversal();
  TEST_ASSERT(cellArray->GetTraversalCellId() == 0);
  validate({ 0, 1, 2 });
  TEST_ASSERT(cellArray->GetTraversalCellId() == 1);
  validate({ 4, 5 });
  TEST_ASSERT(cellArray->GetTraversalCellId() == 2);
  validate({ 9, 4, 5, 1 });

  cellArray->SetTraversalCellId(1);
  TEST_ASSERT(cellArray->GetTraversalCellId() == 1);
  validate({ 4, 5 });
  TEST_ASSERT(cellArray->GetTraversalCellId() == 2);
  validate({ 9, 4, 5, 1 });

  vtkIdType npts;
  const vtkIdType* pts;
  TEST_ASSERT(cellArray->GetNextCell(npts, pts) == 0);
}

void TestTraversalIdList(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5 });
  cellArray->InsertNextCell({ 9, 4, 5, 1 });

  auto validate = [&](const std::initializer_list<vtkIdType>& ref)
  {
    vtkNew<vtkIdList> ids;
    TEST_ASSERT(cellArray->GetNextCell(ids) != 0);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(ids->GetNumberOfIds()));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), ids->GetPointer(0)));
  };

  cellArray->InitTraversal();
  validate({ 0, 1, 2 });
  validate({ 4, 5 });
  validate({ 9, 4, 5, 1 });

  vtkNew<vtkIdList> ids;
  TEST_ASSERT(cellArray->GetNextCell(ids) == 0);
}

void TestGetCellAtId(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5 });
  cellArray->InsertNextCell({ 9, 4, 5, 1 });

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));

    vtkNew<vtkIdList> ids;
    cellArray->GetCellAtId(cellId, ids);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(ids->GetNumberOfIds()));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), ids->GetPointer(0)));
  };

  validate(2, { 9, 4, 5, 1 });
  validate(0, { 0, 1, 2 });
  validate(1, { 4, 5 });
}

void TestGetCellSize(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5 });
  cellArray->InsertNextCell({ 9, 4, 5, 1 });

  TEST_ASSERT(cellArray->GetCellSize(1) == 2);
  TEST_ASSERT(cellArray->GetCellSize(0) == 3);
  TEST_ASSERT(cellArray->GetCellSize(2) == 4);
}

void TestInsertNextCell(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 0);

  {
    vtkNew<vtkQuad> quad;
    auto ids = quad->GetPointIds();
    ids->SetId(0, 4);
    ids->SetId(1, 2);
    ids->SetId(2, 1);
    ids->SetId(3, 5);
    TEST_ASSERT(cellArray->InsertNextCell(quad) == 0);
    TEST_ASSERT(cellArray->GetNumberOfCells() == 1);
  }

  {
    vtkNew<vtkIdList> list;
    list->SetNumberOfIds(3);
    list->SetId(0, 2);
    list->SetId(1, 3);
    list->SetId(2, 1);
    TEST_ASSERT(cellArray->InsertNextCell(list) == 1);
    TEST_ASSERT(cellArray->GetNumberOfCells() == 2);

    list->SetNumberOfIds(2);
    list->SetId(0, 7);
    list->SetId(1, 8);
    TEST_ASSERT(cellArray->InsertNextCell(list->GetNumberOfIds(), list->GetPointer(0)) == 2);
    TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
  }

  TEST_ASSERT(cellArray->InsertNextCell({ 0, 1 }) == 3);
  TEST_ASSERT(cellArray->GetNumberOfCells() == 4);

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  validate(2, { 7, 8 });
  validate(0, { 4, 2, 1, 5 });
  validate(3, { 0, 1 });
  validate(1, { 2, 3, 1 });
}

void TestIncrementalCellInsertion(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->InsertNextCell(3) == 0);
  cellArray->InsertCellPoint(2);
  cellArray->InsertCellPoint(3);
  cellArray->InsertCellPoint(4);
  TEST_ASSERT(cellArray->InsertNextCell(2) == 1);
  cellArray->InsertCellPoint(4);
  cellArray->InsertCellPoint(1);
  TEST_ASSERT(cellArray->InsertNextCell(3) == 2);
  cellArray->InsertCellPoint(5);
  cellArray->UpdateCellCount(4); // changing count to 4
  cellArray->InsertCellPoint(7);
  cellArray->InsertCellPoint(6);
  cellArray->InsertCellPoint(1);
  TEST_ASSERT(cellArray->InsertNextCell(2) == 3);
  cellArray->InsertCellPoint(1);
  cellArray->InsertCellPoint(5);

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  validate(3, { 1, 5 });
  validate(1, { 4, 1 });
  validate(2, { 5, 7, 6, 1 });
  validate(0, { 2, 3, 4 });
}

void TestReverseCellAtId(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 6 });
  cellArray->InsertNextCell({ 7, 8, 9, 1 });
  cellArray->InsertNextCell({ 5, 3, 4 });

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  validate(0, { 0, 1, 2 });
  validate(1, { 4, 6 });
  validate(2, { 7, 8, 9, 1 });
  validate(3, { 5, 3, 4 });

  cellArray->ReverseCellAtId(2);

  validate(0, { 0, 1, 2 });
  validate(1, { 4, 6 });
  validate(2, { 1, 9, 8, 7 });
  validate(3, { 5, 3, 4 });

  cellArray->ReverseCellAtId(0);

  validate(0, { 2, 1, 0 });
  validate(1, { 4, 6 });
  validate(2, { 1, 9, 8, 7 });
  validate(3, { 5, 3, 4 });
}

void TestReplaceCellAtId(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 6 });
  cellArray->InsertNextCell({ 7, 8, 9, 1 });
  cellArray->InsertNextCell({ 5, 3, 4 });

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  validate(0, { 0, 1, 2 });
  validate(1, { 4, 6 });
  validate(2, { 7, 8, 9, 1 });
  validate(3, { 5, 3, 4 });

  {
    vtkNew<vtkIdList> list;
    list->SetNumberOfIds(4);
    list->SetId(0, 1);
    list->SetId(1, 2);
    list->SetId(2, 3);
    list->SetId(3, 4);
    cellArray->ReplaceCellAtId(2, list);
  }

  validate(0, { 0, 1, 2 });
  validate(1, { 4, 6 });
  validate(2, { 1, 2, 3, 4 });
  validate(3, { 5, 3, 4 });

  {
    vtkNew<vtkIdList> list;
    list->SetNumberOfIds(2);
    list->SetId(0, 9);
    list->SetId(1, 4);
    cellArray->ReplaceCellAtId(1, list->GetNumberOfIds(), list->GetPointer(0));
  }

  validate(0, { 0, 1, 2 });
  validate(1, { 9, 4 });
  validate(2, { 1, 2, 3, 4 });
  validate(3, { 5, 3, 4 });

  cellArray->ReplaceCellAtId(0, { 4, 5, 6 });

  validate(0, { 4, 5, 6 });
  validate(1, { 9, 4 });
  validate(2, { 1, 2, 3, 4 });
  validate(3, { 5, 3, 4 });
}

void TestGetMaxCellSize(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->GetMaxCellSize() == 0);
  cellArray->InsertNextCell({ 0, 1 });
  TEST_ASSERT(cellArray->GetMaxCellSize() == 2);
  cellArray->InsertNextCell({ 2, 1, 3 });
  TEST_ASSERT(cellArray->GetMaxCellSize() == 3);
  cellArray->InsertNextCell({ 2, 4 });
  TEST_ASSERT(cellArray->GetMaxCellSize() == 3);
  cellArray->InsertNextCell({ 2, 4, 3 });
  TEST_ASSERT(cellArray->GetMaxCellSize() == 3);
  cellArray->InsertNextCell({ 2, 4, 3, 5 });
  TEST_ASSERT(cellArray->GetMaxCellSize() == 4);
}

void TestDeepCopy(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray(cellArray);

  vtkNew<vtkCellArray> other;
  other->DeepCopy(cellArray);

  TEST_ASSERT(cellArray->IsStorage64Bit() == other->IsStorage64Bit());
  ValidateCellArray(other);

  cellArray->InsertNextCell({ 0, 1, 2 });
  TEST_ASSERT(cellArray->GetNumberOfCells() == other->GetNumberOfCells() + 1);
}

void TestShallowCopy(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  FillCellArray(cellArray);

  vtkNew<vtkCellArray> other;
  other->ShallowCopy(cellArray);

  TEST_ASSERT(cellArray->IsStorage64Bit() == other->IsStorage64Bit());
  ValidateCellArray(other);

  cellArray->InsertNextCell({ 0, 1, 2 });
  TEST_ASSERT(cellArray->GetNumberOfCells() == other->GetNumberOfCells());
  TEST_ASSERT(cellArray->GetOffsetsArray() == other->GetOffsetsArray());
  TEST_ASSERT(cellArray->GetConnectivityArray() == other->GetConnectivityArray());
}

void TestAppendImpl(vtkSmartPointer<vtkCellArray> first, vtkSmartPointer<vtkCellArray> second)
{
  first->InsertNextCell({ 0, 1, 2 });
  first->InsertNextCell({ 3, 5 });
  first->InsertNextCell({ 9, 7, 8 });

  second->InsertNextCell({ 3, 5, 6, 7 });
  second->InsertNextCell({ 3, 2, 4 });
  second->InsertNextCell({ 3, 2, 4, 6, 8 });

  vtkNew<vtkCellArray> concat;
  concat->DeepCopy(first);
  concat->Append(second, 10); // add 10 to all point ids from second
  TEST_ASSERT(concat->GetNumberOfCells() == 6);

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    concat->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  validate(0, { 0, 1, 2 });
  validate(1, { 3, 5 });
  validate(2, { 9, 7, 8 });

  validate(3, { 13, 15, 16, 17 });
  validate(4, { 13, 12, 14 });
  validate(5, { 13, 12, 14, 16, 18 });
}

void TestAppend32(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);
  TestAppendImpl(cellArray, CellArrayFactory<vtkCellArray::Int32>::New());
}

void TestAppend64(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);
  TestAppendImpl(cellArray, CellArrayFactory<vtkCellArray::Int64>::New());
}

void TestLegacyFormatImportExportAppend(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 2, 3 });
  cellArray->InsertNextCell({ 1, 4, 5, 7 });
  cellArray->InsertNextCell({ 2, 8, 9, 1, 4 });
  cellArray->InsertNextCell({ 3, 7 });

  vtkNew<vtkIdTypeArray> legacy;
  cellArray->ExportLegacyFormat(legacy);

  {
    std::vector<vtkIdType> expected{ 3, 0, 2, 3, 4, 1, 4, 5, 7, 5, 2, 8, 9, 1, 4, 2, 3, 7 };
    auto legacyRange = vtk::DataArrayValueRange<1>(legacy);
    TEST_ASSERT(std::equal(expected.cbegin(), expected.cend(), legacyRange.cbegin()));
  }

  cellArray->Initialize();
  cellArray->ImportLegacyFormat(legacy);

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  TEST_ASSERT(cellArray->GetNumberOfCells() == 4);
  validate(0, { 0, 2, 3 });
  validate(1, { 1, 4, 5, 7 });
  validate(2, { 2, 8, 9, 1, 4 });
  validate(3, { 3, 7 });

  // check that the next import doesn't have this
  cellArray->InsertNextCell({ 4, 5 });
  cellArray->ImportLegacyFormat(legacy->GetPointer(0), legacy->GetNumberOfTuples());

  TEST_ASSERT(cellArray->GetNumberOfCells() == 4);
  validate(0, { 0, 2, 3 });
  validate(1, { 1, 4, 5, 7 });
  validate(2, { 2, 8, 9, 1, 4 });
  validate(3, { 3, 7 });

  cellArray->AppendLegacyFormat(legacy, 10);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 8);
  validate(0, { 0, 2, 3 });
  validate(1, { 1, 4, 5, 7 });
  validate(2, { 2, 8, 9, 1, 4 });
  validate(3, { 3, 7 });
  validate(4, { 10, 12, 13 });
  validate(5, { 11, 14, 15, 17 });
  validate(6, { 12, 18, 19, 11, 14 });
  validate(7, { 13, 17 });

  cellArray->AppendLegacyFormat(legacy->GetPointer(0), legacy->GetNumberOfTuples(), 20);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 12);
  validate(0, { 0, 2, 3 });
  validate(1, { 1, 4, 5, 7 });
  validate(2, { 2, 8, 9, 1, 4 });
  validate(3, { 3, 7 });
  validate(4, { 10, 12, 13 });
  validate(5, { 11, 14, 15, 17 });
  validate(6, { 12, 18, 19, 11, 14 });
  validate(7, { 13, 17 });
  validate(8, { 20, 22, 23 });
  validate(9, { 21, 24, 25, 27 });
  validate(10, { 22, 28, 29, 21, 24 });
  validate(11, { 23, 27 });
}

//==============================================================================
void TestLegacyAllocate(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // Assumes triangles:
  const vtkIdType numTri = 1024;
  cellArray->Allocate(numTri * 4); // 4 legacy ids per triangle

  TEST_ASSERT(cellArray->GetOffsetsArray()->GetSize() == numTri * 4 + 1);
  TEST_ASSERT(cellArray->GetConnectivityArray()->GetSize() == numTri * 4);
}

void TestLegacyEstimateSize(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  TEST_ASSERT(cellArray->EstimateSize(10, 3) == 40);
}

void TestLegacyGetSize(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->AllocateExact(99, 100);
  TEST_ASSERT(cellArray->GetSize() == 200);

  // Test that cells can be retrieved correctly, even in special cases,
  // such as polyline containing single point.

  vtkNew<vtkPoints> points;
  vtkIdType pointId = points->InsertNextPoint(12.3, 45.6, 78.9);

  vtkNew<vtkIdList> lineIds;
  lineIds->InsertNextId(pointId);
  cellArray->InsertNextCell(lineIds);

  vtkNew<vtkPolyData> polyData;
  polyData->SetPoints(points);
  polyData->SetLines(cellArray);

  vtkIdType numberOfCells = polyData->GetNumberOfCells();
  TEST_ASSERT(numberOfCells == 1);

  vtkCell* cell = polyData->GetCell(0);
  TEST_ASSERT(cell != nullptr);
}

void TestLegacyGetNumberOfConnectivityEntries(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->ResizeExact(99, 100);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityEntries() == 199);
}

void TestLegacyGetCell(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 3, 4 });
  cellArray->InsertNextCell({ 5, 6, 7 });

  auto validate = [&](const vtkIdType loc, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCell(loc, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));

    vtkNew<vtkIdList> ids;
    cellArray->GetCell(loc, ids);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(ids->GetNumberOfIds()));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), ids->GetPointer(0)));
  };

  // Use the location API:
  validate(0, { 0, 1, 2 });
  validate(4, { 3, 4 });
  validate(7, { 5, 6, 7 });
}

void TestLegacyGetInsertLocation(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  TEST_ASSERT(cellArray->GetInsertLocation(3) == 0);
  cellArray->InsertNextCell({ 4, 5 });
  TEST_ASSERT(cellArray->GetInsertLocation(2) == 4);
  cellArray->InsertNextCell({ 6, 7, 8, 2 });
  TEST_ASSERT(cellArray->GetInsertLocation(4) == 7);
}

void TestLegacyGetSetTraversalLocation(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 5 });
  cellArray->InsertNextCell({ 6, 7, 8, 2 });

  vtkNew<vtkIdList> ids;
  cellArray->InitTraversal();
  TEST_ASSERT(cellArray->GetTraversalLocation() == 0);
  cellArray->GetNextCell(ids);
  TEST_ASSERT(cellArray->GetTraversalLocation() == 4);
  TEST_ASSERT(cellArray->GetTraversalLocation(3) == 0);
  cellArray->GetNextCell(ids);
  TEST_ASSERT(cellArray->GetTraversalLocation() == 7);
  TEST_ASSERT(cellArray->GetTraversalLocation(2) == 4);
  cellArray->GetNextCell(ids);
  TEST_ASSERT(cellArray->GetTraversalLocation() == 12);
  TEST_ASSERT(cellArray->GetTraversalLocation(4) == 7);

  cellArray->SetTraversalLocation(0);
  TEST_ASSERT(cellArray->GetTraversalCellId() == 0);
  cellArray->SetTraversalLocation(4);
  TEST_ASSERT(cellArray->GetTraversalCellId() == 1);
  cellArray->SetTraversalLocation(7);
  TEST_ASSERT(cellArray->GetTraversalCellId() == 2);
}

void TestLegacyReverseCell(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 6 });
  cellArray->InsertNextCell({ 7, 8, 9, 1 });
  cellArray->InsertNextCell({ 5, 3, 4 });

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  validate(0, { 0, 1, 2 });
  validate(1, { 4, 6 });
  validate(2, { 7, 8, 9, 1 });
  validate(3, { 5, 3, 4 });

  cellArray->ReverseCell(7);

  validate(0, { 0, 1, 2 });
  validate(1, { 4, 6 });
  validate(2, { 1, 9, 8, 7 });
  validate(3, { 5, 3, 4 });

  cellArray->ReverseCell(0);

  validate(0, { 2, 1, 0 });
  validate(1, { 4, 6 });
  validate(2, { 1, 9, 8, 7 });
  validate(3, { 5, 3, 4 });
}

void TestLegacyReplaceCell(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 1, 2 });
  cellArray->InsertNextCell({ 4, 6 });
  cellArray->InsertNextCell({ 7, 8, 9, 1 });
  cellArray->InsertNextCell({ 5, 3, 4 });

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  validate(0, { 0, 1, 2 });
  validate(1, { 4, 6 });
  validate(2, { 7, 8, 9, 1 });
  validate(3, { 5, 3, 4 });

  {
    vtkNew<vtkIdList> list;
    list->SetNumberOfIds(4);
    list->SetId(0, 1);
    list->SetId(1, 2);
    list->SetId(2, 3);
    list->SetId(3, 4);
    cellArray->ReplaceCell(7, list->GetNumberOfIds(), list->GetPointer(0));
  }

  validate(0, { 0, 1, 2 });
  validate(1, { 4, 6 });
  validate(2, { 1, 2, 3, 4 });
  validate(3, { 5, 3, 4 });

  {
    vtkNew<vtkIdList> list;
    list->SetNumberOfIds(2);
    list->SetId(0, 9);
    list->SetId(1, 4);
    cellArray->ReplaceCell(4, list->GetNumberOfIds(), list->GetPointer(0));
  }

  validate(0, { 0, 1, 2 });
  validate(1, { 9, 4 });
  validate(2, { 1, 2, 3, 4 });
  validate(3, { 5, 3, 4 });

  {
    vtkNew<vtkIdList> list;
    list->SetNumberOfIds(3);
    list->SetId(0, 4);
    list->SetId(1, 5);
    list->SetId(2, 6);
    cellArray->ReplaceCell(0, list->GetNumberOfIds(), list->GetPointer(0));
  }

  validate(0, { 4, 5, 6 });
  validate(1, { 9, 4 });
  validate(2, { 1, 2, 3, 4 });
  validate(3, { 5, 3, 4 });
}

void TestLegacyGetData(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->InsertNextCell({ 0, 2, 3 });
  cellArray->InsertNextCell({ 1, 4, 5, 7 });
  cellArray->InsertNextCell({ 2, 8, 9, 1, 4 });
  cellArray->InsertNextCell({ 3, 7 });

  vtkIdTypeArray* legacy = cellArray->GetData();

  {
    std::vector<vtkIdType> expected{ 3, 0, 2, 3, 4, 1, 4, 5, 7, 5, 2, 8, 9, 1, 4, 2, 3, 7 };
    auto legacyRange = vtk::DataArrayValueRange<1>(legacy);
    TEST_ASSERT(std::equal(expected.cbegin(), expected.cend(), legacyRange.cbegin()));
  }
}

void TestLegacySetCells(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  vtkNew<vtkIdTypeArray> legacy;
  legacy->InsertNextValue(3);
  legacy->InsertNextValue(0);
  legacy->InsertNextValue(1);
  legacy->InsertNextValue(2);

  legacy->InsertNextValue(2);
  legacy->InsertNextValue(3);
  legacy->InsertNextValue(5);

  legacy->InsertNextValue(4);
  legacy->InsertNextValue(9);
  legacy->InsertNextValue(6);
  legacy->InsertNextValue(5);
  legacy->InsertNextValue(2);

  cellArray->SetCells(3, legacy);

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    cellArray->GetCellAtId(cellId, npts, pts);
    TEST_ASSERT(ref.size() == static_cast<std::size_t>(npts));
    TEST_ASSERT(std::equal(ref.begin(), ref.end(), pts));
  };

  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
  validate(0, { 0, 1, 2 });
  validate(1, { 3, 5 });
  validate(2, { 9, 6, 5, 2 });
}

template <vtkCellArray::StorageTypes StorageType, typename... ArrayTypes>
void RunLegacyTests()
{
  vtkLogScopeFunction(INFO);

  TestLegacyAllocate(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyEstimateSize(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyGetSize(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyGetNumberOfConnectivityEntries(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyGetCell(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyGetInsertLocation(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyGetSetTraversalLocation(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyReverseCell(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyReplaceCell(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyGetData(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacySetCells(CellArrayFactory<StorageType, ArrayTypes...>::New());
}

const char* StorageTypeToString(vtkCellArray::StorageTypes storageType)
{
  switch (storageType)
  {
    case vtkCellArray::Int64:
      return "Int64";
    case vtkCellArray::Int32:
      return "Int32";
    case vtkCellArray::Generic:
    default:
      return "Generic";
  }
}

template <vtkCellArray::StorageTypes StorageType, typename... ArrayTypes>
void RunTests()
{
  vtkLogScopeF(INFO, "Testing %s storage.", StorageTypeToString(StorageType));

  TestAllocate(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestResize(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestInitialize(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestSqueeze(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestReset(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestIsValid(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetNumberOfCells(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetNumberOfOffsets(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetNumberOfConnectivityIds(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestNewIterator(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestSetData(CellArrayFactory<StorageType, ArrayTypes...>::New());
  if (StorageType != vtkCellArray::Generic)
  {
    // When storage type is generic, offsets and connectivity can have mixed bit width.
    TestIsStorage64Bit(CellArrayFactory<StorageType, ArrayTypes...>::New());
  }
  TestUse32BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestUse64BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestUseDefaultStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestCanConvertTo32BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestCanConvertTo64BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestConvertTo32BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestConvertTo64BitStorage(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetOffsetsArray(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetConnectivityArray(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestIsHomogeneous(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestTraversalSizePointer(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestTraversalIdList(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetCellAtId(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetCellSize(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestInsertNextCell(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestIncrementalCellInsertion(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestReverseCellAtId(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestReplaceCellAtId(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestGetMaxCellSize(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestDeepCopy(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestShallowCopy(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestAppend32(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestAppend64(CellArrayFactory<StorageType, ArrayTypes...>::New());
  TestLegacyFormatImportExportAppend(CellArrayFactory<StorageType, ArrayTypes...>::New());

  RunLegacyTests<StorageType, ArrayTypes...>();
}

} // end anon namespace
