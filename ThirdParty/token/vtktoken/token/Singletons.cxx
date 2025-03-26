#include "token/Singletons.h"

#include <cstdlib>
#include <mutex>

token_BEGIN_NAMESPACE
namespace {

TypeContainer* s_singletons = nullptr;
std::mutex s_singletonsMutex;

} // anonymous namespace

TypeContainer& singletons()
{
  std::lock_guard<std::mutex> guard(s_singletonsMutex);
  if (!s_singletons)
  {
    s_singletons = new TypeContainer;
    atexit(finalizeSingletons);
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
