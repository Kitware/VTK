// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Regression test for the fixed-size storage silent-corruption hazard:
// inserting a cell whose size differs from the fixed cell size (the affine
// offsets slope) used to silently desynchronize the offsets and connectivity
// arrays, because vtkAffineArray ignores inserted values. The mutating entry
// points now auto-convert to explicit storage of the same bit width
// (see vtkCellArray::EnsureStorageForCellSize()).
//
// See https://gitlab.kitware.com/vtk/vtk/-/issues/20127

#include "vtkCellArray.h"

#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTriangle.h"

#include <stdexcept>
#include <string>

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

vtkSmartPointer<vtkCellArray> NewFixedSizeCellArray(bool use64Bit, vtkIdType cellSize)
{
  auto cellArray = vtkSmartPointer<vtkCellArray>::New();
  if (use64Bit)
  {
    cellArray->UseFixedSize64BitStorage(cellSize);
    TEST_ASSERT(cellArray->GetStorageType() == vtkCellArray::StorageTypes::FixedSizeInt64);
  }
  else
  {
    cellArray->UseFixedSize32BitStorage(cellSize);
    TEST_ASSERT(cellArray->GetStorageType() == vtkCellArray::StorageTypes::FixedSizeInt32);
  }
  return cellArray;
}

vtkCellArray::StorageTypes ExpectedExplicitStorage(bool use64Bit)
{
  return use64Bit ? vtkCellArray::StorageTypes::Int64 : vtkCellArray::StorageTypes::Int32;
}

void ValidateCell(vtkCellArray* cellArray, vtkIdType cellId, std::initializer_list<vtkIdType> ref)
{
  vtkNew<vtkIdList> ids;
  cellArray->GetCellAtId(cellId, ids);
  TEST_ASSERT(ids->GetNumberOfIds() == static_cast<vtkIdType>(ref.size()));
  vtkIdType i = 0;
  for (const vtkIdType refId : ref)
  {
    TEST_ASSERT(ids->GetId(i++) == refId);
  }
}

void TestInsertNextCellAutoConverts(bool use64Bit)
{
  vtkLogScopeFunction(INFO);

  auto cellArray = NewFixedSizeCellArray(use64Bit, 3);
  TEST_ASSERT(cellArray->InsertNextCell({ 0, 1, 2 }) == 0);
  TEST_ASSERT(cellArray->InsertNextCell({ 3, 4, 5 }) == 1);

  // Matching cell size must not change the storage type:
  TEST_ASSERT(cellArray->IsStorageFixedSize());

  // Mismatched cell size must auto-convert to explicit storage of the same
  // bit width and keep the topology consistent:
  TEST_ASSERT(cellArray->InsertNextCell({ 6, 7, 8, 9 }) == 2);
  TEST_ASSERT(cellArray->GetStorageType() == ExpectedExplicitStorage(use64Bit));
  TEST_ASSERT(cellArray->IsValid());
  TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 10);
  ValidateCell(cellArray, 0, { 0, 1, 2 });
  ValidateCell(cellArray, 1, { 3, 4, 5 });
  ValidateCell(cellArray, 2, { 6, 7, 8, 9 });
}

void TestInsertNextCellIdListAndCellAutoConvert(bool use64Bit)
{
  vtkLogScopeFunction(INFO);

  { // vtkIdList overload:
    auto cellArray = NewFixedSizeCellArray(use64Bit, 2);
    TEST_ASSERT(cellArray->InsertNextCell({ 0, 1 }) == 0);
    vtkNew<vtkIdList> ids;
    ids->InsertNextId(2);
    ids->InsertNextId(3);
    ids->InsertNextId(4);
    TEST_ASSERT(cellArray->InsertNextCell(ids) == 1);
    TEST_ASSERT(cellArray->GetStorageType() == ExpectedExplicitStorage(use64Bit));
    TEST_ASSERT(cellArray->IsValid());
    ValidateCell(cellArray, 0, { 0, 1 });
    ValidateCell(cellArray, 1, { 2, 3, 4 });
  }

  { // vtkCell overload:
    auto cellArray = NewFixedSizeCellArray(use64Bit, 2);
    TEST_ASSERT(cellArray->InsertNextCell({ 0, 1 }) == 0);
    vtkNew<vtkTriangle> triangle;
    triangle->GetPointIds()->SetId(0, 5);
    triangle->GetPointIds()->SetId(1, 6);
    triangle->GetPointIds()->SetId(2, 7);
    TEST_ASSERT(cellArray->InsertNextCell(triangle) == 1);
    TEST_ASSERT(cellArray->GetStorageType() == ExpectedExplicitStorage(use64Bit));
    TEST_ASSERT(cellArray->IsValid());
    ValidateCell(cellArray, 0, { 0, 1 });
    ValidateCell(cellArray, 1, { 5, 6, 7 });
  }
}

void TestIncrementalInsertionAutoConverts(bool use64Bit)
{
  vtkLogScopeFunction(INFO);

  { // Mismatched size known up front:
    auto cellArray = NewFixedSizeCellArray(use64Bit, 3);
    TEST_ASSERT(cellArray->InsertNextCell({ 0, 1, 2 }) == 0);
    TEST_ASSERT(cellArray->InsertNextCell(4) == 1);
    TEST_ASSERT(cellArray->GetStorageType() == ExpectedExplicitStorage(use64Bit));
    cellArray->InsertCellPoint(3);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(5);
    cellArray->InsertCellPoint(6);
    TEST_ASSERT(cellArray->IsValid());
    ValidateCell(cellArray, 0, { 0, 1, 2 });
    ValidateCell(cellArray, 1, { 3, 4, 5, 6 });
  }

  { // Mismatched size only known at UpdateCellCount() time:
    auto cellArray = NewFixedSizeCellArray(use64Bit, 3);
    TEST_ASSERT(cellArray->InsertNextCell({ 0, 1, 2 }) == 0);
    TEST_ASSERT(cellArray->InsertNextCell(3) == 1);
    TEST_ASSERT(cellArray->IsStorageFixedSize());
    cellArray->InsertCellPoint(3);
    cellArray->InsertCellPoint(4);
    cellArray->InsertCellPoint(5);
    cellArray->InsertCellPoint(6);
    cellArray->UpdateCellCount(4);
    TEST_ASSERT(cellArray->GetStorageType() == ExpectedExplicitStorage(use64Bit));
    TEST_ASSERT(cellArray->IsValid());
    ValidateCell(cellArray, 0, { 0, 1, 2 });
    ValidateCell(cellArray, 1, { 3, 4, 5, 6 });
  }
}

void TestAppendAutoConverts(bool use64Bit)
{
  vtkLogScopeFunction(INFO);

  { // Homogeneous source with matching cell size keeps fixed size storage:
    auto cellArray = NewFixedSizeCellArray(use64Bit, 3);
    cellArray->InsertNextCell({ 0, 1, 2 });
    vtkNew<vtkCellArray> src;
    src->InsertNextCell({ 0, 1, 2 });
    src->InsertNextCell({ 1, 2, 3 });
    cellArray->Append(src, 10);
    TEST_ASSERT(cellArray->IsStorageFixedSize());
    TEST_ASSERT(cellArray->IsValid());
    TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
    ValidateCell(cellArray, 0, { 0, 1, 2 });
    ValidateCell(cellArray, 1, { 10, 11, 12 });
    ValidateCell(cellArray, 2, { 11, 12, 13 });
  }

  { // Heterogeneous source auto-converts:
    auto cellArray = NewFixedSizeCellArray(use64Bit, 3);
    cellArray->InsertNextCell({ 0, 1, 2 });
    vtkNew<vtkCellArray> src;
    src->InsertNextCell({ 0, 1, 2, 3 });
    src->InsertNextCell({ 4, 5 });
    cellArray->Append(src, 10);
    TEST_ASSERT(cellArray->GetStorageType() == ExpectedExplicitStorage(use64Bit));
    TEST_ASSERT(cellArray->IsValid());
    TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
    ValidateCell(cellArray, 0, { 0, 1, 2 });
    ValidateCell(cellArray, 1, { 10, 11, 12, 13 });
    ValidateCell(cellArray, 2, { 14, 15 });
  }
}

void TestAppendLegacyFormatAutoConverts(bool use64Bit)
{
  vtkLogScopeFunction(INFO);

  { // Matching cell sizes keep fixed size storage:
    auto cellArray = NewFixedSizeCellArray(use64Bit, 3);
    cellArray->InsertNextCell({ 0, 1, 2 });
    const vtkIdType legacy[] = { 3, 3, 4, 5 };
    cellArray->AppendLegacyFormat(legacy, 4, 0);
    TEST_ASSERT(cellArray->IsStorageFixedSize());
    TEST_ASSERT(cellArray->IsValid());
    ValidateCell(cellArray, 0, { 0, 1, 2 });
    ValidateCell(cellArray, 1, { 3, 4, 5 });
  }

  { // Mismatched cell size auto-converts:
    auto cellArray = NewFixedSizeCellArray(use64Bit, 3);
    cellArray->InsertNextCell({ 0, 1, 2 });
    const vtkIdType legacy[] = { 3, 3, 4, 5, 4, 6, 7, 8, 9 };
    cellArray->AppendLegacyFormat(legacy, 9, 0);
    TEST_ASSERT(cellArray->GetStorageType() == ExpectedExplicitStorage(use64Bit));
    TEST_ASSERT(cellArray->IsValid());
    TEST_ASSERT(cellArray->GetNumberOfCells() == 3);
    ValidateCell(cellArray, 0, { 0, 1, 2 });
    ValidateCell(cellArray, 1, { 3, 4, 5 });
    ValidateCell(cellArray, 2, { 6, 7, 8, 9 });
  }
}

void TestEmptyFixedSizeAutoConverts(bool use64Bit)
{
  vtkLogScopeFunction(INFO);

  auto cellArray = NewFixedSizeCellArray(use64Bit, 3);
  TEST_ASSERT(cellArray->InsertNextCell({ 0, 1 }) == 0);
  TEST_ASSERT(cellArray->GetStorageType() == ExpectedExplicitStorage(use64Bit));
  TEST_ASSERT(cellArray->IsValid());
  ValidateCell(cellArray, 0, { 0, 1 });
}

void RunTests(bool use64Bit)
{
  TestInsertNextCellAutoConverts(use64Bit);
  TestInsertNextCellIdListAndCellAutoConvert(use64Bit);
  TestIncrementalInsertionAutoConverts(use64Bit);
  TestAppendAutoConverts(use64Bit);
  TestAppendLegacyFormatAutoConverts(use64Bit);
  TestEmptyFixedSizeAutoConverts(use64Bit);
}

} // end anonymous namespace

int TestCellArrayFixedSizeAutoConvert(int, char*[])
{
  try
  {
    ::RunTests(false); // FixedSizeInt32
    ::RunTests(true);  // FixedSizeInt64
  }
  catch (std::exception& err)
  {
    vtkLog(ERROR, << err.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
