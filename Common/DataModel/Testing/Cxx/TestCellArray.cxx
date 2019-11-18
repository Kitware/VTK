/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellArray.h"

#include "vtkCellArrayIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkNew.h"
#include "vtkQuad.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// The legacy methods in vtkCellArray have been soft-deprecated with a comment
// in the documentation for now. Once they are hard-deprecated and actually
// removed from non-legacy builds, re-enable this check:
//#ifndef VTK_LEGACY_REMOVE
#define TEST_LEGACY_METHODS
//#endif

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
    if (!(cond))                                                                                   \
    {                                                                                              \
      ThrowAssertError(vtkQuoteMacro(__FILE__) ":" vtkQuoteMacro(                                  \
        __LINE__) ": test assertion failed: (" #cond ")");                                         \
    }                                                                                              \
  } while (false)

vtkSmartPointer<vtkCellArray> NewCellArray(bool use64BitStorage)
{
  auto cellArray = vtkSmartPointer<vtkCellArray>::New();
  if (use64BitStorage)
  {
    cellArray->Use64BitStorage();
  }
  else
  {
    cellArray->Use32BitStorage();
  }

  TEST_ASSERT(cellArray->IsStorage64Bit() == use64BitStorage);

  return cellArray;
}

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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

template <typename ArrayType>
void TestSetDataImpl(vtkSmartPointer<vtkCellArray> cellArray, bool checkNoCopy)
{
  using ValueType = typename ArrayType::ValueType;

  vtkNew<vtkCellArray> test;
  test->DeepCopy(cellArray); // copy config settings

  vtkNew<ArrayType> offsets;
  vtkNew<ArrayType> conn;
  test->SetData(offsets, conn);

  static_assert(
    sizeof(ValueType) == 4 || sizeof(ValueType) == 8, "Invalid type for cell array storage.");

  if (sizeof(ValueType) == 4)
  {
    TEST_ASSERT(!test->IsStorage64Bit());
  }
  else
  {
    TEST_ASSERT(test->IsStorage64Bit());
  }

  if (checkNoCopy)
  {
    TEST_ASSERT(test->GetConnectivityArray()->GetVoidPointer(0) == conn->GetVoidPointer(0));
    TEST_ASSERT(test->GetOffsetsArray()->GetVoidPointer(0) == offsets->GetVoidPointer(0));
  }
}

void TestSetData(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  // These are documented to not deep copy the input arrays.
  TestSetDataImpl<vtkCellArray::ArrayType32>(cellArray, true);
  TestSetDataImpl<vtkCellArray::ArrayType64>(cellArray, true);
  TestSetDataImpl<vtkIdTypeArray>(cellArray, true);

  // These should work, but may deep copy:
  TestSetDataImpl<vtkTypeInt32Array>(cellArray, false);
  TestSetDataImpl<vtkTypeInt64Array>(cellArray, false);
  TestSetDataImpl<vtkIntArray>(cellArray, false);
  TestSetDataImpl<vtkLongArray>(cellArray, false);
  TestSetDataImpl<vtkLongLongArray>(cellArray, false);
}

struct TestIsStorage64BitImpl
{
  template <typename CellStateT>
  void operator()(CellStateT&, bool expect64Bit) const
  {
    // Check the actual arrays, not the typedefs:
    using OffsetsArrayType =
      typename std::decay<decltype(*std::declval<CellStateT>().GetOffsets())>::type;

    using ConnArrayType =
      typename std::decay<decltype(*std::declval<CellStateT>().GetConnectivity())>::type;

    using OffsetsValueType = typename OffsetsArrayType::ValueType;
    using ConnValueType = typename ConnArrayType::ValueType;

    const bool connIs64Bit = sizeof(ConnValueType) == 8;
    const bool offsetsIs64Bit = sizeof(OffsetsValueType) == 8;

    TEST_ASSERT(connIs64Bit == expect64Bit);
    TEST_ASSERT(offsetsIs64Bit == expect64Bit);
  }
};

void TestIsStorage64Bit(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  cellArray->Visit(TestIsStorage64BitImpl{}, cellArray->IsStorage64Bit());
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

  if (cellArray->IsStorage64Bit())
  {
    TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsArray64());
  }
  else
  {
    TEST_ASSERT(cellArray->GetOffsetsArray() == cellArray->GetOffsetsArray32());
  }
}

void TestGetConnectivityArray(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);

  if (cellArray->IsStorage64Bit())
  {
    TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray64());
  }
  else
  {
    TEST_ASSERT(cellArray->GetConnectivityArray() == cellArray->GetConnectivityArray32());
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

  auto validate = [&](const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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
  TestAppendImpl(cellArray, NewCellArray(false));
}

void TestAppend64(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);
  TestAppendImpl(cellArray, NewCellArray(true));
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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
#ifdef TEST_LEGACY_METHODS

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

  auto validate = [&](const vtkIdType loc, const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

  auto validate = [&](const vtkIdType cellId, const std::initializer_list<vtkIdType>& ref) {
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

void RunLegacyTests(bool use64BitStorage)
{
  vtkLogScopeFunction(INFO);

  TestLegacyAllocate(NewCellArray(use64BitStorage));
  TestLegacyEstimateSize(NewCellArray(use64BitStorage));
  TestLegacyGetSize(NewCellArray(use64BitStorage));
  TestLegacyGetNumberOfConnectivityEntries(NewCellArray(use64BitStorage));
  TestLegacyGetCell(NewCellArray(use64BitStorage));
  TestLegacyGetInsertLocation(NewCellArray(use64BitStorage));
  TestLegacyGetSetTraversalLocation(NewCellArray(use64BitStorage));
  TestLegacyReverseCell(NewCellArray(use64BitStorage));
  TestLegacyReplaceCell(NewCellArray(use64BitStorage));
  TestLegacyGetData(NewCellArray(use64BitStorage));
  TestLegacySetCells(NewCellArray(use64BitStorage));
}

//==============================================================================
#else  // TEST_LEGACY_METHODS

void RunLegacyTests(bool)
{

  // no-op
}

//==============================================================================
#endif // TEST_LEGACY_METHODS

void RunTests(bool use64BitStorage)
{
  vtkLogScopeF(INFO, "Testing %d-bit storage.", use64BitStorage ? 64 : 32);

  TestAllocate(NewCellArray(use64BitStorage));
  TestResize(NewCellArray(use64BitStorage));
  TestInitialize(NewCellArray(use64BitStorage));
  TestSqueeze(NewCellArray(use64BitStorage));
  TestReset(NewCellArray(use64BitStorage));
  TestIsValid(NewCellArray(use64BitStorage));
  TestGetNumberOfCells(NewCellArray(use64BitStorage));
  TestGetNumberOfOffsets(NewCellArray(use64BitStorage));
  TestGetNumberOfConnectivityIds(NewCellArray(use64BitStorage));
  TestNewIterator(NewCellArray(use64BitStorage));
  TestSetData(NewCellArray(use64BitStorage));
  TestIsStorage64Bit(NewCellArray(use64BitStorage));
  TestUse32BitStorage(NewCellArray(use64BitStorage));
  TestUse64BitStorage(NewCellArray(use64BitStorage));
  TestUseDefaultStorage(NewCellArray(use64BitStorage));
  TestCanConvertTo32BitStorage(NewCellArray(use64BitStorage));
  TestCanConvertTo64BitStorage(NewCellArray(use64BitStorage));
  TestConvertTo32BitStorage(NewCellArray(use64BitStorage));
  TestConvertTo64BitStorage(NewCellArray(use64BitStorage));
  TestGetOffsetsArray(NewCellArray(use64BitStorage));
  TestGetConnectivityArray(NewCellArray(use64BitStorage));
  TestIsHomogeneous(NewCellArray(use64BitStorage));
  TestTraversalSizePointer(NewCellArray(use64BitStorage));
  TestTraversalIdList(NewCellArray(use64BitStorage));
  TestGetCellAtId(NewCellArray(use64BitStorage));
  TestGetCellSize(NewCellArray(use64BitStorage));
  TestInsertNextCell(NewCellArray(use64BitStorage));
  TestIncrementalCellInsertion(NewCellArray(use64BitStorage));
  TestReverseCellAtId(NewCellArray(use64BitStorage));
  TestReplaceCellAtId(NewCellArray(use64BitStorage));
  TestGetMaxCellSize(NewCellArray(use64BitStorage));
  TestDeepCopy(NewCellArray(use64BitStorage));
  TestShallowCopy(NewCellArray(use64BitStorage));
  TestAppend32(NewCellArray(use64BitStorage));
  TestAppend64(NewCellArray(use64BitStorage));
  TestLegacyFormatImportExportAppend(NewCellArray(use64BitStorage));

  RunLegacyTests(use64BitStorage);
}

void RunTests()
{
  RunTests(false);
  RunTests(true);
}

} // end anon namespace

int TestCellArray(int, char*[])
{
  try
  {
    RunTests();
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
