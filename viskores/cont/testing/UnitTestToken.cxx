//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Token.h>

#include <viskores/cont/testing/Testing.h>

#include <chrono>
#include <future>
#include <thread>

namespace
{

struct TestObject
{
  viskores::cont::Token::ReferenceCount* TokenCount;
  std::mutex* Mutex;
  std::condition_variable* ConditionVariable;
  viskores::Id* ReferenceCount;

  TestObject()
    : TokenCount(new viskores::cont::Token::ReferenceCount(0))
    , Mutex(new std::mutex)
    , ConditionVariable(new std::condition_variable)
    , ReferenceCount(new viskores::Id(1))
  {
  }

  TestObject(const TestObject& src)
    : TokenCount(src.TokenCount)
    , Mutex(src.Mutex)
    , ConditionVariable(src.ConditionVariable)
    , ReferenceCount(src.ReferenceCount)
  {
    std::unique_lock<std::mutex> lock(*this->Mutex);
    (*this->ReferenceCount)++;
  }

  ~TestObject()
  {
    std::unique_lock<std::mutex> lock(*this->Mutex);
    (*this->ReferenceCount)--;
    if (*this->ReferenceCount == 0)
    {
      lock.unlock();
      delete this->TokenCount;
      delete this->Mutex;
      delete this->ConditionVariable;
      delete this->ReferenceCount;
    }
  }

  void Attach(viskores::cont::Token& token)
  {
    token.Attach(*this, this->TokenCount, this->Mutex, this->ConditionVariable);
  }
};

#define CHECK_OBJECT(object, expectedTokens, expectedRefs)            \
  VISKORES_TEST_ASSERT(*(object).TokenCount == (expectedTokens),      \
                       "Expected object to have token count of ",     \
                       (expectedTokens),                              \
                       ". It actually was ",                          \
                       *(object).TokenCount);                         \
  VISKORES_TEST_ASSERT(*(object).ReferenceCount == (expectedRefs),    \
                       "Expected object to have reference count of ", \
                       (expectedRefs),                                \
                       ". It actually was ",                          \
                       *(object).ReferenceCount)

void TestBasicAttachDetatch()
{
  std::cout << "Test basic attach detach." << std::endl;

  std::cout << "  Create objects" << std::endl;
  TestObject object1;
  TestObject object2;
  TestObject object3;

  CHECK_OBJECT(object1, 0, 1);
  CHECK_OBJECT(object2, 0, 1);
  CHECK_OBJECT(object3, 0, 1);

  std::cout << "  Create outer token" << std::endl;
  viskores::cont::Token outerToken;

  std::cout << "  Attach outer token" << std::endl;
  object1.Attach(outerToken);
  object2.Attach(outerToken);
  object3.Attach(outerToken);

  CHECK_OBJECT(object1, 1, 2);
  CHECK_OBJECT(object2, 1, 2);
  CHECK_OBJECT(object3, 1, 2);

  {
    std::cout << "  Create/Attach inner token" << std::endl;
    viskores::cont::Token innerToken;
    object1.Attach(innerToken);
    object2.Attach(innerToken);
    object3.Attach(innerToken);

    CHECK_OBJECT(object1, 2, 3);
    CHECK_OBJECT(object2, 2, 3);
    CHECK_OBJECT(object3, 2, 3);

    std::cout << "  Recursively attach outer token" << std::endl;
    object1.Attach(outerToken);

    CHECK_OBJECT(object1, 2, 3);
    CHECK_OBJECT(object2, 2, 3);
    CHECK_OBJECT(object3, 2, 3);

    std::cout << "  Detach from inner token (through scoping)" << std::endl;
  }
  CHECK_OBJECT(object1, 1, 2);
  CHECK_OBJECT(object2, 1, 2);
  CHECK_OBJECT(object3, 1, 2);

  std::cout << "  Detatch outer token" << std::endl;
  outerToken.DetachFromAll();

  CHECK_OBJECT(object1, 0, 1);
  CHECK_OBJECT(object2, 0, 1);
  CHECK_OBJECT(object3, 0, 1);
}

void WaitForDetachment(TestObject* object)
{
  std::unique_lock<std::mutex> lock(*object->Mutex);
  object->ConditionVariable->wait(lock, [object] { return *object->TokenCount < 1; });
  std::cout << "  Thread woke up" << std::endl;
}

void TestThreadWake()
{
  std::cout << "Testing thread wakeup" << std::endl;

  TestObject object;
  CHECK_OBJECT(object, 0, 1);

  viskores::cont::Token token;
  object.Attach(token);
  CHECK_OBJECT(object, 1, 2);

  std::cout << "  Launching coordinated thread" << std::endl;
  auto future = std::async(std::launch::async, WaitForDetachment, &object);

  std::cout << "  Sleep 500 milliseconds for thread to lock" << std::endl;
  // 500 milliseconds should be ample time for the spawned thread to launch. If the systems busy then
  // we might actually unlock the object before the thread gets there, but hopefully on some
  // systems it will test correctly.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "  Main thread woke up. Detach token." << std::endl;
  token.DetachFromAll();

  std::cout << "  Wait for thread to finish. Could deadlock if did not properly wake." << std::endl;
  future.get();

  std::cout << "  Returned to main thread" << std::endl;
  CHECK_OBJECT(object, 0, 1);
}

void DoTest()
{
  TestBasicAttachDetatch();
  TestThreadWake();
}

} // anonymous namespace

int UnitTestToken(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
