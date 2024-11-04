#include "token/Singletons.h"

#include <mutex>

token_BEGIN_NAMESPACE
namespace {

// rely on default initialization to 0 for static variables
unsigned int s_singletonsCleanupCounter;
TypeContainer* s_singletons;
std::mutex* s_singletonsMutex;

} // anonymous namespace

namespace detail {

singletonsCleanup::singletonsCleanup()
{
  if (++s_singletonsCleanupCounter == 1)
  {
    s_singletons = nullptr;
    s_singletonsMutex = new std::mutex;
  }
}

singletonsCleanup::~singletonsCleanup()
{
  if (--s_singletonsCleanupCounter == 0)
  {
    finalizeSingletons();
    delete s_singletonsMutex;
  }
}

} // namespace detail

TypeContainer& singletons()
{
  std::lock_guard<std::mutex> guard(*s_singletonsMutex);
  if (!s_singletons)
  {
    s_singletons = new TypeContainer;
  }
  return *s_singletons;
}

void finalizeSingletons()
{
  // Per @michael.migliore this can cause exceptions on macos in cases
  // when the mutex is destroyed before finalizeSingletons() is called:
  // std::lock_guard<std::mutex> guard(s_singletonsMutex);

  delete s_singletons;
  s_singletons = nullptr;
}

token_CLOSE_NAMESPACE
