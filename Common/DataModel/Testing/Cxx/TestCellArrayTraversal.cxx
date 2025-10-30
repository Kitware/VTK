// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkTypeUInt8Array.h"

namespace
{

void RunTest(vtkCellArray::StorageTypes storageType)
{
  constexpr vtkIdType numTris = 500000;
  vtkIdType num;

  auto ca = vtkSmartPointer<vtkCellArray>::New();
  switch (storageType)
  {
    case vtkCellArray::Int32:
    {
      cout << "\n=== Test performance of new vtkCellArray: 32-bit storage ===\n";
      ca->Use32BitStorage();
      break;
    }
    case vtkCellArray::Int64:
    {
      cout << "\n=== Test performance of new vtkCellArray: 64-bit storage ===\n";
      ca->Use64BitStorage();
      break;
    }
    case vtkCellArray::Generic:
    default:
    {
      cout << "\n=== Test performance of new vtkCellArray: generic storage ===\n";
      // By passing array types which are NOT in vtkArrayDispatch::InputConnectivityArrays,
      // vtkCellArray can be put into the "Generic" storage mode.
      vtkNew<vtkTypeUInt8Array> placeholderConn;
      vtkNew<vtkAffineArray<vtkIdType>> offsets;
      offsets->ConstructBackend(/*cellSize=*/3, 0);
      offsets->InsertNextValue(0); // initialize the offsets array with one element
      ca->SetData(offsets, placeholderConn);
      break;
    }
  }
  vtkIdType tri[3] = { 0, 1, 2 };
  auto timer = vtkSmartPointer<vtkTimerLog>::New();

  vtkIdType npts;
  const vtkIdType* pts;

  // Insert
  num = 0;
  timer->StartTimer();
  for (auto i = 0; i < numTris; ++i)
  {
    ca->InsertNextCell(3, tri);
    ++num;
  }
  timer->StopTimer();
  cout << "Insert triangles: " << timer->GetElapsedTime() << "\n";
  cout << "   " << num << " triangles inserted\n";
  cout << "   Memory used: " << ca->GetActualMemorySize() << " kb\n";

  // Iterate directly over cell array
  num = 0;
  timer->StartTimer();
  for (ca->InitTraversal(); ca->GetNextCell(npts, pts);)
  {
    assert(npts == 3);
    ++num;
  }
  timer->StopTimer();
  cout << "Traverse cell array (legacy GetNextCell()): " << timer->GetElapsedTime() << "\n";
  cout << "   " << num << " triangles visited\n";

  // Iterate directly over cell array
  num = 0;
  timer->StartTimer();
  vtkIdType numCells = ca->GetNumberOfCells();
  for (auto cellId = 0; cellId < numCells; ++cellId)
  {
    ca->GetCellAtId(cellId, npts, pts);
    assert(npts == 3);
    ++num;
  }
  timer->StopTimer();
  cout << "Traverse cell array (new GetCellAtId(vtkIdType, vtkIdType&, vtkIdType const*&)): "
       << timer->GetElapsedTime() << "\n";
  cout << "   " << num << " triangles visited\n";

  // Iterate directly over cell array such that point ids are copied
  num = 0;
  timer->StartTimer();
  vtkNew<vtkIdList> ptIds;
  for (auto cellId = 0; cellId < numCells; ++cellId)
  {
    ca->GetCellAtId(cellId, ptIds);
    assert(ptIds->GetNumberOfIds() == 3);
    assert(ptIds->GetId(0) == 0);
    assert(ptIds->GetId(1) == 1);
    assert(ptIds->GetId(2) == 2);
    ++num;
  }
  timer->StopTimer();
  cout << "Traverse cell array (new GetCellAtId(vtkIdType, vtkIdList*)): "
       << timer->GetElapsedTime() << "\n";
  cout << "   " << num << " triangles visited\n";

  // Iterate using iterator
  num = 0;
  timer->StartTimer();
  auto iter = vtk::TakeSmartPointer(ca->NewIterator());
  for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
  {
    iter->GetCurrentCell(npts, pts);
    assert(npts == 3);
    ++num;
  }
  timer->StopTimer();
  cout << "Iterator traversal: " << timer->GetElapsedTime() << "\n";
  cout << "   " << num << " triangles visited\n";
} // RunTest

void RunTests()
{
  // What is the size of vtkIdType?
  cout << "=== vtkIdType is: " << (sizeof(vtkIdType) * 8) << " bits ===\n";

  RunTest(vtkCellArray::StorageTypes::Int32); // 32-bit
  RunTest(vtkCellArray::StorageTypes::Int64); // 64-bit
  // For generic storage, we make offsets an implicit array to support constant cell size
  // and connectivity will be a vtkTypeUInt8Array because we know that point ID never exceeds 3.
  // With this trick, memory used will be 20% of the total memory used by 32-bit storage and only
  // 10% of 64-bit scheme. However, there is a slight performance penalty.
  RunTest(vtkCellArray::StorageTypes::Generic);
}

} // end anon namespace

int TestCellArrayTraversal(int, char*[])
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
