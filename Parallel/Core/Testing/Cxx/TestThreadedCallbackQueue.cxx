// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkThreadedCallbackQueue.h"

#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace
{
//-----------------------------------------------------------------------------
void RunThreads(int nthreadsBegin, int nthreadsEnd)
{
  vtkNew<vtkThreadedCallbackQueue> queue;
  std::atomic_int count(0);
  int N = 10000;

  // Spamming controls
  for (int i = 0; i < 6; ++i)
  {
    queue->SetNumberOfThreads(nthreadsBegin);
    queue->SetNumberOfThreads(nthreadsEnd);
  }

  // We are testing if the queue can properly resize itself and doesn't have deadlocks
  for (vtkIdType i = 0; i < N; ++i)
  {
    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    queue->Push(
      [&count](const int& n, const double&&, char, vtkIntArray* a1, vtkIntArray* a2) {
        a1->SetName(std::to_string(n).c_str());
        a2->SetName(std::to_string(n).c_str());
        ++count;
      },
      i, 0, 'a', vtkNew<vtkIntArray>(), array);
  }

  // If the jobs are not run, this test will do an infinite loop
  while (count != N)
    ;

  // Checking how the queue behaves when being destroyed.
  queue->SetNumberOfThreads(nthreadsBegin);
  queue->SetNumberOfThreads(nthreadsEnd);
}

//=============================================================================
struct A
{
  A() { vtkLog(INFO, "Constructor"); }
  A(A&& other)
  noexcept
    : array(std::move(other.array))
    , val(other.val)
  {
    vtkLog(INFO, "Move constructor");
  }
  A(const A& other)
    : array(other.array)
    , val(other.val)
  {
    vtkLog(INFO, "Copy constructor called.");
  }
  void f(A&, A&&) {}
  void const_f(A&, A&&) const {}
  void operator()(A&, A&&) { std::cout << *array << std::endl; }
  int& get() { return val; }

  vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
  int val = 0;
};

//-----------------------------------------------------------------------------
void f(A&, A&&) {}

//-----------------------------------------------------------------------------
bool TestFunctionTypeCompleteness()
{
  // We create a queue outside of the score where things are pushed to ensure that the pushed
  // objects are persistent.
  vtkNew<vtkThreadedCallbackQueue> queue;
  {
    // Testing the queue on some exotic inputs

    // lambdas
    queue->Push([](A&&) {}, ::A());
    queue->Push([](::A&, const ::A&, ::A&&, const ::A&&) {}, ::A(), ::A(), ::A(), ::A());

    // member function pointers
    queue->Push(&::A::f, ::A(), ::A(), ::A());
    queue->Push(&::A::const_f, ::A(), ::A(), ::A());

    std::shared_ptr<A> persistentA = std::make_shared<A>();

    // Fetching an lvalue reference return type
    auto future = queue->Push(&::A::get, persistentA);

    // functor
    queue->Push(::A(), ::A(), ::A());

    // function pointer
    queue->Push(&::f, ::A(), ::A());

    // Passing an lvalue reference, which needs to be copied.
    A a;
    queue->Push(a, ::A(), ::A());

    // Passing a pointer wrapped functor
    queue->Push(std::unique_ptr<A>(new ::A()), ::A(), ::A());

    // Passing a pointer wrapped object with a member function pointer
    queue->Push(&::A::f, std::unique_ptr<A>(new ::A()), ::A(), ::A());

    // Passing a std::function
    std::function<void(::A&, ::A &&)> func = f;
    queue->Push(func, ::A(), ::A());

    // Testing lvalue reference return type behavior
    int& val = queue->Get(future);
    if (&val != &persistentA->val)
    {
      vtkLog(ERROR, "lvalue reference was not correctly passed through the queue.");
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool TestSharedFutures()
{
  int N = 10;
  bool retVal = true;
  while (--N && retVal)
  {
    vtkNew<vtkThreadedCallbackQueue> queue;
    queue->SetNumberOfThreads(4);

    std::atomic_int count(0);
    std::mutex mutex;

    auto f = [&count, &mutex](std::string& s, int low) {
      std::unique_lock<std::mutex> lock(mutex);
      if (count++ < low)
      {
        vtkLog(ERROR,
          "Task " << s.c_str() << " started too early, in " << count << "th position"
                  << " instead of " << low + 1 << "th.");
        return false;
      }
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      return true;
    };

    using Array = std::vector<vtkThreadedCallbackQueue::SharedFuturePointer<bool>>;

    int n = 10;

    Array futures;

    auto future1 = queue->Push(f, "t1", 0);
    auto future2 = queue->PushDependent(Array{ future1 }, f, "t2", 1);
    auto future3 = queue->PushDependent(Array{ future1, future2 }, f, "t3", 2);
    // These pushes makes the scenario where future2 and future4 are ready to run but have a higher
    // future id than them. SharedFuture2 and future4 will need to wait here and we're ensuring
    // everything goes well.
    for (int i = 0; i < n; ++i)
    {
      futures.emplace_back(queue->Push(f, "spam", 0));
    }
    auto fastFuture = queue->Push(f, "spam", 0);
    auto future4 = queue->PushDependent(Array{ future2 }, f, "t4", 3);
    auto future5 = queue->PushDependent(Array{ future3, future4 }, f, "t5", 4);
    auto future6 = queue->Push(f, "t6", 0);

    futures.emplace_back(future1);
    futures.emplace_back(future2);
    futures.emplace_back(future3);
    futures.emplace_back(future4);
    futures.emplace_back(future5);
    futures.emplace_back(future6);

    // Testing the case where Wait executes the task associated with a function that wasn't invoked
    // yet.
    queue->Wait(Array{ fastFuture });

    // Testing all other scenarios in Wait
    queue->Wait(futures);

    for (auto& future : futures)
    {
      retVal &= queue->Get(future);
    }
  }

  return retVal;
}
} // anonymous namespace

int TestThreadedCallbackQueue(int, char*[])
{
  vtkLog(INFO, "Testing futures");
  bool retVal = ::TestSharedFutures();

  retVal &= ::TestFunctionTypeCompleteness();

  vtkLog(INFO, "Testing expanding from 2 to 8 threads");
  // Testing expanding the number of threads
  ::RunThreads(2, 8);

  vtkLog(INFO, "Testing shrinking from 8 to 2 threads");
  // Testing shrinking the number of threads
  ::RunThreads(8, 2);

  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
