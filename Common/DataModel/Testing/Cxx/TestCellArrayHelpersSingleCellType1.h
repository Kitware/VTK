// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkImplicitArray.h"
#include "vtkLogger.h"

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
      std::cout << "false\n";                                                                      \
      ThrowAssertError(vtkQuoteMacro(__FILE__) ":" vtkQuoteMacro(                                  \
        __LINE__) ": test assertion failed: (" #cond ")");                                         \
    }                                                                                              \
    std::cout << "true\n";                                                                         \
  } while (false)

template <typename ConnectivityArrayType>
void TestSetDataSingleCellType(vtkSmartPointer<vtkCellArray> cellArray)
{
  vtkLogScopeFunction(INFO);
  // Offsets     : 0 3 6
  // Connectivity: 0 1 2 0 2 3
  vtkNew<ConnectivityArrayType> connectivity;
  // tri 0: 0 1 2
  connectivity->InsertNextValue(0);
  connectivity->InsertNextValue(1);
  connectivity->InsertNextValue(2);
  // tri 1: 0 2 3
  connectivity->InsertNextValue(0);
  connectivity->InsertNextValue(2);
  connectivity->InsertNextValue(3);
  cellArray->SetData(/*cellSize=*/3, connectivity);
  // cellArray->PrintDebug(std::cout);

  TEST_ASSERT(cellArray->GetNumberOfCells() == 2);
  TEST_ASSERT(cellArray->GetNumberOfConnectivityIds() == 6);
  TEST_ASSERT(cellArray->GetNumberOfOffsets() == 3);
  {
    vtkIdType npts;
    const vtkIdType* pts;

    auto iter = vtk::TakeSmartPointer(cellArray->NewIterator());
    TEST_ASSERT(!iter->IsDoneWithTraversal());
    iter->GoToFirstCell();

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
    TEST_ASSERT(pts[0] == 0);
    TEST_ASSERT(pts[1] == 2);
    TEST_ASSERT(pts[2] == 3);
    iter->GoToNextCell();

    TEST_ASSERT(iter->IsDoneWithTraversal());
  }
}
}
