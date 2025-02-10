#include <windows.h>

namespace diy
{

namespace coroutine
{

/**********************
 * "Global varialbes" *
 **********************/
inline cothread_t& co_active_()
{
    static thread_local cothread_t x = 0;
    return x;
}
/**********************/

static void __stdcall co_thunk(void* coentry) {
  ((void (*)(void))coentry)();
}

cothread_t co_active() {
  if(!co_active_()) {
    ConvertThreadToFiber(0);
    co_active_() = GetCurrentFiber();
  }
  return co_active_();
}

cothread_t co_create(unsigned int heapsize, void (*coentry)(void)) {
  if(!co_active_()) {
    ConvertThreadToFiber(0);
    co_active_() = GetCurrentFiber();
  }
  return (cothread_t)CreateFiber(heapsize, co_thunk, (void*)coentry);
}

void co_delete(cothread_t cothread) {
  DeleteFiber(cothread);
}

void co_switch(cothread_t cothread) {
  co_active_() = cothread;
  SwitchToFiber(cothread);
}

}

}
