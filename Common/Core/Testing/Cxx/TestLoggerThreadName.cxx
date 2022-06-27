/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLoggerThreadName.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Test that vtkLogger::GetThreadName is unaffected by concurrent accesses
// and usage of vtkLogger::Init()

#include "vtkLogger.h"

#include <atomic>
#include <string>
#include <thread>

// Control the order of operations between the threads
std::atomic_bool wait1;
std::atomic_bool wait2;

void Thread1()
{
  const std::string threaName = "T1";
  while (!wait1.load())
  {
  }

  vtkLogger::SetThreadName(threaName);

  wait2.store(true);

  if (vtkLogger::GetThreadName() != threaName)
  {
    vtkLogF(ERROR, "Name mismatch !");
  }
}

void Thread2()
{
  const std::string threaName = "T2";
  vtkLogger::SetThreadName(threaName);

  wait1.store(true);
  while (!wait2.load())
  {
  }

  vtkLogger::Init();

  if (vtkLogger::GetThreadName() != threaName)
  {
    vtkLogF(ERROR, "Name mismatch !");
  }
}

int TestLoggerThreadName(int, char*[])
{
  wait1.store(false);
  wait2.store(false);
  std::thread t1(Thread1);
  std::thread t2(Thread2);

  t1.join();
  t2.join();
  return EXIT_SUCCESS;
}
