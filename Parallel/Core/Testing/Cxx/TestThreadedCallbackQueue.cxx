/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreadedCallbackQueue.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThreadedCallbackQueue.h"

#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <atomic>

int TestThreadedCallbackQueue(int, char*[])
{
  vtkNew<vtkThreadedCallbackQueue> queue;
  queue->SetNumberOfThreads(2);
  queue->Start();
  std::atomic_int count(0);
  int N = 100000;

  // We are testing if the queue can properly resize itself and doesn't have deadlocks
  for (vtkIdType i = 0; i < N; ++i)
  {
    if (i == N / 2)
    {
      queue->Stop();
    }
    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    queue->Push(
      [&count](const int& n, int&, vtkIntArray* a1, vtkIntArray* a2) {
        a1->SetName(std::to_string(n).c_str());
        a2->SetName(std::to_string(n).c_str());
        ++count;
      },
      i, 0, vtkNew<vtkIntArray>(), array);
  }

  queue->SetNumberOfThreads(16);
  queue->Start();

  // If the jobs are not run, this test will do an infinite loop
  while (count != N)
    ;

  return EXIT_SUCCESS;
}
