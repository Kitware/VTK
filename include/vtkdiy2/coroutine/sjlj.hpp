/*
  note this was designed for UNIX systems. Based on ideas expressed in a paper by Ralf Engelschall.
  for SJLJ on other systems, one would want to rewrite springboard() and co_create() and hack the jmb_buf stack pointer.
*/

#if __USE_FORTIFY_LEVEL
#define DIY_USE_FORTIFY_LEVEL __USE_FORTIFY_LEVEL
#undef __USE_FORTIFY_LEVEL
#warning "diy::coroutine (sjlj.hpp) cannot be compiled with _FORTIFY_SOURCE; disabling."
#endif


//#define _BSD_SOURCE
//#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

namespace diy
{

namespace coroutine
{

struct cothread_struct
{
  sigjmp_buf context;
  void (*coentry)(void);
  void* stack;
};

/**********************
 * "Global varialbes" *
 **********************/
inline cothread_struct& co_primary()
{
    static thread_local cothread_struct x;
    return x;
}

inline cothread_struct*& creating()
{
    static thread_local cothread_struct* x;
    return x;
}

inline cothread_struct*& co_running()
{
    static thread_local cothread_struct* x = 0;
    return x;
}
/**********************/

static void springboard(int) {
  if(sigsetjmp(creating()->context, 0)) {
    co_running()->coentry();
  }
}

cothread_t co_active() {
  if(!co_running()) co_running() = &co_primary();
  return (cothread_t)co_running();
}

cothread_t co_create(unsigned int size, void (*coentry)(void)) {
  if(!co_running()) co_running() = &co_primary();

  cothread_struct* thread = (cothread_struct*)malloc(sizeof(cothread_struct));
  if(thread) {
    struct sigaction handler;
    struct sigaction old_handler;

    stack_t stack;
    stack_t old_stack;

    thread->coentry = 0;
    thread->stack = 0;

    stack.ss_flags = 0;
    stack.ss_size = size;
    thread->stack = stack.ss_sp = malloc(size);
    if(stack.ss_sp && !sigaltstack(&stack, &old_stack)) {
      handler.sa_handler = springboard;
      handler.sa_flags = SA_ONSTACK;
      sigemptyset(&handler.sa_mask);
      creating() = thread;

      if(!sigaction(SIGUSR1, &handler, &old_handler)) {
        if(!raise(SIGUSR1)) {
          thread->coentry = coentry;
        }
        sigaltstack(&old_stack, 0);
        sigaction(SIGUSR1, &old_handler, 0);
      }
    }

    if(thread->coentry != coentry) {
      co_delete(thread);
      thread = 0;
    }
  }

  return (cothread_t)thread;
}

void co_delete(cothread_t cothread) {
  if(cothread) {
    if(((cothread_struct*)cothread)->stack) {
      free(((cothread_struct*)cothread)->stack);
    }
    free(cothread);
  }
}

void co_switch(cothread_t cothread) {
  if(!sigsetjmp(co_running()->context, 0)) {
    co_running() = (cothread_struct*)cothread;
    siglongjmp(co_running()->context, 1);
  }
}

}

}

#if DIY_USE_FORTIFY_LEVEL
#define __USE_FORTIFY_LEVEL DIY_USE_FORTIFY_LEVEL
#undef DIY_USE_FORTIFY_LEVEL
#endif
