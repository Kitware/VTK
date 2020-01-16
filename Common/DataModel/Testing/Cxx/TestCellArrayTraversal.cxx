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
#include "vtkLogger.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

namespace
{

void RunTest(bool use64BitStorage)
{
  const vtkIdType numTris = 25000;
  vtkIdType num;

  auto ca = vtkSmartPointer<vtkCellArray>::New();
  if (use64BitStorage)
  {
    cout << "\n=== Test performance of new vtkCellArray: 64-bit storage ===\n";
    ca->Use64BitStorage();
  }
  else
  {
    cout << "\n=== Test performance of new vtkCellArray: 32-bit storage ===\n";
    ca->Use32BitStorage();
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
  cout << "Traverse cell array (new GetCellAtId()): " << timer->GetElapsedTime() << "\n";
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

  RunTest(false); // 32-bit
  RunTest(true);  // 64-bit
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
