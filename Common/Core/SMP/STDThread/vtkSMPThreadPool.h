/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSMPThreadPool.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPThreadPool - A thread pool implementation using std::thread
//
// .SECTION Description
// vtkSMPThreadPool class creates a thread pool of std::thread, the number
// of thread must be specified at the initialization of the class.
// The DoJob() method is used attributes the job to a free thread, if all
// threads are working, the job is kept in a queue. Note that vtkSMPThreadPool
// destructor joins threads and finish the jobs in the queue.

#ifndef vtkSMPThreadPool_h
#define vtkSMPThreadPool_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

#include <condition_variable> // For std::condition_variable
#include <functional>         // For std::function
#include <mutex>              // For std::mutex
#include <queue>              // For std::queue
#include <thread>             // For std::thread

namespace vtk
{
namespace detail
{
namespace smp
{

class VTKCOMMONCORE_EXPORT vtkSMPThreadPool
{
public:
  explicit vtkSMPThreadPool(int ThreadNumber);

  void Join();
  void DoJob(std::function<void(void)> job);

private:
  void ThreadJob();

private:
  std::mutex Mutex;
  bool Joining = false;
  std::condition_variable ConditionVariable;
  std::queue<std::function<void(void)>> JobQueue;
  std::vector<std::thread> Threads;
};

} // namespace smp
} // namespace detail
} // namespace vtk

#endif
